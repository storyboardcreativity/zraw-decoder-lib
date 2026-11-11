#pragma once

#include <stdexcept>

#include "BitReader.hpp"
#include "ZRawImageBlockLine.hpp"

// Step 1: reads the values of one image line. A line is a series of blocks of 32 pairs of values
// (components A and B). Call ReadBlockHeader() and ReadPairs() for each block, then EndLine().
// Raw blocks hold pixel values, VL blocks hold codes of the difference to the predicted pixel.
class ZRawImageLineBinaryParser
{
public:
    struct Parameters
    {
        int max_allowed_raw_value;
        bool stride;
        int align_mode;
        bool lossless;
        int bitdepth_real;
    };

    ZRawImageLineBinaryParser(Parameters param)
        : g_a(4), g_b(4), _read_values_count(0), _block_is_raw(false),
          _bitdepth_diff(0), _param(param), _huffman(_huffmanTable()) {}

    // Header value and mode of the block are stored at block_index
    void ReadBlockHeader(BitReader *reader, int block_index, uint32_t* header_values, int* is_block_vl)
    {
        uint64_t decoding_mode = _readBlockHeader(reader);

        header_values[block_index] = _bitdepth_diff;
        is_block_vl[block_index] = decoding_mode != 0x100000001;

        _block_is_raw = decoding_mode == 0x100000001;
    }

    // Reads count pairs of the current block and calls handler(value_a, value_b) for each.
    // The reading state is kept in locals so the handler can be inlined.
    template <class PairHandler>
    void ReadPairs(BitReader *reader, int count, PairHandler handler)
    {
        uint64_t position = reader->CurrentPositionInBits();

        // Near the end of the data the reads need bounds checks.
        // A pair is at most two 9-bit prefix codes and two full size values.
        bool is_inside = reader->AreBitsInside(position, (uint64_t)count * (18 + 2 * _param.bitdepth_real));

        if (_block_is_raw)
        {
            if (is_inside)
                _readPairsRawMode<false>(reader, position, count, handler);
            else
                _readPairsRawMode<true>(reader, position, count, handler);
        }
        else
        {
            if (is_inside)
                _readPairsVariableLengthMode<false>(reader, position, count, handler);
            else
                _readPairsVariableLengthMode<true>(reader, position, count, handler);
        }

        reader->SeekToBit(position);
        _read_values_count += count;
    }

    void EndLine(BitReader *reader)
    {
        // Alignment after last block
        if (_param.stride)
            _alignLastBlock(reader);
    }

protected:

    template <bool kCheckEnd>
    ZRAW_FORCE_INLINE uint64_t _peek(BitReader *reader, uint64_t position)
    {
        return kCheckEnd ? reader->PeekAt(position) : reader->PeekAtInside(position);
    }

    template <bool kCheckEnd, class PairHandler>
    void _readPairsRawMode(BitReader *reader, uint64_t& position_inout, int count, PairHandler handler)
    {
        uint64_t position = position_inout;
        const int bitdepth_diff = _bitdepth_diff;
        const int default_lsb_size = _param.bitdepth_real - bitdepth_diff;
        const uint64_t mask = (1ull << default_lsb_size) - 1;

        for (int i = 0; i < count; ++i)
        {
            int32_t value_a = (_peek<kCheckEnd>(reader, position) & mask) << bitdepth_diff;
            position += default_lsb_size;
            int32_t value_b = (_peek<kCheckEnd>(reader, position) & mask) << bitdepth_diff;
            position += default_lsb_size;

            handler(value_a, value_b);
        }

        position_inout = position;
    }

    template <bool kCheckEnd, class PairHandler>
    void _readPairsVariableLengthMode(BitReader *reader, uint64_t& position_inout, int count, PairHandler handler)
    {
        uint64_t position = position_inout;
        const int default_lsb_size = _param.bitdepth_real - _bitdepth_diff;

        int ga = g_a, gb = g_b;
        const int max_allowed_raw_value = _param.max_allowed_raw_value;
        const HuffmanTable& huffman = _huffman;

        for (int i = 0; i < count; ++i)
        {
            int a = _getValueBitSizeMinus1_ButMax6(ga);
            int b = _getValueBitSizeMinus1_ButMax6(gb);

            // Pre-read data (the first 48 bits)
            uint64_t data = _peek<kCheckEnd>(reader, position) & 0xFFFFFFFFFFFFull;

            // Read component A most significant bits
            uint32_t size_in_bits_of_msb_a = huffman.size[data & 0x1FF];
            uint32_t msb_a = huffman.value[data & 0x1FF];
            data >>= size_in_bits_of_msb_a;

            // Read component B most significant bits
            uint32_t size_in_bits_of_msb_b = huffman.size[data & 0x1FF];
            uint32_t msb_b = huffman.value[data & 0x1FF];
            data >>= size_in_bits_of_msb_b;

            // Read component A least significant bits
            uint32_t lsb_a_size = msb_a == 12 ? default_lsb_size : a;
            uint32_t lsb_a = data & ((1ull << lsb_a_size) - 1);
            data >>= lsb_a_size;

            // Read component B least significant bits
            uint32_t lsb_b_size = msb_b == 12 ? default_lsb_size : b;
            uint32_t lsb_b = data & ((1ull << lsb_b_size) - 1);

            position += size_in_bits_of_msb_a + size_in_bits_of_msb_b + lsb_a_size + lsb_b_size;

            // Construct component values
            int value_a = msb_a == 12 ? lsb_a + 1 : (msb_a << lsb_a_size) | lsb_a;
            int value_b = msb_b == 12 ? lsb_b + 1 : (msb_b << lsb_b_size) | lsb_b;

            handler(value_a, value_b);

            // ===

            value_a -= (value_a >> a) > 11;
            value_a = _roundValue(0, value_a, max_allowed_raw_value);

            // Calculate block context next value
            ga = (2 * value_a + 2 * ga + 2) / 4;

            // ===

            value_b -= (value_b >> b) > 11;
            value_b = _roundValue(0, value_b, max_allowed_raw_value);

            // Calculate block context next value
            gb = (2 * value_b + 2 * gb + 2) / 4;
        }

        g_a = ga;
        g_b = gb;
        position_inout = position;
    }

    // Same as: for (i = 0; i < 6; ++i) if (!(value >> (i + 1))) break; return i;
    ZRAW_FORCE_INLINE int _getValueBitSizeMinus1_ButMax6(int value)
    {
        static const uint8_t sizes[65] =
        {
            0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
            5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
            5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
            6
        };
        return sizes[(unsigned)value > 64 ? 64 : value];
    }

    // _readHuffmanValue for every 9-bit prefix
    struct HuffmanTable
    {
        uint8_t value[512];
        uint8_t size[512];
    };

    static const HuffmanTable& _huffmanTable()
    {
        static const HuffmanTable table = _buildHuffmanTable();
        return table;
    }

    static HuffmanTable _buildHuffmanTable()
    {
        HuffmanTable table;
        for (uint32_t i = 0; i < 512; ++i)
        {
            uint32_t size_in_bits = 0;
            table.value[i] = _readHuffmanValue(i, size_in_bits);
            table.size[i] = size_in_bits;
        }
        return table;
    }

    static uint32_t _readHuffmanValue(uint32_t data_in, uint32_t& size_in_bits_out)
    {
        int i = 0;
        for (; i < 9; ++i)
        {
            if (data_in & 1)
                break;
            data_in >>= 1;
        }

        switch (i)
        {
        case 0:
            size_in_bits_out = 1;
            return 0;

        case 1:
            size_in_bits_out = 2;
            return 1;

        case 2:
            size_in_bits_out = 3;
            return 2;

        case 3:
            size_in_bits_out = 4;
            return 3;

        case 4:
            size_in_bits_out = 5;
            return 4;

        case 5:
            size_in_bits_out = 7;
            return ((data_in & 3) == 1 ? 5 : 6);

        case 6:
            size_in_bits_out = 8;
            return ((data_in & 3) == 1 ? 7 : 8);

        case 7:
            size_in_bits_out = 9;
            return ((data_in & 3) == 1 ? 11 : 12);

        case 8:
            size_in_bits_out = 9;
            return 10;

        default:
            break;
        }

        // default (000000000)
        size_in_bits_out = 9;
        return 9;
    }

    void _alignLastBlock(BitReader *reader)
    {
        reader->BitAlignTo(_param.align_mode == 1 ? 256 : 128);
    }

    ZRAW_FORCE_INLINE int _roundValue(int left, int value, int right)
    {
        if (value > right)
            value = right;
        if (value < left)
            value = left;
        return value;
    }

    uint64_t _readBlockHeader(BitReader *reader)
    {
        if (_param.lossless)
            _bitdepth_diff = 0;
        else if (_read_values_count > 0)
        {
            // Unknown flag
            auto flag0 = reader->ReadBits(1);
            if (flag0)
            {
                // Unknown value
                switch (reader->ReadBits(2))
                {
                case 0:
                    _bitdepth_diff = _bitdepth_diff - 2;
                    break;
                case 1:
                    _bitdepth_diff = _bitdepth_diff - 1;
                    break;
                case 2:
                    _bitdepth_diff = _bitdepth_diff + 1;
                    break;
                case 3:
                    _bitdepth_diff = _bitdepth_diff + 2;
                    break;
                }
            }
            //else
            //    _bitdepth_diff = _bitdepth_diff; // (no changes)
        }
        else
            _bitdepth_diff = reader->ReadBits(4);

        // The shifts need 0 <= diff <= bitdepth (only damaged data gets here)
        if (_bitdepth_diff < 0 || _bitdepth_diff > _param.bitdepth_real)
            throw std::runtime_error("ZRAW block header: bit depth difference is out of range");

        // Read block decoding mode
        return reader->ReadBits(1) ? 0x100000001 : 0x0;
    }

    int g_a, g_b;

    int _read_values_count;
    bool _block_is_raw;

    int _bitdepth_diff;

    Parameters _param;
    const HuffmanTable& _huffman;
};

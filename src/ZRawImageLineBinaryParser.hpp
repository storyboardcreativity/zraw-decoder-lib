#pragma once

#include <fstream>

#include "BitReader.hpp"
#include "ZRawImageBlockLine.hpp"

class ZRawImageLineBinaryParser
{
public:
    struct Parameters
    {
        int max_allowed_raw_value;
        int max_values_count;
        int blocks_count;
        bool stride;
        int align_mode;
        bool lossless;
        int bitdepth_real;

        uint16_t* dest;
    };

    ZRawImageLineBinaryParser(Parameters param)
        : _param(param), _read_values_count(0), _current_block_index(0),
          g_a(4), g_b(4) {}

    // Returns vector with blocks header values of the image line
    void Read(BitReader *reader, std::vector<uint32_t>& header_values, std::vector<int>& is_block_vl)
    {
        bool is_last_block_read = false;
        while (!is_last_block_read)
            is_last_block_read = ReadNext(reader, header_values, is_block_vl);
    }

    bool ReadNext(BitReader *reader, std::vector<uint32_t>& header_values, std::vector<int>& is_block_vl)
    {
        if (_current_block_index >= _param.blocks_count)
            return true;

        uint64_t decoding_mode = _readBlockHeader(reader);

        header_values.push_back(_bitdepth_diff);
        is_block_vl.push_back(decoding_mode != 0x100000001);

        if (decoding_mode == 0x100000001)
            _readNextBlockRawMode(reader);
        else
            _readNextBlockVariableLengthMode(reader);

        // Increase block counter
        ++_current_block_index;

        // If last block is just read
        if (_current_block_index >= _param.blocks_count)
        {
            // Alignment after last block
            if (_param.stride)
                _alignLastBlock(reader);

            // Last block is read
            return true;
        }

        // Not last block is read
        return false;
    }

protected:

    void _readNextBlockRawMode(BitReader *reader)
    {
        auto dest = _param.dest;
        for (int i = 0; i < ZRAW_LINE_BLOCK_SIZE && _read_values_count < _param.max_values_count; ++i, ++_read_values_count)
        {
            dest[_read_values_count * 2 + 0] = (reader->ReadBits(_param.bitdepth_real - _bitdepth_diff) << _bitdepth_diff);
            dest[_read_values_count * 2 + 1] = (reader->ReadBits(_param.bitdepth_real - _bitdepth_diff) << _bitdepth_diff);
        }
    }

    void _readNextBlockVariableLengthMode(BitReader *reader)
    {
        auto dest = _param.dest;
        for (int i = 0; i < ZRAW_LINE_BLOCK_SIZE && _read_values_count < _param.max_values_count; ++i, ++_read_values_count)
        {
            //std::fstream f_out("file_out_new.txt", std::ios::out | std::ios::app);

            int default_lsb_size = (_param.bitdepth_real - _bitdepth_diff);

            //f_out << "a.g = " << g_a << " b.g = " << g_b;

            int a = _getValueBitSizeMinus1_ButMax6(g_a);
            int b = _getValueBitSizeMinus1_ButMax6(g_b);

            //f_out << " a = " << a << " b = " << b;

            // Pre-read data
            uint64_t data = reader->ShowBits(48);

            // Read component A most significant bits
            uint32_t size_in_bits_of_msb_a = 0;
            uint32_t msb_a = _readHuffmanValue(data, size_in_bits_of_msb_a);
            data >>= size_in_bits_of_msb_a;

            // Read component B most significant bits
            uint32_t size_in_bits_of_msb_b = 0;
            uint32_t msb_b = _readHuffmanValue(data, size_in_bits_of_msb_b);
            data >>= size_in_bits_of_msb_b;

            //f_out << " msb_a = " << msb_a << " msb_b = " << msb_b << std::endl;

            // Read component A least significant bits
            uint32_t lsb_a_size = msb_a == 12 ? default_lsb_size : a;
            uint32_t lsb_a = 0;
            if (lsb_a_size > 0)
            {
                lsb_a = ((data << (64 - lsb_a_size)) >> (64 - lsb_a_size));
                data >>= lsb_a_size;
            }

            // Read component B least significant bits
            uint32_t lsb_b_size = msb_b == 12 ? default_lsb_size : b;
            uint32_t lsb_b = 0;
            if (lsb_b_size > 0)
            {
                lsb_b = ((data << (64 - lsb_b_size)) >> (64 - lsb_b_size));
                data >>= lsb_b_size;
            }

            reader->FlushBits(size_in_bits_of_msb_a + size_in_bits_of_msb_b + lsb_a_size + lsb_b_size);

            // Construct component values
            int value_a = msb_a == 12 ? lsb_a + 1 : (msb_a << lsb_a_size) | lsb_a;
            int value_b = msb_b == 12 ? lsb_b + 1 : (msb_b << lsb_b_size) | lsb_b;

            dest[_read_values_count * 2 + 0] = value_a;
            dest[_read_values_count * 2 + 1] = value_b;

            // ===

            if (value_a >> a > 11)
                --value_a;
            value_a = _roundValue(0, value_a, _param.max_allowed_raw_value);

            // Calculate block context next value
            g_a = (2 * value_a + 2 * g_a + 2) / 4;

            // ===

            if (value_b >> b > 11)
                --value_b;
            value_b = _roundValue(0, value_b, _param.max_allowed_raw_value);

            // Calculate block context next value
            g_b = (2 * value_b + 2 * g_b + 2) / 4;
        }
    }

    int _getValueBitSizeMinus1_ButMax6(int value)
    {
        int i = 0;
        for (; i < 6; ++i)
            if (!(value >> (i + 1)))
                break;
        return i;
    }

    uint32_t _readHuffmanValue(uint32_t data_in, uint32_t& size_in_bits_out)
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

    int _roundValue(int left, int value, int right)
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

        // Read block decoding mode
        return reader->ReadBits(1) ? 0x100000001 : 0x0;
    }

    int g_a, g_b;

    int _current_block_index;
    int _read_values_count;
    uint64_t _decoding_mode;

    int _bitdepth_diff;

    Parameters _param;
};
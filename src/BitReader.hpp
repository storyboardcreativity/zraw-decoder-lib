#pragma once

#include <istream>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <cstring>

class BitReader
{
public:
    BitReader(std::istream &stream, bool cached)
        : _stream(stream),
          _curr_bytes(0x00),
          _bits_left_in_curr_bytes(0),
          _position_in_bits(0),
          _cached_bytes_curr_pos(0),
          _cached(cached)
    {
        _stream.seekg(0, _stream.beg);

        if (cached)
        {
            _cached_bytes = std::vector<char>((std::istreambuf_iterator<char>(_stream)), std::istreambuf_iterator<char>());
            _cached_size = _cached_bytes.size();
        }
    }

    uint64_t ReadBits(uint32_t bit_count)
    {
        if (bit_count == 0)
            return 0;

        if (bit_count > sizeof(uint64_t) * 8)
            throw new std::invalid_argument("bit_count can't be more than 64!");

        return _getBits(bit_count);
    }

    void FlushBits(uint32_t bit_count)
    {
        // Fast variant if we have cached bits
        if (bit_count <= _bits_left_in_curr_bytes)
        {
            _bits_left_in_curr_bytes -= bit_count;
            _position_in_bits += bit_count;
            _curr_bytes >>= bit_count;
            return;
        }

        SeekToBit(CurrentPositionInBits() + bit_count);
    }

    void BitAlignTo(uint32_t value)
    {
        if (value == 0)
            return;

        uint32_t mod = _position_in_bits % value;
        if (mod)
            FlushBits(value - mod);
    }

    void SeekToBit(uint64_t bit_index)
    {
        uint64_t byte_index = bit_index / 8;
        if (_cached)
            _cached_bytes_curr_pos = byte_index;
        else
            _stream.seekg(byte_index, _stream.beg);

        // Reset counters and refresh current byte
        _bits_left_in_curr_bytes = 0;
        _position_in_bits = bit_index;

        // Here we know that (_bits_left_in_curr_bytes == 0) - use unsafe function
        _refreshCurrentByteUnsafe();

        // Skip last bits
        _curr_bytes >>= (bit_index % 8);
        _bits_left_in_curr_bytes -= (bit_index % 8);
    }

    uint64_t ShowBits(uint32_t bit_count)
    {
        // Fast variant if we have cached bits
        if (bit_count <= _bits_left_in_curr_bytes)
        {
            uint64_t res = _curr_bytes;
            res <<= (64 - bit_count);
            res >>= (64 - bit_count);
            return res;
        }

        // Save current position
        uint64_t bits_before = _position_in_bits;

        // Read bits
        uint64_t bits_to_show = ReadBits(bit_count);

        // Set carry back to saved position
        SeekToBit(bits_before);

        // Return result
        return bits_to_show;
    }

    uint64_t CurrentPositionInBits()
    {
        return _position_in_bits;
    }

private:
    void _refreshCurrentByte()
    {
        // If we have no bits left in current byte
        if (_bits_left_in_curr_bytes == 0)
            _refreshCurrentByteUnsafe();
    }

    void _refreshCurrentByteUnsafe()
    {
        // Read next byte
        if (_cached)
        {
            _curr_bytes = 0;
            int32_t left = (_cached_size - _cached_bytes_curr_pos);
            int32_t sz_to_read = left > ((int32_t)sizeof(_curr_bytes)) ? sizeof(_curr_bytes) : left;

            if (sz_to_read > 0)
                std::memcpy(&_curr_bytes, &_cached_bytes.data()[_cached_bytes_curr_pos], sz_to_read);

            _cached_bytes_curr_pos += sizeof(_curr_bytes);
        }
        else
            _stream.read((char *)&_curr_bytes, sizeof(_curr_bytes));

        // Refresh left bits counter
        _bits_left_in_curr_bytes = sizeof(_curr_bytes) * 8;
    }

    bool _getNextBit()
    {
        // Refresh current byte in case we have no bits left
        _refreshCurrentByte();

        // Get current bit
        bool result = _curr_bytes & 0x01;

        // Make next bit current
        _curr_bytes >>= 1;

        // Refresh bit counters
        --_bits_left_in_curr_bytes;
        ++_position_in_bits;

        // Return saved bit
        return result;
    }

    uint64_t _getBits(uint64_t bit_count)
    {
        // Refresh current byte in case we have no bits left
        _refreshCurrentByte();

        if (bit_count > _bits_left_in_curr_bytes)
        {
            uint64_t res_a = _curr_bytes;
            res_a <<= (64 - _bits_left_in_curr_bytes);
            res_a >>= (64 - _bits_left_in_curr_bytes);
            uint64_t res_a_sz = _bits_left_in_curr_bytes;

            bit_count -= _bits_left_in_curr_bytes;
            _position_in_bits += _bits_left_in_curr_bytes;
            _bits_left_in_curr_bytes = 0;

            // Here we know that (_bits_left_in_curr_bytes == 0) - use unsafe function
            _refreshCurrentByteUnsafe();

            uint64_t res_b = _curr_bytes;
            res_b <<= (64 - bit_count);
            res_b >>= (64 - bit_count);

            _bits_left_in_curr_bytes -= bit_count;
            _position_in_bits += bit_count;

            _curr_bytes >>= bit_count;

            return (res_a | (res_b << res_a_sz));
        }

        uint64_t res = _curr_bytes;
        res <<= (64 - bit_count);
        res >>= (64 - bit_count);

        _bits_left_in_curr_bytes -= bit_count;
        _position_in_bits += bit_count;

        _curr_bytes >>= bit_count;

        return res;
    }

    std::istream &_stream;
    uint64_t _curr_bytes;
    int _bits_left_in_curr_bytes;
    uint64_t _position_in_bits;

    std::vector<char> _cached_bytes;
    int64_t _cached_size;

    int _cached_bytes_curr_pos;
    bool _cached;
};

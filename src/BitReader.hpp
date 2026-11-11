#pragma once

#include <stdexcept>
#include <cstdint>
#include <cstddef>
#include <cstring>

// The pixel reading loops need these small functions inlined
#ifndef ZRAW_FORCE_INLINE
#if defined(_MSC_VER)
#define ZRAW_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define ZRAW_FORCE_INLINE inline __attribute__((always_inline))
#else
#define ZRAW_FORCE_INLINE inline
#endif
#endif

// Reads bits, least significant bit first, from a block of memory.
// Reading past the end gives zero bits. Loads 8 bytes at a time, so it needs a little-endian CPU.
class BitReader
{
public:
    BitReader(const uint8_t *data, size_t size)
        : _data(data),
          _size(size),
          _position_in_bits(0) {}

    // Up to 57 bits at once
    uint64_t ReadBits(uint32_t bit_count)
    {
        if (bit_count == 0)
            return 0;

        if (bit_count > 57)
            throw std::invalid_argument("bit_count can't be more than 57!");

        uint64_t res = _peek(_position_in_bits) & ((1ull << bit_count) - 1);
        _position_in_bits += bit_count;
        return res;
    }

    void FlushBits(uint32_t bit_count)
    {
        _position_in_bits += bit_count;
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
        _position_in_bits = bit_index;
    }

    // Up to 57 bits at once, does not move the position
    uint64_t ShowBits(uint32_t bit_count)
    {
        if (bit_count == 0)
            return 0;

        if (bit_count > 57)
            throw std::invalid_argument("bit_count can't be more than 57!");

        return _peek(_position_in_bits) & ((1ull << bit_count) - 1);
    }

    uint64_t CurrentPositionInBits()
    {
        return _position_in_bits;
    }

    // At least 57 bits from the given bit, the reader's own position is not used
    ZRAW_FORCE_INLINE uint64_t PeekAt(uint64_t position_in_bits) const
    {
        return _peek(position_in_bits);
    }

    // True if PeekAtInside is safe for these bits, it reads 8 bytes and does not check the size
    bool AreBitsInside(uint64_t position_in_bits, uint64_t bit_count) const
    {
        return ((position_in_bits + bit_count) >> 3) + 8 <= _size;
    }

    ZRAW_FORCE_INLINE uint64_t PeekAtInside(uint64_t position_in_bits) const
    {
        uint64_t bytes;
        std::memcpy(&bytes, _data + (position_in_bits >> 3), 8);
        return bytes >> (position_in_bits & 7);
    }

private:
    ZRAW_FORCE_INLINE uint64_t _peek(uint64_t position_in_bits) const
    {
        uint64_t byte_index = position_in_bits >> 3;
        uint64_t bytes = 0;

        // Fewer than 8 bytes are left at the end, the rest reads as zeros
        if (byte_index + 8 <= _size)
            std::memcpy(&bytes, _data + byte_index, 8);
        else if (byte_index < _size)
            std::memcpy(&bytes, _data + byte_index, _size - byte_index);

        return bytes >> (position_in_bits & 7);
    }

    const uint8_t *_data;
    size_t _size;
    uint64_t _position_in_bits;
};

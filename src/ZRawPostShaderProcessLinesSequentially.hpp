#pragma once

#include <stdint.h>
#include <string.h>

#include <vector>

#include "ZRawImageBlockLine.hpp"

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

// The line smoothing (post A and post B) and the Bayer line writer have AVX2 versions, picked at
// run time, no -mavx2 needed. Define ZRAW_NO_SIMD to build the portable code only.
#if !defined(ZRAW_NO_SIMD) && (defined(__x86_64__) || defined(_M_X64))
#define ZRAW_HAVE_AVX2_PATH
#include <immintrin.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#if defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#define ZRAW_TARGET_AVX2 __attribute__((target("avx2")))
#else
#define ZRAW_TARGET_AVX2
#endif
#endif

// Step 2: the lines must be processed in order. A line is predicted from the smoothed and truncated
// two lines above it, and the smoothing depends on the noise statistics of the earlier lines.
class ZRawPostShaderProcessLinesSequentially
{
public:
    struct Parameters
    {
        int width;                      // in photodiodes (two components per pair)
        int height;
        uint32_t bayer_pattern;
        uint16_t default_pix_value;
        int noise_level_distance;
        int real_bitdepth;
        uint32_t nl1;
        uint32_t nl2;
    };

    ZRawPostShaderProcessLinesSequentially(Parameters param)
        : _param(param), _pair_count(param.width / 2), _line_index(0),
          _max_allowed_pixel_value((1 << param.real_bitdepth) - 1), _curr_noise_level(0),
          _line_a(param.width / 2), _line_b(param.width / 2),
          _prev_a(param.width / 2, param.default_pix_value),
          _prev_b(param.width / 2, param.default_pix_value),
          _prev_c(param.width / 2, param.default_pix_value),
          _tmp(param.width / 2)
    {
        // The noise level history is kept for the whole frame, not cleared on every line
        memset(_noise_levels, 0x00, sizeof(_noise_levels));

        const int diff = 10;
        _truncate_bit_count = param.real_bitdepth - diff;
        if (_truncate_bit_count < 0)
            _truncate_bit_count = 0;

        // The smoothing sums of 12-bit values fit into 16-bit vector lanes, the sums of more bits do not
        _use_avx2 = false;
#ifdef ZRAW_HAVE_AVX2_PATH
        _use_avx2 = param.real_bitdepth <= 12 && _cpu_has_avx2();
#endif
    }

    // Per line: BeginLine(), then BeginBlock() and FixPair() for each pair of every block, then EndLine()

    void BeginLine()
    {
        // The line above (main subline) and the line two above (secondary subline), smoothed and truncated.
        // Lines of one parity share a secondary reference, so there are two of them.
        _prev_main_line = _prev_a.data();
        _prev_sec_line = (_line_index & 1) == 0 ? _prev_b.data() : _prev_c.data();

        // The left neighbors start at the default value on each line
        _pair_index = 0;
        _prev_pixel_a = _prev_pixel_b = _param.default_pix_value;
        _pr_p_pixel_a = _pr_p_pixel_b = _param.default_pix_value;
        _noise_distances_counter = 0;
    }

    void BeginBlock(bool is_block_vl, uint32_t block_header_value)
    {
        _is_block_vl = is_block_vl;

        if (is_block_vl)
        {
            _param_a = block_header_value;
            _param_b = 1 << _param_a;
            _param_c = (_param_b >> 1) - 1;
            if (_param_c < 0)
                _param_c = 0;
            _param_d = ((2 * _param_c + _max_allowed_pixel_value) >> _param_a) + 1;
        }
    }

    // Raw blocks already hold pixel values, VL blocks hold differences to the predicted values.
    // Uses current and previous line (main and secondary sublines).
    // Also collects noise level statistics (main subline only).
    ZRAW_FORCE_INLINE void FixPair(int32_t curr_value_a, int32_t curr_value_b)
    {
        int i = _pair_index++;

        int pr_c_pixel_a = _prev_main_line[i];
        int pr_c_pixel_b = _prev_sec_line[i];

        int pixel_value_a = curr_value_a;
        int pixel_value_b = curr_value_b;

        if (_is_block_vl)
        {
            // Predict offsets
            auto predicted_offset_a = _fixPrediction(
                _prev_pixel_a,
                pr_c_pixel_a,
                _pr_p_pixel_a);
            auto predicted_offset_b = _fixPrediction(
                _prev_pixel_b,
                pr_c_pixel_b,
                _pr_p_pixel_b);

            // Get two's complement from constructed A value according to sign bit
            // Based on: https://en.wikipedia.org/wiki/Two%27s_complement
            bool sign_a = curr_value_a & 1;
            int body_a = (curr_value_a + 1) >> 1;
            int complement_a = sign_a ? -body_a : body_a;
            bool sign_b = curr_value_b & 1;
            int body_b = (curr_value_b + 1) >> 1;
            int complement_b = sign_b ? -body_b : body_b;

            // Fix component A value
            pixel_value_a = _unmodValue(
                _param_b * complement_a + predicted_offset_a,
                _param_d, _max_allowed_pixel_value,
                _param_c, _param_b);

            // Round pixel value to [0; max_allowed_pixel_value)
            pixel_value_a = _roundValue(0, pixel_value_a, _max_allowed_pixel_value);

            // Fix component B value
            pixel_value_b = _unmodValue(
                _param_b * complement_b + predicted_offset_b,
                _param_d, _max_allowed_pixel_value,
                _param_c, _param_b);

            // Round pixel value to [0; max_allowed_pixel_value)
            pixel_value_b = _roundValue(0, pixel_value_b, _max_allowed_pixel_value);
        }

        // Save new read pixel values
        _line_a[i] = pixel_value_a;
        _line_b[i] = pixel_value_b;

        // Get distance between 2 last pixel values
        int l1 = pixel_value_a - _prev_pixel_a;
        l1 = l1 < 0 ? -l1 : l1;

        // Get distance between last pixel value and prev lines prev pixel value
        int l2 = _pr_p_pixel_a - pixel_value_a;
        l2 = l2 < 0 ? -l2 : l2;

        // Use least distance
        int l12 = l1 <= l2 ? l1 : l2;

        // Get distance between last pixel values from different lines
        int l3 = pr_c_pixel_a - pixel_value_a;
        l3 = l3 < 0 ? -l3 : l3;

        // Use least distance
        int l123 = l12 <= l3 ? l12 : l3;

        // If distance is less than parameter - increase counter
        _noise_distances_counter += l123 < _param.noise_level_distance;

        // These become the left neighbors of the next pair
        _prev_pixel_a = pixel_value_a;
        _prev_pixel_b = pixel_value_b;
        _pr_p_pixel_a = pr_c_pixel_a;
        _pr_p_pixel_b = pr_c_pixel_b;
    }

    // Writes the line to out_line (width values). block_header_values has one value per block,
    // the same ones that were given to BeginBlock.
    void EndLine(const uint32_t* block_header_values, uint16_t* out_line)
    {
        int i = _line_index;

        // Smoothing and truncation go to the reference lines only; the output keeps the decoded values.
        // Step 3: correct Bayer pattern
        _post_process_correct_bayer(out_line, i);

        // PostProcess main line
        _pre_process_line_post_A(_line_a.data(), _prev_a.data(), _needed_field(_param.bayer_pattern, i), _curr_noise_level);

        // PostProcess secondary line
        _pre_process_line_post_B(_line_b.data(), _prev_sec_line, block_header_values, _curr_noise_level);

        // Estimate noise level for the next line
        _curr_noise_level = _pre_process_line_estimate_noise_level(_noise_distances_counter, _param.nl1, _param.nl2);

        ++_line_index;
    }

protected:

    ZRAW_FORCE_INLINE int _fixPrediction(int p1, int p2, int value)
    {
        // This function inverts value in local interval:
        // [a-------b]---value => result = a
        // value---[a-------b] => result = b
        // ================================================
        // Or standard case:
        // [a---------value--b] => result = a + b - value:
        // [a--result--------b] (offsets from a and b are exchanged)
        // ================================================
        // More simple formula for standard case (when result is not a and not b):
        // result = ((-(value - a)) mod (b-a)) + a

        // Branchless, the choice depends on the pixel data and is hard to predict
        int a = p1 <= p2 ? p1 : p2;
        int b = p1 <= p2 ? p2 : p1;

        int result = a < value ? a + b - value : b;
        return b <= value ? a : result;
    }

    ZRAW_FORCE_INLINE int _unmodValue(int value, int step, int safe_offset, int range, int step_count)
    {
        int result = value;

        if (value < -range)
            result = value + step_count * step;

        if (range + safe_offset < value)
            result = value - step_count * step;

        return result;
    }

    ZRAW_FORCE_INLINE int _roundValue(int left, int value, int right)
    {
        if (value > right)
            value = right;
        if (value < left)
            value = left;
        return value;
    }

    // Does not use any lines
    int64_t _pre_process_line_estimate_noise_level(int noise_distances_counter, int noise_level_1, int noise_level_2)
    {
        if (noise_distances_counter >= noise_level_2)
            _place_noise_level(noise_distances_counter < noise_level_1, _noise_levels);
        else
            _place_noise_level(2u, _noise_levels);

        return _select_noise_level(_noise_levels);
    }

    void _place_noise_level(unsigned int noise_level, unsigned int *noise_levels)
    {
        // [A B C D E F G H]
        // to
        // [A A B C D E F G]
        // (move all elements to the right by 1)
        for (int i = 7; i > 0; --i)
            noise_levels[i] = noise_levels[i - 1];

        // First element is set from outside
        // [A A B C D E F G]
        // to
        // [X A B C D E F G], where X = noise_level
        noise_levels[0] = noise_level;
    }

    // ---

    // Smooths the main subline (component A). Uses only 1 line, writes into another one.
    void _pre_process_line_post_A(const uint16_t* line, uint16_t* dest, bool is_needed_field, int noise_level)
    {
        const int length = _pair_count;
        uint16_t* tmp = _tmp.data();

        // Pixels near the line borders use clamped indices, the others do not need them.
        // Interior: i + is_needed_field - 2 >= 0 and i + is_needed_field + 1 < length
        const int first = length < 2 ? length : 2;
        const int last = length - 2 < first ? first : length - 2;

        for (int i = 0; i < first; ++i)
            tmp[i] = _post_A_border_pixel(line, i, is_needed_field, noise_level);

        int i = first;
        const uint16_t* p = line + is_needed_field;
#ifdef ZRAW_HAVE_AVX2_PATH
        if (_use_avx2)
            i = _post_A_interior_avx2(p, tmp, i, last, noise_level);
#endif
        for (; i < last; ++i)
            tmp[i] = _post_A_pixel(p[i - 2], p[i - 1], p[i], p[i + 1], noise_level);

        for (i = last; i < length; ++i)
            tmp[i] = _post_A_border_pixel(line, i, is_needed_field, noise_level);

        _truncate(tmp, dest, length);
    }

    int _post_A_border_pixel(const uint16_t* line, int i, int is_needed_field, int noise_level)
    {
        int index0 = _round(0, i - 2 + is_needed_field, _pair_count);
        int index3 = _round(0, i - 1 + is_needed_field, _pair_count);
        int index2 = _round(0, i + 0 + is_needed_field, _pair_count);
        int index1 = _round(0, i + 1 + is_needed_field, _pair_count);

        return _post_A_pixel(line[index0], line[index3], line[index2], line[index1], noise_level);
    }

    // Two neighbors on the left, one on the right
    int _post_A_pixel(int left_2, int left_1, int center, int right_1, int noise_level)
    {
        if (noise_level < 2)
            return (8 * center + 8 * left_1 + 8) / 16;
        return (3 * right_1 + 5 * center + 5 * left_1 + 3 * left_2 + 8) / 16;
    }

    // Smooths the secondary subline (component B). Uses only 1 line, writes into another one.
    void _pre_process_line_post_B(const uint16_t* line, uint16_t* dest, const uint32_t* line_header_values, int noise_level)
    {
        const int length = _pair_count;
        uint16_t* tmp = _tmp.data();

        // Pixels near the line borders use clamped indices, the others do not need them
        const int first = length < 2 ? length : 2;
        const int last = length - 2 < first ? first : length - 2;

        for (int i = 0; i < first; ++i)
            tmp[i] = _post_B_border_pixel(line, i, line_header_values, noise_level);

        int i = first;
#ifdef ZRAW_HAVE_AVX2_PATH
        // The vector part starts at a multiple of 16, so that the values of each vector
        // share one block (one header value)
        if (_use_avx2 && length >= 36)
        {
            for (; i < 16; ++i)
                tmp[i] = _post_B_pixel(line + i, line_header_values[i / ZRAW_LINE_BLOCK_SIZE], noise_level);
            i = _post_B_interior_avx2(line, tmp, line_header_values, noise_level, i, last);
        }
#endif
        for (; i < last; ++i)
            tmp[i] = _post_B_pixel(line + i, line_header_values[i / ZRAW_LINE_BLOCK_SIZE], noise_level);

        for (i = last; i < length; ++i)
            tmp[i] = _post_B_border_pixel(line, i, line_header_values, noise_level);

        _truncate(tmp, dest, length);
    }

    int _post_B_border_pixel(const uint16_t* line, int i, const uint32_t* line_header_values, int noise_level)
    {
        // Same as _post_B_pixel, but the neighbors come from the nearest pixels inside the line
        uint16_t neighbors[5];
        for (int k = 0; k < 5; ++k)
            neighbors[k] = line[_round(0, i + k - 2, _pair_count)];
        return _post_B_pixel(neighbors + 2, line_header_values[i / ZRAW_LINE_BLOCK_SIZE], noise_level);
    }

    // line points to the pixel, two pixels on each side must be readable
    int _post_B_pixel(const uint16_t* line, uint32_t block_header_value, int noise_level)
    {
        int block_header_val = 1 << block_header_value;

        int v0 = line[-2];
        int v1 = line[-1];
        int v2 = line[0];
        int v3 = line[1];
        int v4 = line[2];

        if (noise_level)
        {
            if (noise_level == 1)
                return ((v3 << 2) + (v2 << 3) + (v1 << 2) + 8) >> 4;
            return ((v4 << 1) + (v3 << 2) + (v2 << 2) + (v1 << 2) + (v0 << 1) + 8) >> 4;
        }

        int v18 = (v3 + 2 * v2 + v1 + 2) >> 2;
        int offset = (block_header_val >> 1);    // block_header_val / 2;

        if (v18 - v2 <= offset)
            offset = (v18 - v2 >= -offset) ? v18 - v2 : -offset;

        return offset + v2;
    }

    int _needed_field(uint32_t bayer_pattern, int current_line_index)
    {
        uint32_t a = 1;
        if (bayer_pattern != 3)
            a = bayer_pattern == 0;
        return ((current_line_index & 1) != a) ? 0 : 1;
    }

    // Rounds to [begin; end)
    int _round(int begin, int val, int end)
    {
        if (val < end)
        {
            if (val >= begin)
                return val;
            return begin;
        }
        return end - 1;
    }

    // ---

    // Writes the smoothed line to dest with the lowest bits cleared. Uses only 1 line.
    void _truncate(const uint16_t* line, uint16_t* dest, int length)
    {
        int i = 0;
#ifdef ZRAW_HAVE_AVX2_PATH
        if (_use_avx2)
            i = _truncate_avx2(line, dest, length, _truncate_bit_count);
#endif
        for (; i < length; ++i)
            dest[i] = (signed int)line[i] >> _truncate_bit_count << _truncate_bit_count;
    }

    // ---

    // Step 3: the two components of a pair are swapped on even lines for Bayer patterns 0 and 3,
    // and on odd lines for patterns 1 and 2.
    void _post_process_correct_bayer(uint16_t* out_line, int current_line_index)
    {
        bool is_upper_field = (current_line_index & 1) == 0;
        bool swap = is_upper_field ?
            (_param.bayer_pattern == 0 || _param.bayer_pattern == 3) :
            (_param.bayer_pattern == 1 || _param.bayer_pattern == 2);

        // A last line without a partner (odd height) stays as it is
        if (current_line_index >= _param.height / 2 * 2)
            swap = false;

        const uint16_t* first = swap ? _line_b.data() : _line_a.data();
        const uint16_t* second = swap ? _line_a.data() : _line_b.data();

        int i = 0;
#ifdef ZRAW_HAVE_AVX2_PATH
        if (_use_avx2)
            i = _interleave_avx2(first, second, out_line, _pair_count);
#endif
        for (; i < _pair_count; ++i)
        {
            out_line[i * 2 + 0] = first[i];
            out_line[i * 2 + 1] = second[i];
        }
    }

    // ---

    uint32_t _select_noise_level(uint32_t *noise_levels)
    {
        int cnt_0 = 0;
        int cnt_1 = 0;
        int cnt_2 = 0;
        for (int i = 0; i < 8; ++i)
        {
            switch (noise_levels[i])
            {
            case 0:
                ++cnt_0;
                break;
            case 1:
                ++cnt_1;
                break;
            case 2:
                ++cnt_2;
                break;
            }
        }

        // If cnt_0 is biggest - return 0
        if (cnt_0 >= cnt_1 && cnt_0 >= cnt_2)
            return 0;

        // If cnt_1 is biggest - return 1
        if (cnt_1 >= cnt_0 && cnt_1 >= cnt_2)
            return 1;

        // If cnt_2 is biggest - return 2
        return 2;
    }

#ifdef ZRAW_HAVE_AVX2_PATH
    static bool _cpu_has_avx2()
    {
        static const bool has_avx2 = _detect_avx2();
        return has_avx2;
    }

    static bool _detect_avx2()
    {
#if defined(__GNUC__) || defined(__clang__)
        unsigned int a, b, c, d;
        if (__get_cpuid_max(0, nullptr) < 7)
            return false;

        // AVX needs the operating system to save the vector registers
        __cpuid(1, a, b, c, d);
        bool os_saves_registers = (c & (1 << 27)) != 0;
        bool has_avx = (c & (1 << 28)) != 0;
        if (!os_saves_registers || !has_avx)
            return false;

        unsigned int xcr0, xcr0_high;
        __asm__("xgetbv" : "=a"(xcr0), "=d"(xcr0_high) : "c"(0));
        if ((xcr0 & 6) != 6)
            return false;

        __cpuid_count(7, 0, a, b, c, d);
        return (b & (1 << 5)) != 0;
#else
        int info[4];
        __cpuid(info, 0);
        if (info[0] < 7)
            return false;

        // AVX needs the operating system to save the vector registers
        __cpuid(info, 1);
        bool os_saves_registers = (info[2] & (1 << 27)) != 0;
        bool has_avx = (info[2] & (1 << 28)) != 0;
        if (!os_saves_registers || !has_avx || (_xgetbv(0) & 6) != 6)
            return false;

        __cpuidex(info, 7, 0);
        return (info[1] & (1 << 5)) != 0;
#endif
    }

    ZRAW_TARGET_AVX2 static __m256i _load16(const uint16_t* p)
    {
        return _mm256_loadu_si256((const __m256i*)p);
    }

    // Post A for the interior pixels, 16 at a time. Returns the first pixel that is not done.
    // p is the line shifted by is_needed_field.
    ZRAW_TARGET_AVX2 static int _post_A_interior_avx2(const uint16_t* p, uint16_t* tmp, int i, int last, int noise_level)
    {
        if (noise_level >= 2)
        {
            const __m256i k3 = _mm256_set1_epi16(3), k5 = _mm256_set1_epi16(5), k8 = _mm256_set1_epi16(8);
            for (; i + 16 <= last; i += 16)
            {
                __m256i outer = _mm256_mullo_epi16(_mm256_add_epi16(_load16(p + i + 1), _load16(p + i - 2)), k3);
                __m256i inner = _mm256_mullo_epi16(_mm256_add_epi16(_load16(p + i), _load16(p + i - 1)), k5);
                _mm256_storeu_si256((__m256i*)(tmp + i), _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(outer, inner), k8), 4));
            }
        }
        else
        {
            // (8a + 8b + 8) / 16 is the average of a and b, rounded up
            for (; i + 16 <= last; i += 16)
                _mm256_storeu_si256((__m256i*)(tmp + i), _mm256_avg_epu16(_load16(p + i), _load16(p + i - 1)));
        }
        return i;
    }

    // Post B for the pixels [i; end), 16 at a time, i is a multiple of 16.
    // Returns the first pixel that is not done.
    ZRAW_TARGET_AVX2 static int _post_B_interior_avx2(const uint16_t* line, uint16_t* tmp, const uint32_t* line_header_values, int noise_level, int i, int end)
    {
        const __m256i k2 = _mm256_set1_epi16(2), k8 = _mm256_set1_epi16(8);
        for (; i + 16 <= end; i += 16)
        {
            __m256i m1 = _load16(line + i - 1), c = _load16(line + i), p1 = _load16(line + i + 1);
            __m256i result;
            if (noise_level == 1)
                result = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(m1, p1), _mm256_add_epi16(_mm256_add_epi16(c, c), k2)), 2);
            else if (noise_level >= 2)
            {
                __m256i near3 = _mm256_slli_epi16(_mm256_add_epi16(_mm256_add_epi16(m1, p1), c), 2);
                __m256i far2 = _mm256_slli_epi16(_mm256_add_epi16(_load16(line + i - 2), _load16(line + i + 2)), 1);
                result = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(near3, far2), k8), 4);
            }
            else
            {
                // The value moves towards the local average by at most half of the block step
                __m256i average = _mm256_srli_epi16(_mm256_add_epi16(_mm256_add_epi16(m1, p1), _mm256_add_epi16(_mm256_add_epi16(c, c), k2)), 2);
                __m256i offset = _mm256_set1_epi16((short)((1 << line_header_values[i / ZRAW_LINE_BLOCK_SIZE]) >> 1));
                __m256i delta = _mm256_sub_epi16(average, c);
                delta = _mm256_min_epi16(_mm256_max_epi16(delta, _mm256_sub_epi16(_mm256_setzero_si256(), offset)), offset);
                result = _mm256_add_epi16(c, delta);
            }
            _mm256_storeu_si256((__m256i*)(tmp + i), result);
        }
        return i;
    }

    // Clears the lowest bits, 16 values at a time. Returns the first value that is not done.
    ZRAW_TARGET_AVX2 static int _truncate_avx2(const uint16_t* line, uint16_t* dest, int length, int bit_count)
    {
        const __m256i keep = _mm256_set1_epi16((short)(0xFFFF << bit_count));
        int i = 0;
        for (; i + 16 <= length; i += 16)
            _mm256_storeu_si256((__m256i*)(dest + i), _mm256_and_si256(_load16(line + i), keep));
        return i;
    }

    // Interleaves two components, 16 pairs at a time. Returns the first pair that is not done.
    ZRAW_TARGET_AVX2 static int _interleave_avx2(const uint16_t* first, const uint16_t* second, uint16_t* dest, int pair_count)
    {
        int i = 0;
        for (; i + 16 <= pair_count; i += 16)
        {
            __m256i a = _load16(first + i), b = _load16(second + i);
            __m256i lo = _mm256_unpacklo_epi16(a, b), hi = _mm256_unpackhi_epi16(a, b);
            _mm256_storeu_si256((__m256i*)(dest + 2 * i), _mm256_permute2x128_si256(lo, hi, 0x20));
            _mm256_storeu_si256((__m256i*)(dest + 2 * i + 16), _mm256_permute2x128_si256(lo, hi, 0x31));
        }
        return i;
    }
#endif

    Parameters _param;
    int _pair_count;
    int _line_index;
    int _max_allowed_pixel_value;

    // The current line
    int _pair_index;
    const uint16_t* _prev_main_line;
    uint16_t* _prev_sec_line;
    int _prev_pixel_a, _prev_pixel_b;       // the pixels on the left of the current pair
    int _pr_p_pixel_a, _pr_p_pixel_b;       // the pixels of the line above them (on the upper left)
    int _noise_distances_counter;

    // The current block
    bool _is_block_vl;
    uint32_t _param_a;
    int _param_b, _param_c, _param_d;
    int _truncate_bit_count;
    bool _use_avx2;

    // Noise level of the previous lines (history of 8) and the one used to smooth the current line
    uint32_t _noise_levels[8];
    uint32_t _curr_noise_level;

    // Decoded current line, one array per component
    std::vector<uint16_t> _line_a;
    std::vector<uint16_t> _line_b;

    // Smoothed and truncated lines used to predict the next ones: the component A of the previous
    // line, the component B of the previous line of the same parity (even or odd)
    std::vector<uint16_t> _prev_a;
    std::vector<uint16_t> _prev_b;
    std::vector<uint16_t> _prev_c;

    std::vector<uint16_t> _tmp;
};

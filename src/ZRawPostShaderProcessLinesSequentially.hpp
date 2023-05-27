#pragma once

#include <stdint.h>
#include <string.h>

#include <vector>

#include "ZRawImageBlockLine.hpp"

class ZRawPostShaderProcessLinesSequentially
{
public:
    // Returns noise levels
    void Process(uint16_t** image, int** is_block_vl,
        uint32_t** block_header_values, uint32_t bayer_pattern,
        int width, int height, uint16_t default_pix_value,
        int noise_level_distance, int real_bitdepth,
        uint32_t nl1, uint32_t nl2)
    {
        // Prepare dummy line
        uint16_t* dummy_image_line = new uint16_t[width];
        for (int i = 0; i < width; ++i)
            dummy_image_line[i] = default_pix_value;

        const int diff = 10;
        int bit_count = real_bitdepth - diff;
        if (bit_count < 0)
            bit_count = 0;

        // Process each line and prev
        uint32_t curr_noise_level = 0;
        for (int i = 0; i < height; ++i)
        {
            auto prev_main_line = (i < 1) ? dummy_image_line : image[i - 1];
            auto prev_sec_line = (i < 2) ? dummy_image_line : image[i - 2];

            if (i > 0)
            {
                auto prev_main_line = (i < 1) ? dummy_image_line : image[i - 1];
                auto prev_sec_line = (i < 2) ? dummy_image_line : image[i - 2];
            }

            // Fix values from VL blocks in each line
            _pre_process_line_vl_blocks(image[i], is_block_vl[i], block_header_values[i], prev_main_line, prev_sec_line, width, default_pix_value, real_bitdepth);

            // Collect noise level statistics for the current line
            auto noise_level_stat_value = _pre_process_line_noise_level_statistics(image[i], prev_main_line, width, default_pix_value, noise_level_distance);

            //std::cout << "nl[" << i << "] = " << curr_noise_level << std::endl;

            // PostProcess main line
            _pre_process_line_post_A(image[i], _needed_field(bayer_pattern, i), curr_noise_level, width);

            // PostProcess secondary lin
            _pre_process_line_post_B(image[i], block_header_values[i], curr_noise_level, width);

            // Truncate all pixels
            _pre_process_line_truncate(image[i], bit_count, width);

            // Estimate noise level for the current line
            memset(_noise_levels, 0x00, sizeof(_noise_levels));
            curr_noise_level = _pre_process_line_estimate_noise_level(noise_level_stat_value, nl1, nl2);
        }

        // Free dummy line
        delete[] dummy_image_line;
    }

protected:
    // Uses current and previous line (main and secondary sublines)
    void _pre_process_line_vl_blocks(uint16_t* line, int* is_block_vl, uint32_t* block_header_values, uint16_t* prev_main_line, uint16_t* prev_sec_line, int length, uint16_t default_pix_value, int real_bitdepth)
    {
        for (int block_index = 0; block_index < length / (ZRAW_LINE_BLOCK_SIZE * 2); ++block_index)
        {
            if (is_block_vl[block_index] == 0)
                continue;

            int max_allowed_pixel_value = (1 << real_bitdepth) - 1;
            uint32_t param_a = block_header_values[block_index];
            int param_b = 1 << param_a;
            int param_c = (param_b >> 1) - 1;
            if (param_c < 0)
                param_c = 0;
            uint32_t param_d = ((2 * param_c + max_allowed_pixel_value) >> param_a) + 1;

            // Here we process VL block and fix values
            for (int i = 0; i < (ZRAW_LINE_BLOCK_SIZE * 2); i += 2)
            {
                auto pix_index = block_index * ZRAW_LINE_BLOCK_SIZE * 2 + i;

                auto curr_pixel_a = line[pix_index];
                auto prev_pixel_a = (pix_index - 2 < 0) ? default_pix_value : line[pix_index - 2];
                auto pr_c_pixel_a = prev_main_line[pix_index];
                auto pr_p_pixel_a = (pix_index - 2 < 0) ? default_pix_value : prev_main_line[pix_index - 2];

                auto curr_pixel_b = line[pix_index + 1];
                auto prev_pixel_b = (pix_index - 1 < 0) ? default_pix_value : line[pix_index - 1];
                auto pr_c_pixel_b = prev_sec_line[pix_index + 1];
                auto pr_p_pixel_b = (pix_index - 1 < 0) ? default_pix_value : prev_sec_line[pix_index - 1];

                // Predict offsets
                auto predicted_offset_a = _fixPrediction(
                    prev_pixel_a,
                    pr_c_pixel_a,
                    pr_p_pixel_a);
                auto predicted_offset_b = _fixPrediction(
                    prev_pixel_b,
                    pr_c_pixel_b,
                    pr_p_pixel_b);

                // Get two's complement from constructed A value according to sign bit
                // Based on: https://en.wikipedia.org/wiki/Two%27s_complement
                bool sign_a = curr_pixel_a & 1;
                int body_a = (curr_pixel_a + 1) >> 1;
                int complement_a = sign_a ? -body_a : body_a;
                bool sign_b = curr_pixel_b & 1;
                int body_b = (curr_pixel_b + 1) >> 1;
                int complement_b = sign_b ? -body_b : body_b;

                // Fix component A value
                int pixel_value = _unmodValue(
                    param_b * complement_a + predicted_offset_a,
                    param_d, max_allowed_pixel_value,
                    param_c, param_b);

                // Round pixel value to [0; max_allowed_pixel_value)
                pixel_value = _roundValue(0, pixel_value, max_allowed_pixel_value);

                // Save new read pixel value
                line[pix_index] = pixel_value;

                // Fix component B value
                pixel_value = _unmodValue(
                    param_b * complement_b + predicted_offset_b,
                    param_d, max_allowed_pixel_value,
                    param_c, param_b);

                // Round pixel value to [0; max_allowed_pixel_value)
                pixel_value = _roundValue(0, pixel_value, max_allowed_pixel_value);

                // Save new read pixel value
                line[pix_index + 1] = pixel_value;

                //std::cout << "[" << pix_index << "] value_a_vl = " << line[pix_index] << " value_b_vl = " << line[pix_index + 1] << " ";
                //std::cout << "curr_pixel_a = " << curr_pixel_a << " curr_pixel_b = " << curr_pixel_b << " ";
                //std::cout << "prev_pixel_a = " << prev_pixel_a << " prev_pixel_b = " << prev_pixel_b << " ";
                //std::cout << "pr_c_pixel_a = " << pr_c_pixel_a << " pr_c_pixel_b = " << pr_c_pixel_b << " ";
                //std::cout << "pr_p_pixel_a = " << pr_p_pixel_a << " pr_p_pixel_b = " << pr_p_pixel_b << " ";
                //std::cout << "predicted_offset_a = " << predicted_offset_a << " predicted_offset_b = " << predicted_offset_b << " ";
                //std::cout << "param_b = " << param_b << " param_c = " << param_c << " ";
                //std::cout << "param_d = " << param_d << " max_allowed_pixel_value = " << max_allowed_pixel_value << " bitdepth_diff = " << param_a << " ";
                //std::cout << std::endl;
            }
        }
    }

    // Uses current and previous line (only main subline)
    uint32_t _pre_process_line_noise_level_statistics(uint16_t* line, uint16_t* prev_line, int length, uint16_t default_pix_value, int noise_level_distance)
    {
        uint32_t result = 0;

        // We process A pixels ([0], [2], [4], [6], ...)
        for (int i = 0; i < length; i += 2)
        {
            auto curr_pixel = line[i];
            auto prev_pixel = (i - 2 < 0) ? default_pix_value : line[i - 2];
            auto pr_c_pixel = prev_line[i];
            auto pr_p_pixel = (i - 2 < 0) ? default_pix_value : prev_line[i - 2];

            // Get distance between 2 last pixel values
            int l1 = curr_pixel - prev_pixel;
            l1 = l1 < 0 ? -l1 : l1;

            // Get distance between last pixel value and prev lines prev pixel value
            int l2 = pr_p_pixel - curr_pixel;
            l2 = l2 < 0 ? -l2 : l2;

            // Use least distance
            int l12 = l1 <= l2 ? l1 : l2;

            // Get distance between last pixel values from different lines
            int l3 = pr_c_pixel - curr_pixel;
            l3 = l3 < 0 ? -l3 : l3;

            // Use least distance
            int l123 = l12 <= l3 ? l12 : l3;

            // If distance is less than parameter - increase counter
            if (l123 < noise_level_distance)
                ++result;
        }

        return result;
    }

    int _fixPrediction(int p1, int p2, int value)
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

        int a = p1 <= p2 ? p1 : p2;
        int b = p1 <= p2 ? p2 : p1;

        if (b <= value)
            return a;

        if (a < value)
            return a + b - value;

        return b;
    }

    int _unmodValue(int value, int step, int safe_offset, int range, int step_count)
    {
        int result = value;

        if (value < -range)
            result = value + step_count * step;

        if (range + safe_offset < value)
            result = value - step_count * step;

        return result;
    }

    int _roundValue(int left, int value, int right)
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

    // Uses only 1 line
    void _pre_process_line_post_A(uint16_t* line, bool is_needed_field, int noise_level, int length)
    {
        uint16_t* tmp = new uint16_t[length / 2];
        for (uint32_t i = 0; i < length / 2; ++i)
        {
            int index0 = _round(0, i - 2 + is_needed_field, length / 2) * 2;
            int index3 = _round(0, i - 1 + is_needed_field, length / 2) * 2;
            int index2 = _round(0, i + 0 + is_needed_field, length / 2) * 2;
            int index1 = _round(0, i + 1 + is_needed_field, length / 2) * 2;

            if (noise_level < 2)
                tmp[i] = (8 * line[index2] + 8 * line[index3] + 8) / 16;
            else
                tmp[i] = (3 * line[index1] + 5 * line[index2] + 5 * line[index3] + 3 * line[index0] + 8) / 16;
        }

        for (uint32_t i = 0; i < length / 2; ++i)
            line[i * 2] = tmp[i];

        delete[] tmp;
    }

    // Uses only 1 line
    void _pre_process_line_post_B(uint16_t* line, uint32_t* line_header_values, int noise_level, int length)
    {
        uint16_t* tmp = new uint16_t[length];
        for (uint32_t i = 0; i < length / 2; ++i)
        {
            int block_header_val = 1 << line_header_values[i / ZRAW_LINE_BLOCK_SIZE];

            int index0 = _round(0, i - 2, length / 2) * 2 + 1;
            int index1 = _round(0, i - 1, length / 2) * 2 + 1;
            int index2 = _round(0, i + 0, length / 2) * 2 + 1;
            int index3 = _round(0, i + 1, length / 2) * 2 + 1;
            int index4 = _round(0, i + 2, length / 2) * 2 + 1;

            if (noise_level)
            {
                if (noise_level == 1)
                    tmp[i] = ((line[index3] << 2) + (line[index2] << 3) + (line[index1] << 2) + 8) >> 4;
                else
                    tmp[i] = ((line[index4] << 1) + (line[index3] << 2) + (line[index2] << 2) + (line[index1] << 2) + (line[index0] << 1) + 8) >> 4;
            }
            else
            {
                int v18 = (line[index3] + 2 * line[index2] + line[index1] + 2) >> 2;
                int offset = (block_header_val >> 1);    // block_header_val / 2;

                if (v18 - line[index2] <= offset)
                    offset = (v18 - line[index2] >= -offset) ? v18 - line[index2] : -offset;

                tmp[i] = offset + line[index2];
            }
        }

        for (uint32_t i = 0; i < length / 2; ++i)
            line[i * 2 + 1] = tmp[i];

        delete[] tmp;
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

    // Uses only 1 line
    void _pre_process_line_truncate(uint16_t* line, int bit_count, int length)
    {
        for (uint32_t i = 0; i < length; ++i)
            line[i] = (signed int)line[i] >> bit_count << bit_count;
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

    uint32_t _noise_levels[8];
};
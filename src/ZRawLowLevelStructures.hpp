#pragma once

#include <stdint.h>

typedef struct ZRawFrameVersion_s
{
    uint32_t version;
} ZRawFrameVersion_t;

typedef struct ZRawFrameParameters_s
{
    uint32_t bayer_mode;        // Type of bayer pattern
    uint32_t cmp_mode;          // Compression mode (0 - encoded, 1 - raw)
    uint32_t is_lossless;
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t frame_bitdepth;    // = (bitdepth_real - 8) / 2
    uint32_t part_cmp_en;
    uint32_t stride_en;
    uint32_t stride;
    uint32_t align_mode;
    uint32_t debug_mode;
    uint32_t nl2_num_thr;
    uint32_t nl1_num_thr;
    uint32_t noise_grad_thr;
} ZRawFrameParameters_t;

#define ZRAW_DEFECTIVE_PIXELS_MAX 0x4000
typedef struct ZRawFrameDefectivePixelsInfo_s
{
    uint32_t tables_count;
    struct
    {
        uint32_t a;
        uint32_t bright_pixels_count;
        uint32_t bright_pixels_table[ZRAW_DEFECTIVE_PIXELS_MAX];
        uint32_t dark_pixels_count;
        uint32_t dark_pixels_table[ZRAW_DEFECTIVE_PIXELS_MAX];
    } defective_pixels_corretion_tables[32];
} ZRawFrameDefectivePixelsInfo_t;

typedef struct ZRawFrameAutoWhiteBalanceInfo_s
{
    uint32_t gain_red;
    uint32_t gain_green;
    uint32_t gain_blue;
} ZRawFrameAutoWhiteBalanceInfo_t;

typedef struct ZRawFrameSensorBlackLevels_s
{
    uint16_t levels_per_component[4];
} ZRawFrameSensorBlackLevels_t;

typedef struct ZRawFrameMetaData_s
{
    uint32_t metadata;
} ZRawFrameMetaData_t;

#define ZRAW_COLOR_CORRETION_MATRICES_MAX 10
typedef struct ZRawFrameColorCorretionMatrices_s
{
    uint32_t matrices_count;
    struct
    {
        uint16_t a;
        uint32_t b[9];
    } matrices[ZRAW_COLOR_CORRETION_MATRICES_MAX];
} ZRawFrameColorCorretionMatrices_t;

#define ZRAW_GAMMA_CURVE_POINTS_MAX 2048
typedef struct ZRawFrameGammaCurveInfo_s
{
    uint32_t curve_type; // 0 - default; 1 - sRGB; 2 - HDR; 3 - user-defined
    uint32_t curve_points_count;
    uint16_t curve_points[ZRAW_GAMMA_CURVE_POINTS_MAX];
} ZRawFrameGammaCurveInfo_t;

#pragma once

#include <byteswap.hpp>

#define ZRAW_TEST_FRAME_WIDTH 0
#define ZRAW_TEST_FRAME_HEIGHT 0
#define ZRAW_TEST_DPC_TABLES_COUNT 32
#define ZRAW_TEST_DPC_TABLE_BRIGHT_PIXELS_COUNT 0x4000
#define ZRAW_TEST_DPC_TABLE_DARK_PIXELS_COUNT 0x4000
#define ZRAW_TEST_CCM_MATRICES_COUNT 1
#define ZRAW_TEST_CCM_AES_DATA_SIZE 0
#define ZRAW_TEST_GAMM_CURVE_POINTS_COUNT 10
#define ZRAW_TEST_DATA_LINE_ELEMENTS 0

#define ZRAW_TEST_HEADER_NAME_MACRO(a, b, c, d) ((d << 24) | (c << 16) | (b << 8) | a)

#pragma pack(push, 1)

typedef struct
{
    uint32_t block_size;
    uint32_t block_magic;
} ZRAW_block_header_t;

typedef struct ZRAW_block_ver_s
{
    ZRAW_block_ver_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('v', 'e', 'r', ' ');
        header.block_size = bswap_32(sizeof(ZRAW_block_ver_s));
    }

    ZRAW_block_header_t header;
    struct
    {
        uint32_t version;
        uint8_t reserved_data[16];
    } data;
} ZRAW_block_ver_t;

typedef struct ZRAW_block_pram_s
{
    ZRAW_block_pram_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('p', 'r', 'a', 'm');
        header.block_size = bswap_32(sizeof(ZRAW_block_pram_s));
    }

    ZRAW_block_header_t header;
    struct
    {
        uint32_t bayer_mode;
        uint32_t cmp_mode;
        uint32_t is_lossless;
        uint32_t frame_width;
        uint32_t frame_height;
        uint32_t frame_bitdepth;
        uint32_t part_cmp_en;
        uint32_t pcmp_start_hpos;
        uint32_t pcmp_end_hpos;
        uint32_t stride_en;
        uint32_t stride;
        uint32_t align_mode;
        uint32_t debug_mode;
        uint32_t mb_bits;
        uint32_t smth_thr;
        uint32_t still_thr;
        uint32_t big_grad_thr;
        uint32_t smth_pix_num_thr;
        uint32_t still_pix_num_thr;
        uint32_t noise_pix_num_thr;
        uint32_t qp_inc1_bits_thr;
        uint32_t qp_inc2_bits_thr;
        uint32_t qp_dec1_bits_thr;
        uint32_t qp_dec2_bits_thr;
        uint32_t min_mb_bits;
        uint32_t qp_force_en;
        uint32_t first_mb_adj_bits;
        uint32_t first_row_adj_bits;
        uint32_t first_col_adj_bits;
        uint32_t buffer_init_bits;
        uint32_t buf_fullness_thr_reg0;
        uint32_t buf_fullness_thr_reg1;
        uint32_t buf_fullness_thr_reg2;
        uint32_t qp_rge_reg0;
        uint32_t qp_rge_reg1;
        uint32_t qp_rge_reg2;
        uint32_t bits_offset_reg0;
        uint32_t bits_offset_reg1;
        uint32_t bits_offset_reg2;
        uint32_t est_err_gain_map;
        uint32_t smooth_status_thr;
        uint32_t nl2_num_thr;
        uint32_t nl1_num_thr;
        uint32_t noise_grad_thr;
    } data;
} ZRAW_block_pram_t;

typedef struct ZRAW_block_dpc_s
{
    ZRAW_block_dpc_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('d', 'p', 'c', ' ');
        header.block_size = bswap_32(sizeof(ZRAW_block_dpc_s));

        data.iso_num = bswap_32(ZRAW_TEST_DPC_TABLES_COUNT);
        for(int i = 0; i < ZRAW_TEST_DPC_TABLES_COUNT; ++i)
        {
            data.tables[i].bright_pixels_count = bswap_32(sizeof(data.tables[i].bright_pixels_table) / sizeof(uint32_t));
            data.tables[i].dark_pixels_count = bswap_32(sizeof(data.tables[i].dark_pixels_table) / sizeof(uint32_t));
        }
    }

    ZRAW_block_header_t header;
    struct
    {
        uint32_t iso_num;
        struct
        {
            uint32_t a;
            uint32_t bright_pixels_count;
            uint32_t bright_pixels_table[ZRAW_TEST_DPC_TABLE_BRIGHT_PIXELS_COUNT];
            uint32_t dark_pixels_count;
            uint32_t dark_pixels_table[ZRAW_TEST_DPC_TABLE_DARK_PIXELS_COUNT];
        } tables[ZRAW_TEST_DPC_TABLES_COUNT];
    } data;
} ZRAW_block_dpc_t;

typedef struct ZRAW_block_awb_s
{
    ZRAW_block_awb_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('a', 'w', 'b', ' ');
        header.block_size = bswap_32(sizeof(ZRAW_block_awb_s));

        data.gain_red = 0;
        data.gain_green = 0;
        data.gain_blue = 0;
    }

    ZRAW_block_header_t header;
    struct
    {
        uint32_t gain_red;
        uint32_t gain_green;
        uint32_t gain_blue;
    } data;
} ZRAW_block_awb_t;

typedef struct ZRAW_block_blc_s
{
    ZRAW_block_blc_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('b', 'l', 'c', ' ');
        header.block_size = bswap_32(sizeof(ZRAW_block_blc_s));

        for (int i = 0; i < 4; ++i)
            data.levels_per_component[i] = 0;
    }

    ZRAW_block_header_t header;
    struct
    {
        uint16_t levels_per_component[4];
    } data;
} ZRAW_block_blc_t;

typedef struct ZRAW_block_meta_s
{
    ZRAW_block_meta_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('m', 'e', 't', 'a');
        header.block_size = bswap_32(sizeof(ZRAW_block_meta_s));
    }

    ZRAW_block_header_t header;
    struct
    {
        uint32_t metadata;
    } data;
} ZRAW_block_meta_t;

typedef struct ZRAW_block_ccm_s
{
    ZRAW_block_ccm_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('c', 'c', 'm', ' ');
        header.block_size = bswap_32(sizeof(ZRAW_block_ccm_s));

        data.matrices_count = bswap_32(ZRAW_TEST_CCM_MATRICES_COUNT);
    }

    ZRAW_block_header_t header;
    struct
    {
        uint32_t matrices_count;
        uint8_t aes_encrypted_data[ZRAW_TEST_CCM_AES_DATA_SIZE];
    } data;
} ZRAW_block_ccm_t;

typedef struct ZRAW_block_gamm_s
{
    ZRAW_block_gamm_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('g', 'a', 'm', 'm');
        header.block_size = bswap_32(sizeof(ZRAW_block_gamm_s));

        data.curve_points_count = bswap_32(ZRAW_TEST_GAMM_CURVE_POINTS_COUNT);
    }

    ZRAW_block_header_t header;
    struct
    {
        uint32_t curve_type;
        uint32_t curve_points_count;
        uint16_t curve_points[ZRAW_TEST_GAMM_CURVE_POINTS_COUNT];
    } data;
} ZRAW_block_gamm_t;

typedef struct ZRAW_block_data_s
{
    ZRAW_block_data_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('d', 'a', 't', 'a');
        header.block_size = bswap_32(sizeof(ZRAW_block_data_s));
    }

    ZRAW_block_header_t header;
    struct
    {
        struct
        {
            uint32_t count;
            uint8_t line_data[ZRAW_TEST_DATA_LINE_ELEMENTS];
        } lines[ZRAW_TEST_FRAME_HEIGHT];
    } data;
} ZRAW_block_data_t;

typedef struct ZRAW_example_frame_s
{
    ZRAW_example_frame_s()
    {
        header.block_magic = ZRAW_TEST_HEADER_NAME_MACRO('z', 'r', 'a', 'w');
        header.block_size = bswap_32(sizeof(ZRAW_example_frame_s));
    }

    ZRAW_block_header_t header;
    ZRAW_block_ver_t ver;
    ZRAW_block_pram_t pram;
    ZRAW_block_dpc_t dpc;
    ZRAW_block_awb_t awb;
    ZRAW_block_blc_t blc;
    ZRAW_block_meta_t meta;
    ZRAW_block_ccm_t ccm;
    ZRAW_block_gamm_t gamm;
    ZRAW_block_data_t data;
} ZRAW_example_frame_t;

#pragma pack(pop)
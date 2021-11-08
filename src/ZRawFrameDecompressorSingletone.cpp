#include "ZRawFrameDecompressorSingletone.hpp"

ZRawFrameDecompressorSingletone* ZRawFrameDecompressorSingletone::_instance = nullptr;

ZRawFrameDecompressorSingletone& ZRawFrameDecompressorSingletone::Instance()
{
    if (_instance == nullptr)
        _instance = new ZRawFrameDecompressorSingletone();
    return *_instance;
}

void ZRawFrameDecompressorSingletone::DecompressFrame(ZRawFrame &frame)
{
    // If compression mode is "raw" - frame is already decompressed
    if (frame.Parameters().cmp_mode != 0)
        return;

    // Each horizontal block has size of 64 bytes (32 x sizeof(uint16_t))
    uint32_t horizontal_blocks_count = (frame.Parameters().frame_width / 2 + 31) / 32;

    uint32_t linebuf_default_value = 1 << ((frame.Parameters().frame_bitdepth * 2 + 8) - 1);

    ZRawImageLineBlockReader::Parameters param;
    param.default_pix_value = linebuf_default_value;
    param.bitdepth_real = frame.Parameters().frame_bitdepth * 2 + 8;
    param.max_allowed_pixel_value = (1 << param.bitdepth_real) - 1;
    param.max_allowed_raw_value = 255;
    param.max_values_count = frame.Parameters().frame_width / 2;
    param.blocks_count = ((signed int)frame.Parameters().frame_width / 2 + 31) / 32;
    param.stride = frame.Parameters().stride;
    param.align_mode = frame.Parameters().align_mode;
    param.lossless = !(frame.Parameters().is_lossless == 0 && frame.Parameters().part_cmp_en == 0);
    param.bayer_pattern = frame.Parameters().bayer_mode;
    param.noise_level_1 = frame.Parameters().nl1_num_thr;
    param.noise_level_2 = frame.Parameters().nl2_num_thr;
    param.noise_level_distance = frame.Parameters().noise_grad_thr;

    ZRawImageLineBlockReader block_reader(param);

    // Process each compressed line
    for (int i = 0; i < frame.Parameters().frame_height; ++i)
    {
        std::istrstream databuf_stream(reinterpret_cast<const char *>(frame.Data()[i].data()), frame.Data()[i].size());

        // Create bitreader for line (cached)
        BitReader line_reader(databuf_stream, true);

        // This flag is true on each second line
        bool upper_field = (i & 1) == 0;

        // DCMPBAYER_V1_TopAvailAna

        // Skip first 16 bits if stride checking is enabled
        // FIXME: commented because these bits are skipped on reading process
        //if (frame.Parameters().is_stride_enabled)
        //    line_reader.ReadBits(16);

        std::vector<uint16_t> line_data;

        block_reader.ReadLine(&line_reader);
        auto line_a = block_reader.LineA();
        auto line_b = block_reader.LineB();

        for (int i = 0; i < line_a.size(); ++i)
        {
            line_data.push_back(line_a[i]);
            line_data.push_back(line_b[i]);
        }

        // Save read line to component vectors
        frame.Pixels().push_back(line_data);

        if (frame.Parameters().stride_en)
        {
            line_reader.BitAlignTo(128);
            line_reader.SeekToBit(128 * frame.Parameters().stride * (i + 1));
        }

        block_reader.FinalizeLine();
    }

    _post_process_correct_bayer(frame);
}

void ZRawFrameDecompressorSingletone::_post_process_correct_bayer(ZRawFrame &frame)
{
    uint32_t w = frame.Parameters().frame_width / 2;
    uint32_t h = frame.Parameters().frame_height / 2;

    uint32_t pat = frame.Parameters().bayer_mode;

    bool uv_swap = (pat == 0 || pat == 1);

    for (int i = 0; i < h; ++i)
        for (int p = 0; p < w; ++p)
        {
            uint16_t *a = &frame.Pixels()[2 * i].data()[2 * p];
            uint16_t *b = &frame.Pixels()[2 * i + 1].data()[2 * p];

            uint16_t y1 = a[0];
            uint16_t y2 = b[0];
            uint16_t u = uv_swap ? b[1] : a[1];
            uint16_t v = uv_swap ? a[1] : b[1];

            switch (pat)
            {
            case 0:
                a[0] = v;
                a[1] = y1;
                b[0] = y2;
                b[1] = u;
                break;
            case 1:
                a[0] = y1;
                a[1] = v;
                b[0] = u;
                b[1] = y2;
                break;
            case 2:
                a[0] = y1;
                a[1] = u;
                b[0] = v;
                b[1] = y2;
                break;
            case 3:
                a[0] = u;
                a[1] = y1;
                b[0] = y2;
                b[1] = v;
                break;
            }
        }
}
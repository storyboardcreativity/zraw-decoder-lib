#include "ZRawFrameDecompressorSingletone.hpp"

#include "ZRawImageLineBinaryParser.hpp"
#include "ZRawPostShaderProcessLinesSequentially.hpp"

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

    uint16_t** frame_low_level_data = new uint16_t*[frame.Parameters().frame_height];

    std::vector<std::vector<uint32_t>> frame_block_header_values;
    std::vector<std::vector<int>> frame_block_is_vl;

    // Step 1: decode lines using Golomb and Huffman
    for (int i = 0; i < frame.Parameters().frame_height; ++i)
    {
        std::istrstream databuf_stream(reinterpret_cast<const char *>(frame.Data()[i].data()), frame.Data()[i].size());

        // Create bitreader for line (cached)
        BitReader line_reader(databuf_stream, true);

        // This flag is true on each second line
        bool upper_field = (i & 1) == 0;

        ZRawImageLineBinaryParser::Parameters paramBin;
        paramBin.max_allowed_raw_value = 255;
        paramBin.max_values_count = frame.Parameters().frame_width / 2;
        paramBin.blocks_count = ((signed int)frame.Parameters().frame_width / 2 + 31) / 32;
        paramBin.stride = frame.Parameters().stride;
        paramBin.align_mode = frame.Parameters().align_mode;
        paramBin.lossless = !(frame.Parameters().is_lossless == 0 && frame.Parameters().part_cmp_en == 0);
        paramBin.bitdepth_real = frame.Parameters().frame_bitdepth * 2 + 8;

        std::vector<uint16_t> line_data;
        line_data.resize(paramBin.max_values_count * 2);

        paramBin.dest = line_data.data();
        ZRawImageLineBinaryParser bin_line_reader(paramBin);

        std::vector<uint32_t> header_values;
        std::vector<int> is_block_vl;
        bin_line_reader.Read(&line_reader, header_values, is_block_vl);

        frame_block_header_values.push_back(header_values);
        frame_block_is_vl.push_back(is_block_vl);

        // Save read line to component vectors
        frame.Pixels().push_back(line_data);

        frame_low_level_data[i] = frame.Pixels()[i].data();
    }

    uint32_t** frame_low_level_block_header_values = new uint32_t*[frame_block_header_values.size()];
    for (int i = 0; i < frame_block_header_values.size(); ++i)
        frame_low_level_block_header_values[i] = frame_block_header_values[i].data();

    int** frame_low_level_block_is_vl = new int*[frame_block_is_vl.size()];
    for (int i = 0; i < frame_block_is_vl.size(); ++i)
        frame_low_level_block_is_vl[i] = frame_block_is_vl[i].data();

    // Step 2: collect noise level statistics and fix VL pixel values
    ZRawPostShaderProcessLinesSequentially step_2;
    step_2.Process(
        frame_low_level_data,
        frame_low_level_block_is_vl,
        frame_low_level_block_header_values,
        frame.Parameters().bayer_mode,
        frame.Parameters().frame_width,
        frame.Parameters().frame_height,
        linebuf_default_value,
        frame.Parameters().noise_grad_thr,
        frame.Parameters().frame_bitdepth * 2 + 8,
        frame.Parameters().nl1_num_thr,
        frame.Parameters().nl2_num_thr
    );

    // Free temp ptr buffers
    delete[] frame_low_level_data;
    delete[] frame_low_level_block_header_values;

    // Step 3: correct Bayer pattern
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
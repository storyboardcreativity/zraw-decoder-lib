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

    int max_values_count = frame.Parameters().frame_width / 2;
    int blocks_count = ((signed int)frame.Parameters().frame_width / 2 + 31) / 32;

    ZRawImageLineBinaryParser::Parameters paramBin;
    paramBin.max_allowed_raw_value = 255;
    paramBin.stride = frame.Parameters().stride;
    paramBin.align_mode = frame.Parameters().align_mode;
    paramBin.lossless = !(frame.Parameters().is_lossless == 0 && frame.Parameters().part_cmp_en == 0);
    paramBin.bitdepth_real = frame.Parameters().frame_bitdepth * 2 + 8;

    ZRawPostShaderProcessLinesSequentially::Parameters paramPost;
    paramPost.width = frame.Parameters().frame_width;
    paramPost.height = frame.Parameters().frame_height;
    paramPost.bayer_pattern = frame.Parameters().bayer_mode;
    paramPost.default_pix_value = linebuf_default_value;
    paramPost.noise_level_distance = frame.Parameters().noise_grad_thr;
    paramPost.real_bitdepth = frame.Parameters().frame_bitdepth * 2 + 8;
    paramPost.nl1 = frame.Parameters().nl1_num_thr;
    paramPost.nl2 = frame.Parameters().nl2_num_thr;

    ZRawPostShaderProcessLinesSequentially step_2(paramPost);

    // Block info from step 1, step 2 uses it too
    std::vector<uint32_t> header_values(blocks_count);
    std::vector<int> is_block_vl(blocks_count);

    frame.Pixels().reserve(frame.Pixels().size() + frame.Parameters().frame_height);

    // Step 1 passes its values to step 2 pair by pair, so the two steps run interleaved.
    // Step 2 needs the previous lines, step 1 needs no other line.
    for (int i = 0; i < frame.Parameters().frame_height; ++i)
    {
        // Create bitreader for line
        BitReader line_reader(frame.Data()[i].data(), frame.Data()[i].size());

        ZRawImageLineBinaryParser bin_line_reader(paramBin);

        step_2.BeginLine();
        for (int block_index = 0; block_index < blocks_count; ++block_index)
        {
            bin_line_reader.ReadBlockHeader(&line_reader, block_index, header_values.data(), is_block_vl.data());
            step_2.BeginBlock(is_block_vl[block_index] != 0, header_values[block_index]);

            int pairs_count = max_values_count - block_index * ZRAW_LINE_BLOCK_SIZE;
            if (pairs_count > ZRAW_LINE_BLOCK_SIZE)
                pairs_count = ZRAW_LINE_BLOCK_SIZE;

            // Step 1: decode pairs of values using Golomb and Huffman
            // Step 2: collect noise level statistics and fix VL pixel values
            auto step_2_handler = [&step_2](int32_t value_a, int32_t value_b) { step_2.FixPair(value_a, value_b); };
            bin_line_reader.ReadPairs(&line_reader, pairs_count, step_2_handler);
        }
        bin_line_reader.EndLine(&line_reader);

        frame.Pixels().push_back(std::vector<uint16_t>(max_values_count * 2));

        // Step 3: correct Bayer pattern, done while the line is stored
        step_2.EndLine(header_values.data(), frame.Pixels().back().data());
    }
}

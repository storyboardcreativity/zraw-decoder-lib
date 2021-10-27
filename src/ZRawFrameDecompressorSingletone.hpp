#pragma once

#include <strstream>

#include "ZRawFrame.hpp"
#include "BitReader.hpp"
#include "ZRawImageLineBlockReader.hpp"
#include "ZRawFramePreProcessorSingletone.hpp"

class ZRawFrameDecompressorSingletone
{
    ZRawFrameDecompressorSingletone() {}
    static ZRawFrameDecompressorSingletone* _instance;

public:
    static ZRawFrameDecompressorSingletone& Instance();

    void DecompressFrame(ZRawFrame& frame);

    void _post_process_correct_bayer(ZRawFrame& frame);
};
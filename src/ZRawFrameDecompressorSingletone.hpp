#pragma once

#include "ZRawFrame.hpp"
#include "BitReader.hpp"
#include "ZRawFramePreProcessorSingletone.hpp"

class ZRawFrameDecompressorSingletone
{
    ZRawFrameDecompressorSingletone() {}
    static ZRawFrameDecompressorSingletone* _instance;

public:
    static ZRawFrameDecompressorSingletone& Instance();

    void DecompressFrame(ZRawFrame& frame);
};

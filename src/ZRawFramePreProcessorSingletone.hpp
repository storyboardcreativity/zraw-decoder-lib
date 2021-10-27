#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "byteswap.hpp"
#include "ZRawFrame.hpp"

class ZRawFramePreProcessorSingletone
{
    ZRawFramePreProcessorSingletone() {}
    static ZRawFramePreProcessorSingletone* _instance;

public:
    static ZRawFramePreProcessorSingletone& Instance();

    void PreProcess(ZRawFrame& frame);
};
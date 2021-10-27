#pragma once

#include <stdint.h>
#include <vector>

#include <ZRawLowLevelStructures.hpp>

class ZRawFrame
{
public:
    ZRawFrameVersion_t &Version()
    {
        return _version;
    }

    ZRawFrameParameters_t &Parameters()
    {
        return _parameters;
    }

    ZRawFrameDefectivePixelsInfo_t &DefectionPixelsTable()
    {
        return _defection;
    }

    ZRawFrameAutoWhiteBalanceInfo_t &AutoWhiteBalance()
    {
        return _auto_white_balance;
    }

    ZRawFrameSensorBlackLevels_t &SensorBlackLevels()
    {
        return _black_levels;
    }

    ZRawFrameMetaData_t &MetaData()
    {
        return _metadata;
    }

    ZRawFrameColorCorretionMatrices_t &ColorCorrectionMatrices()
    {
        return _color_corretion_matrices;
    }

    ZRawFrameGammaCurveInfo_t &GammaCurveInfo()
    {
        return _gamma_curve;
    }

    std::vector<std::vector<uint8_t>> &Data()
    {
        return _data;
    }

    std::vector<std::vector<uint16_t>> &Pixels()
    {
        return _pixels;
    }

private:
    ZRawFrameVersion_t _version;
    ZRawFrameParameters_t _parameters;
    ZRawFrameDefectivePixelsInfo_t _defection;
    ZRawFrameAutoWhiteBalanceInfo_t _auto_white_balance;
    ZRawFrameSensorBlackLevels_t _black_levels;
    ZRawFrameMetaData_t _metadata;
    ZRawFrameColorCorretionMatrices_t _color_corretion_matrices;
    ZRawFrameGammaCurveInfo_t _gamma_curve;
    std::vector<std::vector<uint8_t>> _data;
    std::vector<std::vector<uint16_t>> _pixels;
};
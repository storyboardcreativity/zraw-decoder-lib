#pragma once

#include <istream>
#include <string.h>

// For AES decryption
#include <openssl/aes.h>
#include <openssl/conf.h>
#include <openssl/evp.h>

#include "byteswap.hpp"
#include "ZRawFrame.hpp"

class ZRawFrameContainerParserSingletone
{
    ZRawFrameContainerParserSingletone() {}
    static ZRawFrameContainerParserSingletone* _instance;

public:
    static ZRawFrameContainerParserSingletone& Instance();

    ZRawFrame ParseFrame(std::istream& stream);
    void ParseFrame(ZRawFrame& frame, std::istream& stream);

private:
    int32_t _processBlock(std::istream& stream, ZRawFrame& frame);

    /* === Block processors == */

    int32_t _processZrawBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
    int32_t _processVersionBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
    int32_t _processParametersBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
    int32_t _processDefectivePixelsCorrectionBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
    int32_t _processAutoWhiteBalanceBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
    int32_t _processSensorBlackLevelsBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
    int32_t _processMetaDataBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
    int32_t _processColorCorrectionMatricesBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
    int32_t _processGammaCurveBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
    int32_t _processFrameDataBlock(std::istream& stream, ZRawFrame& frame, uint32_t data_size);
};
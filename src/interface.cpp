#define LIB_ZRAW_COMPILING
#include <libzraw.h>

#include <map>
#include <strstream>

#include <ZRawFrame.hpp>
#include <ZRawFrameContainerParserSingletone.hpp>
#include <ZRawFrameDecompressorSingletone.hpp>

class ZrawDecoderContext
{
public:
    ZRawFrame& frame__get() {return frame;}
private:
    ZRawFrame frame;
};

std::map<void*, ZrawDecoderContext*> g_zraw_decoding_contexts;

std::string g_last_exception_message;

// Creates decoder
extern "C" LIB_ZRAW_PUBLIC ZRAW_DECODER_HANDLE zraw_decoder__create()
{
    try
    {
        auto context = new ZrawDecoderContext;
        g_zraw_decoding_contexts[context] = context;
        return context;
    }
    catch(...)
    {
        return nullptr;
    }
}

// Reads ZRAW frame
extern "C" LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__read_hisi_frame(ZRAW_DECODER_HANDLE decoder, void* data, int max_size)
{
    try
    {
        auto it = g_zraw_decoding_contexts.find(decoder);
        if (it == g_zraw_decoding_contexts.end())
            return ZRAW_DECODER_STATE__INVALID_INSTANCE;

        // Create stream for data
        std::istrstream databuf_stream(reinterpret_cast<const char *>(data), max_size);

        // Parse ZRAW structure
        ZRawFrameContainerParserSingletone::Instance().ParseFrame(it->second->frame__get(), databuf_stream);
    }
    catch (const std::exception& ex)
    {
        g_last_exception_message = ex.what();
        return ZRAW_DECODER_STATE__EXCEPTION;
    }
    catch (...)
    {
        return ZRAW_DECODER_STATE__UNEXPECTED_FAILURE;
    }

    return ZRAW_DECODER_STATE__FRAME_IS_READ;
}

extern "C" LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__get_hisi_frame_info(ZRAW_DECODER_HANDLE decoder, zraw_frame_info_t& info_out)
{
    try
    {
        auto it = g_zraw_decoding_contexts.find(decoder);
        if (it == g_zraw_decoding_contexts.end())
            return ZRAW_DECODER_STATE__INVALID_INSTANCE;

        auto decoder_instance = it->second;

        auto& params = decoder_instance->frame__get().Parameters();
        info_out.width_in_photodiodes = params.frame_width;
        info_out.height_in_photodiodes = params.frame_height;
        info_out.bits_per_photodiode_value = 8 + params.frame_bitdepth * 2;

        auto& awb = decoder_instance->frame__get().AutoWhiteBalance();
        info_out.awb_gain_r = awb.gain_red;
        info_out.awb_gain_g = awb.gain_green;
        info_out.awb_gain_b = awb.gain_blue;

        auto& blc = decoder_instance->frame__get().SensorBlackLevels();
        info_out.cfa_black_levels[0] = blc.levels_per_component[0];
        info_out.cfa_black_levels[1] = blc.levels_per_component[1];
        info_out.cfa_black_levels[2] = blc.levels_per_component[2];
        info_out.cfa_black_levels[3] = blc.levels_per_component[3];

        auto& ccm = decoder_instance->frame__get().ColorCorrectionMatrices();
        for (int i = 0; i < ccm.matrices_count; ++i)
        {
            printf("=== CCM ===\n");
            auto& matrix = ccm.matrices[i];

            if (matrix.values[0] == 1000 &&
                matrix.values[1] == 0 &&
                matrix.values[2] == 0 &&
                matrix.values[3] == 0 &&
                matrix.values[4] == 1000 &&
                matrix.values[5] == 0 &&
                matrix.values[6] == 0 &&
                matrix.values[7] == 0 &&
                matrix.values[8] == 1000)
                info_out.wb_in_K = matrix.correlated_color_temperature;

            printf("TEMP = %d K\n", matrix.correlated_color_temperature);
            for (int p = 0; p < 3; ++p)
                printf("%d %d %d\n", matrix.values[p * 3 + 0], matrix.values[p * 3 + 1], matrix.values[p * 3 + 2]);
        }
    }
    catch (const std::exception& ex)
    {
        g_last_exception_message = ex.what();
        return ZRAW_DECODER_STATE__EXCEPTION;
    }
    catch (...)
    {
        return ZRAW_DECODER_STATE__UNEXPECTED_FAILURE;
    }

    return ZRAW_DECODER_STATE__STANDBY;
}

// Processes initialized frame data
extern "C" LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__decompress_hisi_frame(ZRAW_DECODER_HANDLE decoder)
{
    try
    {
        auto it = g_zraw_decoding_contexts.find(decoder);
        if (it == g_zraw_decoding_contexts.end())
            return ZRAW_DECODER_STATE__INVALID_INSTANCE;

        ZRawFrameDecompressorSingletone::Instance().DecompressFrame(it->second->frame__get());
        //ZRawFramePreProcessorSingletone::Instance().PreProcess(it->second->frame__get());
    }
    catch (const std::exception& ex)
    {
        g_last_exception_message = ex.what();
        return ZRAW_DECODER_STATE__EXCEPTION;
    }
    catch (...)
    {
        return ZRAW_DECODER_STATE__UNEXPECTED_FAILURE;
    }

    return ZRAW_DECODER_STATE__FRAME_IS_DECOMPRESSED;
}

// Writes decompressed CFA to external buffer
extern "C" LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__get_decompressed_CFA(ZRAW_DECODER_HANDLE decoder, uint16_t* cfa_buffer_out, int32_t size)
{
    try
    {
        auto it = g_zraw_decoding_contexts.find(decoder);
        if (it == g_zraw_decoding_contexts.end())
            return ZRAW_DECODER_STATE__INVALID_INSTANCE;

        zraw_frame_info_t info;
        auto state = zraw_decoder__get_hisi_frame_info(decoder, info);

        if (size < info.width_in_photodiodes * info.height_in_photodiodes * sizeof(uint16_t))
            return ZRAW_DECODER_STATE__NO_SPACE_TO_WRITE_CFA;

        auto decoder_instance = it->second;
        auto pixels = decoder_instance->frame__get().Pixels();

        for (int y = 0; y < info.height_in_photodiodes; ++y)
            for (int x = 0; x < info.width_in_photodiodes; ++x)
                cfa_buffer_out[y * info.width_in_photodiodes + x] = pixels[y][x];
    }
    catch (const std::exception& ex)
    {
        g_last_exception_message = ex.what();
        return ZRAW_DECODER_STATE__EXCEPTION;
    }
    catch (...)
    {
        return ZRAW_DECODER_STATE__UNEXPECTED_FAILURE;
    }

    return ZRAW_DECODER_STATE__STANDBY;
}

// Frees decoder
extern "C" LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__free(ZRAW_DECODER_HANDLE decoder)
{
    try
    {
        auto it = g_zraw_decoding_contexts.find(decoder);
        if (it == g_zraw_decoding_contexts.end())
            return ZRAW_DECODER_STATE__INVALID_INSTANCE;

        delete it->second;
        g_zraw_decoding_contexts.erase(it);
    }
    catch (const std::exception& ex)
    {
        g_last_exception_message = ex.what();
        return ZRAW_DECODER_STATE__EXCEPTION;
    }
    catch (...)
    {
        return ZRAW_DECODER_STATE__UNEXPECTED_FAILURE;
    }

    return ZRAW_DECODER_STATE__INSTANCE_IS_REMOVED;
}

// Returns last exception message
extern "C" LIB_ZRAW_PUBLIC const char* zraw_decoder__exception_message()
{
    return g_last_exception_message.c_str();
}
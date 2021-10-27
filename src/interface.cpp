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

// Creates decoder
extern "C" LIB_ZRAW_PUBLIC ZRAW_DECODER_HANDLE zraw_decoder__create()
{
    auto context = new ZrawDecoderContext;
    g_zraw_decoding_contexts[context] = context;
    return context;
}

// Reads ZRAW frame
extern "C" LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__read_hisi_frame(ZRAW_DECODER_HANDLE decoder, void* data, int max_size)
{
    auto it = g_zraw_decoding_contexts.find(decoder);
    if (it == g_zraw_decoding_contexts.end())
        return ZRAW_DECODER_STATE__INVALID_INSTANCE;
    
    // Create stream for data
    std::istrstream databuf_stream(reinterpret_cast<const char *>(data), max_size);

    // Parse ZRAW structure
    ZRawFrameContainerParserSingletone::Instance().ParseFrame(it->second->frame__get(), databuf_stream);

    return ZRAW_DECODER_STATE__FRAME_IS_READ;
}

extern "C" LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__get_hisi_frame_info(ZRAW_DECODER_HANDLE decoder, zraw_frame_info_t& info_out)
{
    auto it = g_zraw_decoding_contexts.find(decoder);
    if (it == g_zraw_decoding_contexts.end())
        return ZRAW_DECODER_STATE__INVALID_INSTANCE;

    auto decoder_instance = it->second;

    info_out.width_in_photodiodes = decoder_instance->frame__get().Parameters().frame_width;
    info_out.height_in_photodiodes = decoder_instance->frame__get().Parameters().frame_height;
    info_out.bits_per_photodiode_value = 8 + decoder_instance->frame__get().Parameters().frame_bitdepth * 2;

    return ZRAW_DECODER_STATE__STANDBY;
}

// Processes initialized frame data
extern "C" LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__decompress_hisi_frame(ZRAW_DECODER_HANDLE decoder)
{
    auto it = g_zraw_decoding_contexts.find(decoder);
    if (it == g_zraw_decoding_contexts.end())
        return ZRAW_DECODER_STATE__INVALID_INSTANCE;

    ZRawFrameDecompressorSingletone::Instance().DecompressFrame(it->second->frame__get());

    return ZRAW_DECODER_STATE__FRAME_IS_DECOMPRESSED;
}

// Frees decoder
extern "C" LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__free(ZRAW_DECODER_HANDLE decoder)
{
    auto it = g_zraw_decoding_contexts.find(decoder);
    if (it == g_zraw_decoding_contexts.end())
        return ZRAW_DECODER_STATE__INVALID_INSTANCE;

    delete it->second;
    g_zraw_decoding_contexts.erase(it);

    return ZRAW_DECODER_STATE__INSTANCE_IS_REMOVED;
}
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
extern "C" LIB_ZRAW_PUBLIC void zraw_decoder__read_hisi_frame(ZRAW_DECODER_HANDLE decoder, void* data, int max_size)
{
    auto it = g_zraw_decoding_contexts.find(decoder);
    if (it == g_zraw_decoding_contexts.end())
        return;
    
    // Create stream for data
    std::istrstream databuf_stream(reinterpret_cast<const char *>(data), max_size);

    // Parse ZRAW structure
    ZRawFrameContainerParserSingletone::Instance().ParseFrame(it->second->frame__get(), databuf_stream);
}

// Processes initialized frame data
extern "C" LIB_ZRAW_PUBLIC void zraw_decoder__decompress_hisi_frame(ZRAW_DECODER_HANDLE decoder)
{
    auto it = g_zraw_decoding_contexts.find(decoder);
    if (it == g_zraw_decoding_contexts.end())
        return;

    ZRawFrameDecompressorSingletone::Instance().DecompressFrame(it->second->frame__get());
}

// Frees decoder
extern "C" LIB_ZRAW_PUBLIC void zraw_decoder__free(ZRAW_DECODER_HANDLE decoder)
{
    auto it = g_zraw_decoding_contexts.find(decoder);
    if (it == g_zraw_decoding_contexts.end())
        return;

    delete it->second;
    g_zraw_decoding_contexts.erase(it);
}
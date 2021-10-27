typedef void* ZRAW_DECODER_HANDLE;

#ifdef _MSC_VER
    // Microsoft 
    #define EXPORT __declspec(dllexport)
    #define IMPORT __declspec(dllimport)
#elif __GNUC__
    // GCC
    #define EXPORT __attribute__((visibility("default")))
    #define IMPORT
#else
    // do nothing and hope for the best?
    #define EXPORT
    #define IMPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif

#ifdef LIB_ZRAW_COMPILING
#   define LIB_ZRAW_PUBLIC EXPORT
#else
#   define LIB_ZRAW_PUBLIC IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

    // Creates decoder
    LIB_ZRAW_PUBLIC ZRAW_DECODER_HANDLE zraw_decoder__create();

    // Reads ZRAW frame
    LIB_ZRAW_PUBLIC void zraw_decoder__read_hisi_frame(ZRAW_DECODER_HANDLE decoder, void* data, int max_size);

    // Processes initialized frame data
    LIB_ZRAW_PUBLIC void zraw_decoder__decompress_hisi_frame(ZRAW_DECODER_HANDLE decoder);

    // Frees decoder
    LIB_ZRAW_PUBLIC void zraw_decoder__free(ZRAW_DECODER_HANDLE decoder);

#ifdef __cplusplus
}
#endif

#include <stdint.h>

typedef void* ZRAW_DECODER_HANDLE;

#ifdef _MSC_VER
    // Microsoft
    #ifdef LIB_ZRAW_COMPILING
    #   define LIB_ZRAW_PUBLIC __declspec(dllexport)
    #else
    #   define LIB_ZRAW_PUBLIC __declspec(dllimport)
    #endif
#elif __GNUC__
    // GCC
    #ifdef LIB_ZRAW_COMPILING
    #   define LIB_ZRAW_PUBLIC __attribute__((visibility("default")))
    #else
    #   define LIB_ZRAW_PUBLIC
    #endif
#else
    // do nothing and hope for the best?
    #pragma error Unknown dynamic link import/export semantics.
#endif

#pragma pack(push, 1)
typedef struct zraw_frame_info_s
{
    uint32_t width_in_photodiodes;
    uint32_t height_in_photodiodes;
    uint32_t bits_per_photodiode_value;

} zraw_frame_info_t;
#pragma pack(pop)

typedef enum zraw_decoder_state_e
{
    ZRAW_DECODER_STATE__INVALID_INSTANCE = 0,
    ZRAW_DECODER_STATE__STANDBY,

    ZRAW_DECODER_STATE__NO_SPACE_TO_WRITE_CFA,

    ZRAW_DECODER_STATE__FRAME_IS_READ,
    ZRAW_DECODER_STATE__FRAME_READING_FAILED,

    ZRAW_DECODER_STATE__FRAME_IS_DECOMPRESSED,
    ZRAW_DECODER_STATE__FRAME_DECOMPRESSION_FAILED,

    ZRAW_DECODER_STATE__INSTANCE_IS_REMOVED
} zraw_decoder_state_t;

#ifdef __cplusplus
extern "C" {
#endif

    // Creates decoder
    LIB_ZRAW_PUBLIC ZRAW_DECODER_HANDLE zraw_decoder__create();

    // Reads ZRAW frame
    LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__read_hisi_frame(ZRAW_DECODER_HANDLE decoder, void* data, int max_size);

    // Returns current ZRAW frame info
    LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__get_hisi_frame_info(ZRAW_DECODER_HANDLE decoder, zraw_frame_info_t& info_out);

    // Processes initialized frame data
    LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__decompress_hisi_frame(ZRAW_DECODER_HANDLE decoder);

    // Writes decompressed CFA to external buffer
    LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__get_decompressed_CFA(ZRAW_DECODER_HANDLE decoder, uint16_t* cfa_buffer_out, int32_t size);

    // Frees decoder
    LIB_ZRAW_PUBLIC zraw_decoder_state_t zraw_decoder__free(ZRAW_DECODER_HANDLE decoder);

#ifdef __cplusplus
}
#endif

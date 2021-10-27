#include <stdint.h>
#include <acutest.h>

#include <libzraw.h>
#include <test_types.hpp>

void test__alloc_dealloc()
{
    auto decoder = zraw_decoder__create();
    TEST_ASSERT_(decoder != nullptr, "0x%lX != nullptr", (uint64_t)decoder);

    zraw_decoder__free(decoder);
}

void test__read_simple_frame()
{
    auto decoder = zraw_decoder__create();
    TEST_ASSERT_(decoder != nullptr, "0x%lX != nullptr", (uint64_t)decoder);

    ZRAW_example_frame_t example_frame;
    zraw_decoder__read_hisi_frame(decoder, &example_frame, sizeof(example_frame));

    zraw_decoder__free(decoder);
}

TEST_LIST =
{
    {"instance allocation and deallocation", test__alloc_dealloc},
    {"simple frame reading", test__read_simple_frame},
    {NULL, NULL}
};
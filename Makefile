CC := gcc
CFL := -shared -fPIC -fvisibility=hidden -fvisibility-inlines-hidden
CFL_OBJS := -ggdb -std=c++11 -Wno-deprecated -fPIC -c -fvisibility=hidden -fvisibility-inlines-hidden
CFL_TESTS := -ggdb -std=c++11 -fPIC -Wno-deprecated -fvisibility=hidden -fvisibility-inlines-hidden
TOOLCHAIN :=
OUT_LIBRARY_NAME := zraw
OUT_SHARED_LIB_NAME := lib$(OUT_LIBRARY_NAME).so
OUT_STATIC_LIB_NAME := lib$(OUT_LIBRARY_NAME).a
OUT_TESTS_NAME := lib$(OUT_LIBRARY_NAME)_tests
ARCH :=
INCS := -Iinclude
LIBS := -lcrypto -lpthread -lstdc++

BUILDDIR := build/
OBJDIR := $(BUILDDIR)obj/

# Sources
SOURCE_FILES := src/interface.cpp
SOURCE_FILES += src/ZRawFrameContainerParserSingletone.cpp
SOURCE_FILES += src/ZRawFrameDecompressorSingletone.cpp
SOURCE_FILES += src/ZRawFramePreProcessorSingletone.cpp
SOURCE_FILES += src/Tools.cpp

# Test sources
TEST_SOURCE_FILES := tests/tests.cpp

all: shared_lib

interface_obj:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/interface.cpp $(INCS) -Isrc -o $(OBJDIR)interface.o

zrawconparsin_obj:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/ZRawFrameContainerParserSingletone.cpp $(INCS) -Isrc -o $(OBJDIR)ZRawFrameContainerParserSingletone.o

zrawdcmpsin_obj:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/ZRawFrameDecompressorSingletone.cpp $(INCS) -Isrc -o $(OBJDIR)ZRawFrameDecompressorSingletone.o

zrawpprocsin_obj:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/ZRawFramePreProcessorSingletone.cpp $(INCS) -Isrc -o $(OBJDIR)ZRawFramePreProcessorSingletone.o

zrawtools_obj:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/Tools.cpp $(INCS) -Isrc -o $(OBJDIR)Tools.o

static_lib: interface_obj zrawconparsin_obj zrawdcmpsin_obj zrawpprocsin_obj zrawtools_obj
	ar rcs $(BUILDDIR)$(OUT_STATIC_LIB_NAME) $(OBJDIR)*.o

shared_lib: static_lib
	@mkdir -p $(BUILDDIR)
	$(TOOLCHAIN)$(CC) $(CFL) $(ARCH) -Wl,--whole-archive $(BUILDDIR)$(OUT_STATIC_LIB_NAME) -Wl,--no-whole-archive $(LIBS) -o $(BUILDDIR)$(OUT_SHARED_LIB_NAME)

test: shared_lib
	@mkdir -p $(BUILDDIR)
	$(TOOLCHAIN)$(CC) $(CFL_TESTS) $(ARCH) $(TEST_SOURCE_FILES) $(INCS) -Iacutest -Itests -lstdc++ -L./$(BUILDDIR) -l$(OUT_LIBRARY_NAME) -o $(BUILDDIR)$(OUT_TESTS_NAME)
	( LD_LIBRARY_PATH=$(shell pwd)/$(BUILDDIR) ./$(BUILDDIR)$(OUT_TESTS_NAME) )

clear:
	rm -rf $(BUILDDIR)

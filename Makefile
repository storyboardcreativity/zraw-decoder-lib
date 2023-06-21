# Get OS name
UNAME := $(shell uname)

CC := gcc
CFL := -O3 -shared -fPIC -fvisibility=hidden -fvisibility-inlines-hidden # -ggdb
CFL_OBJS := -O3 -std=c++11 -Wno-deprecated -fPIC -c -fvisibility=hidden -fvisibility-inlines-hidden
CFL_TESTS := -O3 -std=c++11 -fPIC -Wno-deprecated -fvisibility=hidden -fvisibility-inlines-hidden

ifeq ($(UNAME),Darwin)
MACOS_VERSION := 10.15
CFL += -mmacosx-version-min=$(MACOS_VERSION)
CFL_OBJS += -mmacosx-version-min=$(MACOS_VERSION)
CFL_TESTS += -mmacosx-version-min=$(MACOS_VERSION)
endif

TOOLCHAIN :=
OUT_LIBRARY_NAME := zraw
OUT_SHARED_LIB_NAME := lib$(OUT_LIBRARY_NAME).so
OUT_STATIC_LIB_NAME := lib$(OUT_LIBRARY_NAME).a
OUT_TESTS_NAME := lib$(OUT_LIBRARY_NAME)_tests
ARCH :=

INCS := -Iinclude
LIBS := -lcrypto -lpthread -lstdc++

ifeq ($(UNAME),Darwin)
INCS_X86_64 := $(INCS) -I$(shell x86_64-brew --prefix openssl)/include
INCS_ARM64 :=  $(INCS) -I$(shell arm64-brew --prefix openssl)/include
LIBS_X86_64 := -L$(shell x86_64-brew --prefix openssl)/lib -lpthread -lstdc++
LIBS_ARM64 :=  -L$(shell arm64-brew --prefix openssl)/lib $(LIBS)
LIB_OPENSSL_X86_64 := $(shell x86_64-brew --prefix openssl)/lib/libcrypto.a
LIB_OPENSSL_ARM64 := $(shell arm64-brew --prefix openssl)/lib/libcrypto.a
ARCH := -arch x86_64 -arch arm64
endif

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
ifeq ($(UNAME),Darwin)
	@mkdir -p $(OBJDIR)x86_64
	@mkdir -p $(OBJDIR)arm64
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch x86_64 src/interface.cpp $(INCS_X86_64) -Isrc -o $(OBJDIR)x86_64/interface.o
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch arm64 src/interface.cpp $(INCS_ARM64) -Isrc -o $(OBJDIR)arm64/interface.o
else
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/interface.cpp $(INCS) -Isrc -o $(OBJDIR)interface.o
endif

zrawconparsin_obj:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
ifeq ($(UNAME),Darwin)
	@mkdir -p $(OBJDIR)x86_64
	@mkdir -p $(OBJDIR)arm64
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch x86_64 src/ZRawFrameContainerParserSingletone.cpp $(INCS_X86_64) -Isrc -o $(OBJDIR)x86_64/ZRawFrameContainerParserSingletone.o
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch arm64 src/ZRawFrameContainerParserSingletone.cpp $(INCS_ARM64) -Isrc -o $(OBJDIR)arm64/ZRawFrameContainerParserSingletone.o
else
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/ZRawFrameContainerParserSingletone.cpp $(INCS) -Isrc -o $(OBJDIR)ZRawFrameContainerParserSingletone.o
endif

zrawdcmpsin_obj:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
ifeq ($(UNAME),Darwin)
	@mkdir -p $(OBJDIR)x86_64
	@mkdir -p $(OBJDIR)arm64
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch x86_64 src/ZRawFrameDecompressorSingletone.cpp $(INCS_X86_64) -Isrc -o $(OBJDIR)x86_64/ZRawFrameDecompressorSingletone.o
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch arm64 src/ZRawFrameDecompressorSingletone.cpp $(INCS_ARM64) -Isrc -o $(OBJDIR)arm64/ZRawFrameDecompressorSingletone.o
else
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/ZRawFrameDecompressorSingletone.cpp $(INCS) -Isrc -o $(OBJDIR)ZRawFrameDecompressorSingletone.o
endif

zrawpprocsin_obj:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
ifeq ($(UNAME),Darwin)
	@mkdir -p $(OBJDIR)x86_64
	@mkdir -p $(OBJDIR)arm64
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch x86_64 src/ZRawFramePreProcessorSingletone.cpp $(INCS_X86_64) -Isrc -o $(OBJDIR)x86_64/ZRawFramePreProcessorSingletone.o
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch arm64 src/ZRawFramePreProcessorSingletone.cpp $(INCS_ARM64) -Isrc -o $(OBJDIR)arm64/ZRawFramePreProcessorSingletone.o
else
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/ZRawFramePreProcessorSingletone.cpp $(INCS) -Isrc -o $(OBJDIR)ZRawFramePreProcessorSingletone.o
endif

zrawtools_obj:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(OBJDIR)
ifeq ($(UNAME),Darwin)
	@mkdir -p $(OBJDIR)x86_64
	@mkdir -p $(OBJDIR)arm64
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch x86_64 src/Tools.cpp $(INCS_X86_64) -Isrc -o $(OBJDIR)x86_64/Tools.o
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) -arch arm64 src/Tools.cpp $(INCS_ARM64) -Isrc -o $(OBJDIR)arm64/Tools.o
else
	$(TOOLCHAIN)$(CC) $(CFL_OBJS) $(ARCH) src/Tools.cpp $(INCS) -Isrc -o $(OBJDIR)Tools.o
endif

static_lib: interface_obj zrawconparsin_obj zrawdcmpsin_obj zrawpprocsin_obj zrawtools_obj
ifeq ($(UNAME),Darwin)
	@rm -f $(BUILDDIR)$(OUT_STATIC_LIB_NAME)

# Make ARM64 static binary
	ar rcs $(BUILDDIR)_arm64_$(OUT_STATIC_LIB_NAME) $(OBJDIR)arm64/*.o
	libtool -static -o $(BUILDDIR)arm64_$(OUT_STATIC_LIB_NAME) $(BUILDDIR)_arm64_$(OUT_STATIC_LIB_NAME) $(LIB_OPENSSL_ARM64)
	@rm -f $(BUILDDIR)_arm64_$(OUT_STATIC_LIB_NAME)

# Make X86_64 static binary
	ar rcs $(BUILDDIR)_x86_64_$(OUT_STATIC_LIB_NAME) $(OBJDIR)x86_64/*.o
	libtool -static -o $(BUILDDIR)x86_64_$(OUT_STATIC_LIB_NAME) $(BUILDDIR)_x86_64_$(OUT_STATIC_LIB_NAME) $(LIB_OPENSSL_X86_64)
	@rm -f $(BUILDDIR)_x86_64_$(OUT_STATIC_LIB_NAME)

# Merge static binaries into the universal package
	lipo $(BUILDDIR)x86_64_$(OUT_STATIC_LIB_NAME) $(BUILDDIR)arm64_$(OUT_STATIC_LIB_NAME) -create -output $(BUILDDIR)$(OUT_STATIC_LIB_NAME)
else
	ar rcs $(BUILDDIR)$(OUT_STATIC_LIB_NAME) $(OBJDIR)*.o
endif

shared_lib: static_lib
	@mkdir -p $(BUILDDIR)
ifeq ($(UNAME),Darwin)
else
	$(TOOLCHAIN)$(CC) $(CFL) $(ARCH) -Wl,--whole-archive $(BUILDDIR)$(OUT_STATIC_LIB_NAME) -Wl,--no-whole-archive $(LIBS) -o $(BUILDDIR)$(OUT_SHARED_LIB_NAME)
endif

test: shared_lib
	@mkdir -p $(BUILDDIR)
	$(TOOLCHAIN)$(CC) $(CFL_TESTS) $(ARCH) $(TEST_SOURCE_FILES) $(INCS) -Iacutest -Itests -lstdc++ -L./$(BUILDDIR) -l$(OUT_LIBRARY_NAME) -o $(BUILDDIR)$(OUT_TESTS_NAME)
	( LD_LIBRARY_PATH=$(shell pwd)/$(BUILDDIR) ./$(BUILDDIR)$(OUT_TESTS_NAME) )

clear:
	rm -rf $(BUILDDIR)

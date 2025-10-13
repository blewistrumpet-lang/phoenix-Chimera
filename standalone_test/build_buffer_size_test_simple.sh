#!/bin/bash

# Simple build script for buffer size independence test using pre-compiled JUCE modules

set -e

echo "=========================================================================="
echo "  Building Buffer Size Independence Test (Using Pre-compiled JUCE)"
echo "=========================================================================="
echo ""

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"

# Compiler settings
CXX="clang++"
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"

# Include paths
INCLUDES="-I. -I${PLUGIN_SRC} -I${PLUGIN_SRC}/../JuceLibraryCode -I${JUCE_DIR}/modules"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation \
            -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security \
            -framework QuartzCore -framework CoreImage -framework CoreGraphics \
            -framework CoreText -framework WebKit -framework DiscRecording"

# Defines
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
         -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# Create build directory
mkdir -p ${BUILD_DIR}

echo "Step 1: Compiling test source..."
echo "-----------------------------------"

echo "  Compiling test_buffer_size_independence.cpp..."
${CXX} ${CPP_FLAGS} ${INCLUDES} ${DEFINES} -c \
    test_buffer_size_independence.cpp \
    -o ${BUILD_DIR}/test_buffer_size_independence.o

echo ""
echo "Step 2: Linking executable..."
echo "-----------------------------------"

# Use pre-compiled JUCE objects from standalone_test directory
JUCE_OBJS="./juce_core.o ./juce_audio_basics.o ./juce_dsp.o"

# Collect engine objects from build/obj
ENGINE_OBJS=$(find ./build/obj -name "*.o" -not -name "juce_*" -not -name "test_*" -not -name "SheenBidi.o" 2>/dev/null | tr '\n' ' ')

echo "  Linking test_buffer_size_independence..."
${CXX} ${CPP_FLAGS} \
    ${BUILD_DIR}/test_buffer_size_independence.o \
    ${JUCE_OBJS} \
    ${ENGINE_OBJS} \
    ${FRAMEWORKS} \
    -L/opt/homebrew/lib -lharfbuzz \
    -o ${BUILD_DIR}/test_buffer_size_independence

echo ""
echo "=========================================================================="
echo "  Build complete!"
echo "=========================================================================="
echo ""
echo "Executable: ${BUILD_DIR}/test_buffer_size_independence"
echo ""
echo "To run the test:"
echo "  cd ${BUILD_DIR}"
echo "  ./test_buffer_size_independence"
echo ""
echo "This will generate:"
echo "  - buffer_size_independence_report.txt"
echo "  - buffer_size_independence_results.csv"
echo ""

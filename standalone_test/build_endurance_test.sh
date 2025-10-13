#!/bin/bash

# Build script for Endurance Test Suite
# Tests reverbs and time-based effects for 5+ minutes each

set -e

echo "════════════════════════════════════════════════════════════"
echo "  Building Endurance Test for Reverbs & Time-Based Effects"
echo "════════════════════════════════════════════════════════════"

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

# Check if main build exists
if [ ! -d "$OBJ_DIR" ]; then
    echo "ERROR: Main build must be completed first (run build_v2.sh)"
    exit 1
fi

# Compiler flags (matching existing build system)
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo ""
echo "Compiling compilation time stub..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c juce_compilation_time_stub.cpp \
    -o "$OBJ_DIR/juce_compilation_time_stub.o"

echo "Compiling endurance_test.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c endurance_test.cpp \
    -o "$OBJ_DIR/endurance_test.o"

echo "Linking endurance test..."
# Get all object files except other test files and problematic ones
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" \
    ! -name "standalone_test.o" \
    ! -name "reverb_test.o" \
    ! -name "filter_test.o" \
    ! -name "distortion_test.o" \
    ! -name "dynamics_test.o" \
    ! -name "pitch_test.o" \
    ! -name "modulation_test.o" \
    ! -name "spatial_test.o" \
    ! -name "utility_test_simple.o" \
    ! -name "utility_test.o" \
    ! -name "juce_core_CompilationTime.o" \
    ! -name "endurance_test.o" \
    ! -name "test_comprehensive_thd.o" \
    ! -name "ComprehensiveTHDEngineFactory.o" \
    ! -name "juce_compilation_time_stub.o" \
    ! -name "cpu_benchmark_main.o" \
    ! -name "*test*.o" \
    ! -name "*benchmark*.o" \
    | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/endurance_test.o" \
    "$OBJ_DIR/juce_compilation_time_stub.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/endurance_test"

if [ $? -eq 0 ]; then
    echo ""
    echo "════════════════════════════════════════════════════════════"
    echo "  Build Complete!"
    echo "════════════════════════════════════════════════════════════"
    echo ""
    echo "Run the test:"
    echo "  cd $BUILD_DIR && ./endurance_test [duration_minutes]"
    echo ""
    echo "Examples:"
    echo "  cd $BUILD_DIR && ./endurance_test     # 5-minute tests (default)"
    echo "  cd $BUILD_DIR && ./endurance_test 10  # 10-minute tests"
    echo "  cd $BUILD_DIR && ./endurance_test 1   # Quick 1-minute tests"
    echo ""
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

#!/bin/bash
# Build script for Distortion Test Engines 21-23

set -e

echo "Building Distortion Test for Engines 21-23..."
echo "============================================="

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

if [ ! -d "$OBJ_DIR" ]; then
    echo "ERROR: Main build must be completed first (object files not found)"
    echo "Run: ./build_all.sh"
    exit 1
fi

# Compiler flags
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo "Compiling test_distortion_21_23_simple.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_distortion_21_23_simple.cpp \
    -o "$OBJ_DIR/test_distortion_21_23_simple.o"

echo "Linking test executable..."
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" \
    ! -name "standalone_test.o" \
    ! -name "reverb_test.o" \
    ! -name "validate_reverb_test.o" \
    ! -name "validate_reverb_test_nodamp.o" \
    ! -name "dynamics_test.o" \
    ! -name "filter_test.o" \
    ! -name "distortion_test.o" \
    ! -name "modulation_test.o" \
    ! -name "pitch_test.o" \
    ! -name "spatial_test.o" \
    ! -name "utility_test.o" \
    ! -name "utility_test_simple.o" \
    ! -name "test_dynamics_engines.o" \
    ! -name "test_8_engines_regression.o" \
    ! -name "test_engine7.o" \
    ! -name "test_engine49_warmup.o" \
    ! -name "test_engines_21_23.o" \
    ! -name "test_*.o" \
    ! -name "juce_build_info.o" \
    ! -name "*_test*.o" | grep -v "test_distortion_21_23_simple.o" | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_distortion_21_23_simple.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_distortion_21_23"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Run test with:"
    echo "  $BUILD_DIR/test_distortion_21_23"
    echo ""
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

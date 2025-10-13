#!/bin/bash
# Build Engine 7 (Parametric EQ) Test Suite

set -e

echo "════════════════════════════════════════════════════════"
echo "  Building Engine 7 Parametric EQ Test Suite"
echo "════════════════════════════════════════════════════════"

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"

# Use existing object files from main build
OBJ_DIR="$BUILD_DIR/obj"

if [ ! -d "$OBJ_DIR" ]; then
    echo "ERROR: Main build must be completed first (run build_all.sh)"
    exit 1
fi

# Compiler flags
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo ""
echo "Step 1: Compiling test_engine7_parametric_eq.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_engine7_parametric_eq.cpp \
    -o "$OBJ_DIR/test_engine7.o"

echo "Step 2: Linking test executable..."
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" ! -name "standalone_test.o" ! -name "reverb_test.o" ! -name "validate_reverb_test.o" ! -name "dynamics_test.o" ! -name "filter_test.o" ! -name "distortion_test.o" ! -name "modulation_test.o" ! -name "pitch_test.o" ! -name "spatial_test.o" ! -name "utility_test.o" ! -name "utility_test_simple.o" ! -name "test_engine7.o" ! -name "juce_build_info.o" | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_engine7.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_engine7"

if [ $? -eq 0 ]; then
    echo ""
    echo "════════════════════════════════════════════════════════"
    echo "  ✓ Build successful!"
    echo "════════════════════════════════════════════════════════"
    echo ""
    echo "Run the test with:"
    echo "  cd $BUILD_DIR && ./test_engine7"
    echo ""
    echo "Output files will be generated:"
    echo "  - impulse_engine_7.csv"
    echo "  - frequency_response_engine_7.csv"
    echo "  - engine_7_test_summary.txt"
    echo ""
else
    echo ""
    echo "════════════════════════════════════════════════════════"
    echo "  ✗ Build failed!"
    echo "════════════════════════════════════════════════════════"
    exit 1
fi

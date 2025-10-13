#!/bin/bash
# Build reverb deep analysis test

set -e

echo "Building Reverb Deep Analysis Test..."
echo "======================================"

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"

# Use existing object files from main build
OBJ_DIR="$BUILD_DIR/obj"

if [ ! -d "$OBJ_DIR" ]; then
    echo "ERROR: Main build must be completed first (run build_v2.sh)"
    exit 1
fi

# Compiler flags
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo "Compiling reverb_test.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c reverb_test.cpp \
    -o "$OBJ_DIR/reverb_test.o"

echo "Linking reverb test..."
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" ! -name "standalone_test.o" ! -name "reverb_test.o" ! -name "filter_test.o" ! -name "distortion_test.o" ! -name "dynamics_test.o" ! -name "pitch_test.o" ! -name "modulation_test.o" ! -name "spatial_test.o" ! -name "utility_test_simple.o" ! -name "utility_test.o" ! -name "juce_core_CompilationTime.o" | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/reverb_test.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/reverb_test"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Run reverb analysis with:"
    echo "  cd $BUILD_DIR && ./reverb_test"
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

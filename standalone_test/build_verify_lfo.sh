#!/bin/bash
# Build and run LFO verification test

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║         LFO Calibration Fix Verification Build            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Paths
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

if [ ! -d "$OBJ_DIR" ]; then
    echo "ERROR: Main build must be completed first"
    exit 1
fi

# Compiler flags
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo "[1/3] Compiling verify_lfo_constants.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c verify_lfo_constants.cpp \
    -o "$OBJ_DIR/verify_lfo_constants.o"

if [ $? -ne 0 ]; then
    echo "✗ Compilation failed"
    exit 1
fi

echo "[2/3] Linking verification test..."
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" \
    ! -name "standalone_test.o" \
    ! -name "test_*.o" \
    ! -name "*_test.o" \
    ! -name "*_test_simple.o" \
    ! -name "verify_*.o" \
    ! -name "juce_core_CompilationTime.o" \
    ! -name "juce_compilation_time_stub.o" \
    ! -name "SpectralGate_Platinum_new.o" \
    ! -name "SMBPitchShiftFixed_standalone.o" \
    ! -name "PhaseAlign_Platinum_fixed.o" \
    | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/verify_lfo_constants.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/verify_lfo_constants"

if [ $? -ne 0 ]; then
    echo "✗ Linking failed"
    exit 1
fi

echo "✓ Build successful"
echo ""
echo "[3/3] Running verification..."
echo ""

cd "$BUILD_DIR"
./verify_lfo_constants

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║              VERIFICATION COMPLETE                         ║"
echo "╚════════════════════════════════════════════════════════════╝"

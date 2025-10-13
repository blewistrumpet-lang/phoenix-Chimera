#!/bin/bash
# Build script for LFO Calibration Test
# Tests engines 23, 24, 27, 28 LFO frequency calibration

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║       Building ChimeraPhoenix LFO Calibration Test        ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Paths
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

echo "[1/3] Compiling test_lfo_calibration.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_lfo_calibration.cpp \
    -o "$OBJ_DIR/test_lfo_calibration.o"

if [ $? -ne 0 ]; then
    echo "✗ Compilation failed"
    exit 1
fi

echo "[2/3] Linking LFO calibration test..."
# Exclude all test files and duplicate implementations, but keep juce_stubs
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" \
    ! -name "standalone_test.o" \
    ! -name "test_*.o" \
    ! -name "*_test.o" \
    ! -name "*_test_simple.o" \
    ! -name "juce_core_CompilationTime.o" \
    ! -name "juce_compilation_time_stub.o" \
    ! -name "SpectralGate_Platinum_new.o" \
    ! -name "SMBPitchShiftFixed_standalone.o" \
    ! -name "PhaseAlign_Platinum_fixed.o" \
    | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_lfo_calibration.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_lfo_calibration"

if [ $? -ne 0 ]; then
    echo "✗ Linking failed"
    exit 1
fi

echo "✓ Build successful"
echo ""
echo "[3/3] Running LFO calibration tests..."
echo ""

cd "$BUILD_DIR"
./test_lfo_calibration

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                LFO CALIBRATION TEST COMPLETE               ║"
echo "╚════════════════════════════════════════════════════════════╝"

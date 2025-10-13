#!/bin/bash
################################################################################
# build_phased_vocoder_test.sh - Build and test PhasedVocoder Engine 49
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
OBJ_DIR="$BUILD_DIR/obj"
SOURCE_DIR="$SCRIPT_DIR/../JUCE_Plugin/Source"
JUCE_DIR="/Users/Branden/JUCE"

echo "════════════════════════════════════════════════════════════════"
echo "Building PhasedVocoder Test (Engine 49)"
echo "════════════════════════════════════════════════════════════════"

# Create build directory
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Common flags
INCLUDES="-I. -I$SOURCE_DIR -I$SOURCE_DIR/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"

# Recompile PhasedVocoder.cpp with updated fixes
echo "Recompiling PhasedVocoder.cpp with fixes..."
if clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$SOURCE_DIR/PhasedVocoder.cpp" \
    -o "$OBJ_DIR/PhasedVocoder_fixed.o" 2>&1; then
    echo "  ✓ PhasedVocoder compiled"
else
    echo "  ✗ PhasedVocoder compilation failed"
    exit 1
fi

# Compile test source
echo "Compiling test_phased_vocoder_fix.cpp..."
if clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$SCRIPT_DIR/test_phased_vocoder_fix.cpp" \
    -o "$OBJ_DIR/test_phased_vocoder_fix.o" 2>&1; then
    echo "  ✓ Test compiled"
else
    echo "  ✗ Test compilation failed"
    exit 1
fi

# Link executable (with needed JUCE modules only)
echo "Linking executable..."
if clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_phased_vocoder_fix.o" \
    "$OBJ_DIR/PhasedVocoder_fixed.o" \
    "$OBJ_DIR/juce_stubs.o" \
    "$OBJ_DIR/juce_core.o" \
    "$OBJ_DIR/juce_audio_basics.o" \
    "$OBJ_DIR/juce_dsp.o" \
    "$OBJ_DIR/juce_events.o" \
    "$OBJ_DIR/juce_data_structures.o" \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_phased_vocoder_fix" 2>&1; then
    echo "  ✓ Build successful!"
    echo ""
    echo "Executable: $BUILD_DIR/test_phased_vocoder_fix"
    echo ""
    echo "════════════════════════════════════════════════════════════════"
    echo "Running PhasedVocoder Test..."
    echo "════════════════════════════════════════════════════════════════"
    echo ""
    cd "$SCRIPT_DIR" && ./build/test_phased_vocoder_fix
else
    echo "  ✗ Linking failed"
    exit 1
fi

#!/bin/bash
# Build PhasedVocoder test

set -e

echo "Building PhasedVocoder Test..."

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo "Compiling test_phasedvocoder.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_phasedvocoder.cpp \
    -o "$OBJ_DIR/test_phasedvocoder.o"

echo "Recompiling PhasedVocoder.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$PLUGIN_SRC/PhasedVocoder.cpp" \
    -o "$OBJ_DIR/PhasedVocoder.o"

echo "Linking test..."
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" \
    ! -name "standalone_test.o" \
    ! -name "reverb_test.o" \
    ! -name "validate_reverb_test.o" \
    ! -name "filter_test.o" \
    ! -name "distortion_test.o" \
    ! -name "dynamics_test.o" \
    ! -name "pitch_test.o" \
    ! -name "modulation_test.o" \
    ! -name "spatial_test.o" \
    ! -name "utility_test_simple.o" \
    ! -name "utility_test.o" \
    ! -name "test_phasedvocoder.o" \
    ! -name "test_conv_fix.o" \
    ! -name "test_conv_41.o" \
    ! -name "test_feedbacknetwork_fix.o" \
    ! -name "juce_core_CompilationTime.o" | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_phasedvocoder.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_phasedvocoder"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Run test with:"
    echo "  cd $BUILD_DIR && ./test_phasedvocoder"
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

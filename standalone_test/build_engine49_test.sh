#!/bin/bash
# Build Engine 49 warmup test

set -e

echo "Building Engine 49 Warmup Test..."

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# Ensure directories exist
mkdir -p "$OBJ_DIR"

echo "Compiling test_engine49_warmup.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_engine49_warmup.cpp \
    -o "$OBJ_DIR/test_engine49_warmup.o"

echo "Recompiling PhasedVocoder.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$PLUGIN_SRC/PhasedVocoder.cpp" \
    -o "$OBJ_DIR/PhasedVocoder_test49.o"

echo "Compiling JUCE stubs..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c juce_stubs.cpp \
    -o "$OBJ_DIR/juce_core_CompilationTime.o"

echo "Compiling MidSideProcessor_Platinum.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$PLUGIN_SRC/MidSideProcessor_Platinum.cpp" \
    -o "$OBJ_DIR/MidSideProcessor_Platinum_test49.o"

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
    ! -name "test_dynamics_engines.o" | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_engine49_warmup.o" \
    "$OBJ_DIR/PhasedVocoder_test49.o" \
    "$OBJ_DIR/MidSideProcessor_Platinum_test49.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_engine49_warmup"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Run test with:"
    echo "  cd $BUILD_DIR && ./test_engine49_warmup"
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

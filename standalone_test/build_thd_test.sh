#!/bin/bash
# Build script for DetuneDoubler THD Test (Bug #6 Verification)

set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║    Building DetuneDoubler THD Test (Engine 32 - Bug #6)      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Paths
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

# Compiler flags
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo "[1/2] Compiling test_detunedoubler_thd.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_detunedoubler_thd.cpp \
    -o "$OBJ_DIR/test_detunedoubler_thd.o"

if [ $? -ne 0 ]; then
    echo "✗ Compilation failed"
    exit 1
fi

echo "[2/2] Linking test executable..."
# Collect all object files except test mains
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" ! -name "standalone_test.o" ! -name "reverb_test.o" ! -name "modulation_test.o" ! -name "validate_reverb_test.o" ! -name "filter_test.o" ! -name "distortion_test.o" ! -name "dynamics_test.o" ! -name "pitch_test.o" ! -name "spatial_test.o" ! -name "utility_test.o" ! -name "*test_simple.o" ! -name "juce_core_CompilationTime.o" ! -name "test_phasedvocoder.o" ! -name "test_conv_fix.o" ! -name "test_feedbacknetwork_fix.o" ! -name "test_conv_41.o" ! -name "test_conv41_damping.o" ! -name "validate_reverb_test_nodamp.o" ! -name "test_phasedvocoder_simple.o" | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_detunedoubler_thd.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_detunedoubler_thd"

if [ $? -ne 0 ]; then
    echo "✗ Linking failed"
    exit 1
fi

echo "✓ Build successful"
echo ""
echo "Executable: $BUILD_DIR/test_detunedoubler_thd"
echo ""

#!/bin/bash
# Build script for DynamicEQ THD Test

set -e

echo "Building DynamicEQ THD Test..."
echo "=============================="

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

if [ ! -d "$OBJ_DIR" ]; then
    echo "ERROR: Main build must be completed first (object files not found)"
    exit 1
fi

# Compiler flags - DEBUG mode to match juce_core, juce_dsp, juce_audio_basics
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo "Compiling test_dynamiceq_thd.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_dynamiceq_thd.cpp \
    -o "$OBJ_DIR/test_dynamiceq_thd.o"

# Recompile DynamicEQ.cpp with updated code
echo "Recompiling DynamicEQ.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c $PLUGIN_SRC/DynamicEQ.cpp \
    -o "$OBJ_DIR/DynamicEQ.o"

echo "Linking test executable..."
# Link only JUCE modules in DEBUG mode (core, dsp, audio_basics)
# Exclude RELEASE mode modules (audio_formats, gui_*, etc)
clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_dynamiceq_thd.o" \
    "$OBJ_DIR/DynamicEQ.o" \
    "$OBJ_DIR/juce_core.o" \
    "$OBJ_DIR/juce_audio_basics.o" \
    "$OBJ_DIR/juce_dsp.o" \
    $FRAMEWORKS \
    -o "$BUILD_DIR/test_dynamiceq_thd"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Run test with:"
    echo "  $BUILD_DIR/test_dynamiceq_thd"
    echo ""
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

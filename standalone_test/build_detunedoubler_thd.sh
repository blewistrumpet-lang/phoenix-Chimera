#!/bin/bash
# Build script for DetuneDoubler THD Test

set -e

echo "Building DetuneDoubler THD Test..."
echo "===================================="

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

echo "Compiling JUCE stubs..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c juce_stubs.cpp \
    -o "$OBJ_DIR/juce_stubs.o"

echo "Compiling test_detunedoubler_thd.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_detunedoubler_thd.cpp \
    -o "$OBJ_DIR/test_detunedoubler_thd.o"

# Recompile DetuneDoubler.cpp with updated code
echo "Recompiling DetuneDoubler.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c $PLUGIN_SRC/DetuneDoubler.cpp \
    -o "$OBJ_DIR/DetuneDoubler.o"

# Recompile PitchShifter and dependencies in DEBUG mode
echo "Recompiling PitchShifter..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c $PLUGIN_SRC/PitchShifter.cpp \
    -o "$OBJ_DIR/PitchShifter.o"

echo "Recompiling SMBPitchShiftFixed..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c $PLUGIN_SRC/SMBPitchShiftFixed.cpp \
    -o "$OBJ_DIR/SMBPitchShiftFixed.o"

echo "Recompiling PitchShiftFactory..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c $PLUGIN_SRC/PitchShiftFactory.cpp \
    -o "$OBJ_DIR/PitchShiftFactory.o"

echo "Linking test executable..."
# Link only JUCE modules in DEBUG mode (core, dsp, audio_basics)
# Avoid RELEASE mode modules (audio_formats, gui_*, etc)
clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_detunedoubler_thd.o" \
    "$OBJ_DIR/juce_stubs.o" \
    "$OBJ_DIR/DetuneDoubler.o" \
    "$OBJ_DIR/PitchShifter.o" \
    "$OBJ_DIR/SMBPitchShiftFixed.o" \
    "$OBJ_DIR/PitchShiftFactory.o" \
    "$OBJ_DIR/juce_core.o" \
    "$OBJ_DIR/juce_audio_basics.o" \
    "$OBJ_DIR/juce_dsp.o" \
    $FRAMEWORKS \
    -o "$BUILD_DIR/test_detunedoubler_thd"

if [ $? -eq 0 ]; then
    echo ""
    echo "═══════════════════════════════════════════"
    echo " BUILD SUCCESSFUL!"
    echo "═══════════════════════════════════════════"
    echo ""
    echo "Run with: $BUILD_DIR/test_detunedoubler_thd"
    echo ""
else
    echo "Build failed"
    exit 1
fi

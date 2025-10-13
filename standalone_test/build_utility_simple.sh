#!/bin/bash
# Build utility test - simple version (no EngineFactory)

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Building Utility Engines Test (Simple)                   ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

mkdir -p "$OBJ_DIR"

# Compiler flags
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# Step 1: Compile utility engines
echo "[1/3] Compiling utility engines..."

if [ ! -f "$OBJ_DIR/GainUtility_Platinum.o" ]; then
    echo "  Compiling GainUtility_Platinum..."
    clang++ $CPP_FLAGS $INCLUDES $DEFINES \
        -c "$PLUGIN_SRC/GainUtility_Platinum.cpp" \
        -o "$OBJ_DIR/GainUtility_Platinum.o"
else
    echo "  GainUtility_Platinum already compiled"
fi

if [ ! -f "$OBJ_DIR/MonoMaker_Platinum.o" ]; then
    echo "  Compiling MonoMaker_Platinum..."
    clang++ $CPP_FLAGS $INCLUDES $DEFINES \
        -c "$PLUGIN_SRC/MonoMaker_Platinum.cpp" \
        -o "$OBJ_DIR/MonoMaker_Platinum.o"
else
    echo "  MonoMaker_Platinum already compiled"
fi

# Step 2: Compile test
echo ""
echo "[2/3] Compiling utility_test_simple.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c utility_test_simple.cpp \
    -o "$OBJ_DIR/utility_test_simple.o"

# Step 3: Link (only JUCE modules + utility engines)
echo ""
echo "[3/3] Linking..."

# Use all JUCE modules
JUCE_OBJS="$OBJ_DIR/juce_core.o $OBJ_DIR/juce_audio_basics.o $OBJ_DIR/juce_audio_formats.o $OBJ_DIR/juce_dsp.o $OBJ_DIR/juce_build_info.o $OBJ_DIR/juce_events.o $OBJ_DIR/juce_graphics.o $OBJ_DIR/juce_gui_basics.o $OBJ_DIR/juce_data_structures.o"

clang++ $CPP_FLAGS \
    "$OBJ_DIR/utility_test_simple.o" \
    "$OBJ_DIR/GainUtility_Platinum.o" \
    "$OBJ_DIR/MonoMaker_Platinum.o" \
    $JUCE_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/utility_test"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo "  Executable: $BUILD_DIR/utility_test"
    echo ""
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

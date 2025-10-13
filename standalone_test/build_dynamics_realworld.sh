#!/bin/bash

set -e  # Exit on error

echo "========================================================================"
echo "    Building Real-World Dynamics Engine Test"
echo "========================================================================"
echo ""

# Configuration
PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
JUCE_PATH="$PROJECT_ROOT/JUCE"
SOURCE_PATH="$PROJECT_ROOT/JUCE_Plugin/Source"
STANDALONE_PATH="$PROJECT_ROOT/standalone_test"
BUILD_DIR="$STANDALONE_PATH/build"

# Check if build directory exists with JUCE objects
if [ ! -f "$BUILD_DIR/juce_core.o" ]; then
    echo "ERROR: JUCE objects not found. Please run main build first."
    echo "Run: cd $STANDALONE_PATH && ./build_v2.sh"
    exit 1
fi

# Output
OUTPUT="$BUILD_DIR/test_dynamics_realworld"

# Compiler flags
INCLUDES="-I$JUCE_PATH/modules -I$SOURCE_PATH -I$SOURCE_PATH/.."
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"
DEFINES="$DEFINES -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0"
DEFINES="$DEFINES -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter"

# Frameworks (minimal for audio)
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation"
FRAMEWORKS="$FRAMEWORKS -framework AudioToolbox -framework Cocoa -framework IOKit"
FRAMEWORKS="$FRAMEWORKS -framework Security -framework QuartzCore -framework CoreImage"
FRAMEWORKS="$FRAMEWORKS -framework CoreGraphics -framework CoreText -framework WebKit"
FRAMEWORKS="$FRAMEWORKS -framework DiscRecording"

# Libraries
LIBS="-L/opt/homebrew/lib -lharfbuzz"

echo "Step 1: Compiling test main..."
echo ""

clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$STANDALONE_PATH/test_dynamics_realworld.cpp" \
    -o "$BUILD_DIR/test_dynamics_realworld.o"

echo "Step 2: Compiling dynamics engines..."
echo ""

# Compile each engine
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$SOURCE_PATH/ClassicCompressor.cpp" \
    -o "$BUILD_DIR/ClassicCompressor_dynamics.o"

clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$SOURCE_PATH/VintageOptoCompressor.cpp" \
    -o "$BUILD_DIR/VintageOptoCompressor_dynamics.o"

clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$SOURCE_PATH/NoiseGate.cpp" \
    -o "$BUILD_DIR/NoiseGate_dynamics.o"

clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$SOURCE_PATH/MasteringLimiter_Platinum.cpp" \
    -o "$BUILD_DIR/MasteringLimiter_dynamics.o"

clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$SOURCE_PATH/DynamicEQ.cpp" \
    -o "$BUILD_DIR/DynamicEQ_dynamics.o"

echo "Step 3: Linking..."
echo ""

clang++ $CPP_FLAGS \
    "$BUILD_DIR/test_dynamics_realworld.o" \
    "$BUILD_DIR/ClassicCompressor_dynamics.o" \
    "$BUILD_DIR/VintageOptoCompressor_dynamics.o" \
    "$BUILD_DIR/NoiseGate_dynamics.o" \
    "$BUILD_DIR/MasteringLimiter_dynamics.o" \
    "$BUILD_DIR/DynamicEQ_dynamics.o" \
    "$BUILD_DIR/juce_core.o" \
    "$BUILD_DIR/juce_audio_basics.o" \
    "$BUILD_DIR/juce_audio_formats.o" \
    "$BUILD_DIR/juce_audio_processors.o" \
    "$BUILD_DIR/juce_data_structures.o" \
    "$BUILD_DIR/juce_events.o" \
    "$BUILD_DIR/juce_graphics.o" \
    "$BUILD_DIR/juce_gui_basics.o" \
    "$BUILD_DIR/juce_core_CompilationTime.o" \
    $FRAMEWORKS \
    $LIBS \
    -o "$OUTPUT"

if [ $? -eq 0 ]; then
    echo ""
    echo "========================================================================"
    echo "BUILD SUCCESSFUL!"
    echo "========================================================================"
    echo "Executable: $OUTPUT"
    echo ""
    echo "To run: cd $STANDALONE_PATH && ./test_dynamics_realworld"
    echo ""
else
    echo ""
    echo "========================================================================"
    echo "BUILD FAILED!"
    echo "========================================================================"
    exit 1
fi

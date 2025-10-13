#!/bin/bash

echo "========================================"
echo "Building Real-World Audio Testing Suite"
echo "========================================"
echo ""

# Set paths
PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
JUCE_DIR="$PROJECT_ROOT/JUCE"
SOURCE_DIR="$PROJECT_ROOT/JUCE_Plugin/Source"
STANDALONE_DIR="$PROJECT_ROOT/standalone_test"

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O3 -I$PROJECT_ROOT -I$JUCE_DIR/modules -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"

# JUCE frameworks (macOS)
FRAMEWORKS="-framework Accelerate -framework AudioToolbox -framework CoreAudio -framework CoreMIDI -framework IOKit -framework WebKit -framework Cocoa"

echo "Step 1: Compiling test program..."
$CXX $CXXFLAGS \
    -c "$STANDALONE_DIR/test_real_world_audio.cpp" \
    -o "$STANDALONE_DIR/build/test_real_world_audio.o" \
    || { echo "Failed to compile test_real_world_audio.cpp"; exit 1; }

echo "Step 2: Compiling engine factory..."
$CXX $CXXFLAGS \
    -c "$STANDALONE_DIR/ComprehensiveTHDEngineFactory.cpp" \
    -o "$STANDALONE_DIR/build/ComprehensiveTHDEngineFactory.o" \
    || { echo "Failed to compile engine factory"; exit 1; }

echo "Step 3: Compiling JUCE stubs..."
if [ ! -f "$STANDALONE_DIR/build/juce_stubs.o" ]; then
    $CXX $CXXFLAGS \
        -c "$STANDALONE_DIR/juce_stubs.cpp" \
        -o "$STANDALONE_DIR/build/juce_stubs.o" \
        || { echo "Failed to compile JUCE stubs"; exit 1; }
fi

echo "Step 4: Linking executable..."
$CXX \
    "$STANDALONE_DIR/build/test_real_world_audio.o" \
    "$STANDALONE_DIR/build/ComprehensiveTHDEngineFactory.o" \
    "$STANDALONE_DIR/build/juce_stubs.o" \
    $FRAMEWORKS \
    -o "$STANDALONE_DIR/test_real_world_audio" \
    || { echo "Failed to link executable"; exit 1; }

echo ""
echo "========================================"
echo "Build successful!"
echo "========================================"
echo ""
echo "To run the test suite:"
echo "  1. Generate test materials:"
echo "     python3 generate_musical_materials.py"
echo ""
echo "  2. Run the test suite:"
echo "     ./test_real_world_audio"
echo ""
echo "  3. View the report:"
echo "     open REAL_WORLD_AUDIO_TESTING_REPORT.md"
echo ""

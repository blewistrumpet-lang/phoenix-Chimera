#!/bin/bash

echo "Building DynamicEQ Debug Test..."
echo "================================="

# Directories
BUILD_DIR="build"
OBJ_DIR="$BUILD_DIR/obj"
JUCE_PATH="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE"
SOURCE_PATH="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source"

# Create build directories
mkdir -p "$OBJ_DIR"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 -I$JUCE_PATH/modules -I$SOURCE_PATH -I."
LDFLAGS="-framework Accelerate -framework CoreFoundation -framework CoreAudio -framework CoreMIDI -framework AudioToolbox"

# Compile test
echo "Compiling test_dynamiceq_debug.cpp..."
$CXX $CXXFLAGS -c test_dynamiceq_debug.cpp -o $OBJ_DIR/test_dynamiceq_debug.o || exit 1

# Compile DynamicEQ
echo "Compiling DynamicEQ.cpp..."
$CXX $CXXFLAGS -c $SOURCE_PATH/DynamicEQ.cpp -o $OBJ_DIR/DynamicEQ_debug.o || exit 1

# Compile required JUCE modules
echo "Compiling juce_core..."
$CXX $CXXFLAGS -c juce_stubs.cpp -DJUCE_CORE -o $OBJ_DIR/juce_core_debug.o || exit 1

echo "Compiling juce_audio_basics..."
$CXX $CXXFLAGS -c juce_stubs.cpp -DJUCE_AUDIO_BASICS -o $OBJ_DIR/juce_audio_basics_debug.o || exit 1

echo "Compiling juce_dsp..."
$CXX $CXXFLAGS -c juce_stubs.cpp -DJUCE_DSP -o $OBJ_DIR/juce_dsp_debug.o || exit 1

# Link
echo "Linking..."
$CXX $LDFLAGS \
    $OBJ_DIR/test_dynamiceq_debug.o \
    $OBJ_DIR/DynamicEQ_debug.o \
    $OBJ_DIR/juce_core_debug.o \
    $OBJ_DIR/juce_audio_basics_debug.o \
    $OBJ_DIR/juce_dsp_debug.o \
    -o $BUILD_DIR/test_dynamiceq_debug || exit 1

echo ""
echo "✓ Build successful!"
echo ""
echo "Run test with:"
echo "  ./build/test_dynamiceq_debug"

#!/bin/bash

# Build script for pitch engines 34-38 test

set -e  # Exit on error

echo "Building Pitch Engines 34-38 Test..."

# Set up paths
PROJECT_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
JUCE_DIR="$PROJECT_DIR/JUCE"
PLUGIN_DIR="$PROJECT_DIR/JUCE_Plugin/Source"
BUILD_DIR="build"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"
INCLUDES="-I$JUCE_DIR/modules -I$PLUGIN_DIR -I."

# JUCE framework linking
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework Carbon -framework Cocoa -framework IOKit -framework QuartzCore"

# Create build directory if it doesn't exist
mkdir -p $BUILD_DIR

echo "Compiling test_pitch_engines_34_38.cpp..."
$CXX $CXXFLAGS $INCLUDES \
    test_pitch_engines_34_38.cpp \
    $BUILD_DIR/EngineFactory.o \
    $BUILD_DIR/juce_core.o \
    $BUILD_DIR/juce_audio_basics.o \
    $BUILD_DIR/juce_dsp.o \
    $FRAMEWORKS \
    -o $BUILD_DIR/test_pitch_34_38

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Running test..."
    echo "═══════════════════════════════════════════════════════════"
    $BUILD_DIR/test_pitch_34_38
else
    echo "✗ Build failed!"
    exit 1
fi

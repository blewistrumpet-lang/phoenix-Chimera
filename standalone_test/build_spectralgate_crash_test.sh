#!/bin/bash

set -e

echo "Building SpectralGate Crash Test (using existing JUCE libs)..."

JUCE_DIR="../JUCE"
SOURCE_DIR="../JUCE_Plugin/Source"
BUILD_DIR="build"

# Compiler flags
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -I$JUCE_DIR/modules -I$SOURCE_DIR -I../JUCE_Plugin/Source -I../JUCE_Plugin/JuceLibraryCode"
CXXFLAGS="$CXXFLAGS -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
CXXFLAGS="$CXXFLAGS -Wno-deprecated-declarations -ffast-math -funroll-loops"

# JUCE framework flags
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework CoreFoundation -framework Foundation"

echo "Step 1: Recompiling SpectralGate_Platinum with fixes..."
$CXX $CXXFLAGS -c "$SOURCE_DIR/SpectralGate_Platinum.cpp" -o "$BUILD_DIR/obj/SpectralGate_Platinum_new.o"

echo "Step 2: Compiling test..."
$CXX $CXXFLAGS -c test_spectralgate_crash.cpp -o "$BUILD_DIR/obj/test_spectralgate_crash.o"

echo "Step 3: Linking with existing JUCE libraries..."
$CXX -o test_spectralgate_crash \
    "$BUILD_DIR/obj/test_spectralgate_crash.o" \
    "$BUILD_DIR/obj/SpectralGate_Platinum_new.o" \
    "$BUILD_DIR/juce_core.o" \
    "$BUILD_DIR/juce_audio_basics.o" \
    "$BUILD_DIR/juce_dsp.o" \
    $FRAMEWORKS

echo ""
echo "Build successful! Running tests..."
echo "=========================================="
./test_spectralgate_crash

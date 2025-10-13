#!/bin/bash

set -e

echo "Building Simplified Pitch Engine Test..."

# Directories
BUILD_DIR="build"
OBJ_DIR="$BUILD_DIR/obj"

# Create directories
mkdir -p "$OBJ_DIR"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -I. -I../JUCE_Plugin/Source"
LDFLAGS="-framework Accelerate"

echo "Compiling sources..."

# Compile SMBPitchShiftFixed
$CXX $CXXFLAGS -c SMBPitchShiftFixed_standalone.cpp -o "$OBJ_DIR/SMBPitchShiftFixed_standalone.o"

# Compile IntelligentHarmonizer
$CXX $CXXFLAGS -c IntelligentHarmonizer_standalone.cpp -o "$OBJ_DIR/IntelligentHarmonizer_standalone.o"

# Compile test
$CXX $CXXFLAGS -c test_pitch_simple.cpp -o "$OBJ_DIR/test_pitch_simple.o"

echo "Linking..."
$CXX $LDFLAGS \
    "$OBJ_DIR/test_pitch_simple.o" \
    "$OBJ_DIR/IntelligentHarmonizer_standalone.o" \
    "$OBJ_DIR/SMBPitchShiftFixed_standalone.o" \
    -o test_pitch_simple

echo "✓ Build complete: ./test_pitch_simple"
echo ""
echo "Running test..."
./test_pitch_simple

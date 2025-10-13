#!/bin/bash

set -e

echo "Building IntelligentHarmonizer Debug Test..."

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

# Compile SMBPitchShiftFixed (needs signalsmith-stretch)
$CXX $CXXFLAGS -c SMBPitchShiftFixed_standalone.cpp -o "$OBJ_DIR/SMBPitchShiftFixed_standalone.o"

# Compile IntelligentHarmonizer
$CXX $CXXFLAGS -c IntelligentHarmonizer_standalone.cpp -o "$OBJ_DIR/IntelligentHarmonizer_standalone.o"

# Compile test
$CXX $CXXFLAGS -c test_harmonizer_debug.cpp -o "$OBJ_DIR/test_harmonizer_debug.o"

echo "Linking..."
$CXX $LDFLAGS \
    "$OBJ_DIR/test_harmonizer_debug.o" \
    "$OBJ_DIR/IntelligentHarmonizer_standalone.o" \
    "$OBJ_DIR/SMBPitchShiftFixed_standalone.o" \
    -o test_harmonizer_debug

echo "Build complete: ./test_harmonizer_debug"
echo ""
echo "Running test..."
./test_harmonizer_debug

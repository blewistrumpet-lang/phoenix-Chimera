#!/bin/bash

# Build script for PhasedVocoder verification test
# Tests warmup fix, latency, pitch shift, and time stretch

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
JUCE_PATH="/Users/Branden/JUCE"
BUILD_DIR="$SCRIPT_DIR/build"
OBJ_DIR="$BUILD_DIR/obj"

# Create build directories
mkdir -p "$OBJ_DIR"

echo "Building PhasedVocoder Verification Test..."

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
INCLUDES="-I$PROJECT_ROOT/JUCE_Plugin/Source -I$JUCE_PATH/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework Foundation"

# Source files
SOURCE_DIR="$PROJECT_ROOT/JUCE_Plugin/Source"

# Compile dependencies
echo "Compiling dependencies..."

# PhasedVocoder
$CXX $CXXFLAGS $INCLUDES -c "$SOURCE_DIR/PhasedVocoder.cpp" -o "$OBJ_DIR/PhasedVocoder.o"

# EngineFactory
$CXX $CXXFLAGS $INCLUDES -c "$SOURCE_DIR/EngineFactory.cpp" -o "$OBJ_DIR/EngineFactory.o"

# Compile test
echo "Compiling test..."
$CXX $CXXFLAGS $INCLUDES -c "$SCRIPT_DIR/test_phasedvocoder_verify.cpp" -o "$OBJ_DIR/test_phasedvocoder_verify.o"

# Link
echo "Linking..."
$CXX $CXXFLAGS \
    "$OBJ_DIR/test_phasedvocoder_verify.o" \
    "$OBJ_DIR/PhasedVocoder.o" \
    "$OBJ_DIR/EngineFactory.o" \
    $FRAMEWORKS \
    -o "$BUILD_DIR/test_phasedvocoder_verify"

echo "Build complete: $BUILD_DIR/test_phasedvocoder_verify"
echo ""
echo "Running test..."
echo "========================================"
"$BUILD_DIR/test_phasedvocoder_verify"

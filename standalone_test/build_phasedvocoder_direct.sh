#!/bin/bash

# Direct build for PhasedVocoder verification test (no EngineFactory)

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
JUCE_PATH="/Users/Branden/JUCE"
BUILD_DIR="$SCRIPT_DIR/build"
OBJ_DIR="$BUILD_DIR/obj"

# Create build directories
mkdir -p "$OBJ_DIR"

echo "Building PhasedVocoder Direct Verification Test..."

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"
INCLUDES="-I$PROJECT_ROOT/JUCE_Plugin/Source -I$PROJECT_ROOT/JUCE_Plugin/JuceLibraryCode -I$JUCE_PATH/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework Foundation"

# Source files
SOURCE_DIR="$PROJECT_ROOT/JUCE_Plugin/Source"

# Compile PhasedVocoder
echo "Compiling PhasedVocoder..."
$CXX $CXXFLAGS $INCLUDES -c "$SOURCE_DIR/PhasedVocoder.cpp" -o "$OBJ_DIR/PhasedVocoder.o"

# Compile test
echo "Compiling test..."
$CXX $CXXFLAGS $INCLUDES -c "$SCRIPT_DIR/test_phasedvocoder_direct.cpp" -o "$OBJ_DIR/test_phasedvocoder_direct.o"

# Link
echo "Linking..."
$CXX $CXXFLAGS \
    "$OBJ_DIR/test_phasedvocoder_direct.o" \
    "$OBJ_DIR/PhasedVocoder.o" \
    $FRAMEWORKS \
    -o "$BUILD_DIR/test_phasedvocoder_direct"

echo "Build complete: $BUILD_DIR/test_phasedvocoder_direct"
echo ""
echo "Running test..."
echo "========================================"
"$BUILD_DIR/test_phasedvocoder_direct"

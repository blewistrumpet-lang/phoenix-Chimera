#!/bin/bash

# Build script for Modulation Engines 28-31 Test Suite

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "Building Modulation Engines 28-31 Test"
echo "========================================"

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O2 -I. -I../JUCE_Plugin/Source"
JUCE_DIR="../JUCE/modules"

# JUCE module includes
JUCE_INCLUDES="-I${JUCE_DIR} -I${JUCE_DIR}/juce_audio_basics -I${JUCE_DIR}/juce_audio_formats -I${JUCE_DIR}/juce_core -I${JUCE_DIR}/juce_dsp -I${JUCE_DIR}/juce_events"

# JUCE preprocessor flags
JUCE_FLAGS="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"

# macOS frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework Foundation"

OUTPUT_DIR="build"
mkdir -p "$OUTPUT_DIR"

echo ""
echo "Step 1: Compiling JUCE modules..."

# Compile JUCE modules
$CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_audio_basics/juce_audio_basics.mm -o ${OUTPUT_DIR}/juce_audio_basics.o
$CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_core/juce_core.mm -o ${OUTPUT_DIR}/juce_core.o
$CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_dsp/juce_dsp.mm -o ${OUTPUT_DIR}/juce_dsp.o

echo ""
echo "Step 2: Compiling engine implementations..."

# Get list of required engines
ENGINES=(
    "HarmonicTremolo"
    "ClassicTremolo"
    "RotarySpeaker_Platinum"
    "PitchShifter"
)

# Compile each engine
for engine in "${ENGINES[@]}"; do
    ENGINE_FILE="../JUCE_Plugin/Source/${engine}.cpp"
    if [ -f "$ENGINE_FILE" ]; then
        echo "  Compiling ${engine}..."
        $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c "$ENGINE_FILE" -o "${OUTPUT_DIR}/${engine}.o"
    else
        echo "  Warning: ${ENGINE_FILE} not found"
    fi
done

# Compile EngineFactory (EngineBase is header-only)
echo "  Compiling EngineFactory..."
$CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ../JUCE_Plugin/Source/EngineFactory.cpp -o ${OUTPUT_DIR}/EngineFactory.o

echo ""
echo "Step 3: Compiling test file..."
$CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c test_modulation_28_31.cpp -o ${OUTPUT_DIR}/test_modulation_28_31.o

echo ""
echo "Step 4: Linking..."

# Collect all object files
OBJECT_FILES="${OUTPUT_DIR}/test_modulation_28_31.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/EngineFactory.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_audio_basics.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_core.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_dsp.o"

# Add engine object files
for engine in "${ENGINES[@]}"; do
    if [ -f "${OUTPUT_DIR}/${engine}.o" ]; then
        OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/${engine}.o"
    fi
done

# Link
$CXX $OBJECT_FILES $FRAMEWORKS -o test_modulation_28_31

echo ""
echo "========================================"
echo "Build complete!"
echo "========================================"
echo ""
echo "Run with: ./test_modulation_28_31"
echo ""

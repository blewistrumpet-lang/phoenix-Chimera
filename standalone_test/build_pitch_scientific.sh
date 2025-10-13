#!/bin/bash

# Build script for Scientific Pitch Accuracy Analysis Suite
# Tests all pitch-related engines with 6 detection algorithms

set -e

echo "╔═══════════════════════════════════════════════════════════════════════════╗"
echo "║      Building Scientific Pitch Accuracy Analysis Suite                   ║"
echo "╚═══════════════════════════════════════════════════════════════════════════╝"
echo ""

# Set paths
JUCE_PATH="/Users/Branden/JUCE"
SOURCE_PATH="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source"
BUILD_DIR="build"
OUTPUT_BIN="$BUILD_DIR/test_pitch_accuracy_scientific"

# Create build directory
mkdir -p "$BUILD_DIR"

echo "Configuration:"
echo "  JUCE Path: $JUCE_PATH"
echo "  Source Path: $SOURCE_PATH"
echo "  Output: $OUTPUT_BIN"
echo ""

# Compiler flags
CXXFLAGS="-std=c++17 -O3 -DJUCE_STANDALONE_APPLICATION=1"
CXXFLAGS="$CXXFLAGS -I$JUCE_PATH/modules"
CXXFLAGS="$CXXFLAGS -I$SOURCE_PATH"
CXXFLAGS="$CXXFLAGS -I."

# Platform-specific flags
CXXFLAGS="$CXXFLAGS -framework Accelerate -framework CoreAudio -framework CoreMIDI"
CXXFLAGS="$CXXFLAGS -framework IOKit -framework Carbon -framework Cocoa"
CXXFLAGS="$CXXFLAGS -framework QuartzCore -framework AudioToolbox"

# Warnings
CXXFLAGS="$CXXFLAGS -Wall -Wno-deprecated-declarations"

# JUCE module sources
JUCE_SOURCES=(
    "juce_audio_basics"
    "juce_audio_devices"
    "juce_audio_processors"
    "juce_core"
    "juce_data_structures"
    "juce_events"
    "juce_graphics"
    "juce_gui_basics"
)

echo "═══════════════════════════════════════════════════════════════════════════"
echo "Step 1: Compiling JUCE modules..."
echo "═══════════════════════════════════════════════════════════════════════════"
echo ""

JUCE_OBJECTS=()

for module in "${JUCE_SOURCES[@]}"; do
    OBJ_FILE="${module}.o"

    if [ -f "$OBJ_FILE" ]; then
        echo "  ✓ Using cached: $OBJ_FILE"
        JUCE_OBJECTS+=("$OBJ_FILE")
        continue
    fi

    echo "  Compiling: $module..."

    cat > "${module}_compile.cpp" << EOF
#define JUCE_STANDALONE_APPLICATION 1
#include <JuceHeader.h>

#if __has_include("${JUCE_PATH}/modules/${module}/${module}.mm")
  #include "${JUCE_PATH}/modules/${module}/${module}.mm"
#elif __has_include("${JUCE_PATH}/modules/${module}/${module}.cpp")
  #include "${JUCE_PATH}/modules/${module}/${module}.cpp"
#endif
EOF

    if clang++ $CXXFLAGS -c "${module}_compile.cpp" -o "$OBJ_FILE" 2>&1 | grep -v "warning:"; then
        echo "    → Success"
        JUCE_OBJECTS+=("$OBJ_FILE")
        rm -f "${module}_compile.cpp"
    else
        echo "    → ERROR compiling $module"
        exit 1
    fi
done

echo ""
echo "═══════════════════════════════════════════════════════════════════════════"
echo "Step 2: Compiling Engine Sources..."
echo "═══════════════════════════════════════════════════════════════════════════"
echo ""

# Engine sources (pitch-related engines only)
ENGINE_SOURCES=(
    "EngineBase"
    "EngineFactory"
    "PitchShifter"
    "DetuneDoubler"
    "IntelligentHarmonizer"
    "ShimmerReverb"
    "PhasedVocoder"
    "GranularCloud"
)

ENGINE_OBJECTS=()

for engine in "${ENGINE_SOURCES[@]}"; do
    SRC_FILE="$SOURCE_PATH/${engine}.cpp"
    OBJ_FILE="$BUILD_DIR/${engine}.o"

    if [ ! -f "$SRC_FILE" ]; then
        echo "  ⚠ Skipping: $engine (source not found)"
        continue
    fi

    echo "  Compiling: $engine..."

    if clang++ $CXXFLAGS -c "$SRC_FILE" -o "$OBJ_FILE" 2>&1 | grep -v "warning:"; then
        echo "    → Success"
        ENGINE_OBJECTS+=("$OBJ_FILE")
    else
        echo "    → ERROR compiling $engine"
        exit 1
    fi
done

echo ""
echo "═══════════════════════════════════════════════════════════════════════════"
echo "Step 3: Compiling Test Suite..."
echo "═══════════════════════════════════════════════════════════════════════════"
echo ""

TEST_OBJ="$BUILD_DIR/test_pitch_accuracy_scientific.o"

echo "  Compiling: test_pitch_accuracy_scientific.cpp..."

if clang++ $CXXFLAGS -c test_pitch_accuracy_scientific.cpp -o "$TEST_OBJ" 2>&1 | grep -v "warning:"; then
    echo "    → Success"
else
    echo "    → ERROR compiling test suite"
    exit 1
fi

echo ""
echo "═══════════════════════════════════════════════════════════════════════════"
echo "Step 4: Linking..."
echo "═══════════════════════════════════════════════════════════════════════════"
echo ""

ALL_OBJECTS=("${JUCE_OBJECTS[@]}" "${ENGINE_OBJECTS[@]}" "$TEST_OBJ")

echo "  Linking: $OUTPUT_BIN"
echo "  Objects: ${#ALL_OBJECTS[@]}"

if clang++ $CXXFLAGS "${ALL_OBJECTS[@]}" -o "$OUTPUT_BIN"; then
    echo "    → Success"
else
    echo "    → ERROR linking"
    exit 1
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════════════════════╗"
echo "║                          BUILD SUCCESSFUL                                 ║"
echo "╚═══════════════════════════════════════════════════════════════════════════╝"
echo ""
echo "Executable: $OUTPUT_BIN"
echo ""
echo "Run with:"
echo "  $OUTPUT_BIN"
echo ""
echo "This will test:"
echo "  • Engine 31: Pitch Shifter"
echo "  • Engine 32: Detune Doubler"
echo "  • Engine 33: Intelligent Harmonizer"
echo "  • Engine 42: Shimmer Reverb"
echo "  • Engine 49: Phased Vocoder"
echo "  • Engine 50: Granular Cloud"
echo ""
echo "Using 6 detection algorithms:"
echo "  1. YIN Autocorrelation"
echo "  2. Cepstrum Analysis"
echo "  3. FFT Peak Detection"
echo "  4. Zero-Crossing Rate"
echo "  5. Harmonic Product Spectrum"
echo "  6. AMDF"
echo ""
echo "Output reports:"
echo "  • PITCH_ACCURACY_SCIENTIFIC_ANALYSIS.md"
echo "  • build/pitch_scientific_results.csv"
echo ""

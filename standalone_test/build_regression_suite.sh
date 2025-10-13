#!/bin/bash

# Build script for Comprehensive Regression Test Suite
# Project Chimera Phoenix v3.0

set -e

echo "========================================="
echo " Building Regression Test Suite"
echo "========================================="

# Configuration
JUCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE"
SOURCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source"
TEST_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test"
BUILD_DIR="$TEST_DIR/build"
OBJ_DIR="$BUILD_DIR/obj"

# Create build directories
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Compiler flags
CXXFLAGS="-std=c++17 -O2 -I$JUCE_DIR/modules -I$SOURCE_DIR -I$TEST_DIR"
CXXFLAGS="$CXXFLAGS -DJUCE_STANDALONE_APPLICATION=1"
CXXFLAGS="$CXXFLAGS -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
CXXFLAGS="$CXXFLAGS -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0"
CXXFLAGS="$CXXFLAGS -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreMIDI"
CXXFLAGS="$CXXFLAGS -framework AudioToolbox -framework Accelerate -framework QuartzCore"

echo ""
echo "Step 1: Compiling JUCE stubs..."
if [ ! -f "$OBJ_DIR/juce_stubs.o" ]; then
    clang++ $CXXFLAGS -c $TEST_DIR/juce_stubs.cpp -o $OBJ_DIR/juce_stubs.o
    echo "  ✓ JUCE stubs compiled"
else
    echo "  ✓ JUCE stubs already compiled (skipping)"
fi

echo ""
echo "Step 2: Compiling DSP Engines..."

# List of engines needed for regression testing
ENGINES=(
    "StereoChorus"           # Engine 23 - LFO fix
    "ResonantChorus"         # Engine 24 - LFO fix
    "FrequencyShifter"       # Engine 27 - LFO fix
    "HarmonicTremolo"        # Engine 28 - LFO fix
    "PlateReverb"            # Engine 39 - Memory leak fix
    "ShimmerReverb"          # Engine 40 - Memory leak fix
    "ConvolutionReverb"      # Engine 41 - Memory leak fix
    "SpringReverb"           # Engine 42 - Memory leak fix
    "GatedReverb"            # Engine 43 - Memory leak fix
    "PhasedVocoder"          # Engine 49 - Critical fix
)

for engine in "${ENGINES[@]}"; do
    if [ -f "$SOURCE_DIR/${engine}.cpp" ]; then
        if [ ! -f "$OBJ_DIR/${engine}.o" ]; then
            echo "  Compiling ${engine}..."
            clang++ $CXXFLAGS -c "$SOURCE_DIR/${engine}.cpp" -o "$OBJ_DIR/${engine}.o"
        else
            echo "  ✓ ${engine} already compiled"
        fi
    else
        echo "  ⚠ Warning: ${engine}.cpp not found, skipping"
    fi
done

echo ""
echo "Step 3: Compiling supporting DSP components..."

# Common DSP components
COMPONENTS=(
    "Freeverb"
    "AdvancedSpringDispersion"
    "DelayLine"
)

for component in "${COMPONENTS[@]}"; do
    if [ -f "$SOURCE_DIR/${component}.cpp" ]; then
        if [ ! -f "$OBJ_DIR/${component}.o" ]; then
            echo "  Compiling ${component}..."
            clang++ $CXXFLAGS -c "$SOURCE_DIR/${component}.cpp" -o "$OBJ_DIR/${component}.o"
        else
            echo "  ✓ ${component} already compiled"
        fi
    fi
done

echo ""
echo "Step 4: Creating minimal EngineFactory for regression tests..."

cat > $TEST_DIR/EngineFactory_regression.cpp << 'EOF'
#include <memory>
#include <JuceHeader.h>
#include "EngineBase.h"

// Engine includes
#include "StereoChorus.h"
#include "ResonantChorus.h"
#include "FrequencyShifter.h"
#include "HarmonicTremolo.h"
#include "PlateReverb.h"
#include "ShimmerReverb.h"
#include "ConvolutionReverb.h"
#include "SpringReverb.h"
#include "GatedReverb.h"
#include "PhasedVocoder.h"

std::unique_ptr<EngineBase> createEngine(int engineID, int sampleRate) {
    switch (engineID) {
        case 23: return std::make_unique<StereoChorus>();
        case 24: return std::make_unique<ResonantChorus>();
        case 27: return std::make_unique<FrequencyShifter>();
        case 28: return std::make_unique<HarmonicTremolo>();
        case 39: return std::make_unique<PlateReverb>();
        case 40: return std::make_unique<ShimmerReverb>();
        case 41: return std::make_unique<ConvolutionReverb>();
        case 42: return std::make_unique<SpringReverb>();
        case 43: return std::make_unique<GatedReverb>();
        case 49: return std::make_unique<PhasedVocoder>();
        default: return nullptr;
    }
}
EOF

echo "  Compiling EngineFactory..."
clang++ $CXXFLAGS -c $TEST_DIR/EngineFactory_regression.cpp -o $OBJ_DIR/EngineFactory_regression.o

echo ""
echo "Step 5: Compiling regression test suite..."
clang++ $CXXFLAGS -c $TEST_DIR/test_regression_suite.cpp -o $OBJ_DIR/test_regression_suite.o

echo ""
echo "Step 6: Linking final executable..."

# Collect all object files
OBJECTS=""
for obj in $OBJ_DIR/*.o; do
    if [[ "$obj" != *"standalone_test.o" ]]; then
        OBJECTS="$OBJECTS $obj"
    fi
done

clang++ $CXXFLAGS $OBJECTS -o $BUILD_DIR/test_regression_suite

echo ""
echo "========================================="
echo " Build Complete!"
echo "========================================="
echo ""
echo "Executable: $BUILD_DIR/test_regression_suite"
echo ""
echo "Usage:"
echo "  ./build/test_regression_suite --mode full      # Run all regression tests"
echo "  ./build/test_regression_suite --mode verify    # Verify against baseline"
echo "  ./build/test_regression_suite --mode baseline  # Capture baseline"
echo ""

# Make executable runnable
chmod +x $BUILD_DIR/test_regression_suite

echo "Ready to run regression tests!"
echo ""

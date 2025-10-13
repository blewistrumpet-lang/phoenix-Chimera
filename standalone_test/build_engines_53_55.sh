#!/bin/bash
# Build script for Engines 53-55 test

set -e

echo "Building Engines 53-55 Test..."

JUCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE"
SOURCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source"
BUILD_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/build/obj"
OUTPUT_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Compiler settings
CXX="g++"
CXXFLAGS="-std=c++17 -O2"
INCLUDES="-I${JUCE_DIR}/modules -I${SOURCE_DIR} -I${OUTPUT_DIR}"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework Foundation"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DDEBUG=1"

echo "Step 1: Compiling JUCE modules..."
if [ ! -f "$BUILD_DIR/juce_core.o" ]; then
    echo "  Compiling juce_core..."
    $CXX $CXXFLAGS $INCLUDES $DEFINES -c "${JUCE_DIR}/modules/juce_core/juce_core.cpp" -o "$BUILD_DIR/juce_core.o"
fi

if [ ! -f "$BUILD_DIR/juce_audio_basics.o" ]; then
    echo "  Compiling juce_audio_basics..."
    $CXX $CXXFLAGS $INCLUDES $DEFINES -c "${JUCE_DIR}/modules/juce_audio_basics/juce_audio_basics.cpp" -o "$BUILD_DIR/juce_audio_basics.o"
fi

if [ ! -f "$BUILD_DIR/juce_dsp.o" ]; then
    echo "  Compiling juce_dsp..."
    $CXX $CXXFLAGS $INCLUDES $DEFINES -c "${JUCE_DIR}/modules/juce_dsp/juce_dsp.cpp" -o "$BUILD_DIR/juce_dsp.o"
fi

echo "Step 2: Compiling Engine sources..."

# Engine 53: MidSideProcessor_Platinum
echo "  Compiling MidSideProcessor_Platinum..."
$CXX $CXXFLAGS $INCLUDES $DEFINES -c "${SOURCE_DIR}/MidSideProcessor_Platinum.cpp" -o "$BUILD_DIR/MidSideProcessor_Platinum.o"

# Engine 54: GainUtility_Platinum
echo "  Compiling GainUtility_Platinum..."
$CXX $CXXFLAGS $INCLUDES $DEFINES -c "${SOURCE_DIR}/GainUtility_Platinum.cpp" -o "$BUILD_DIR/GainUtility_Platinum.o"

# Engine 55: MonoMaker_Platinum
echo "  Compiling MonoMaker_Platinum..."
$CXX $CXXFLAGS $INCLUDES $DEFINES -c "${SOURCE_DIR}/MonoMaker_Platinum.cpp" -o "$BUILD_DIR/MonoMaker_Platinum.o"

echo "Step 3: Compiling EngineFactory..."
$CXX $CXXFLAGS $INCLUDES $DEFINES -c "${SOURCE_DIR}/EngineFactory.cpp" -o "$BUILD_DIR/EngineFactory.o"

echo "Step 4: Compiling test program..."
$CXX $CXXFLAGS $INCLUDES $DEFINES -c "test_engines_53_55.cpp" -o "$BUILD_DIR/test_engines_53_55.o"

echo "Step 5: Linking..."
$CXX $CXXFLAGS \
    "$BUILD_DIR/test_engines_53_55.o" \
    "$BUILD_DIR/EngineFactory.o" \
    "$BUILD_DIR/MidSideProcessor_Platinum.o" \
    "$BUILD_DIR/GainUtility_Platinum.o" \
    "$BUILD_DIR/MonoMaker_Platinum.o" \
    "$BUILD_DIR/juce_core.o" \
    "$BUILD_DIR/juce_audio_basics.o" \
    "$BUILD_DIR/juce_dsp.o" \
    $FRAMEWORKS \
    -o "${OUTPUT_DIR}/test_engines_53_55"

echo "Build complete! Executable: ${OUTPUT_DIR}/test_engines_53_55"

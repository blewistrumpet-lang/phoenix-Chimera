#!/bin/bash
# Build script for Utility Engines Real-World Test (50, 51, 54, 55)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║     Building Utility Engines Real-World Test            ║"
echo "║     Engines 50, 51, 54, 55                              ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O2 -DDEBUG=1 -I. -I../JUCE_Plugin/Source"
JUCE_DIR="../JUCE/modules"

# JUCE module includes
JUCE_INCLUDES="-I${JUCE_DIR} -I${JUCE_DIR}/juce_audio_basics -I${JUCE_DIR}/juce_audio_formats -I${JUCE_DIR}/juce_core -I${JUCE_DIR}/juce_dsp -I${JUCE_DIR}/juce_events"

# JUCE preprocessor flags
JUCE_FLAGS="-DDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"

# macOS frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework Foundation -framework Carbon -framework Cocoa -framework IOKit -framework QuartzCore -framework Security"

OUTPUT_DIR="build"
mkdir -p "$OUTPUT_DIR"

echo "Step 1: Checking for JUCE modules..."
# Check if JUCE modules are already compiled
NEED_COMPILE=0
for module in juce_audio_basics juce_audio_formats juce_core juce_dsp; do
    if [ ! -f "${OUTPUT_DIR}/${module}.o" ]; then
        NEED_COMPILE=1
        break
    fi
done

if [ $NEED_COMPILE -eq 1 ]; then
    echo "  Compiling JUCE modules..."
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_audio_basics/juce_audio_basics.mm -o ${OUTPUT_DIR}/juce_audio_basics.o 2>&1 | head -20
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_audio_formats/juce_audio_formats.mm -o ${OUTPUT_DIR}/juce_audio_formats.o 2>&1 | head -20
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_core/juce_core.mm -o ${OUTPUT_DIR}/juce_core.o 2>&1 | head -20
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_dsp/juce_dsp.mm -o ${OUTPUT_DIR}/juce_dsp.o 2>&1 | head -20
else
    echo "  JUCE modules already compiled"
fi

echo ""
echo "Step 2: Compiling engine implementations..."

# Engine 50: GranularCloud
if [ ! -f "${OUTPUT_DIR}/GranularCloud.o" ]; then
    echo "  Compiling GranularCloud..."
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ../JUCE_Plugin/Source/GranularCloud.cpp -o ${OUTPUT_DIR}/GranularCloud.o
fi

# Engine 51: ChaosGenerator
if [ ! -f "${OUTPUT_DIR}/ChaosGenerator.o" ]; then
    echo "  Compiling ChaosGenerator..."
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ../JUCE_Plugin/Source/ChaosGenerator.cpp -o ${OUTPUT_DIR}/ChaosGenerator.o
fi

# ChaosGenerator Platinum
if [ ! -f "${OUTPUT_DIR}/ChaosGenerator_Platinum.o" ]; then
    echo "  Compiling ChaosGenerator_Platinum..."
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ../JUCE_Plugin/Source/ChaosGenerator_Platinum.cpp -o ${OUTPUT_DIR}/ChaosGenerator_Platinum.o
fi

# Engine 54: GainUtility_Platinum
if [ ! -f "${OUTPUT_DIR}/GainUtility_Platinum.o" ]; then
    echo "  Compiling GainUtility_Platinum..."
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ../JUCE_Plugin/Source/GainUtility_Platinum.cpp -o ${OUTPUT_DIR}/GainUtility_Platinum.o
fi

# Engine 55: MonoMaker_Platinum
if [ ! -f "${OUTPUT_DIR}/MonoMaker_Platinum.o" ]; then
    echo "  Compiling MonoMaker_Platinum..."
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ../JUCE_Plugin/Source/MonoMaker_Platinum.cpp -o ${OUTPUT_DIR}/MonoMaker_Platinum.o
fi

# Skip EngineFactory - we're directly instantiating engines
echo "  (Skipping EngineFactory - using direct instantiation)"

echo ""
echo "Step 3: Compiling test file..."
$CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c test_utility_realworld.cpp -o ${OUTPUT_DIR}/test_utility_realworld.o 2>&1 | grep -v "warning:" | head -20

echo ""
echo "Step 4: Linking..."

# Collect all object files
OBJECT_FILES="${OUTPUT_DIR}/test_utility_realworld.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/GranularCloud.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/ChaosGenerator.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/ChaosGenerator_Platinum.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/GainUtility_Platinum.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/MonoMaker_Platinum.o"

# Add only essential JUCE modules
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_audio_basics.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_audio_formats.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_dsp.o"

# Add juce_core if it exists with any suffix
if [ -f "${OUTPUT_DIR}/juce_core.o" ]; then
    OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_core.o"
elif [ -f "${OUTPUT_DIR}/juce_core_CompilationTime.o" ]; then
    OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_core_CompilationTime.o"
fi

# Add juce_events if available
if [ -f "${OUTPUT_DIR}/juce_events.o" ]; then
    OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_events.o"
fi

# Link
$CXX $OBJECT_FILES $FRAMEWORKS -o ${OUTPUT_DIR}/test_utility_realworld

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║                  BUILD SUCCESSFUL                        ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""
echo "Run test with: ${OUTPUT_DIR}/test_utility_realworld"
echo ""

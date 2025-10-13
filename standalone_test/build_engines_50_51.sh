#!/bin/bash
# Build script for Engines 50-51 test (GranularCloud & ChaosGenerator)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║   Building Engines 50-51 Test (GranularCloud & Chaos)   ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O2 -I. -I../JUCE_Plugin/Source"
JUCE_DIR="../JUCE/modules"

# JUCE module includes
JUCE_INCLUDES="-I${JUCE_DIR} -I${JUCE_DIR}/juce_audio_basics -I${JUCE_DIR}/juce_audio_formats -I${JUCE_DIR}/juce_core -I${JUCE_DIR}/juce_dsp -I${JUCE_DIR}/juce_events"

# JUCE preprocessor flags
JUCE_FLAGS="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"

# macOS frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework Foundation -framework Carbon -framework Cocoa -framework IOKit -framework QuartzCore -framework Security"

OUTPUT_DIR="build"
mkdir -p "$OUTPUT_DIR"

echo "Step 1: Checking for JUCE modules..."
# Check if JUCE modules are already compiled
NEED_COMPILE=0
for module in juce_audio_basics juce_core juce_dsp; do
    if [ ! -f "${OUTPUT_DIR}/${module}.o" ]; then
        NEED_COMPILE=1
        break
    fi
done

if [ $NEED_COMPILE -eq 1 ]; then
    echo "  Compiling JUCE modules..."
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_audio_basics/juce_audio_basics.mm -o ${OUTPUT_DIR}/juce_audio_basics.o
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_core/juce_core.mm -o ${OUTPUT_DIR}/juce_core.o
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ${JUCE_DIR}/juce_dsp/juce_dsp.mm -o ${OUTPUT_DIR}/juce_dsp.o
else
    echo "  JUCE modules already compiled"
fi

echo ""
echo "Step 2: Compiling engine implementations..."

# Compile GranularCloud
echo "  Compiling GranularCloud..."
$CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ../JUCE_Plugin/Source/GranularCloud.cpp -o ${OUTPUT_DIR}/GranularCloud.o

# Compile ChaosGenerator
echo "  Compiling ChaosGenerator..."
$CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ../JUCE_Plugin/Source/ChaosGenerator.cpp -o ${OUTPUT_DIR}/ChaosGenerator.o

# Compile EngineFactory if needed
if [ ! -f "${OUTPUT_DIR}/EngineFactory.o" ]; then
    echo "  Compiling EngineFactory..."
    $CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c ../JUCE_Plugin/Source/EngineFactory.cpp -o ${OUTPUT_DIR}/EngineFactory.o
fi

echo ""
echo "Step 3: Compiling test file..."
$CXX $CXXFLAGS $JUCE_INCLUDES $JUCE_FLAGS -c test_engines_50_51.cpp -o ${OUTPUT_DIR}/test_engines_50_51.o

echo ""
echo "Step 4: Linking..."

# Collect all object files
OBJECT_FILES="${OUTPUT_DIR}/test_engines_50_51.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/GranularCloud.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/ChaosGenerator.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_audio_basics.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_core.o"
OBJECT_FILES="$OBJECT_FILES ${OUTPUT_DIR}/juce_dsp.o"

# Link
$CXX $OBJECT_FILES $FRAMEWORKS -o ${OUTPUT_DIR}/test_engines_50_51

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║                  BUILD SUCCESSFUL                        ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""
echo "Run test with: ${OUTPUT_DIR}/test_engines_50_51"
echo ""

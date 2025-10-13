#!/bin/bash

echo "======================================================"
echo "Building Pitch Engine Performance Profiler"
echo "======================================================"

SOURCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
JUCE_DIR="${SOURCE_DIR}/JUCE"
PLUGIN_DIR="${SOURCE_DIR}/JUCE_Plugin/Source"
STANDALONE_DIR="${SOURCE_DIR}/standalone_test"

OUTPUT="${STANDALONE_DIR}/test_pitch_performance"

echo "Source directory: ${SOURCE_DIR}"
echo "Output: ${OUTPUT}"
echo ""

# Compiler flags
CXXFLAGS="-std=c++17 -O3 -Wall -Wno-deprecated-declarations"
CXXFLAGS="${CXXFLAGS} -I${JUCE_DIR}/modules"
CXXFLAGS="${CXXFLAGS} -I${SOURCE_DIR}/JUCE_Plugin/JuceLibraryCode"
CXXFLAGS="${CXXFLAGS} -I${PLUGIN_DIR}"

# JUCE defines
DEFINES="-DJUCE_STANDALONE_APPLICATION=1"
DEFINES="${DEFINES} -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
DEFINES="${DEFINES} -DJUCE_USE_CURL=0"
DEFINES="${DEFINES} -DJUCE_WEB_BROWSER=0"
DEFINES="${DEFINES} -DJUCE_USE_CAMERA=0"

# macOS frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI"
FRAMEWORKS="${FRAMEWORKS} -framework AudioToolbox -framework CoreFoundation"
FRAMEWORKS="${FRAMEWORKS} -framework AppKit -framework IOKit"

echo "Step 1: Compiling performance profiler..."
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${STANDALONE_DIR}/test_pitch_engines_performance.cpp" \
    -o "${STANDALONE_DIR}/build/test_pitch_performance.o"

if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed!"
    exit 1
fi

echo "Step 2: Compiling JUCE modules..."

# Compile minimal JUCE stubs as Objective-C++ on macOS
cat > "${STANDALONE_DIR}/juce_minimal_performance.mm" << 'EOF'
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#include "../../JUCE/modules/juce_core/juce_core.mm"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.mm"
#include "../../JUCE/modules/juce_events/juce_events.mm"
EOF

g++ ${CXXFLAGS} ${DEFINES} \
    -c "${STANDALONE_DIR}/juce_minimal_performance.mm" \
    -o "${STANDALONE_DIR}/build/juce_minimal_performance.o"

if [ $? -ne 0 ]; then
    echo "ERROR: JUCE compilation failed!"
    exit 1
fi

echo "Step 3: Compiling pitch engines..."

# PitchShifter
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/PitchShifter.cpp" \
    -o "${STANDALONE_DIR}/build/PitchShifter.o"

# DetuneDoubler
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/DetuneDoubler.cpp" \
    -o "${STANDALONE_DIR}/build/DetuneDoubler.o"

# IntelligentHarmonizer
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/IntelligentHarmonizer.cpp" \
    -o "${STANDALONE_DIR}/build/IntelligentHarmonizer.o"

# TapeEcho
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/TapeEcho.cpp" \
    -o "${STANDALONE_DIR}/build/TapeEcho.o"

# DigitalDelay
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/DigitalDelay.cpp" \
    -o "${STANDALONE_DIR}/build/DigitalDelay.o"

# MagneticDrumEcho
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/MagneticDrumEcho.cpp" \
    -o "${STANDALONE_DIR}/build/MagneticDrumEcho.o"

# BucketBrigadeDelay
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/BucketBrigadeDelay.cpp" \
    -o "${STANDALONE_DIR}/build/BucketBrigadeDelay.o"

# BufferRepeat
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/BufferRepeat.cpp" \
    -o "${STANDALONE_DIR}/build/BufferRepeat.o"

# Pitch shift strategies
g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/PhaseVocoderPitchShift.cpp" \
    -o "${STANDALONE_DIR}/build/PhaseVocoderPitchShift.o"

g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/SMBPitchShiftFixed.cpp" \
    -o "${STANDALONE_DIR}/build/SMBPitchShiftFixed.o"

g++ ${CXXFLAGS} ${DEFINES} \
    -c "${PLUGIN_DIR}/PitchShiftFactory.cpp" \
    -o "${STANDALONE_DIR}/build/PitchShiftFactory.o"

echo "Step 4: Linking..."

g++ -o "${OUTPUT}" \
    "${STANDALONE_DIR}/build/test_pitch_performance.o" \
    "${STANDALONE_DIR}/build/juce_minimal_performance.o" \
    "${STANDALONE_DIR}/build/PitchShifter.o" \
    "${STANDALONE_DIR}/build/DetuneDoubler.o" \
    "${STANDALONE_DIR}/build/IntelligentHarmonizer.o" \
    "${STANDALONE_DIR}/build/TapeEcho.o" \
    "${STANDALONE_DIR}/build/DigitalDelay.o" \
    "${STANDALONE_DIR}/build/MagneticDrumEcho.o" \
    "${STANDALONE_DIR}/build/BucketBrigadeDelay.o" \
    "${STANDALONE_DIR}/build/BufferRepeat.o" \
    "${STANDALONE_DIR}/build/PhaseVocoderPitchShift.o" \
    "${STANDALONE_DIR}/build/SMBPitchShiftFixed.o" \
    "${STANDALONE_DIR}/build/PitchShiftFactory.o" \
    ${FRAMEWORKS}

if [ $? -eq 0 ]; then
    echo ""
    echo "======================================================"
    echo "BUILD SUCCESSFUL!"
    echo "======================================================"
    echo "Executable: ${OUTPUT}"
    echo ""
    echo "Running performance profiler..."
    echo ""
    "${OUTPUT}"
else
    echo ""
    echo "ERROR: Linking failed!"
    exit 1
fi

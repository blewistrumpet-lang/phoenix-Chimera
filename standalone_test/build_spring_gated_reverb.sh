#!/bin/bash

# Build script for Spring & Gated Reverb test (Engines 42-43)
set -e

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Building Spring & Gated Reverb Test (Engines 42-43)        ║"
echo "╚══════════════════════════════════════════════════════════════╝"

# Project paths
PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SOURCE="${PROJECT_ROOT}/JUCE_Plugin/Source"
STANDALONE_DIR="${PROJECT_ROOT}/standalone_test"
BUILD_DIR="${STANDALONE_DIR}/build"

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2"
INCLUDES="-I${JUCE_DIR}/modules -I${PLUGIN_SOURCE} -I${STANDALONE_DIR}"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework AudioUnit -framework Foundation -framework CoreFoundation -framework AppKit -framework IOKit -framework Security"

# Check if we have pre-compiled JUCE objects
JUCE_CORE=""
JUCE_AUDIO=""
JUCE_DSP=""
JUCE_AUDIO_FORMATS=""

# Try different locations for JUCE objects
if [ -f "${STANDALONE_DIR}/juce_core.o" ]; then
    JUCE_CORE="${STANDALONE_DIR}/juce_core.o"
elif [ -f "${BUILD_DIR}/juce_core.o" ]; then
    JUCE_CORE="${BUILD_DIR}/juce_core.o"
fi

if [ -f "${STANDALONE_DIR}/juce_audio_basics.o" ]; then
    JUCE_AUDIO="${STANDALONE_DIR}/juce_audio_basics.o"
elif [ -f "${BUILD_DIR}/juce_audio_basics.o" ]; then
    JUCE_AUDIO="${BUILD_DIR}/juce_audio_basics.o"
fi

if [ -f "${STANDALONE_DIR}/juce_dsp.o" ]; then
    JUCE_DSP="${STANDALONE_DIR}/juce_dsp.o"
elif [ -f "${BUILD_DIR}/juce_dsp.o" ]; then
    JUCE_DSP="${BUILD_DIR}/juce_dsp.o"
fi

if [ -f "${STANDALONE_DIR}/juce_audio_formats.o" ]; then
    JUCE_AUDIO_FORMATS="${STANDALONE_DIR}/juce_audio_formats.o"
elif [ -f "${BUILD_DIR}/juce_audio_formats.o" ]; then
    JUCE_AUDIO_FORMATS="${BUILD_DIR}/juce_audio_formats.o"
elif [ -f "${BUILD_DIR}/obj/juce_audio_formats.o" ]; then
    JUCE_AUDIO_FORMATS="${BUILD_DIR}/obj/juce_audio_formats.o"
fi

if [ -n "$JUCE_CORE" ] && [ -n "$JUCE_AUDIO" ] && [ -n "$JUCE_DSP" ]; then
    echo "✓ Using pre-compiled JUCE objects"
    JUCE_OBJECTS="$JUCE_CORE $JUCE_AUDIO $JUCE_DSP"
    if [ -n "$JUCE_AUDIO_FORMATS" ]; then
        JUCE_OBJECTS="$JUCE_OBJECTS $JUCE_AUDIO_FORMATS"
        echo "  Including juce_audio_formats.o"
    fi
    # Note: Skipping juce_graphics to avoid harfbuzz/SheenBidi dependencies
    # Not needed for audio engine testing
else
    echo "✗ Error: JUCE objects not found."
    echo "  juce_core.o: $JUCE_CORE"
    echo "  juce_audio_basics.o: $JUCE_AUDIO"
    echo "  juce_dsp.o: $JUCE_DSP"
    exit 1
fi

# Compile test
echo "→ Compiling test_spring_gated_reverb.cpp..."
${CXX} ${CXXFLAGS} ${INCLUDES} ${DEFINES} \
    -c "${STANDALONE_DIR}/test_spring_gated_reverb.cpp" \
    -o "${BUILD_DIR}/test_spring_gated_reverb.o"

# Compile engine sources (only those needed for Spring & Gated Reverb)
# Note: EngineBase is header-only, no .cpp file
# Using minimal EngineFactory to avoid linking all engines

echo "→ Compiling MinimalEngineFactory_Spring_Gated.cpp..."
${CXX} ${CXXFLAGS} ${INCLUDES} ${DEFINES} \
    -c "${STANDALONE_DIR}/MinimalEngineFactory_Spring_Gated.cpp" \
    -o "${BUILD_DIR}/MinimalEngineFactory_Spring_Gated.o"

# Compile SpringReverb
echo "→ Compiling SpringReverb.cpp..."
${CXX} ${CXXFLAGS} ${INCLUDES} ${DEFINES} \
    -c "${PLUGIN_SOURCE}/SpringReverb.cpp" \
    -o "${BUILD_DIR}/SpringReverb.o"

# Compile GatedReverb
echo "→ Compiling GatedReverb.cpp..."
${CXX} ${CXXFLAGS} ${INCLUDES} ${DEFINES} \
    -c "${PLUGIN_SOURCE}/GatedReverb.cpp" \
    -o "${BUILD_DIR}/GatedReverb.o"

# Compile AdvancedSpringDispersion (dependency)
if [ -f "${PLUGIN_SOURCE}/AdvancedSpringDispersion.cpp" ]; then
    echo "→ Compiling AdvancedSpringDispersion.cpp..."
    ${CXX} ${CXXFLAGS} ${INCLUDES} ${DEFINES} \
        -c "${PLUGIN_SOURCE}/AdvancedSpringDispersion.cpp" \
        -o "${BUILD_DIR}/AdvancedSpringDispersion.o" 2>/dev/null || echo "  (skipped)"
fi

# Link everything together
echo "→ Linking..."
OBJECTS="${BUILD_DIR}/test_spring_gated_reverb.o \
         ${BUILD_DIR}/MinimalEngineFactory_Spring_Gated.o \
         ${BUILD_DIR}/SpringReverb.o \
         ${BUILD_DIR}/GatedReverb.o \
         ${JUCE_OBJECTS}"

# Add AdvancedSpringDispersion if it was compiled
if [ -f "${BUILD_DIR}/AdvancedSpringDispersion.o" ]; then
    OBJECTS="$OBJECTS ${BUILD_DIR}/AdvancedSpringDispersion.o"
fi

${CXX} ${OBJECTS} ${FRAMEWORKS} -o "${BUILD_DIR}/test_spring_gated_reverb"

echo ""
echo "✓ Build complete! Executable: ${BUILD_DIR}/test_spring_gated_reverb"
echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Running Spring & Gated Reverb Test Suite                   ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

"${BUILD_DIR}/test_spring_gated_reverb"

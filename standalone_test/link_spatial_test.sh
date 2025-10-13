#!/bin/bash

# Link script for spatial_test using precompiled objects

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Linking Spatial Test with Precompiled Objects            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

BUILD_DIR="build"
CXX="clang++"
CXXFLAGS="-std=c++17 -O3"

# Compile just the test file
echo "Compiling spatial_test.cpp..."
$CXX $CXXFLAGS -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 \
    -DJUCE_MODULE_AVAILABLE_juce_audio_processors=1 \
    -DJUCE_MODULE_AVAILABLE_juce_core=1 \
    -DJUCE_MODULE_AVAILABLE_juce_dsp=1 \
    -DJUCE_MODULE_AVAILABLE_juce_events=1 \
    -I../JUCE_Plugin/Source \
    -I/Users/Branden/JUCE/modules \
    -I. \
    -c spatial_test.cpp -o ${BUILD_DIR}/obj/spatial_test.o

# Link with all existing objects
echo "Linking..."

# Collect all needed object files
OBJECT_FILES=(
    "${BUILD_DIR}/obj/spatial_test.o"
    "${BUILD_DIR}/obj/EngineFactory.o"
    "${BUILD_DIR}/obj/StereoWidener.o"
    "${BUILD_DIR}/obj/StereoImager.o"
    "${BUILD_DIR}/obj/DimensionExpander.o"
    "${BUILD_DIR}/obj/SpectralFreeze.o"
    "${BUILD_DIR}/obj/SpectralGate_Platinum.o"
    "${BUILD_DIR}/obj/PhasedVocoder.o"
    "${BUILD_DIR}/obj/GranularCloud.o"
    "${BUILD_DIR}/obj/ChaosGenerator.o"
    "${BUILD_DIR}/obj/FeedbackNetwork.o"
    "${BUILD_DIR}/obj/PhaseAlign_Platinum.o"
    "${BUILD_DIR}/obj/juce_core.o"
    "${BUILD_DIR}/obj/juce_audio_basics.o"
    "${BUILD_DIR}/obj/juce_audio_processors.o"
    "${BUILD_DIR}/obj/juce_dsp.o"
    "${BUILD_DIR}/obj/juce_events.o"
)

# Add ALL engine objects from build directory (for EngineFactory dependencies)
for obj in ${BUILD_DIR}/obj/*.o; do
    if [ -f "$obj" ]; then
        # Skip if already in list
        skip=0
        for existing in "${OBJECT_FILES[@]}"; do
            if [ "$obj" = "$existing" ]; then
                skip=1
                break
            fi
        done
        if [ $skip -eq 0 ]; then
            OBJECT_FILES+=("$obj")
        fi
    fi
done

FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework CoreMidi -framework IOKit -framework AppKit -framework WebKit -framework Security"

$CXX $CXXFLAGS "${OBJECT_FILES[@]}" $FRAMEWORKS -o "${BUILD_DIR}/spatial_test"

if [ -f "${BUILD_DIR}/spatial_test" ]; then
    echo ""
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║  ✓ Link successful!                                        ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Run with: cd build && ./spatial_test"
    echo ""
else
    echo "✗ Link failed!"
    exit 1
fi

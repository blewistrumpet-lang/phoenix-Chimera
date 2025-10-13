#!/bin/bash

# Build script for spatial_test.cpp

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Building Spatial/Spectral/Special Test Suite             ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Paths
JUCE_PATH="/Users/Branden/JUCE/modules"
SOURCE_PATH="../JUCE_Plugin/Source"
BUILD_DIR="build"

# Create build directory
mkdir -p "$BUILD_DIR"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O3 -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_audio_processors=1 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_dsp=1 -DJUCE_MODULE_AVAILABLE_juce_events=1"

# Include paths
INCLUDES="-I${SOURCE_PATH} -I${JUCE_PATH} -I."

# Required source files (engines and infrastructure)
SOURCES=(
    "spatial_test.cpp"
    "${SOURCE_PATH}/EngineFactory.cpp"
    "${SOURCE_PATH}/EngineBase.cpp"

    # Spatial engines (44-46)
    "${SOURCE_PATH}/StereoWidener.cpp"
    "${SOURCE_PATH}/StereoImager.cpp"
    "${SOURCE_PATH}/DimensionExpander.cpp"

    # Spectral engines (47-49)
    "${SOURCE_PATH}/SpectralFreeze.cpp"
    "${SOURCE_PATH}/SpectralGate_Platinum.cpp"
    "${SOURCE_PATH}/PhasedVocoder.cpp"

    # Granular/Chaos (50-52)
    "${SOURCE_PATH}/GranularCloud.cpp"
    "${SOURCE_PATH}/ChaosGenerator.cpp"
    "${SOURCE_PATH}/FeedbackNetwork.cpp"

    # Utility (56)
    "${SOURCE_PATH}/PhaseAlign_Platinum.cpp"
)

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework CoreMidi -framework IOKit -framework AppKit -framework WebKit"

# JUCE modules to compile
JUCE_MODULES=(
    "${JUCE_PATH}/juce_core/juce_core.mm"
    "${JUCE_PATH}/juce_audio_basics/juce_audio_basics.mm"
    "${JUCE_PATH}/juce_audio_processors/juce_audio_processors.mm"
    "${JUCE_PATH}/juce_dsp/juce_dsp.mm"
    "${JUCE_PATH}/juce_events/juce_events.mm"
)

echo "Step 1: Compiling JUCE modules..."
for module in "${JUCE_MODULES[@]}"; do
    module_name=$(basename "$module" .mm)
    obj_file="${BUILD_DIR}/obj/${module_name}.o"

    if [ ! -f "$obj_file" ]; then
        echo "  Compiling ${module_name}..."
        mkdir -p "${BUILD_DIR}/obj"
        $CXX $CXXFLAGS $INCLUDES -c "$module" -o "$obj_file"
    else
        echo "  ✓ ${module_name} (cached)"
    fi
done

echo ""
echo "Step 2: Compiling engine sources..."
OBJECT_FILES=()

for source in "${SOURCES[@]}"; do
    if [ -f "$source" ]; then
        source_name=$(basename "$source" .cpp)
        obj_file="${BUILD_DIR}/obj/${source_name}.o"

        echo "  Compiling ${source_name}..."
        $CXX $CXXFLAGS $INCLUDES -c "$source" -o "$obj_file"
        OBJECT_FILES+=("$obj_file")
    else
        echo "  ⚠️  Warning: $source not found, skipping..."
    fi
done

# Add JUCE module objects
for module in "${JUCE_MODULES[@]}"; do
    module_name=$(basename "$module" .mm)
    OBJECT_FILES+=("${BUILD_DIR}/obj/${module_name}.o")
done

echo ""
echo "Step 3: Linking..."
$CXX $CXXFLAGS "${OBJECT_FILES[@]}" $FRAMEWORKS -o "${BUILD_DIR}/spatial_test"

if [ -f "${BUILD_DIR}/spatial_test" ]; then
    echo ""
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║  ✓ Build successful!                                       ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Run with: ./build/spatial_test"
    echo ""
else
    echo "✗ Build failed!"
    exit 1
fi

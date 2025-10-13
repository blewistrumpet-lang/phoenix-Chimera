#!/bin/bash
# Quick build for testing engines 46-48

set -e

echo "Building engines 46-48 test..."

JUCE_PATH="/Users/Branden/JUCE/modules"
SOURCE_PATH="../JUCE_Plugin/Source"
BUILD_DIR="build"

mkdir -p "$BUILD_DIR"

CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
INCLUDES="-I${SOURCE_PATH} -I${JUCE_PATH} -I."
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework CoreMidi -framework IOKit -framework AppKit -framework WebKit"

# Compile JUCE modules (use cached if available)
echo "Compiling JUCE modules..."
for module in juce_core juce_audio_basics juce_dsp juce_events juce_audio_processors; do
    if [ ! -f "${BUILD_DIR}/${module}.o" ]; then
        echo "  ${module}..."
        $CXX $CXXFLAGS $INCLUDES -c "${JUCE_PATH}/${module}/${module}.mm" -o "${BUILD_DIR}/${module}.o"
    fi
done

# Compile engines 46-48
echo "Compiling engines..."
ENGINES=(
    "EngineFactory.cpp"
    "EngineBase.cpp"
    "DimensionExpander.cpp"
    "SpectralFreeze.cpp"
    "SpectralGate_Platinum.cpp"
)

for engine in "${ENGINES[@]}"; do
    if [ -f "${SOURCE_PATH}/${engine}" ]; then
        name=$(basename "$engine" .cpp)
        echo "  ${name}..."
        $CXX $CXXFLAGS $INCLUDES -c "${SOURCE_PATH}/${engine}" -o "${BUILD_DIR}/${name}.o"
    fi
done

# Compile test
echo "Compiling test..."
$CXX $CXXFLAGS $INCLUDES -c test_spatial_engines_46_48.cpp -o "${BUILD_DIR}/test_46_48.o"

# Link
echo "Linking..."
$CXX ${BUILD_DIR}/*.o $FRAMEWORKS -o test_spatial_46_48

echo ""
echo "✓ Build complete: ./test_spatial_46_48"

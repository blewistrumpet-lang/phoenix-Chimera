#!/bin/bash

# Build script for Delay Engines Test (Engines 35-36)

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Building Delay Engines Test Suite (35-36)                ║"
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
CXXFLAGS="-std=c++17 -O3 -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_events=1 -DJUCE_MODULE_AVAILABLE_juce_graphics=1 -DJUCE_MODULE_AVAILABLE_juce_gui_basics=1 -DJUCE_MODULE_AVAILABLE_juce_data_structures=1"

# Include paths
INCLUDES="-I${SOURCE_PATH} -I${JUCE_PATH} -I."

# Required source files (engines and infrastructure)
SOURCES=(
    "test_delay_engines_35_36.cpp"
    "${SOURCE_PATH}/EngineFactory.cpp"

    # Delay engines (35-36)
    "${SOURCE_PATH}/BucketBrigadeDelay.cpp"
    "${SOURCE_PATH}/MagneticDrumEcho.cpp"
)

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework CoreMidi -framework IOKit -framework AppKit -framework WebKit -framework Security -framework QuartzCore"

# JUCE modules to compile
JUCE_MODULES=(
    "${JUCE_PATH}/juce_core/juce_core.mm"
    "${JUCE_PATH}/juce_audio_basics/juce_audio_basics.mm"
    "${JUCE_PATH}/juce_events/juce_events.mm"
    "${JUCE_PATH}/juce_graphics/juce_graphics.mm"
    "${JUCE_PATH}/juce_gui_basics/juce_gui_basics.mm"
    "${JUCE_PATH}/juce_data_structures/juce_data_structures.mm"
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
    source_name=$(basename "$source" .cpp)
    obj_file="${BUILD_DIR}/obj/${source_name}.o"
    OBJECT_FILES+=("$obj_file")

    echo "  Compiling ${source_name}..."
    $CXX $CXXFLAGS $INCLUDES -c "$source" -o "$obj_file"
done

# Add JUCE module objects
for module in "${JUCE_MODULES[@]}"; do
    module_name=$(basename "$module" .mm)
    OBJECT_FILES+=("${BUILD_DIR}/obj/${module_name}.o")
done

echo ""
echo "Step 3: Linking..."
OUTPUT="delay_test"
$CXX "${OBJECT_FILES[@]}" $FRAMEWORKS -o "$OUTPUT"

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Build successful!                                         ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Running test..."
echo "================================================================"
./"$OUTPUT"

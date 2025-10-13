#!/bin/bash

set -e

echo "╔════════════════════════════════════════════════════════════════════════╗"
echo "║               Building DC Offset Test (Using EngineFactory)           ║"
echo "╚════════════════════════════════════════════════════════════════════════╝"
echo ""

# Paths
JUCE_PATH="/Users/Branden/JUCE/modules"
SOURCE_PATH="../JUCE_Plugin/Source"
BUILD_DIR="build"

# Create build directory
mkdir -p "$BUILD_DIR/obj"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_dsp=1"

# Include paths
INCLUDES="-I${SOURCE_PATH} -I${JUCE_PATH} -I."

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework CoreMidi -framework IOKit -framework AppKit -framework WebKit -framework Security -framework QuartzCore"

# JUCE modules
JUCE_MODULES=(
    "${JUCE_PATH}/juce_core/juce_core.mm"
    "${JUCE_PATH}/juce_audio_basics/juce_audio_basics.mm"
    "${JUCE_PATH}/juce_dsp/juce_dsp.mm"
)

echo "Step 1: Compiling JUCE modules (if needed)..."
for module in "${JUCE_MODULES[@]}"; do
    module_name=$(basename "$module" .mm)
    obj_file="${BUILD_DIR}/obj/${module_name}.o"

    if [ ! -f "$obj_file" ]; then
        echo "  Compiling ${module_name}..."
        $CXX $CXXFLAGS $INCLUDES -c "$module" -o "$obj_file" 2>&1 | grep -v "warning:" || true
    else
        echo "  ✓ ${module_name} (cached)"
    fi
done

echo ""
echo "Step 2: Compiling test source..."
test_obj="${BUILD_DIR}/obj/test_dc_offset.o"
echo "  Compiling test_dc_offset..."
$CXX $CXXFLAGS $INCLUDES -c "test_dc_offset.cpp" -o "$test_obj" 2>&1 | grep -v "warning:" || true

echo ""
echo "Step 3: Collecting existing engine object files..."
# Collect all existing .o files from build/obj that are engines
OBJECT_FILES=("$test_obj")

# Add all engine .o files that exist in build/obj
for obj in ${BUILD_DIR}/obj/*.o; do
    if [ -f "$obj" ]; then
        filename=$(basename "$obj")
        # Skip JUCE modules and test file
        if [[ ! "$filename" =~ ^juce_ ]] && [[ "$filename" != "test_dc_offset.o" ]]; then
            OBJECT_FILES+=("$obj")
            echo "  ✓ Using $(basename $obj)"
        fi
    fi
done

# Add JUCE modules
for module in "${JUCE_MODULES[@]}"; do
    module_name=$(basename "$module" .mm)
    OBJECT_FILES+=("${BUILD_DIR}/obj/${module_name}.o")
done

echo ""
echo "Step 4: Linking..."
OUTPUT="test_dc_offset"
$CXX $CXXFLAGS ${OBJECT_FILES[@]} $FRAMEWORKS -o "$OUTPUT" 2>&1

if [ $? -eq 0 ]; then
    echo ""
    echo "╔════════════════════════════════════════════════════════════════════════╗"
    echo "║                       BUILD SUCCESSFUL!                                ║"
    echo "╚════════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Executable: ./${OUTPUT}"
    echo ""
    echo "To run the test:"
    echo "  ./${OUTPUT}"
    echo ""
else
    echo ""
    echo "ERROR: Build failed"
    exit 1
fi

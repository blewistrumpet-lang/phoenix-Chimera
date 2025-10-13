#!/bin/bash

set -e

echo "╔════════════════════════════════════════════════════════════════════════╗"
echo "║               Building DC Offset Test (Clean Build)                   ║"
echo "╚════════════════════════════════════════════════════════════════════════╝"
echo ""

# Paths
JUCE_PATH="/Users/Branden/JUCE/modules"
SOURCE_PATH="../JUCE_Plugin/Source"
BUILD_DIR="build"

# Create build directory
mkdir -p "$BUILD_DIR/dc_test_obj"

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

echo "Step 1: Compiling JUCE modules..."
for module in "${JUCE_MODULES[@]}"; do
    module_name=$(basename "$module" .mm)
    obj_file="${BUILD_DIR}/dc_test_obj/${module_name}.o"

    if [ ! -f "$obj_file" ]; then
        echo "  Compiling ${module_name}..."
        $CXX $CXXFLAGS $INCLUDES -c "$module" -o "$obj_file" 2>&1 | grep -v "warning:" || true
    else
        echo "  ✓ ${module_name} (cached)"
    fi
done

echo ""
echo "Step 2: Compiling test source..."
test_obj="${BUILD_DIR}/dc_test_obj/test_dc_offset.o"
echo "  Compiling test_dc_offset..."
$CXX $CXXFLAGS $INCLUDES -c "test_dc_offset.cpp" -o "$test_obj" 2>&1 | grep -v "warning:" || true

echo ""
echo "Step 3: Collecting required engine object files..."
# Collect only engine .o files (not test files)
OBJECT_FILES=("$test_obj")

# Only add engine object files that exist
for obj in ${BUILD_DIR}/obj/*.o; do
    if [ -f "$obj" ]; then
        filename=$(basename "$obj")
        # Only include actual engine files, skip test files and JUCE modules
        if [[ ! "$filename" =~ ^juce_ ]] && \
           [[ ! "$filename" =~ _test ]] && \
           [[ ! "$filename" =~ test_ ]] && \
           [[ ! "$filename" =~ standalone_test ]] && \
           [[ "$filename" != "ComprehensiveTHDEngineFactory.o" ]]; then
            # Check if it's an engine file by checking for common engine suffixes
            if [[ "$filename" =~ (Engine|Factory|Compressor|Filter|Delay|Reverb|Phaser|Chorus|Tremolo|Shifter|Doubler|Distortion|Fuzz|Overdrive|Saturator|Exciter|Crusher|Folder|Freeze|Gate|Shaper|Limiter|EQ|Resonator|Vocoder|Harmonizer|Cloud|Chaos|Network|Widener|Imager|Expander|Utility|Processor|Maker|Align)\.o$ ]]; then
                OBJECT_FILES+=("$obj")
                echo "  ✓ $(basename $obj)"
            fi
        fi
    fi
done

# Add JUCE modules
for module in "${JUCE_MODULES[@]}"; do
    module_name=$(basename "$module" .mm)
    OBJECT_FILES+=("${BUILD_DIR}/dc_test_obj/${module_name}.o")
done

echo ""
echo "Total object files: ${#OBJECT_FILES[@]}"
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

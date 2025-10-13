#!/bin/bash
# Build script for Extreme Parameter Stress Test
# Uses the existing build infrastructure from Makefile

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════════╗"
echo "║     Building Extreme Parameter Stress Test Suite          ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"
OUTPUT="$BUILD_DIR/stress_test_extreme_parameters"

# Check JUCE exists
if [ ! -d "$JUCE_DIR/modules" ]; then
    echo "ERROR: JUCE not found at $JUCE_DIR"
    exit 1
fi

# Create build directories
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Common settings
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1 -DJUCE_BUILD_INFO_SUFFIX=\"\""
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
OBJCPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

# Check if JUCE objects exist, if not compile them
JUCE_OBJS=""
JUCE_MODULES="juce_core juce_audio_basics juce_audio_formats juce_audio_processors juce_dsp juce_events juce_data_structures juce_graphics juce_gui_basics juce_gui_extra"

echo "Step 1: Checking/compiling JUCE modules..."
for module in $JUCE_MODULES; do
    OBJ_FILE="$OBJ_DIR/${module}.o"
    if [ ! -f "$OBJ_FILE" ]; then
        echo "  - Compiling $module..."
        clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/../JuceLibraryCode/include_${module}.mm" -o "$OBJ_FILE" 2>&1 | grep -v "warning:" || true
    else
        echo "  ✓ Using cached $module"
    fi
    JUCE_OBJS="$JUCE_OBJS $OBJ_FILE"
done

# Compile juce_core_CompilationTime if needed
CORE_TIME_OBJ="$OBJ_DIR/juce_core_CompilationTime.o"
if [ ! -f "$CORE_TIME_OBJ" ]; then
    echo "  - Compiling juce_core_CompilationTime..."
    clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES -c "$JUCE_DIR/modules/juce_core/juce_core_CompilationTime.cpp" -o "$CORE_TIME_OBJ" 2>&1 | grep -v "warning:" || true
else
    echo "  ✓ Using cached juce_core_CompilationTime"
fi
JUCE_OBJS="$JUCE_OBJS $CORE_TIME_OBJ"

# Compile SheenBidi if needed
SHEENBIDI_OBJ="$OBJ_DIR/SheenBidi.o"
if [ ! -f "$SHEENBIDI_OBJ" ]; then
    echo "  - Compiling SheenBidi..."
    clang -std=c11 -O2 $INCLUDES -DSB_CONFIG_UNITY -c "$JUCE_DIR/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c" -o "$SHEENBIDI_OBJ" 2>&1 | grep -v "warning:" || true
else
    echo "  ✓ Using cached SheenBidi"
fi
JUCE_OBJS="$JUCE_OBJS $SHEENBIDI_OBJ"

echo ""
echo "Step 2: Checking/compiling engine sources..."

# Compile engines from required_engines.txt
ENGINE_OBJS=""
while IFS= read -r engine_cpp; do
    if [ -n "$engine_cpp" ]; then
        engine_name=$(basename "$engine_cpp" .cpp)
        OBJ_FILE="$OBJ_DIR/${engine_name}.o"

        if [ ! -f "$OBJ_FILE" ]; then
            echo "  - Compiling $engine_name..."
            clang++ $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/$engine_cpp" -o "$OBJ_FILE" 2>&1 | grep -v "warning:" || true
        else
            echo "  ✓ Using cached $engine_name"
        fi
        ENGINE_OBJS="$ENGINE_OBJS $OBJ_FILE"
    fi
done < required_engines.txt

echo ""
echo "Step 3: Compiling stress test..."
TEST_OBJ="$OBJ_DIR/stress_test_extreme_parameters.o"
echo "  - stress_test_extreme_parameters.cpp"
clang++ $CPP_FLAGS $INCLUDES $DEFINES -c "stress_test_extreme_parameters.cpp" -o "$TEST_OBJ"

echo ""
echo "Step 4: Linking..."
clang++ -o "$OUTPUT" \
    "$TEST_OBJ" \
    $ENGINE_OBJS \
    $JUCE_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz

echo ""
echo "════════════════════════════════════════════════════════════"
echo "✓ Build complete!"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Executable: $OUTPUT"
echo ""
echo "To run the stress test:"
echo "  $OUTPUT"
echo ""

#!/bin/bash
# Build script for fixed ShimmerReverb test

set -e

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"

echo "Building Fixed ShimmerReverb Test..."
echo "====================================="

# Compiler settings
CXX="clang++"
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
OBJCPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

# Include paths
INCLUDES="-I. -I${PLUGIN_SRC} -I${PLUGIN_SRC}/../JuceLibraryCode -I${JUCE_DIR}/modules"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation \
            -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security \
            -framework QuartzCore -framework CoreImage -framework CoreGraphics \
            -framework CoreText -framework WebKit -framework DiscRecording"

# Defines
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
         -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

mkdir -p "$BUILD_DIR"

# Compile test
echo "Compiling test_shimmer_fixed.cpp..."
$CXX $CPP_FLAGS $INCLUDES $DEFINES -c test_shimmer_fixed.cpp -o "$BUILD_DIR/test_shimmer_fixed.o"

# Compile ShimmerReverb
echo "Compiling ShimmerReverb.cpp (with fixes)..."
$CXX $CPP_FLAGS $INCLUDES $DEFINES -c "${PLUGIN_SRC}/ShimmerReverb.cpp" -o "$BUILD_DIR/ShimmerReverb_fixed.o"

# Compile JUCE modules if not already compiled
JUCE_MODULES=(
    "juce_core"
    "juce_audio_basics"
    "juce_audio_formats"
    "juce_audio_processors"
    "juce_dsp"
    "juce_events"
    "juce_data_structures"
    "juce_graphics"
    "juce_gui_basics"
    "juce_gui_extra"
)

echo "Compiling JUCE modules..."
for module in "${JUCE_MODULES[@]}"; do
    obj_file="$BUILD_DIR/${module}.o"
    if [ ! -f "$obj_file" ]; then
        echo "  → Compiling $module"
        $CXX $OBJCPP_FLAGS $INCLUDES $DEFINES -c "${JUCE_DIR}/modules/${module}/${module}.cpp" -o "$obj_file"
    fi
done

# Compile juce_core_CompilationTime
if [ ! -f "$BUILD_DIR/juce_core_CompilationTime.o" ]; then
    echo "  → Compiling juce_core_CompilationTime"
    $CXX $OBJCPP_FLAGS $INCLUDES $DEFINES -c "${JUCE_DIR}/modules/juce_core/juce_core_CompilationTime.cpp" -o "$BUILD_DIR/juce_core_CompilationTime.o"
fi

# Compile SheenBidi
if [ ! -f "$BUILD_DIR/SheenBidi.o" ]; then
    echo "  → Compiling SheenBidi"
    clang -std=c11 -O2 $INCLUDES -DSB_CONFIG_UNITY -c "${JUCE_DIR}/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c" -o "$BUILD_DIR/SheenBidi.o"
fi

# Link
echo "Linking test_shimmer_fixed..."
$CXX $CPP_FLAGS \
    "$BUILD_DIR/test_shimmer_fixed.o" \
    "$BUILD_DIR/ShimmerReverb_fixed.o" \
    "$BUILD_DIR"/juce_*.o \
    "$BUILD_DIR/SheenBidi.o" \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_shimmer_fixed"

echo ""
echo "✓ Build complete: $BUILD_DIR/test_shimmer_fixed"
echo ""

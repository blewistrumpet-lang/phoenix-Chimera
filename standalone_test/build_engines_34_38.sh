#!/bin/bash
# Build script for engines 34-38 test

set -e

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"

echo "Building Engines 34-38 Test..."
echo "========================================"

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

# Required engine sources
ENGINES=(
    "TapeEcho.cpp"
    "DigitalDelay.cpp"
    "MagneticDrumEcho.cpp"
    "BucketBrigadeDelay.cpp"
    "BufferRepeat_Platinum.cpp"
)

# Compile test
echo "Compiling test_engines_34_38_simple.cpp..."
$CXX $CPP_FLAGS $INCLUDES $DEFINES -c test_engines_34_38_simple.cpp -o "$BUILD_DIR/test_engines_34_38_simple.o"

# Compile engines if not already compiled
echo "Compiling engine sources..."
for engine in "${ENGINES[@]}"; do
    obj_file="$BUILD_DIR/${engine%.cpp}.o"
    if [ ! -f "$obj_file" ]; then
        echo "  -> Compiling $engine"
        $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "${PLUGIN_SRC}/$engine" -o "$obj_file"
    else
        echo "  -> Using cached $engine"
    fi
done

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
        echo "  -> Compiling $module"
        $CXX $OBJCPP_FLAGS $INCLUDES $DEFINES -c "${JUCE_DIR}/modules/${module}/${module}.cpp" -o "$obj_file"
    else
        echo "  -> Using cached $module"
    fi
done

# Compile juce_core_CompilationTime
if [ ! -f "$BUILD_DIR/juce_core_CompilationTime.o" ]; then
    echo "  -> Compiling juce_core_CompilationTime"
    $CXX $OBJCPP_FLAGS $INCLUDES $DEFINES -c "${JUCE_DIR}/modules/juce_core/juce_core_CompilationTime.cpp" -o "$BUILD_DIR/juce_core_CompilationTime.o"
fi

# Compile SheenBidi
if [ ! -f "$BUILD_DIR/SheenBidi.o" ]; then
    echo "  -> Compiling SheenBidi"
    clang -std=c11 -O2 $INCLUDES -DSB_CONFIG_UNITY -c "${JUCE_DIR}/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c" -o "$BUILD_DIR/SheenBidi.o"
fi

# Link (only link necessary objects, not all of them)
echo "Linking test_engines_34_38..."
$CXX $CPP_FLAGS \
    "$BUILD_DIR/test_engines_34_38_simple.o" \
    "$BUILD_DIR/TapeEcho.o" \
    "$BUILD_DIR/DigitalDelay.o" \
    "$BUILD_DIR/MagneticDrumEcho.o" \
    "$BUILD_DIR/BucketBrigadeDelay.o" \
    "$BUILD_DIR/BufferRepeat_Platinum.o" \
    "$BUILD_DIR/juce_core.o" \
    "$BUILD_DIR/juce_audio_basics.o" \
    "$BUILD_DIR/juce_audio_formats.o" \
    "$BUILD_DIR/juce_audio_processors.o" \
    "$BUILD_DIR/juce_dsp.o" \
    "$BUILD_DIR/juce_events.o" \
    "$BUILD_DIR/juce_data_structures.o" \
    "$BUILD_DIR/juce_graphics.o" \
    "$BUILD_DIR/juce_gui_basics.o" \
    "$BUILD_DIR/juce_gui_extra.o" \
    "$BUILD_DIR/juce_core_CompilationTime.o" \
    "$BUILD_DIR/SheenBidi.o" \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_engines_34_38"

echo ""
echo "Build complete: $BUILD_DIR/test_engines_34_38"
echo ""

# Check if executable was created
if [ -f "$BUILD_DIR/test_engines_34_38" ]; then
    echo "Running test..."
    echo "================================================================"
    "$BUILD_DIR/test_engines_34_38"
else
    echo "ERROR: Executable not created"
    exit 1
fi

#!/bin/bash

# Build script for Latency Measurement Suite
# Compiles and links the comprehensive latency test for all pitch, reverb, and delay engines

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════════════════════════╗"
echo "║        Building Latency Measurement Suite for ChimeraPhoenix              ║"
echo "╚════════════════════════════════════════════════════════════════════════════╝"
echo ""

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

# Create directories
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Compiler settings
CXX="clang++"
CC="clang"
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
OBJCPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

# Include paths
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation \
            -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security \
            -framework QuartzCore -framework CoreImage -framework CoreGraphics \
            -framework CoreText -framework WebKit -framework DiscRecording"

# Defines
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
         -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1 \
         -DJUCE_BUILD_INFO_SUFFIX=\"\""

# Check if required_engines.txt exists
if [ ! -f "required_engines.txt" ]; then
    echo "ERROR: required_engines.txt not found!"
    echo "This file should list all engine source files needed."
    exit 1
fi

echo "Step 1: Compiling JUCE modules..."
echo "------------------------------------------------------------"

# JUCE modules (only compile if not already compiled)
JUCE_MODULES="juce_core juce_audio_basics juce_audio_formats juce_audio_processors \
              juce_dsp juce_events juce_data_structures juce_graphics juce_gui_basics juce_gui_extra"

for module in $JUCE_MODULES; do
    OBJ_FILE="$OBJ_DIR/${module}.o"
    SRC_FILE="$JUCE_DIR/modules/${module}/${module}.cpp"

    if [ ! -f "$OBJ_FILE" ]; then
        echo "  → Compiling JUCE module: $module"
        $CXX $OBJCPP_FLAGS $INCLUDES $DEFINES -c "$SRC_FILE" -o "$OBJ_FILE"
    else
        echo "  ✓ Using cached: $module"
    fi
done

# juce_core_CompilationTime
OBJ_FILE="$OBJ_DIR/juce_core_CompilationTime.o"
if [ ! -f "$OBJ_FILE" ]; then
    echo "  → Compiling juce_core_CompilationTime"
    $CXX $OBJCPP_FLAGS $INCLUDES $DEFINES -c \
        "$JUCE_DIR/modules/juce_core/juce_core_CompilationTime.cpp" -o "$OBJ_FILE"
else
    echo "  ✓ Using cached: juce_core_CompilationTime"
fi

# SheenBidi (C library)
OBJ_FILE="$OBJ_DIR/SheenBidi.o"
if [ ! -f "$OBJ_FILE" ]; then
    echo "  → Compiling SheenBidi"
    $CC -std=c11 -O2 $INCLUDES -DSB_CONFIG_UNITY -c \
        "$JUCE_DIR/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c" -o "$OBJ_FILE"
else
    echo "  ✓ Using cached: SheenBidi"
fi

echo ""
echo "Step 2: Compiling engine sources..."
echo "------------------------------------------------------------"

# Compile engine sources
ENGINE_OBJS=""
while IFS= read -r engine_src; do
    if [ -n "$engine_src" ]; then
        ENGINE_NAME=$(basename "$engine_src" .cpp)
        OBJ_FILE="$OBJ_DIR/${ENGINE_NAME}.o"
        SRC_FILE="$PLUGIN_SRC/$engine_src"

        if [ ! -f "$OBJ_FILE" ]; then
            echo "  → Compiling engine: $ENGINE_NAME"
            $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$SRC_FILE" -o "$OBJ_FILE"
        else
            echo "  ✓ Using cached: $ENGINE_NAME"
        fi

        ENGINE_OBJS="$ENGINE_OBJS $OBJ_FILE"
    fi
done < required_engines.txt

echo ""
echo "Step 3: Compiling latency_measurement_suite.cpp..."
echo "------------------------------------------------------------"

TEST_OBJ="$OBJ_DIR/latency_measurement_suite.o"
echo "  → Compiling latency_measurement_suite.cpp"
$CXX $CPP_FLAGS $INCLUDES $DEFINES -c latency_measurement_suite.cpp -o "$TEST_OBJ"

echo ""
echo "Step 4: Linking latency_measurement_suite executable..."
echo "------------------------------------------------------------"

# Collect all object files
ALL_OBJS="$TEST_OBJ"
for module in $JUCE_MODULES; do
    ALL_OBJS="$ALL_OBJS $OBJ_DIR/${module}.o"
done
ALL_OBJS="$ALL_OBJS $OBJ_DIR/juce_core_CompilationTime.o"
ALL_OBJS="$ALL_OBJS $OBJ_DIR/SheenBidi.o"
ALL_OBJS="$ALL_OBJS $ENGINE_OBJS"

OUTPUT_BIN="$BUILD_DIR/latency_measurement_suite"
echo "  → Linking $OUTPUT_BIN"
$CXX $CPP_FLAGS $ALL_OBJS $FRAMEWORKS -L/opt/homebrew/lib -lharfbuzz -o "$OUTPUT_BIN"

echo ""
echo "╔════════════════════════════════════════════════════════════════════════════╗"
echo "║                          BUILD SUCCESSFUL                                  ║"
echo "╚════════════════════════════════════════════════════════════════════════════╝"
echo ""
echo "Executable: $OUTPUT_BIN"
echo ""
echo "To run the test:"
echo "  cd $(pwd)"
echo "  $OUTPUT_BIN"
echo ""
echo "This will:"
echo "  - Test all pitch shifters (engines 31, 32, 33, 49)"
echo "  - Test all reverbs (engines 39, 40, 41, 42, 43)"
echo "  - Test all delays (engines 34, 35, 36, 37, 38)"
echo "  - Measure latency from impulse to first output"
echo "  - Generate detailed report with samples and milliseconds"
echo "  - Save CSV file: latency_report.csv"
echo ""

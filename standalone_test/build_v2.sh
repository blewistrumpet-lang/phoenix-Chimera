#!/bin/bash
# Build script for standalone engine tester - Version 2
# Separate compilation for C++ and Objective-C++ files

set -e  # Exit on error

echo "Building ChimeraPhoenix Standalone Test Suite..."
echo "=================================================="

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

# Check JUCE exists
if [ ! -d "$JUCE_DIR/modules" ]; then
    echo "ERROR: JUCE not found at $JUCE_DIR"
    exit 1
fi

# Create build directories
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Common flags
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1 -DJUCE_BUILD_INFO_SUFFIX=\"\""

# Compiler flags for C++
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"

# Compiler flags for Objective-C++ (JUCE modules)
OBJCPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

echo ""
echo "Step 1: Compiling JUCE modules (Objective-C++)..."
JUCE_OBJS=""

for module in juce_core juce_audio_basics juce_audio_formats juce_audio_processors juce_dsp juce_events juce_data_structures juce_graphics juce_gui_basics juce_gui_extra; do
    echo "  - $module..."
    clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES \
        -c "$JUCE_DIR/modules/$module/$module.cpp" \
        -o "$OBJ_DIR/$module.o"
    JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/$module.o"
done

echo "  - juce_core_CompilationTime (build metadata)..."
clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES \
    -c "$JUCE_DIR/modules/juce_core/juce_core_CompilationTime.cpp" \
    -o "$OBJ_DIR/juce_core_CompilationTime.o"
JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/juce_core_CompilationTime.o"

echo ""
echo "Step 2: Compiling SheenBidi library (C)..."
clang -std=c11 -O2 $INCLUDES -DSB_CONFIG_UNITY \
    -c "$JUCE_DIR/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c" \
    -o "$OBJ_DIR/SheenBidi.o"
SHEENBIDI_OBJ="$OBJ_DIR/SheenBidi.o"

echo ""
echo "Step 3: Compiling engine sources (C++)..."
ENGINE_OBJS=""
while IFS= read -r filename; do
    filepath="$PLUGIN_SRC/$filename"
    if [ -f "$filepath" ]; then
        basename=$(basename "$filename" .cpp)
        echo "  - $basename..."
        clang++ $CPP_FLAGS $INCLUDES $DEFINES \
            -c "$filepath" \
            -o "$OBJ_DIR/$basename.o"
        ENGINE_OBJS="$ENGINE_OBJS $OBJ_DIR/$basename.o"
    fi
done < required_engines.txt

echo ""
echo "Step 4: Compiling test harness (C++)..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c standalone_test.cpp \
    -o "$OBJ_DIR/standalone_test.o"

echo ""
echo "Step 5: Linking..."
# Collect all unique object files
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" | sort)

# Link with homebrew harfbuzz and SheenBidi
clang++ $CPP_FLAGS \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/standalone_test"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Run tests with:"
    echo "  cd $BUILD_DIR && ./standalone_test"
    echo "  cd $BUILD_DIR && ./standalone_test --verbose"
    echo "  cd $BUILD_DIR && ./standalone_test --engine 1"
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

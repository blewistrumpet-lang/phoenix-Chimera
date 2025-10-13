#!/bin/bash
# Build script for Bug #5 verification test

set -e

echo "Building Bug #5 verification test..."

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"

mkdir -p "$BUILD_DIR"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
OBJCXXFLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

# Includes
INCLUDES="-I. -I${PLUGIN_SRC} -I${PLUGIN_SRC}/../JuceLibraryCode -I${JUCE_DIR}/modules"

# Defines
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DDEBUG=1"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation \
            -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security \
            -framework QuartzCore -framework CoreImage -framework CoreGraphics \
            -framework CoreText -framework WebKit -framework DiscRecording"

echo "Compiling JUCE modules..."

# Compile JUCE modules (if not already compiled)
if [ ! -f "$BUILD_DIR/juce_core.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_core/juce_core.cpp -o $BUILD_DIR/juce_core.o
fi

if [ ! -f "$BUILD_DIR/juce_audio_basics.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_audio_basics/juce_audio_basics.cpp -o $BUILD_DIR/juce_audio_basics.o
fi

if [ ! -f "$BUILD_DIR/juce_audio_formats.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_audio_formats/juce_audio_formats.cpp -o $BUILD_DIR/juce_audio_formats.o
fi

if [ ! -f "$BUILD_DIR/juce_audio_processors.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_audio_processors/juce_audio_processors.cpp -o $BUILD_DIR/juce_audio_processors.o
fi

if [ ! -f "$BUILD_DIR/juce_dsp.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_dsp/juce_dsp.cpp -o $BUILD_DIR/juce_dsp.o
fi

if [ ! -f "$BUILD_DIR/juce_events.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_events/juce_events.cpp -o $BUILD_DIR/juce_events.o
fi

if [ ! -f "$BUILD_DIR/juce_data_structures.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_data_structures/juce_data_structures.cpp -o $BUILD_DIR/juce_data_structures.o
fi

if [ ! -f "$BUILD_DIR/juce_graphics.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_graphics/juce_graphics.cpp -o $BUILD_DIR/juce_graphics.o
fi

if [ ! -f "$BUILD_DIR/juce_gui_basics.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_gui_basics/juce_gui_basics.cpp -o $BUILD_DIR/juce_gui_basics.o
fi

if [ ! -f "$BUILD_DIR/juce_gui_extra.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_gui_extra/juce_gui_extra.cpp -o $BUILD_DIR/juce_gui_extra.o
fi

# Compile SheenBidi (C library)
if [ ! -f "$BUILD_DIR/SheenBidi.o" ]; then
    clang -std=c11 -O2 $INCLUDES -DSB_CONFIG_UNITY -c ${JUCE_DIR}/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c -o $BUILD_DIR/SheenBidi.o
fi

# Compile juce_core_CompilationTime
if [ ! -f "$BUILD_DIR/juce_core_CompilationTime.o" ]; then
    $CXX $OBJCXXFLAGS $INCLUDES $DEFINES -c ${JUCE_DIR}/modules/juce_core/juce_core_CompilationTime.cpp -o $BUILD_DIR/juce_core_CompilationTime.o
fi

echo "Compiling engine sources..."

# Compile IntelligentHarmonizer
$CXX $CXXFLAGS $INCLUDES $DEFINES -c ${PLUGIN_SRC}/IntelligentHarmonizer.cpp -o $BUILD_DIR/IntelligentHarmonizer.o

# Compile SMBPitchShiftFixed
$CXX $CXXFLAGS $INCLUDES $DEFINES -c ${PLUGIN_SRC}/SMBPitchShiftFixed.cpp -o $BUILD_DIR/SMBPitchShiftFixed.o

echo "Compiling test program..."
$CXX $CXXFLAGS $INCLUDES $DEFINES -c test_harmonizer_bug5.cpp -o $BUILD_DIR/test_harmonizer_bug5.o

echo "Linking..."
$CXX $CXXFLAGS \
    $BUILD_DIR/test_harmonizer_bug5.o \
    $BUILD_DIR/IntelligentHarmonizer.o \
    $BUILD_DIR/SMBPitchShiftFixed.o \
    $BUILD_DIR/juce_core.o \
    $BUILD_DIR/juce_core_CompilationTime.o \
    $BUILD_DIR/juce_audio_basics.o \
    $BUILD_DIR/juce_audio_formats.o \
    $BUILD_DIR/juce_audio_processors.o \
    $BUILD_DIR/juce_dsp.o \
    $BUILD_DIR/juce_events.o \
    $BUILD_DIR/juce_data_structures.o \
    $BUILD_DIR/juce_graphics.o \
    $BUILD_DIR/juce_gui_basics.o \
    $BUILD_DIR/juce_gui_extra.o \
    $BUILD_DIR/SheenBidi.o \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o $BUILD_DIR/test_harmonizer_bug5

echo "Build complete! Executable: $BUILD_DIR/test_harmonizer_bug5"

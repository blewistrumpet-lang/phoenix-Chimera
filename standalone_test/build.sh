#!/bin/bash
# Build script for standalone engine tester

set -e  # Exit on error

echo "Building ChimeraPhoenix Standalone Test Suite..."
echo "=================================================="

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"

# Check JUCE exists
if [ ! -d "$JUCE_DIR/modules" ]; then
    echo "ERROR: JUCE not found at $JUCE_DIR"
    exit 1
fi

# Create build directory
mkdir -p "$BUILD_DIR"

# Compiler flags - add Objective-C++ support for JUCE on macOS
CXX_FLAGS="-std=c++17 -O3 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -x objective-c++"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules -I/opt/homebrew/include/harfbuzz"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
LIBS="-L/opt/homebrew/lib -lharfbuzz"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"

# Source files to compile - EXACT list from EngineFactory.cpp includes
# This ensures we only compile what's actually used by the factory
ENGINE_SOURCES=""
while IFS= read -r filename; do
    filepath="$PLUGIN_SRC/$filename"
    if [ -f "$filepath" ]; then
        ENGINE_SOURCES="$ENGINE_SOURCES $filepath"
    else
        echo "Warning: $filepath not found"
    fi
done < required_engines.txt

echo ""
echo "Compiling standalone test..."

# JUCE modules need to be compiled and linked
# Need audio_processors for EngineBase/AudioProcessor base class
JUCE_MODULES="$JUCE_DIR/modules/juce_core/juce_core.cpp \
    $JUCE_DIR/modules/juce_audio_basics/juce_audio_basics.cpp \
    $JUCE_DIR/modules/juce_audio_processors/juce_audio_processors.cpp \
    $JUCE_DIR/modules/juce_dsp/juce_dsp.cpp \
    $JUCE_DIR/modules/juce_events/juce_events.cpp \
    $JUCE_DIR/modules/juce_data_structures/juce_data_structures.cpp \
    $JUCE_DIR/modules/juce_graphics/juce_graphics.cpp \
    $JUCE_DIR/modules/juce_gui_basics/juce_gui_basics.cpp"

# Use clang++ on macOS for Objective-C++ support in JUCE
clang++ $CXX_FLAGS \
    standalone_test.cpp \
    $ENGINE_SOURCES \
    $JUCE_MODULES \
    $INCLUDES \
    $FRAMEWORKS \
    $LIBS \
    $DEFINES \
    -o $BUILD_DIR/standalone_test

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

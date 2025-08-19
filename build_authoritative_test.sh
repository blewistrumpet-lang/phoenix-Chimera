#!/bin/bash

# Build script for authoritative engine test
# Adapted from build_real_test.sh but using local JUCE

echo "================================================"
echo "   Building Authoritative Engine Test"
echo "================================================"

# Configuration
JUCE_PATH="./JUCE"
SOURCE_DIR="JUCE_Plugin/Source"
OUTPUT="authoritative_engine_test"

# Check if JUCE exists
if [ ! -d "$JUCE_PATH" ]; then
    echo "Error: JUCE not found at $JUCE_PATH"
    exit 1
fi

echo "Using JUCE from: $JUCE_PATH"
echo "Compiling authoritative engine test..."

# Get all engine source files
ENGINE_SOURCES=$(find "$SOURCE_DIR" -name "*.cpp" -not -name "Plugin*" -not -name "*Test*" -not -name "*test*" | tr '\n' ' ')

# Compile command with all necessary engine source files
g++ -std=c++17 -O2 \
    -I"$JUCE_PATH/modules" \
    -I"$SOURCE_DIR" \
    -I"JUCE_Plugin/JuceLibraryCode" \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_USE_CURL=0 \
    -DJUCE_WEB_BROWSER=0 \
    -framework CoreFoundation \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioToolbox \
    -framework Accelerate \
    -framework AudioUnit \
    -framework Carbon \
    -framework Cocoa \
    -framework IOKit \
    -framework QuartzCore \
    authoritative_engine_test_simple.cpp \
    $ENGINE_SOURCES \
    -o $OUTPUT 2>&1

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo "Run with: ./$OUTPUT"
else
    echo "❌ Build failed. Check error messages above."
    exit 1
fi
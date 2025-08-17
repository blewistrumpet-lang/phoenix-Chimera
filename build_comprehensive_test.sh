#!/bin/bash

# Build script for comprehensive engine test suite
# This compiles against actual JUCE and engine implementations

echo "================================================"
echo "   Building Comprehensive Engine Test Suite"
echo "================================================"

# Configuration
JUCE_PATH="/Users/Branden/JUCE"
SOURCE_DIR="JUCE_Plugin/Source"
OUTPUT="comprehensive_engine_test"

# Check if JUCE exists
if [ ! -d "$JUCE_PATH" ]; then
    echo "Error: JUCE not found at $JUCE_PATH"
    exit 1
fi

echo "Using JUCE from: $JUCE_PATH"
echo "Compiling comprehensive engine test suite..."

# Get all engine source files
ENGINE_SOURCES=""
for engine_file in $SOURCE_DIR/*.cpp; do
    # Skip certain files that might cause issues
    if [[ "$engine_file" != *"Test"* ]] && [[ "$engine_file" != *"Plugin"* ]] && [[ "$engine_file" != *"Editor"* ]]; then
        ENGINE_SOURCES="$ENGINE_SOURCES $engine_file"
    fi
done

echo "Found engine sources: $(echo $ENGINE_SOURCES | wc -w) files"

# Compile command with all necessary engine source files
g++ -std=c++17 -O2 \
    -I"$JUCE_PATH/modules" \
    -I"$SOURCE_DIR" \
    -I"JUCE_Plugin/JuceLibraryCode" \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_USE_CURL=0 \
    -DJUCE_WEB_BROWSER=0 \
    -DJUCE_USE_CAMERA=disabled \
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
    comprehensive_engine_test.cpp \
    $ENGINE_SOURCES \
    -o $OUTPUT 2>&1

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo "Run with: ./$OUTPUT"
    echo ""
    echo "Running comprehensive engine tests..."
    echo "===================================="
    ./$OUTPUT
else
    echo "❌ Build failed. Check error messages above."
    exit 1
fi
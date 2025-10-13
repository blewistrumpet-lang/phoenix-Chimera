#!/bin/bash

# Build script for Audio Quality Analysis Suite

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║    Building Audio Quality Analysis Suite                     ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

JUCE_PATH="/Users/Branden/JUCE"
SOURCE_PATH="../JUCE_Plugin/Source"
OUTPUT_DIR="build_quality_analysis"
OUTPUT_BINARY="audio_quality_analysis"

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "[1/3] Compiling test_audio_quality_analysis.cpp..."

clang++ -std=c++17 \
    -O2 \
    -I"$JUCE_PATH/modules" \
    -I"$SOURCE_PATH" \
    -I. \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_USE_CURL=0 \
    -DJUCE_WEB_BROWSER=0 \
    -DJUCE_DSP_USE_INTEL_MKL=0 \
    -DDONT_SET_USING_JUCE_NAMESPACE=1 \
    -c test_audio_quality_analysis.cpp \
    -o "$OUTPUT_DIR/test_audio_quality_analysis.o"

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

echo "✓ Compilation successful"
echo ""
echo "[2/3] Compiling JUCE modules..."

# Compile minimal JUCE stubs
clang++ -std=c++17 \
    -O2 \
    -I"$JUCE_PATH/modules" \
    -c juce_stubs.cpp \
    -o "$OUTPUT_DIR/juce_stubs.o"

if [ $? -ne 0 ]; then
    echo "❌ JUCE stubs compilation failed!"
    exit 1
fi

echo "✓ JUCE stubs compiled"
echo ""
echo "[3/3] Linking..."

clang++ \
    "$OUTPUT_DIR/test_audio_quality_analysis.o" \
    "$OUTPUT_DIR/juce_stubs.o" \
    -framework CoreFoundation \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework Accelerate \
    -o "$OUTPUT_DIR/$OUTPUT_BINARY"

if [ $? -ne 0 ]; then
    echo "❌ Linking failed!"
    exit 1
fi

echo "✓ Linking successful"
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║    Build Complete!                                            ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "Run with: ./$OUTPUT_DIR/$OUTPUT_BINARY"
echo ""

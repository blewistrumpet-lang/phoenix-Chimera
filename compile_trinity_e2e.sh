#!/bin/bash

# Compilation script for Trinity E2E Integration Test
# This script compiles test_trinity_e2e.cpp with JUCE headers

echo "========================================="
echo "Compiling Trinity E2E Integration Test"
echo "========================================="

# Check if JUCE_DIR is set
if [ -z "$JUCE_DIR" ]; then
    echo "WARNING: JUCE_DIR environment variable not set"
    echo "Attempting to use common JUCE locations..."

    # Try common JUCE locations
    if [ -d "/Applications/JUCE" ]; then
        export JUCE_DIR="/Applications/JUCE"
    elif [ -d "$HOME/JUCE" ]; then
        export JUCE_DIR="$HOME/JUCE"
    elif [ -d "/usr/local/JUCE" ]; then
        export JUCE_DIR="/usr/local/JUCE"
    else
        echo "ERROR: Could not find JUCE installation"
        echo "Please set JUCE_DIR environment variable to your JUCE installation directory"
        echo "Example: export JUCE_DIR=/path/to/JUCE"
        exit 1
    fi
fi

echo "Using JUCE at: $JUCE_DIR"

# Compile the test
echo "Compiling test_trinity_e2e.cpp..."

g++ -std=c++17 \
    -I"$JUCE_DIR/modules" \
    -I"JUCE_Plugin/Source" \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreFoundation \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioToolbox \
    -framework Accelerate \
    -framework QuartzCore \
    test_trinity_e2e.cpp \
    -o test_trinity_e2e

if [ $? -eq 0 ]; then
    echo "========================================="
    echo "Compilation successful!"
    echo "========================================="
    echo "Run the test with: ./test_trinity_e2e"
    echo ""
else
    echo "========================================="
    echo "Compilation failed!"
    echo "========================================="
    exit 1
fi

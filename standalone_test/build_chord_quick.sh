#!/bin/bash

echo "========================================"
echo "Building Quick Chord Verification"
echo "========================================"

cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Compile
g++ -std=c++17 -O2 \
    -I. \
    -I../JUCE_Plugin/Source \
    test_chord_harmonizer_quick.cpp \
    IntelligentHarmonizer_standalone.cpp \
    SMBPitchShiftFixed_standalone.cpp \
    -o test_chord_quick \
    2>&1

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo ""
    echo "========================================"
    echo "Running Quick Test..."
    echo "========================================"
    echo ""
    timeout 60s ./test_chord_quick
    EXIT_CODE=$?

    if [ $EXIT_CODE -eq 124 ]; then
        echo ""
        echo "Test timed out after 60 seconds"
        exit 1
    fi

    exit $EXIT_CODE
else
    echo ""
    echo "Build failed!"
    exit 1
fi

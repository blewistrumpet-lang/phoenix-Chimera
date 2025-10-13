#!/bin/bash

echo "========================================"
echo "Building Chord Harmonizer Verification"
echo "========================================"

cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Compile
g++ -std=c++17 -O2 \
    -I. \
    -I../JUCE_Plugin/Source \
    test_chord_harmonizer_verification.cpp \
    IntelligentHarmonizer_standalone.cpp \
    SMBPitchShiftFixed_standalone.cpp \
    -o test_chord_verification \
    2>&1

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo ""
    echo "========================================"
    echo "Running Verification Test..."
    echo "========================================"
    echo ""
    ./test_chord_verification
else
    echo ""
    echo "Build failed! Check errors above."
    exit 1
fi

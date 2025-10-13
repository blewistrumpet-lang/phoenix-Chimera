#!/bin/bash

echo "Building Gender Bender Verification Test..."
echo "==========================================="

cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O2 -I../JUCE_Plugin/Source -I."
LDFLAGS="-framework Cocoa -framework CoreAudio -framework CoreMIDI -framework IOKit -framework Accelerate -framework QuartzCore"

# Define JUCE symbols
JUCE_DEFS="-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0"

# Source files
TEST_SRC="test_gender_bender_verification.cpp"

# Pitch shifting sources
PITCH_SHIFTER="../JUCE_Plugin/Source/PitchShifter.cpp"
PITCH_FACTORY="../JUCE_Plugin/Source/PitchShiftFactory.cpp"
PHASE_VOCODER="../JUCE_Plugin/Source/PhaseVocoderPitchShift.cpp"
SMB_PITCH="../JUCE_Plugin/Source/SMBPitchShiftFixed.cpp"

# Reuse compiled JUCE object files if they exist
if [ -f "juce_core.o" ] && [ -f "juce_audio_basics.o" ] && [ -f "juce_events.o" ]; then
    echo "Using existing JUCE object files..."
    JUCE_OBJS="juce_core.o juce_audio_basics.o juce_events.o"
else
    echo "Error: JUCE object files not found!"
    echo "Please run a test that builds JUCE first, or compile JUCE manually."
    exit 1
fi

# Output
OUTPUT="test_gender_bender_verification"

echo "Compiling test..."

$CXX $CXXFLAGS $JUCE_DEFS \
    $TEST_SRC \
    $PITCH_SHIFTER \
    $PITCH_FACTORY \
    $PHASE_VOCODER \
    $SMB_PITCH \
    $JUCE_OBJS \
    $LDFLAGS \
    -o $OUTPUT 2>&1 | head -50

if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo ""
    echo "Running test..."
    echo ""
    ./$OUTPUT

    if [ -f "GENDER_BENDER_VERIFICATION_REPORT.md" ]; then
        echo ""
        echo "========================================="
        echo "Report generated successfully!"
        echo "========================================="
    fi
else
    echo "Build failed!"
    exit 1
fi

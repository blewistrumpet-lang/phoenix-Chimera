#!/bin/bash

# Build script for real engine test harness
# This compiles against actual JUCE and engine implementations

echo "================================================"
echo "   Building Real Engine Test Harness"
echo "================================================"

# Configuration
JUCE_PATH="/Users/Branden/JUCE"
SOURCE_DIR="JUCE_Plugin/Source"
OUTPUT="real_engine_test"

# Check if JUCE exists
if [ ! -d "$JUCE_PATH" ]; then
    echo "Error: JUCE not found at $JUCE_PATH"
    exit 1
fi

echo "Using JUCE from: $JUCE_PATH"
echo "Compiling engine test harness..."

# Compile command with all necessary engine source files
g++ -std=c++17 -O2 \
    -I"$JUCE_PATH/modules" \
    -I"$SOURCE_DIR" \
    -I"JUCE_Plugin/JuceLibraryCode" \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_USE_CURL=0 \
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
    -framework WebKit \
    real_engine_integration_test.cpp \
    $SOURCE_DIR/EngineFactory.cpp \
    $SOURCE_DIR/DefaultParameterValues.cpp \
    $SOURCE_DIR/EngineMetadataInit.cpp \
    $SOURCE_DIR/CompleteEngineMetadata.cpp \
    $SOURCE_DIR/TestSignalGenerator.cpp \
    $SOURCE_DIR/AudioMeasurements.cpp \
    $SOURCE_DIR/TapeEcho.cpp \
    $SOURCE_DIR/PlateReverb.cpp \
    $SOURCE_DIR/VintageOptoCompressor.cpp \
    $SOURCE_DIR/ClassicCompressor.cpp \
    $SOURCE_DIR/TransientShaper.cpp \
    $SOURCE_DIR/NoiseGate.cpp \
    $SOURCE_DIR/MasteringLimiter.cpp \
    $SOURCE_DIR/DynamicEQ.cpp \
    $SOURCE_DIR/ParametricEQ.cpp \
    $SOURCE_DIR/VintageConsoleEQ.cpp \
    $SOURCE_DIR/LadderFilter.cpp \
    $SOURCE_DIR/StateVariableFilter.cpp \
    $SOURCE_DIR/FormantFilter.cpp \
    $SOURCE_DIR/EnvelopeFilter.cpp \
    $SOURCE_DIR/CombResonator.cpp \
    $SOURCE_DIR/VocalFormantFilter.cpp \
    $SOURCE_DIR/VintageTubePreamp.cpp \
    $SOURCE_DIR/WaveFolder.cpp \
    $SOURCE_DIR/HarmonicExciter.cpp \
    $SOURCE_DIR/BitCrusher.cpp \
    $SOURCE_DIR/MultibandSaturator.cpp \
    $SOURCE_DIR/MuffFuzz.cpp \
    $SOURCE_DIR/RodentDistortion.cpp \
    $SOURCE_DIR/KStyleOverdrive.cpp \
    $SOURCE_DIR/StereoChorus.cpp \
    $SOURCE_DIR/ResonantChorus.cpp \
    $SOURCE_DIR/AnalogPhaser.cpp \
    $SOURCE_DIR/RingModulator.cpp \
    $SOURCE_DIR/FrequencyShifter.cpp \
    $SOURCE_DIR/HarmonicTremolo.cpp \
    $SOURCE_DIR/ClassicTremolo.cpp \
    $SOURCE_DIR/RotarySpeaker.cpp \
    $SOURCE_DIR/PitchShifter.cpp \
    $SOURCE_DIR/DetuneDoubler.cpp \
    $SOURCE_DIR/IntelligentHarmonizer.cpp \
    $SOURCE_DIR/DigitalDelay.cpp \
    $SOURCE_DIR/MagneticDrumEcho.cpp \
    $SOURCE_DIR/BucketBrigadeDelay.cpp \
    $SOURCE_DIR/BufferRepeat.cpp \
    $SOURCE_DIR/SpringReverb.cpp \
    $SOURCE_DIR/ConvolutionReverb.cpp \
    $SOURCE_DIR/ShimmerReverb.cpp \
    $SOURCE_DIR/GatedReverb.cpp \
    $SOURCE_DIR/StereoWidener.cpp \
    $SOURCE_DIR/StereoImager.cpp \
    $SOURCE_DIR/DimensionExpander.cpp \
    $SOURCE_DIR/SpectralFreeze.cpp \
    $SOURCE_DIR/SpectralGate.cpp \
    $SOURCE_DIR/PhasedVocoder.cpp \
    $SOURCE_DIR/GranularCloud.cpp \
    $SOURCE_DIR/ChaosGenerator.cpp \
    $SOURCE_DIR/FeedbackNetwork.cpp \
    $SOURCE_DIR/MidSideProcessor.cpp \
    $SOURCE_DIR/GainUtility.cpp \
    $SOURCE_DIR/MonoMaker.cpp \
    $SOURCE_DIR/PhaseAlign.cpp \
    -o $OUTPUT 2>&1

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo "Run with: ./$OUTPUT"
else
    echo "❌ Build failed. Check error messages above."
    exit 1
fi
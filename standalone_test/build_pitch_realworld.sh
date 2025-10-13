#!/bin/bash

# Build script for real-world pitch engine testing

echo "Building Pitch Engine Real-World Test..."

# Configuration
CXX="clang++"
JUCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE"
PLUGIN_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source"
OUTPUT="test_pitch_realworld"

# Compiler flags
CXXFLAGS="-std=c++17 -O2 -DJUCE_STANDALONE_APPLICATION=1"

# JUCE includes
INCLUDES="-I${JUCE_DIR}/modules"
INCLUDES="${INCLUDES} -I${PLUGIN_DIR}"

# Required JUCE modules
JUCE_SOURCES="${JUCE_DIR}/modules/juce_core/juce_core.mm"
JUCE_SOURCES="${JUCE_SOURCES} ${JUCE_DIR}/modules/juce_audio_basics/juce_audio_basics.mm"
JUCE_SOURCES="${JUCE_SOURCES} ${JUCE_DIR}/modules/juce_audio_formats/juce_audio_formats.mm"
JUCE_SOURCES="${JUCE_SOURCES} ${JUCE_DIR}/modules/juce_dsp/juce_dsp.mm"

# Engine sources (all pitch-related engines)
ENGINE_SOURCES="${PLUGIN_DIR}/PitchShifter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/IntelligentHarmonizer.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PhasedVocoder.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/SMBPitchShiftFixed.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PitchShiftFactory.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/EngineFactory.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/EngineBase.cpp"

# Add all other required engines (for EngineFactory)
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/NoneEngine.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/VintageOptoCompressor_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ClassicCompressor.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/TransientShaper_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/NoiseGate_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/MasteringLimiter_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/DynamicEQ.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ParametricEQ_Studio.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/VintageConsoleEQ_Studio.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/LadderFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/StateVariableFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/FormantFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/EnvelopeFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/CombResonator.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/VocalFormantFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/VintageTubePreamp_Studio.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/WaveFolder.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/HarmonicExciter_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/BitCrusher.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/MultibandSaturator.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/MuffFuzz.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/RodentDistortion.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/KStyleOverdrive.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/StereoChorus.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ResonantChorus_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/AnalogPhaser.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PlatinumRingModulator.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/FrequencyShifter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/HarmonicTremolo.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ClassicTremolo.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/RotarySpeaker_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/DetuneDoubler.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/TapeEcho.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/DigitalDelay.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/MagneticDrumEcho.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/BucketBrigadeDelay.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/BufferRepeat_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PlateReverb.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/SpringReverb.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ConvolutionReverb.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ShimmerReverb.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/GatedReverb.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/StereoWidener.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/StereoImager.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/DimensionExpander.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/SpectralFreeze.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/SpectralGate_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/GranularCloud.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ChaosGenerator.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/FeedbackNetwork.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/MidSideProcessor_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/GainUtility_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/MonoMaker_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PhaseAlign_Platinum.cpp"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox -framework Foundation -framework AppKit"

# Build
echo "Compiling..."
${CXX} ${CXXFLAGS} ${INCLUDES} \
    test_pitch_realworld.cpp \
    ${JUCE_SOURCES} \
    ${ENGINE_SOURCES} \
    ${FRAMEWORKS} \
    -o ${OUTPUT}

if [ $? -eq 0 ]; then
    echo "✓ Build successful: ${OUTPUT}"
    echo ""
    echo "Running test..."
    ./${OUTPUT}
else
    echo "✗ Build failed"
    exit 1
fi

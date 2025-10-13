#!/bin/bash

# Build script for real-world modulation engine testing

echo "Building Modulation Engine Real-World Test..."

# Configuration
CXX="clang++"
JUCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE"
PLUGIN_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source"
OUTPUT="test_modulation_realworld"

# Compiler flags
CXXFLAGS="-std=c++17 -O2 -DJUCE_STANDALONE_APPLICATION=1 -Wno-deprecated-declarations"

# JUCE includes
INCLUDES="-I${JUCE_DIR}/modules"
INCLUDES="${INCLUDES} -I${PLUGIN_DIR}"

# Required JUCE modules (inline compilation)
JUCE_SOURCES="${JUCE_DIR}/modules/juce_core/juce_core.mm"
JUCE_SOURCES="${JUCE_SOURCES} ${JUCE_DIR}/modules/juce_audio_basics/juce_audio_basics.mm"
JUCE_SOURCES="${JUCE_SOURCES} ${JUCE_DIR}/modules/juce_audio_formats/juce_audio_formats.mm"
JUCE_SOURCES="${JUCE_SOURCES} ${JUCE_DIR}/modules/juce_dsp/juce_dsp.mm"

# Modulation engine sources (engines 23-33)
ENGINE_SOURCES="${PLUGIN_DIR}/EngineFactory.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/EngineBase.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/NoneEngine.cpp"

# Modulation engines 23-33
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/StereoChorus.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ClassicFlanger.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/AnalogPhaser.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ClassicTremolo.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/FrequencyShifter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/RingModulator_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/BucketBrigadeChorus.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/DetuneDoubler.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/SimplePitchShift.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/SMBPitchShiftFixed.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/IntelligentHarmonizer.cpp"

# Add other engines needed by EngineFactory
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/VintageOptoCompressor_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ClassicCompressor.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/TransientShaper_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/NoiseGate_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/MasteringLimiter_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/DynamicEQ.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ParametricEQ.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/LadderFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/StateVariableFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/CombFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/VocalFormantFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/GraphicEQ.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PeakingFilter.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/Muff_Fuzz.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/Screamer_Overdrive.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/DS_Distortion.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/Metal_Distortion.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/Rodent_Distortion.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/Tube_Amp.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/Bit_Crusher.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/Waveshaper_Clipper.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PhasedVocoder.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PingPongDelay.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/TapeDelay.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/SpringReverb.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PlateReverb_Platinum.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ShimmerReverb.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/ConvolutionReverb.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/GatedReverb.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/StereoWidener.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/MidSideProcessor.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/HaasEffect.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/VinylSimulator.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/TapeSimulator.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/LoFiProcessor.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/GranularSynthesis.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/SpectralGate.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PhaseAlign.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_DIR}/PitchShifter.cpp"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation"
FRAMEWORKS="${FRAMEWORKS} -framework AudioToolbox -framework CoreMIDI"
FRAMEWORKS="${FRAMEWORKS} -framework IOKit -framework Cocoa"
FRAMEWORKS="${FRAMEWORKS} -framework QuartzCore -framework CoreImage"
FRAMEWORKS="${FRAMEWORKS} -framework CoreGraphics -framework CoreText"

# Compile
echo "Compiling..."
${CXX} ${CXXFLAGS} ${INCLUDES} \
    test_modulation_realworld.cpp \
    ${JUCE_SOURCES} \
    ${ENGINE_SOURCES} \
    ${FRAMEWORKS} \
    -o ${OUTPUT}

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    ls -lh ${OUTPUT}
else
    echo "✗ Build failed"
    exit 1
fi

#!/bin/bash
set -e

echo "Linking test_engines_53_55..."

cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

clang++ -std=c++17 -O2 \
    build/obj/test_engines_53_55.o \
    build/obj/EngineFactory.o \
    build/obj/MidSideProcessor_Platinum.o \
    build/obj/GainUtility_Platinum.o \
    build/obj/MonoMaker_Platinum.o \
    build/obj/AnalogPhaser.o \
    build/obj/BitCrusher.o \
    build/obj/BucketBrigadeDelay.o \
    build/obj/BufferRepeat_Platinum.o \
    build/obj/ChaosGenerator.o \
    build/obj/ClassicCompressor.o \
    build/obj/ClassicTremolo.o \
    build/obj/CombResonator.o \
    build/obj/ConvolutionReverb.o \
    build/obj/DetuneDoubler.o \
    build/obj/DigitalDelay.o \
    build/obj/DimensionExpander.o \
    build/obj/DynamicEQ.o \
    build/obj/EnvelopeFilter.o \
    build/obj/FeedbackNetwork.o \
    build/obj/FormantFilter.o \
    build/obj/FrequencyShifter.o \
    build/obj/GatedReverb.o \
    build/obj/GranularCloud.o \
    build/obj/HarmonicExciter_Platinum.o \
    build/obj/HarmonicTremolo.o \
    build/obj/IntelligentHarmonizer_FINAL.o \
    build/obj/KStyleOverdrive.o \
    build/obj/LadderFilter.o \
    build/obj/MagneticDrumEcho.o \
    build/obj/MasteringLimiter_Platinum.o \
    build/obj/MuffFuzz.o \
    build/obj/MultibandSaturator.o \
    build/obj/NoiseGate_Platinum.o \
    build/obj/ParametricEQ_Studio.o \
    build/obj/PhaseAlign_Platinum.o \
    build/obj/PhasedVocoder.o \
    build/obj/PitchShifter.o \
    build/obj/PitchShiftFactory.o \
    build/obj/PlateReverb.o \
    build/obj/PlatinumRingModulator.o \
    build/obj/ResonantChorus_Platinum.o \
    build/obj/RodentDistortion.o \
    build/obj/RotarySpeaker_Platinum.o \
    build/obj/ShimmerReverb.o \
    build/obj/SMBPitchShiftFixed.o \
    build/obj/SpectralFreeze.o \
    build/obj/SpectralGate_Platinum.o \
    build/obj/SpringReverb.o \
    build/obj/StateVariableFilter.o \
    build/obj/StereoChorus.o \
    build/obj/StereoImager.o \
    build/obj/StereoWidener.o \
    build/obj/TapeEcho.o \
    build/obj/TransientShaper_Platinum.o \
    build/obj/UnifiedDefaultParameters.o \
    build/obj/VintageConsoleEQ_Studio.o \
    build/obj/VintageOptoCompressor_Platinum.o \
    build/obj/VintageTubePreamp_Studio.o \
    build/obj/VocalFormantFilter.o \
    build/obj/WaveFolder.o \
    build/obj/juce_core.o \
    build/obj/juce_core_CompilationTime.o \
    build/obj/juce_audio_basics.o \
    build/obj/juce_audio_formats.o \
    build/obj/juce_audio_processors.o \
    build/obj/juce_dsp.o \
    build/obj/juce_events.o \
    build/obj/juce_data_structures.o \
    build/obj/juce_graphics.o \
    build/obj/juce_gui_basics.o \
    build/obj/juce_gui_extra.o \
    build/obj/SheenBidi.o \
    -framework Accelerate \
    -framework CoreAudio \
    -framework CoreFoundation \
    -framework AudioToolbox \
    -framework Cocoa \
    -framework IOKit \
    -framework Security \
    -framework QuartzCore \
    -framework CoreImage \
    -framework CoreGraphics \
    -framework CoreText \
    -framework WebKit \
    -framework DiscRecording \
    -L/opt/homebrew/lib -lharfbuzz \
    -o build/test_engines_53_55

echo "Link successful!"
echo "Executable: build/test_engines_53_55"

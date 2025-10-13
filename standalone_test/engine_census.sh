#!/bin/bash

# ENGINE CENSUS MISSION - Check all 57 engines for file existence and status

SOURCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source"
OUTPUT_FILE="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/engine_census_data.csv"

echo "ID,Engine_Name,Header_Exists,CPP_Exists,Header_Size,CPP_Size,Header_Lines,CPP_Lines,Header_ModDate,CPP_ModDate" > "$OUTPUT_FILE"

# Engine 0
check_engine() {
    ID=$1
    NAME=$2
    HEADER=$3
    CPP=$4

    HEADER_PATH="$SOURCE_DIR/$HEADER"
    CPP_PATH="$SOURCE_DIR/$CPP"

    if [ -f "$HEADER_PATH" ]; then
        H_EXISTS="YES"
        H_SIZE=$(wc -c < "$HEADER_PATH" 2>/dev/null || echo "0")
        H_LINES=$(wc -l < "$HEADER_PATH" 2>/dev/null || echo "0")
        H_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M:%S" "$HEADER_PATH" 2>/dev/null || echo "UNKNOWN")
    else
        H_EXISTS="NO"
        H_SIZE="0"
        H_LINES="0"
        H_DATE="N/A"
    fi

    if [ -f "$CPP_PATH" ]; then
        C_EXISTS="YES"
        C_SIZE=$(wc -c < "$CPP_PATH" 2>/dev/null || echo "0")
        C_LINES=$(wc -l < "$CPP_PATH" 2>/dev/null || echo "0")
        C_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M:%S" "$CPP_PATH" 2>/dev/null || echo "UNKNOWN")
    else
        C_EXISTS="NO"
        C_SIZE="0"
        C_LINES="0"
        C_DATE="N/A"
    fi

    echo "$ID,$NAME,$H_EXISTS,$C_EXISTS,$H_SIZE,$C_SIZE,$H_LINES,$C_LINES,$H_DATE,$C_DATE" >> "$OUTPUT_FILE"
}

# All 57 engines (0-56)
check_engine 0 "None" "NoneEngine.h" "NoneEngine.cpp"
check_engine 1 "VintageOptoCompressor" "VintageOptoCompressor_Platinum.h" "VintageOptoCompressor_Platinum.cpp"
check_engine 2 "ClassicCompressor" "ClassicCompressor.h" "ClassicCompressor.cpp"
check_engine 3 "TransientShaper" "TransientShaper_Platinum.h" "TransientShaper_Platinum.cpp"
check_engine 4 "NoiseGate" "NoiseGate_Platinum.h" "NoiseGate_Platinum.cpp"
check_engine 5 "MasteringLimiter" "MasteringLimiter_Platinum.h" "MasteringLimiter_Platinum.cpp"
check_engine 6 "DynamicEQ" "DynamicEQ.h" "DynamicEQ.cpp"
check_engine 7 "ParametricEQ" "ParametricEQ_Studio.h" "ParametricEQ_Studio.cpp"
check_engine 8 "VintageConsoleEQ" "VintageConsoleEQ_Studio.h" "VintageConsoleEQ_Studio.cpp"
check_engine 9 "LadderFilter" "LadderFilter.h" "LadderFilter.cpp"
check_engine 10 "StateVariableFilter" "StateVariableFilter.h" "StateVariableFilter.cpp"
check_engine 11 "FormantFilter" "FormantFilter.h" "FormantFilter.cpp"
check_engine 12 "EnvelopeFilter" "EnvelopeFilter.h" "EnvelopeFilter.cpp"
check_engine 13 "CombResonator" "CombResonator.h" "CombResonator.cpp"
check_engine 14 "VocalFormantFilter" "VocalFormantFilter.h" "VocalFormantFilter.cpp"
check_engine 15 "VintageTube" "VintageTubePreamp_Studio.h" "VintageTubePreamp_Studio.cpp"
check_engine 16 "WaveFolder" "WaveFolder.h" "WaveFolder.cpp"
check_engine 17 "HarmonicExciter" "HarmonicExciter_Platinum.h" "HarmonicExciter_Platinum.cpp"
check_engine 18 "BitCrusher" "BitCrusher.h" "BitCrusher.cpp"
check_engine 19 "MultibandSaturator" "MultibandSaturator.h" "MultibandSaturator.cpp"
check_engine 20 "MuffFuzz" "MuffFuzz.h" "MuffFuzz.cpp"
check_engine 21 "RodentDistortion" "RodentDistortion.h" "RodentDistortion.cpp"
check_engine 22 "KStyleOverdrive" "KStyleOverdrive.h" "KStyleOverdrive.cpp"
check_engine 23 "StereoChorus" "StereoChorus.h" "StereoChorus.cpp"
check_engine 24 "ResonantChorus" "ResonantChorus_Platinum.h" "ResonantChorus_Platinum.cpp"
check_engine 25 "AnalogPhaser" "AnalogPhaser.h" "AnalogPhaser.cpp"
check_engine 26 "RingModulator" "PlatinumRingModulator.h" "PlatinumRingModulator.cpp"
check_engine 27 "FrequencyShifter" "FrequencyShifter.h" "FrequencyShifter.cpp"
check_engine 28 "HarmonicTremolo" "HarmonicTremolo.h" "HarmonicTremolo.cpp"
check_engine 29 "ClassicTremolo" "ClassicTremolo.h" "ClassicTremolo.cpp"
check_engine 30 "RotarySpeaker" "RotarySpeaker_Platinum.h" "RotarySpeaker_Platinum.cpp"
check_engine 31 "PitchShifter" "PitchShifter.h" "PitchShifter.cpp"
check_engine 32 "DetuneDoubler" "DetuneDoubler.h" "DetuneDoubler.cpp"
check_engine 33 "IntelligentHarmonizer" "IntelligentHarmonizer.h" "IntelligentHarmonizer.cpp"
check_engine 34 "TapeEcho" "TapeEcho.h" "TapeEcho.cpp"
check_engine 35 "DigitalDelay" "DigitalDelay.h" "DigitalDelay.cpp"
check_engine 36 "MagneticDrumEcho" "MagneticDrumEcho.h" "MagneticDrumEcho.cpp"
check_engine 37 "BucketBrigadeDelay" "BucketBrigadeDelay.h" "BucketBrigadeDelay.cpp"
check_engine 38 "BufferRepeat" "BufferRepeat_Platinum.h" "BufferRepeat_Platinum.cpp"
check_engine 39 "PlateReverb" "PlateReverb.h" "PlateReverb.cpp"
check_engine 40 "SpringReverb" "SpringReverb.h" "SpringReverb.cpp"
check_engine 41 "ConvolutionReverb" "ConvolutionReverb.h" "ConvolutionReverb.cpp"
check_engine 42 "ShimmerReverb" "ShimmerReverb.h" "ShimmerReverb.cpp"
check_engine 43 "GatedReverb" "GatedReverb.h" "GatedReverb.cpp"
check_engine 44 "StereoWidener" "StereoWidener.h" "StereoWidener.cpp"
check_engine 45 "StereoImager" "StereoImager.h" "StereoImager.cpp"
check_engine 46 "DimensionExpander" "DimensionExpander.h" "DimensionExpander.cpp"
check_engine 47 "SpectralFreeze" "SpectralFreeze.h" "SpectralFreeze.cpp"
check_engine 48 "SpectralGate" "SpectralGate_Platinum.h" "SpectralGate_Platinum.cpp"
check_engine 49 "PhasedVocoder" "PhasedVocoder.h" "PhasedVocoder.cpp"
check_engine 50 "GranularCloud" "GranularCloud.h" "GranularCloud.cpp"
check_engine 51 "ChaosGenerator" "ChaosGenerator.h" "ChaosGenerator.cpp"
check_engine 52 "FeedbackNetwork" "FeedbackNetwork.h" "FeedbackNetwork.cpp"
check_engine 53 "MidSideProcessor" "MidSideProcessor_Platinum.h" "MidSideProcessor_Platinum.cpp"
check_engine 54 "GainUtility" "GainUtility_Platinum.h" "GainUtility_Platinum.cpp"
check_engine 55 "MonoMaker" "MonoMaker_Platinum.h" "MonoMaker_Platinum.cpp"
check_engine 56 "PhaseAlign" "PhaseAlign_Platinum.h" "PhaseAlign_Platinum.cpp"

echo "Census data written to: $OUTPUT_FILE"
cat "$OUTPUT_FILE"

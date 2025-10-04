cmake_minimum_required(VERSION 3.15)
project(ChimeraPhoenix VERSION 3.0.0)

# Add JUCE as a subdirectory (you'll need to download JUCE)
# add_subdirectory(JUCE)

# For now, let's create a basic structure for manual compilation
set(CMAKE_CXX_STANDARD 17)

# Define the plugin target
add_library(ChimeraPhoenix MODULE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/EngineFactory.cpp
    Source/CompleteEngineMetadata.cpp
    Source/EngineMetadataInit.cpp
    Source/KStyleOverdrive.cpp
    Source/TapeEcho.cpp
    Source/PlateReverb.cpp
    # Add other engine source files as needed
)

# Define the test executable
add_executable(QualityTestRunner
    ../tests/unit/QualityTestRunner.cpp
    ../tests/unit/EngineQualityTest.cpp
    Source/EngineFactory.cpp
    Source/CompleteEngineMetadata.cpp
    Source/EngineMetadataInit.cpp
    # Add all engine source files
    Source/KStyleOverdrive.cpp
    Source/TapeEcho.cpp
    Source/PlateReverb.cpp
    # Add other engine source files as needed
)

# Define the comprehensive test harness executable
add_executable(ComprehensiveTestHarness
    ../tests/harness/ComprehensiveTestHarnessMain.cpp
    ../tests/harness/ComprehensiveTestHarness.cpp
    ../tests/harness/ComprehensiveTestImplementations.cpp
    ../tests/harness/ComprehensiveTestPerformance.cpp
    ../tests/harness/ComprehensiveTestReporting.cpp
    Source/EngineFactory.cpp
    Source/CompleteEngineMetadata.cpp
    Source/EngineMetadataInit.cpp
    ../tests/unit/TestSignalGenerator.cpp
    ../tests/unit/AudioMeasurements.cpp
    # Add all engine source files that exist
    Source/KStyleOverdrive.cpp
    Source/TapeEcho.cpp
    Source/PlateReverb.cpp
    Source/AnalogPhaser.cpp
    Source/AnalogRingModulator.cpp
    Source/BitCrusher.cpp
    Source/BucketBrigadeDelay.cpp
    Source/BufferRepeat.cpp
    Source/BufferRepeat_Platinum.cpp
    Source/ChaosGenerator.cpp
    Source/ChaosGenerator_Platinum.cpp
    Source/ClassicCompressor.cpp
    Source/ClassicTremolo.cpp
    Source/CombResonator.cpp
    Source/ConvolutionReverb.cpp
    Source/DetuneDoubler.cpp
    Source/DigitalDelay.cpp
    Source/DimensionExpander.cpp
    Source/DynamicEQ.cpp
    Source/EnvelopeFilter.cpp
    Source/FeedbackNetwork.cpp
    Source/FormantFilter.cpp
    Source/FrequencyShifter.cpp
    Source/GainUtility_Platinum.cpp
    Source/GatedReverb.cpp
    Source/GranularCloud.cpp
    Source/HarmonicExciter.cpp
    Source/HarmonicExciter_Platinum.cpp
    Source/HarmonicTremolo.cpp
    Source/IntelligentHarmonizer.cpp
    Source/LadderFilter.cpp
    Source/MagneticDrumEcho.cpp
    Source/MasteringLimiter_Platinum.cpp
    Source/MidSideProcessor_Platinum.cpp
    Source/MonoMaker_Platinum.cpp
    Source/MuffFuzz.cpp
    Source/MultibandSaturator.cpp
    Source/NoiseGate.cpp
    Source/NoiseGate_Platinum.cpp
    Source/ParametricEQ.cpp
    Source/ParametricEQ_Platinum.cpp
    Source/PhaseAlign_Platinum.cpp
    Source/PhasedVocoder.cpp
    Source/PitchShifter.cpp
    Source/PlatinumRingModulator.cpp
    Source/ResonantChorus.cpp
    Source/ResonantChorus_Platinum.cpp
    Source/RodentDistortion.cpp
    Source/RotarySpeaker.cpp
    Source/RotarySpeaker_Platinum.cpp
    Source/ShimmerReverb.cpp
    Source/SpectralFreeze.cpp
    Source/SpectralGate.cpp
    Source/SpectralGate_Platinum.cpp
    Source/SpringReverb.cpp
    Source/SpringReverb_Platinum.cpp
    Source/StateVariableFilter.cpp
    Source/StereoChorus.cpp
    Source/StereoImager.cpp
    Source/StereoWidener.cpp
    Source/TransientShaper_Platinum.cpp
    Source/VintageConsoleEQ.cpp
    Source/VintageConsoleEQ_Platinum.cpp
    Source/VintageOptoCompressor.cpp
    Source/VintageOptoCompressor_Platinum.cpp
    Source/VintageTubePreamp.cpp
    Source/VocalFormantFilter.cpp
    Source/WaveFolder.cpp
)

# Include directories
target_include_directories(ChimeraPhoenix PRIVATE
    Source
)

target_include_directories(QualityTestRunner PRIVATE
    Source
    ../tests/unit
    ../tests/integration
    ../tests/quality
    ../tests/validation
    ../tests/harness
)

target_include_directories(ComprehensiveTestHarness PRIVATE
    Source
    ../tests/unit
    ../tests/integration
    ../tests/quality
    ../tests/validation
    ../tests/harness
)

# Compiler settings for comprehensive test harness
target_compile_features(ComprehensiveTestHarness PRIVATE cxx_std_17)
target_compile_options(ComprehensiveTestHarness PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -O2>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
)

# Link threading library for comprehensive test harness
find_package(Threads REQUIRED)
target_link_libraries(ComprehensiveTestHarness PRIVATE Threads::Threads)

# Define the final validation test executable
add_executable(FinalValidationTest
    final_validation_test.cpp
    Source/EngineFactory.cpp
    Source/CompleteEngineMetadata.cpp
    Source/EngineMetadataInit.cpp
    # Add all engine source files that exist
    Source/KStyleOverdrive.cpp
    Source/TapeEcho.cpp
    Source/PlateReverb.cpp
    Source/AnalogPhaser.cpp
    Source/AnalogRingModulator.cpp
    Source/BitCrusher.cpp
    Source/BucketBrigadeDelay.cpp
    Source/BufferRepeat.cpp
    Source/BufferRepeat_Platinum.cpp
    Source/ChaosGenerator.cpp
    Source/ChaosGenerator_Platinum.cpp
    Source/ClassicCompressor.cpp
    Source/ClassicTremolo.cpp
    Source/CombResonator.cpp
    Source/ConvolutionReverb.cpp
    Source/DetuneDoubler.cpp
    Source/DigitalDelay.cpp
    Source/DimensionExpander.cpp
    Source/DynamicEQ.cpp
    Source/EnvelopeFilter.cpp
    Source/FeedbackNetwork.cpp
    Source/FormantFilter.cpp
    Source/FrequencyShifter.cpp
    Source/GainUtility_Platinum.cpp
    Source/GatedReverb.cpp
    Source/GranularCloud.cpp
    Source/HarmonicExciter.cpp
    Source/HarmonicExciter_Platinum.cpp
    Source/HarmonicTremolo.cpp
    Source/IntelligentHarmonizer.cpp
    Source/LadderFilter.cpp
    Source/MagneticDrumEcho.cpp
    Source/MasteringLimiter_Platinum.cpp
    Source/MidSideProcessor_Platinum.cpp
    Source/MonoMaker_Platinum.cpp
    Source/MuffFuzz.cpp
    Source/MultibandSaturator.cpp
    Source/NoiseGate.cpp
    Source/NoiseGate_Platinum.cpp
    Source/ParametricEQ.cpp
    Source/ParametricEQ_Platinum.cpp
    Source/PhaseAlign_Platinum.cpp
    Source/PhasedVocoder.cpp
    Source/PitchShifter.cpp
    Source/PlatinumRingModulator.cpp
    Source/ResonantChorus.cpp
    Source/ResonantChorus_Platinum.cpp
    Source/RodentDistortion.cpp
    Source/RotarySpeaker.cpp
    Source/RotarySpeaker_Platinum.cpp
    Source/ShimmerReverb.cpp
    Source/SpectralFreeze.cpp
    Source/SpectralGate.cpp
    Source/SpectralGate_Platinum.cpp
    Source/SpringReverb.cpp
    Source/SpringReverb_Platinum.cpp
    Source/StateVariableFilter.cpp
    Source/StereoChorus.cpp
    Source/StereoImager.cpp
    Source/StereoWidener.cpp
    Source/TransientShaper_Platinum.cpp
    Source/VintageConsoleEQ.cpp
    Source/VintageConsoleEQ_Platinum.cpp
    Source/VintageOptoCompressor.cpp
    Source/VintageOptoCompressor_Platinum.cpp
    Source/VintageTubePreamp.cpp
    Source/VocalFormantFilter.cpp
    Source/WaveFolder.cpp
)

target_include_directories(FinalValidationTest PRIVATE
    Source
)

target_compile_features(FinalValidationTest PRIVATE cxx_std_17)
target_compile_options(FinalValidationTest PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -O2>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
)

target_link_libraries(FinalValidationTest PRIVATE Threads::Threads)

# macOS specific settings for AU
if(APPLE)
    set_target_properties(ChimeraPhoenix PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "component"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.chimera.phoenix"
        MACOSX_BUNDLE_BUNDLE_NAME "Chimera Phoenix"
        MACOSX_BUNDLE_BUNDLE_VERSION "3.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "3.0.0"
    )
endif()
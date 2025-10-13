cmake_minimum_required(VERSION 3.15)
project(PitchScientificTest VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE path
set(JUCE_DIR "/Users/Branden/JUCE")
set(SOURCE_DIR "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source")

# Add JUCE
add_subdirectory(${JUCE_DIR} juce_build)

# Source files
set(ENGINE_SOURCES
    ${SOURCE_DIR}/EngineBase.cpp
    ${SOURCE_DIR}/EngineFactory.cpp
    ${SOURCE_DIR}/PitchShifter.cpp
    ${SOURCE_DIR}/DetuneDoubler.cpp
    ${SOURCE_DIR}/IntelligentHarmonizer.cpp
    ${SOURCE_DIR}/IntelligentHarmonizerChords.cpp
    ${SOURCE_DIR}/ShimmerReverb.cpp
    ${SOURCE_DIR}/PhasedVocoder.cpp
    ${SOURCE_DIR}/GranularCloud.cpp
    ${SOURCE_DIR}/PlateReverb.cpp
    ${SOURCE_DIR}/BiquadFilter.cpp
    ${SOURCE_DIR}/ModulatedDelay.cpp
    ${SOURCE_DIR}/StateVariableFilter.cpp
)

# Create executable
juce_add_console_app(test_pitch_scientific
    PRODUCT_NAME "Pitch Scientific Test"
    VERSION 1.0.0)

target_sources(test_pitch_scientific PRIVATE
    test_pitch_accuracy_scientific.cpp
    ${ENGINE_SOURCES}
)

target_compile_definitions(test_pitch_scientific PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

target_include_directories(test_pitch_scientific PRIVATE
    ${SOURCE_DIR}
    ${SOURCE_DIR}/../JuceLibraryCode
    .
)

target_link_libraries(test_pitch_scientific PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_dsp
)

# Output to build directory
set_target_properties(test_pitch_scientific PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)

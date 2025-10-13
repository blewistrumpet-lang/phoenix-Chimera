cmake_minimum_required(VERSION 3.15)
project(AudioQualityValidation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# JUCE setup
add_subdirectory(/Users/Branden/JUCE JUCE_BUILD)

# Glob all engine source files
file(GLOB ENGINE_SOURCES
    ../../JUCE_Plugin/Source/*.cpp
)

# Remove plugin processor and main files
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*PluginProcessor\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*Main\\.cpp$")

# Add test executable
add_executable(test_audio_quality_validation
    ../test_audio_quality_validation.cpp
    ${ENGINE_SOURCES}
)

target_include_directories(test_audio_quality_validation PRIVATE
    ../../JUCE_Plugin/Source
    ../../JUCE_Plugin/JuceLibraryCode
    ..
)

target_link_libraries(test_audio_quality_validation PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_dsp
)

target_compile_definitions(test_audio_quality_validation PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

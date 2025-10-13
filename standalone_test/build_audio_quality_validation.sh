#!/bin/bash

echo "================================================================================"
echo "Building Professional Audio Quality Validation Suite"
echo "================================================================================"

BUILD_DIR="build_audio_quality"
SOURCE_DIR=".."
JUCE_DIR="/Users/Branden/JUCE"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || exit 1

# CMake configuration
cat > CMakeLists.txt << 'EOF'
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
EOF

echo ""
echo "Running CMake..."
cmake . -DCMAKE_BUILD_TYPE=Release

echo ""
echo "Building..."
make -j$(sysctl -n hw.ncpu)

if [ $? -eq 0 ]; then
    echo ""
    echo "================================================================================"
    echo "Build successful!"
    echo "================================================================================"
    echo ""
    echo "Running Audio Quality Validation..."
    echo ""
    ./test_audio_quality_validation

    if [ $? -eq 0 ]; then
        echo ""
        echo "================================================================================"
        echo "VALIDATION COMPLETE - ALL ENGINES PASS"
        echo "================================================================================"
    else
        echo ""
        echo "================================================================================"
        echo "VALIDATION COMPLETE - SOME ISSUES DETECTED"
        echo "================================================================================"
    fi
else
    echo ""
    echo "================================================================================"
    echo "Build failed!"
    echo "================================================================================"
    exit 1
fi

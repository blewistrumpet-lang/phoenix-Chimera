#!/bin/bash

echo "Building Platform Compatibility Test..."

clang++ -std=c++17 \
    -I../JUCE_Plugin/Source \
    -I/Users/Branden/JUCE/modules \
    -o test_platform_compatibility \
    test_platform_compatibility.cpp \
    ../JUCE_Plugin/Source/EngineFactory.cpp \
    ../JUCE_Plugin/Source/ParametricEQ.cpp \
    -framework Accelerate \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioToolbox \
    -framework Cocoa \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_USE_CURL=0 \
    -DJUCE_WEB_BROWSER=0 \
    -w

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo ""
    echo "Running platform compatibility tests..."
    echo ""
    ./test_platform_compatibility
else
    echo "Build failed!"
    exit 1
fi

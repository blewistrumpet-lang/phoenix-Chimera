#!/bin/bash

# Temporary compile script for new engines
# This manually compiles the new engine files and adds them to the Xcode project

cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Builds/MacOSX

# Compile RodentDistortion
clang++ -c -O3 -std=c++17 -arch arm64 -arch x86_64 \
    -I../../JuceLibraryCode \
    -I../../../JUCE/modules \
    -I../../Source \
    ../../Source/RodentDistortion.cpp \
    -o build/RodentDistortion.o

# Compile MuffFuzz
clang++ -c -O3 -std=c++17 -arch arm64 -arch x86_64 \
    -I../../JuceLibraryCode \
    -I../../../JUCE/modules \
    -I../../Source \
    ../../Source/MuffFuzz.cpp \
    -o build/MuffFuzz.o

# Compile ClassicTremolo
clang++ -c -O3 -std=c++17 -arch arm64 -arch x86_64 \
    -I../../JuceLibraryCode \
    -I../../../JUCE/modules \
    -I../../Source \
    ../../Source/ClassicTremolo.cpp \
    -o build/ClassicTremolo.o

# Add to the static library
ar -r build/Release/libChimeraPhoenix.a \
    build/RodentDistortion.o \
    build/MuffFuzz.o \
    build/ClassicTremolo.o

echo "New engines compiled and added to library"
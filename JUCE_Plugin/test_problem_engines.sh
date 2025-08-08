#!/bin/bash

echo "Testing Problem Engines in Isolation"
echo "===================================="

# Build the quick test
echo "Building test program..."
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++ \
    -std=c++17 \
    -arch arm64 \
    -I/Users/Branden/JUCE/modules \
    -I/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/JuceLibraryCode \
    -I/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin \
    -framework CoreAudio \
    -framework Accelerate \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_STANDALONE_APPLICATION=0 \
    -DJUCE_PLUGINHOST_AU=1 \
    quick_engine_test.cpp \
    Source/EngineFactory.cpp \
    Source/EngineBase.cpp \
    Source/TrinityPipelineManager.cpp \
    Source/Engines/*.cpp \
    ~/JUCE/modules/juce_audio_basics/juce_audio_basics.mm \
    ~/JUCE/modules/juce_core/juce_core.mm \
    ~/JUCE/modules/juce_dsp/juce_dsp.mm \
    -o quick_engine_test 2>/dev/null

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "Testing key engines:"
echo ""

# Test specific engines
engines=(
    "1:Rodent Distortion"
    "2:Vintage Overdrive"
    "11:Tape Delay"
    "21:Plate Reverb"
    "31:Spring Reverb"
    "6:Moog Filter"
    "16:Granular"
    "26:Spectral Filter"
)

for engine in "${engines[@]}"; do
    IFS=':' read -r id name <<< "$engine"
    echo "Testing $name (ID: $id)"
    echo "------------------------"
    ./quick_engine_test $id
    echo ""
done

echo "Test complete!"
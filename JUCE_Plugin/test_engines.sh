#!/bin/bash
echo "Building quick engine test..."
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++ -std=c++17 -arch arm64 -I/Users/Branden/JUCE/modules -I/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/JuceLibraryCode -I/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin -framework CoreAudio -framework Accelerate -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=0 -DJUCE_PLUGINHOST_AU=1 quick_engine_test.cpp Source/EngineFactory.cpp Source/EngineBase.cpp Source/TrinityPipelineManager.cpp Source/Engines/*.cpp ~/JUCE/modules/juce_audio_basics/juce_audio_basics.mm ~/JUCE/modules/juce_core/juce_core.mm ~/JUCE/modules/juce_dsp/juce_dsp.mm -o quick_engine_test 2>&1

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo ""
    echo "Testing Rodent Distortion (1):"
    ./quick_engine_test 1
    echo ""
    echo "Testing Vintage Overdrive (2):"
    ./quick_engine_test 2
    echo ""
    echo "Testing Tape Delay (11):"
    ./quick_engine_test 11
    echo ""
    echo "Testing Plate Reverb (21):"
    ./quick_engine_test 21
else
    echo "Build failed - checking what's missing..."
    ls -la Source/ | head -10
fi
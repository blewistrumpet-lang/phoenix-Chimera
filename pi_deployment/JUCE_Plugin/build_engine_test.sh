#!/bin/bash

echo "Building Engine Isolation Test..."

# Compile the test
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++ \
    -std=c++17 \
    -arch arm64 \
    -I/Users/Branden/JUCE/modules \
    -I/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/JuceLibraryCode \
    -I/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioToolbox \
    -framework Accelerate \
    -framework CoreFoundation \
    -framework AppKit \
    -framework CoreGraphics \
    -framework CoreText \
    -framework IOKit \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_STANDALONE_APPLICATION=0 \
    -DJUCE_PLUGINHOST_AU=1 \
    -DJUCE_USE_CURL=0 \
    -DJUCE_WEB_BROWSER=0 \
    test_engine_isolation.cpp \
    Source/EngineFactory.cpp \
    Source/EngineBase.cpp \
    Source/TrinityPipelineManager.cpp \
    Source/Engines/*.cpp \
    ~/JUCE/modules/juce_audio_basics/juce_audio_basics.mm \
    ~/JUCE/modules/juce_core/juce_core.mm \
    ~/JUCE/modules/juce_events/juce_events.mm \
    ~/JUCE/modules/juce_audio_devices/juce_audio_devices.mm \
    ~/JUCE/modules/juce_audio_formats/juce_audio_formats.mm \
    ~/JUCE/modules/juce_audio_processors/juce_audio_processors.mm \
    ~/JUCE/modules/juce_gui_basics/juce_gui_basics.mm \
    ~/JUCE/modules/juce_graphics/juce_graphics.mm \
    ~/JUCE/modules/juce_data_structures/juce_data_structures.mm \
    ~/JUCE/modules/juce_dsp/juce_dsp.mm \
    -o test_engine_isolation

if [ $? -eq 0 ]; then
    echo "Build successful! Running test..."
    echo ""
    ./test_engine_isolation
else
    echo "Build failed!"
    exit 1
fi
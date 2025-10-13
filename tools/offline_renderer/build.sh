#!/bin/bash
set -e
PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
cd "$PROJECT_ROOT/JUCE_Plugin/Builds/MacOSX"
xcodebuild -project ChimeraPhoenix.xcodeproj -configuration Debug -target "ChimeraPhoenix - Shared Code" -quiet
cd "$PROJECT_ROOT/tools/offline_renderer"
clang++ -std=c++17 -O2 \
    -I"$PROJECT_ROOT/JUCE_Plugin/Source" -I"$PROJECT_ROOT/JUCE_Plugin/JuceLibraryCode" -I/Users/Branden/JUCE/modules \
    -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -framework CoreAudio -framework CoreFoundation -framework Accelerate -framework AudioToolbox \
    -framework AudioUnit -framework CoreAudioKit -framework CoreMIDI -framework Cocoa \
    -framework Carbon -framework QuartzCore -framework IOKit -framework Security \
    -framework WebKit -framework Metal -framework MetalKit \
    OfflineRender.cpp "$PROJECT_ROOT/JUCE_Plugin/Builds/MacOSX/build/Debug/libChimeraPhoenix.a" \
    -o offline_renderer
echo "✅ Built: $PROJECT_ROOT/tools/offline_renderer/offline_renderer"

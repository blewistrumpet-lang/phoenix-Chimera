#!/bin/bash
# Build script for test_modulation_realworld

echo "Building test_modulation_realworld..."

# Get engine objects from required_engines.txt
ENGINE_OBJS=""
while IFS= read -r line; do
    OBJ="build/obj/$(basename "$line" .cpp).o"
    ENGINE_OBJS="$ENGINE_OBJS $OBJ"
done < required_engines.txt

# JUCE modules
JUCE_OBJS="juce_core.o juce_audio_basics.o juce_dsp.o build/obj/juce_audio_formats.o build/obj/juce_events.o build/obj/juce_data_structures.o build/obj/juce_graphics.o build/obj/juce_gui_basics.o build/obj/juce_gui_extra.o build/obj/juce_audio_processors.o build/obj/juce_core_CompilationTime.o build/obj/SheenBidi.o"

# Compile
clang++ -std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable \
    -I. -I../JUCE_Plugin/Source -I../JUCE_Plugin/Source/../JuceLibraryCode -I/Users/Branden/JUCE/modules \
    -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DDEBUG=1 \
    -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1 \
    test_modulation_realworld.cpp \
    $JUCE_OBJS $ENGINE_OBJS \
    -framework Accelerate -framework CoreAudio -framework CoreFoundation \
    -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security \
    -framework QuartzCore -framework CoreImage -framework CoreGraphics \
    -framework CoreText -framework WebKit -framework DiscRecording \
    -L/opt/homebrew/lib -lharfbuzz \
    -o test_modulation_realworld

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    ls -lh test_modulation_realworld
else
    echo "✗ Build failed"
    exit 1
fi

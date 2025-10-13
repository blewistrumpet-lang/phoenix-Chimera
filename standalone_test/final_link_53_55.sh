#!/bin/bash
set -e

cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

echo "Collecting engine object files..."
ENGINE_OBJS=$(ls build/obj/*.o | grep -E "^build/obj/[A-Z]" | grep -v "_test" | grep -v "SheenBidi" | tr '\n' ' ')

echo "Linking test_engines_53_55..."

clang++ -std=c++17 -O2 \
    build/obj/test_engines_53_55.o \
    $ENGINE_OBJS \
    build/obj/juce_core.o \
    build/obj/juce_audio_basics.o \
    build/obj/juce_dsp.o \
    build/obj/juce_events.o \
    build/obj/juce_data_structures.o \
    build/obj/juce_graphics.o \
    build/obj/juce_gui_basics.o \
    build/obj/juce_audio_formats.o \
    build/obj/juce_core_CompilationTime.o \
    build/obj/SheenBidi.o \
    -framework Accelerate \
    -framework CoreAudio \
    -framework CoreFoundation \
    -framework AudioToolbox \
    -framework Cocoa \
    -framework IOKit \
    -framework Security \
    -framework QuartzCore \
    -framework CoreImage \
    -framework CoreGraphics \
    -framework CoreText \
    -framework WebKit \
    -framework DiscRecording \
    -L/opt/homebrew/lib -lharfbuzz \
    -o build/test_engines_53_55

if [ $? -eq 0 ]; then
    echo ""
    echo "================================"
    echo "SUCCESS! Executable created:"
    echo "  build/test_engines_53_55"
    echo "================================"
    ls -lh build/test_engines_53_55
else
    echo "Link failed!"
    exit 1
fi

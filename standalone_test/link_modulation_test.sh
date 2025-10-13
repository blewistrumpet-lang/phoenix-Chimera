#!/bin/bash
# Link test_modulation_realworld

echo "Linking test_modulation_realworld..."

ENGINE_OBJS=""
while IFS= read -r line; do
    OBJ="build/obj/$(basename "$line" .cpp).o"
    ENGINE_OBJS="$ENGINE_OBJS $OBJ"
done < required_engines.txt

JUCE_OBJS="build/obj/juce_core.o build/obj/juce_audio_basics.o build/obj/juce_dsp.o"
JUCE_OBJS="$JUCE_OBJS build/obj/juce_audio_formats.o build/obj/juce_events.o"
JUCE_OBJS="$JUCE_OBJS build/obj/juce_data_structures.o build/obj/juce_graphics.o"
JUCE_OBJS="$JUCE_OBJS build/obj/juce_gui_basics.o build/obj/juce_gui_extra.o"
JUCE_OBJS="$JUCE_OBJS build/obj/juce_audio_processors.o build/obj/juce_core_CompilationTime.o"
JUCE_OBJS="$JUCE_OBJS build/obj/SheenBidi.o"

clang++ build/obj/test_modulation_realworld_release.o $JUCE_OBJS $ENGINE_OBJS \
    -framework Accelerate -framework CoreAudio -framework CoreFoundation \
    -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security \
    -framework QuartzCore -framework CoreImage -framework CoreGraphics \
    -framework CoreText -framework WebKit -framework DiscRecording \
    -L/opt/homebrew/lib -lharfbuzz \
    -o test_modulation_realworld

if [ $? -eq 0 ]; then
    echo "✓ Link successful!"
    ls -lh test_modulation_realworld
else
    echo "✗ Link failed"
    exit 1
fi

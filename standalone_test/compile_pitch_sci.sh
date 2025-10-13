#!/bin/bash
clang++ -std=c++17 -O2 -DJUCE_STANDALONE_APPLICATION=1 -DNDEBUG=1 \
  -I. -I../JUCE_Plugin/Source -I../JUCE_Plugin/JuceLibraryCode -I/Users/Branden/JUCE/modules \
  -o build/test_pitch_accuracy_scientific \
  test_pitch_accuracy_scientific.cpp \
  juce_core.o juce_audio_basics.o juce_audio_devices.o juce_audio_processors.o juce_dsp.o \
  juce_data_structures.o juce_events.o juce_graphics.o juce_gui_basics.o \
  build/obj/EngineFactory.o build/obj/PitchShifter.o build/obj/DetuneDoubler.o \
  build/obj/IntelligentHarmonizer.o build/obj/ShimmerReverb.o build/obj/PhasedVocoder.o \
  build/obj/GranularCloud.o build/obj/PlateReverb.o build/obj/BiquadFilter.o \
  build/obj/ModulatedDelay.o build/obj/StateVariableFilter.o build/obj/IntelligentHarmonizerChords.o \
  -framework Accelerate -framework CoreAudio -framework CoreFoundation \
  -framework AudioToolbox -framework Cocoa -framework IOKit

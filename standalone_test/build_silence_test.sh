#!/bin/bash
#
# build_silence_test.sh
#
# Builds the comprehensive silence handling test for all 56 engines
#

set -e  # Exit on error

echo "========================================================================"
echo "    Building Silence Handling Test for All 56 Engines"
echo "========================================================================"
echo ""

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

# Check JUCE exists
if [ ! -d "$JUCE_DIR/modules" ]; then
    echo "ERROR: JUCE not found at $JUCE_DIR"
    exit 1
fi

# Create build directories
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Common flags
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# Compiler flags
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
OBJCPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

# Output executable
OUTPUT="$BUILD_DIR/test_silence_handling"

echo "Step 1: Using existing JUCE modules..."
echo ""

# JUCE modules object files (use existing ones from build directory)
JUCE_OBJS=""
JUCE_MODULES="juce_core juce_audio_basics juce_audio_formats juce_audio_processors juce_dsp juce_events juce_data_structures juce_graphics juce_gui_basics juce_gui_extra juce_core_CompilationTime"

for module in $JUCE_MODULES; do
    if [ -f "$BUILD_DIR/$module.o" ]; then
        echo "  ✓ $module (from build dir)"
        JUCE_OBJS="$JUCE_OBJS $BUILD_DIR/$module.o"
    elif [ -f "$OBJ_DIR/$module.o" ]; then
        echo "  ✓ $module (from obj dir)"
        JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/$module.o"
    else
        echo "  ✗ $module not found - please run build_all.sh first"
        exit 1
    fi
done

echo ""
echo "Step 2: Compiling engine sources..."
echo ""

# List of all engine source files
ENGINE_SOURCES=(
    "EngineFactory.cpp"
    "EngineBase.cpp"
    "NoneEngine.cpp"
    "PitchShiftFactory.cpp"
    "VintageOptoCompressor_Platinum.cpp"
    "ClassicCompressor.cpp"
    "TransientShaper_Platinum.cpp"
    "NoiseGate_Platinum.cpp"
    "MasteringLimiter_Platinum.cpp"
    "DynamicEQ.cpp"
    "ParametricEQ_Studio.cpp"
    "VintageConsoleEQ_Studio.cpp"
    "LadderFilter.cpp"
    "StateVariableFilter.cpp"
    "FormantFilter.cpp"
    "EnvelopeFilter.cpp"
    "CombResonator.cpp"
    "VocalFormantFilter.cpp"
    "VintageTubePreamp_Studio.cpp"
    "WaveFolder.cpp"
    "HarmonicExciter_Platinum.cpp"
    "BitCrusher.cpp"
    "MultibandSaturator.cpp"
    "MuffFuzz.cpp"
    "RodentDistortion.cpp"
    "KStyleOverdrive.cpp"
    "StereoChorus.cpp"
    "ResonantChorus_Platinum.cpp"
    "AnalogPhaser.cpp"
    "PlatinumRingModulator.cpp"
    "FrequencyShifter.cpp"
    "HarmonicTremolo.cpp"
    "ClassicTremolo.cpp"
    "RotarySpeaker_Platinum.cpp"
    "PitchShifter.cpp"
    "DetuneDoubler.cpp"
    "IntelligentHarmonizer.cpp"
    "TapeEcho.cpp"
    "DigitalDelay.cpp"
    "MagneticDrumEcho.cpp"
    "BucketBrigadeDelay.cpp"
    "BufferRepeat_Platinum.cpp"
    "PlateReverb.cpp"
    "SpringReverb.cpp"
    "ConvolutionReverb.cpp"
    "ShimmerReverb.cpp"
    "GatedReverb.cpp"
    "StereoWidener.cpp"
    "StereoImager.cpp"
    "DimensionExpander.cpp"
    "SpectralFreeze.cpp"
    "SpectralGate_Platinum.cpp"
    "PhasedVocoder.cpp"
    "GranularCloud.cpp"
    "ChaosGenerator.cpp"
    "FeedbackNetwork.cpp"
    "MidSideProcessor_Platinum.cpp"
    "GainUtility_Platinum.cpp"
    "MonoMaker_Platinum.cpp"
    "PhaseAlign_Platinum.cpp"
)

# Compile each engine source
ENGINE_OBJS=""
for src in "${ENGINE_SOURCES[@]}"; do
    obj_file="$OBJ_DIR/$(basename $src .cpp).o"

    if [ -f "$obj_file" ]; then
        echo "  ✓ $src (cached)"
    else
        if [ -f "$PLUGIN_SRC/$src" ]; then
            echo "  → Compiling $src..."
            clang++ $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/$src" -o "$obj_file"
        else
            echo "  ✗ Source file not found: $src"
        fi
    fi

    if [ -f "$obj_file" ]; then
        ENGINE_OBJS="$ENGINE_OBJS $obj_file"
    fi
done

echo ""
echo "Step 3: Compiling test main..."
MAIN_OBJ="$OBJ_DIR/test_silence_handling_main.o"
echo "  → Compiling test_silence_handling.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES -c test_silence_handling.cpp -o "$MAIN_OBJ"

echo ""
echo "Step 4: Adding additional required object files..."
ADDITIONAL_OBJS="$OBJ_DIR/SheenBidi.o $OBJ_DIR/SMBPitchShiftFixed.o"

echo ""
echo "Step 5: Linking..."
clang++ $CPP_FLAGS $MAIN_OBJ $ENGINE_OBJS $JUCE_OBJS $ADDITIONAL_OBJS $FRAMEWORKS -L/opt/homebrew/lib -lharfbuzz -o "$OUTPUT"

echo ""
if [ -f "$OUTPUT" ]; then
    echo "========================================================================"
    echo "    Build Successful!"
    echo "========================================================================"
    echo ""
    echo "Executable: $OUTPUT"
    echo ""
    echo "To run the test:"
    echo "  cd $BUILD_DIR"
    echo "  ./test_silence_handling"
    echo ""
    echo "Output will be saved to: silence_handling_report.txt"
    echo ""
else
    echo "========================================================================"
    echo "    Build Failed!"
    echo "========================================================================"
    exit 1
fi

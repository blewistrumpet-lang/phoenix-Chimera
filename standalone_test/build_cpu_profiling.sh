#!/bin/bash
#
# build_cpu_profiling.sh
#
# Builds the comprehensive CPU profiling suite
#

set -e  # Exit on error

echo "========================================================================"
echo "    Building Comprehensive CPU Profiling Suite"
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

# Compiler flags - optimized for performance
CPP_FLAGS="-std=c++17 -O3 -march=native -Wall -Wno-unused-parameter -Wno-unused-variable"
OBJCPP_FLAGS="-std=c++17 -O3 -march=native -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

# Output executable
OUTPUT="$BUILD_DIR/test_cpu_profiling"

echo "Step 1: Using existing JUCE modules..."
echo ""

# JUCE modules object files (should already exist from build_all.sh)
JUCE_OBJS=""
JUCE_MODULES="juce_core juce_audio_basics juce_audio_formats juce_audio_processors juce_dsp juce_events juce_data_structures juce_graphics juce_gui_basics juce_gui_extra"

for module in $JUCE_MODULES; do
    if [ -f "$OBJ_DIR/$module.o" ]; then
        echo "  ✓ $module (cached)"
        JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/$module.o"
    else
        echo "  → Compiling $module..."
        clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES \
            -c "$JUCE_DIR/modules/$module/$module.cpp" \
            -o "$OBJ_DIR/$module.o"
        JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/$module.o"
    fi
done

# juce_core_CompilationTime
if [ -f "$OBJ_DIR/juce_core_CompilationTime.o" ]; then
    echo "  ✓ juce_core_CompilationTime (cached)"
else
    echo "  → Compiling juce_core_CompilationTime..."
    clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES \
        -c "$JUCE_DIR/modules/juce_core/juce_core_CompilationTime.cpp" \
        -o "$OBJ_DIR/juce_core_CompilationTime.o"
fi
JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/juce_core_CompilationTime.o"

echo ""
echo "Step 2: Compiling engine sources..."
echo ""

# List of all engine source files
ENGINE_SOURCES=(
    "EngineFactory.cpp"
    "EngineBase.cpp"
    "NoneEngine.cpp"
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
echo "Step 3: Compiling CPU profiling suite main..."
MAIN_OBJ="$OBJ_DIR/test_cpu_profiling.o"
echo "  → Compiling test_cpu_profiling.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES -c test_cpu_profiling.cpp -o "$MAIN_OBJ"

echo ""
echo "Step 4: Linking..."
clang++ $CPP_FLAGS $MAIN_OBJ $ENGINE_OBJS $JUCE_OBJS $FRAMEWORKS -o "$OUTPUT"

echo ""
if [ -f "$OUTPUT" ]; then
    echo "========================================================================"
    echo "    Build Successful!"
    echo "========================================================================"
    echo ""
    echo "Executable: $OUTPUT"
    echo ""
    echo "To run the profiling suite:"
    echo "  cd $BUILD_DIR"
    echo "  ./test_cpu_profiling"
    echo ""
    echo "Or from this directory:"
    echo "  $OUTPUT"
    echo ""
    echo "Results will be saved to:"
    echo "  - cpu_profiling_detailed.csv"
    echo "  - cpu_profiling_multi_engine.csv"
    echo ""
else
    echo "========================================================================"
    echo "    Build Failed!"
    echo "========================================================================"
    exit 1
fi

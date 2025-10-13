#!/bin/bash
#
# build_sample_rate_test.sh
#
# Builds the sample rate independence test for key engines
#

set -e  # Exit on error

echo "========================================================================"
echo "    Building Sample Rate Independence Test"
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
OUTPUT="$BUILD_DIR/test_sample_rate_independence"

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

# Compile SheenBidi (required for juce_graphics)
if [ -f "$OBJ_DIR/SheenBidi.o" ]; then
    echo "  ✓ SheenBidi (cached)"
else
    echo "  → Compiling SheenBidi..."
    clang -O2 -I. -I$JUCE_DIR/modules \
        -c "$JUCE_DIR/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c" \
        -o "$OBJ_DIR/SheenBidi.o"
fi
JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/SheenBidi.o"

echo ""
echo "Step 2: Compiling required engine sources..."
echo ""

# Key engines needed for sample rate test
ENGINE_SOURCES=(
    "EngineFactory.cpp"
    "EngineBase.cpp"
    "NoneEngine.cpp"
    "VintageOptoCompressor_Platinum.cpp"
    "ClassicCompressor.cpp"
    "MasteringLimiter_Platinum.cpp"
    "ParametricEQ_Studio.cpp"
    "LadderFilter.cpp"
    "StateVariableFilter.cpp"
    "VintageTubePreamp_Studio.cpp"
    "MuffFuzz.cpp"
    "KStyleOverdrive.cpp"
    "StereoChorus.cpp"
    "AnalogPhaser.cpp"
    "ClassicTremolo.cpp"
    "RotarySpeaker_Platinum.cpp"
    "TapeEcho.cpp"
    "DigitalDelay.cpp"
    "BucketBrigadeDelay.cpp"
    "PlateReverb.cpp"
    "SpringReverb.cpp"
    "ShimmerReverb.cpp"
    "StereoWidener.cpp"
    "DimensionExpander.cpp"
    "SpectralFreeze.cpp"
    "PhasedVocoder.cpp"
    "GranularCloud.cpp"
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
MAIN_OBJ="$OBJ_DIR/sample_rate_test_main.o"
echo "  → Compiling test_sample_rate_independence.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES -c test_sample_rate_independence.cpp -o "$MAIN_OBJ"

echo ""
echo "Step 4: Linking..."
clang++ $CPP_FLAGS $MAIN_OBJ $ENGINE_OBJS $JUCE_OBJS $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$OUTPUT"

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
    echo "  ./test_sample_rate_independence"
    echo ""
    echo "This will generate: sample_rate_compatibility_report.txt"
    echo ""
else
    echo "========================================================================"
    echo "    Build Failed!"
    echo "========================================================================"
    exit 1
fi

#!/bin/bash

set -e

echo "╔════════════════════════════════════════════════════════════════════════╗"
echo "║               Building DC Offset Handling Test                        ║"
echo "╚════════════════════════════════════════════════════════════════════════╝"
echo ""

# Paths
JUCE_PATH="/Users/Branden/JUCE/modules"
SOURCE_PATH="../JUCE_Plugin/Source"
BUILD_DIR="build"

# Create build directory
mkdir -p "$BUILD_DIR/obj"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_dsp=1"

# Include paths
INCLUDES="-I${SOURCE_PATH} -I${JUCE_PATH} -I."

# Main test source
TEST_SOURCE="test_dc_offset.cpp"

# Engine source files (all 57 engines: 0-56)
ENGINE_SOURCES=(
    # Engine 0: None
    "${SOURCE_PATH}/NoneEngine.cpp"

    # Dynamics (1-6)
    "${SOURCE_PATH}/OptoCompressor.cpp"
    "${SOURCE_PATH}/VCACompressor.cpp"
    "${SOURCE_PATH}/TransientShaper.cpp"
    "${SOURCE_PATH}/NoiseGate.cpp"
    "${SOURCE_PATH}/MasteringLimiter.cpp"
    "${SOURCE_PATH}/DynamicEQ.cpp"

    # Filters/EQ (7-14)
    "${SOURCE_PATH}/ParametricEQ.cpp"
    "${SOURCE_PATH}/VintageConsoleEQ.cpp"
    "${SOURCE_PATH}/LadderFilter.cpp"
    "${SOURCE_PATH}/StateVariableFilter.cpp"
    "${SOURCE_PATH}/FormantFilter.cpp"
    "${SOURCE_PATH}/EnvelopeFilter.cpp"
    "${SOURCE_PATH}/CombResonator.cpp"
    "${SOURCE_PATH}/VocalFormant.cpp"

    # Distortion (15-22)
    "${SOURCE_PATH}/VintageTubePreamp.cpp"
    "${SOURCE_PATH}/WaveFolder.cpp"
    "${SOURCE_PATH}/HarmonicExciter.cpp"
    "${SOURCE_PATH}/BitCrusher.cpp"
    "${SOURCE_PATH}/MultibandSaturator.cpp"
    "${SOURCE_PATH}/MuffFuzz.cpp"
    "${SOURCE_PATH}/RodentDistortion.cpp"
    "${SOURCE_PATH}/KStyleOverdrive.cpp"

    # Modulation (23-33)
    "${SOURCE_PATH}/StereoChorus.cpp"
    "${SOURCE_PATH}/ResonantChorus.cpp"
    "${SOURCE_PATH}/AnalogPhaser.cpp"
    "${SOURCE_PATH}/RingModulator.cpp"
    "${SOURCE_PATH}/FrequencyShifter.cpp"
    "${SOURCE_PATH}/HarmonicTremolo.cpp"
    "${SOURCE_PATH}/ClassicTremolo.cpp"
    "${SOURCE_PATH}/RotarySpeaker.cpp"
    "${SOURCE_PATH}/PitchShifter.cpp"
    "${SOURCE_PATH}/DetuneDoubler.cpp"
    "${SOURCE_PATH}/IntelligentHarmonizer.cpp"

    # Delays (34-38)
    "${SOURCE_PATH}/TapeEcho.cpp"
    "${SOURCE_PATH}/DigitalDelay.cpp"
    "${SOURCE_PATH}/MagneticDrumEcho.cpp"
    "${SOURCE_PATH}/BucketBrigadeDelay.cpp"
    "${SOURCE_PATH}/BufferRepeat.cpp"

    # Reverbs (39-43)
    "${SOURCE_PATH}/PlateReverb.cpp"
    "${SOURCE_PATH}/SpringReverb.cpp"
    "${SOURCE_PATH}/ConvolutionReverb.cpp"
    "${SOURCE_PATH}/ShimmerReverb.cpp"
    "${SOURCE_PATH}/GatedReverb.cpp"

    # Spatial (44-48)
    "${SOURCE_PATH}/StereoWidener.cpp"
    "${SOURCE_PATH}/StereoImager.cpp"
    "${SOURCE_PATH}/DimensionExpander.cpp"
    "${SOURCE_PATH}/SpectralFreeze.cpp"
    "${SOURCE_PATH}/SpectralGate.cpp"

    # Special (49-52)
    "${SOURCE_PATH}/PhasedVocoder.cpp"
    "${SOURCE_PATH}/GranularCloud.cpp"
    "${SOURCE_PATH}/ChaosGenerator.cpp"
    "${SOURCE_PATH}/FeedbackNetwork.cpp"

    # Utility (53-56)
    "${SOURCE_PATH}/MidSideProcessor.cpp"
    "${SOURCE_PATH}/GainUtility.cpp"
    "${SOURCE_PATH}/MonoMaker.cpp"
    "${SOURCE_PATH}/PhaseAlign.cpp"

    # EngineFactory
    "${SOURCE_PATH}/EngineFactory.cpp"

    # Utilities
    "${SOURCE_PATH}/DspEngineUtilities.cpp"
)

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework CoreMidi -framework IOKit -framework AppKit -framework WebKit -framework Security -framework QuartzCore"

# JUCE modules to compile
JUCE_MODULES=(
    "${JUCE_PATH}/juce_core/juce_core.mm"
    "${JUCE_PATH}/juce_audio_basics/juce_audio_basics.mm"
    "${JUCE_PATH}/juce_dsp/juce_dsp.mm"
)

echo "Step 1: Compiling JUCE modules..."
for module in "${JUCE_MODULES[@]}"; do
    module_name=$(basename "$module" .mm)
    obj_file="${BUILD_DIR}/obj/${module_name}.o"

    if [ ! -f "$obj_file" ]; then
        echo "  Compiling ${module_name}..."
        $CXX $CXXFLAGS $INCLUDES -c "$module" -o "$obj_file" 2>&1 | grep -v "warning:" || true
    else
        echo "  ✓ ${module_name} (cached)"
    fi
done

echo ""
echo "Step 2: Compiling test source..."
test_name=$(basename "$TEST_SOURCE" .cpp)
obj_file="${BUILD_DIR}/obj/${test_name}.o"
echo "  Compiling ${test_name}..."
$CXX $CXXFLAGS $INCLUDES -c "$TEST_SOURCE" -o "$obj_file" 2>&1 | grep -v "warning:" || true

echo ""
echo "Step 3: Compiling engine sources..."
OBJECT_FILES=("$obj_file")

for source in "${ENGINE_SOURCES[@]}"; do
    if [ ! -f "$source" ]; then
        echo "  ⚠ Skipping ${source} (not found)"
        continue
    fi

    source_name=$(basename "$source" .cpp)
    obj_file="${BUILD_DIR}/obj/${source_name}.o"
    OBJECT_FILES+=("$obj_file")

    echo "  Compiling ${source_name}..."
    $CXX $CXXFLAGS $INCLUDES -c "$source" -o "$obj_file" 2>&1 | grep -v "warning:" || true
done

# Add JUCE module objects
for module in "${JUCE_MODULES[@]}"; do
    module_name=$(basename "$module" .mm)
    OBJECT_FILES+=("${BUILD_DIR}/obj/${module_name}.o")
done

echo ""
echo "Step 4: Linking..."
OUTPUT="test_dc_offset"
$CXX $CXXFLAGS ${OBJECT_FILES[@]} $FRAMEWORKS -o "$OUTPUT"

if [ $? -eq 0 ]; then
    echo ""
    echo "╔════════════════════════════════════════════════════════════════════════╗"
    echo "║                       BUILD SUCCESSFUL!                                ║"
    echo "╚════════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Executable: ./${OUTPUT}"
    echo ""
    echo "To run the test:"
    echo "  ./${OUTPUT}"
    echo ""
    echo "Output files:"
    echo "  • dc_offset_test_results.csv - Full DC offset test results"
    echo ""
else
    echo ""
    echo "ERROR: Build failed"
    exit 1
fi

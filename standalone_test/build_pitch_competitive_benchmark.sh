#!/bin/bash

# Competitive Benchmark Build Script
# Compiles the pitch engines competitive benchmark against industry standards

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║     Building Pitch Engines Competitive Benchmark Suite          ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo ""

# Create build directory
mkdir -p build

# Source and output
SOURCE="test_pitch_engines_competitive_benchmark.cpp"
OUTPUT="build/pitch_competitive_benchmark"

# Paths
JUCE_PATH="../JUCE_Plugin/JuceLibraryCode"
ENGINE_PATH="../JUCE_Plugin/Source"

echo "[1/3] Compiling benchmark suite..."
echo "      Source: $SOURCE"
echo ""

# Compile with optimizations
clang++ -std=c++17 -O3 \
    -I"$JUCE_PATH" \
    -I"$ENGINE_PATH" \
    -I"$JUCE_PATH/modules" \
    "$SOURCE" \
    -o "$OUTPUT" \
    -framework Accelerate \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioUnit \
    -framework AudioToolbox \
    -framework CoreFoundation \
    -framework Carbon \
    -framework Cocoa \
    -framework IOKit \
    -framework QuartzCore \
    -framework WebKit \
    -framework DiscRecording \
    -ljuce_audio_basics \
    -ljuce_audio_devices \
    -ljuce_audio_processors \
    -ljuce_core \
    -ljuce_data_structures \
    -ljuce_events \
    -ljuce_graphics \
    -ljuce_gui_basics \
    -L"$JUCE_PATH" \
    -Wno-deprecated-declarations \
    -Wno-unknown-warning-option

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Compilation successful!"
    echo ""
    echo "[2/3] Verifying executable..."

    if [ -x "$OUTPUT" ]; then
        echo "✓ Executable is ready"
        echo ""
        echo "[3/3] Setup complete!"
        echo ""
        echo "╔══════════════════════════════════════════════════════════════════╗"
        echo "║                  BUILD SUCCESSFUL                                ║"
        echo "╚══════════════════════════════════════════════════════════════════╝"
        echo ""
        echo "Run the benchmark:"
        echo "  ./$OUTPUT"
        echo ""
        echo "This will test all 8 pitch engines and compare them against:"
        echo "  • Melodyne (Best-in-class)"
        echo "  • Auto-Tune (Professional)"
        echo "  • Waves Tune (Mid-tier)"
        echo "  • Little AlterBoy (Creative)"
        echo ""
        echo "Metrics measured:"
        echo "  1. Pitch Accuracy (cents error)"
        echo "  2. THD (Total Harmonic Distortion %)"
        echo "  3. Latency (milliseconds)"
        echo "  4. CPU Usage (% of single core)"
        echo "  5. Formant Preservation (spectral quality)"
        echo "  6. Artifact Level (noise floor, dB)"
        echo "  7. Transient Preservation (attack time accuracy)"
        echo ""
        echo "Results will be saved to:"
        echo "  build/pitch_engines_competitive_benchmark.csv"
        echo ""
    else
        echo "✗ Executable not found or not executable"
        exit 1
    fi
else
    echo ""
    echo "✗ Compilation failed"
    exit 1
fi

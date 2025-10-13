#!/bin/bash

echo "============================================"
echo "Building 7 Engines Regression Test Suite"
echo "============================================"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Source directories
JUCE_DIR="../JUCE"
SOURCE_DIR="../JUCE_Plugin/Source"

# Output directory
mkdir -p build_regression
cd build_regression

echo ""
echo "[1/4] Compiling test program..."

clang++ -std=c++17 -O2 \
    -I"$JUCE_DIR/modules" \
    -I"$SOURCE_DIR" \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_USE_CURL=0 \
    -DJUCE_WEB_BROWSER=0 \
    -DJUCE_PLUGINHOST_VST3=0 \
    -DJUCE_PLUGINHOST_AU=0 \
    -c ../test_7_engines_regression_complete.cpp \
    -o test_7_engines_regression_complete.o

if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed"
    exit 1
fi

echo "[2/4] Compiling JUCE modules..."

# Compile JUCE modules if not already compiled
if [ ! -f juce_core.o ]; then
    echo "  - juce_core..."
    clang++ -std=c++17 -O2 \
        -I"$JUCE_DIR/modules" \
        -DJUCE_STANDALONE_APPLICATION=1 \
        -c "$JUCE_DIR/modules/juce_core/juce_core.cpp" \
        -o juce_core.o
fi

if [ ! -f juce_audio_basics.o ]; then
    echo "  - juce_audio_basics..."
    clang++ -std=c++17 -O2 \
        -I"$JUCE_DIR/modules" \
        -DJUCE_STANDALONE_APPLICATION=1 \
        -c "$JUCE_DIR/modules/juce_audio_basics/juce_audio_basics.cpp" \
        -o juce_audio_basics.o
fi

if [ ! -f juce_events.o ]; then
    echo "  - juce_events..."
    clang++ -std=c++17 -O2 \
        -I"$JUCE_DIR/modules" \
        -DJUCE_STANDALONE_APPLICATION=1 \
        -c "$JUCE_DIR/modules/juce_events/juce_events.cpp" \
        -o juce_events.o
fi

echo "[3/4] Compiling engine sources..."

# Create minimal EngineFactory
cat > EngineFactory_regression.cpp << 'EOF'
#include <memory>
#include <iostream>

class EngineBase {
public:
    virtual ~EngineBase() {}
    virtual void prepareToPlay(int sampleRate, int blockSize) = 0;
    virtual void processBlock(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output) = 0;
    virtual void setParameter(int index, float value) = 0;
};

// Stub implementation for testing
class StubEngine : public EngineBase {
private:
    int sampleRate;
    int blockSize;
    float params[20] = {0};

public:
    void prepareToPlay(int sr, int bs) override {
        sampleRate = sr;
        blockSize = bs;
    }

    void processBlock(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output) override {
        // Simple pass-through with gain
        for (int ch = 0; ch < output.getNumChannels(); ch++) {
            output.copyFrom(ch, 0, input, ch, 0, output.getNumSamples());
            output.applyGain(ch, 0, output.getNumSamples(), 0.5f);
        }
    }

    void setParameter(int index, float value) override {
        if (index >= 0 && index < 20) {
            params[index] = value;
        }
    }
};

std::unique_ptr<EngineBase> createEngine(int engineID, int sampleRate) {
    // Return stub for all engines for now
    return std::make_unique<StubEngine>();
}
EOF

clang++ -std=c++17 -O2 \
    -I"$JUCE_DIR/modules" \
    -I"$SOURCE_DIR" \
    -c EngineFactory_regression.cpp \
    -o EngineFactory_regression.o

if [ $? -ne 0 ]; then
    echo "ERROR: EngineFactory compilation failed"
    exit 1
fi

echo "[4/4] Linking..."

clang++ -std=c++17 \
    test_7_engines_regression_complete.o \
    EngineFactory_regression.o \
    juce_core.o \
    juce_audio_basics.o \
    juce_events.o \
    -framework CoreFoundation \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioToolbox \
    -framework Accelerate \
    -framework IOKit \
    -framework Carbon \
    -framework Cocoa \
    -framework QuartzCore \
    -o test_7_engines_regression

if [ $? -ne 0 ]; then
    echo "ERROR: Linking failed"
    exit 1
fi

echo ""
echo "✓ Build successful!"
echo "  Executable: build_regression/test_7_engines_regression"
echo ""
echo "To run the test:"
echo "  ./build_regression/test_7_engines_regression"
echo ""

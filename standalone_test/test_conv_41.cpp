#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>

int main() {
    auto engine = EngineFactory::createEngine(41);
    if (!engine) {
        std::cerr << "Failed to create engine 41\n";
        return 1;
    }

    std::cout << "Testing Engine 41 (ConvolutionReverb) with damping fix\n";

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    engine->prepareToPlay(sampleRate, blockSize);

    // Set parameters - param[4] = 1.0 (full damping) was causing the bug
    std::map<int, float> params;
    params[0] = 1.0f;  // Mix = 100% wet
    params[1] = 0.7f;  // IR Select
    params[2] = 0.5f;  // Size
    params[4] = 1.0f;  // Damping (THIS WAS THE BUG!)

    engine->updateParameters(params);

    // Create impulse
    juce::AudioBuffer<float> impulse(2, blockSize);
    impulse.clear();
    impulse.setSample(0, 0, 1.0f);
    impulse.setSample(1, 0, 1.0f);

    // Process
    engine->process(impulse);

    // Check output
    float peak = 0.0f;
    int nonZeroCount = 0;
    for (int i = 0; i < blockSize; ++i) {
        float sample = std::abs(impulse.getSample(0, i));
        peak = std::max(peak, sample);
        if (sample > 0.001f) nonZeroCount++;
    }

    std::cout << "\nResults:\n";
    std::cout << "  Peak output: " << peak << "\n";
    std::cout << "  Non-zero samples: " << nonZeroCount << " / " << blockSize << "\n";

    if (peak > 0.1f && nonZeroCount > 10) {
        std::cout << "\n✓ SUCCESS: Reverb is producing output!\n";
        std::cout << "  The damping filter fix is working.\n";
        return 0;
    } else {
        std::cout << "\n✗ FAILURE: Still producing zero/minimal output\n";
        return 1;
    }
}

// Test ShimmerReverb with predelay = 0 to isolate the issue
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include "ShimmerReverb.h"
#include <JuceHeader.h>

int main() {
    std::cout << "Testing ShimmerReverb with Pre-delay = 0\n";
    std::cout << "=========================================\n\n";

    // Create ShimmerReverb directly
    auto engine = std::make_unique<ShimmerReverb>();

    std::cout << "Engine name: " << engine->getName().toStdString() << "\n";

    // Prepare
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set parameters - NO PRE-DELAY
    std::map<int, float> params;
    params[0] = 1.0f;   // Mix = 100% wet
    params[2] = 0.5f;   // Shimmer = 50%
    params[3] = 0.7f;   // Size = 70%
    params[6] = 0.0f;   // Pre-delay = 0% (DISABLED)
    engine->updateParameters(params);

    // Create impulse test buffer (stereo)
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();

    // Send impulse
    buffer.setSample(0, 0, 1.0f);  // Left channel impulse
    buffer.setSample(1, 0, 0.0f);  // Right channel silent

    // Process
    engine->process(buffer);

    // Check first 100 samples
    std::cout << "\nFirst 20 samples of output:\n";
    std::cout << "Sample | Left      | Right\n";
    std::cout << "-------|-----------|----------\n";
    for (int i = 0; i < 20; ++i) {
        float l = buffer.getSample(0, i);
        float r = buffer.getSample(1, i);
        std::cout << "  " << i << "    | " << l << " | " << r << "\n";
    }

    // Calculate RMS for first 100 samples
    double sumL = 0.0, sumR = 0.0;
    for (int i = 0; i < 100 && i < blockSize; ++i) {
        float l = buffer.getSample(0, i);
        float r = buffer.getSample(1, i);
        sumL += l * l;
        sumR += r * r;
    }

    double rmsL = std::sqrt(sumL / 100.0);
    double rmsR = std::sqrt(sumR / 100.0);

    std::cout << "\nRMS (first 100 samples):\n";
    std::cout << "  Left:  " << rmsL << "\n";
    std::cout << "  Right: " << rmsR << "\n";

    if (rmsL > 0.001 && rmsR > 0.001) {
        std::cout << "\n✓ PASS: Both channels have output\n";
        return 0;
    } else {
        std::cout << "\n✗ FAIL: One or both channels have no output\n";
        return 1;
    }
}

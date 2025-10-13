// Test PlateReverb as a baseline comparison
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include "PlateReverb.h"
#include <JuceHeader.h>

int main() {
    std::cout << "Testing PlateReverb as Baseline\n";
    std::cout << "================================\n\n";

    // Create PlateReverb directly
    auto engine = std::make_unique<PlateReverb>();

    std::cout << "Engine name: " << engine->getName().toStdString() << "\n";

    // Prepare
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set parameters
    std::map<int, float> params;
    params[0] = 1.0f;   // Mix = 100% wet
    params[1] = 0.7f;   // Size = 70%
    params[3] = 0.0f;   // Pre-delay = 0%
    engine->updateParameters(params);

    // Create impulse test buffer (stereo)
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();

    // Send impulse
    buffer.setSample(0, 0, 1.0f);  // Left channel impulse
    buffer.setSample(1, 0, 0.0f);  // Right channel silent

    // Process
    engine->process(buffer);

    // Check first 20 samples
    std::cout << "\nFirst 20 samples of output:\n";
    std::cout << "Sample | Left      | Right\n";
    std::cout << "-------|-----------|----------\n";
    for (int i = 0; i < 20; ++i) {
        float l = buffer.getSample(0, i);
        float r = buffer.getSample(1, i);
        std::cout << "  " << i << "    | " << l << " | " << r << "\n";
    }

    // Calculate RMS
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
        std::cout << "\n✓ PASS: PlateReverb produces output on both channels\n";
        return 0;
    } else {
        std::cout << "\n✗ FAIL: PlateReverb has insufficient output\n";
        return 1;
    }
}

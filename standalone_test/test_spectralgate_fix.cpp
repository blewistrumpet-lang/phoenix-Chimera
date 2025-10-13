#include <iostream>
#include <vector>
#include <cmath>
#include "../JUCE_Plugin/Source/SpectralGate_Platinum.h"

int main() {
    std::cout << "Testing SpectralGate_Platinum (Engine 48) for Bug #3 fix...\n";
    std::cout << "Testing impulse response to verify non-zero output\n\n";

    // Create engine
    SpectralGate_Platinum engine;

    // Prepare
    const double sampleRate = 44100.0;
    const int bufferSize = 512;
    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    // Set parameters to ensure processing happens
    std::map<int, float> params;
    params[0] = -40.0f;  // Threshold (dB)
    params[1] = 4.0f;    // Ratio
    params[2] = 10.0f;   // Attack (ms)
    params[3] = 100.0f;  // Release (ms)
    params[4] = 40.0f;   // Range (dB)
    params[5] = 0.0f;    // Lookahead
    params[6] = 1.0f;    // Frequency
    params[7] = 1.0f;    // Mix (FULL WET - this was causing the crash!)
    engine.updateParameters(params);

    // Create impulse signal
    juce::AudioBuffer<float> buffer(2, bufferSize);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);  // Impulse on left channel
    buffer.setSample(1, 0, 1.0f);  // Impulse on right channel

    // Process several blocks to allow FFT to fill up
    int totalSamples = 0;
    double sumSquares = 0.0;
    double maxValue = 0.0;
    int nonZeroSamples = 0;

    for (int block = 0; block < 20; ++block) {
        engine.process(buffer);

        // Analyze output
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < bufferSize; ++i) {
                float value = data[i];
                if (std::abs(value) > 0.0001f) {
                    nonZeroSamples++;
                }
                sumSquares += value * value;
                maxValue = std::max(maxValue, static_cast<double>(std::abs(value)));
            }
        }
        totalSamples += bufferSize * 2;

        // Reset buffer for next iteration
        if (block == 0) {
            buffer.clear();
        }
    }

    double rms = std::sqrt(sumSquares / totalSamples);

    // Report results
    std::cout << "Results after processing " << totalSamples << " samples:\n";
    std::cout << "  RMS level: " << rms << "\n";
    std::cout << "  Max value: " << maxValue << "\n";
    std::cout << "  Non-zero samples: " << nonZeroSamples << " ("
              << (100.0 * nonZeroSamples / totalSamples) << "%)\n\n";

    // Test verdict
    bool passed = (rms > 0.0001 || maxValue > 0.0001 || nonZeroSamples > 0);

    if (passed) {
        std::cout << "✓ TEST PASSED: Engine produces non-zero output\n";
        std::cout << "✓ Bug #3 FIXED: Early return issue resolved\n";
        return 0;
    } else {
        std::cout << "✗ TEST FAILED: Engine produces only zeros (appears crashed)\n";
        std::cout << "✗ Bug #3 NOT FIXED: Engine still appears to crash\n";
        return 1;
    }
}

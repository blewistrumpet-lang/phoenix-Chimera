/*
  Debug test for DynamicEQ - check actual output
*/

#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/DynamicEQ.h"
#include "../JUCE_Plugin/Source/EngineTypes.h"
#include "../JUCE_Plugin/Source/DspEngineUtilities.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>

int main() {
    std::cout << "DynamicEQ Debug Test\n";
    std::cout << "====================\n\n";

    // Create engine
    DynamicEQ engine;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

    engine.prepareToPlay(sampleRate, blockSize);

    // Test 1: Complete bypass (mix = 0)
    std::cout << "Test 1: Complete Bypass (Mix = 0%)\n";
    {
        std::map<int, float> params;
        params[6] = 0.0f; // Mix = 0% (completely dry)
        engine.updateParameters(params);
        engine.reset();

        // Generate test signal: 1kHz sine @ -6dBFS
        juce::AudioBuffer<float> buffer(2, 100);
        const float amplitude = std::pow(10.0f, -6.0f / 20.0f); // -6dBFS

        for (int i = 0; i < 100; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            float sample = amplitude * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        // Save input
        std::vector<float> input_samples(100);
        for (int i = 0; i < 100; ++i) {
            input_samples[i] = buffer.getSample(0, i);
        }

        // Process
        engine.process(buffer);

        // Check output
        std::cout << "First 10 samples:\n";
        std::cout << "  Index | Input      | Output     | Difference\n";
        std::cout << "  ------|------------|------------|------------\n";
        for (int i = 0; i < 10; ++i) {
            float input = input_samples[i];
            float output = buffer.getSample(0, i);
            float diff = output - input;
            std::cout << "  " << std::setw(5) << i << " | "
                     << std::setw(10) << std::fixed << std::setprecision(6) << input << " | "
                     << std::setw(10) << output << " | "
                     << std::setw(10) << diff << "\n";
        }

        // Check if it's actually bypassed
        float max_diff = 0.0f;
        for (int i = 0; i < 100; ++i) {
            float diff = std::abs(buffer.getSample(0, i) - input_samples[i]);
            max_diff = std::max(max_diff, diff);
        }
        std::cout << "\nMaximum difference: " << max_diff << "\n";
        std::cout << "Status: " << (max_diff < 0.0001f ? "PASS (True bypass)" : "FAIL (Not bypassed)") << "\n\n";
    }

    // Test 2: Neutral settings (should pass through unchanged)
    std::cout << "Test 2: Neutral Settings (no processing)\n";
    {
        std::map<int, float> params;
        params[0] = 0.5f;  // Frequency = 1kHz
        params[1] = 1.0f;  // Threshold = 0dB (very high, no compression)
        params[2] = 0.0f;  // Ratio = 1:1 (no compression)
        params[5] = 0.5f;  // Gain = 0dB
        params[6] = 1.0f;  // Mix = 100%
        engine.updateParameters(params);
        engine.reset();

        // Warmup
        juce::AudioBuffer<float> warmup(2, blockSize);
        warmup.clear();
        for (int i = 0; i < 10; ++i) {
            engine.process(warmup);
        }

        // Generate test signal
        juce::AudioBuffer<float> buffer(2, 100);
        const float amplitude = std::pow(10.0f, -6.0f / 20.0f); // -6dBFS

        for (int i = 0; i < 100; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            float sample = amplitude * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        // Save input
        std::vector<float> input_samples(100);
        for (int i = 0; i < 100; ++i) {
            input_samples[i] = buffer.getSample(0, i);
        }

        // Process
        engine.process(buffer);

        // Check output
        std::cout << "First 10 samples:\n";
        std::cout << "  Index | Input      | Output     | Gain (dB)\n";
        std::cout << "  ------|------------|------------|------------\n";
        for (int i = 0; i < 10; ++i) {
            float input = input_samples[i];
            float output = buffer.getSample(0, i);
            float gain_linear = (std::abs(input) > 0.0001f) ? output / input : 1.0f;
            float gain_db = 20.0f * std::log10(std::abs(gain_linear));
            std::cout << "  " << std::setw(5) << i << " | "
                     << std::setw(10) << std::fixed << std::setprecision(6) << input << " | "
                     << std::setw(10) << output << " | "
                     << std::setw(10) << std::setprecision(2) << gain_db << " dB\n";
        }

        // Check RMS levels
        float input_rms = 0.0f;
        float output_rms = 0.0f;
        for (int i = 0; i < 100; ++i) {
            input_rms += input_samples[i] * input_samples[i];
            output_rms += buffer.getSample(0, i) * buffer.getSample(0, i);
        }
        input_rms = std::sqrt(input_rms / 100.0f);
        output_rms = std::sqrt(output_rms / 100.0f);

        float input_db = 20.0f * std::log10(input_rms);
        float output_db = 20.0f * std::log10(output_rms);
        float gain_db = output_db - input_db;

        std::cout << "\nRMS Analysis:\n";
        std::cout << "  Input RMS:  " << input_db << " dBFS\n";
        std::cout << "  Output RMS: " << output_db << " dBFS\n";
        std::cout << "  Gain:       " << gain_db << " dB\n";
        std::cout << "Status: " << (std::abs(gain_db) < 1.0f ? "PASS (Unity gain)" : "FAIL (Gain changed)") << "\n\n";
    }

    std::cout << "\nDebug test complete!\n";

    return 0;
}

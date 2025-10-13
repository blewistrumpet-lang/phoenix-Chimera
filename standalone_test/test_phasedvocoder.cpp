#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>

// Test PhasedVocoder with correct parameter mapping
int main() {
    std::cout << "=== PhasedVocoder Parameter Test ===\n\n";

    auto engine = EngineFactory::createEngine(49);
    if (!engine) {
        std::cerr << "Failed to create Engine 49 (PhasedVocoder)\n";
        return 1;
    }

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    engine->prepareToPlay(sampleRate, blockSize);

    std::cout << "Engine Name: " << engine->getName() << "\n";
    std::cout << "Num Parameters: " << engine->getNumParameters() << "\n\n";

    // Display parameter names
    std::cout << "Parameter Mapping:\n";
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        std::cout << "  param[" << i << "] = " << engine->getParameterName(i) << "\n";
    }
    std::cout << "\n";

    // Set correct parameters for PhasedVocoder
    std::map<int, float> params;
    params[0] = 0.2f;   // TimeStretch = 1.0x (0.2 maps to 1.0x, see line 775)
    params[1] = 0.5f;   // PitchShift = 0 semitones (0.5 = center)
    params[2] = 0.0f;   // SpectralSmear = 0%
    params[6] = 1.0f;   // Mix = 100% wet
    params[7] = 0.0f;   // Freeze = OFF

    std::cout << "Setting parameters:\n";
    for (const auto& [idx, val] : params) {
        std::cout << "  param[" << idx << "] (" << engine->getParameterName(idx)
                  << ") = " << val << "\n";
    }

    engine->updateParameters(params);

    // Generate test signal: 1kHz sine wave
    // Use 4096 samples - with fix, warmup is only 2048 samples
    juce::AudioBuffer<float> testBuffer(2, 4096);

    std::cout << "\nGenerating 1kHz sine wave test signal (" << testBuffer.getNumSamples() << " samples)...\n";
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            testBuffer.setSample(ch, i, 0.5f * std::sin(phase));
        }
    }

    // Calculate input RMS
    float inputRMS = 0.0f;
    for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
        float sample = testBuffer.getSample(0, i);
        inputRMS += sample * sample;
    }
    inputRMS = std::sqrt(inputRMS / testBuffer.getNumSamples());

    std::cout << "Input RMS: " << inputRMS << "\n";

    // Process the buffer
    std::cout << "Processing...\n";
    engine->process(testBuffer);

    // Calculate output RMS
    float outputRMS = 0.0f;
    for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
        float sample = testBuffer.getSample(0, i);
        outputRMS += sample * sample;
    }
    outputRMS = std::sqrt(outputRMS / testBuffer.getNumSamples());

    std::cout << "Output RMS: " << outputRMS << "\n";

    // Calculate gain change
    float gainDB = 20.0f * std::log10(outputRMS / inputRMS);
    std::cout << "Gain Change: " << gainDB << " dB\n";

    // Check for output
    bool hasOutput = outputRMS > 0.001f;
    std::cout << "\nResult: " << (hasOutput ? "PASS - Engine produces output" : "FAIL - No output detected") << "\n";

    // Sample a few output values
    std::cout << "\nSample output values:\n";
    for (int i = 0; i < 10; ++i) {
        int idx = i * (testBuffer.getNumSamples() / 10);
        std::cout << "  [" << idx << "] = " << testBuffer.getSample(0, idx) << "\n";
    }

    return hasOutput ? 0 : 1;
}

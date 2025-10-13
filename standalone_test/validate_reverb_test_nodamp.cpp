#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>

// Simple validation: Generate impulse, capture response, save to file for manual inspection
int main(int argc, char* argv[]) {
    int engineId = 41; // Plate Reverb
    if (argc > 1) engineId = std::atoi(argv[1]);

    std::cout << "Testing Engine " << engineId << " with parameter validation\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cerr << "Failed to create engine " << engineId << "\n";
        return 1;
    }

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    engine->prepareToPlay(sampleRate, blockSize);

    // Set parameters with detailed logging
    std::map<int, float> params;
    params[0] = 1.0f;  // Mix = 100% wet
    params[1] = 0.0f;  // IR Select = 0 (Concert Hall)
    params[2] = 1.0f;  // Size = 1.0 (full length)
    params[4] = 0.0f;  // Damping = 0.0 (NO DAMPING!)

    std::cout << "Setting parameters:\n";
    for (const auto& [idx, val] : params) {
        std::cout << "  param[" << idx << "] = " << val << "\n";
    }

    engine->updateParameters(params);

    // Generate 5 second impulse response
    const int totalSamples = static_cast<int>(sampleRate * 5);
    juce::AudioBuffer<float> impulseResponse(2, totalSamples);
    impulseResponse.clear();

    // Create impulse
    impulseResponse.setSample(0, 0, 1.0f);
    impulseResponse.setSample(1, 0, 1.0f);

    std::cout << "Processing impulse response (" << totalSamples << " samples)...\n";

    // Process in blocks
    for (int start = 0; start < totalSamples; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, totalSamples - start);
        juce::AudioBuffer<float> block(impulseResponse.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);

        if (start % (static_cast<int>(sampleRate) / 10) == 0) {
            std::cout << "  " << (start / static_cast<int>(sampleRate)) << "s...\n" << std::flush;
        }
    }

    std::cout << "Done processing.\n";

    // Analyze the impulse response
    const float* left = impulseResponse.getReadPointer(0);
    const float* right = impulseResponse.getReadPointer(1);

    // Find peak
    float peakL = 0.0f, peakR = 0.0f;
    int peakIdxL = 0, peakIdxR = 0;

    for (int i = 0; i < totalSamples; ++i) {
        if (std::abs(left[i]) > peakL) {
            peakL = std::abs(left[i]);
            peakIdxL = i;
        }
        if (std::abs(right[i]) > peakR) {
            peakR = std::abs(right[i]);
            peakIdxR = i;
        }
    }

    std::cout << "\nImpulse Response Analysis:\n";
    std::cout << "  Peak Left:  " << peakL << " at sample " << peakIdxL
              << " (" << (peakIdxL / sampleRate) << "s)\n";
    std::cout << "  Peak Right: " << peakR << " at sample " << peakIdxR
              << " (" << (peakIdxR / sampleRate) << "s)\n";

    // Sample values at key points
    std::cout << "\nSample values at key times:\n";
    std::cout << "  0ms:    L=" << left[0] << " R=" << right[0] << "\n";
    std::cout << "  10ms:   L=" << left[480] << " R=" << right[480] << "\n";
    std::cout << "  100ms:  L=" << left[4800] << " R=" << right[4800] << "\n";
    std::cout << "  500ms:  L=" << left[24000] << " R=" << right[24000] << "\n";
    std::cout << "  1s:     L=" << left[48000] << " R=" << right[48000] << "\n";
    std::cout << "  2s:     L=" << left[96000] << " R=" << right[96000] << "\n";

    // Check if there's ANY significant signal
    bool hasSignal = false;
    for (int i = 0; i < totalSamples; ++i) {
        if (std::abs(left[i]) > 0.001f || std::abs(right[i]) > 0.001f) {
            hasSignal = true;
            break;
        }
    }

    std::cout << "\nHas signal above 0.001: " << (hasSignal ? "YES" : "NO") << "\n";

    // Save impulse response to CSV for inspection
    std::string filename = "impulse_engine_" + std::to_string(engineId) + ".csv";
    std::ofstream file(filename);

    file << "sample,time_s,left,right\n";
    for (int i = 0; i < std::min(totalSamples, 48000); ++i) {  // Save first 1 second
        file << i << "," << (i / sampleRate) << "," << left[i] << "," << right[i] << "\n";
    }

    file.close();
    std::cout << "\nSaved impulse response to: " << filename << "\n";
    std::cout << "(First 1 second only)\n";

    // Now test frequency response at 1kHz WITHOUT reset
    std::cout << "\n--- Testing 1kHz Frequency Response ---\n";

    juce::AudioBuffer<float> testBuffer(2, blockSize * 4);

    // Generate 1kHz sine
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            testBuffer.setSample(ch, i, 0.5f * std::sin(phase));
        }
    }

    std::cout << "Processing 1kHz sine wave (" << testBuffer.getNumSamples() << " samples)...\n" << std::flush;

    // Re-apply parameters (simulating what the test does)
    engine->updateParameters(params);

    std::cout << "Calling process()...\n" << std::flush;
    engine->process(testBuffer);
    std::cout << "Process() completed!\n";

    // Measure output
    float rms = 0.0f;
    for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
        rms += testBuffer.getSample(0, i) * testBuffer.getSample(0, i);
    }
    rms = std::sqrt(rms / testBuffer.getNumSamples());

    std::cout << "Output RMS: " << rms << "\n";
    std::cout << "Output dB:  " << 20.0f * std::log10(rms / 0.5f) << " dB\n";

    return 0;
}

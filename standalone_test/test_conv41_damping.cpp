#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>

// Test ConvolutionReverb with different damping values
int main() {
    std::cout << "Testing ConvolutionReverb damping parameter\n\n";

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    // Test with NO damping (damping = 0.0)
    {
        std::cout << "=== Test 1: NO Damping (damping = 0.0) ===\n";
        auto engine = EngineFactory::createEngine(41);
        engine->prepareToPlay(sampleRate, blockSize);

        std::map<int, float> params;
        params[0] = 1.0f;  // Mix = 100% wet
        params[2] = 0.5f;  // Size = 0.5
        params[4] = 0.0f;  // Damping = 0.0 (NO DAMPING)
        engine->updateParameters(params);

        juce::AudioBuffer<float> impulse(2, 48000); // 1 second
        impulse.clear();
        impulse.setSample(0, 0, 1.0f);
        impulse.setSample(1, 0, 1.0f);

        // Process
        for (int start = 0; start < 48000; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, 48000 - start);
            juce::AudioBuffer<float> block(impulse.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Check for reverb tail
        const float* left = impulse.getReadPointer(0);
        int nonZeroCount = 0;
        for (int i = 100; i < 48000; i++) {
            if (std::abs(left[i]) > 0.001f) nonZeroCount++;
        }
        std::cout << "Non-zero samples (after sample 100): " << nonZeroCount << "\n";
        std::cout << "Sample at 100ms: " << left[4800] << "\n";
        std::cout << "Sample at 500ms: " << left[24000] << "\n\n";
    }

    // Test with LOW damping (damping = 0.3)
    {
        std::cout << "=== Test 2: LOW Damping (damping = 0.3) ===\n";
        auto engine = EngineFactory::createEngine(41);
        engine->prepareToPlay(sampleRate, blockSize);

        std::map<int, float> params;
        params[0] = 1.0f;  // Mix = 100% wet
        params[2] = 0.5f;  // Size = 0.5
        params[4] = 0.3f;  // Damping = 0.3
        engine->updateParameters(params);

        juce::AudioBuffer<float> impulse(2, 48000);
        impulse.clear();
        impulse.setSample(0, 0, 1.0f);
        impulse.setSample(1, 0, 1.0f);

        for (int start = 0; start < 48000; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, 48000 - start);
            juce::AudioBuffer<float> block(impulse.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        const float* left = impulse.getReadPointer(0);
        int nonZeroCount = 0;
        for (int i = 100; i < 48000; i++) {
            if (std::abs(left[i]) > 0.001f) nonZeroCount++;
        }
        std::cout << "Non-zero samples (after sample 100): " << nonZeroCount << "\n";
        std::cout << "Sample at 100ms: " << left[4800] << "\n";
        std::cout << "Sample at 500ms: " << left[24000] << "\n\n";
    }

    // Test with MAX damping (damping = 1.0) - this is what the current test uses!
    {
        std::cout << "=== Test 3: MAX Damping (damping = 1.0) ===\n";
        auto engine = EngineFactory::createEngine(41);
        engine->prepareToPlay(sampleRate, blockSize);

        std::map<int, float> params;
        params[0] = 1.0f;  // Mix = 100% wet
        params[2] = 0.5f;  // Size = 0.5
        params[4] = 1.0f;  // Damping = 1.0 (MAX DAMPING!)
        engine->updateParameters(params);

        juce::AudioBuffer<float> impulse(2, 48000);
        impulse.clear();
        impulse.setSample(0, 0, 1.0f);
        impulse.setSample(1, 0, 1.0f);

        for (int start = 0; start < 48000; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, 48000 - start);
            juce::AudioBuffer<float> block(impulse.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        const float* left = impulse.getReadPointer(0);
        int nonZeroCount = 0;
        for (int i = 100; i < 48000; i++) {
            if (std::abs(left[i]) > 0.001f) nonZeroCount++;
        }
        std::cout << "Non-zero samples (after sample 100): " << nonZeroCount << "\n";
        std::cout << "Sample at 100ms: " << left[4800] << "\n";
        std::cout << "Sample at 500ms: " << left[24000] << "\n\n";
    }

    return 0;
}

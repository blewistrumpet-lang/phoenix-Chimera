// Simple engine test that can be added to existing project
#include "EngineFactory.h"
#include "EngineIDs.h"
#include <cmath>

void testSingleEngine(int engineID) {
    DBG("Testing Engine ID: " << engineID);
    
    auto engine = EngineFactory::createEngine(engineID);
    if (!engine) {
        DBG("  FAILED to create engine!");
        return;
    }
    
    DBG("  Name: " << engine->getName());
    
    // Prepare
    engine->prepareToPlay(44100.0, 512);
    
    // Create test buffer
    juce::AudioBuffer<float> buffer(2, 512);
    juce::AudioBuffer<float> original(2, 512);
    
    // Fill with test signal
    for (int ch = 0; ch < 2; ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < 512; ++i) {
            data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        }
    }
    original.makeCopyOf(buffer);
    
    // Calculate input level
    float inputLevel = 0;
    for (int ch = 0; ch < 2; ++ch) {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            inputLevel += std::abs(data[i]);
        }
    }
    inputLevel /= (2 * 512);
    
    // Set parameters for maximum effect
    std::map<int, float> params;
    params[0] = 0.8f;  // Main parameter high
    params[3] = 1.0f;  // Mix 100%
    params[5] = 1.0f;  // Alternate mix 100%
    params[6] = 1.0f;  // Alternate mix 100%
    params[7] = 1.0f;  // Alternate mix 100%
    
    engine->updateParameters(params);
    engine->process(buffer);
    
    // Calculate output level and difference
    float outputLevel = 0;
    float maxDiff = 0;
    for (int ch = 0; ch < 2; ++ch) {
        const auto* outData = buffer.getReadPointer(ch);
        const auto* inData = original.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            outputLevel += std::abs(outData[i]);
            maxDiff = std::max(maxDiff, std::abs(outData[i] - inData[i]));
        }
    }
    outputLevel /= (2 * 512);
    
    DBG("  Input level:  " << inputLevel);
    DBG("  Output level: " << outputLevel);
    DBG("  Max diff:     " << maxDiff);
    DBG("  Result: " << (maxDiff > 0.01f ? "WORKING ✅" : "NOT WORKING ❌"));
    DBG("");
}

// Add this function to be called from somewhere in the plugin
void runEngineTests() {
    DBG("=== ENGINE ISOLATION TESTS ===");
    DBG("");
    
    // Test key engines
    testSingleEngine(0);   // None
    testSingleEngine(1);   // Rodent
    testSingleEngine(2);   // Vintage
    testSingleEngine(6);   // Moog Filter
    testSingleEngine(11);  // Tape Delay
    testSingleEngine(21);  // Plate Reverb
    testSingleEngine(31);  // Spring Reverb
    
    DBG("=== TESTS COMPLETE ===");
}
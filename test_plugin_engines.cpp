#include <iostream>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineTypes.h"

int main() {
    std::cout << "\n=== Testing Engine Creation and Processing ===\n\n";
    
    // Test a simple engine - BitCrusher
    std::cout << "1. Testing BitCrusher (Engine ID 18):\n";
    auto bitcrusher = EngineFactory::createEngine(18);
    if (bitcrusher) {
        std::cout << "   ✓ Created successfully\n";
        
        // Prepare
        bitcrusher->prepareToPlay(44100, 512);
        std::cout << "   ✓ Prepared at 44100Hz, 512 samples\n";
        
        // Create test signal
        juce::AudioBuffer<float> buffer(2, 512);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(ch, i, 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f));
            }
        }
        
        float beforeRMS = buffer.getRMSLevel(0, 0, 512);
        
        // Set parameters
        std::map<int, float> params;
        params[0] = 8.0f;    // Bit depth
        params[1] = 0.5f;    // Sample rate reduction
        params[4] = 0.5f;    // Mix
        bitcrusher->updateParameters(params);
        
        // Process
        bitcrusher->process(buffer);
        
        float afterRMS = buffer.getRMSLevel(0, 0, 512);
        
        std::cout << "   Before RMS: " << beforeRMS << "\n";
        std::cout << "   After RMS:  " << afterRMS << "\n";
        
        if (std::abs(afterRMS - beforeRMS) < 0.0001f) {
            std::cout << "   ⚠️  WARNING: No audio change detected!\n";
        } else {
            std::cout << "   ✓ Audio processing working\n";
        }
    } else {
        std::cout << "   ✗ Failed to create engine\n";
    }
    
    // Test Gain Utility
    std::cout << "\n2. Testing Gain Utility (Engine ID 54):\n";
    auto gain = EngineFactory::createEngine(54);
    if (gain) {
        std::cout << "   ✓ Created successfully\n";
        
        gain->prepareToPlay(44100, 512);
        
        // Create test signal
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f); // Single impulse
        
        // Set gain to +6dB (2x)
        std::map<int, float> params;
        params[0] = 0.75f;  // Gain parameter (0.5 = unity, 1.0 = +12dB)
        gain->updateParameters(params);
        
        gain->process(buffer);
        
        float output = buffer.getSample(0, 0);
        std::cout << "   Input: 1.0, Output: " << output << "\n";
        
        if (output > 1.5f && output < 2.5f) {
            std::cout << "   ✓ Gain processing working\n";
        } else {
            std::cout << "   ⚠️  Unexpected gain output\n";
        }
    }
    
    // Test filter
    std::cout << "\n3. Testing Ladder Filter (Engine ID 9):\n";
    auto filter = EngineFactory::createEngine(9);
    if (filter) {
        std::cout << "   ✓ Created successfully\n";
        
        filter->prepareToPlay(44100, 512);
        
        // Create white noise
        juce::AudioBuffer<float> buffer(2, 512);
        juce::Random random;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 512; ++i) {
                buffer.setSample(ch, i, random.nextFloat() * 0.5f - 0.25f);
            }
        }
        
        float beforeRMS = buffer.getRMSLevel(0, 0, 512);
        
        // Set low cutoff frequency
        std::map<int, float> params;
        params[0] = 0.2f;  // Low cutoff
        params[1] = 0.5f;  // Resonance
        params[4] = 1.0f;  // Full wet
        filter->updateParameters(params);
        
        filter->process(buffer);
        
        float afterRMS = buffer.getRMSLevel(0, 0, 512);
        
        std::cout << "   Before RMS: " << beforeRMS << "\n";
        std::cout << "   After RMS:  " << afterRMS << "\n";
        
        if (afterRMS < beforeRMS * 0.8f) {
            std::cout << "   ✓ Filter attenuating high frequencies\n";
        } else {
            std::cout << "   ⚠️  Filter may not be working\n";
        }
    }
    
    std::cout << "\n=== Test Complete ===\n";
    
    return 0;
}
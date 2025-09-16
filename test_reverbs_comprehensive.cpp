// Comprehensive reverb test - tests multiple input types and ALL parameters
#include <iostream>
#include <memory>
#include <cmath>
#include <map>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
// HallReverb removed - not part of original engine list

void testReverb(const std::string& name, EngineBase* reverb) {
    std::cout << "\n=== Testing " << name << " ===" << std::endl;
    
    reverb->prepareToPlay(44100, 512);
    
    // Get parameter count
    int numParams = reverb->getNumParameters();
    std::cout << "Number of parameters: " << numParams << std::endl;
    
    // List all parameters
    for (int i = 0; i < numParams; i++) {
        std::cout << "  Param " << i << ": " << reverb->getParameterName(i).toStdString() << std::endl;
    }
    
    // Test 1: IMPULSE RESPONSE
    std::cout << "\n1. IMPULSE TEST:" << std::endl;
    {
        // Set all parameters to middle values
        std::map<int, float> params;
        for (int i = 0; i < numParams; i++) {
            params[i] = 0.5f;
        }
        params[numParams-1] = 1.0f; // Mix to 100% wet
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        std::cout << "  Processing impulse..." << std::endl;
        float totalEnergy = 0.0f;
        for (int block = 0; block < 5; block++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            float max = buffer.getMagnitude(0, 512);
            std::cout << "    Block " << block << ": RMS=" << rms << " MAX=" << max << std::endl;
            totalEnergy += rms;
            if (block == 0) buffer.clear();
        }
        std::cout << "  Total energy: " << totalEnergy << (totalEnergy > 0.01f ? " [HAS REVERB]" : " [NO REVERB!]") << std::endl;
    }
    
    // Test 2: SINE WAVE at different frequencies
    std::cout << "\n2. SINE WAVE TESTS:" << std::endl;
    float testFreqs[] = {100.0f, 440.0f, 1000.0f, 4000.0f};
    for (float freq : testFreqs) {
        reverb->reset();
        std::cout << "  " << freq << "Hz:" << std::endl;
        
        juce::AudioBuffer<float> buffer(2, 512);
        float phase = 0.0f;
        
        // Generate sine
        for (int s = 0; s < 512; s++) {
            float sample = 0.3f * std::sin(2.0f * M_PI * phase);
            buffer.setSample(0, s, sample);
            buffer.setSample(1, s, sample);
            phase += freq / 44100.0f;
            if (phase > 1.0f) phase -= 1.0f;
        }
        
        float inputRMS = buffer.getRMSLevel(0, 0, 512);
        reverb->process(buffer);
        float outputRMS = buffer.getRMSLevel(0, 0, 512);
        float maxOut = buffer.getMagnitude(0, 512);
        
        std::cout << "    Input RMS: " << inputRMS << " Output RMS: " << outputRMS 
                  << " Max: " << maxOut;
        if (maxOut > 1.5f) std::cout << " [CLIPPING/EXPLOSION!]";
        std::cout << std::endl;
    }
    
    // Test 3: WHITE NOISE
    std::cout << "\n3. WHITE NOISE TEST:" << std::endl;
    {
        reverb->reset();
        juce::AudioBuffer<float> buffer(2, 512);
        juce::Random rng;
        
        for (int s = 0; s < 512; s++) {
            float sample = rng.nextFloat() * 0.2f - 0.1f;
            buffer.setSample(0, s, sample);
            buffer.setSample(1, s, sample);
        }
        
        float inputRMS = buffer.getRMSLevel(0, 0, 512);
        reverb->process(buffer);
        float outputRMS = buffer.getRMSLevel(0, 0, 512);
        std::cout << "  Input RMS: " << inputRMS << " Output RMS: " << outputRMS << std::endl;
    }
    
    // Test 4: PARAMETER SWEEP - test if each parameter actually does something
    std::cout << "\n4. PARAMETER SWEEP TEST:" << std::endl;
    for (int paramIndex = 0; paramIndex < numParams; paramIndex++) {
        std::cout << "  Testing " << reverb->getParameterName(paramIndex).toStdString() << ":" << std::endl;
        
        // Test parameter at 0, 0.5, and 1.0
        float testValues[] = {0.0f, 0.5f, 1.0f};
        float results[3];
        
        for (int v = 0; v < 3; v++) {
            reverb->reset();
            
            // Set all params to 0.5, except the one we're testing
            std::map<int, float> params;
            for (int i = 0; i < numParams; i++) {
                params[i] = 0.5f;
            }
            params[paramIndex] = testValues[v];
            params[numParams-1] = 1.0f; // Keep mix at 100%
            reverb->updateParameters(params);
            
            // Process white noise
            juce::AudioBuffer<float> buffer(2, 512);
            juce::Random rng;
            for (int s = 0; s < 512; s++) {
                float sample = rng.nextFloat() * 0.1f - 0.05f;
                buffer.setSample(0, s, sample);
                buffer.setSample(1, s, sample);
            }
            
            reverb->process(buffer);
            results[v] = buffer.getRMSLevel(0, 0, 512);
            std::cout << "    Value " << testValues[v] << ": RMS=" << results[v] << std::endl;
        }
        
        // Check if parameter actually changes output
        float diff = std::abs(results[2] - results[0]);
        if (diff < 0.001f) {
            std::cout << "    WARNING: Parameter has NO EFFECT!" << std::endl;
        } else {
            std::cout << "    Parameter is WORKING (difference: " << diff << ")" << std::endl;
        }
    }
    
    // Test 5: STABILITY - process 100 blocks to check for explosion
    std::cout << "\n5. STABILITY TEST (100 blocks):" << std::endl;
    {
        reverb->reset();
        
        // Set aggressive parameters
        std::map<int, float> params;
        for (int i = 0; i < numParams; i++) {
            params[i] = 1.0f; // Max everything
        }
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        juce::Random rng;
        float maxLevel = 0.0f;
        bool exploded = false;
        
        for (int block = 0; block < 100; block++) {
            // Generate new noise each block
            for (int s = 0; s < 512; s++) {
                float sample = rng.nextFloat() * 0.05f - 0.025f;
                buffer.setSample(0, s, sample);
                buffer.setSample(1, s, sample);
            }
            
            reverb->process(buffer);
            float level = buffer.getMagnitude(0, 512);
            maxLevel = std::max(maxLevel, level);
            
            if (level > 2.0f) {
                std::cout << "  EXPLOSION at block " << block << " (level: " << level << ")" << std::endl;
                exploded = true;
                break;
            }
            
            if (block % 20 == 0) {
                std::cout << "  Block " << block << ": Level=" << level << std::endl;
            }
        }
        
        if (!exploded) {
            std::cout << "  STABLE - Max level: " << maxLevel << std::endl;
        }
    }
}

int main() {
    std::cout << "=== COMPREHENSIVE REVERB TESTING ===" << std::endl;
    std::cout << "Testing with multiple signal types and all parameters\n" << std::endl;
    
    // Test each reverb
    auto springReverb = std::make_unique<SpringReverb>();
    testReverb("SpringReverb", springReverb.get());
    
    auto shimmerReverb = std::make_unique<ShimmerReverb>();
    testReverb("ShimmerReverb", shimmerReverb.get());
    
    auto gatedReverb = std::make_unique<GatedReverb>();
    testReverb("GatedReverb", gatedReverb.get());
    
    // HallReverb removed - not part of original engine list
    
    std::cout << "\n=== TESTING COMPLETE ===" << std::endl;
    
    return 0;
}
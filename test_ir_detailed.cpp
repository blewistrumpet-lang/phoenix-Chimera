// Detailed impulse response test to understand what's really happening
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void testReverbIR(EngineBase* reverb, const std::string& name) {
    std::cout << "\n=== Testing " << name << " ===\n" << std::endl;
    
    reverb->prepareToPlay(44100, 512);
    
    // Test 1: Pure dry (Mix = 0)
    std::cout << "TEST 1: Pure Dry Signal (Mix = 0.0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.0f;  // Mix = 0 (100% dry)
        params[1] = 0.5f;
        params[2] = 0.5f;
        params[3] = 0.5f;
        reverb->updateParameters(params);
        
        // Send a simple test signal
        juce::AudioBuffer<float> buffer(2, 5);
        for (int i = 0; i < 5; i++) {
            buffer.setSample(0, i, 1.0f);
            buffer.setSample(1, i, 1.0f);
        }
        
        std::cout << "Input:  ";
        for (int i = 0; i < 5; i++) {
            std::cout << buffer.getSample(0, i) << " ";
        }
        std::cout << std::endl;
        
        reverb->process(buffer);
        
        std::cout << "Output: ";
        for (int i = 0; i < 5; i++) {
            std::cout << buffer.getSample(0, i) << " ";
        }
        std::cout << std::endl;
        
        // Check if it's truly dry
        bool isDry = true;
        for (int i = 0; i < 5; i++) {
            if (std::abs(buffer.getSample(0, i) - 1.0f) > 0.01f) {
                isDry = false;
                break;
            }
        }
        std::cout << "Result: " << (isDry ? "PASS - Signal unchanged" : "FAIL - Signal modified") << std::endl;
    }
    
    // Test 2: Pure wet (Mix = 1)
    std::cout << "\nTEST 2: Pure Wet Signal (Mix = 1.0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 1.0f;  // Mix = 1 (100% wet)
        params[1] = 0.5f;
        params[2] = 0.5f;
        params[3] = 0.7f;  // Larger size for more reverb
        reverb->updateParameters(params);
        
        // Send impulse
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse
        buffer.setSample(1, 0, 1.0f);
        
        std::cout << "Sending impulse..." << std::endl;
        
        // Process multiple blocks to see reverb tail
        float totalEnergy = 0.0f;
        for (int block = 0; block < 10; block++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            totalEnergy += rms;
            
            if (block < 5) {
                std::cout << "Block " << block << " RMS: " << rms;
                if (rms > 0.001f) std::cout << " <- REVERB";
                std::cout << std::endl;
            }
            
            // After first block, should be reverb tail only
            if (block == 0) {
                // Check first sample (should be mostly wet reverb, not dry impulse)
                float firstSample = buffer.getSample(0, 0);
                std::cout << "First sample after impulse: " << firstSample 
                          << " (should be near 0 for wet)" << std::endl;
                buffer.clear();
            }
        }
        
        std::cout << "Total reverb energy: " << totalEnergy << std::endl;
        std::cout << "Result: " << (totalEnergy > 0.01f ? "PASS - Reverb tail present" : 
                                     "FAIL - No reverb tail") << std::endl;
    }
    
    // Test 3: 50/50 mix
    std::cout << "\nTEST 3: 50/50 Mix" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.5f;  // Mix = 0.5 (50/50)
        params[1] = 0.5f;
        params[2] = 0.5f;
        params[3] = 0.5f;
        reverb->updateParameters(params);
        
        // Send constant signal
        juce::AudioBuffer<float> buffer(2, 10);
        for (int i = 0; i < 10; i++) {
            buffer.setSample(0, i, 0.5f);
            buffer.setSample(1, i, 0.5f);
        }
        
        reverb->process(buffer);
        
        float avg = 0.0f;
        for (int i = 0; i < 10; i++) {
            avg += buffer.getSample(0, i);
        }
        avg /= 10.0f;
        
        std::cout << "Input: 0.5 constant" << std::endl;
        std::cout << "Output average: " << avg << std::endl;
        std::cout << "Expected range: 0.2 to 0.5 (some wet reverb mixed in)" << std::endl;
        
        bool inRange = (avg >= 0.2f && avg <= 0.5f);
        std::cout << "Result: " << (inRange ? "PASS - Mix working" : "FAIL - Mix not working") << std::endl;
    }
    
    // Test 4: Check if reverb actually processes
    std::cout << "\nTEST 4: Reverb Processing Check" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 1.0f;  // 100% wet
        params[1] = 0.3f;
        params[2] = 0.5f;
        params[3] = 0.9f;  // Large size
        reverb->updateParameters(params);
        
        // Send noise burst
        juce::AudioBuffer<float> buffer(2, 512);
        juce::Random rng;
        for (int i = 0; i < 100; i++) {
            buffer.setSample(0, i, rng.nextFloat() * 0.5f - 0.25f);
            buffer.setSample(1, i, rng.nextFloat() * 0.5f - 0.25f);
        }
        
        float inputRMS = buffer.getRMSLevel(0, 0, 100);
        
        // Clear rest of buffer
        for (int i = 100; i < 512; i++) {
            buffer.setSample(0, i, 0.0f);
            buffer.setSample(1, i, 0.0f);
        }
        
        reverb->process(buffer);
        
        // Check for reverb in the tail (after sample 100)
        float tailRMS = buffer.getRMSLevel(0, 100, 412);
        
        std::cout << "Input burst RMS: " << inputRMS << std::endl;
        std::cout << "Tail RMS (should have reverb): " << tailRMS << std::endl;
        
        bool hasReverb = tailRMS > 0.001f;
        std::cout << "Result: " << (hasReverb ? "PASS - Reverb in tail" : "FAIL - No reverb in tail") << std::endl;
    }
}

int main() {
    std::cout << "DETAILED IMPULSE RESPONSE INVESTIGATION" << std::endl;
    std::cout << "========================================" << std::endl;
    
    auto spring = std::make_unique<SpringReverb>();
    testReverbIR(spring.get(), "SpringReverb");
    
    auto plate = std::make_unique<PlateReverb>();
    testReverbIR(plate.get(), "PlateReverb");
    
    auto conv = std::make_unique<ConvolutionReverb>();
    testReverbIR(conv.get(), "ConvolutionReverb");
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "CONCLUSION:" << std::endl;
    std::cout << "Check results above to see if reverbs are actually working" << std::endl;
    
    return 0;
}
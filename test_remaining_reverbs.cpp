// Test ShimmerReverb and GatedReverb with correct parameters
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"

void testShimmerReverb() {
    std::cout << "\n=== Testing ShimmerReverb ===\n" << std::endl;
    
    auto reverb = std::make_unique<ShimmerReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Need to check ShimmerReverb parameter order
    std::cout << "Parameters:" << std::endl;
    for (int i = 0; i < reverb->getNumParameters(); i++) {
        std::cout << "  " << i << ": " << reverb->getParameterName(i) << std::endl;
    }
    
    // Test 1: Pure dry (Mix = 0)
    std::cout << "\nTEST 1: Pure Dry Signal (Mix = 0.0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.0f;  // Assuming Mix is at index 0
        params[1] = 0.0f;  // PitchShift
        params[2] = 0.5f;  // Shimmer
        params[3] = 0.5f;  // Feedback
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 5);
        for (int i = 0; i < 5; i++) {
            buffer.setSample(0, i, 1.0f);
            buffer.setSample(1, i, 1.0f);
        }
        
        reverb->process(buffer);
        
        bool isDry = true;
        for (int i = 0; i < 5; i++) {
            if (std::abs(buffer.getSample(0, i) - 1.0f) > 0.01f) {
                isDry = false;
                break;
            }
        }
        std::cout << "First sample: " << buffer.getSample(0, 0) 
                  << " (expected 1.0)" << std::endl;
        std::cout << "Result: " << (isDry ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
    
    // Test 2: Pure wet (Mix = 1)
    std::cout << "\nTEST 2: Pure Wet Signal (Mix = 1.0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 1.0f;  // Mix
        params[1] = 0.5f;  // PitchShift (octave up)
        params[2] = 0.7f;  // Shimmer amount
        params[3] = 0.8f;  // High feedback
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse
        buffer.setSample(1, 0, 1.0f);
        
        float totalEnergy = 0.0f;
        for (int block = 0; block < 10; block++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            totalEnergy += rms;
            
            if (block < 3) {
                std::cout << "Block " << block << " RMS: " << rms;
                if (rms > 0.001f) std::cout << " <- SHIMMER";
                std::cout << std::endl;
            }
            
            if (block == 0) buffer.clear();
        }
        
        std::cout << "Total energy: " << totalEnergy << std::endl;
        std::cout << "Result: " << (totalEnergy > 0.01f ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
    
    // Test 3: Check if mix parameter actually changes
    std::cout << "\nTEST 3: Mix Parameter Check" << std::endl;
    {
        reverb->reset();
        
        // Generate test signal
        juce::AudioBuffer<float> testBuffer(2, 100);
        for (int i = 0; i < 100; i++) {
            testBuffer.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / 44100.0f));
            testBuffer.setSample(1, i, std::sin(2.0f * M_PI * 440.0f * i / 44100.0f));
        }
        
        // Test with Mix = 0.0
        std::map<int, float> params;
        params[0] = 0.0f;
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer1(testBuffer);
        reverb->process(buffer1);
        float rms1 = buffer1.getRMSLevel(0, 0, 100);
        
        // Test with Mix = 1.0
        reverb->reset();
        params[0] = 1.0f;
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer2(testBuffer);
        reverb->process(buffer2);
        float rms2 = buffer2.getRMSLevel(0, 0, 100);
        
        std::cout << "RMS with Mix=0.0: " << rms1 << std::endl;
        std::cout << "RMS with Mix=1.0: " << rms2 << std::endl;
        std::cout << "Result: " << (std::abs(rms1 - rms2) > 0.01f ? "PASS ✓ - Mix works" : 
                                     "FAIL ✗ - Mix stuck") << std::endl;
    }
}

void testGatedReverb() {
    std::cout << "\n=== Testing GatedReverb ===\n" << std::endl;
    
    auto reverb = std::make_unique<GatedReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Check parameter order
    std::cout << "Parameters:" << std::endl;
    for (int i = 0; i < reverb->getNumParameters(); i++) {
        std::cout << "  " << i << ": " << reverb->getParameterName(i) << std::endl;
    }
    
    // Test 1: Gate fully open (Threshold = 0)
    std::cout << "\nTEST 1: Gate Fully Open (Threshold = 0.0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.0f;  // Threshold = 0 (gate always open)
        params[1] = 0.5f;  // Hold
        params[2] = 0.5f;  // Release
        params[3] = 0.7f;  // Mix (high wet)
        params[4] = 0.7f;  // Size
        params[5] = 0.3f;  // Damping
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse
        buffer.setSample(1, 0, 1.0f);
        
        float totalEnergy = 0.0f;
        for (int block = 0; block < 5; block++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            totalEnergy += rms;
            
            std::cout << "Block " << block << " RMS: " << rms;
            if (rms > 0.001f) std::cout << " <- GATED REVERB";
            std::cout << std::endl;
            
            if (block == 0) buffer.clear();
        }
        
        std::cout << "Total energy: " << totalEnergy << std::endl;
        std::cout << "Result: " << (totalEnergy > 0.01f ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
    
    // Test 2: Gate closed (high threshold, quiet signal)
    std::cout << "\nTEST 2: Gate Closed (High Threshold)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.9f;  // High threshold
        params[1] = 0.5f;  // Hold
        params[2] = 0.5f;  // Release
        params[3] = 1.0f;  // Mix = 100% wet
        params[4] = 0.7f;  // Size
        params[5] = 0.3f;  // Damping
        reverb->updateParameters(params);
        
        // Send quiet signal below threshold
        juce::AudioBuffer<float> buffer(2, 100);
        for (int i = 0; i < 100; i++) {
            buffer.setSample(0, i, 0.01f);  // Very quiet
            buffer.setSample(1, i, 0.01f);
        }
        
        reverb->process(buffer);
        
        float rms = buffer.getRMSLevel(0, 0, 100);
        std::cout << "Output RMS: " << rms << " (should be near 0)" << std::endl;
        std::cout << "Result: " << (rms < 0.001f ? "PASS ✓ - Gate closed" : 
                                     "FAIL ✗ - Gate not working") << std::endl;
    }
    
    // Test 3: Gate opening with loud signal
    std::cout << "\nTEST 3: Gate Opening (Above Threshold)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.3f;  // Medium threshold
        params[1] = 0.1f;  // Short hold
        params[2] = 0.2f;  // Short release
        params[3] = 1.0f;  // Mix = 100% wet
        params[4] = 0.5f;  // Size
        params[5] = 0.3f;  // Damping
        reverb->updateParameters(params);
        
        // Send loud burst
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        for (int i = 0; i < 50; i++) {
            buffer.setSample(0, i, 0.8f);  // Loud burst
            buffer.setSample(1, i, 0.8f);
        }
        
        reverb->process(buffer);
        
        // Check if reverb tail exists after burst
        float tailRMS = buffer.getRMSLevel(0, 100, 400);
        std::cout << "Tail RMS after burst: " << tailRMS << std::endl;
        std::cout << "Result: " << (tailRMS > 0.001f ? "PASS ✓ - Gate opened" : 
                                     "FAIL ✗ - No reverb tail") << std::endl;
    }
}

int main() {
    std::cout << "TESTING REMAINING REVERBS" << std::endl;
    std::cout << "==========================" << std::endl;
    
    testShimmerReverb();
    testGatedReverb();
    
    std::cout << "\n==========================" << std::endl;
    std::cout << "Tests complete. Check results above." << std::endl;
    
    return 0;
}
// Corrected reverb test using proper parameter indices
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void testSpringReverb() {
    std::cout << "\n=== Testing SpringReverb ===\n" << std::endl;
    
    auto reverb = std::make_unique<SpringReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // SpringReverb parameters: 0=Tension, 1=Damping, 2=Decay, 3=Mix
    
    // Test 1: Pure dry (Mix = 0)
    std::cout << "TEST 1: Pure Dry Signal (Mix = 0.0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[3] = 0.0f;  // Mix at index 3
        params[0] = 0.5f;  // Tension
        params[1] = 0.5f;  // Damping
        params[2] = 0.5f;  // Decay
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
        params[3] = 1.0f;  // Mix at index 3
        params[0] = 0.5f;  // Tension
        params[1] = 0.3f;  // Low damping
        params[2] = 0.8f;  // High decay
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
                if (rms > 0.001f) std::cout << " <- REVERB";
                std::cout << std::endl;
            }
            
            if (block == 0) buffer.clear();
        }
        
        std::cout << "Total energy: " << totalEnergy << std::endl;
        std::cout << "Result: " << (totalEnergy > 0.01f ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
}

void testPlateReverb() {
    std::cout << "\n=== Testing PlateReverb ===\n" << std::endl;
    
    auto reverb = std::make_unique<PlateReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // PlateReverb parameters: 0=Size, 1=Damping, 2=Predelay, 3=Mix
    
    // Test 1: Pure dry (Mix = 0)
    std::cout << "TEST 1: Pure Dry Signal (Mix = 0.0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[3] = 0.0f;  // Mix at index 3
        params[0] = 0.5f;  // Size
        params[1] = 0.5f;  // Damping
        params[2] = 0.0f;  // No predelay
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
        params[3] = 1.0f;  // Mix at index 3
        params[0] = 0.8f;  // Large size
        params[1] = 0.2f;  // Low damping
        params[2] = 0.0f;  // No predelay
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
                if (rms > 0.001f) std::cout << " <- REVERB";
                std::cout << std::endl;
            }
            
            if (block == 0) buffer.clear();
        }
        
        std::cout << "Total energy: " << totalEnergy << std::endl;
        std::cout << "Result: " << (totalEnergy > 0.01f ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
}

void testConvolutionReverb() {
    std::cout << "\n=== Testing ConvolutionReverb ===\n" << std::endl;
    
    auto reverb = std::make_unique<ConvolutionReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // ConvolutionReverb parameters: 0=Mix, 1=Predelay, 2=Damping, 3=Size
    
    // Test 1: Pure dry (Mix = 0)
    std::cout << "TEST 1: Pure Dry Signal (Mix = 0.0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.0f;  // Mix at index 0
        params[1] = 0.0f;  // No predelay
        params[2] = 0.5f;  // Damping
        params[3] = 0.5f;  // Size
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
        params[0] = 1.0f;  // Mix at index 0
        params[1] = 0.0f;  // No predelay
        params[2] = 0.2f;  // Low damping
        params[3] = 0.8f;  // Large size
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
                if (rms > 0.001f) std::cout << " <- REVERB";
                std::cout << std::endl;
            }
            
            if (block == 0) buffer.clear();
        }
        
        std::cout << "Total energy: " << totalEnergy << std::endl;
        std::cout << "Result: " << (totalEnergy > 0.01f ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
}

int main() {
    std::cout << "CORRECTED REVERB TESTS WITH PROPER PARAMETER INDICES" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    testSpringReverb();
    testPlateReverb();
    testConvolutionReverb();
    
    std::cout << "\n=====================================================" << std::endl;
    std::cout << "Tests complete. Check results above." << std::endl;
    
    return 0;
}
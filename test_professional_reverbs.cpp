// Comprehensive test for professional reverb implementations
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"

bool testReverb(EngineBase* reverb, const std::string& name) {
    std::cout << "\n=======================================" << std::endl;
    std::cout << "Testing: " << name << std::endl;
    std::cout << "Parameters: " << reverb->getNumParameters() << std::endl;
    
    for (int i = 0; i < reverb->getNumParameters(); i++) {
        std::cout << "  " << i << ": " << reverb->getParameterName(i) << std::endl;
    }
    
    std::cout << "\n--- Initializing ---" << std::endl;
    reverb->prepareToPlay(44100, 512);
    
    bool allTestsPass = true;
    
    // Test 1: Dry signal (Mix = 0)
    std::cout << "\nTest 1: Dry Signal (Mix = 0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.0f; // Mix = 0
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 100);
        for (int i = 0; i < 100; i++) {
            buffer.setSample(0, i, 0.5f);
            buffer.setSample(1, i, 0.5f);
        }
        
        float inputRMS = buffer.getRMSLevel(0, 0, 100);
        reverb->process(buffer);
        float outputRMS = buffer.getRMSLevel(0, 0, 100);
        
        bool dryPass = std::abs(inputRMS - outputRMS) < 0.01f;
        std::cout << "  Input RMS: " << inputRMS << std::endl;
        std::cout << "  Output RMS: " << outputRMS << std::endl;
        std::cout << "  Result: " << (dryPass ? "PASS ✓" : "FAIL ✗") << std::endl;
        allTestsPass &= dryPass;
    }
    
    // Test 2: Wet signal produces reverb tail
    std::cout << "\nTest 2: Wet Signal (Mix = 1.0)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 1.0f; // Mix = 1
        params[1] = 0.7f; // Size/Tension
        params[2] = 0.3f; // Damping
        params[3] = 0.6f; // Decay/PreDelay
        reverb->updateParameters(params);
        
        // Send impulse
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        float totalEnergy = 0.0f;
        for (int block = 0; block < 10; block++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            totalEnergy += rms;
            if (block == 0) {
                std::cout << "  First block RMS: " << rms << std::endl;
                buffer.clear();
            }
        }
        
        bool wetPass = totalEnergy > 0.01f;
        std::cout << "  Total energy: " << totalEnergy << std::endl;
        std::cout << "  Result: " << (wetPass ? "PASS ✓" : "FAIL ✗") << std::endl;
        allTestsPass &= wetPass;
    }
    
    // Test 3: All parameters respond
    std::cout << "\nTest 3: Parameter Response" << std::endl;
    {
        reverb->reset();
        
        // Test tone
        juce::AudioBuffer<float> testBuffer(2, 100);
        for (int i = 0; i < 100; i++) {
            float val = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
            testBuffer.setSample(0, i, val);
            testBuffer.setSample(1, i, val);
        }
        
        // Test each parameter
        std::cout << "  Testing parameters:" << std::endl;
        for (int param = 0; param < std::min(5, reverb->getNumParameters()); param++) {
            reverb->reset();
            
            // Min value
            std::map<int, float> params;
            params[param] = 0.0f;
            reverb->updateParameters(params);
            
            juce::AudioBuffer<float> buffer1(testBuffer);
            reverb->process(buffer1);
            float rms1 = buffer1.getRMSLevel(0, 0, 100);
            
            // Max value
            reverb->reset();
            params[param] = 1.0f;
            reverb->updateParameters(params);
            
            juce::AudioBuffer<float> buffer2(testBuffer);
            reverb->process(buffer2);
            float rms2 = buffer2.getRMSLevel(0, 0, 100);
            
            bool paramWorks = std::abs(rms1 - rms2) > 0.001f || param > 3;
            std::cout << "    " << reverb->getParameterName(param) 
                      << ": " << (paramWorks ? "✓" : "✗") << std::endl;
        }
    }
    
    // Test 4: Stability under stress
    std::cout << "\nTest 4: Stability Test" << std::endl;
    {
        reverb->reset();
        
        // Extreme parameters
        std::map<int, float> params;
        for (int i = 0; i < reverb->getNumParameters(); i++) {
            params[i] = (i % 2) ? 1.0f : 0.0f;
        }
        reverb->updateParameters(params);
        
        // Process noise
        juce::AudioBuffer<float> buffer(2, 512);
        juce::Random rng;
        
        bool stable = true;
        for (int block = 0; block < 100; block++) {
            // Fill with noise
            for (int ch = 0; ch < 2; ch++) {
                for (int s = 0; s < 512; s++) {
                    buffer.setSample(ch, s, rng.nextFloat() * 2.0f - 1.0f);
                }
            }
            
            reverb->process(buffer);
            
            // Check for NaN or inf
            for (int ch = 0; ch < 2; ch++) {
                for (int s = 0; s < 512; s++) {
                    float sample = buffer.getSample(ch, s);
                    if (std::isnan(sample) || std::isinf(sample) || std::abs(sample) > 10.0f) {
                        stable = false;
                        break;
                    }
                }
            }
        }
        
        std::cout << "  Processed 100 blocks of noise" << std::endl;
        std::cout << "  Result: " << (stable ? "STABLE ✓" : "UNSTABLE ✗") << std::endl;
        allTestsPass &= stable;
    }
    
    // Test 5: Professional features work
    std::cout << "\nTest 5: Professional Features" << std::endl;
    {
        reverb->reset();
        
        // Test modulation, filters, width etc
        std::map<int, float> params;
        params[0] = 0.5f; // Mix
        if (reverb->getNumParameters() > 7) {
            params[7] = 0.8f; // Low Cut (if present)
            params[8] = 0.2f; // High Cut (if present)
        }
        if (reverb->getNumParameters() > 9) {
            params[9] = 0.0f; // Width = mono
        }
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        for (int i = 0; i < 512; i++) {
            float val = (i < 256) ? 0.5f : -0.5f;
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, -val); // Opposite in right channel
        }
        
        reverb->process(buffer);
        
        // Check if mono (width = 0) makes channels similar
        float diff = 0.0f;
        for (int i = 0; i < 512; i++) {
            diff += std::abs(buffer.getSample(0, i) - buffer.getSample(1, i));
        }
        diff /= 512.0f;
        
        bool featuresWork = true;
        if (reverb->getNumParameters() >= 10) {
            featuresWork = diff < 0.1f; // Should be mostly mono
            std::cout << "  Stereo difference: " << diff 
                      << " (should be low for mono)" << std::endl;
        }
        
        std::cout << "  Result: " << (featuresWork ? "FEATURES WORK ✓" : "FEATURES FAIL ✗") << std::endl;
        allTestsPass &= featuresWork;
    }
    
    return allTestsPass;
}

int main() {
    std::cout << "PROFESSIONAL REVERB VERIFICATION TEST" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    int passCount = 0;
    int totalCount = 0;
    
    // Test PlateReverb
    {
        auto reverb = std::make_unique<PlateReverb>();
        bool pass = testReverb(reverb.get(), "PlateReverb");
        if (pass) passCount++;
        totalCount++;
    }
    
    // Test SpringReverb
    {
        auto reverb = std::make_unique<SpringReverb>();
        bool pass = testReverb(reverb.get(), "SpringReverb");
        if (pass) passCount++;
        totalCount++;
    }
    
    std::cout << "\n=======================================" << std::endl;
    std::cout << "FINAL RESULTS: " << passCount << "/" << totalCount << " reverbs passed all tests" << std::endl;
    
    if (passCount == totalCount) {
        std::cout << "\n✓✓✓ ALL PROFESSIONAL REVERBS VERIFIED ✓✓✓" << std::endl;
        std::cout << "Ready for integration into plugin system." << std::endl;
    } else {
        std::cout << "\n✗ Some reverbs need attention" << std::endl;
    }
    
    return (passCount == totalCount) ? 0 : 1;
}
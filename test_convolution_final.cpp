// Test for ConvolutionReverb
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void printTestHeader(const std::string& testName) {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << testName << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

int main() {
    std::cout << "CONVOLUTIONREVERB VERIFICATION TEST" << std::endl;
    std::cout << "Testing convolution-based reverb engine" << std::endl;
    
    auto reverb = std::make_unique<ConvolutionReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // TEST 1: BASIC OUTPUT
    printTestHeader("TEST 1: BASIC OUTPUT");
    {
        std::cout << "Testing if reverb produces output..." << std::endl;
        
        reverb->reset();
        
        // Set parameters for audible reverb
        std::map<int, float> params;
        params[0] = 1.0f;  // Mix: 100% wet
        params[1] = 0.0f;  // PreDelay
        params[2] = 0.3f;  // Damping
        params[3] = 0.7f;  // Size
        params[4] = 1.0f;  // Width
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse
        buffer.setSample(1, 0, 1.0f);
        
        std::cout << "Processing impulse..." << std::endl;
        float totalEnergy = 0.0f;
        
        for (int block = 0; block < 10; block++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            totalEnergy += rms;
            
            if (block < 5) {
                std::cout << "Block " << block << ": RMS=" << std::fixed 
                          << std::setprecision(6) << rms;
                if (rms > 0.001f) std::cout << " <- REVERB OUTPUT";
                std::cout << std::endl;
            }
            
            if (block == 0) buffer.clear();
        }
        
        std::cout << "Total energy: " << totalEnergy << std::endl;
        std::cout << "Result: " << (totalEnergy > 0.01f ? "PRODUCES OUTPUT ✓" : "NO OUTPUT ✗") << std::endl;
    }
    
    // TEST 2: MIX PARAMETER
    printTestHeader("TEST 2: MIX PARAMETER");
    {
        std::cout << "Testing dry/wet mix..." << std::endl;
        
        float mixValues[3] = {0.0f, 0.5f, 1.0f};
        float outputs[3];
        
        for (int i = 0; i < 3; i++) {
            reverb->reset();
            
            std::map<int, float> params;
            params[0] = mixValues[i];  // Mix
            params[1] = 0.0f;  // PreDelay
            params[2] = 0.3f;  // Damping
            params[3] = 0.5f;  // Size
            params[4] = 1.0f;  // Width
            reverb->updateParameters(params);
            
            juce::AudioBuffer<float> buffer(2, 1);
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);
            
            reverb->process(buffer);
            outputs[i] = buffer.getSample(0, 0);
            
            std::cout << "Mix=" << mixValues[i] << " -> Output=" << outputs[i] << std::endl;
        }
        
        bool mixWorks = (std::abs(outputs[0] - 1.0f) < 0.01f) && // Dry
                        (std::abs(outputs[2]) < 0.1f) && // Wet (should be near zero on first sample)
                        (outputs[1] > 0.4f && outputs[1] < 0.6f); // 50/50
        std::cout << "Result: " << (mixWorks ? "MIX WORKS ✓" : "MIX BROKEN ✗") << std::endl;
    }
    
    // TEST 3: SIZE PARAMETER
    printTestHeader("TEST 3: SIZE PARAMETER");
    {
        std::cout << "Testing if size affects reverb tail..." << std::endl;
        
        float sizeEnergies[2] = {0.0f, 0.0f};
        float sizes[2] = {0.1f, 0.9f};  // Small vs large
        
        for (int test = 0; test < 2; test++) {
            reverb->reset();
            
            std::map<int, float> params;
            params[0] = 1.0f;  // Mix: 100% wet
            params[1] = 0.0f;  // PreDelay
            params[2] = 0.3f;  // Damping
            params[3] = sizes[test];  // Size
            params[4] = 1.0f;  // Width
            reverb->updateParameters(params);
            
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);
            
            for (int block = 0; block < 10; block++) {
                reverb->process(buffer);
                if (block > 0) {
                    sizeEnergies[test] += buffer.getRMSLevel(0, 0, 512);
                }
                if (block == 0) buffer.clear();
            }
            
            std::cout << "Size=" << sizes[test] 
                      << " -> Total energy: " << sizeEnergies[test] << std::endl;
        }
        
        float ratio = sizeEnergies[1] / (sizeEnergies[0] + 0.0001f);
        std::cout << "Energy ratio (large/small): " << ratio << std::endl;
        std::cout << "Result: " << (ratio > 1.2f ? "SIZE AFFECTS REVERB ✓" : "NO EFFECT ✗") << std::endl;
    }
    
    // TEST 4: CONTINUOUS SIGNAL
    printTestHeader("TEST 4: CONTINUOUS SIGNAL");
    {
        std::cout << "Testing with continuous sine wave..." << std::endl;
        
        reverb->reset();
        
        std::map<int, float> params;
        params[0] = 0.7f;  // Mix: 70%
        params[1] = 0.0f;  // PreDelay
        params[2] = 0.3f;  // Damping
        params[3] = 0.7f;  // Size
        params[4] = 1.0f;  // Width
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        
        // Generate and process sine wave
        for (int block = 0; block < 5; block++) {
            for (int s = 0; s < 512; s++) {
                float sample = 0.3f * std::sin(2.0f * M_PI * s * 440.0f / 44100.0f);
                buffer.setSample(0, s, sample);
                buffer.setSample(1, s, sample);
            }
            
            reverb->process(buffer);
        }
        
        float finalRMS = buffer.getRMSLevel(0, 0, 512);
        std::cout << "Final RMS: " << finalRMS << std::endl;
        std::cout << "Result: " << (finalRMS > 0.1f ? "PROCESSES CONTINUOUS SIGNAL ✓" : "BROKEN ✗") << std::endl;
    }
    
    // TEST 5: PARAMETER NAMES
    printTestHeader("TEST 5: PARAMETER INFO");
    {
        std::cout << "Checking parameter names..." << std::endl;
        int numParams = reverb->getNumParameters();
        std::cout << "Number of parameters: " << numParams << std::endl;
        
        for (int i = 0; i < std::min(8, numParams); i++) {
            std::cout << "Param " << i << ": " << reverb->getParameterName(i) << std::endl;
        }
    }
    
    // FINAL SUMMARY
    printTestHeader("FINAL VERIFICATION SUMMARY");
    std::cout << "ConvolutionReverb test results" << std::endl;
    std::cout << "Check output above for pass/fail status" << std::endl;
    
    return 0;
}
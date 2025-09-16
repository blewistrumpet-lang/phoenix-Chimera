// Debug test for GatedReverb to identify gate issues
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/GatedReverb.h"

int main() {
    std::cout << "GATEDREVERB DEBUG TEST" << std::endl;
    std::cout << "Identifying why gate doesn't open on loud signals" << std::endl;
    
    auto reverb = std::make_unique<GatedReverb>();
    reverb->prepareToPlay(44100, 512);
    
    std::cout << "\n=== TEST 1: Very Low Threshold ===" << std::endl;
    {
        reverb->reset();
        
        // Set extremely low threshold
        std::map<int, float> params;
        params[0] = 0.01f;  // VERY low threshold
        params[1] = 0.5f;   // Medium hold
        params[2] = 0.7f;   // Room size
        params[3] = 0.3f;   // Damping
        params[4] = 1.0f;   // Mix: 100% wet
        reverb->updateParameters(params);
        
        // Send loud signal
        juce::AudioBuffer<float> buffer(2, 512);
        for (int s = 0; s < 512; s++) {
            float sample = 0.8f * std::sin(2.0f * M_PI * s * 440.0f / 44100.0f);
            buffer.setSample(0, s, sample);
            buffer.setSample(1, s, sample);
        }
        
        float inputRMS = buffer.getRMSLevel(0, 0, 512);
        std::cout << "Input RMS: " << inputRMS << std::endl;
        
        reverb->process(buffer);
        float outputRMS = buffer.getRMSLevel(0, 0, 512);
        std::cout << "Output RMS after loud signal: " << outputRMS << std::endl;
        
        // Process a few more blocks to see if gate opens with delay
        for (int i = 0; i < 5; i++) {
            reverb->process(buffer);
            outputRMS = buffer.getRMSLevel(0, 0, 512);
            std::cout << "Block " << (i+2) << " RMS: " << outputRMS << std::endl;
        }
        
        std::cout << "Result: " << (outputRMS > 0.1f ? "Gate opens eventually" : "Gate never opens") << std::endl;
    }
    
    std::cout << "\n=== TEST 2: Check Initial Gate State ===" << std::endl;
    {
        reverb->reset();
        
        // Force gate open by setting threshold to 0
        std::map<int, float> params;
        params[0] = 0.0f;   // Zero threshold (should always be open)
        params[1] = 0.5f;   
        params[2] = 0.7f;   
        params[3] = 0.3f;   
        params[4] = 1.0f;   // 100% wet
        reverb->updateParameters(params);
        
        // Send signal
        juce::AudioBuffer<float> buffer(2, 512);
        for (int s = 0; s < 512; s++) {
            float sample = 0.5f * std::sin(2.0f * M_PI * s * 440.0f / 44100.0f);
            buffer.setSample(0, s, sample);
            buffer.setSample(1, s, sample);
        }
        
        reverb->process(buffer);
        float outputRMS = buffer.getRMSLevel(0, 0, 512);
        std::cout << "Output with zero threshold: " << outputRMS << std::endl;
        std::cout << "Result: " << (outputRMS > 0.1f ? "Zero threshold works" : "Gate broken even at zero threshold") << std::endl;
    }
    
    std::cout << "\n=== TEST 3: Check Reverb Without Gate ===" << std::endl;
    {
        // Test if reverb itself produces output when gate is forced open
        reverb->reset();
        
        std::map<int, float> params;
        params[0] = 0.0f;   // Zero threshold
        params[1] = 1.0f;   // Max hold (keep open)
        params[2] = 0.9f;   // Large room
        params[3] = 0.1f;   // Low damping
        params[4] = 1.0f;   // 100% wet
        reverb->updateParameters(params);
        
        // Send impulse
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        std::cout << "Sending impulse..." << std::endl;
        
        float totalEnergy = 0.0f;
        for (int i = 0; i < 10; i++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            totalEnergy += rms;
            
            if (i < 5) {
                std::cout << "Block " << i << ": RMS=" << rms;
                if (rms > 0.001f) std::cout << " <- OUTPUT DETECTED";
                std::cout << std::endl;
            }
            
            if (i == 0) buffer.clear();  // Clear after first block
        }
        
        std::cout << "Total reverb energy: " << totalEnergy << std::endl;
        std::cout << "Result: " << (totalEnergy > 0.01f ? "Reverb works when gate is open" : "Reverb itself is broken") << std::endl;
    }
    
    std::cout << "\n=== TEST 4: Gradual Signal Increase ===" << std::endl;
    {
        reverb->reset();
        
        std::map<int, float> params;
        params[0] = 0.2f;   // Low-medium threshold
        params[1] = 0.3f;   
        params[2] = 0.7f;   
        params[3] = 0.3f;   
        params[4] = 1.0f;   // 100% wet
        reverb->updateParameters(params);
        
        // Gradually increase signal level
        for (int level = 1; level <= 10; level++) {
            juce::AudioBuffer<float> buffer(2, 512);
            float amplitude = level * 0.1f;
            
            for (int s = 0; s < 512; s++) {
                float sample = amplitude * std::sin(2.0f * M_PI * s * 440.0f / 44100.0f);
                buffer.setSample(0, s, sample);
                buffer.setSample(1, s, sample);
            }
            
            reverb->process(buffer);
            float outputRMS = buffer.getRMSLevel(0, 0, 512);
            
            std::cout << "Input amplitude " << amplitude << " -> Output RMS: " << outputRMS;
            if (outputRMS > 0.01f) {
                std::cout << " <- GATE OPENED!";
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "\n=== DIAGNOSIS ===" << std::endl;
    std::cout << "Checking what's broken:" << std::endl;
    std::cout << "1. Gate mechanism" << std::endl;
    std::cout << "2. Envelope follower" << std::endl;
    std::cout << "3. Threshold scaling" << std::endl;
    std::cout << "4. Initial gate state" << std::endl;
    
    return 0;
}
// Diagnostic test to understand why only PlateReverb works
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 512;

template<typename ReverbType>
void diagnoseReverb(const std::string& name, ReverbType& reverb) {
    std::cout << "\n=== Diagnosing " << name << " ===" << std::endl;
    
    // Initialize with typical settings
    reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Set to 100% wet with reasonable reverb settings
    std::map<int, float> params;
    
    if (name == "PlateReverb") {
        params[0] = 0.7f;  // Size
        params[1] = 0.3f;  // Damping  
        params[2] = 1.0f;  // Mix (100% wet)
    } else if (name == "SpringReverb") {
        params[0] = 0.5f;  // Tension
        params[1] = 0.3f;  // Damping
        params[2] = 0.5f;  // Springs
        params[3] = 0.7f;  // Diffusion
        params[4] = 0.5f;  // Brightness
        params[5] = 0.2f;  // Drip
        params[6] = 1.0f;  // Mix (100% wet)
    } else if (name == "ConvolutionReverb") {
        params[0] = 1.0f;  // Mix (100% wet)
    } else if (name == "ShimmerReverb") {
        params[0] = 0.7f;  // Size
        params[1] = 0.3f;  // Damping
        params[2] = 0.5f;  // Shimmer
        params[3] = 0.5f;  // Pitch
        params[4] = 0.3f;  // Modulation
        params[5] = 0.2f;  // Low cut
        params[6] = 0.8f;  // High cut
        params[7] = 0.0f;  // Freeze off
        params[8] = 1.0f;  // Mix (100% wet)
    } else if (name == "GatedReverb") {
        params[0] = 0.7f;  // Size
        params[1] = 0.5f;  // Gate time
        params[2] = 0.05f; // Pre-delay
        params[3] = 0.3f;  // Damping
        params[4] = 0.7f;  // Diffusion
        params[5] = 0.3f;  // Hold
        params[6] = 1.0f;  // Mix (100% wet)
    }
    
    reverb.updateParameters(params);
    reverb.reset();
    
    // Test 1: Simple sine wave burst
    std::cout << "\n1. Testing with sine wave burst (440Hz, 100ms):" << std::endl;
    {
        juce::AudioBuffer<float> buffer(2, BUFFER_SIZE * 4);
        
        // Generate 100ms of 440Hz sine
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 4410; ++i) { // 100ms at 44100Hz
                if (i < buffer.getNumSamples()) {
                    float sample = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
                    // Apply fade out
                    if (i > 3500) {
                        sample *= (4410.0f - i) / 910.0f;
                    }
                    buffer.setSample(ch, i, sample);
                }
            }
        }
        
        // Process blocks
        float totalEnergy = 0.0f;
        for (int block = 0; block < 4; ++block) {
            juce::AudioBuffer<float> subBuffer(
                buffer.getArrayOfWritePointers(),
                buffer.getNumChannels(),
                block * BUFFER_SIZE,
                BUFFER_SIZE
            );
            reverb.process(subBuffer);
            
            // Measure energy
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                totalEnergy += std::abs(subBuffer.getSample(0, i));
            }
        }
        
        std::cout << "  Total output energy: " << totalEnergy << std::endl;
        
        // Now process empty buffers to check for tail
        std::cout << "  Checking for reverb tail in subsequent buffers:" << std::endl;
        for (int extra = 0; extra < 3; ++extra) {
            juce::AudioBuffer<float> emptyBuffer(2, BUFFER_SIZE);
            emptyBuffer.clear();
            reverb.process(emptyBuffer);
            
            float tailEnergy = 0.0f;
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                tailEnergy += std::abs(emptyBuffer.getSample(0, i));
            }
            std::cout << "    Buffer " << (extra+1) << " tail energy: " << tailEnergy;
            if (tailEnergy > 0.001f) {
                std::cout << " ✓";
            } else {
                std::cout << " ✗";
            }
            std::cout << std::endl;
        }
    }
    
    // Test 2: Check parameter response
    std::cout << "\n2. Testing parameter changes (dry to wet transition):" << std::endl;
    {
        reverb.reset();
        
        // Start with dry signal
        auto dryParams = params;
        dryParams[dryParams.rbegin()->first] = 0.0f; // Set mix to 0
        reverb.updateParameters(dryParams);
        
        juce::AudioBuffer<float> buffer(2, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, 0.5f * std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE));
            buffer.setSample(1, i, buffer.getSample(0, i));
        }
        
        reverb.process(buffer);
        float dryEnergy = buffer.getMagnitude(0, BUFFER_SIZE);
        std::cout << "  Dry (mix=0%) energy: " << dryEnergy << std::endl;
        
        // Switch to wet
        reverb.updateParameters(params); // 100% wet
        buffer.clear();
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, 0.5f * std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE));
            buffer.setSample(1, i, buffer.getSample(0, i));
        }
        
        reverb.process(buffer);
        float wetEnergy = buffer.getMagnitude(0, BUFFER_SIZE);
        std::cout << "  Wet (mix=100%) energy: " << wetEnergy << std::endl;
        
        if (std::abs(wetEnergy - dryEnergy) > 0.01f) {
            std::cout << "  ✓ Mix parameter is working" << std::endl;
        } else {
            std::cout << "  ✗ Mix parameter NOT working (wet == dry)" << std::endl;
        }
    }
    
    // Test 3: Check initialization state
    std::cout << "\n3. Testing repeated resets:" << std::endl;
    {
        float energies[3];
        for (int test = 0; test < 3; ++test) {
            reverb.reset();
            
            juce::AudioBuffer<float> buffer(2, BUFFER_SIZE);
            // Single impulse
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);
            
            reverb.process(buffer);
            energies[test] = buffer.getMagnitude(0, BUFFER_SIZE);
            std::cout << "  Reset " << (test+1) << " energy: " << energies[test] << std::endl;
        }
        
        if (std::abs(energies[0] - energies[1]) < 0.001f && 
            std::abs(energies[1] - energies[2]) < 0.001f) {
            std::cout << "  ✓ Consistent behavior after reset" << std::endl;
        } else {
            std::cout << "  ✗ Inconsistent behavior after reset" << std::endl;
        }
    }
}

int main() {
    std::cout << "=== REVERB DIAGNOSTIC TEST ===" << std::endl;
    std::cout << "Comparing working PlateReverb with non-working reverbs\n" << std::endl;
    
    // Test PlateReverb (reportedly working)
    {
        PlateReverb reverb;
        diagnoseReverb("PlateReverb", reverb);
    }
    
    // Test SpringReverb (not working)
    {
        SpringReverb reverb;
        diagnoseReverb("SpringReverb", reverb);
    }
    
    // Test ConvolutionReverb (weak)
    {
        ConvolutionReverb reverb;
        diagnoseReverb("ConvolutionReverb", reverb);
    }
    
    // Test ShimmerReverb (unstable)
    {
        ShimmerReverb reverb;
        diagnoseReverb("ShimmerReverb", reverb);
    }
    
    // Test GatedReverb (not working)
    {
        GatedReverb reverb;
        diagnoseReverb("GatedReverb", reverb);
    }
    
    std::cout << "\n=== DIAGNOSIS COMPLETE ===" << std::endl;
    std::cout << "Look for differences between PlateReverb and the others:" << std::endl;
    std::cout << "- Tail energy in empty buffers" << std::endl;
    std::cout << "- Mix parameter response" << std::endl;
    std::cout << "- Consistency after reset" << std::endl;
    
    return 0;
}
// Simple test to check if reverbs are producing any output
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include <JuceHeader.h>
#include <iostream>
#include <map>

int main() {
    const int SAMPLE_RATE = 44100;
    const int BUFFER_SIZE = 512;
    
    std::cout << "=== SIMPLE REVERB TEST ===" << std::endl;
    
    // Test Plate Reverb
    {
        std::cout << "\nPlate Reverb:" << std::endl;
        PlateReverb reverb;
        
        // Setup
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        // Set parameters for full wet signal
        std::map<int, float> params;
        params[0] = 0.7f;  // Size (large)
        params[1] = 0.3f;  // Damping (low)
        params[2] = 1.0f;  // Mix (100% wet)
        reverb.updateParameters(params);
        reverb.reset();
        
        // Create impulse
        juce::AudioBuffer<float> buffer(2, BUFFER_SIZE);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse on left channel
        buffer.setSample(1, 0, 1.0f);  // Impulse on right channel
        
        // Save original for comparison
        float originalSample = buffer.getSample(0, 0);
        
        // Process
        reverb.process(buffer);
        
        // Check output
        float processedSample = buffer.getSample(0, 0);
        float totalEnergy = 0.0f;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            totalEnergy += std::abs(buffer.getSample(0, i));
        }
        
        std::cout << "  Original impulse: " << originalSample << std::endl;
        std::cout << "  Processed sample[0]: " << processedSample << std::endl;
        std::cout << "  Total energy: " << totalEnergy << std::endl;
        
        if (totalEnergy > 0.1f) {
            std::cout << "  ✓ Reverb is producing output" << std::endl;
        } else {
            std::cout << "  ✗ Reverb is NOT producing output!" << std::endl;
        }
        
        // Process a few more blocks to see if reverb tail develops
        for (int block = 0; block < 5; ++block) {
            buffer.clear();
            reverb.process(buffer);
            
            float blockEnergy = 0.0f;
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                blockEnergy += std::abs(buffer.getSample(0, i));
            }
            std::cout << "  Block " << (block+2) << " energy: " << blockEnergy << std::endl;
        }
    }
    
    // Test Spring Reverb
    {
        std::cout << "\nSpring Reverb:" << std::endl;
        SpringReverb reverb;
        
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        std::map<int, float> params;
        params[0] = 0.5f;  // Tension
        params[1] = 0.3f;  // Damping
        params[2] = 0.5f;  // Springs
        params[3] = 0.7f;  // Diffusion
        params[4] = 0.5f;  // Brightness
        params[5] = 0.0f;  // Drip
        params[6] = 1.0f;  // Mix (100% wet)
        reverb.updateParameters(params);
        reverb.reset();
        
        juce::AudioBuffer<float> buffer(2, BUFFER_SIZE);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        reverb.process(buffer);
        
        float totalEnergy = 0.0f;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            totalEnergy += std::abs(buffer.getSample(0, i));
        }
        
        std::cout << "  Total energy: " << totalEnergy << std::endl;
        
        if (totalEnergy > 0.1f) {
            std::cout << "  ✓ Reverb is producing output" << std::endl;
        } else {
            std::cout << "  ✗ Reverb is NOT producing output!" << std::endl;
        }
    }
    
    // Test Shimmer Reverb
    {
        std::cout << "\nShimmer Reverb:" << std::endl;
        ShimmerReverb reverb;
        
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        std::map<int, float> params;
        params[0] = 0.7f;  // Size
        params[1] = 0.3f;  // Damping
        params[2] = 0.5f;  // Shimmer amount
        params[3] = 0.6f;  // Pitch
        params[4] = 0.5f;  // Modulation
        params[5] = 0.2f;  // Low cut
        params[6] = 0.8f;  // High cut
        params[7] = 0.0f;  // Freeze off
        params[8] = 1.0f;  // Mix (100% wet)
        reverb.updateParameters(params);
        reverb.reset();
        
        juce::AudioBuffer<float> buffer(2, BUFFER_SIZE);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        reverb.process(buffer);
        
        float totalEnergy = 0.0f;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            totalEnergy += std::abs(buffer.getSample(0, i));
        }
        
        std::cout << "  Total energy: " << totalEnergy << std::endl;
        
        if (totalEnergy > 0.1f) {
            std::cout << "  ✓ Reverb is producing output" << std::endl;
        } else {
            std::cout << "  ✗ Reverb is NOT producing output!" << std::endl;
        }
    }
    
    return 0;
}
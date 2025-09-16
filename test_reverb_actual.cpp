// Test to verify if ANY reverb is actually creating a reverb tail
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 512;

template<typename ReverbType>
void testReverbTail(const std::string& name, ReverbType& reverb, const std::map<int, float>& params) {
    std::cout << "\n=== " << name << " ===" << std::endl;
    
    // Initialize
    reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    reverb.updateParameters(params);
    reverb.reset();
    
    // Create a single impulse in first buffer
    juce::AudioBuffer<float> impulseBuffer(2, BUFFER_SIZE);
    impulseBuffer.clear();
    impulseBuffer.setSample(0, 0, 1.0f);
    impulseBuffer.setSample(1, 0, 1.0f);
    
    // Process the impulse
    std::cout << "Processing impulse..." << std::endl;
    reverb.process(impulseBuffer);
    
    // Check first buffer output
    float firstBufferEnergy = 0.0f;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        firstBufferEnergy += std::abs(impulseBuffer.getSample(0, i));
    }
    std::cout << "Buffer 1 (with impulse) energy: " << firstBufferEnergy << std::endl;
    
    // Now process EMPTY buffers to see if reverb tail continues
    std::cout << "\nProcessing empty buffers (should contain reverb tail):" << std::endl;
    for (int block = 2; block <= 10; ++block) {
        juce::AudioBuffer<float> emptyBuffer(2, BUFFER_SIZE);
        emptyBuffer.clear(); // All zeros
        
        // Process empty buffer - reverb should add tail
        reverb.process(emptyBuffer);
        
        // Measure output energy
        float energy = 0.0f;
        float peak = 0.0f;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = emptyBuffer.getSample(0, i);
            energy += std::abs(sample);
            peak = std::max(peak, std::abs(sample));
        }
        
        std::cout << "  Buffer " << block << ": energy=" << std::fixed << std::setprecision(6) 
                  << energy << " peak=" << peak;
        
        if (energy > 0.001f) {
            std::cout << " ✓ Has reverb tail";
        } else {
            std::cout << " ✗ No reverb tail";
        }
        std::cout << std::endl;
        
        // Stop if completely silent
        if (energy < 0.0001f && block > 3) break;
    }
}

int main() {
    std::cout << "=== REVERB TAIL VERIFICATION TEST ===" << std::endl;
    std::cout << "Testing if reverbs produce tail after impulse\n" << std::endl;
    
    // Test PlateReverb with 100% wet
    {
        PlateReverb reverb;
        std::map<int, float> params;
        params[0] = 0.9f;  // Large size
        params[1] = 0.1f;  // Low damping (long decay)
        params[2] = 1.0f;  // 100% wet
        testReverbTail("PlateReverb (100% wet, large)", reverb, params);
    }
    
    // Test SpringReverb with 100% wet
    {
        SpringReverb reverb;
        std::map<int, float> params;
        params[0] = 0.5f;  // Tension
        params[1] = 0.1f;  // Low damping
        params[2] = 0.8f;  // Many springs
        params[3] = 0.8f;  // High diffusion
        params[4] = 0.5f;  // Brightness
        params[5] = 0.3f;  // Some drip
        params[6] = 1.0f;  // 100% wet
        testReverbTail("SpringReverb (100% wet)", reverb, params);
    }
    
    // Test ConvolutionReverb
    {
        ConvolutionReverb reverb;
        std::map<int, float> params;
        params[0] = 1.0f;  // 100% wet
        testReverbTail("ConvolutionReverb (100% wet)", reverb, params);
    }
    
    // Test ShimmerReverb - regular
    {
        ShimmerReverb reverb;
        std::map<int, float> params;
        params[0] = 0.9f;  // Large size
        params[1] = 0.1f;  // Low damping
        params[2] = 0.7f;  // High shimmer
        params[3] = 0.7f;  // Pitch shift
        params[4] = 0.5f;  // Modulation
        params[5] = 0.0f;  // No low cut
        params[6] = 1.0f;  // No high cut
        params[7] = 0.0f;  // No freeze
        params[8] = 1.0f;  // 100% wet
        testReverbTail("ShimmerReverb (100% wet, large)", reverb, params);
    }
    
    // Test ShimmerReverb - frozen (should sustain forever)
    {
        ShimmerReverb reverb;
        std::map<int, float> params;
        params[0] = 0.9f;  // Large size
        params[1] = 0.0f;  // No damping
        params[2] = 0.5f;  // Shimmer
        params[3] = 0.5f;  // Pitch
        params[4] = 0.5f;  // Modulation
        params[5] = 0.0f;  // No low cut
        params[6] = 1.0f;  // No high cut
        params[7] = 1.0f;  // FREEZE ON (infinite sustain)
        params[8] = 1.0f;  // 100% wet
        testReverbTail("ShimmerReverb (FROZEN - should sustain)", reverb, params);
    }
    
    // Test GatedReverb
    {
        GatedReverb reverb;
        std::map<int, float> params;
        params[0] = 0.9f;  // Large size
        params[1] = 0.9f;  // Long gate time
        params[2] = 0.0f;  // No pre-delay
        params[3] = 0.1f;  // Low damping
        params[4] = 0.8f;  // High diffusion
        params[5] = 0.8f;  // Long hold
        params[6] = 1.0f;  // 100% wet
        testReverbTail("GatedReverb (100% wet, long gate)", reverb, params);
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "A proper reverb should show decaying energy across multiple buffers." << std::endl;
    std::cout << "If all buffers after the first are silent, the reverb is NOT working." << std::endl;
    
    return 0;
}
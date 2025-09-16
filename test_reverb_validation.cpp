#include <iostream>
#include <memory>
#include <map>
#include <cmath>
#include <vector>
#include <JuceHeader.h>

// Forward declarations
class PlateReverb;
class SpringReverb;
class ShimmerReverb;
class GatedReverb;
class ConvolutionReverb;

#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void testReverb(const std::string& name, std::unique_ptr<EngineBase> reverb) {
    std::cout << "\n========== " << name << " ==========\n";
    
    // Initialize
    reverb->prepareToPlay(44100, 512);
    
    // Test with varying mix values
    std::vector<float> mixValues = {0.0f, 0.5f, 1.0f};
    
    for (float mix : mixValues) {
        std::cout << "\nMix = " << mix << ":\n";
        
        // Set parameters
        std::map<int, float> params;
        params[0] = mix;  // Mix
        params[1] = 0.7f; // Size/Tension/etc
        params[2] = 0.5f; // Damping/etc
        reverb->updateParameters(params);
        
        // Create impulse
        juce::AudioBuffer<float> buffer(2, 44100); // 1 second buffer
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        // Process
        reverb->process(buffer);
        
        // Measure response at different time points
        float immediate = buffer.getRMSLevel(0, 0, 100);
        float early = buffer.getRMSLevel(0, 100, 1000);
        float late = buffer.getRMSLevel(0, 10000, 10000);
        
        std::cout << "  Immediate (0-100): " << immediate << "\n";
        std::cout << "  Early (100-1100): " << early << "\n";
        std::cout << "  Late (10k-20k): " << late << "\n";
        
        // Check if reverb tail exists
        float tailEnergy = 0;
        for (int i = 1000; i < 44100; i++) {
            float sample = buffer.getSample(0, i);
            tailEnergy += sample * sample;
        }
        
        if (tailEnergy > 0.001f) {
            std::cout << "  ✓ Reverb tail present (energy: " << tailEnergy << ")\n";
        } else {
            std::cout << "  ✗ No reverb tail detected\n";
        }
    }
}

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "REVERB VALIDATION TEST\n";
    std::cout << "======================\n";
    std::cout << "Testing with 1-second buffers to capture full reverb tail\n";
    
    testReverb("PlateReverb", std::make_unique<PlateReverb>());
    testReverb("SpringReverb", std::make_unique<SpringReverb>());
    testReverb("ShimmerReverb", std::make_unique<ShimmerReverb>());
    testReverb("GatedReverb", std::make_unique<GatedReverb>());
    testReverb("ConvolutionReverb", std::make_unique<ConvolutionReverb>());
    
    return 0;
}

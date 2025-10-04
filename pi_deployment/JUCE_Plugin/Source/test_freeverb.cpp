#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <iomanip>
#include <memory>
#include "PlateReverb.h"

int main() {
    std::cout << "=== FREEVERB PLATE REVERB TEST ===" << std::endl;
    
    PlateReverb reverb;
    reverb.prepareToPlay(44100, 512);
    reverb.reset();
    
    // Set reverb parameters for maximum effect
    std::map<int, float> params;
    params[0] = 0.9f; // Size (large room)
    params[1] = 0.3f; // Damping (moderate)
    params[2] = 0.0f; // Predelay
    params[3] = 1.0f; // Mix (full wet)
    reverb.updateParameters(params);
    
    // Create impulse
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f); // Impulse on left channel
    buffer.setSample(1, 0, 1.0f); // Impulse on right channel
    
    std::cout << "\nProcessing impulse..." << std::endl;
    
    // Process impulse
    reverb.process(buffer);
    float impulseOut = buffer.getRMSLevel(0, 0, 512);
    std::cout << "After impulse: RMS = " << impulseOut 
              << ", First sample = " << buffer.getSample(0, 0) << std::endl;
    
    // Process silence and check for tail
    std::cout << "\nReverb tail (processing silence):" << std::endl;
    for (int block = 0; block < 10; ++block) {
        buffer.clear();
        reverb.process(buffer);
        float rms = buffer.getRMSLevel(0, 0, 512);
        
        std::cout << "Block " << block << ": RMS = " << rms;
        
        // Also check first few samples
        std::cout << " [Samples: ";
        for (int i = 0; i < 5; ++i) {
            std::cout << buffer.getSample(0, i) << " ";
        }
        std::cout << "]" << std::endl;
        
        // Check if we have a proper tail
        if (block == 0 && rms < 0.001f) {
            std::cout << "WARNING: No reverb tail detected!" << std::endl;
        }
    }
    
    // Test mix parameter
    std::cout << "\nTesting mix parameter:" << std::endl;
    
    // Reset and test with mix = 0 (dry)
    reverb.reset();
    params[3] = 0.0f; // Dry
    reverb.updateParameters(params);
    
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    reverb.process(buffer);
    std::cout << "Mix=0 (dry): Output = " << buffer.getSample(0, 0) << std::endl;
    
    // Reset and test with mix = 0.5
    reverb.reset();
    params[3] = 0.5f;
    reverb.updateParameters(params);
    
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    reverb.process(buffer);
    std::cout << "Mix=0.5: Output = " << buffer.getSample(0, 0) << std::endl;
    
    return 0;
}

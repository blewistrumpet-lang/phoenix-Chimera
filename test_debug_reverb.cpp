// Debug test to see what's actually happening in the reverbs
#include <iostream>
#include <memory>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/SpringReverb.h"

int main() {
    std::cout << "=== DEBUG: What's happening in SpringReverb? ===" << std::endl;
    
    auto reverb = std::make_unique<SpringReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Set mix to 100% wet
    std::map<int, float> params;
    params[3] = 1.0f; // Mix = 100% wet
    reverb->updateParameters(params);
    
    // Create buffer with constant signal
    juce::AudioBuffer<float> buffer(2, 512);
    for (int s = 0; s < 512; s++) {
        buffer.setSample(0, s, 0.5f);
        buffer.setSample(1, s, 0.5f);
    }
    
    std::cout << "\nBEFORE processing:" << std::endl;
    std::cout << "  First 10 samples: ";
    for (int i = 0; i < 10; i++) {
        std::cout << buffer.getSample(0, i) << " ";
    }
    std::cout << std::endl;
    std::cout << "  RMS: " << buffer.getRMSLevel(0, 0, 512) << std::endl;
    
    // Process
    reverb->process(buffer);
    
    std::cout << "\nAFTER processing:" << std::endl;
    std::cout << "  First 10 samples: ";
    for (int i = 0; i < 10; i++) {
        std::cout << buffer.getSample(0, i) << " ";
    }
    std::cout << std::endl;
    std::cout << "  RMS: " << buffer.getRMSLevel(0, 0, 512) << std::endl;
    
    // Check if all samples are zero
    bool allZero = true;
    for (int s = 0; s < 512; s++) {
        if (buffer.getSample(0, s) != 0.0f) {
            allZero = false;
            break;
        }
    }
    
    if (allZero) {
        std::cout << "\nPROBLEM: All samples are ZERO after processing!" << std::endl;
    } else {
        std::cout << "\nSamples are non-zero but RMS calculation might be wrong" << std::endl;
    }
    
    return 0;
}

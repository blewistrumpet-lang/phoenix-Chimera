// Simple test for ConvolutionReverb
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

int main() {
    std::cout << "CONVOLUTIONREVERB SIMPLE TEST" << std::endl;
    
    auto reverb = std::make_unique<ConvolutionReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Set to 100% wet, medium size
    std::map<int, float> params;
    params[0] = 1.0f;  // Mix: 100% wet
    params[1] = 0.0f;  // PreDelay: none
    params[2] = 0.3f;  // Damping: low
    params[3] = 0.7f;  // Size: large
    params[4] = 1.0f;  // Width: full stereo
    reverb->updateParameters(params);
    
    // Send impulse
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    std::cout << "\nProcessing impulse through reverb..." << std::endl;
    
    float totalEnergy = 0.0f;
    for (int block = 0; block < 10; block++) {
        reverb->process(buffer);
        float rms = buffer.getRMSLevel(0, 0, 512);
        totalEnergy += rms;
        
        std::cout << "Block " << block << ": RMS=" << std::fixed 
                  << std::setprecision(6) << rms;
        if (rms > 0.001f) std::cout << " <- REVERB TAIL";
        std::cout << std::endl;
        
        if (block == 0) buffer.clear(); // Clear after first block
    }
    
    std::cout << "\nTotal energy: " << totalEnergy << std::endl;
    std::cout << "Result: " << (totalEnergy > 0.01f ? 
        "CONVOLUTION REVERB WORKING ✓" : "NO REVERB OUTPUT ✗") << std::endl;
    
    return 0;
}
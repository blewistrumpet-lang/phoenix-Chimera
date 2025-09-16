// Minimal test to verify ShimmerReverb parameter issue
#include <iostream>
#include <memory>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"

// Directly include the source to debug
#include "JUCE_Plugin/Source/ShimmerReverb.cpp"

int main() {
    std::cout << "Testing ShimmerReverb directly..." << std::endl;
    
    // Create instance
    ShimmerReverb reverb;
    reverb.prepareToPlay(44100, 512);
    
    // Test parameter setting
    std::map<int, float> params;
    params[4] = 0.0f;  // Mix = 0 (dry only)
    
    std::cout << "Setting mix to 0.0..." << std::endl;
    reverb.updateParameters(params);
    
    // Process
    juce::AudioBuffer<float> buffer(2, 1);
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    reverb.process(buffer);
    
    float output = buffer.getSample(0, 0);
    std::cout << "Input: 1.0, Output: " << output << std::endl;
    std::cout << "Expected: 1.0 (dry), Got: " << output << std::endl;
    
    // Now test wet
    params[4] = 1.0f;  // Mix = 1 (wet only)
    std::cout << "\nSetting mix to 1.0..." << std::endl;
    reverb.updateParameters(params);
    
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    reverb.process(buffer);
    
    output = buffer.getSample(0, 0);
    std::cout << "Input: 1.0, Output: " << output << std::endl;
    std::cout << "Expected: ~0.0 (wet only, no direct), Got: " << output << std::endl;
    
    return 0;
}
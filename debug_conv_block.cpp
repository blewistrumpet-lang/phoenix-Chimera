#include <iostream>
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

int main() {
    ConvolutionReverb conv;
    
    // Initialize with 512 sample blocks
    conv.prepareToPlay(44100, 512);
    conv.reset();
    
    // Set mix to 0
    std::map<int, float> params;
    params[0] = 0.0f;
    conv.updateParameters(params);
    
    // Test with 512 sample block
    juce::AudioBuffer<float> buffer(2, 512);
    for (int i = 0; i < 512; ++i) {
        buffer.setSample(0, i, 0.5f);
        buffer.setSample(1, i, 0.5f);
    }
    
    std::cout << "Before process: " << buffer.getSample(0, 0) << std::endl;
    conv.process(buffer);
    std::cout << "After process: " << buffer.getSample(0, 0) << std::endl;
    
    // Check a few samples
    std::cout << "Sample 0: " << buffer.getSample(0, 0) << std::endl;
    std::cout << "Sample 100: " << buffer.getSample(0, 100) << std::endl;
    std::cout << "Sample 200: " << buffer.getSample(0, 200) << std::endl;
    std::cout << "Sample 300: " << buffer.getSample(0, 300) << std::endl;
    std::cout << "Sample 400: " << buffer.getSample(0, 400) << std::endl;
    std::cout << "Sample 511: " << buffer.getSample(0, 511) << std::endl;
    
    return 0;
}

#include <iostream>
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

int main() {
    ConvolutionReverb conv;
    
    std::cout << "After construction, testing initial state..." << std::endl;
    
    // Test immediately after construction
    juce::AudioBuffer<float> buffer(2, 1);
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    conv.process(buffer);
    std::cout << "  Without prepareToPlay: 1.0 -> " << buffer.getSample(0, 0) << std::endl;
    
    // Now with prepareToPlay
    conv.prepareToPlay(44100, 512);
    conv.reset();
    
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    conv.process(buffer);
    std::cout << "  After prepareToPlay: 1.0 -> " << buffer.getSample(0, 0) << std::endl;
    
    // Set mix to 0 explicitly
    std::map<int, float> params;
    params[0] = 0.0f;  // Mix = 0
    conv.updateParameters(params);
    
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    conv.process(buffer);
    std::cout << "  After setting mix=0: 1.0 -> " << buffer.getSample(0, 0) << std::endl;
    
    // Process multiple times to let smoothing settle
    for (int i = 0; i < 100; ++i) {
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        conv.process(buffer);
    }
    std::cout << "  After 100 processes: 1.0 -> " << buffer.getSample(0, 0) << std::endl;
    
    return 0;
}

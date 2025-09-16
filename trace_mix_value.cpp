#include <iostream>
#include <iomanip>
#include "JUCE_Plugin/Source/PlateReverb.h"

// Hack to access private members for debugging
class PlateReverbDebug : public PlateReverb {
public:
    float getMixValue() {
        if (m_mix) {
            return m_mix->getCurrentValue();
        }
        return -1.0f;
    }
};

int main() {
    PlateReverbDebug reverb;
    
    // Initialize
    reverb.prepareToPlay(44100, 1);
    reverb.reset();
    
    // Set mix to 0
    std::map<int, float> params;
    params[3] = 0.0f;  // Mix = 0
    reverb.updateParameters(params);
    
    std::cout << "After setting mix to 0:" << std::endl;
    std::cout << "  Internal mix value: " << reverb.getMixValue() << std::endl;
    
    // Process some blocks
    juce::AudioBuffer<float> buffer(2, 1);
    for (int i = 0; i < 10; ++i) {
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        reverb.process(buffer);
        std::cout << "  After process " << i << ": mix=" << reverb.getMixValue() 
                  << ", output=" << buffer.getSample(0, 0) << std::endl;
    }
    
    return 0;
}

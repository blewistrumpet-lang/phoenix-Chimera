#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <iomanip>
#include <memory>
#include "PlateReverb.h"

int main() {
    std::cout << "=== REVERB TRACE TEST ===" << std::endl;
    
    PlateReverb reverb;
    reverb.prepareToPlay(44100, 512);
    reverb.reset();
    
    // Test with DRY signal first
    {
        std::map<int, float> params;
        params[0] = 1.0f; // Size
        params[1] = 0.3f; // Damping
        params[2] = 0.0f; // Predelay
        params[3] = 0.0f; // Mix (DRY)
        reverb.updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        
        reverb.process(buffer);
        std::cout << "DRY (mix=0): Output = " << buffer.getSample(0, 0) 
                  << ", RMS = " << buffer.getRMSLevel(0, 0, 512) << std::endl;
    }
    
    // Reset and test with WET signal
    reverb.reset();
    {
        std::map<int, float> params;
        params[0] = 1.0f; // Size
        params[1] = 0.3f; // Damping
        params[2] = 0.0f; // Predelay
        params[3] = 1.0f; // Mix (WET)
        reverb.updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        
        std::cout << "\nWET (mix=1):" << std::endl;
        std::cout << "Input: " << buffer.getSample(0, 0) << std::endl;
        
        reverb.process(buffer);
        std::cout << "After process: Output[0] = " << buffer.getSample(0, 0) 
                  << ", RMS = " << buffer.getRMSLevel(0, 0, 512) << std::endl;
        
        // Now process silence
        buffer.clear();
        reverb.process(buffer);
        std::cout << "After silence: Output[0] = " << buffer.getSample(0, 0)
                  << ", RMS = " << buffer.getRMSLevel(0, 0, 512) << std::endl;
        
        // Check a few more samples
        std::cout << "First 10 samples: ";
        for (int i = 0; i < 10; ++i) {
            std::cout << buffer.getSample(0, i) << " ";
        }
        std::cout << std::endl;
    }
    
    return 0;
}

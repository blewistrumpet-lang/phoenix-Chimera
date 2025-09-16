#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PlateReverb_Proven.cpp"
#include <iostream>

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    
    PlateReverb reverb;
    reverb.prepareToPlay(44100, 512);
    
    // Create test signal - impulse
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    // Set to 100% wet
    std::map<int, float> params;
    params[0] = 1.0f; // Mix = 100% wet
    params[1] = 0.7f; // Size
    params[2] = 0.5f; // Damping
    reverb.updateParameters(params);
    
    // Process
    reverb.process(buffer);
    
    // Check output
    float leftRMS = buffer.getRMSLevel(0, 0, 512);
    float rightRMS = buffer.getRMSLevel(1, 0, 512);
    
    std::cout << "After processing impulse with Mix=1.0:\n";
    std::cout << "Left RMS: " << leftRMS << "\n";
    std::cout << "Right RMS: " << rightRMS << "\n";
    
    // Check first few samples
    std::cout << "\nFirst 10 samples (left):\n";
    for (int i = 0; i < 10; i++) {
        std::cout << "  " << i << ": " << buffer.getSample(0, i) << "\n";
    }
    
    // Process more blocks to see reverb tail
    buffer.clear();
    for (int block = 0; block < 5; block++) {
        reverb.process(buffer);
        float rms = buffer.getRMSLevel(0, 0, 512);
        std::cout << "Block " << (block + 1) << " RMS: " << rms << "\n";
    }
    
    return 0;
}

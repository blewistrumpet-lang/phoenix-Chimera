#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "Testing ConvolutionReverb in detail\n";
    std::cout << "====================================\n";
    
    ConvolutionReverb reverb;
    
    // Initialize with proper block size
    std::cout << "\n1. Initializing with 44100 Hz, 4096 samples\n";
    reverb.prepareToPlay(44100, 4096);
    
    // Set to 100% wet
    std::map<int, float> params;
    params[0] = 1.0f;  // Mix = 100% wet
    params[1] = 0.0f;  // IR Select = Concert Hall
    params[2] = 1.0f;  // Size = Full
    params[3] = 0.0f;  // Pre-delay = 0
    params[4] = 0.0f;  // Damping = 0
    
    std::cout << "\n2. Setting parameters (Mix=1.0, IR=ConcertHall)\n";
    reverb.updateParameters(params);
    
    // Process multiple blocks to ensure IR is loaded
    juce::AudioBuffer<float> buffer(2, 4096);
    
    std::cout << "\n3. Processing warm-up blocks...\n";
    for (int i = 0; i < 5; i++) {
        buffer.clear();
        reverb.process(buffer);
    }
    
    // Now send impulse
    std::cout << "\n4. Sending impulse signal\n";
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    reverb.process(buffer);
    
    // Check output
    float immediateRMS = buffer.getRMSLevel(0, 0, 100);
    float overallRMS = buffer.getRMSLevel(0, 0, 4096);
    
    std::cout << "\n5. Results:\n";
    std::cout << "   Immediate RMS (0-100): " << immediateRMS << "\n";
    std::cout << "   Overall RMS: " << overallRMS << "\n";
    
    // Check first few samples
    std::cout << "\n   First 10 samples:\n";
    for (int i = 0; i < 10; i++) {
        std::cout << "     " << i << ": " << buffer.getSample(0, i) << "\n";
    }
    
    // Process more blocks to see tail
    std::cout << "\n6. Processing tail blocks:\n";
    float totalTailEnergy = 0;
    for (int block = 0; block < 10; block++) {
        buffer.clear();
        reverb.process(buffer);
        float blockRMS = buffer.getRMSLevel(0, 0, 4096);
        totalTailEnergy += blockRMS * blockRMS;
        std::cout << "   Block " << block << " RMS: " << blockRMS << "\n";
    }
    
    std::cout << "\n7. Total tail energy: " << totalTailEnergy << "\n";
    
    if (totalTailEnergy > 0.001f) {
        std::cout << "\n✓ ConvolutionReverb is working!\n";
    } else {
        std::cout << "\n✗ ConvolutionReverb not producing reverb tail\n";
    }
    
    // Test latency
    int latency = reverb.getLatencySamples();
    std::cout << "\n8. Reported latency: " << latency << " samples\n";
    
    return 0;
}

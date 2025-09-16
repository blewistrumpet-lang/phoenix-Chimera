#include <iostream>
#include <iomanip>
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"

void testReverb(EngineBase* reverb, const std::string& name) {
    std::cout << "\n=== " << name << " ===" << std::endl;
    
    // Initialize
    reverb->prepareToPlay(44100, 1);
    reverb->reset();
    
    // Set all params to 0
    std::map<int, float> params;
    for (int i = 0; i < reverb->getNumParameters(); ++i) {
        params[i] = 0.0f;
    }
    reverb->updateParameters(params);
    
    // Process some dummy blocks to stabilize
    juce::AudioBuffer<float> dummy(2, 1);
    dummy.clear();
    for (int i = 0; i < 100; ++i) {
        reverb->process(dummy);
    }
    
    // Now test with real signal
    juce::AudioBuffer<float> buffer(2, 1);
    
    // Test 1: Single sample
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    reverb->process(buffer);
    std::cout << "  Single 1.0 -> " << buffer.getSample(0, 0) << std::endl;
    
    // Test 2: Negative value
    buffer.setSample(0, 0, -1.0f);
    buffer.setSample(1, 0, -1.0f);
    reverb->process(buffer);
    std::cout << "  Single -1.0 -> " << buffer.getSample(0, 0) << std::endl;
    
    // Test 3: Small value
    buffer.setSample(0, 0, 0.1f);
    buffer.setSample(1, 0, 0.1f);
    reverb->process(buffer);
    std::cout << "  Single 0.1 -> " << buffer.getSample(0, 0) << std::endl;
    
    // Test with mix = 1.0 (full wet)
    params[reverb->getNumParameters() - 1] = 1.0f; // Assuming mix is last param
    reverb->updateParameters(params);
    
    // Process dummy to stabilize
    for (int i = 0; i < 100; ++i) {
        dummy.clear();
        reverb->process(dummy);
    }
    
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    reverb->process(buffer);
    std::cout << "  Mix=1.0, 1.0 -> " << buffer.getSample(0, 0) << std::endl;
}

int main() {
    PlateReverb plate;
    testReverb(&plate, "PlateReverb");
    
    ShimmerReverb shimmer;
    testReverb(&shimmer, "ShimmerReverb");
    
    return 0;
}

#include <iostream>
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void testReverbPassthrough(EngineBase* reverb, const std::string& name) {
    std::cout << "\n" << name << ":" << std::endl;
    
    // Fresh initialization
    reverb->prepareToPlay(44100, 512);
    reverb->reset();
    
    // Force mix to 0
    std::map<int, float> params;
    for (int i = 0; i < reverb->getNumParameters(); ++i) {
        params[i] = 0.0f;
    }
    reverb->updateParameters(params);
    
    // Let parameters settle
    juce::AudioBuffer<float> silence(2, 512);
    silence.clear();
    for (int i = 0; i < 10; ++i) {
        reverb->process(silence);
    }
    
    // Test passthrough with fresh buffer
    juce::AudioBuffer<float> test(2, 512);
    for (int i = 0; i < 512; ++i) {
        test.setSample(0, i, 0.5f);
        test.setSample(1, i, 0.5f);
    }
    
    // Save original
    juce::AudioBuffer<float> original(test);
    
    // Process
    reverb->process(test);
    
    // Check results
    float sumDiff = 0.0f;
    float maxDiff = 0.0f;
    for (int i = 0; i < 512; ++i) {
        float diff = std::abs(test.getSample(0, i) - original.getSample(0, i));
        sumDiff += diff;
        maxDiff = std::max(maxDiff, diff);
    }
    
    float avgOut = 0.0f;
    for (int i = 0; i < 512; ++i) {
        avgOut += test.getSample(0, i);
    }
    avgOut /= 512.0f;
    
    std::cout << "  Input: 0.5, Output avg: " << avgOut << std::endl;
    std::cout << "  Max difference: " << maxDiff << std::endl;
    std::cout << "  Status: " << (maxDiff < 0.01f ? "PASS" : "FAIL") << std::endl;
}

int main() {
    std::cout << "Testing reverb passthrough with mix=0..." << std::endl;
    
    PlateReverb plate;
    testReverbPassthrough(&plate, "PlateReverb");
    
    ShimmerReverb shimmer;
    testReverbPassthrough(&shimmer, "ShimmerReverb");
    
    SpringReverb spring;
    testReverbPassthrough(&spring, "SpringReverb");
    
    GatedReverb gated;
    testReverbPassthrough(&gated, "GatedReverb");
    
    ConvolutionReverb conv;
    testReverbPassthrough(&conv, "ConvolutionReverb");
    
    return 0;
}

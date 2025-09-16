// Simple test for all 5 reverbs
#include <iostream>
#include <memory>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "/Users/Branden/JUCE/modules/juce_dsp/juce_dsp.h"
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void testReverb(const std::string& name, EngineBase* reverb) {
    std::cout << "\nTesting " << name << "..." << std::endl;
    
    // Check parameters
    int numParams = reverb->getNumParameters();
    std::cout << "  Parameters: " << numParams << std::endl;
    
    if (numParams != 10) {
        std::cout << "  ✗ FAIL - Expected 10 parameters" << std::endl;
        return;
    }
    
    // Initialize
    reverb->prepareToPlay(44100, 512);
    reverb->reset();
    
    // Set mix to 50%
    std::map<int, float> params;
    params[0] = 0.5f;
    reverb->updateParameters(params);
    
    // Process test signal
    juce::AudioBuffer<float> buffer(2, 512);
    for (int i = 0; i < 512; i++) {
        float val = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f) * 0.5f;
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }
    
    float inputRMS = buffer.getRMSLevel(0, 0, 512);
    reverb->process(buffer);
    float outputRMS = buffer.getRMSLevel(0, 0, 512);
    
    std::cout << "  Input RMS: " << inputRMS << std::endl;
    std::cout << "  Output RMS: " << outputRMS << std::endl;
    
    if (outputRMS > 0.001f && outputRMS < 2.0f) {
        std::cout << "  ✓ PASS - Audio processing works" << std::endl;
    } else {
        std::cout << "  ✗ FAIL - Audio issue" << std::endl;
    }
}

int main() {
    std::cout << "CHIMERA PHOENIX - REVERB VERIFICATION" << std::endl;
    std::cout << "======================================" << std::endl;
    
    // Test each reverb
    {
        auto reverb = std::make_unique<PlateReverb>();
        testReverb("PlateReverb", reverb.get());
    }
    
    {
        auto reverb = std::make_unique<SpringReverb>();
        testReverb("SpringReverb", reverb.get());
    }
    
    {
        auto reverb = std::make_unique<ShimmerReverb>();
        testReverb("ShimmerReverb", reverb.get());
    }
    
    {
        auto reverb = std::make_unique<GatedReverb>();
        testReverb("GatedReverb", reverb.get());
    }
    
    {
        auto reverb = std::make_unique<ConvolutionReverb>();
        testReverb("ConvolutionReverb", reverb.get());
    }
    
    std::cout << "\n======================================" << std::endl;
    std::cout << "All 5 reverbs tested!" << std::endl;
    
    return 0;
}
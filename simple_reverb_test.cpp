#include <iostream>
#include <iomanip>
#include <cmath>
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void testReverb(EngineBase* reverb, const std::string& name) {
    std::cout << "\n=== " << name << " ===" << std::endl;
    
    reverb->prepareToPlay(44100, 512);
    reverb->reset();
    
    // Test with mix at 50%
    std::map<int, float> params;
    for (int i = 0; i < reverb->getNumParameters(); ++i) {
        juce::String paramName = reverb->getParameterName(i).toLowerCase();
        if (paramName.contains("mix")) {
            params[i] = 0.5f;
            std::cout << "Setting Mix to 50%" << std::endl;
        } else if (paramName.contains("size") || paramName.contains("room")) {
            params[i] = 0.7f;
            std::cout << "Setting Size/Room to 70%" << std::endl;
        } else if (paramName.contains("time")) {
            params[i] = 0.7f;
            std::cout << "Setting Time to 70%" << std::endl;
        } else {
            params[i] = 0.5f;
        }
    }
    reverb->updateParameters(params);
    
    // Create test signal - impulse
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    // Process impulse
    reverb->process(buffer);
    
    // Check output
    float maxOutput = 0.0f;
    float sumOutput = 0.0f;
    for (int i = 0; i < 512; ++i) {
        float sample = std::abs(buffer.getSample(0, i));
        maxOutput = std::max(maxOutput, sample);
        sumOutput += sample;
    }
    
    std::cout << "After impulse:" << std::endl;
    std::cout << "  Max output: " << maxOutput << std::endl;
    std::cout << "  Average: " << (sumOutput / 512.0f) << std::endl;
    
    // Process silence and check for tail
    for (int block = 0; block < 10; ++block) {
        buffer.clear();
        reverb->process(buffer);
        
        float blockMax = 0.0f;
        float blockSum = 0.0f;
        for (int i = 0; i < 512; ++i) {
            float sample = std::abs(buffer.getSample(0, i));
            blockMax = std::max(blockMax, sample);
            blockSum += sample;
        }
        
        if (block == 0 || block == 9) {
            std::cout << "Block " << block << ":" << std::endl;
            std::cout << "  Max: " << std::scientific << blockMax << std::endl;
            std::cout << "  Avg: " << (blockSum / 512.0f) << std::endl;
        }
    }
    
    // Test stability with continuous input
    std::cout << "\nStability test with continuous sine:" << std::endl;
    for (int block = 0; block < 10; ++block) {
        for (int i = 0; i < 512; ++i) {
            float sine = 0.5f * std::sin(2.0f * M_PI * 440.0f * (block * 512 + i) / 44100.0f);
            buffer.setSample(0, i, sine);
            buffer.setSample(1, i, sine);
        }
        reverb->process(buffer);
    }
    
    // Check final output
    maxOutput = 0.0f;
    for (int i = 0; i < 512; ++i) {
        maxOutput = std::max(maxOutput, std::abs(buffer.getSample(0, i)));
    }
    std::cout << "Final max after sine input: " << maxOutput;
    if (maxOutput > 2.0f) {
        std::cout << " ✗ (unstable)";
    } else if (maxOutput < 0.1f) {
        std::cout << " ⚠ (too quiet)";
    } else {
        std::cout << " ✓";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "SIMPLE REVERB TEST" << std::endl;
    std::cout << "==================" << std::endl;
    
    PlateReverb plate;
    testReverb(&plate, "PlateReverb");
    
    ShimmerReverb shimmer;
    testReverb(&shimmer, "ShimmerReverb");
    
    SpringReverb spring;
    testReverb(&spring, "SpringReverb");
    
    GatedReverb gated;
    testReverb(&gated, "GatedReverb");
    
    ConvolutionReverb conv;
    testReverb(&conv, "ConvolutionReverb");
    
    return 0;
}
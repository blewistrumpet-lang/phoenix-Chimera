#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void validateReverb(EngineBase* reverb, const std::string& name) {
    std::cout << "\n" << name << ":" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    reverb->prepareToPlay(44100, 512);
    reverb->reset();
    
    // Test 1: Absolute silence with mix=0
    std::map<int, float> params;
    for (int i = 0; i < reverb->getNumParameters(); ++i) {
        params[i] = 0.0f;
    }
    reverb->updateParameters(params);
    
    juce::AudioBuffer<float> buffer(2, 512);
    for (int i = 0; i < 512; ++i) {
        buffer.setSample(0, i, 0.5f);
        buffer.setSample(1, i, 0.5f);
    }
    
    reverb->process(buffer);
    
    float error = 0.0f;
    for (int i = 0; i < 512; ++i) {
        error += std::abs(buffer.getSample(0, i) - 0.5f);
    }
    error /= 512.0f;
    
    std::cout << "  Dry passthrough error: " << std::scientific << error;
    if (error < 0.001f) {
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ✗" << std::endl;
    }
    
    // Test 2: Long-term stability with full wet
    int mixParam = -1;
    for (int i = 0; i < reverb->getNumParameters(); ++i) {
        if (reverb->getParameterName(i).containsIgnoreCase("mix")) {
            mixParam = i;
            break;
        }
    }
    
    params[mixParam] = 1.0f;
    reverb->updateParameters(params);
    reverb->reset();
    
    // Send impulse and monitor decay
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    std::vector<float> peakLevels;
    for (int block = 0; block < 200; ++block) {
        reverb->process(buffer);
        
        float peak = 0.0f;
        for (int i = 0; i < 512; ++i) {
            peak = std::max(peak, std::abs(buffer.getSample(0, i)));
        }
        peakLevels.push_back(peak);
        
        buffer.clear();
    }
    
    // Check for stability
    int growthCount = 0;
    for (size_t i = 20; i < peakLevels.size() - 1; ++i) {
        if (peakLevels[i] > peakLevels[i-1] * 1.05f && peakLevels[i] > 0.001f) {
            growthCount++;
        }
    }
    
    std::cout << "  Growth events in decay: " << growthCount;
    if (growthCount < 5) {
        std::cout << " ✓" << std::endl;
    } else {
        std::cout << " ⚠" << std::endl;
    }
    
    // Test 3: No explosion with extreme input
    params[mixParam] = 0.8f;
    reverb->updateParameters(params);
    
    for (int i = 0; i < 512; ++i) {
        buffer.setSample(0, i, 0.9f * std::sin(2.0f * M_PI * 100.0f * i / 44100.0f));
        buffer.setSample(1, i, 0.9f * std::sin(2.0f * M_PI * 100.0f * i / 44100.0f));
    }
    
    bool stable = true;
    for (int block = 0; block < 50; ++block) {
        reverb->process(buffer);
        
        for (int i = 0; i < 512; ++i) {
            float sample = buffer.getSample(0, i);
            if (!std::isfinite(sample) || std::abs(sample) > 2.0f) {
                stable = false;
                break;
            }
        }
        if (!stable) break;
    }
    
    std::cout << "  Extreme input stability: " << (stable ? "✓" : "✗") << std::endl;
    
    // Test 4: Verify reasonable decay time
    params[mixParam] = 1.0f;
    for (int i = 0; i < reverb->getNumParameters(); ++i) {
        if (i != mixParam) params[i] = 0.7f; // Higher settings
    }
    reverb->updateParameters(params);
    reverb->reset();
    
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    reverb->process(buffer);
    
    float initialPeak = 0.0f;
    for (int i = 0; i < 512; ++i) {
        initialPeak = std::max(initialPeak, std::abs(buffer.getSample(0, i)));
    }
    
    int decayBlocks = 0;
    for (int block = 1; block < 500; ++block) {
        buffer.clear();
        reverb->process(buffer);
        
        float peak = 0.0f;
        for (int i = 0; i < 512; ++i) {
            peak = std::max(peak, std::abs(buffer.getSample(0, i)));
        }
        
        if (peak < initialPeak * 0.001f) { // -60dB
            decayBlocks = block;
            break;
        }
    }
    
    float decayTime = decayBlocks * 512.0f / 44100.0f;
    std::cout << "  Decay time (RT60): " << std::fixed << std::setprecision(2) << decayTime << "s";
    if (decayTime > 0.1f && decayTime < 10.0f) {
        std::cout << " ✓" << std::endl;
    } else if (decayTime == 0) {
        std::cout << " (>5.8s) ⚠" << std::endl;
    } else {
        std::cout << " ✗" << std::endl;
    }
}

int main() {
    std::cout << "\n============================================" << std::endl;
    std::cout << "    FINAL REVERB VALIDATION" << std::endl;
    std::cout << "============================================" << std::endl;
    
    PlateReverb plate;
    validateReverb(&plate, "PlateReverb");
    
    ShimmerReverb shimmer;
    validateReverb(&shimmer, "ShimmerReverb");
    
    SpringReverb spring;
    validateReverb(&spring, "SpringReverb");
    
    GatedReverb gated;
    validateReverb(&gated, "GatedReverb");
    
    ConvolutionReverb conv;
    validateReverb(&conv, "ConvolutionReverb");
    
    std::cout << "\n============================================" << std::endl;
    std::cout << "✓ = Pass  ⚠ = Warning  ✗ = Fail" << std::endl;
    std::cout << "\nAll reverbs should:" << std::endl;
    std::cout << "  • Pass dry signal unchanged when mix=0" << std::endl;
    std::cout << "  • Decay smoothly without growing energy" << std::endl;
    std::cout << "  • Remain stable with extreme inputs" << std::endl;
    std::cout << "  • Have reasonable decay times" << std::endl;
    std::cout << "============================================" << std::endl;
    
    return 0;
}

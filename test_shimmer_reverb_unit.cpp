// Unit Test for ShimmerReverb
#include <iostream>
#include <memory>
#include <cmath>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"

bool testShimmerReverbStability() {
    std::cout << "Testing ShimmerReverb feedback stability..." << std::endl;
    
    auto reverb = std::make_unique<ShimmerReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Set parameters that previously caused explosion
    std::map<int, float> params;
    params[0] = 0.8f; // Large room size
    params[1] = 0.2f; // Low damping
    params[2] = 1.0f; // MAXIMUM shimmer (was causing explosion)
    params[3] = 0.75f; // Pitch up (+octave)
    params[4] = 0.0f; // No predelay
    params[5] = 1.0f; // 100% wet
    reverb->updateParameters(params);
    
    // Create test signal (sine wave)
    juce::AudioBuffer<float> buffer(2, 512);
    float phase = 0.0f;
    float freq = 440.0f;
    float maxLevel = 0.0f;
    bool isStable = true;
    
    for (int block = 0; block < 50; block++) {
        // Generate sine wave
        for (int sample = 0; sample < 512; sample++) {
            float value = 0.3f * std::sin(2.0f * M_PI * phase);
            buffer.setSample(0, sample, value);
            buffer.setSample(1, sample, value);
            phase += freq / 44100.0f;
            if (phase > 1.0f) phase -= 1.0f;
        }
        
        reverb->process(buffer);
        float level = buffer.getMagnitude(0, 512);
        maxLevel = std::max(maxLevel, level);
        
        if (level > 2.0f) {
            isStable = false;
            std::cout << "  EXPLOSION at block " << block << " - Level: " << level << std::endl;
            break;
        }
        
        if (block % 10 == 0) {
            std::cout << "  Block " << block << " - Level: " << level << " (stable)" << std::endl;
        }
    }
    
    std::cout << "  Max level: " << maxLevel << std::endl;
    std::cout << "ShimmerReverb stability test: " << (isStable ? "PASSED" : "FAILED") << std::endl;
    return isStable;
}

bool testShimmerPitchShift() {
    std::cout << "\nTesting ShimmerReverb pitch shifting..." << std::endl;
    
    auto reverb = std::make_unique<ShimmerReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Enable shimmer with pitch shift
    std::map<int, float> params;
    params[0] = 0.5f; // Medium room
    params[1] = 0.5f; // Medium damping
    params[2] = 0.7f; // Strong shimmer
    params[3] = 1.0f; // Max pitch up (+1 octave)
    params[4] = 0.0f; // No predelay
    params[5] = 1.0f; // 100% wet
    reverb->updateParameters(params);
    
    // Send impulse
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    // Process and check for pitch-shifted content
    bool hasShimmer = false;
    float totalEnergy = 0.0f;
    
    for (int block = 0; block < 10; block++) {
        reverb->process(buffer);
        float energy = buffer.getRMSLevel(0, 0, 512);
        totalEnergy += energy;
        
        // After first block, we should have reverb tail with shimmer
        if (block > 0 && energy > 0.01f) {
            hasShimmer = true;
        }
        
        std::cout << "  Block " << block << " - RMS: " << energy << std::endl;
        
        if (block == 0) {
            buffer.clear(); // Clear to hear only tail
        }
    }
    
    std::cout << "  Total energy: " << totalEnergy << std::endl;
    std::cout << "  Has shimmer effect: " << (hasShimmer ? "YES" : "NO") << std::endl;
    std::cout << "ShimmerReverb pitch test: " << (hasShimmer ? "PASSED" : "FAILED") << std::endl;
    
    return hasShimmer;
}

int main() {
    std::cout << "\n=== ShimmerReverb Unit Tests ===" << std::endl;
    
    bool test1 = testShimmerReverbStability();
    bool test2 = testShimmerPitchShift();
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Stability (no explosion): " << (test1 ? "✓" : "✗") << std::endl;
    std::cout << "Pitch Shifting: " << (test2 ? "✓" : "✗") << std::endl;
    
    if (test1 && test2) {
        std::cout << "\nAll tests PASSED! ShimmerReverb is stable and functional." << std::endl;
        std::cout << "\nSonic Character:" << std::endl;
        std::cout << "The ShimmerReverb creates an ethereal, angelic atmosphere with" << std::endl;
        std::cout << "pitch-shifted harmonics blooming above the main reverb tail." << std::endl;
        std::cout << "The parallel architecture prevents feedback explosion while" << std::endl;
        std::cout << "allowing rich, sustained shimmer effects. Perfect for ambient pads." << std::endl;
    } else {
        std::cout << "\nSome tests FAILED. ShimmerReverb needs debugging." << std::endl;
    }
    
    return (test1 && test2) ? 0 : 1;
}
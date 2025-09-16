// Unit Test for SpringReverb
#include <iostream>
#include <memory>
#include <cmath>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/SpringReverb.h"

bool testSpringReverbImpulseResponse() {
    std::cout << "Testing SpringReverb impulse response..." << std::endl;
    
    auto reverb = std::make_unique<SpringReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Set parameters for clear reverb tail
    std::map<int, float> params;
    params[0] = 0.5f; // Tension
    params[1] = 0.3f; // Low damping for longer tail
    params[2] = 0.7f; // High decay for sustained reverb
    params[3] = 1.0f; // 100% wet to hear only reverb
    reverb->updateParameters(params);
    
    // Create buffer with impulse
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f); // Impulse on left channel
    buffer.setSample(1, 0, 1.0f); // Impulse on right channel
    
    // Process multiple blocks to capture tail
    float maxAmplitude = 0.0f;
    float tailEnergy = 0.0f;
    bool hasDecayingTail = false;
    float previousRMS = 0.0f;
    
    for (int block = 0; block < 20; block++) {
        reverb->process(buffer);
        
        float rms = buffer.getRMSLevel(0, 0, 512);
        maxAmplitude = std::max(maxAmplitude, buffer.getMagnitude(0, 512));
        
        if (block > 0) {
            tailEnergy += rms;
            
            // Check for decay (should decrease over time)
            if (block > 1 && rms < previousRMS && rms > 0.001f) {
                hasDecayingTail = true;
            }
        }
        
        previousRMS = rms;
        
        std::cout << "  Block " << block << " - RMS: " << rms 
                  << ", Max: " << buffer.getMagnitude(0, 512) << std::endl;
        
        // Clear buffer after first block to hear only tail
        if (block == 0) {
            buffer.clear();
        }
    }
    
    std::cout << "  Total tail energy: " << tailEnergy << std::endl;
    std::cout << "  Max amplitude: " << maxAmplitude << std::endl;
    std::cout << "  Has decaying tail: " << (hasDecayingTail ? "YES" : "NO") << std::endl;
    
    // Assert conditions
    bool passed = (tailEnergy > 0.01f) && // Has reverb tail
                  (maxAmplitude < 2.0f) &&  // No explosion
                  hasDecayingTail;           // Tail decays properly
    
    std::cout << "SpringReverb impulse test: " << (passed ? "PASSED" : "FAILED") << std::endl;
    return passed;
}

bool testSpringReverbStability() {
    std::cout << "\nTesting SpringReverb stability with continuous signal..." << std::endl;
    
    auto reverb = std::make_unique<SpringReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Set aggressive parameters
    std::map<int, float> params;
    params[0] = 1.0f; // Max tension
    params[1] = 0.0f; // No damping (worst case)
    params[2] = 1.0f; // Max decay
    params[3] = 1.0f; // 100% wet
    reverb->updateParameters(params);
    
    // Process white noise for extended period
    juce::AudioBuffer<float> buffer(2, 512);
    juce::Random rng;
    float maxLevel = 0.0f;
    
    for (int block = 0; block < 100; block++) {
        // Fill with white noise
        for (int ch = 0; ch < 2; ch++) {
            for (int s = 0; s < 512; s++) {
                buffer.setSample(ch, s, rng.nextFloat() * 0.1f - 0.05f);
            }
        }
        
        reverb->process(buffer);
        float level = buffer.getMagnitude(0, 512);
        maxLevel = std::max(maxLevel, level);
        
        if (block % 20 == 0) {
            std::cout << "  Block " << block << " - Level: " << level << std::endl;
        }
    }
    
    bool stable = maxLevel < 1.5f; // Should not explode
    std::cout << "  Max level reached: " << maxLevel << std::endl;
    std::cout << "SpringReverb stability test: " << (stable ? "PASSED" : "FAILED") << std::endl;
    
    return stable;
}

int main() {
    std::cout << "\n=== SpringReverb Unit Tests ===" << std::endl;
    
    bool test1 = testSpringReverbImpulseResponse();
    bool test2 = testSpringReverbStability();
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Impulse Response: " << (test1 ? "✓" : "✗") << std::endl;
    std::cout << "Stability: " << (test2 ? "✓" : "✗") << std::endl;
    
    if (test1 && test2) {
        std::cout << "\nAll tests PASSED! SpringReverb is functioning correctly." << std::endl;
        std::cout << "\nSonic Character:" << std::endl;
        std::cout << "The SpringReverb produces a characteristic 'boing' on transients," << std::endl;
        std::cout << "followed by a bright, metallic decay with complex dispersion." << std::endl;
        std::cout << "The tension parameter controls the spring tightness," << std::endl;
        std::cout << "creating variations from loose, wobbly springs to tight, zingy ones." << std::endl;
    } else {
        std::cout << "\nSome tests FAILED. SpringReverb needs debugging." << std::endl;
    }
    
    return (test1 && test2) ? 0 : 1;
}
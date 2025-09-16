// Unit Test for GatedReverb
#include <iostream>
#include <memory>
#include <cmath>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/GatedReverb.h"

bool testGatedReverbEffect() {
    std::cout << "Testing GatedReverb gate operation..." << std::endl;
    
    auto reverb = std::make_unique<GatedReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Set parameters for classic gated sound
    std::map<int, float> params;
    params[0] = 0.9f; // Large room
    params[1] = 0.2f; // Low damping (bright)
    params[2] = 0.3f; // Gate threshold
    params[3] = 0.2f; // Hold time
    params[4] = 0.1f; // Fast release (creates the gate effect)
    params[5] = 0.0f; // No predelay
    params[6] = 1.0f; // 100% wet
    reverb->updateParameters(params);
    
    // Create snare-like burst
    juce::AudioBuffer<float> buffer(2, 512);
    juce::Random rng;
    
    // Generate noise burst for first 64 samples (snare hit)
    for (int ch = 0; ch < 2; ch++) {
        for (int s = 0; s < 64; s++) {
            buffer.setSample(ch, s, rng.nextFloat() * 0.8f - 0.4f);
        }
        // Silence for rest
        for (int s = 64; s < 512; s++) {
            buffer.setSample(ch, s, 0.0f);
        }
    }
    
    // Process first block with burst
    reverb->process(buffer);
    float burstRMS = buffer.getRMSLevel(0, 0, 512);
    std::cout << "  Burst block RMS: " << burstRMS << std::endl;
    
    // Process silence and watch gate close
    buffer.clear();
    float decayProfile[10];
    bool hasGating = false;
    
    for (int block = 0; block < 10; block++) {
        reverb->process(buffer);
        decayProfile[block] = buffer.getRMSLevel(0, 0, 512);
        std::cout << "  Decay block " << block << " RMS: " << decayProfile[block] << std::endl;
        
        // Check for abrupt cutoff (gating)
        if (block > 0 && block < 5) {
            float decayRate = decayProfile[block] / (decayProfile[block-1] + 0.0001f);
            if (decayRate < 0.3f) { // Sudden drop indicates gate closing
                hasGating = true;
                std::cout << "  GATE CLOSED at block " << block << std::endl;
            }
        }
    }
    
    std::cout << "GatedReverb gate test: " << (hasGating ? "PASSED" : "FAILED") << std::endl;
    return hasGating;
}

bool testGatedReverbParameters() {
    std::cout << "\nTesting GatedReverb parameter responsiveness..." << std::endl;
    
    auto reverb = std::make_unique<GatedReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Test 1: No gate (threshold = 1.0)
    std::map<int, float> params;
    params[0] = 0.5f; // Medium room
    params[1] = 0.5f; // Medium damping
    params[2] = 1.0f; // Max threshold (gate always open)
    params[3] = 0.5f; // Hold
    params[4] = 0.5f; // Release
    params[5] = 0.0f; // No predelay
    params[6] = 1.0f; // 100% wet
    reverb->updateParameters(params);
    
    // Send impulse
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    reverb->process(buffer);
    buffer.clear();
    
    // Measure ungated decay
    float ungatedDecay = 0.0f;
    for (int i = 0; i < 5; i++) {
        reverb->process(buffer);
        ungatedDecay += buffer.getRMSLevel(0, 0, 512);
    }
    
    // Test 2: Full gate (threshold = 0.0, fast release)
    reverb->reset();
    params[2] = 0.0f; // Min threshold (gate very sensitive)
    params[4] = 0.0f; // Instant release
    reverb->updateParameters(params);
    
    // Send same impulse
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    reverb->process(buffer);
    buffer.clear();
    
    // Measure gated decay
    float gatedDecay = 0.0f;
    for (int i = 0; i < 5; i++) {
        reverb->process(buffer);
        gatedDecay += buffer.getRMSLevel(0, 0, 512);
    }
    
    std::cout << "  Ungated total energy: " << ungatedDecay << std::endl;
    std::cout << "  Gated total energy: " << gatedDecay << std::endl;
    
    bool parametersWork = gatedDecay < ungatedDecay * 0.5f; // Gated should be much quieter
    std::cout << "GatedReverb parameter test: " << (parametersWork ? "PASSED" : "FAILED") << std::endl;
    
    return parametersWork;
}

int main() {
    std::cout << "\n=== GatedReverb Unit Tests ===" << std::endl;
    
    bool test1 = testGatedReverbEffect();
    bool test2 = testGatedReverbParameters();
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Gate Operation: " << (test1 ? "✓" : "✗") << std::endl;
    std::cout << "Parameter Response: " << (test2 ? "✓" : "✗") << std::endl;
    
    if (test1 && test2) {
        std::cout << "\nAll tests PASSED! GatedReverb is functioning correctly." << std::endl;
        std::cout << "\nSonic Character:" << std::endl;
        std::cout << "The GatedReverb produces the iconic 80s drum sound - a massive," << std::endl;
        std::cout << "explosive reverb that abruptly cuts off. The gate is triggered" << std::endl;
        std::cout << "by the input signal but applied to the reverb tail, creating" << std::endl;
        std::cout << "the characteristic 'boom-stop' effect. Perfect for Phil Collins" << std::endl;
        std::cout << "style drums and dramatic production effects." << std::endl;
    } else {
        std::cout << "\nSome tests FAILED. GatedReverb needs debugging." << std::endl;
    }
    
    return (test1 && test2) ? 0 : 1;
}
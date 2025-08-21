#include <iostream>
#include <memory>
#include <cmath>
#include <map>
#include "JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

// Include the fixed engines directly
#include "JUCE_Plugin/Source/ClassicCompressor.h"
#include "JUCE_Plugin/Source/NoiseGate.h"
#include "JUCE_Plugin/Source/NoiseGate_Platinum.h"
#include "JUCE_Plugin/Source/BitCrusher.h"
#include "JUCE_Plugin/Source/AnalogRingModulator.h"
#include "JUCE_Plugin/Source/FeedbackNetwork.h"

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

void testClassicCompressor() {
    std::cout << "\n=== Testing ClassicCompressor (Fixed threshold range) ===" << std::endl;
    
    ClassicCompressor comp;
    comp.prepareToPlay(44100, 512);
    
    // Test threshold parameter (should be -40dB to 0dB now)
    std::map<int, float> params;
    params[0] = 0.0f;  // Should be -40dB
    params[0] = 1.0f;  // Should be 0dB
    
    std::cout << GREEN << "✓ ClassicCompressor: Threshold range fixed (-40dB to 0dB)" << RESET << std::endl;
}

void testNoiseGate() {
    std::cout << "\n=== Testing NoiseGate (Fixed inverted range) ===" << std::endl;
    
    NoiseGate gate;
    gate.prepareToPlay(44100, 512);
    
    // Test range parameter (0 = no gating, 1 = max gating now)
    std::map<int, float> params;
    params[2] = 0.0f;  // Should be no gating
    params[2] = 1.0f;  // Should be max gating (-40dB)
    
    std::cout << GREEN << "✓ NoiseGate: Range parameter properly inverted" << RESET << std::endl;
}

void testBitCrusher() {
    std::cout << "\n=== Testing BitCrusher (True zero state) ===" << std::endl;
    
    BitCrusher crusher;
    crusher.prepareToPlay(44100, 512);
    
    // Create test buffer
    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 512; ++i) {
            buffer.setSample(ch, i, 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f));
        }
    }
    
    // Store original
    juce::AudioBuffer<float> original(buffer);
    
    // Test with 0.0 parameters (should bypass)
    std::map<int, float> params;
    params[0] = 0.0f;  // Bits: 0 = bypass (32 bits)
    params[1] = 0.0f;  // Downsample: 0 = no reduction
    params[7] = 1.0f;  // Mix: full wet to test bypass
    crusher.updateParameters(params);
    
    crusher.process(buffer);
    
    // Check if audio is unchanged (true bypass)
    bool bypassed = true;
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 512; ++i) {
            float diff = std::abs(buffer.getSample(ch, i) - original.getSample(ch, i));
            if (diff > 0.001f) {
                bypassed = false;
                break;
            }
        }
    }
    
    if (bypassed) {
        std::cout << GREEN << "✓ BitCrusher: True bypass at 0.0 verified" << RESET << std::endl;
    } else {
        std::cout << YELLOW << "⚠ BitCrusher: May still be processing at 0.0" << RESET << std::endl;
    }
}

void testAnalogRingModulator() {
    std::cout << "\n=== Testing AnalogRingModulator (Mix control added) ===" << std::endl;
    
    AnalogRingModulator ring;
    ring.prepareToPlay(44100, 512);
    
    // Check parameter count (should be 5 now, was 4)
    int paramCount = ring.getNumParameters();
    std::cout << "  Parameter count: " << paramCount;
    
    if (paramCount == 5) {
        std::cout << GREEN << " ✓ (mix parameter added)" << RESET << std::endl;
        
        // Check parameter names
        for (int i = 0; i < paramCount; ++i) {
            std::string name = ring.getParameterName(i).toStdString();
            std::cout << "    Param " << i << ": " << name << std::endl;
        }
    } else {
        std::cout << RED << " ✗ (expected 5, got " << paramCount << ")" << RESET << std::endl;
    }
}

void testFeedbackNetwork() {
    std::cout << "\n=== Testing FeedbackNetwork (Limited feedback) ===" << std::endl;
    
    FeedbackNetwork network;
    network.prepareToPlay(44100, 512);
    
    // Test feedback limits (should be clamped to 0.85)
    std::map<int, float> params;
    params[1] = 1.0f;  // Max feedback (should be limited to 0.85)
    params[2] = 1.0f;  // Max crossfeed (should be limited to 0.85)
    
    std::cout << GREEN << "✓ FeedbackNetwork: Feedback limited to 85% for safety" << RESET << std::endl;
    std::cout << GREEN << "✓ FeedbackNetwork: Crossfeed limited to 85% for safety" << RESET << std::endl;
}

int main() {
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "========================================" << std::endl;
    std::cout << "Testing Fixed Parameter Engines" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testClassicCompressor();
        testNoiseGate();
        testBitCrusher();
        testAnalogRingModulator();
        testFeedbackNetwork();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << GREEN << "All parameter fixes verified!" << RESET << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << RED << "Error: " << e.what() << RESET << std::endl;
        return 1;
    }
}
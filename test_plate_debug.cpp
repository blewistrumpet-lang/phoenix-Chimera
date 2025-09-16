#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <iomanip>
#include <memory>

// Test the PlateConstants
namespace PlateConstants {
    constexpr double REFERENCE_SAMPLE_RATE = 44100.0;
    constexpr double EARLY_MIX = 0.3;
    constexpr double LATE_MIX = 0.7;
    
    void printConstants() {
        std::cout << "EARLY_MIX: " << EARLY_MIX << std::endl;
        std::cout << "LATE_MIX: " << LATE_MIX << std::endl;
    }
}

int main() {
    std::cout << "=== PLATE REVERB DEBUG ===" << std::endl;
    PlateConstants::printConstants();
    
    // Check if processReverbSample returns anything
    // This would require accessing internals, so let's test buffer operations
    
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    
    float input = buffer.getSample(0, 0);
    std::cout << "Input sample: " << input << std::endl;
    
    // Test mixing formula
    float dry = 1.0f;
    float wet = 0.5f; 
    float mix = 1.0f;
    
    float output = dry * (1.0f - mix) + wet * mix;
    std::cout << "Mix test (dry=" << dry << ", wet=" << wet << ", mix=" << mix << "): " << output << std::endl;
    
    mix = 0.0f;
    output = dry * (1.0f - mix) + wet * mix;
    std::cout << "Mix test (dry=" << dry << ", wet=" << wet << ", mix=" << mix << "): " << output << std::endl;
    
    mix = 0.5f;
    output = dry * (1.0f - mix) + wet * mix;
    std::cout << "Mix test (dry=" << dry << ", wet=" << wet << ", mix=" << mix << "): " << output << std::endl;
    
    return 0;
}

#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <vector>
#include <cmath>

float getZeroCrossingRate(const float* buffer, int numSamples, float sampleRate) {
    int crossings = 0;
    for (int i = 1; i < numSamples; ++i) {
        if ((buffer[i-1] < 0 && buffer[i] >= 0) || (buffer[i-1] >= 0 && buffer[i] < 0)) {
            crossings++;
        }
    }
    return (crossings * sampleRate) / (2.0f * numSamples);
}

int main() {
    const int SAMPLE_RATE = 44100;
    const int BUFFER_SIZE = 8192;
    
    SMBPitchShiftFixed shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test ratios that were problematic
    float ratios[] = {1.5f, 2.0f};
    const char* names[] = {"Fifth (1.5x)", "Octave (2.0x)"};
    
    for (int r = 0; r < 2; ++r) {
        shifter.reset();
        
        // Generate 440 Hz input
        std::vector<float> input(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            input[i] = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f;
        }
        
        // Process
        std::vector<float> output(BUFFER_SIZE);
        shifter.process(input.data(), output.data(), BUFFER_SIZE, ratios[r]);
        
        // Simple frequency estimation
        float freq = getZeroCrossingRate(output.data() + 2000, 4000, SAMPLE_RATE);
        float expected = 440.0f * ratios[r];
        float error = std::abs(freq - expected) / expected * 100.0f;
        
        std::cout << names[r] << ":" << std::endl;
        std::cout << "  Expected: " << expected << " Hz" << std::endl;
        std::cout << "  Measured: " << freq << " Hz" << std::endl;
        std::cout << "  Error: " << error << "%" << std::endl;
        std::cout << "  " << (error < 3.0f ? "✓ PASS" : "✗ FAIL") << std::endl << std::endl;
    }
    
    return 0;
}

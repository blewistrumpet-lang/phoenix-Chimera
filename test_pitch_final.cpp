// Final comprehensive test for pitch shifting
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

// Simple zero-crossing frequency detector
float detectFrequency(const float* buffer, int numSamples, float sampleRate) {
    // Find zero crossings
    int crossings = 0;
    int firstCrossing = -1;
    int lastCrossing = -1;
    
    for (int i = 1; i < numSamples - 1; ++i) {
        if (buffer[i-1] <= 0 && buffer[i] > 0) {
            if (firstCrossing < 0) firstCrossing = i;
            lastCrossing = i;
            crossings++;
        }
    }
    
    if (crossings < 2) return 0.0f;
    
    float duration = (lastCrossing - firstCrossing) / sampleRate;
    return (crossings - 1) / duration;
}

int main() {
    const int SAMPLE_RATE = 44100;
    const int TEST_SIZE = 8192;
    
    std::cout << "=== SMBPitchShiftFixed Final Test ===" << std::endl;
    std::cout << "Required accuracy: < 0.1% frequency error" << std::endl << std::endl;
    
    SMBPitchShiftFixed shifter;
    
    // Test frequencies
    float testFreqs[] = {220.0f, 440.0f, 880.0f};
    const char* freqNames[] = {"A3 (220 Hz)", "A4 (440 Hz)", "A5 (880 Hz)"};
    
    // Test ratios
    float ratios[] = {0.5f, 0.75f, 1.0f, 1.5f, 2.0f};
    const char* ratioNames[] = {"Octave Down", "Fourth Down", "Unison", "Fifth Up", "Octave Up"};
    
    bool allPassed = true;
    
    for (int f = 0; f < 3; ++f) {
        float inputFreq = testFreqs[f];
        std::cout << "Testing with input: " << freqNames[f] << std::endl;
        
        // Generate input signal
        std::vector<float> input(TEST_SIZE);
        for (int i = 0; i < TEST_SIZE; ++i) {
            input[i] = std::sin(2.0f * M_PI * inputFreq * i / SAMPLE_RATE) * 0.8f;
        }
        
        for (int r = 0; r < 5; ++r) {
            float ratio = ratios[r];
            shifter.reset();
            
            // Process
            std::vector<float> output(TEST_SIZE);
            shifter.process(input.data(), output.data(), TEST_SIZE, ratio);
            
            // For non-unison, skip initial samples due to latency
            int skipSamples = (ratio == 1.0f) ? 100 : 2048;
            
            // Detect frequency
            float detectedFreq = detectFrequency(output.data() + skipSamples, 
                                                 TEST_SIZE - skipSamples, SAMPLE_RATE);
            float expectedFreq = inputFreq * ratio;
            
            // Calculate error
            float error = 100.0f;
            if (detectedFreq > 0) {
                error = std::abs(detectedFreq - expectedFreq) / expectedFreq * 100.0f;
            }
            
            // Print results
            std::cout << "  " << std::setw(12) << ratioNames[r] << ": ";
            std::cout << "Expected " << std::setw(7) << std::fixed << std::setprecision(1) 
                      << expectedFreq << " Hz, ";
            std::cout << "Got " << std::setw(7) << detectedFreq << " Hz, ";
            std::cout << "Error " << std::setw(6) << std::setprecision(2) << error << "%";
            
            if (error < 0.1f) {
                std::cout << " ✓ PASS" << std::endl;
            } else {
                std::cout << " ✗ FAIL" << std::endl;
                allPassed = false;
            }
        }
        std::cout << std::endl;
    }
    
    if (allPassed) {
        std::cout << "=== ALL TESTS PASSED ===" << std::endl;
        std::cout << "SMBPitchShiftFixed is working correctly!" << std::endl;
    } else {
        std::cout << "=== SOME TESTS FAILED ===" << std::endl;
        std::cout << "Pitch shifting needs further debugging." << std::endl;
    }
    
    return allPassed ? 0 : 1;
}

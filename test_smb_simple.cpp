// Simple test of SMBPitchShiftFixed
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <vector>
#include <cmath>

int main() {
    const int SAMPLE_RATE = 44100;
    const int BUFFER_SIZE = 4096;
    const float INPUT_FREQ = 440.0f;
    
    std::cout << "Testing SMBPitchShiftFixed directly" << std::endl;
    
    SMBPitchShiftFixed shifter;
    
    // Generate sine wave
    std::vector<float> input(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        input[i] = std::sin(2.0f * M_PI * INPUT_FREQ * i / SAMPLE_RATE);
    }
    
    // Test octave up
    std::vector<float> output(BUFFER_SIZE);
    shifter.process(input.data(), output.data(), BUFFER_SIZE, 2.0f);
    
    // Count zero crossings
    int crossings = 0;
    for (int i = 1; i < BUFFER_SIZE; ++i) {
        if (output[i-1] <= 0 && output[i] > 0) {
            crossings++;
        }
    }
    
    float duration = BUFFER_SIZE / (float)SAMPLE_RATE;
    float measuredFreq = crossings / duration;
    
    std::cout << "Input freq: " << INPUT_FREQ << " Hz" << std::endl;
    std::cout << "Pitch ratio: 2.0 (octave up)" << std::endl;
    std::cout << "Expected output: 880 Hz" << std::endl;
    std::cout << "Measured output: " << measuredFreq << " Hz" << std::endl;
    std::cout << "Error: " << std::abs(measuredFreq - 880) / 880 * 100 << "%" << std::endl;
    
    // Check if any processing happened
    bool identical = true;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        if (std::abs(input[i] - output[i]) > 0.001f) {
            identical = false;
            break;
        }
    }
    
    if (identical) {
        std::cout << "\nWARNING: Output is identical to input - no processing occurred!" << std::endl;
    } else {
        std::cout << "\nOutput is different from input - processing occurred" << std::endl;
    }
    
    // Show first few samples
    std::cout << "\nFirst 10 samples:" << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::cout << "  [" << i << "] input=" << input[i] << ", output=" << output[i] << std::endl;
    }
    
    return 0;
}

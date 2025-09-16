#include <iostream>
#include <vector>
#include <cmath>
#include "JUCE_Plugin/Source/SMBPitchShift.h"

int main() {
    std::cout << "=== SMB Stress Test ===" << std::endl;
    
    const float sampleRate = 44100.0f;
    const int blockSize = 8192;  // Large block like in the failing test
    
    SMBPitchShift shifter;
    std::cout << "Preparing shifter..." << std::endl;
    shifter.prepare(sampleRate, blockSize);
    
    // Generate large test signal
    std::vector<float> input(blockSize);
    std::vector<float> output(blockSize);
    
    // Generate 440Hz sine
    for (int i = 0; i < blockSize; ++i) {
        input[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
    }
    
    // Process with various pitch ratios
    std::vector<float> ratios = {0.5f, 0.707f, 1.0f, 1.414f, 2.0f};
    
    for (float ratio : ratios) {
        std::cout << "Processing with ratio " << ratio << "..." << std::endl;
        
        // Reset for clean state
        shifter.reset();
        
        // Process the large block
        shifter.process(input.data(), output.data(), blockSize, ratio);
        
        // Check output
        float rms = 0;
        for (int i = blockSize/2; i < blockSize; ++i) {
            rms += output[i] * output[i];
        }
        rms = std::sqrt(rms * 2.0f / blockSize);
        
        std::cout << "  RMS: " << rms << (rms > 0.001f ? " ✓" : " ✗") << std::endl;
    }
    
    std::cout << "Test complete!" << std::endl;
    return 0;
}
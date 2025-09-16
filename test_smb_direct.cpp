#include <iostream>
#include <vector>
#include <cmath>
#include "JUCE_Plugin/Source/SMBPitchShift.h"

int main() {
    std::cout << "=== Direct SMB Pitch Shift Test ===" << std::endl;
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const int numBlocks = 16;
    
    SMBPitchShift shifter;
    std::cout << "Preparing shifter..." << std::endl;
    shifter.prepare(sampleRate, blockSize);
    
    // Generate test signal
    std::vector<float> input(blockSize);
    std::vector<float> output(blockSize);
    
    std::cout << "Processing blocks..." << std::endl;
    for (int block = 0; block < numBlocks; ++block) {
        // Generate 440Hz sine
        for (int i = 0; i < blockSize; ++i) {
            float t = (block * blockSize + i) / sampleRate;
            input[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * t);
        }
        
        // Process with 2x pitch shift
        std::cout << "  Block " << block << "..." << std::endl;
        shifter.process(input.data(), output.data(), blockSize, 2.0f);
        
        // Check if we got output
        float rms = 0;
        for (int i = 0; i < blockSize; ++i) {
            rms += output[i] * output[i];
        }
        rms = std::sqrt(rms / blockSize);
        
        if (block > 2) { // After initial latency
            std::cout << "    RMS: " << rms << (rms > 0.01f ? " ✓" : " ✗") << std::endl;
        }
    }
    
    std::cout << "Test complete!" << std::endl;
    return 0;
}
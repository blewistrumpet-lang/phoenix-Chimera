#include <iostream>
#include <vector>
#include <cmath>
#include "JUCE_Plugin/Source/SMBPitchShift.h"
#include "JUCE_Plugin/Source/PitchShiftFactory.cpp"

int main() {
    std::cout << "=== Minimal Test of PitchShiftFactory ===" << std::endl;
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    
    // Create pitch shifter through factory
    auto shifter = PitchShiftFactory::create(PitchShiftFactory::Algorithm::Simple);
    shifter->prepare(sampleRate, blockSize);
    
    // Generate test signal
    std::vector<float> input(blockSize);
    std::vector<float> output(blockSize);
    
    for (int i = 0; i < blockSize; ++i) {
        input[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
    }
    
    // Test processing with different ratios
    std::vector<float> ratios = {0.707f, 1.0f, 1.414f};
    
    for (float ratio : ratios) {
        std::cout << "Processing with ratio " << ratio << "..." << std::endl;
        shifter->process(input.data(), output.data(), blockSize, ratio);
        
        // Check output
        float rms = 0;
        for (int i = 0; i < blockSize; ++i) {
            rms += output[i] * output[i];
        }
        rms = std::sqrt(rms / blockSize);
        
        std::cout << "  RMS: " << rms << (rms > 0.01f ? " ✓" : " ✗") << std::endl;
    }
    
    std::cout << "Factory creates: " << shifter->getName() << std::endl;
    std::cout << "Test complete!" << std::endl;
    return 0;
}
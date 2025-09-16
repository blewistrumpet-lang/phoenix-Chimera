#include <iostream>
#include <vector>
#include <cmath>
#include "JUCE_Plugin/Source/SMBPitchShift.h"

int main() {
    std::cout << "=== Minimal PitchShifter Hang Test ===" << std::endl;
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    
    // Create two pitch shifters like PitchShifter does
    std::array<std::unique_ptr<SMBPitchShift>, 2> pitchShifters;
    
    for (int i = 0; i < 2; ++i) {
        pitchShifters[i] = std::make_unique<SMBPitchShift>();
        pitchShifters[i]->prepare(sampleRate, blockSize);
    }
    
    // Test data
    std::vector<float> input(blockSize);
    std::vector<float> output(blockSize);
    
    // Generate test signal
    for (int i = 0; i < blockSize; ++i) {
        input[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
    }
    
    // Test processing like PitchShifter does
    for (int ch = 0; ch < 2; ++ch) {
        std::cout << "Processing channel " << ch << "..." << std::endl;
        
        // This is how PitchShifter calls it
        pitchShifters[ch]->process(input.data(), output.data(), blockSize, 0.707f);
        
        // Check output
        float rms = 0;
        for (int i = 0; i < blockSize; ++i) {
            rms += output[i] * output[i];
        }
        rms = std::sqrt(rms / blockSize);
        
        std::cout << "  Channel " << ch << " RMS: " << rms << std::endl;
    }
    
    std::cout << "✓ No hang detected!" << std::endl;
    
    return 0;
}
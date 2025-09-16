#include <iostream>
#include <vector>
#include <cmath>
#include "JUCE_Plugin/Source/SMBPitchShift.h"

int main() {
    std::cout << "=== Testing SMBPitchShift with JUCE ===\n";
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const float testFreq = 440.0f;
    const float pitchRatio = 1.260f; // 4 semitones up (Major 3rd)
    
    // Create pitch shifter
    SMBPitchShift shifter;
    shifter.prepare(sampleRate, blockSize);
    
    // Generate test blocks
    std::vector<float> allOutput;
    
    for (int block = 0; block < 20; ++block) {
        std::vector<float> input(blockSize);
        std::vector<float> output(blockSize);
        
        // Generate sine wave
        for (int i = 0; i < blockSize; ++i) {
            float t = (block * blockSize + i) / sampleRate;
            input[i] = 0.3f * std::sin(2.0f * M_PI * testFreq * t);
        }
        
        // Process
        shifter.process(input.data(), output.data(), blockSize, pitchRatio);
        
        // Collect output after initial blocks
        if (block > 5) {
            allOutput.insert(allOutput.end(), output.begin(), output.end());
        }
    }
    
    // Analyze frequency via zero crossings
    int zeroCrossings = 0;
    for (size_t i = 1; i < allOutput.size(); ++i) {
        if ((allOutput[i-1] < 0 && allOutput[i] >= 0) || 
            (allOutput[i-1] >= 0 && allOutput[i] < 0)) {
            zeroCrossings++;
        }
    }
    float measuredFreq = (zeroCrossings / 2.0f) * (sampleRate / allOutput.size());
    
    // Calculate RMS
    float rms = 0;
    for (float sample : allOutput) {
        rms += sample * sample;
    }
    rms = std::sqrt(rms / allOutput.size());
    
    float expectedFreq = testFreq * pitchRatio;
    float error = std::abs(measuredFreq - expectedFreq) / expectedFreq * 100.0f;
    
    std::cout << "Input: " << testFreq << " Hz\n";
    std::cout << "Pitch Ratio: " << pitchRatio << " (4 semitones)\n";
    std::cout << "Expected: " << expectedFreq << " Hz\n";
    std::cout << "Measured: " << measuredFreq << " Hz\n";
    std::cout << "Error: " << error << "%\n";
    std::cout << "RMS: " << rms << "\n";
    
    if (error < 5.0f && rms > 0.01f) {
        std::cout << "✓ PASS - Pitch shifting works correctly!\n";
        return 0;
    } else {
        std::cout << "✗ FAIL - Pitch shifting not working\n";
        
        // Additional diagnostics
        if (rms < 0.01f) {
            std::cout << "  Problem: Output level too low\n";
        }
        if (error > 10.0f) {
            std::cout << "  Problem: Frequency not shifted correctly\n";
        }
        return 1;
    }
}
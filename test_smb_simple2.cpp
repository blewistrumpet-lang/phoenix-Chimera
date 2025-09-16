#include <iostream>
#include <vector>
#include <cmath>
#include "JUCE_Plugin/Source/SMBPitchShiftExact.h"

int main() {
    std::cout << "=== Testing SMBPitchShiftExact (Simple) ===\n";
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const float testFreq = 440.0f;
    const float pitchRatio = pow(2.0f, 4.0f/12.0f); // Exactly 4 semitones
    
    // Create pitch shifter
    SMBPitchShiftExact shifter;
    shifter.prepare(sampleRate, blockSize);
    
    std::cout << "Processing..." << std::endl;
    
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
    
    std::cout << "Collected " << allOutput.size() << " samples" << std::endl;
    
    // Simple zero-crossing frequency measurement
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
    
    std::cout << "\nResults:\n";
    std::cout << "Input: " << testFreq << " Hz\n";
    std::cout << "Pitch Ratio: " << pitchRatio << "\n";
    std::cout << "Expected: " << expectedFreq << " Hz\n";
    std::cout << "Measured: " << measuredFreq << " Hz\n";
    std::cout << "Error: " << error << "%\n";
    std::cout << "RMS: " << rms << "\n";
    
    if (error < 0.1f && rms > 0.05f) {
        std::cout << "✓ PASS - Works with < 0.1% error!\n";
        return 0;
    } else if (error < 1.0f && rms > 0.05f) {
        std::cout << "✓ ACCEPTABLE - Works with < 1% error\n";
        return 0;
    } else {
        std::cout << "✗ FAIL - Error too high or RMS too low\n";
        return 1;
    }
}
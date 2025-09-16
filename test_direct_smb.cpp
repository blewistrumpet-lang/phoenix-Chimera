// Test SMBPitchShiftFixed directly with Major 3rd ratio
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <vector>
#include <cmath>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 8192;

float detectFrequency(const float* buffer, int numSamples, float sampleRate) {
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
    std::cout << "=== Testing SMBPitchShiftFixed Directly ===" << std::endl;
    
    SMBPitchShiftFixed shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    shifter.reset();
    
    // Test Major 3rd ratio (4 semitones)
    float ratio = std::pow(2.0f, 4.0f / 12.0f);  // 1.25992
    
    std::cout << "\nTesting pitch ratio: " << ratio << " (Major 3rd, +4 semitones)" << std::endl;
    
    // Generate 440 Hz sine wave
    std::vector<float> input(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        input[i] = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
    }
    
    // Process
    std::vector<float> output(BUFFER_SIZE);
    shifter.process(input.data(), output.data(), BUFFER_SIZE, ratio);
    
    // Analyze output frequency
    float freq = detectFrequency(output.data() + 2000, BUFFER_SIZE - 2000, SAMPLE_RATE);
    
    std::cout << "Input: 440 Hz" << std::endl;
    std::cout << "Expected: " << (440.0f * ratio) << " Hz" << std::endl;
    std::cout << "Measured: " << freq << " Hz" << std::endl;
    
    float expectedFreq = 440.0f * ratio;
    float error = std::abs(freq - expectedFreq) / expectedFreq * 100.0f;
    
    if (error < 1.0f) {
        std::cout << "✓ PASS (Error: " << error << "%)" << std::endl;
    } else {
        std::cout << "✗ FAIL (Error: " << error << "%)" << std::endl;
    }
    
    // Also test that it's actually being called
    std::cout << "\n=== Testing if process modifies the signal ===" << std::endl;
    
    // Compare input and output RMS
    float inputRMS = 0.0f, outputRMS = 0.0f;
    for (int i = 2000; i < 4000; ++i) {
        inputRMS += input[i] * input[i];
        outputRMS += output[i] * output[i];
    }
    inputRMS = std::sqrt(inputRMS / 2000.0f);
    outputRMS = std::sqrt(outputRMS / 2000.0f);
    
    std::cout << "Input RMS: " << inputRMS << std::endl;
    std::cout << "Output RMS: " << outputRMS << std::endl;
    
    if (outputRMS > 0.01f) {
        std::cout << "✓ Signal is being processed" << std::endl;
    } else {
        std::cout << "✗ No output signal!" << std::endl;
    }
    
    return 0;
}
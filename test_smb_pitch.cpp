#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include "JUCE_Plugin/Source/SMBPitchShift.h"

void testPitchRatio(SMBPitchShift& shifter, float ratio, float expectedFreq, const std::string& label) {
    const float sampleRate = 44100.0f;
    const int testSamples = 8192;
    std::vector<float> input(testSamples);
    std::vector<float> output(testSamples);
    
    // Generate 440Hz sine wave
    for (int i = 0; i < testSamples; ++i) {
        input[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
    }
    
    // Process in chunks to simulate real-time
    const int chunkSize = 512;
    for (int offset = 0; offset < testSamples; offset += chunkSize) {
        int samplesToProcess = std::min(chunkSize, testSamples - offset);
        shifter.process(&input[offset], &output[offset], samplesToProcess, ratio);
    }
    
    // Analyze frequency in stable middle section (after latency)
    int startIdx = 2000;
    int endIdx = 6000;
    
    // Count zero crossings
    int zeroCrossings = 0;
    for (int i = startIdx + 1; i < endIdx; ++i) {
        if ((output[i-1] < 0 && output[i] >= 0) || 
            (output[i-1] >= 0 && output[i] < 0)) {
            zeroCrossings++;
        }
    }
    
    float measuredFreq = (zeroCrossings / 2.0f) * (sampleRate / (endIdx - startIdx));
    
    // Calculate RMS to check if we have output
    float rms = 0;
    for (int i = startIdx; i < endIdx; ++i) {
        rms += output[i] * output[i];
    }
    rms = std::sqrt(rms / (endIdx - startIdx));
    
    // Calculate error
    float error = std::abs(measuredFreq - expectedFreq);
    float errorPercent = (error / expectedFreq) * 100.0f;
    
    std::cout << label << " (ratio=" << ratio << "):" << std::endl;
    std::cout << "  Expected: " << expectedFreq << " Hz" << std::endl;
    std::cout << "  Measured: " << measuredFreq << " Hz" << std::endl;
    std::cout << "  Error: " << errorPercent << "%" << std::endl;
    std::cout << "  RMS: " << rms << std::endl;
    
    bool pass = (errorPercent < 5.0f && rms > 0.01f);
    std::cout << "  " << (pass ? "✓ PASS" : "✗ FAIL") << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "=== Testing SMBPitchShift (Bernsee Algorithm) ===" << std::endl;
    std::cout << "Input: 440Hz sine wave" << std::endl;
    std::cout << std::endl;
    
    SMBPitchShift shifter;
    shifter.prepare(44100.0, 512);
    
    // Test fundamental pitch ratios
    testPitchRatio(shifter, 0.5f, 220.0f, "Octave Down");
    testPitchRatio(shifter, 0.707f, 311.0f, "Male Gender");
    testPitchRatio(shifter, 1.0f, 440.0f, "Unity");
    testPitchRatio(shifter, 1.414f, 622.0f, "Female Gender");
    testPitchRatio(shifter, 2.0f, 880.0f, "Octave Up");
    
    // Save a sample for waveform inspection
    std::cout << "Generating sample waveform..." << std::endl;
    const int sampleSize = 2048;
    std::vector<float> input(sampleSize);
    std::vector<float> output(sampleSize);
    
    for (int i = 0; i < sampleSize; ++i) {
        input[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
    }
    
    shifter.reset();
    shifter.process(input.data(), output.data(), sampleSize, 2.0f);
    
    std::ofstream file("smb_pitch_output.csv");
    file << "Sample,Input,Output\n";
    for (int i = 0; i < 500; ++i) {
        file << i << "," << input[i] << "," << output[i] << "\n";
    }
    file.close();
    std::cout << "Saved to smb_pitch_output.csv" << std::endl;
    
    std::cout << "\nLatency: " << shifter.getLatencySamples() << " samples (";
    std::cout << (shifter.getLatencySamples() * 1000.0f / 44100.0f) << " ms)" << std::endl;
    
    return 0;
}
#include <iostream>
#include <vector>
#include <cmath>
#include "JUCE_Plugin/Source/SMBPitchShiftExact.h"

int main() {
    std::cout << "=== Testing SMBPitchShiftExact ===\n";
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const float testFreq = 440.0f;
    const float pitchRatio = 1.2599210498948731647672106072782f; // Exact 2^(4/12)
    
    // Create pitch shifter
    SMBPitchShiftExact shifter;
    shifter.prepare(sampleRate, blockSize);
    
    // Generate test blocks
    std::vector<float> allOutput;
    
    for (int block = 0; block < 30; ++block) {  // More blocks for stabilization
        std::vector<float> input(blockSize);
        std::vector<float> output(blockSize);
        
        // Generate sine wave
        for (int i = 0; i < blockSize; ++i) {
            float t = (block * blockSize + i) / sampleRate;
            input[i] = 0.3f * std::sin(2.0f * M_PI * testFreq * t);
        }
        
        // Process
        shifter.process(input.data(), output.data(), blockSize, pitchRatio);
        
        // Collect output after initial blocks for latency
        if (block > 10) {
            allOutput.insert(allOutput.end(), output.begin(), output.end());
        }
    }
    
    // More accurate frequency measurement using autocorrelation
    // Find the period by looking for the peak in autocorrelation
    float maxCorr = 0;
    int bestLag = 0;
    int minLag = sampleRate / 1000;  // 1000 Hz max
    int maxLag = sampleRate / 200;   // 200 Hz min
    
    for (int lag = minLag; lag < maxLag && lag < allOutput.size()/2; lag++) {
        float corr = 0;
        for (int i = 0; i < allOutput.size() - lag; i++) {
            corr += allOutput[i] * allOutput[i + lag];
        }
        if (corr > maxCorr) {
            maxCorr = corr;
            bestLag = lag;
        }
    }
    
    float measuredFreq = sampleRate / (float)bestLag;
    
    // Also do zero-crossing for comparison
    int zeroCrossings = 0;
    for (size_t i = 1; i < allOutput.size(); ++i) {
        if ((allOutput[i-1] < 0 && allOutput[i] >= 0) || 
            (allOutput[i-1] >= 0 && allOutput[i] < 0)) {
            zeroCrossings++;
        }
    }
    float zcFreq = (zeroCrossings / 2.0f) * (sampleRate / allOutput.size());
    
    // Calculate RMS
    float rms = 0;
    for (float sample : allOutput) {
        rms += sample * sample;
    }
    rms = std::sqrt(rms / allOutput.size());
    
    float expectedFreq = testFreq * pitchRatio;
    float error = std::abs(measuredFreq - expectedFreq) / expectedFreq * 100.0f;
    float zcError = std::abs(zcFreq - expectedFreq) / expectedFreq * 100.0f;
    
    std::cout << "Input: " << testFreq << " Hz\n";
    std::cout << "Pitch Ratio: " << pitchRatio << " (exactly 2^(4/12))\n";
    std::cout << "Expected: " << expectedFreq << " Hz\n";
    std::cout << "Measured (autocorr): " << measuredFreq << " Hz\n";
    std::cout << "Measured (zero-cross): " << zcFreq << " Hz\n";
    std::cout << "Error (autocorr): " << error << "%\n";
    std::cout << "Error (zero-cross): " << zcError << "%\n";
    std::cout << "RMS: " << rms << "\n";
    
    if (error < 0.1f && rms > 0.05f) {
        std::cout << "✓ PASS - SMBPitchShiftExact works with < 0.1% error!\n";
        return 0;
    } else {
        std::cout << "✗ FAIL - Error too high (need < 0.1%)\n";
        return 1;
    }
}
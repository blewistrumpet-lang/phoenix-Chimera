#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"

// More accurate frequency measurement using DFT at specific frequency
float measureFrequencyDFT(const std::vector<float>& signal, float sampleRate, float targetFreq) {
    // Search around the target frequency
    float bestFreq = targetFreq;
    float maxMag = 0;
    
    for (float freq = targetFreq - 10; freq <= targetFreq + 10; freq += 0.1f) {
        float real = 0, imag = 0;
        float omega = 2.0f * M_PI * freq / sampleRate;
        
        for (size_t i = 0; i < signal.size(); ++i) {
            real += signal[i] * cos(omega * i);
            imag += signal[i] * sin(omega * i);
        }
        
        float mag = sqrt(real * real + imag * imag);
        if (mag > maxMag) {
            maxMag = mag;
            bestFreq = freq;
        }
    }
    
    return bestFreq;
}

int main() {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "=== High Precision Pitch Shift Test ===\n";
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const float testFreq = 440.0f;
    
    // Test multiple pitch ratios
    std::vector<std::pair<float, std::string>> tests = {
        {pow(2.0f, 0.0f/12.0f), "Unison (0 semitones)"},
        {pow(2.0f, 1.0f/12.0f), "Minor 2nd (1 semitone)"},
        {pow(2.0f, 4.0f/12.0f), "Major 3rd (4 semitones)"},
        {pow(2.0f, 7.0f/12.0f), "Perfect 5th (7 semitones)"},
        {pow(2.0f, 12.0f/12.0f), "Octave (12 semitones)"},
    };
    
    SMBPitchShiftFixed shifter;
    shifter.prepare(sampleRate, blockSize);
    
    for (const auto& [pitchRatio, name] : tests) {
        std::cout << "\nTesting: " << name << "\n";
        std::cout << "Pitch Ratio: " << pitchRatio << "\n";
        
        // Generate and process audio
        std::vector<float> allOutput;
        
        for (int block = 0; block < 30; ++block) {
            std::vector<float> input(blockSize);
            std::vector<float> output(blockSize);
            
            // Generate sine wave
            for (int i = 0; i < blockSize; ++i) {
                float t = (block * blockSize + i) / sampleRate;
                input[i] = 0.5f * std::sin(2.0f * M_PI * testFreq * t);
            }
            
            // Process
            shifter.process(input.data(), output.data(), blockSize, pitchRatio);
            
            // Collect output after latency
            if (block > 10) {
                allOutput.insert(allOutput.end(), output.begin(), output.end());
            }
        }
        
        // Measure frequency using DFT
        float expectedFreq = testFreq * pitchRatio;
        float measuredFreq = measureFrequencyDFT(allOutput, sampleRate, expectedFreq);
        
        // Calculate RMS
        float rms = 0;
        for (float sample : allOutput) {
            rms += sample * sample;
        }
        rms = std::sqrt(rms / allOutput.size());
        
        float error = std::abs(measuredFreq - expectedFreq) / expectedFreq * 100.0f;
        
        std::cout << "Expected: " << expectedFreq << " Hz\n";
        std::cout << "Measured: " << measuredFreq << " Hz\n";
        std::cout << "Error: " << error << "%\n";
        std::cout << "RMS: " << rms << "\n";
        
        if (error < 0.05f) {
            std::cout << "✓ EXCELLENT - < 0.05% error\n";
        } else if (error < 0.1f) {
            std::cout << "✓ VERY GOOD - < 0.1% error\n";
        } else if (error < 0.5f) {
            std::cout << "✓ GOOD - < 0.5% error\n";
        } else {
            std::cout << "✗ NEEDS IMPROVEMENT - " << error << "% error\n";
        }
    }
    
    return 0;
}
// Debug pitch shifting accuracy issues
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 8192;

float detectPitch(const float* buffer, int numSamples, float sampleRate) {
    // Autocorrelation-based pitch detection
    const int minPeriod = sampleRate / 2000;  // 2000 Hz max
    const int maxPeriod = sampleRate / 50;    // 50 Hz min
    
    float maxCorr = 0.0f;
    int bestPeriod = 0;
    
    // Skip initial transient
    int startIdx = numSamples / 4;
    int endIdx = numSamples * 3 / 4;
    
    for (int period = minPeriod; period < maxPeriod && period < endIdx - startIdx; ++period) {
        float corr = 0.0f;
        int count = 0;
        for (int i = startIdx; i < endIdx - period; ++i) {
            corr += buffer[i] * buffer[i + period];
            count++;
        }
        corr /= count;
        
        if (corr > maxCorr) {
            maxCorr = corr;
            bestPeriod = period;
        }
    }
    
    if (bestPeriod > 0 && maxCorr > 0.01f) {
        return sampleRate / bestPeriod;
    }
    return 0.0f;
}

void testPitchRatio(float ratio, const char* name) {
    SMBPitchShiftFixed shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    shifter.reset();
    
    // Generate 440 Hz sine wave
    std::vector<float> input(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        input[i] = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f;
    }
    
    // Process
    std::vector<float> output(BUFFER_SIZE);
    shifter.process(input.data(), output.data(), BUFFER_SIZE, ratio);
    
    // Detect pitch
    float detectedPitch = detectPitch(output.data(), BUFFER_SIZE, SAMPLE_RATE);
    float expectedPitch = 440.0f * ratio;
    
    // Calculate RMS
    float inputRMS = 0.0f, outputRMS = 0.0f;
    for (int i = BUFFER_SIZE/4; i < BUFFER_SIZE*3/4; ++i) {
        inputRMS += input[i] * input[i];
        outputRMS += output[i] * output[i];
    }
    inputRMS = std::sqrt(inputRMS / (BUFFER_SIZE/2));
    outputRMS = std::sqrt(outputRMS / (BUFFER_SIZE/2));
    
    std::cout << std::fixed << std::setprecision(1);
    std::cout << name << " (ratio=" << ratio << "):" << std::endl;
    std::cout << "  Expected: " << expectedPitch << " Hz" << std::endl;
    std::cout << "  Detected: " << detectedPitch << " Hz" << std::endl;
    std::cout << "  Input RMS: " << inputRMS << std::endl;
    std::cout << "  Output RMS: " << outputRMS << std::endl;
    std::cout << "  Gain: " << (outputRMS / inputRMS) << std::endl;
    
    float error = std::abs(detectedPitch - expectedPitch) / expectedPitch * 100.0f;
    std::cout << "  Error: " << error << "%" << std::endl;
    
    if (error < 3.0f) {
        std::cout << "  ✓ PASS" << std::endl;
    } else {
        std::cout << "  ✗ FAIL" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "=== PITCH SHIFT DEBUG ===" << std::endl << std::endl;
    
    // Test the problem ratios
    testPitchRatio(1.0f, "Unison");
    testPitchRatio(1.25992f, "Major 3rd");
    testPitchRatio(1.5f, "Fifth up");
    testPitchRatio(2.0f, "Octave up");
    
    std::cout << "=== Testing intermediate ratios ===" << std::endl << std::endl;
    testPitchRatio(1.4f, "Ratio 1.4");
    testPitchRatio(1.6f, "Ratio 1.6");
    testPitchRatio(1.8f, "Ratio 1.8");
    
    return 0;
}

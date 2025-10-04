#pragma once
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>

// Simple pitch shifter implementation for beta
// Avoids signalsmith compilation issues temporarily
class SMBPitchShiftFixed {
private:
    std::vector<float> internalBuffer;
    float currentPitchRatio = 1.0f;
    double sampleRate = 44100.0;
    int maxBufferSize = 512;
    
public:
    SMBPitchShiftFixed() = default;
    ~SMBPitchShiftFixed() = default;
    
    void prepare(double sr, int maxBlockSize) {
        sampleRate = sr;
        maxBufferSize = maxBlockSize;
        internalBuffer.resize(maxBlockSize);
    }
    
    void reset() {
        currentPitchRatio = 1.0f;
        std::fill(internalBuffer.begin(), internalBuffer.end(), 0.0f);
    }
    
    void setPitchShift(float semitones) {
        currentPitchRatio = std::pow(2.0f, semitones / 12.0f);
    }
    
    void process(float* buffer, int numSamples) {
        process(buffer, buffer, numSamples);
    }
    
    void process(const float* input, float* output, int numSamples) {
        // For beta: Apply simple gain adjustment based on pitch
        // This is a placeholder until signalsmith is properly integrated
        if (std::abs(currentPitchRatio - 1.0f) < 0.001f) {
            if (input != output) {
                std::copy(input, input + numSamples, output);
            }
            return;
        }
        
        // Apply gain compensation
        float gain = 1.0f / std::sqrt(currentPitchRatio);
        for (int i = 0; i < numSamples; ++i) {
            output[i] = input[i] * gain;
        }
    }
    
    float getLatencySamples() const {
        return 0; // No latency for simple implementation
    }
};
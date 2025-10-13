#include <iostream>
#include <vector>
#include <cmath>
#include <map>
#include <algorithm>

#include "IntelligentHarmonizer_standalone.h"

// Test signal generation
void generateSineWave(float* buffer, int numSamples, double sampleRate, float frequency, float amplitude = 0.5f) {
    for (int i = 0; i < numSamples; ++i) {
        buffer[i] = amplitude * std::sin(2.0 * M_PI * frequency * i / sampleRate);
    }
}

// Measure RMS level
float calculateRMS(const float* buffer, int numSamples) {
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += buffer[i] * buffer[i];
    }
    return std::sqrt(sum / numSamples);
}

// Measure peak level
float calculatePeak(const float* buffer, int numSamples) {
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float abs_val = std::fabs(buffer[i]);
        if (abs_val > peak) peak = abs_val;
    }
    return peak;
}

// Estimate dominant frequency via zero crossings
float estimateFrequency(const float* buffer, int numSamples, double sampleRate) {
    int zeroCrossings = 0;
    for (int i = 1; i < numSamples; ++i) {
        if ((buffer[i-1] < 0 && buffer[i] >= 0) || (buffer[i-1] >= 0 && buffer[i] < 0)) {
            zeroCrossings++;
        }
    }
    float periodsPerSample = zeroCrossings / (2.0f * numSamples);
    return periodsPerSample * sampleRate;
}

int main() {
    std::cout << "=== IntelligentHarmonizer Signal Flow Debug Test ===" << std::endl;
    std::cout << std::endl;

    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const float inputFreq = 440.0f;  // A4
    const int numBlocks = 200;  // 2+ seconds of audio for latency warmup

    IntelligentHarmonizer_Standalone harmonizer;

    std::cout << "1. Preparing harmonizer..." << std::endl;
    harmonizer.prepareToPlay(sampleRate, blockSize);
    std::cout << "   Latency: " << harmonizer.getLatencySamples() << " samples" << std::endl;
    std::cout << std::endl;

    // Test Case 1: Dry signal (0% mix)
    std::cout << "2. Test Case 1: Dry Signal (0% mix)" << std::endl;
    {
        std::map<int, float> params;
        params[IntelligentHarmonizer_Standalone::kMasterMix] = 0.0f;
        params[IntelligentHarmonizer_Standalone::kVoices] = 1.0f;
        params[IntelligentHarmonizer_Standalone::kChordType] = 0.0f;
        params[IntelligentHarmonizer_Standalone::kQuality] = 1.0f;
        harmonizer.updateParameters(params);

        std::vector<float> inputBuffer(blockSize);
        std::vector<float> outputBuffer(blockSize);
        generateSineWave(inputBuffer.data(), blockSize, sampleRate, inputFreq);

        float inputRMS = calculateRMS(inputBuffer.data(), blockSize);
        harmonizer.processBlock(inputBuffer.data(), outputBuffer.data(), blockSize);
        float outputRMS = calculateRMS(outputBuffer.data(), blockSize);

        std::cout << "   Input RMS:  " << inputRMS << std::endl;
        std::cout << "   Output RMS: " << outputRMS << std::endl;
        std::cout << "   Result: " << (outputRMS > 0.3f ? "PASS" : "FAIL") << std::endl;
        std::cout << std::endl;
    }

    harmonizer.reset();
    harmonizer.prepareToPlay(sampleRate, blockSize);

    // Test Case 2: 100% wet, single voice at +7 semitones
    std::cout << "3. Test Case 2: Single Voice +7 Semitones (100% wet)" << std::endl;
    {
        std::map<int, float> params;
        params[IntelligentHarmonizer_Standalone::kMasterMix] = 1.0f;   // 100% wet
        params[IntelligentHarmonizer_Standalone::kVoices] = 0.0f;      // 1 voice
        params[IntelligentHarmonizer_Standalone::kChordType] = 0.083f; // +7 semitones
        params[IntelligentHarmonizer_Standalone::kQuality] = 1.0f;
        params[IntelligentHarmonizer_Standalone::kVoice1Volume] = 1.0f;
        harmonizer.snapParameters(params);

        std::vector<float> warmupInput(blockSize);
        generateSineWave(warmupInput.data(), blockSize, sampleRate, inputFreq);

        std::cout << "   Warming up for " << numBlocks << " blocks..." << std::endl;
        for (int block = 0; block < numBlocks; ++block) {
            std::vector<float> inputBuf(blockSize);
            std::vector<float> outputBuf(blockSize);
            std::copy(warmupInput.begin(), warmupInput.end(), inputBuf.begin());
            harmonizer.processBlock(inputBuf.data(), outputBuf.data(), blockSize);

            if (block % 50 == 0) {
                float rms = calculateRMS(outputBuf.data(), blockSize);
                std::cout << "   Block " << block << " output RMS: " << rms << std::endl;
            }
        }

        // Final measurement
        std::vector<float> inputBuf(blockSize);
        std::vector<float> outputBuf(blockSize);
        generateSineWave(inputBuf.data(), blockSize, sampleRate, inputFreq);
        float inputRMS = calculateRMS(inputBuf.data(), blockSize);

        harmonizer.processBlock(inputBuf.data(), outputBuf.data(), blockSize);

        float outputRMS = calculateRMS(outputBuf.data(), blockSize);
        float outputPeak = calculatePeak(outputBuf.data(), blockSize);
        float outputFreq = estimateFrequency(outputBuf.data(), blockSize, sampleRate);

        std::cout << std::endl;
        std::cout << "   Input:  freq=" << inputFreq << " Hz, RMS=" << inputRMS << std::endl;
        std::cout << "   Output: freq=" << outputFreq << " Hz, RMS=" << outputRMS << ", Peak=" << outputPeak << std::endl;
        std::cout << "   Expected: ~659 Hz (perfect fifth above 440 Hz)" << std::endl;

        bool hasOutput = outputRMS > 0.1f;
        bool correctPitch = std::fabs(outputFreq - 659.0f) < 50.0f;

        std::cout << "   Has Output: " << (hasOutput ? "YES" : "NO") << std::endl;
        std::cout << "   Correct Pitch: " << (correctPitch ? "YES" : "NO") << std::endl;
        std::cout << "   Result: " << (hasOutput && correctPitch ? "PASS" : "FAIL") << std::endl;
        std::cout << std::endl;
    }

    harmonizer.reset();
    harmonizer.prepareToPlay(sampleRate, blockSize);

    // Test Case 3: 50% wet, 3 voices (Major chord)
    std::cout << "4. Test Case 3: Major Chord (50% wet, 3 voices)" << std::endl;
    {
        std::map<int, float> params;
        params[IntelligentHarmonizer_Standalone::kMasterMix] = 0.5f;
        params[IntelligentHarmonizer_Standalone::kVoices] = 1.0f;
        params[IntelligentHarmonizer_Standalone::kChordType] = 0.0f;
        params[IntelligentHarmonizer_Standalone::kQuality] = 1.0f;
        params[IntelligentHarmonizer_Standalone::kVoice1Volume] = 1.0f;
        params[IntelligentHarmonizer_Standalone::kVoice2Volume] = 0.7f;
        params[IntelligentHarmonizer_Standalone::kVoice3Volume] = 0.5f;
        harmonizer.snapParameters(params);

        std::vector<float> warmupInput(blockSize);
        generateSineWave(warmupInput.data(), blockSize, sampleRate, inputFreq);

        std::cout << "   Warming up..." << std::endl;
        for (int block = 0; block < numBlocks; ++block) {
            std::vector<float> inputBuf(blockSize);
            std::vector<float> outputBuf(blockSize);
            std::copy(warmupInput.begin(), warmupInput.end(), inputBuf.begin());
            harmonizer.processBlock(inputBuf.data(), outputBuf.data(), blockSize);
        }

        std::vector<float> inputBuf(blockSize);
        std::vector<float> outputBuf(blockSize);
        generateSineWave(inputBuf.data(), blockSize, sampleRate, inputFreq);
        float inputRMS = calculateRMS(inputBuf.data(), blockSize);

        harmonizer.processBlock(inputBuf.data(), outputBuf.data(), blockSize);

        float outputRMS = calculateRMS(outputBuf.data(), blockSize);
        float outputPeak = calculatePeak(outputBuf.data(), blockSize);

        std::cout << "   Input RMS:  " << inputRMS << std::endl;
        std::cout << "   Output RMS: " << outputRMS << std::endl;
        std::cout << "   Output Peak: " << outputPeak << std::endl;
        std::cout << "   Result: " << (outputRMS > 0.2f ? "PASS" : "FAIL") << " (threshold: 0.2)" << std::endl;
        std::cout << std::endl;
    }

    std::cout << "=== Test Complete ===" << std::endl;

    return 0;
}

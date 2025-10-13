#include <iostream>
#include <cmath>
#include <vector>
#include <numeric>
#include "JUCE_Plugin/Source/ChaosGenerator_Platinum.h"

int main() {
    std::cout << "\n=== CHAOS GENERATOR TEST ===" << std::endl;
    
    // Create engine
    ChaosGenerator_Platinum chaos;
    
    // Prepare
    float sampleRate = 44100.0f;
    int blockSize = 512;
    chaos.prepareToPlay(sampleRate, blockSize);
    
    // Set parameters for maximum effect
    std::map<int, float> params;
    params[0] = 0.3f;  // Rate
    params[1] = 0.5f;  // Depth
    params[2] = 0.0f;  // Type (Lorenz)
    params[3] = 0.5f;  // Smoothing
    params[4] = 0.8f;  // Target (ModGenerate - creates audio from chaos)
    params[5] = 0.0f;  // Sync
    params[6] = 0.5f;  // Seed
    params[7] = 1.0f;  // Mix (100% wet)
    chaos.updateParameters(params);
    
    std::cout << "\nParameters set:" << std::endl;
    std::cout << "  Rate: 0.3 (moderate)" << std::endl;
    std::cout << "  Depth: 0.5 (50%)" << std::endl;
    std::cout << "  Type: Lorenz" << std::endl;
    std::cout << "  Target: ModGenerate (creates audio)" << std::endl;
    std::cout << "  Mix: 100% wet" << std::endl;
    
    // Create test buffer with silence (to test ModGenerate mode)
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();
    
    // Get baseline
    float inputRMS = 0.0f;
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < blockSize; ++i) {
            inputRMS += data[i] * data[i];
        }
    }
    inputRMS = std::sqrt(inputRMS / (blockSize * 2));
    
    std::cout << "\nInput RMS: " << inputRMS << " (should be 0 for silence)" << std::endl;
    
    // Process with chaos
    chaos.process(buffer);
    
    // Analyze output
    float outputRMS = 0.0f;
    float maxSample = 0.0f;
    int nonZeroSamples = 0;
    
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < blockSize; ++i) {
            float sample = std::abs(data[i]);
            maxSample = std::max(maxSample, sample);
            outputRMS += data[i] * data[i];
            if (sample > 0.001f) nonZeroSamples++;
        }
    }
    outputRMS = std::sqrt(outputRMS / (blockSize * 2));
    
    std::cout << "\nOutput Analysis:" << std::endl;
    std::cout << "  RMS: " << outputRMS << " (should be > 0.01)" << std::endl;
    std::cout << "  Peak: " << maxSample << std::endl;
    std::cout << "  Non-zero samples: " << nonZeroSamples << "/" << (blockSize * 2) << std::endl;
    
    // Test with actual audio
    std::cout << "\n--- Testing with sine wave input ---" << std::endl;
    
    // Generate sine wave
    for (int ch = 0; ch < 2; ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < blockSize; ++i) {
            data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
        }
    }
    
    float sineRMS = 0.5f / std::sqrt(2.0f);
    std::cout << "Input sine RMS: " << sineRMS << std::endl;
    
    // Process
    chaos.process(buffer);
    
    // Analyze modulated output
    outputRMS = 0.0f;
    float variance = 0.0f;
    std::vector<float> samples;
    
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < blockSize; ++i) {
            samples.push_back(data[i]);
            outputRMS += data[i] * data[i];
        }
    }
    outputRMS = std::sqrt(outputRMS / samples.size());
    
    // Calculate variance to detect modulation
    float mean = std::accumulate(samples.begin(), samples.end(), 0.0f) / samples.size();
    for (float s : samples) {
        variance += (s - mean) * (s - mean);
    }
    variance /= samples.size();
    
    std::cout << "\nModulated Output:" << std::endl;
    std::cout << "  RMS: " << outputRMS << std::endl;
    std::cout << "  Variance: " << variance << " (higher = more chaotic)" << std::endl;
    
    // Verdict
    std::cout << "\n=== TEST RESULTS ===" << std::endl;
    
    bool generatesFromSilence = (nonZeroSamples > blockSize); // Should generate audio from silence
    bool modulatesAudio = (variance > sineRMS * sineRMS * 0.1f); // Should add chaos to sine
    
    if (generatesFromSilence && modulatesAudio) {
        std::cout << "✅ CHAOS GENERATOR IS WORKING!" << std::endl;
        std::cout << "   - Generates audio from silence (ModGenerate mode)" << std::endl;
        std::cout << "   - Adds chaotic modulation to input" << std::endl;
    } else {
        std::cout << "❌ CHAOS GENERATOR ISSUES:" << std::endl;
        if (!generatesFromSilence) {
            std::cout << "   - Not generating audio from silence" << std::endl;
        }
        if (!modulatesAudio) {
            std::cout << "   - Not adding chaotic modulation" << std::endl;
        }
    }
    
    return 0;
}
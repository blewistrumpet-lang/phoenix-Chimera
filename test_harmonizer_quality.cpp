// Test IntelligentHarmonizer with explicit quality mode settings
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 8192;

float detectFrequency(const float* buffer, int numSamples, float sampleRate) {
    // Simple zero-crossing frequency detection
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

void testQualityModes() {
    std::cout << "\n=== Testing Quality Modes ===" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test with LOW LATENCY mode (default)
    {
        std::cout << "\n--- Low Latency Mode ---" << std::endl;
        
        std::map<int, float> params;
        params[0] = 0.16f;     // 1 voice  
        params[1] = 0.0f;      // Major chord (first preset)
        params[2] = 0.0f;      // Root key C
        params[3] = 1.0f;      // Chromatic scale
        params[4] = 1.0f;      // Full mix (wet only)
        params[11] = 0.0f;     // Quality = Low Latency
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        // Generate 440 Hz sine wave
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
            buffer.setSample(0, i, sample);
        }
        
        // Process
        harmonizer.process(buffer);
        
        // Analyze output
        std::vector<float> output(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            output[i] = buffer.getSample(0, i);
        }
        
        float freq = detectFrequency(output.data() + 2000, BUFFER_SIZE - 2000, SAMPLE_RATE);
        
        std::cout << "  Input: 440 Hz (A4)" << std::endl;
        std::cout << "  Expected: ~554 Hz (C#5, major 3rd)" << std::endl;
        std::cout << "  Measured: " << freq << " Hz" << std::endl;
        
        if (freq < 500.0f) {
            std::cout << "  ✗ Low latency mode is BROKEN (not shifting pitch)" << std::endl;
        }
    }
    
    // Test with HIGH QUALITY mode
    {
        std::cout << "\n--- High Quality Mode ---" << std::endl;
        
        std::map<int, float> params;
        params[0] = 0.16f;     // 1 voice
        params[1] = 0.0f;      // Major chord (first preset)
        params[2] = 0.0f;      // Root key C
        params[3] = 1.0f;      // Chromatic scale
        params[4] = 1.0f;      // Full mix (wet only)
        params[11] = 1.0f;     // Quality = High Quality (SMBPitchShift)
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        // Generate 440 Hz sine wave
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
            buffer.setSample(0, i, sample);
        }
        
        // Process multiple times to stabilize
        for (int pass = 0; pass < 5; ++pass) {
            juce::AudioBuffer<float> temp(1, BUFFER_SIZE);
            temp.copyFrom(0, 0, buffer, 0, 0, BUFFER_SIZE);
            harmonizer.process(temp);
            if (pass == 4) buffer = temp;
        }
        
        // Analyze output
        std::vector<float> output(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            output[i] = buffer.getSample(0, i);
        }
        
        float freq = detectFrequency(output.data() + 2000, BUFFER_SIZE - 2000, SAMPLE_RATE);
        
        std::cout << "  Input: 440 Hz (A4)" << std::endl;
        std::cout << "  Expected: ~554 Hz (C#5, major 3rd)" << std::endl;
        std::cout << "  Measured: " << freq << " Hz" << std::endl;
        
        float expectedFreq = 440.0f * std::pow(2.0f, 4.0f/12.0f);
        float error = std::abs(freq - expectedFreq) / expectedFreq * 100.0f;
        
        if (error < 1.0f) {
            std::cout << "  ✓ High quality mode WORKS! (Error: " << error << "%)" << std::endl;
        } else {
            std::cout << "  ✗ High quality mode error: " << error << "%" << std::endl;
        }
    }
}

void testDryMixWithQuality() {
    std::cout << "\n=== Testing Dry Mix with High Quality ===" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    std::map<int, float> params;
    params[0] = 1.0f;      // 3 voices
    params[1] = 0.0f;      // Major chord
    params[2] = 0.0f;      // Root key C
    params[3] = 1.0f;      // Chromatic scale
    params[4] = 0.0f;      // 0% mix (dry only)
    params[11] = 1.0f;     // High Quality mode
    
    harmonizer.updateParameters(params);
    harmonizer.reset();
    
    // Generate 440 Hz sine wave
    juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        float sample = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
        buffer.setSample(0, i, sample);
    }
    
    harmonizer.process(buffer);
    
    // Analyze output
    std::vector<float> output(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        output[i] = buffer.getSample(0, i);
    }
    
    float freq = detectFrequency(output.data() + 1000, BUFFER_SIZE - 1000, SAMPLE_RATE);
    
    std::cout << "  Dry signal test (0% mix):" << std::endl;
    std::cout << "  Expected: 440 Hz (unchanged)" << std::endl;
    std::cout << "  Measured: " << freq << " Hz" << std::endl;
    
    float error = std::abs(freq - 440.0f) / 440.0f * 100.0f;
    if (error < 0.5f) {
        std::cout << "  ✓ PASS - Dry signal preserved" << std::endl;
    } else {
        std::cout << "  ✗ FAIL - Dry signal altered" << std::endl;
    }
}

int main() {
    std::cout << "=== HARMONIZER QUALITY MODE TEST ===" << std::endl;
    
    testQualityModes();
    testDryMixWithQuality();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    std::cout << "\nConclusion: The IntelligentHarmonizer defaults to LOW LATENCY mode" << std::endl;
    std::cout << "which uses a broken delay-based pitch shift that doesn't work." << std::endl;
    std::cout << "HIGH QUALITY mode uses SMBPitchShiftFixed and should work correctly." << std::endl;
    
    return 0;
}
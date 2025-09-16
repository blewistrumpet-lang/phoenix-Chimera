// Test pitch accuracy with various parameter combinations
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/PitchShifter.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>

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

void testPitchAccuracy() {
    std::cout << "\n=== Testing Pitch Accuracy ===" << std::endl;
    
    // Test IntelligentHarmonizer with Major triad
    {
        IntelligentHarmonizer harmonizer;
        harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        std::map<int, float> params;
        params[0] = 0.33f;     // 1 voice
        params[1] = 0.0f;      // First chord type (Major)
        params[2] = 0.0f;      // Root key C
        params[3] = 1.0f;      // Chromatic scale
        params[4] = 1.0f;      // Full mix
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        // Test with 440 Hz input
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
            buffer.setSample(0, i, sample);
        }
        
        // Process several times to stabilize
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
        
        std::cout << "IntelligentHarmonizer Major 3rd test:" << std::endl;
        std::cout << "  Input: 440 Hz (A4)" << std::endl;
        std::cout << "  Expected: ~554 Hz (C#5, major 3rd up)" << std::endl;
        std::cout << "  Measured: " << freq << " Hz" << std::endl;
        
        // The first interval of Major chord is 4 semitones
        float expectedFreq = 440.0f * std::pow(2.0f, 4.0f/12.0f);
        float error = std::abs(freq - expectedFreq) / expectedFreq * 100.0f;
        
        if (error < 1.0f) {
            std::cout << "  ✓ PASS (Error: " << error << "%)" << std::endl;
        } else {
            std::cout << "  ✗ FAIL (Error: " << error << "%)" << std::endl;
        }
    }
    
    // Test PitchShifter in Gender mode
    {
        PitchShifter shifter;
        shifter.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        std::map<int, float> params;
        params[0] = 0.0f;      // Gender mode
        params[1] = 0.75f;     // Female (pitch up)
        params[2] = 0.5f;      // Adult age
        params[3] = 1.0f;      // Full intensity
        
        shifter.updateParameters(params);
        shifter.reset();
        
        // Test with 440 Hz input
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
            buffer.setSample(0, i, sample);
        }
        
        shifter.process(buffer);
        
        // Analyze output
        std::vector<float> output(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            output[i] = buffer.getSample(0, i);
        }
        
        float freq = detectFrequency(output.data() + 2000, BUFFER_SIZE - 2000, SAMPLE_RATE);
        
        std::cout << "\nPitchShifter Gender mode test:" << std::endl;
        std::cout << "  Input: 440 Hz" << std::endl;
        std::cout << "  Gender: Female (75%)" << std::endl;
        std::cout << "  Measured: " << freq << " Hz" << std::endl;
        
        if (freq > 440.0f) {
            std::cout << "  ✓ PASS (Pitch shifted up as expected)" << std::endl;
        } else {
            std::cout << "  ✗ FAIL (Should be higher than 440 Hz)" << std::endl;
        }
    }
}

void testMixParameters() {
    std::cout << "\n=== Testing Mix Parameters ===" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test dry (0% mix)
    {
        std::map<int, float> params;
        params[0] = 1.0f;      // 3 voices
        params[1] = 0.0f;      // Major
        params[2] = 0.0f;      // C
        params[3] = 1.0f;      // Chromatic
        params[4] = 0.0f;      // 0% mix (dry only)
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
            buffer.setSample(0, i, sample);
        }
        
        harmonizer.process(buffer);
        
        std::vector<float> output(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            output[i] = buffer.getSample(0, i);
        }
        
        float freq = detectFrequency(output.data() + 1000, BUFFER_SIZE - 1000, SAMPLE_RATE);
        
        std::cout << "Dry signal test (0% mix):" << std::endl;
        std::cout << "  Expected: 440 Hz (unchanged)" << std::endl;
        std::cout << "  Measured: " << freq << " Hz" << std::endl;
        
        float error = std::abs(freq - 440.0f) / 440.0f * 100.0f;
        if (error < 0.5f) {
            std::cout << "  ✓ PASS" << std::endl;
        } else {
            std::cout << "  ✗ FAIL" << std::endl;
        }
    }
}

int main() {
    std::cout << "=== PITCH ENGINE ACCURACY TEST ===" << std::endl;
    
    testPitchAccuracy();
    testMixParameters();
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    
    return 0;
}

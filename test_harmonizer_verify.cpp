// Verify IntelligentHarmonizer is actually working
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <cmath>
#include <map>

float getZeroCrossingRate(const float* buffer, int numSamples, float sampleRate) {
    int crossings = 0;
    for (int i = 1; i < numSamples; ++i) {
        if ((buffer[i-1] < 0 && buffer[i] >= 0) || (buffer[i-1] >= 0 && buffer[i] < 0)) {
            crossings++;
        }
    }
    return (crossings * sampleRate) / (2.0f * numSamples);
}

int main() {
    std::cout << "=== INTELLIGENTHARMONIZER VERIFICATION ===" << std::endl;
    
    const int SAMPLE_RATE = 44100;
    const int BUFFER_SIZE = 8192;
    
    // Test 1: Direct SMBPitchShiftFixed
    std::cout << "\n1. Testing SMBPitchShiftFixed directly:" << std::endl;
    {
        SMBPitchShiftFixed shifter;
        shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
        shifter.reset();
        
        std::vector<float> input(BUFFER_SIZE);
        std::vector<float> output(BUFFER_SIZE);
        
        // Generate 440 Hz
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            input[i] = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f;
        }
        
        // Process with Major 3rd ratio
        shifter.process(input.data(), output.data(), BUFFER_SIZE, 1.25992f);
        
        float freq = getZeroCrossingRate(output.data() + 2000, 4000, SAMPLE_RATE);
        std::cout << "  Input: 440 Hz" << std::endl;
        std::cout << "  Expected: 554 Hz (Major 3rd)" << std::endl;
        std::cout << "  Measured: " << freq << " Hz" << std::endl;
        std::cout << "  " << (std::abs(freq - 554.0f) < 10.0f ? "✓ WORKING" : "✗ NOT WORKING") << std::endl;
    }
    
    // Test 2: IntelligentHarmonizer with same settings
    std::cout << "\n2. Testing IntelligentHarmonizer:" << std::endl;
    {
        IntelligentHarmonizer harmonizer;
        harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        // Set parameters for Major 3rd, high quality, 100% wet
        std::map<int, float> params;
        params[0] = 0.16f;  // 1 voice
        params[1] = 0.0f;   // Major chord
        params[2] = 0.0f;   // Root C
        params[3] = 1.0f;   // Chromatic
        params[4] = 1.0f;   // 100% wet
        params[5] = 1.0f;   // Voice 1 volume 100%
        params[6] = 0.5f;   // Voice 1 formant neutral
        params[7] = 0.0f;   // Voice 2 volume 0%
        params[8] = 0.5f;   // Voice 2 formant
        params[9] = 0.0f;   // Voice 3 volume 0%
        params[10] = 0.5f;  // Voice 3 formant
        params[11] = 1.0f;  // HIGH quality
        params[12] = 0.0f;  // No humanize
        params[13] = 0.0f;  // No width
        params[14] = 0.5f;  // No transpose
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        // Create buffer
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f);
        }
        
        // Process multiple times to stabilize
        for (int pass = 0; pass < 5; ++pass) {
            juce::AudioBuffer<float> temp(1, BUFFER_SIZE);
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                temp.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f);
            }
            harmonizer.process(temp);
            if (pass == 4) buffer = temp;
        }
        
        // Extract and analyze
        std::vector<float> output(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            output[i] = buffer.getSample(0, i);
        }
        
        float freq = getZeroCrossingRate(output.data() + 2000, 4000, SAMPLE_RATE);
        
        // Also check RMS
        float rms = 0.0f;
        for (int i = 2000; i < 6000; ++i) {
            rms += output[i] * output[i];
        }
        rms = std::sqrt(rms / 4000);
        
        std::cout << "  Input: 440 Hz" << std::endl;
        std::cout << "  Expected: 554 Hz (Major 3rd)" << std::endl;
        std::cout << "  Measured: " << freq << " Hz" << std::endl;
        std::cout << "  Output RMS: " << rms << std::endl;
        std::cout << "  " << (std::abs(freq - 554.0f) < 10.0f ? "✓ WORKING" : "✗ NOT WORKING") << std::endl;
    }
    
    // Test 3: Try with 50% mix to see if we get both signals
    std::cout << "\n3. Testing with 50% mix:" << std::endl;
    {
        IntelligentHarmonizer harmonizer;
        harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        std::map<int, float> params;
        params[0] = 0.16f;  // 1 voice
        params[1] = 0.0f;   // Major chord
        params[4] = 0.5f;   // 50% wet
        params[5] = 1.0f;   // Voice 1 volume 100%
        params[11] = 1.0f;  // HIGH quality
        // Set other params to defaults
        for (int i = 2; i <= 14; ++i) {
            if (params.find(i) == params.end()) {
                params[i] = (i == 3 || i == 11) ? 1.0f : (i == 14 || i == 6 || i == 8 || i == 10) ? 0.5f : 0.0f;
            }
        }
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f);
        }
        
        harmonizer.process(buffer);
        
        // Check if output contains both frequencies
        float maxAmp = 0.0f;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            maxAmp = std::max(maxAmp, std::abs(buffer.getSample(0, i)));
        }
        
        std::cout << "  Max amplitude: " << maxAmp << std::endl;
        std::cout << "  " << (maxAmp > 0.1f ? "✓ Has output" : "✗ Silent") << std::endl;
    }
    
    return 0;
}
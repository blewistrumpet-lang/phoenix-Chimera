// Simplest possible test of the harmonizer processing
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <cmath>
#include <vector>

int main() {
    std::cout << "=== SIMPLE HARMONIZER TEST ===" << std::endl;
    
    const int SAMPLE_RATE = 44100;
    const int BUFFER_SIZE = 2048;  // Larger buffer for better frequency detection
    
    // First verify SMBPitchShiftFixed works
    {
        std::cout << "\n1. Testing SMBPitchShiftFixed directly:" << std::endl;
        
        SMBPitchShiftFixed shifter;
        shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
        
        std::vector<float> input(BUFFER_SIZE);
        std::vector<float> output(BUFFER_SIZE, 0.0f);
        
        // Generate 440 Hz sine
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            input[i] = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f;
        }
        
        // Shift up by major 3rd
        float ratio = std::pow(2.0f, 4.0f / 12.0f);
        shifter.process(input.data(), output.data(), BUFFER_SIZE, ratio);
        
        // Check if we got output
        float maxOut = 0.0f;
        for (float sample : output) {
            maxOut = std::max(maxOut, std::abs(sample));
        }
        
        std::cout << "  Ratio: " << ratio << std::endl;
        std::cout << "  Max output: " << maxOut << std::endl;
        std::cout << "  Status: " << (maxOut > 0.01f ? "WORKING" : "BROKEN") << std::endl;
    }
    
    // Now test IntelligentHarmonizer
    {
        std::cout << "\n2. Testing IntelligentHarmonizer:" << std::endl;
        
        IntelligentHarmonizer harmonizer;
        harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        // Configure for single voice, major 3rd, high quality
        std::map<int, float> params;
        params[0] = 0.16f;  // 1 voice
        params[1] = 0.0f;   // Major chord
        params[2] = 0.0f;   // Root C
        params[3] = 1.0f;   // Chromatic
        params[4] = 1.0f;   // 100% wet
        params[5] = 1.0f;   // Voice 1 volume 100%
        params[6] = 0.5f;   // Voice 1 formant neutral
        params[7] = 0.0f;   // Voice 2 volume 0%
        params[8] = 0.5f;   // Voice 2 formant neutral
        params[9] = 0.0f;   // Voice 3 volume 0%
        params[10] = 0.5f;  // Voice 3 formant neutral
        params[11] = 1.0f;  // HIGH QUALITY mode
        params[12] = 0.0f;  // No humanize
        params[13] = 0.0f;  // No width
        params[14] = 0.5f;  // No transpose
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        // Process test signal
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f);
        }
        
        // Save input for comparison
        std::vector<float> inputCopy(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            inputCopy[i] = buffer.getSample(0, i);
        }
        
        // Process
        harmonizer.process(buffer);
        
        // Analyze output
        float maxIn = 0.0f, maxOut = 0.0f;
        int changedSamples = 0;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float inp = inputCopy[i];
            float out = buffer.getSample(0, i);
            maxIn = std::max(maxIn, std::abs(inp));
            maxOut = std::max(maxOut, std::abs(out));
            if (std::abs(out - inp) > 0.001f) {
                changedSamples++;
            }
        }
        
        std::cout << "  Max input: " << maxIn << std::endl;
        std::cout << "  Max output: " << maxOut << std::endl;
        std::cout << "  Changed samples: " << changedSamples << "/" << BUFFER_SIZE << std::endl;
        std::cout << "  Status: " << (changedSamples > BUFFER_SIZE/2 ? "PROCESSING" : "PASSTHROUGH") << std::endl;
    }
    
    // Test with dry signal (0% mix)
    {
        std::cout << "\n3. Testing dry signal (0% mix):" << std::endl;
        
        IntelligentHarmonizer harmonizer;
        harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        std::map<int, float> params;
        params[0] = 1.0f;   // 3 voices
        params[1] = 0.0f;   // Major chord
        params[4] = 0.0f;   // 0% mix (DRY ONLY)
        params[11] = 1.0f;  // High quality
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f);
        }
        
        std::vector<float> inputCopy(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            inputCopy[i] = buffer.getSample(0, i);
        }
        
        harmonizer.process(buffer);
        
        int unchangedSamples = 0;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            if (std::abs(buffer.getSample(0, i) - inputCopy[i]) < 0.0001f) {
                unchangedSamples++;
            }
        }
        
        std::cout << "  Unchanged samples: " << unchangedSamples << "/" << BUFFER_SIZE << std::endl;
        std::cout << "  Status: " << (unchangedSamples == BUFFER_SIZE ? "PERFECT DRY" : "MODIFIED") << std::endl;
    }
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    
    return 0;
}
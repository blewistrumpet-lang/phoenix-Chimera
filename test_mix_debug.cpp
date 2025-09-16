// Debug the mix parameter issue
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include <iostream>
#include <map>
#include <vector>
#include <cmath>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 512;

int main() {
    std::cout << "=== MIX PARAMETER DEBUG ===" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test 1: Process with NO parameters set (all defaults)
    {
        std::cout << "\nTest 1: No parameters set (using defaults)" << std::endl;
        
        harmonizer.reset();
        
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f);
        }
        
        std::vector<float> input(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            input[i] = buffer.getSample(0, i);
        }
        
        harmonizer.process(buffer);
        
        // Check if output changed
        int changedSamples = 0;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            if (std::abs(buffer.getSample(0, i) - input[i]) > 0.001f) {
                changedSamples++;
            }
        }
        
        std::cout << "  Changed samples: " << changedSamples << "/" << BUFFER_SIZE << std::endl;
        std::cout << "  Default behavior: " << (changedSamples > 10 ? "PROCESSING" : "PASSTHROUGH") << std::endl;
    }
    
    // Test 2: Explicitly set mix to 0.0
    {
        std::cout << "\nTest 2: Explicitly set mix to 0.0" << std::endl;
        
        std::map<int, float> params;
        params[4] = 0.0f;  // Master mix = 0% (dry only)
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f);
        }
        
        std::vector<float> input(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            input[i] = buffer.getSample(0, i);
        }
        
        harmonizer.process(buffer);
        
        int unchangedSamples = 0;
        float maxDiff = 0.0f;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float diff = std::abs(buffer.getSample(0, i) - input[i]);
            maxDiff = std::max(maxDiff, diff);
            if (diff < 0.0001f) {
                unchangedSamples++;
            }
        }
        
        std::cout << "  Unchanged samples: " << unchangedSamples << "/" << BUFFER_SIZE << std::endl;
        std::cout << "  Max difference: " << maxDiff << std::endl;
        std::cout << "  Result: " << (unchangedSamples == BUFFER_SIZE ? "PERFECT DRY" : "STILL PROCESSING") << std::endl;
    }
    
    // Test 3: Set all parameters including mix=0
    {
        std::cout << "\nTest 3: All parameters with mix=0" << std::endl;
        
        std::map<int, float> params;
        params[0] = 1.0f;   // 3 voices
        params[1] = 0.0f;   // Major chord
        params[2] = 0.0f;   // Root C
        params[3] = 1.0f;   // Chromatic
        params[4] = 0.0f;   // Mix = 0%
        params[5] = 1.0f;   // Voice 1 vol
        params[6] = 0.5f;   // Voice 1 formant
        params[7] = 0.7f;   // Voice 2 vol
        params[8] = 0.5f;   // Voice 2 formant
        params[9] = 0.5f;   // Voice 3 vol
        params[10] = 0.5f;  // Voice 3 formant
        params[11] = 1.0f;  // High quality
        params[12] = 0.0f;  // No humanize
        params[13] = 0.0f;  // No width
        params[14] = 0.5f;  // No transpose
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f);
        }
        
        std::vector<float> input(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            input[i] = buffer.getSample(0, i);
        }
        
        harmonizer.process(buffer);
        
        int unchangedSamples = 0;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            if (std::abs(buffer.getSample(0, i) - input[i]) < 0.0001f) {
                unchangedSamples++;
            }
        }
        
        std::cout << "  Unchanged samples: " << unchangedSamples << "/" << BUFFER_SIZE << std::endl;
        std::cout << "  Result: " << (unchangedSamples == BUFFER_SIZE ? "PERFECT DRY" : "STILL PROCESSING") << std::endl;
    }
    
    // Test 4: Process multiple times to check stability
    {
        std::cout << "\nTest 4: Multiple processing passes with mix=0" << std::endl;
        
        std::map<int, float> params;
        params[4] = 0.0f;  // Mix = 0%
        params[11] = 1.0f; // High quality
        
        harmonizer.updateParameters(params);
        harmonizer.reset();
        
        juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
        std::vector<float> original(BUFFER_SIZE);
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f;
            buffer.setSample(0, i, sample);
            original[i] = sample;
        }
        
        // Process 5 times
        for (int pass = 0; pass < 5; ++pass) {
            harmonizer.process(buffer);
            
            int unchanged = 0;
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                if (std::abs(buffer.getSample(0, i) - original[i]) < 0.0001f) {
                    unchanged++;
                }
            }
            
            std::cout << "  Pass " << (pass + 1) << ": " << unchanged << "/" << BUFFER_SIZE << " unchanged" << std::endl;
            
            // Reset buffer for next pass
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                buffer.setSample(0, i, original[i]);
            }
        }
    }
    
    std::cout << "\n=== DEBUG COMPLETE ===" << std::endl;
    
    return 0;
}
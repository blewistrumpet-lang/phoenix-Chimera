// Final test to understand why IntelligentHarmonizer isn't working
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include <iostream>
#include <cmath>
#include <map>

int main() {
    std::cout << "=== FINAL HARMONIZER DEBUG ===" << std::endl;
    
    const int SAMPLE_RATE = 44100;
    const int BUFFER_SIZE = 512;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Set all parameters explicitly
    std::map<int, float> params;
    
    // Core parameters
    params[0] = 0.16f;  // 1 voice
    params[1] = 0.0f;   // Major chord (first in list)
    params[2] = 0.0f;   // Root key C
    params[3] = 1.0f;   // Chromatic scale (no quantization)
    params[4] = 1.0f;   // 100% wet (full effect)
    
    // Voice parameters
    params[5] = 1.0f;   // Voice 1 volume = 100%
    params[6] = 0.5f;   // Voice 1 formant = neutral
    params[7] = 0.0f;   // Voice 2 volume = 0%
    params[8] = 0.5f;   // Voice 2 formant = neutral
    params[9] = 0.0f;   // Voice 3 volume = 0%
    params[10] = 0.5f;  // Voice 3 formant = neutral
    
    // Quality and effects
    params[11] = 1.0f;  // HIGH QUALITY mode
    params[12] = 0.0f;  // No humanize
    params[13] = 0.0f;  // No stereo width
    params[14] = 0.5f;  // No transpose (centered)
    
    std::cout << "\nSetting parameters..." << std::endl;
    harmonizer.updateParameters(params);
    
    std::cout << "\nResetting..." << std::endl;
    harmonizer.reset();
    
    // Create test signal - 440 Hz sine wave
    juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        float sample = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.3f;
        buffer.setSample(0, i, sample);
    }
    
    // Save input for comparison
    std::vector<float> inputCopy(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        inputCopy[i] = buffer.getSample(0, i);
    }
    
    std::cout << "\nProcessing..." << std::endl;
    harmonizer.process(buffer);
    
    // Analyze result
    float inputMax = 0.0f, outputMax = 0.0f;
    float inputRMS = 0.0f, outputRMS = 0.0f;
    int changedSamples = 0;
    
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        float inp = inputCopy[i];
        float out = buffer.getSample(0, i);
        
        inputMax = std::max(inputMax, std::abs(inp));
        outputMax = std::max(outputMax, std::abs(out));
        
        inputRMS += inp * inp;
        outputRMS += out * out;
        
        if (std::abs(out - inp) > 0.001f) {
            changedSamples++;
        }
    }
    
    inputRMS = std::sqrt(inputRMS / BUFFER_SIZE);
    outputRMS = std::sqrt(outputRMS / BUFFER_SIZE);
    
    std::cout << "\n=== RESULTS ===" << std::endl;
    std::cout << "Input:  Max=" << inputMax << " RMS=" << inputRMS << std::endl;
    std::cout << "Output: Max=" << outputMax << " RMS=" << outputRMS << std::endl;
    std::cout << "Changed: " << changedSamples << "/" << BUFFER_SIZE << " samples" << std::endl;
    
    // Expected for Major 3rd: 440 Hz -> 554 Hz
    std::cout << "\nExpected: Major 3rd up (4 semitones) = 440 * 1.26 = 554 Hz" << std::endl;
    std::cout << "Actual: " << (changedSamples > BUFFER_SIZE/2 ? "Processing occurred" : "No processing - FAILED") << std::endl;
    
    if (outputRMS < 0.01f) {
        std::cout << "\n*** ERROR: Output is silent! ***" << std::endl;
    } else if (std::abs(outputRMS - inputRMS) < 0.01f) {
        std::cout << "\n*** ERROR: Output unchanged - pitch shift not working! ***" << std::endl;
    } else {
        std::cout << "\n*** Output modified - check frequency ***" << std::endl;
    }
    
    return 0;
}
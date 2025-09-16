// Track what pitch ratios are actually being used
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/IntelligentHarmonizerChords.h"
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 512;

// Helper to directly test pitch shift
void testDirectPitchShift(float ratio) {
    std::cout << "\n--- Direct SMBPitchShiftFixed test ---" << std::endl;
    std::cout << "Testing ratio: " << ratio << std::endl;
    
    SMBPitchShiftFixed shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    shifter.reset();
    
    // Generate 440 Hz input
    std::vector<float> input(BUFFER_SIZE);
    std::vector<float> output(BUFFER_SIZE);
    
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        input[i] = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
    }
    
    // Process with ratio
    shifter.process(input.data(), output.data(), BUFFER_SIZE, ratio);
    
    // Check output level
    float inputRMS = 0.0f, outputRMS = 0.0f;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        inputRMS += input[i] * input[i];
        outputRMS += output[i] * output[i];
    }
    inputRMS = std::sqrt(inputRMS / BUFFER_SIZE);
    outputRMS = std::sqrt(outputRMS / BUFFER_SIZE);
    
    std::cout << "Input RMS: " << inputRMS << std::endl;
    std::cout << "Output RMS: " << outputRMS << std::endl;
    std::cout << "Gain: " << (outputRMS / inputRMS) << std::endl;
    
    if (outputRMS < 0.01f) {
        std::cout << "ERROR: No output!" << std::endl;
    }
}

int main() {
    std::cout << "=== RATIO TRACKING TEST ===" << std::endl;
    
    // First test direct pitch shifting with the expected ratio
    float majorThirdRatio = std::pow(2.0f, 4.0f / 12.0f);
    testDirectPitchShift(majorThirdRatio);
    
    // Now test what the harmonizer is actually calculating
    std::cout << "\n--- IntelligentHarmonizer parameter calculation ---" << std::endl;
    
    // Simulate the parameter update logic
    float chordNorm = 0.0f;  // Major chord
    auto intervals = IntelligentHarmonizerChords::getChordIntervals(chordNorm);
    
    std::cout << "Chord normalized value: " << chordNorm << std::endl;
    std::cout << "Chord name: " << IntelligentHarmonizerChords::getChordName(chordNorm) << std::endl;
    std::cout << "Intervals: [" << intervals[0] << ", " << intervals[1] << ", " << intervals[2] << "]" << std::endl;
    
    // Calculate ratios as the harmonizer would
    float ratio1 = std::pow(2.0f, intervals[0] / 12.0f);
    float ratio2 = std::pow(2.0f, intervals[1] / 12.0f);
    float ratio3 = std::pow(2.0f, intervals[2] / 12.0f);
    
    std::cout << "Calculated ratios:" << std::endl;
    std::cout << "  Voice 1: " << ratio1 << " (should be " << majorThirdRatio << ")" << std::endl;
    std::cout << "  Voice 2: " << ratio2 << std::endl;
    std::cout << "  Voice 3: " << ratio3 << std::endl;
    
    // Check voice count logic
    std::cout << "\n--- Voice count logic ---" << std::endl;
    float voiceNorms[] = {0.0f, 0.16f, 0.33f, 0.5f, 0.66f, 1.0f};
    for (float norm : voiceNorms) {
        int count = IntelligentHarmonizerChords::getVoiceCount(norm);
        std::cout << "Normalized " << norm << " -> " << count << " voices" << std::endl;
    }
    
    // Test with IntelligentHarmonizer actual processing
    std::cout << "\n--- IntelligentHarmonizer actual processing ---" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    std::map<int, float> params;
    params[0] = 0.16f;     // 1 voice
    params[1] = 0.0f;      // Major chord
    params[2] = 0.0f;      // Root key C
    params[3] = 1.0f;      // Chromatic
    params[4] = 1.0f;      // 100% wet
    params[5] = 1.0f;      // Voice 1 volume 100%
    params[11] = 1.0f;     // High quality mode
    
    harmonizer.updateParameters(params);
    harmonizer.reset();
    
    // Process a test buffer
    juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        buffer.setSample(0, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f);
    }
    
    // Store input for comparison
    std::vector<float> inputCopy(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        inputCopy[i] = buffer.getSample(0, i);
    }
    
    harmonizer.process(buffer);
    
    // Analyze
    float inputRMS = 0.0f, outputRMS = 0.0f;
    bool outputChanged = false;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        float inp = inputCopy[i];
        float out = buffer.getSample(0, i);
        inputRMS += inp * inp;
        outputRMS += out * out;
        if (std::abs(out - inp) > 0.001f) {
            outputChanged = true;
        }
    }
    inputRMS = std::sqrt(inputRMS / BUFFER_SIZE);
    outputRMS = std::sqrt(outputRMS / BUFFER_SIZE);
    
    std::cout << "Input RMS: " << inputRMS << std::endl;
    std::cout << "Output RMS: " << outputRMS << std::endl;
    std::cout << "Output changed from input: " << (outputChanged ? "YES" : "NO") << std::endl;
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    
    return 0;
}
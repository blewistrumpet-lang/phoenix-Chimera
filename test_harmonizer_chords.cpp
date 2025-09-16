// Test IntelligentHarmonizer with all chord presets
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 512;
const float INPUT_FREQ = 440.0f; // A4

// Generate a sine wave
void generateSineWave(float* buffer, int numSamples, float frequency, float sampleRate) {
    static float phase = 0.0f;
    float phaseInc = 2.0f * M_PI * frequency / sampleRate;
    
    for (int i = 0; i < numSamples; ++i) {
        buffer[i] = std::sin(phase);
        phase += phaseInc;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
    }
}

// Analyze frequency using zero-crossing (simple but effective for pure tones)
float analyzeFrequency(const float* buffer, int numSamples, float sampleRate) {
    // Skip initial samples to avoid transients
    int skipSamples = numSamples / 4;
    
    // Find zero crossings
    std::vector<int> zeroCrossings;
    for (int i = skipSamples + 1; i < numSamples - 1; ++i) {
        if (buffer[i-1] <= 0 && buffer[i] > 0) {
            // Interpolate exact crossing point
            float frac = -buffer[i-1] / (buffer[i] - buffer[i-1]);
            zeroCrossings.push_back(i - 1 + frac);
        }
    }
    
    if (zeroCrossings.size() < 2) return 0.0f;
    
    // Calculate average period
    float totalPeriod = 0;
    int periodCount = 0;
    for (size_t i = 1; i < zeroCrossings.size(); ++i) {
        totalPeriod += (zeroCrossings[i] - zeroCrossings[i-1]);
        periodCount++;
    }
    
    if (periodCount == 0) return 0.0f;
    
    float avgPeriod = totalPeriod / periodCount;
    return sampleRate / avgPeriod;
}

// Calculate expected frequency for a given interval
float getExpectedFrequency(float rootFreq, int semitones) {
    return rootFreq * std::pow(2.0f, semitones / 12.0f);
}

void testChordPreset(IntelligentHarmonizer& harmonizer, const std::string& chordName, 
                     float chordParam, const std::vector<int>& expectedIntervals) {
    std::cout << "\n=== Testing " << chordName << " (param=" << std::fixed << std::setprecision(2) 
              << chordParam << ") ===" << std::endl;
    
    // Set parameters - corrected indices based on IntelligentHarmonizer.cpp
    std::map<int, float> params;
    params[0] = 1.0f;      // Voices (1.0 = 3 voices)
    params[1] = chordParam; // Chord type
    params[2] = 0.0f;      // Root Key (C)
    params[3] = 1.0f;      // Scale (1.0 = Chromatic - all notes available)
    params[4] = 1.0f;      // Master Mix (100% wet)
    harmonizer.updateParameters(params);
    
    // Reset and prepare
    harmonizer.reset();
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Generate input signal
    std::vector<float> input(BUFFER_SIZE);
    generateSineWave(input.data(), BUFFER_SIZE, INPUT_FREQ, SAMPLE_RATE);
    
    // Process multiple times to let it stabilize
    juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
    for (int pass = 0; pass < 10; ++pass) {
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, input[i]);
        }
        harmonizer.process(buffer);
    }
    
    // Final processing pass for analysis
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        buffer.setSample(0, i, input[i]);
    }
    harmonizer.process(buffer);
    
    // Extract output
    std::vector<float> output(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        output[i] = buffer.getSample(0, i);
    }
    
    // Test each voice separately
    for (size_t voiceIdx = 0; voiceIdx < expectedIntervals.size(); ++voiceIdx) {
        // Set to test only this voice
        params[4] = 1.0f; // Keep mix at 100%
        harmonizer.updateParameters(params);
        
        // Process with fresh input
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, input[i]);
        }
        
        // Let it stabilize
        for (int pass = 0; pass < 5; ++pass) {
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                buffer.setSample(0, i, input[i]);
            }
            harmonizer.process(buffer);
        }
        
        // Final pass for measurement
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            buffer.setSample(0, i, input[i]);
        }
        harmonizer.process(buffer);
        
        // Extract and analyze
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            output[i] = buffer.getSample(0, i);
        }
        
        float measuredFreq = analyzeFrequency(output.data(), BUFFER_SIZE, SAMPLE_RATE);
        float expectedFreq = getExpectedFrequency(INPUT_FREQ, expectedIntervals[voiceIdx]);
        
        if (measuredFreq > 50.0f) { // Valid measurement
            float error = std::abs(measuredFreq - expectedFreq) / expectedFreq * 100.0f;
            
            std::cout << "  Voice " << (voiceIdx + 1) << " (+" << expectedIntervals[voiceIdx] << " semitones): ";
            std::cout << "Expected " << std::fixed << std::setprecision(1) << expectedFreq << " Hz, ";
            std::cout << "Got " << measuredFreq << " Hz";
            std::cout << " (Error: " << std::setprecision(2) << error << "%)" << std::endl;
            
            if (error > 0.1f) {
                std::cout << "    WARNING: Frequency error exceeds 0.1% threshold!" << std::endl;
            }
        }
    }
}

int main() {
    std::cout << "Testing IntelligentHarmonizer with SMBPitchShiftFixed" << std::endl;
    std::cout << "Input frequency: " << INPUT_FREQ << " Hz (A4)" << std::endl;
    std::cout << "Required accuracy: < 0.1% frequency error" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test all chord presets with their expected intervals
    testChordPreset(harmonizer, "Major Triad", 0.0f, {0, 4, 7});
    testChordPreset(harmonizer, "Minor Triad", 0.083f, {0, 3, 7});
    testChordPreset(harmonizer, "Diminished Triad", 0.167f, {0, 3, 6});
    testChordPreset(harmonizer, "Augmented Triad", 0.25f, {0, 4, 8});
    testChordPreset(harmonizer, "Major 7th", 0.333f, {0, 4, 7, 11});
    testChordPreset(harmonizer, "Minor 7th", 0.417f, {0, 3, 7, 10});
    testChordPreset(harmonizer, "Dominant 7th", 0.5f, {0, 4, 7, 10});
    testChordPreset(harmonizer, "Half-Diminished 7th", 0.583f, {0, 3, 6, 10});
    testChordPreset(harmonizer, "Diminished 7th", 0.667f, {0, 3, 6, 9});
    testChordPreset(harmonizer, "Sus2", 0.75f, {0, 2, 7});
    testChordPreset(harmonizer, "Sus4", 0.833f, {0, 5, 7});
    testChordPreset(harmonizer, "Add9", 0.917f, {0, 4, 7, 14});
    testChordPreset(harmonizer, "Custom", 1.0f, {0}); // Custom mode
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}

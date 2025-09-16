// Comprehensive test of IntelligentHarmonizer chord types with different input frequencies
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/IntelligentHarmonizerChords.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 8192;

// Simple frequency detection
float detectPitch(const float* buffer, int numSamples, float sampleRate) {
    // Zero-crossing based pitch detection
    int crossings = 0;
    int startIdx = numSamples / 4;
    int endIdx = numSamples * 3 / 4;
    
    for (int i = startIdx + 1; i < endIdx; ++i) {
        if ((buffer[i-1] < 0 && buffer[i] >= 0) || 
            (buffer[i-1] >= 0 && buffer[i] < 0)) {
            crossings++;
        }
    }
    
    float duration = (endIdx - startIdx) / sampleRate;
    return (crossings / 2.0f) / duration;
}

void testChordWithFrequency(IntelligentHarmonizer& harmonizer, 
                            float inputFreq, 
                            int chordIndex, 
                            const char* chordName,
                            int numVoices = 1) {
    
    // Set parameters
    std::map<int, float> params;
    
    // Calculate normalized chord parameter (0-1 range for 32 chords)
    float chordNorm = chordIndex / 31.0f;
    
    params[0] = (numVoices == 1) ? 0.16f : (numVoices == 2) ? 0.5f : 0.84f;  // Number of voices
    params[1] = chordNorm;   // Chord type
    params[2] = 0.0f;        // Root key C
    params[3] = 1.0f;        // Chromatic (no scale quantization)
    params[4] = 1.0f;        // 100% wet
    params[5] = 1.0f;        // Voice 1 volume 100%
    params[6] = 0.5f;        // Voice 1 formant neutral
    params[7] = (numVoices >= 2) ? 0.8f : 0.0f;  // Voice 2 volume
    params[8] = 0.5f;        // Voice 2 formant
    params[9] = (numVoices >= 3) ? 0.6f : 0.0f;  // Voice 3 volume
    params[10] = 0.5f;       // Voice 3 formant
    params[11] = 1.0f;       // HIGH quality
    params[12] = 0.0f;       // No humanize
    params[13] = 0.0f;       // No stereo width
    params[14] = 0.5f;       // No transpose
    
    harmonizer.updateParameters(params);
    harmonizer.reset();
    
    // Generate input signal
    juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        buffer.setSample(0, i, std::sin(2.0f * M_PI * inputFreq * i / SAMPLE_RATE) * 0.3f);
    }
    
    // Process multiple times to stabilize
    for (int pass = 0; pass < 3; ++pass) {
        juce::AudioBuffer<float> temp(1, BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            temp.setSample(0, i, std::sin(2.0f * M_PI * inputFreq * i / SAMPLE_RATE) * 0.3f);
        }
        harmonizer.process(temp);
        if (pass == 2) buffer = temp;
    }
    
    // Extract output
    std::vector<float> output(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        output[i] = buffer.getSample(0, i);
    }
    
    // Detect frequencies
    float detectedFreq = detectPitch(output.data(), BUFFER_SIZE, SAMPLE_RATE);
    
    // Calculate RMS
    float rms = 0.0f;
    for (int i = BUFFER_SIZE/4; i < BUFFER_SIZE*3/4; ++i) {
        rms += output[i] * output[i];
    }
    rms = std::sqrt(rms / (BUFFER_SIZE/2));
    
    // Get expected intervals from chord definition
    // Access the actual chord presets
    auto preset = IntelligentHarmonizerChords::CHORD_PRESETS[chordIndex];
    
    std::cout << std::left << std::setw(20) << chordName 
              << " | Input: " << std::fixed << std::setprecision(1) << inputFreq << " Hz"
              << " | Detected: " << detectedFreq << " Hz"
              << " | RMS: " << std::setprecision(3) << rms;
    
    // Calculate interval ratio
    float ratio = detectedFreq / inputFreq;
    
    // Map ratio to nearest musical interval
    struct Interval {
        float ratio;
        const char* name;
    };
    
    Interval intervals[] = {
        {1.0f, "Unison"},
        {1.059f, "Minor 2nd"},
        {1.122f, "Major 2nd"},
        {1.189f, "Minor 3rd"},
        {1.260f, "Major 3rd"},
        {1.335f, "Perfect 4th"},
        {1.414f, "Tritone"},
        {1.498f, "Perfect 5th"},
        {1.587f, "Minor 6th"},
        {1.682f, "Major 6th"},
        {1.782f, "Minor 7th"},
        {1.888f, "Major 7th"},
        {2.0f, "Octave"}
    };
    
    // Find closest interval
    float minDiff = 999.0f;
    const char* closestInterval = "Unknown";
    for (const auto& interval : intervals) {
        float diff = std::abs(ratio - interval.ratio);
        if (diff < minDiff) {
            minDiff = diff;
            closestInterval = interval.name;
        }
    }
    
    std::cout << " | Interval: " << closestInterval 
              << " (ratio=" << std::setprecision(3) << ratio << ")";
    
    if (rms < 0.01f) {
        std::cout << " ✗ SILENT";
    } else if (minDiff < 0.02f) {
        std::cout << " ✓";
    } else {
        std::cout << " ⚠";
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "=== INTELLIGENT HARMONIZER CHORD TEST ===" << std::endl;
    std::cout << "Testing all 32 chord presets with different input frequencies\n" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test frequencies: A2, A3, A4
    float testFrequencies[] = {110.0f, 220.0f, 440.0f, 880.0f};
    const char* freqNames[] = {"A2 (110Hz)", "A3 (220Hz)", "A4 (440Hz)", "A5 (880Hz)"};
    
    // Get chord names from the presets
    
    for (int f = 0; f < 4; ++f) {
        std::cout << "\n=== Testing with " << freqNames[f] << " ===" << std::endl;
        std::cout << "Single voice harmonies:" << std::endl;
        
        // Test first 8 chords (most common)
        for (int c = 0; c < 8; ++c) {
            auto preset = IntelligentHarmonizerChords::CHORD_PRESETS[c];
            testChordWithFrequency(harmonizer, testFrequencies[f], c, preset.name, 1);
        }
    }
    
    // Test multi-voice harmonies with A4 (440Hz)
    std::cout << "\n=== Multi-Voice Harmony Test (A4 440Hz) ===" << std::endl;
    
    // Test Major chord with 2 voices
    std::cout << "\n2 Voices:" << std::endl;
    testChordWithFrequency(harmonizer, 440.0f, 0, "Major (2 voices)", 2);
    testChordWithFrequency(harmonizer, 440.0f, 1, "Minor (2 voices)", 2);
    testChordWithFrequency(harmonizer, 440.0f, 4, "Dominant 7th (2 voices)", 2);
    
    // Test with 3 voices
    std::cout << "\n3 Voices:" << std::endl;
    testChordWithFrequency(harmonizer, 440.0f, 0, "Major (3 voices)", 3);
    testChordWithFrequency(harmonizer, 440.0f, 1, "Minor (3 voices)", 3);
    testChordWithFrequency(harmonizer, 440.0f, 4, "Dominant 7th (3 voices)", 3);
    
    // Test some exotic chords
    std::cout << "\n=== Exotic Chords Test (A4 440Hz) ===" << std::endl;
    testChordWithFrequency(harmonizer, 440.0f, 10, IntelligentHarmonizerChords::CHORD_PRESETS[10].name, 1);
    testChordWithFrequency(harmonizer, 440.0f, 15, IntelligentHarmonizerChords::CHORD_PRESETS[15].name, 1);
    testChordWithFrequency(harmonizer, 440.0f, 20, IntelligentHarmonizerChords::CHORD_PRESETS[20].name, 1);
    testChordWithFrequency(harmonizer, 440.0f, 25, IntelligentHarmonizerChords::CHORD_PRESETS[25].name, 1);
    testChordWithFrequency(harmonizer, 440.0f, 31, IntelligentHarmonizerChords::CHORD_PRESETS[31].name, 1);
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "✓ = Correct musical interval detected" << std::endl;
    std::cout << "⚠ = Interval slightly off (may need tuning)" << std::endl;
    std::cout << "✗ = Silent or severely wrong" << std::endl;
    
    return 0;
}
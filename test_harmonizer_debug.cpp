// Debug the IntelligentHarmonizer's actual processing
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/IntelligentHarmonizerChords.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 512;  // Smaller buffer for debugging

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

int main() {
    std::cout << "=== HARMONIZER DEBUG TEST ===" << std::endl;
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Set parameters for single voice, Major chord, high quality
    std::map<int, float> params;
    params[0] = 0.16f;     // 1 voice
    params[1] = 0.0f;      // Major chord (first preset)
    params[2] = 0.0f;      // Root key C
    params[3] = 1.0f;      // Chromatic scale
    params[4] = 1.0f;      // Full mix (wet only)
    params[5] = 1.0f;      // Voice 1 volume = 100%
    params[6] = 0.5f;      // Voice 1 formant = 0
    params[7] = 0.0f;      // Voice 2 volume = 0%
    params[8] = 0.5f;      // Voice 2 formant = 0
    params[9] = 0.0f;      // Voice 3 volume = 0%
    params[10] = 0.5f;     // Voice 3 formant = 0
    params[11] = 1.0f;     // Quality = High Quality
    params[12] = 0.0f;     // Humanize = 0
    params[13] = 0.0f;     // Width = 0
    params[14] = 0.5f;     // Transpose = 0
    
    std::cout << "\nParameter Settings:" << std::endl;
    std::cout << "  Voices: " << IntelligentHarmonizerChords::getVoiceCountDisplay(params[0]) << std::endl;
    std::cout << "  Chord: " << IntelligentHarmonizerChords::getChordName(params[1]) << std::endl;
    std::cout << "  Quality: " << IntelligentHarmonizerChords::getQualityDisplay(params[11]) << std::endl;
    std::cout << "  Mix: " << (params[4] * 100) << "%" << std::endl;
    
    // Expected intervals for Major chord
    auto intervals = IntelligentHarmonizerChords::getChordIntervals(params[1]);
    std::cout << "\nExpected intervals: " << intervals[0] << ", " << intervals[1] << ", " << intervals[2] << " semitones" << std::endl;
    float expectedRatio = std::pow(2.0f, intervals[0] / 12.0f);
    std::cout << "Expected pitch ratio for voice 1: " << expectedRatio << std::endl;
    std::cout << "Expected frequency: 440 * " << expectedRatio << " = " << (440.0f * expectedRatio) << " Hz" << std::endl;
    
    harmonizer.updateParameters(params);
    harmonizer.reset();
    
    // Generate test signal
    juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
    
    std::cout << "\n--- Processing multiple blocks ---" << std::endl;
    
    // Process several blocks and analyze each one
    for (int block = 0; block < 10; ++block) {
        // Generate 440 Hz sine wave
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float phase = 2.0f * M_PI * 440.0f * (block * BUFFER_SIZE + i) / SAMPLE_RATE;
            float sample = std::sin(phase) * 0.5f;
            buffer.setSample(0, i, sample);
        }
        
        // Keep copy of input
        std::vector<float> inputCopy(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            inputCopy[i] = buffer.getSample(0, i);
        }
        
        // Process
        harmonizer.process(buffer);
        
        // Analyze output
        float inputRMS = 0.0f, outputRMS = 0.0f;
        float maxOut = 0.0f;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float inp = inputCopy[i];
            float out = buffer.getSample(0, i);
            inputRMS += inp * inp;
            outputRMS += out * out;
            maxOut = std::max(maxOut, std::abs(out));
        }
        inputRMS = std::sqrt(inputRMS / BUFFER_SIZE);
        outputRMS = std::sqrt(outputRMS / BUFFER_SIZE);
        
        if (block == 0 || block == 5 || block == 9) {
            std::cout << "Block " << block << ": ";
            std::cout << "Input RMS=" << std::fixed << std::setprecision(3) << inputRMS;
            std::cout << ", Output RMS=" << outputRMS;
            std::cout << ", Max=" << maxOut;
            
            // Try to detect frequency in later blocks
            if (block >= 5 && BUFFER_SIZE >= 256) {
                std::vector<float> output(BUFFER_SIZE);
                for (int i = 0; i < BUFFER_SIZE; ++i) {
                    output[i] = buffer.getSample(0, i);
                }
                float freq = detectFrequency(output.data(), BUFFER_SIZE, SAMPLE_RATE);
                if (freq > 0) {
                    std::cout << ", Freq=" << freq << " Hz";
                }
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    
    return 0;
}
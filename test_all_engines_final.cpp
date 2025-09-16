// Final integration test for all pitch-based engines
#include "JUCE_Plugin/Source/PitchShifter.h"
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/DetuneDoubler.h"
#include "JUCE_Plugin/Source/FrequencyShifter.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 4096;

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

void testEngine(const std::string& name, EngineBase* engine, float inputFreq = 440.0f) {
    std::cout << "\n=== Testing " << name << " ===" << std::endl;
    
    engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    engine->reset();
    
    // Generate test signal
    juce::AudioBuffer<float> buffer(1, BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        float sample = std::sin(2.0f * M_PI * inputFreq * i / SAMPLE_RATE) * 0.5f;
        buffer.setSample(0, i, sample);
    }
    
    // Process
    engine->process(buffer);
    
    // Extract output
    std::vector<float> output(BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        output[i] = buffer.getSample(0, i);
    }
    
    // Check if we have output
    float rms = 0.0f;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        rms += output[i] * output[i];
    }
    rms = std::sqrt(rms / BUFFER_SIZE);
    
    std::cout << "  Output RMS: " << rms << std::endl;
    
    if (rms > 0.001f) {
        float freq = detectFrequency(output.data() + 1000, BUFFER_SIZE - 1000, SAMPLE_RATE);
        std::cout << "  Detected frequency: " << freq << " Hz" << std::endl;
        std::cout << "  ✓ Engine is producing output" << std::endl;
    } else {
        std::cout << "  ✗ No output detected!" << std::endl;
    }
}

int main() {
    std::cout << "=== ALL ENGINES INTEGRATION TEST ===" << std::endl;
    std::cout << "Testing all pitch-related engines with 440 Hz input" << std::endl;
    
    // Test PitchShifter
    {
        PitchShifter shifter;
        std::map<int, float> params;
        params[0] = 0.583f; // Pitch shift amount (up a fifth)
        params[1] = 1.0f;    // Mix
        shifter.updateParameters(params);
        testEngine("PitchShifter (Fifth Up)", &shifter);
    }
    
    // Test IntelligentHarmonizer
    {
        IntelligentHarmonizer harmonizer;
        std::map<int, float> params;
        params[0] = 1.0f;  // 3 voices
        params[1] = 0.0f;  // Major triad
        params[2] = 0.0f;  // Root key C
        params[3] = 1.0f;  // Chromatic scale
        params[4] = 1.0f;  // Full mix
        harmonizer.updateParameters(params);
        testEngine("IntelligentHarmonizer (Major Triad)", &harmonizer);
    }
    
    // Test ShimmerReverb
    {
        ShimmerReverb shimmer;
        std::map<int, float> params;
        params[0] = 0.3f;  // Reverb time
        params[1] = 0.5f;  // Damping
        params[2] = 0.7f;  // Shimmer amount
        params[3] = 0.5f;  // Pitch shift
        params[4] = 0.5f;  // Mix
        shimmer.updateParameters(params);
        testEngine("ShimmerReverb", &shimmer);
    }
    
    // Test DetuneDoubler
    {
        AudioDSP::DetuneDoubler doubler;
        std::map<int, float> params;
        params[0] = 0.3f;  // Detune amount
        params[1] = 0.2f;  // Delay
        params[2] = 0.7f;  // Width
        params[3] = 0.3f;  // Thickness
        params[4] = 0.7f;  // Mix
        doubler.updateParameters(params);
        testEngine("DetuneDoubler", &doubler);
    }
    
    // Test FrequencyShifter
    {
        FrequencyShifter shifter;
        std::map<int, float> params;
        params[0] = 0.55f; // Shift amount (slight up)
        params[1] = 0.0f;  // No feedback
        params[2] = 0.7f;  // Mix
        shifter.updateParameters(params);
        testEngine("FrequencyShifter", &shifter);
    }
    
    std::cout << "\n=== INTEGRATION TEST COMPLETE ===" << std::endl;
    std::cout << "All engines have been tested for basic functionality." << std::endl;
    
    return 0;
}

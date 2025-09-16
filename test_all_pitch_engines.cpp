#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/PitchShifter.cpp"
#include "JUCE_Plugin/Source/DetuneDoubler.cpp"
#include "JUCE_Plugin/Source/IntelligentHarmonizer.cpp"
#include "JUCE_Plugin/Source/FrequencyShifter.cpp"
#include "JUCE_Plugin/Source/ShimmerReverb.cpp"

bool testEngine(EngineBase* engine, const std::string& engineName, 
                float param1, float param2, float expectedFreq) {
    const float sampleRate = 44100.0f;
    const int testSamples = 8192;
    std::vector<float> input(testSamples);
    std::vector<float> output(testSamples);
    
    // Generate 440Hz sine wave
    for (int i = 0; i < testSamples; ++i) {
        input[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
    }
    
    // Prepare engine
    engine->prepare(sampleRate, 512);
    
    // Set parameters
    engine->setParameter(0, 1.0f);  // Mix 100%
    engine->setParameter(1, param1); // Control 1
    engine->setParameter(2, param2); // Control 2
    
    // Process in chunks
    const int chunkSize = 512;
    for (int offset = 0; offset < testSamples; offset += chunkSize) {
        int samplesToProcess = std::min(chunkSize, testSamples - offset);
        float* inPtr = &input[offset];
        float* outPtr = &output[offset];
        engine->process(&inPtr, &outPtr, 1, samplesToProcess);
    }
    
    // Analyze frequency after latency
    int startIdx = 2500;
    int endIdx = 6000;
    
    // Count zero crossings
    int zeroCrossings = 0;
    for (int i = startIdx + 1; i < endIdx; ++i) {
        if ((output[i-1] < 0 && output[i] >= 0) || 
            (output[i-1] >= 0 && output[i] < 0)) {
            zeroCrossings++;
        }
    }
    
    float measuredFreq = (zeroCrossings / 2.0f) * (sampleRate / (endIdx - startIdx));
    
    // Calculate RMS
    float rms = 0;
    for (int i = startIdx; i < endIdx; ++i) {
        rms += output[i] * output[i];
    }
    rms = std::sqrt(rms / (endIdx - startIdx));
    
    // Check results
    float error = std::abs(measuredFreq - expectedFreq);
    float errorPercent = (error / expectedFreq) * 100.0f;
    bool pass = (errorPercent < 10.0f && rms > 0.01f);
    
    std::cout << engineName << ":" << std::endl;
    std::cout << "  Params: control1=" << param1 << ", control2=" << param2 << std::endl;
    std::cout << "  Expected: " << expectedFreq << " Hz" << std::endl;
    std::cout << "  Measured: " << measuredFreq << " Hz" << std::endl;
    std::cout << "  Error: " << errorPercent << "%" << std::endl;
    std::cout << "  RMS: " << rms << std::endl;
    std::cout << "  " << (pass ? "✓ PASS" : "✗ FAIL") << std::endl;
    std::cout << std::endl;
    
    delete engine;
    return pass;
}

int main() {
    std::cout << "=== Testing All Pitch Shifting Engines ===" << std::endl;
    std::cout << "Input: 440Hz sine wave" << std::endl;
    std::cout << std::endl;
    
    int passed = 0;
    int total = 0;
    
    // Test PitchShifter (Gender)
    std::cout << "1. PitchShifter (Vocal Destroyer)" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    // Male gender (control1=0.0)
    total++;
    if (testEngine(new PitchShifter(), "Male Gender", 0.0f, 0.5f, 311.0f)) passed++;
    
    // Female gender (control1=1.0)
    total++;
    if (testEngine(new PitchShifter(), "Female Gender", 1.0f, 0.5f, 622.0f)) passed++;
    
    // Test DetuneDoubler
    std::cout << "2. DetuneDoubler" << std::endl;
    std::cout << "----------------" << std::endl;
    
    // Slight detune (control1=0.05 = 5 cents)
    total++;
    if (testEngine(new AudioDSP::DetuneDoubler(), "5 Cents Detune", 0.05f, 0.5f, 453.0f)) passed++;
    
    // Test IntelligentHarmonizer
    std::cout << "3. IntelligentHarmonizer" << std::endl;
    std::cout << "------------------------" << std::endl;
    
    // Major third up (+4 semitones)
    total++;
    if (testEngine(new IntelligentHarmonizer(), "Major Third", 0.667f, 0.5f, 554.0f)) passed++;
    
    // Test FrequencyShifter
    std::cout << "4. FrequencyShifter" << std::endl;
    std::cout << "-------------------" << std::endl;
    
    // +100Hz shift
    total++;
    if (testEngine(new FrequencyShifter(), "+100Hz Shift", 0.6f, 0.5f, 540.0f)) passed++;
    
    // Test ShimmerReverb
    std::cout << "5. ShimmerReverb" << std::endl;
    std::cout << "----------------" << std::endl;
    
    // Octave up shimmer
    total++;
    if (testEngine(new ShimmerReverb(), "Octave Shimmer", 1.0f, 0.5f, 880.0f)) passed++;
    
    // Summary
    std::cout << "========================================" << std::endl;
    std::cout << "SUMMARY: " << passed << "/" << total << " tests passed" << std::endl;
    
    if (passed == total) {
        std::cout << "✓ ALL PITCH ENGINES WORKING!" << std::endl;
    } else {
        std::cout << "✗ Some engines need attention" << std::endl;
    }
    
    return (passed == total) ? 0 : 1;
}
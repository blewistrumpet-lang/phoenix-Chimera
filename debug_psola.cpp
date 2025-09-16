#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"

constexpr double SAMPLE_RATE = 48000.0;
constexpr int BLOCK_SIZE = 512;
constexpr double PI = 3.14159265358979323846;

// Generate simple sine wave for testing
std::vector<float> generateSineWave(double freq, int numSamples, double sampleRate) {
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        signal[i] = 0.7f * std::sin(2.0 * PI * freq * i / sampleRate);
    }
    return signal;
}

// Simple FFT-based pitch detection
double detectPitch(const std::vector<float>& signal, double sampleRate) {
    int fftSize = 4096;
    juce::dsp::FFT fft(std::log2(fftSize));
    std::vector<std::complex<float>> fftData(fftSize, 0.0f);
    
    // Copy signal to FFT buffer
    for (size_t i = 0; i < std::min(signal.size(), size_t(fftSize)); ++i) {
        fftData[i] = signal[i];
    }
    
    // Perform FFT
    fft.performFrequencyOnlyForwardTransform(reinterpret_cast<float*>(fftData.data()));
    
    // Find peak
    int peakBin = 0;
    float maxMag = 0.0f;
    for (int i = 10; i < fftSize/2; ++i) {  // Skip DC
        float mag = std::abs(fftData[i]);
        if (mag > maxMag) {
            maxMag = mag;
            peakBin = i;
        }
    }
    
    return peakBin * sampleRate / fftSize;
}

void testPitchShift(float intervalParam, const std::string& description) {
    std::cout << "\n" << description << " (param=" << intervalParam << "):\n";
    
    // Create harmonizer
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
    
    // Generate test signal (A440)
    double testFreq = 440.0;
    int testSamples = SAMPLE_RATE * 0.5;  // 0.5 seconds
    auto inputSignal = generateSineWave(testFreq, testSamples, SAMPLE_RATE);
    
    // Set parameters
    std::map<int, float> params;
    params[0] = intervalParam;  // Interval
    params[1] = 0.0f;           // Key (C)
    params[2] = 0.0f;           // Scale (Major)
    params[3] = 0.0f;           // Voices (1)
    params[4] = 0.0f;           // Spread
    params[5] = 0.0f;           // Humanize
    params[6] = 0.0f;           // Formant
    params[7] = 1.0f;           // Mix (100% wet)
    
    harmonizer.updateParameters(params);
    harmonizer.reset();
    
    // Process signal
    std::vector<float> outputSignal;
    outputSignal.reserve(testSamples);
    
    for (int offset = 0; offset < testSamples; offset += BLOCK_SIZE) {
        int samplesToProcess = std::min(BLOCK_SIZE, testSamples - offset);
        
        juce::AudioBuffer<float> buffer(1, samplesToProcess);
        for (int i = 0; i < samplesToProcess; ++i) {
            buffer.setSample(0, i, inputSignal[offset + i]);
        }
        
        harmonizer.process(buffer);
        
        for (int i = 0; i < samplesToProcess; ++i) {
            outputSignal.push_back(buffer.getSample(0, i));
        }
    }
    
    // Analyze results
    double inputPitch = detectPitch(inputSignal, SAMPLE_RATE);
    double outputPitch = detectPitch(outputSignal, SAMPLE_RATE);
    
    // Calculate expected pitch
    float expectedSemitones = (intervalParam - 0.5f) * 48.0f;
    if (std::abs(intervalParam - 0.5f) < 0.01f) expectedSemitones = 0.0f;
    double expectedPitch = testFreq * std::pow(2.0, expectedSemitones / 12.0);
    
    // Calculate RMS
    float rms = 0.0f;
    for (float sample : outputSignal) {
        rms += sample * sample;
    }
    rms = std::sqrt(rms / outputSignal.size());
    
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  Input pitch:    " << inputPitch << " Hz\n";
    std::cout << "  Output pitch:   " << outputPitch << " Hz\n";
    std::cout << "  Expected pitch: " << expectedPitch << " Hz\n";
    std::cout << "  Pitch ratio:    " << (outputPitch / inputPitch) << "\n";
    std::cout << "  Expected ratio: " << std::pow(2.0, expectedSemitones / 12.0) << "\n";
    std::cout << "  Output RMS:     " << rms << "\n";
    
    double error = std::abs(outputPitch - expectedPitch);
    if (error > 20.0 && std::abs(expectedSemitones) > 0.1) {
        std::cout << "  ⚠️  PITCH ERROR: " << error << " Hz\n";
    } else if (std::abs(expectedSemitones) < 0.1 && error < 10.0) {
        std::cout << "  ✓ Unison working correctly\n";
    }
}

void testMixParameter() {
    std::cout << "\n=== TESTING MIX PARAMETER ===\n";
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
    
    // Generate test signal
    auto inputSignal = generateSineWave(440.0, BLOCK_SIZE, SAMPLE_RATE);
    
    // Test Mix = 0 (should be dry)
    std::map<int, float> params;
    params[0] = 0.75f;  // Pitch up
    params[7] = 0.0f;   // Mix = 0 (dry)
    harmonizer.updateParameters(params);
    
    juce::AudioBuffer<float> buffer(1, BLOCK_SIZE);
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        buffer.setSample(0, i, inputSignal[i]);
    }
    
    harmonizer.process(buffer);
    
    // Check correlation
    float correlation = 0.0f;
    float inputNorm = 0.0f;
    float outputNorm = 0.0f;
    
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        float input = inputSignal[i];
        float output = buffer.getSample(0, i);
        correlation += input * output;
        inputNorm += input * input;
        outputNorm += output * output;
    }
    
    if (inputNorm > 0 && outputNorm > 0) {
        correlation /= std::sqrt(inputNorm * outputNorm);
    }
    
    std::cout << "Mix=0 correlation: " << correlation << "\n";
    if (std::abs(correlation) > 0.9f) {
        std::cout << "✓ Dry signal passing correctly\n";
    } else {
        std::cout << "⚠️  Mix parameter not working!\n";
    }
}

int main() {
    std::cout << "=== PSOLA IMPLEMENTATION DEBUG ===\n";
    
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juce_init;
    
    // Test key interval values
    testPitchShift(0.0f, "Minimum (-24 semitones)");
    testPitchShift(0.25f, "Quarter (-12 semitones)");
    testPitchShift(0.5f, "CENTER - UNISON");
    testPitchShift(0.75f, "Three quarters (+12 semitones)");
    testPitchShift(1.0f, "Maximum (+24 semitones)");
    
    // Test mix parameter
    testMixParameter();
    
    std::cout << "\n=== DIAGNOSIS ===\n";
    std::cout << "If all outputs have similar pitch, PSOLA synthesis is broken.\n";
    std::cout << "If Mix=0 correlation is low, dry/wet mixing is broken.\n";
    
    return 0;
}
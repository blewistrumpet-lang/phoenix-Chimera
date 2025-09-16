// Final accuracy test with improved SMBPitchShiftFixed
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 8192;

// Simple but accurate zero-crossing pitch detection
float detectPitchZeroCrossing(const float* buffer, int numSamples, float sampleRate) {
    int crossings = 0;
    int startIdx = numSamples / 4;  // Skip initial transient
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

void testPitchAccuracy() {
    std::cout << "=== FINAL PITCH ACCURACY TEST ===" << std::endl;
    std::cout << "Algorithm improvements:" << std::endl;
    std::cout << "- Double precision phase accumulation" << std::endl;
    std::cout << "- Cubic interpolation for bin remapping" << std::endl;
    std::cout << "- Improved phase unwrapping" << std::endl;
    std::cout << "- Anti-aliasing filter for high ratios" << std::endl;
    std::cout << std::endl;
    
    SMBPitchShiftFixed shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test critical pitch ratios
    struct TestCase {
        float ratio;
        const char* name;
        float inputFreq;
    };
    
    TestCase tests[] = {
        {0.5f, "Octave down", 440.0f},
        {0.75f, "Fourth down", 440.0f},
        {1.0f, "Unison", 440.0f},
        {1.25992f, "Major 3rd", 440.0f},
        {1.5f, "Fifth up", 440.0f},
        {1.68179f, "Major 6th", 440.0f},
        {2.0f, "Octave up", 440.0f},
        // Test with different input frequencies
        {1.5f, "Fifth up (220Hz)", 220.0f},
        {1.5f, "Fifth up (880Hz)", 880.0f},
    };
    
    float totalError = 0.0f;
    int passCount = 0;
    int testCount = sizeof(tests) / sizeof(TestCase);
    
    for (const auto& test : tests) {
        shifter.reset();
        
        // Generate input signal
        std::vector<float> input(BUFFER_SIZE);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            input[i] = std::sin(2.0f * M_PI * test.inputFreq * i / SAMPLE_RATE) * 0.3f;
        }
        
        // Process with multiple passes to stabilize
        std::vector<float> output(BUFFER_SIZE);
        for (int pass = 0; pass < 3; ++pass) {
            std::vector<float> tempOut(BUFFER_SIZE);
            shifter.process(input.data(), tempOut.data(), BUFFER_SIZE, test.ratio);
            if (pass == 2) output = tempOut;
        }
        
        // Measure frequencies
        float inputPitch = detectPitchZeroCrossing(input.data(), BUFFER_SIZE, SAMPLE_RATE);
        float outputPitch = detectPitchZeroCrossing(output.data(), BUFFER_SIZE, SAMPLE_RATE);
        float expectedPitch = test.inputFreq * test.ratio;
        
        // Calculate RMS
        float outputRMS = 0.0f;
        for (int i = BUFFER_SIZE/4; i < BUFFER_SIZE*3/4; ++i) {
            outputRMS += output[i] * output[i];
        }
        outputRMS = std::sqrt(outputRMS / (BUFFER_SIZE/2));
        
        // Calculate error
        float error = 0.0f;
        if (std::abs(test.ratio - 1.0f) < 0.001f) {
            // Unison should pass through unchanged
            error = std::abs(outputPitch - inputPitch) / inputPitch * 100.0f;
        } else {
            error = std::abs(outputPitch - expectedPitch) / expectedPitch * 100.0f;
        }
        
        totalError += error;
        bool pass = error < 0.5f && outputRMS > 0.01f;
        if (pass) passCount++;
        
        // Display results
        std::cout << std::left << std::setw(20) << test.name 
                  << " | Input: " << std::fixed << std::setprecision(1) 
                  << std::setw(6) << test.inputFreq << " Hz"
                  << " | Expected: " << std::setw(6) << expectedPitch << " Hz"
                  << " | Measured: " << std::setw(6) << outputPitch << " Hz"
                  << " | Error: " << std::setprecision(2) << std::setw(5) << error << "%"
                  << " | " << (pass ? "✓ PASS" : "✗ FAIL") << std::endl;
    }
    
    float avgError = totalError / testCount;
    
    std::cout << std::endl;
    std::cout << "=== RESULTS ===" << std::endl;
    std::cout << "Tests passed: " << passCount << "/" << testCount << std::endl;
    std::cout << "Average error: " << std::fixed << std::setprecision(3) << avgError << "%" << std::endl;
    std::cout << "Target: <0.5% error" << std::endl;
    
    if (avgError < 0.5f) {
        std::cout << "✓ SUCCESS: Target accuracy achieved!" << std::endl;
    } else if (avgError < 1.0f) {
        std::cout << "⚠ GOOD: Sub-1% accuracy achieved" << std::endl;
    } else {
        std::cout << "✗ NEEDS WORK: Error still above 1%" << std::endl;
    }
}

int main() {
    testPitchAccuracy();
    return 0;
}
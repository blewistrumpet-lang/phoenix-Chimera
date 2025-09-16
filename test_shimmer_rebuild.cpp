// Complete Debug and Rebuild test for ShimmerReverb
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"

void printTestHeader(const std::string& testName) {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << testName << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

int main() {
    std::cout << "SHIMMERREVERB DEBUG AND REBUILD TEST" << std::endl;
    std::cout << "Testing current implementation to identify issues" << std::endl;
    
    auto reverb = std::make_unique<ShimmerReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // TEST 1: Basic Functionality - Does it produce ANY output?
    printTestHeader("TEST 1: BASIC OUTPUT TEST");
    {
        // Set to 100% wet to hear only reverb
        std::map<int, float> params;
        params[0] = 0.5f;  // Pitch shift: +octave
        params[1] = 0.3f;  // Shimmer amount: moderate
        params[2] = 0.7f;  // Room size: large
        params[3] = 0.3f;  // Damping: low (bright)
        params[4] = 1.0f;  // Mix: 100% wet
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        
        // Create impulse
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        std::cout << "Processing impulse through ShimmerReverb..." << std::endl;
        std::cout << "Block | RMS Level | Peak Level | Status" << std::endl;
        std::cout << "------|-----------|------------|--------" << std::endl;
        
        float totalEnergy = 0.0f;
        bool hasOutput = false;
        
        for (int block = 0; block < 10; block++) {
            reverb->process(buffer);
            
            float rms = buffer.getRMSLevel(0, 0, 512);
            float peak = buffer.getMagnitude(0, 512);
            totalEnergy += rms;
            
            if (rms > 0.0001f) hasOutput = true;
            
            std::cout << std::setw(5) << block << " | " 
                      << std::fixed << std::setprecision(6) << std::setw(9) << rms << " | "
                      << std::setw(10) << peak << " | ";
            
            if (rms > 0.001f) {
                std::cout << "ACTIVE";
            } else if (rms > 0.0001f) {
                std::cout << "MINIMAL";
            } else {
                std::cout << "SILENT";
            }
            std::cout << std::endl;
            
            // Clear input after first block to hear only tail
            if (block == 0) buffer.clear();
        }
        
        std::cout << "\nTotal reverb energy: " << totalEnergy << std::endl;
        std::cout << "Result: " << (hasOutput ? "PRODUCES OUTPUT ✓" : "COMPLETE SILENCE - BROKEN ✗") << std::endl;
    }
    
    // TEST 2: Parameter Response Test
    printTestHeader("TEST 2: PARAMETER RESPONSE");
    {
        std::cout << "Testing if parameters affect output..." << std::endl;
        
        float paramOutputs[3] = {0.0f, 0.0f, 0.0f};
        float shimmerAmounts[3] = {0.0f, 0.5f, 1.0f};  // None, moderate, max
        
        for (int test = 0; test < 3; test++) {
            reverb->reset();
            
            // Set parameters
            std::map<int, float> params;
            params[0] = 0.5f;  // Pitch shift: +octave
            params[1] = shimmerAmounts[test];  // Shimmer amount: variable
            params[2] = 0.7f;  // Room size: large
            params[3] = 0.3f;  // Damping: low
            params[4] = 1.0f;  // Mix: 100% wet
            reverb->updateParameters(params);
            
            // Send impulse
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);
            
            // Measure output energy
            for (int block = 0; block < 10; block++) {
                reverb->process(buffer);
                if (block > 0) {  // Skip first block (contains impulse)
                    paramOutputs[test] += buffer.getRMSLevel(0, 0, 512);
                }
                if (block == 0) buffer.clear();
            }
            
            std::cout << "Shimmer=" << shimmerAmounts[test] 
                      << " -> Total energy: " << paramOutputs[test] << std::endl;
        }
        
        bool parametersWork = (paramOutputs[2] > paramOutputs[0] * 1.2f);
        std::cout << "Result: " << (parametersWork ? "PARAMETERS AFFECT OUTPUT ✓" : "PARAMETERS BROKEN ✗") << std::endl;
    }
    
    // TEST 3: Pitch Shift Detection
    printTestHeader("TEST 3: PITCH SHIFT DETECTION");
    {
        std::cout << "Testing if pitch shift is working..." << std::endl;
        
        reverb->reset();
        
        // Set for maximum shimmer effect
        std::map<int, float> params;
        params[0] = 1.0f;  // Pitch shift: +2 octaves (should be very obvious)
        params[1] = 1.0f;  // Shimmer amount: maximum
        params[2] = 0.5f;  // Room size: medium
        params[3] = 0.1f;  // Damping: minimal (preserve harmonics)
        params[4] = 1.0f;  // Mix: 100% wet
        reverb->updateParameters(params);
        
        // Generate 440Hz sine wave
        juce::AudioBuffer<float> buffer(2, 512);
        float phase = 0.0f;
        for (int s = 0; s < 512; s++) {
            float sample = 0.5f * std::sin(2.0f * M_PI * phase);
            buffer.setSample(0, s, sample);
            buffer.setSample(1, s, sample);
            phase += 440.0f / 44100.0f;
            if (phase > 1.0f) phase -= 1.0f;
        }
        
        float inputRMS = buffer.getRMSLevel(0, 0, 512);
        
        // Process multiple blocks
        for (int i = 0; i < 10; i++) {
            reverb->process(buffer);
        }
        
        float outputRMS = buffer.getRMSLevel(0, 0, 512);
        
        // Simple harmonic detection: count zero crossings
        int zeroCrossings = 0;
        auto* data = buffer.getReadPointer(0);
        for (int i = 1; i < 512; i++) {
            if ((data[i-1] < 0 && data[i] >= 0) || (data[i-1] >= 0 && data[i] < 0)) {
                zeroCrossings++;
            }
        }
        
        float estimatedFreq = (zeroCrossings / 2.0f) * (44100.0f / 512.0f);
        
        std::cout << "Input: 440Hz sine wave (RMS=" << inputRMS << ")" << std::endl;
        std::cout << "Output RMS: " << outputRMS << std::endl;
        std::cout << "Estimated output frequency: " << estimatedFreq << "Hz" << std::endl;
        
        bool hasPitchShift = (estimatedFreq > 600.0f || outputRMS > inputRMS * 0.1f);
        std::cout << "Result: " << (hasPitchShift ? "PITCH SHIFT DETECTED ✓" : "NO PITCH SHIFT ✗") << std::endl;
    }
    
    // TEST 4: Feedback Stability
    printTestHeader("TEST 4: FEEDBACK STABILITY");
    {
        std::cout << "Testing feedback stability with continuous input..." << std::endl;
        
        reverb->reset();
        
        // Set moderate parameters
        std::map<int, float> params;
        params[0] = 0.5f;  // Pitch shift: +octave
        params[1] = 0.7f;  // Shimmer amount: high but not max
        params[2] = 0.8f;  // Room size: large
        params[3] = 0.5f;  // Damping: moderate
        params[4] = 0.7f;  // Mix: 70% wet
        reverb->updateParameters(params);
        
        // Process white noise continuously
        juce::Random rng;
        float maxLevel = 0.0f;
        bool exploded = false;
        
        for (int block = 0; block < 20; block++) {
            juce::AudioBuffer<float> buffer(2, 512);
            
            // Generate white noise
            for (int s = 0; s < 512; s++) {
                float sample = rng.nextFloat() * 0.1f - 0.05f;
                buffer.setSample(0, s, sample);
                buffer.setSample(1, s, sample);
            }
            
            reverb->process(buffer);
            
            float peak = buffer.getMagnitude(0, 512);
            maxLevel = std::max(maxLevel, peak);
            
            if (peak > 2.0f) {
                exploded = true;
                std::cout << "FEEDBACK EXPLOSION at block " << block << " (peak=" << peak << ")" << std::endl;
                break;
            }
            
            if (block % 5 == 0) {
                std::cout << "Block " << block << ": Peak=" << peak << std::endl;
            }
        }
        
        if (!exploded) {
            std::cout << "Maximum peak level: " << maxLevel << std::endl;
            std::cout << "Result: STABLE FEEDBACK ✓" << std::endl;
        } else {
            std::cout << "Result: UNSTABLE - FEEDBACK EXPLOSION ✗" << std::endl;
        }
    }
    
    // DIAGNOSIS
    printTestHeader("DIAGNOSIS SUMMARY");
    std::cout << "ShimmerReverb current status:" << std::endl;
    std::cout << "1. Basic output: ";
    std::cout << "2. Parameter response: ";
    std::cout << "3. Pitch shifting: ";
    std::cout << "4. Feedback stability: ";
    std::cout << "\nROOT CAUSE ANALYSIS:" << std::endl;
    std::cout << "If output is silent, check:" << std::endl;
    std::cout << "- SMBPitchShiftFixed initialization" << std::endl;
    std::cout << "- Parameter mapping to DSP coefficients" << std::endl;
    std::cout << "- Feedback gain coefficients" << std::endl;
    std::cout << "- Mix parameter application" << std::endl;
    
    return 0;
}
// Comprehensive test for all reverbs with different pitches and volumes
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include <vector>
#include <functional>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

struct TestResult {
    bool passed = true;
    std::string details;
};

class ReverbTester {
private:
    double sampleRate = 44100.0;
    int blockSize = 512;
    
    // Test frequencies covering full spectrum
    std::vector<float> testFrequencies = {
        40.0f,    // Sub bass
        80.0f,    // Bass
        200.0f,   // Low mid
        440.0f,   // A4
        1000.0f,  // Mid
        2000.0f,  // Upper mid
        5000.0f,  // Presence
        10000.0f, // High
        15000.0f  // Very high
    };
    
    // Test volumes from quiet to loud
    std::vector<float> testVolumes = {
        0.001f,  // Very quiet (-60dB)
        0.01f,   // Quiet (-40dB)
        0.1f,    // Moderate (-20dB)
        0.3f,    // Normal (-10dB)
        0.5f,    // Loud (-6dB)
        0.7f,    // Very loud (-3dB)
        0.9f,    // Near clipping (-1dB)
        1.0f     // Full scale (0dB)
    };
    
public:
    void printHeader(const std::string& title) {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << title << std::endl;
        std::cout << std::string(70, '=') << std::endl;
    }
    
    void printSubHeader(const std::string& title) {
        std::cout << "\n--- " << title << " ---" << std::endl;
    }
    
    // Generate test signal at specific frequency and volume
    void generateTestSignal(juce::AudioBuffer<float>& buffer, float frequency, float amplitude, bool noise = false) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        for (int ch = 0; ch < numChannels; ch++) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; i++) {
                if (noise) {
                    // White noise for testing frequency response
                    data[i] = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * amplitude;
                } else {
                    // Sine wave
                    float phase = 2.0f * M_PI * frequency * i / sampleRate;
                    data[i] = std::sin(phase) * amplitude;
                }
            }
        }
    }
    
    // Test reverb with various input levels
    TestResult testVolumeResponse(EngineBase* reverb, const std::string& reverbName) {
        TestResult result;
        std::cout << "Testing " << reverbName << " volume response..." << std::endl;
        
        reverb->prepareToPlay(sampleRate, blockSize);
        
        // Set moderate reverb settings
        std::map<int, float> params;
        params[0] = 0.7f;  // Mix
        params[1] = 0.3f;  // Parameter 2 (varies by reverb)
        params[2] = 0.5f;  // Parameter 3
        params[3] = 0.5f;  // Parameter 4
        reverb->updateParameters(params);
        
        bool anyClipping = false;
        bool anyNaN = false;
        bool producesOutput = false;
        
        for (float volume : testVolumes) {
            reverb->reset();
            
            juce::AudioBuffer<float> buffer(2, blockSize);
            generateTestSignal(buffer, 440.0f, volume);
            
            // Store input peak
            float inputPeak = buffer.getMagnitude(0, blockSize);
            
            // Process
            reverb->process(buffer);
            
            // Check output
            float outputPeak = buffer.getMagnitude(0, blockSize);
            float outputRMS = buffer.getRMSLevel(0, 0, blockSize);
            
            // Check for issues
            bool hasNaN = false;
            bool hasClipping = false;
            for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; i++) {
                    if (!std::isfinite(data[i])) {
                        hasNaN = true;
                        anyNaN = true;
                    }
                    if (std::abs(data[i]) > 1.0f) {
                        hasClipping = true;
                        anyClipping = true;
                    }
                }
            }
            
            if (outputRMS > 0.0001f) producesOutput = true;
            
            std::cout << "  Volume " << std::fixed << std::setprecision(3) << volume 
                      << " -> Output peak: " << outputPeak 
                      << " RMS: " << outputRMS;
            
            if (hasNaN) std::cout << " [NaN!]";
            if (hasClipping) std::cout << " [CLIP!]";
            if (outputPeak > inputPeak * 1.5f) std::cout << " [GAIN!]";
            
            std::cout << std::endl;
        }
        
        if (anyNaN) {
            result.passed = false;
            result.details += "NaN values detected! ";
        }
        if (anyClipping) {
            result.passed = false;
            result.details += "Clipping detected! ";
        }
        if (!producesOutput) {
            result.passed = false;
            result.details += "No output produced! ";
        }
        
        if (result.passed) {
            result.details = "All volume levels handled correctly";
        }
        
        return result;
    }
    
    // Test reverb with different frequencies
    TestResult testFrequencyResponse(EngineBase* reverb, const std::string& reverbName) {
        TestResult result;
        std::cout << "Testing " << reverbName << " frequency response..." << std::endl;
        
        reverb->prepareToPlay(sampleRate, blockSize);
        
        // Set moderate reverb settings
        std::map<int, float> params;
        params[0] = 0.7f;  // Mix
        params[1] = 0.3f;  // Parameter 2
        params[2] = 0.5f;  // Parameter 3
        params[3] = 0.5f;  // Parameter 4
        reverb->updateParameters(params);
        
        std::vector<float> frequencyOutputs;
        
        for (float freq : testFrequencies) {
            reverb->reset();
            
            juce::AudioBuffer<float> buffer(2, blockSize);
            generateTestSignal(buffer, freq, 0.3f);
            
            // Process multiple blocks to let reverb build
            float totalEnergy = 0.0f;
            for (int block = 0; block < 5; block++) {
                reverb->process(buffer);
                totalEnergy += buffer.getRMSLevel(0, 0, blockSize);
                if (block == 0) {
                    // Clear after first block to measure reverb tail
                    buffer.clear();
                }
            }
            
            frequencyOutputs.push_back(totalEnergy);
            
            std::cout << "  " << std::setw(7) << freq << " Hz -> Energy: " 
                      << std::fixed << std::setprecision(6) << totalEnergy << std::endl;
        }
        
        // Check if all frequencies produce some output
        bool allFrequenciesWork = true;
        for (size_t i = 0; i < frequencyOutputs.size(); i++) {
            if (frequencyOutputs[i] < 0.0001f) {
                allFrequenciesWork = false;
                result.details += "No output at " + std::to_string(testFrequencies[i]) + "Hz. ";
            }
        }
        
        // Check for reasonable frequency balance (not too much variation)
        float minEnergy = *std::min_element(frequencyOutputs.begin(), frequencyOutputs.end());
        float maxEnergy = *std::max_element(frequencyOutputs.begin(), frequencyOutputs.end());
        float ratio = maxEnergy / (minEnergy + 0.0001f);
        
        if (ratio > 100.0f) {
            result.passed = false;
            result.details += "Extreme frequency imbalance (ratio: " + std::to_string(ratio) + "). ";
        }
        
        if (!allFrequenciesWork) {
            result.passed = false;
        }
        
        if (result.passed) {
            result.details = "All frequencies handled well (balance ratio: " + 
                            std::to_string(ratio) + ")";
        }
        
        return result;
    }
    
    // Test all parameters at different settings
    TestResult testParameterCombinations(EngineBase* reverb, const std::string& reverbName) {
        TestResult result;
        std::cout << "Testing " << reverbName << " parameter combinations..." << std::endl;
        
        reverb->prepareToPlay(sampleRate, blockSize);
        
        int numParams = reverb->getNumParameters();
        
        // Test extreme parameter combinations
        std::vector<std::vector<float>> testCombinations = {
            {0.0f, 0.0f, 0.0f, 0.0f},  // All minimum
            {1.0f, 1.0f, 1.0f, 1.0f},  // All maximum
            {0.0f, 1.0f, 0.0f, 1.0f},  // Alternating
            {1.0f, 0.0f, 1.0f, 0.0f},  // Alternating reversed
            {0.5f, 0.5f, 0.5f, 0.5f},  // All middle
            {0.1f, 0.9f, 0.2f, 0.8f},  // Mixed
            {0.9f, 0.1f, 0.8f, 0.2f}   // Mixed reversed
        };
        
        bool anyIssues = false;
        
        for (const auto& combo : testCombinations) {
            reverb->reset();
            
            std::map<int, float> params;
            for (int i = 0; i < std::min(numParams, (int)combo.size()); i++) {
                params[i] = combo[i];
            }
            reverb->updateParameters(params);
            
            // Test with various inputs
            for (float freq : {100.0f, 1000.0f, 5000.0f}) {
                for (float vol : {0.1f, 0.5f, 0.9f}) {
                    juce::AudioBuffer<float> buffer(2, blockSize);
                    generateTestSignal(buffer, freq, vol);
                    
                    // Process
                    reverb->process(buffer);
                    
                    // Check for issues
                    bool hasNaN = false;
                    for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
                        const float* data = buffer.getReadPointer(ch);
                        for (int i = 0; i < blockSize; i++) {
                            if (!std::isfinite(data[i])) {
                                hasNaN = true;
                                anyIssues = true;
                            }
                        }
                    }
                    
                    if (hasNaN) {
                        std::cout << "  NaN with params: ";
                        for (float p : combo) std::cout << p << " ";
                        std::cout << "at " << freq << "Hz, vol=" << vol << std::endl;
                    }
                }
            }
        }
        
        if (anyIssues) {
            result.passed = false;
            result.details = "Issues found with some parameter combinations";
        } else {
            result.details = "All parameter combinations stable";
        }
        
        return result;
    }
    
    // Test with complex audio (multiple frequencies)
    TestResult testComplexAudio(EngineBase* reverb, const std::string& reverbName) {
        TestResult result;
        std::cout << "Testing " << reverbName << " with complex audio..." << std::endl;
        
        reverb->prepareToPlay(sampleRate, blockSize);
        
        // Set typical reverb settings
        std::map<int, float> params;
        params[0] = 0.5f;  // Mix
        params[1] = 0.3f;
        params[2] = 0.5f;
        params[3] = 0.7f;
        reverb->updateParameters(params);
        
        // Generate complex signal (chord)
        juce::AudioBuffer<float> buffer(2, blockSize * 10);
        buffer.clear();
        
        // Add multiple harmonics (major chord + overtones)
        float fundamentals[] = {261.63f, 329.63f, 392.00f}; // C major
        float amplitudes[] = {0.3f, 0.25f, 0.2f};
        
        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); i++) {
                float sample = 0.0f;
                for (int h = 0; h < 3; h++) {
                    // Fundamental
                    sample += std::sin(2.0f * M_PI * fundamentals[h] * i / sampleRate) * amplitudes[h];
                    // 2nd harmonic
                    sample += std::sin(2.0f * M_PI * fundamentals[h] * 2 * i / sampleRate) * amplitudes[h] * 0.5f;
                    // 3rd harmonic
                    sample += std::sin(2.0f * M_PI * fundamentals[h] * 3 * i / sampleRate) * amplitudes[h] * 0.25f;
                }
                data[i] = sample * 0.3f; // Scale down to prevent clipping
            }
        }
        
        // Process in blocks
        float peakLevel = 0.0f;
        bool hasDistortion = false;
        
        for (int block = 0; block < 10; block++) {
            juce::AudioBuffer<float> blockBuffer(2, blockSize);
            for (int ch = 0; ch < 2; ch++) {
                blockBuffer.copyFrom(ch, 0, buffer, ch, block * blockSize, blockSize);
            }
            
            float inputPeak = blockBuffer.getMagnitude(0, blockSize);
            reverb->process(blockBuffer);
            float outputPeak = blockBuffer.getMagnitude(0, blockSize);
            
            if (outputPeak > peakLevel) peakLevel = outputPeak;
            
            // Check for harmonic distortion (sudden jumps)
            for (int ch = 0; ch < blockBuffer.getNumChannels(); ch++) {
                const float* data = blockBuffer.getReadPointer(ch);
                for (int i = 1; i < blockSize; i++) {
                    float diff = std::abs(data[i] - data[i-1]);
                    if (diff > 0.5f && std::abs(data[i]) < 0.9f) {
                        hasDistortion = true;
                    }
                }
            }
        }
        
        std::cout << "  Peak level: " << peakLevel << std::endl;
        std::cout << "  Distortion: " << (hasDistortion ? "DETECTED" : "None") << std::endl;
        
        if (hasDistortion) {
            result.passed = false;
            result.details = "Harmonic distortion detected with complex audio";
        } else if (peakLevel > 1.0f) {
            result.passed = false;
            result.details = "Output clipping with complex audio";
        } else if (peakLevel < 0.01f) {
            result.passed = false;
            result.details = "No output with complex audio";
        } else {
            result.details = "Complex audio handled correctly";
        }
        
        return result;
    }
    
    // Test impulse response
    TestResult testImpulseResponse(EngineBase* reverb, const std::string& reverbName) {
        TestResult result;
        std::cout << "Testing " << reverbName << " impulse response..." << std::endl;
        
        reverb->prepareToPlay(sampleRate, blockSize);
        
        // Test at different mix levels
        float mixLevels[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float mix : mixLevels) {
            reverb->reset();
            
            std::map<int, float> params;
            params[0] = mix;  // Mix
            params[1] = 0.3f;
            params[2] = 0.5f;
            params[3] = 0.7f;
            reverb->updateParameters(params);
            
            // Send impulse
            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);
            
            // Measure decay
            float energies[10];
            for (int block = 0; block < 10; block++) {
                reverb->process(buffer);
                energies[block] = buffer.getRMSLevel(0, 0, blockSize);
                if (block == 0) buffer.clear();
            }
            
            std::cout << "  Mix=" << mix << " -> ";
            
            // Check behavior at different mix levels
            if (mix == 0.0f) {
                // Should be mostly dry (impulse in first block only)
                if (energies[0] < 0.5f || energies[1] > 0.01f) {
                    std::cout << "Incorrect dry signal" << std::endl;
                    result.passed = false;
                } else {
                    std::cout << "Dry signal correct" << std::endl;
                }
            } else if (mix == 1.0f) {
                // Should have reverb tail
                float totalTail = 0.0f;
                for (int i = 1; i < 10; i++) totalTail += energies[i];
                if (totalTail < 0.01f) {
                    std::cout << "No reverb tail" << std::endl;
                    result.passed = false;
                } else {
                    std::cout << "Reverb tail present (energy=" << totalTail << ")" << std::endl;
                }
            } else {
                // Should be mixed
                std::cout << "Mixed output (first=" << energies[0] << ")" << std::endl;
            }
        }
        
        if (result.passed) {
            result.details = "Impulse response correct at all mix levels";
        }
        
        return result;
    }
    
    // Run all tests for a reverb
    void testReverb(EngineBase* reverb, const std::string& name) {
        printHeader("TESTING " + name);
        
        std::vector<TestResult> results;
        
        printSubHeader("Volume Response Test");
        results.push_back(testVolumeResponse(reverb, name));
        
        printSubHeader("Frequency Response Test");
        results.push_back(testFrequencyResponse(reverb, name));
        
        printSubHeader("Parameter Combinations Test");
        results.push_back(testParameterCombinations(reverb, name));
        
        printSubHeader("Complex Audio Test");
        results.push_back(testComplexAudio(reverb, name));
        
        printSubHeader("Impulse Response Test");
        results.push_back(testImpulseResponse(reverb, name));
        
        // Summary
        printSubHeader("RESULTS FOR " + name);
        int passed = 0;
        int total = results.size();
        
        for (size_t i = 0; i < results.size(); i++) {
            std::string testNames[] = {"Volume", "Frequency", "Parameters", "Complex", "Impulse"};
            std::cout << testNames[i] << ": " 
                      << (results[i].passed ? "PASS ✓" : "FAIL ✗") 
                      << " - " << results[i].details << std::endl;
            if (results[i].passed) passed++;
        }
        
        std::cout << "\nOverall: " << passed << "/" << total << " tests passed";
        if (passed == total) {
            std::cout << " - FULLY FUNCTIONAL ✓" << std::endl;
        } else {
            std::cout << " - NEEDS ATTENTION ✗" << std::endl;
        }
    }
};

int main() {
    std::cout << "COMPREHENSIVE REVERB TEST SUITE" << std::endl;
    std::cout << "Testing all reverbs with various pitches, volumes, and conditions" << std::endl;
    
    ReverbTester tester;
    
    // Test all reverbs
    auto springReverb = std::make_unique<SpringReverb>();
    tester.testReverb(springReverb.get(), "SpringReverb");
    
    auto shimmerReverb = std::make_unique<ShimmerReverb>();
    tester.testReverb(shimmerReverb.get(), "ShimmerReverb");
    
    auto gatedReverb = std::make_unique<GatedReverb>();
    tester.testReverb(gatedReverb.get(), "GatedReverb");
    
    auto plateReverb = std::make_unique<PlateReverb>();
    tester.testReverb(plateReverb.get(), "PlateReverb");
    
    auto convolutionReverb = std::make_unique<ConvolutionReverb>();
    tester.testReverb(convolutionReverb.get(), "ConvolutionReverb");
    
    // Final summary
    tester.printHeader("FINAL SUMMARY");
    std::cout << "All 5 reverb engines have been comprehensively tested with:" << std::endl;
    std::cout << "- 8 different volume levels (from -60dB to 0dB)" << std::endl;
    std::cout << "- 9 different frequencies (40Hz to 15kHz)" << std::endl;
    std::cout << "- Multiple parameter combinations" << std::endl;
    std::cout << "- Complex multi-frequency audio" << std::endl;
    std::cout << "- Impulse response at various mix levels" << std::endl;
    std::cout << "\nCheck results above for any issues that need attention." << std::endl;
    
    return 0;
}
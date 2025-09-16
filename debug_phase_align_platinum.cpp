/**
 * Diagnostic Test for PhaseAlign_Platinum Engine
 * Investigating performance issue under stress conditions
 */

#include <JuceHeader.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <memory>
#include <cmath>
#include <map>
#include <random>

#include "JUCE_Plugin/Source/PhaseAlign_Platinum.h"

void diagnosePhaseAlign() {
    std::cout << "=== PhaseAlign_Platinum Diagnostic Test ===\n\n";

    try {
        auto engine = std::make_unique<PhaseAlign_Platinum>();
        const double sampleRate = 44100.0;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);
        
        juce::AudioBuffer<float> buffer(2, blockSize);
        juce::Random random;
        
        std::cout << "1. Testing with different parameter configurations...\n";
        
        // Test 1: Default parameters
        std::cout << "   a) Default parameters... ";
        std::map<int, float> defaultParams;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            defaultParams[i] = 0.5f;
        }
        engine->updateParameters(defaultParams);
        
        // Generate complex signal
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            float* data = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                float signal = 0.0f;
                signal += 0.2f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * sample / static_cast<float>(sampleRate));
                signal += 0.15f * std::sin(2.0f * juce::MathConstants<float>::pi * 880.0f * sample / static_cast<float>(sampleRate));
                signal += 0.1f * std::sin(2.0f * juce::MathConstants<float>::pi * 1320.0f * sample / static_cast<float>(sampleRate));
                signal += 0.05f * (random.nextFloat() * 2.0f - 1.0f);
                data[sample] = signal;
            }
        }
        
        bool hasValidOutput = true;
        auto start = std::chrono::high_resolution_clock::now();
        engine->process(buffer);
        auto end = std::chrono::high_resolution_clock::now();
        float processingTime = std::chrono::duration<float, std::milli>(end - start).count();
        
        // Check output validity
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* data = buffer.getReadPointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                if (std::isnan(data[sample]) || std::isinf(data[sample])) {
                    hasValidOutput = false;
                    std::cout << "Invalid value at channel " << channel << ", sample " << sample 
                              << ": " << data[sample] << "\n";
                    break;
                }
                if (std::abs(data[sample]) > 10.0f) {
                    hasValidOutput = false;
                    std::cout << "Excessive value at channel " << channel << ", sample " << sample 
                              << ": " << data[sample] << "\n";
                    break;
                }
            }
            if (!hasValidOutput) break;
        }
        
        std::cout << (hasValidOutput ? "✅" : "❌") << " (" << processingTime << " ms)\n";
        
        // Test 2: Auto-align enabled
        std::cout << "   b) Auto-align enabled... ";
        std::map<int, float> autoAlignParams = defaultParams;
        autoAlignParams[0] = 1.0f; // Enable auto-align
        engine->updateParameters(autoAlignParams);
        
        hasValidOutput = true;
        start = std::chrono::high_resolution_clock::now();
        engine->process(buffer);
        end = std::chrono::high_resolution_clock::now();
        processingTime = std::chrono::duration<float, std::milli>(end - start).count();
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* data = buffer.getReadPointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                if (std::isnan(data[sample]) || std::isinf(data[sample]) || std::abs(data[sample]) > 10.0f) {
                    hasValidOutput = false;
                    break;
                }
            }
            if (!hasValidOutput) break;
        }
        
        std::cout << (hasValidOutput ? "✅" : "❌") << " (" << processingTime << " ms)\n";
        
        // Test 3: Extreme phase adjustments
        std::cout << "   c) Extreme phase adjustments... ";
        std::map<int, float> extremeParams = defaultParams;
        extremeParams[2] = 1.0f; // Max low phase
        extremeParams[3] = 1.0f; // Max low-mid phase
        extremeParams[4] = 1.0f; // Max high-mid phase
        extremeParams[5] = 1.0f; // Max high phase
        engine->updateParameters(extremeParams);
        
        hasValidOutput = true;
        start = std::chrono::high_resolution_clock::now();
        engine->process(buffer);
        end = std::chrono::high_resolution_clock::now();
        processingTime = std::chrono::duration<float, std::milli>(end - start).count();
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* data = buffer.getReadPointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                if (std::isnan(data[sample]) || std::isinf(data[sample]) || std::abs(data[sample]) > 10.0f) {
                    hasValidOutput = false;
                    std::cout << "Problem at channel " << channel << ", sample " << sample 
                              << ": " << data[sample] << "\n";
                    break;
                }
            }
            if (!hasValidOutput) break;
        }
        
        std::cout << (hasValidOutput ? "✅" : "❌") << " (" << processingTime << " ms)\n";
        
        // Test 4: Rapid parameter changes (simplified)
        std::cout << "\n2. Testing rapid parameter changes (100 iterations)...\n";
        bool rapidTestPassed = true;
        
        for (int i = 0; i < 100; ++i) {
            std::map<int, float> randomParams;
            for (int j = 0; j < engine->getNumParameters(); ++j) {
                randomParams[j] = random.nextFloat();
            }
            engine->updateParameters(randomParams);
            
            engine->process(buffer);
            
            bool iterationValid = true;
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                const float* data = buffer.getReadPointer(channel);
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                    if (std::isnan(data[sample]) || std::isinf(data[sample]) || std::abs(data[sample]) > 10.0f) {
                        rapidTestPassed = false;
                        iterationValid = false;
                        std::cout << "   Failed at iteration " << i << ", channel " << channel 
                                  << ", sample " << sample << ": " << data[sample] << "\n";
                        std::cout << "   Parameters at failure:\n";
                        for (int k = 0; k < engine->getNumParameters(); ++k) {
                            std::cout << "     [" << k << "] " << engine->getParameterName(k).toStdString() 
                                      << ": " << randomParams[k] << "\n";
                        }
                        break;
                    }
                }
                if (!iterationValid) break;
            }
            if (!iterationValid) break;
        }
        
        std::cout << "Rapid parameter test: " << (rapidTestPassed ? "✅ PASS" : "❌ FAIL") << "\n";
        
        // Test 5: Different input signals
        std::cout << "\n3. Testing with different input signals...\n";
        
        // Reset to safe parameters
        engine->updateParameters(defaultParams);
        
        // Test with silence
        std::cout << "   a) Silence... ";
        buffer.clear();
        engine->process(buffer);
        hasValidOutput = true;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* data = buffer.getReadPointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                if (std::isnan(data[sample]) || std::isinf(data[sample])) {
                    hasValidOutput = false;
                    break;
                }
            }
            if (!hasValidOutput) break;
        }
        std::cout << (hasValidOutput ? "✅" : "❌") << "\n";
        
        // Test with DC offset
        std::cout << "   b) DC offset... ";
        buffer.clear();
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            buffer.getWritePointer(channel)[0] = 0.5f;
        }
        engine->process(buffer);
        hasValidOutput = true;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* data = buffer.getReadPointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                if (std::isnan(data[sample]) || std::isinf(data[sample])) {
                    hasValidOutput = false;
                    break;
                }
            }
            if (!hasValidOutput) break;
        }
        std::cout << (hasValidOutput ? "✅" : "❌") << "\n";
        
        // Test with impulse
        std::cout << "   c) Impulse... ";
        buffer.clear();
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            buffer.getWritePointer(channel)[0] = 1.0f;
        }
        engine->process(buffer);
        hasValidOutput = true;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* data = buffer.getReadPointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                if (std::isnan(data[sample]) || std::isinf(data[sample]) || std::abs(data[sample]) > 10.0f) {
                    hasValidOutput = false;
                    std::cout << "Problem at channel " << channel << ", sample " << sample 
                              << ": " << data[sample] << "\n";
                    break;
                }
            }
            if (!hasValidOutput) break;
        }
        std::cout << (hasValidOutput ? "✅" : "❌") << "\n";
        
        std::cout << "\n=== Diagnostic Complete ===\n";
        
    } catch (const std::exception& e) {
        std::cout << "Exception during diagnostic: " << e.what() << "\n";
    } catch (...) {
        std::cout << "Unknown exception during diagnostic\n";
    }
}

int main() {
    try {
        juce::initialiseJuce_GUI();
        diagnosePhaseAlign();
        juce::shutdownJuce_GUI();
        return 0;
    } catch (...) {
        return 1;
    }
}
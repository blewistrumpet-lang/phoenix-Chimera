#include "EngineDiagnostic.h"
#include "EngineFactory.h"
#include "PlateReverb.h"
#include "ClassicCompressor.h"
#include "RodentDistortion.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>

std::string EngineDiagnostic::DiagnosticResult::toString() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    ss << "=== " << engineName << " Diagnostic ===\n";
    ss << "Processing: " << (isProcessing ? "YES" : "NO") << "\n";
    ss << "Audio Modified: " << (audioModified ? "YES" : "NO") << "\n";
    ss << "Input  - RMS: " << inputRMS << ", Peak: " << inputPeak << "\n";
    ss << "Output - RMS: " << outputRMS << ", Peak: " << outputPeak << "\n";
    ss << "Gain Change: " << gainChange_dB << " dB\n";
    ss << "Mix Value: " << mixValue << "\n";
    
    if (!parameters.empty()) {
        ss << "Parameters:\n";
        for (const auto& [index, value] : parameters) {
            ss << "  [" << index << "] = " << value << "\n";
        }
    }
    
    return ss.str();
}

EngineDiagnostic::DiagnosticResult EngineDiagnostic::testPlateReverb(juce::AudioBuffer<float>& testBuffer) {
    // Create PlateReverb instance
    auto reverb = std::make_unique<PlateReverb>();
    
    // Test parameters for PlateReverb
    std::map<int, float> testParams = {
        {0, 0.7f},  // Size - larger room
        {1, 0.4f},  // Damping - moderate damping  
        {2, 0.1f},  // Predelay - short delay
        {3, 0.5f}   // Mix - 50% wet
    };
    
    return testEngine(testBuffer, "PlateReverb", reverb.get(), testParams);
}

EngineDiagnostic::DiagnosticResult EngineDiagnostic::testClassicCompressor(juce::AudioBuffer<float>& testBuffer) {
    // Create ClassicCompressor instance
    auto compressor = std::make_unique<ClassicCompressor>();
    
    // Test parameters for ClassicCompressor (designed to trigger compression)
    std::map<int, float> testParams = {
        {0, 0.3f},  // Threshold - -18dB (should trigger with our 0.5 amplitude signal)
        {1, 0.6f},  // Ratio - 8:1 ratio
        {2, 0.1f},  // Attack - fast attack (1ms)
        {3, 0.3f},  // Release - moderate release (300ms)
        {4, 0.2f},  // Knee - soft knee
        {5, 0.5f},  // Makeup - some makeup gain
        {6, 1.0f},  // Mix - 100% wet
        {7, 0.0f},  // Lookahead - off
        {8, 0.5f},  // Auto Release - moderate
        {9, 0.0f}   // Sidechain - off
    };
    
    return testEngine(testBuffer, "ClassicCompressor", compressor.get(), testParams);
}

EngineDiagnostic::DiagnosticResult EngineDiagnostic::testRodentDistortion(juce::AudioBuffer<float>& testBuffer) {
    // Create RodentDistortion instance
    auto distortion = std::make_unique<RodentDistortion>();
    
    // Test parameters for RodentDistortion
    std::map<int, float> testParams = {
        {0, 0.7f},  // Gain - high gain for distortion
        {1, 0.5f},  // Filter - moderate filtering
        {2, 0.6f},  // Clipping - significant clipping
        {3, 0.5f},  // Tone - neutral tone
        {4, 0.8f},  // Output - boost output
        {5, 1.0f},  // Mix - 100% wet
        {6, 0.0f},  // Mode - RAT mode
        {7, 0.4f}   // Presence - moderate presence
    };
    
    return testEngine(testBuffer, "RodentDistortion", distortion.get(), testParams);
}

EngineDiagnostic::DiagnosticResult EngineDiagnostic::testEngine(juce::AudioBuffer<float>& testBuffer,
                                                               const std::string& engineName,
                                                               EngineBase* engine,
                                                               const std::map<int, float>& testParams) {
    DiagnosticResult result;
    result.engineName = engineName;
    result.parameters = testParams;
    
    // Create a copy of the input buffer for comparison
    juce::AudioBuffer<float> originalBuffer;
    originalBuffer.makeCopyOf(testBuffer);
    
    // Calculate input levels
    result.inputRMS = calculateRMS(testBuffer);
    result.inputPeak = calculatePeak(testBuffer);
    
    // Extract mix parameter if it exists
    auto mixParam = testParams.find(3);  // Try PlateReverb mix first
    if (mixParam == testParams.end()) {
        mixParam = testParams.find(6);    // Try ClassicCompressor/RodentDistortion mix
    }
    if (mixParam == testParams.end()) {
        mixParam = testParams.find(5);    // Try RodentDistortion mix
    }
    
    if (mixParam != testParams.end()) {
        result.mixValue = mixParam->second;
    }
    
    try {
        // Prepare the engine
        engine->prepareToPlay(44100.0, testBuffer.getNumSamples());
        
        // Update parameters
        engine->updateParameters(testParams);
        
        // Process the buffer
        engine->process(testBuffer);
        result.isProcessing = true;
        
        // Calculate output levels
        result.outputRMS = calculateRMS(testBuffer);
        result.outputPeak = calculatePeak(testBuffer);
        
        // Calculate gain change
        if (result.inputRMS > 0.0001f) {
            result.gainChange_dB = 20.0f * std::log10(result.outputRMS / result.inputRMS);
        } else {
            result.gainChange_dB = 0.0f;
        }
        
        // Check if audio was modified
        result.audioModified = buffersAreDifferent(originalBuffer, testBuffer);
        
    } catch (const std::exception& e) {
        std::cerr << "Error testing " << engineName << ": " << e.what() << std::endl;
        result.isProcessing = false;
    } catch (...) {
        std::cerr << "Unknown error testing " << engineName << std::endl;
        result.isProcessing = false;
    }
    
    return result;
}

float EngineDiagnostic::calculateRMS(const juce::AudioBuffer<float>& buffer) {
    float sumSquares = 0.0f;
    int totalSamples = 0;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* channelData = buffer.getReadPointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            sumSquares += channelData[s] * channelData[s];
            totalSamples++;
        }
    }
    
    return totalSamples > 0 ? std::sqrt(sumSquares / totalSamples) : 0.0f;
}

float EngineDiagnostic::calculatePeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* channelData = buffer.getReadPointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            peak = std::max(peak, std::abs(channelData[s]));
        }
    }
    
    return peak;
}

bool EngineDiagnostic::buffersAreDifferent(const juce::AudioBuffer<float>& buffer1,
                                         const juce::AudioBuffer<float>& buffer2,
                                         float threshold) {
    if (buffer1.getNumChannels() != buffer2.getNumChannels() ||
        buffer1.getNumSamples() != buffer2.getNumSamples()) {
        return true;
    }
    
    for (int ch = 0; ch < buffer1.getNumChannels(); ++ch) {
        const float* data1 = buffer1.getReadPointer(ch);
        const float* data2 = buffer2.getReadPointer(ch);
        
        for (int s = 0; s < buffer1.getNumSamples(); ++s) {
            if (std::abs(data1[s] - data2[s]) > threshold) {
                return true;
            }
        }
    }
    
    return false;
}

void EngineDiagnostic::generateTestTone(juce::AudioBuffer<float>& buffer,
                                      float frequency,
                                      float amplitude,
                                      double sampleRate) {
    const double twoPi = 2.0 * juce::MathConstants<double>::pi;
    const double phaseIncrement = twoPi * frequency / sampleRate;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        double phase = 0.0;
        
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            channelData[s] = amplitude * std::sin(phase);
            phase += phaseIncrement;
            if (phase >= twoPi) {
                phase -= twoPi;
            }
        }
    }
}

void EngineDiagnostic::generateWhiteNoise(juce::AudioBuffer<float>& buffer,
                                        float amplitude,
                                        int seed) {
    std::mt19937 generator(seed);
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            channelData[s] = amplitude * distribution(generator);
        }
    }
}

void EngineDiagnostic::printResults(const std::vector<DiagnosticResult>& results) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "ENGINE DIAGNOSTIC RESULTS\n";
    std::cout << std::string(60, '=') << "\n";
    
    for (const auto& result : results) {
        std::cout << result.toString() << "\n";
    }
    
    // Summary
    std::cout << std::string(60, '-') << "\n";
    std::cout << "SUMMARY:\n";
    
    int workingEngines = 0;
    int modifyingEngines = 0;
    
    for (const auto& result : results) {
        if (result.isProcessing) {
            workingEngines++;
            if (result.audioModified) {
                modifyingEngines++;
            }
        }
    }
    
    std::cout << "Total engines tested: " << results.size() << "\n";
    std::cout << "Engines processing: " << workingEngines << "\n";
    std::cout << "Engines modifying audio: " << modifyingEngines << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

std::vector<EngineDiagnostic::DiagnosticResult> EngineDiagnostic::runComprehensiveTest(double sampleRate, int blockSize) {
    std::vector<DiagnosticResult> results;
    
    std::cout << "Running comprehensive engine diagnostic...\n";
    std::cout << "Sample Rate: " << sampleRate << " Hz\n";
    std::cout << "Block Size: " << blockSize << " samples\n\n";
    
    // Test with different signal types
    std::vector<std::pair<std::string, std::function<void(juce::AudioBuffer<float>&)>>> testSignals = {
        {"1kHz Tone", [sampleRate](juce::AudioBuffer<float>& buf) { generateTestTone(buf, 1000.0f, 0.5f, sampleRate); }},
        {"White Noise", [](juce::AudioBuffer<float>& buf) { generateWhiteNoise(buf, 0.2f); }}
    };
    
    for (const auto& [signalName, signalGenerator] : testSignals) {
        std::cout << "\n--- Testing with " << signalName << " ---\n";
        
        // Create test buffer (stereo)
        juce::AudioBuffer<float> testBuffer(2, blockSize);
        signalGenerator(testBuffer);
        
        // Test PlateReverb
        {
            juce::AudioBuffer<float> buffer;
            buffer.makeCopyOf(testBuffer);
            auto result = testPlateReverb(buffer);
            results.push_back(result);
        }
        
        // Test ClassicCompressor  
        {
            juce::AudioBuffer<float> buffer;
            buffer.makeCopyOf(testBuffer);
            auto result = testClassicCompressor(buffer);
            results.push_back(result);
        }
        
        // Test RodentDistortion
        {
            juce::AudioBuffer<float> buffer;
            buffer.makeCopyOf(testBuffer);
            auto result = testRodentDistortion(buffer);
            results.push_back(result);
        }
    }
    
    return results;
}
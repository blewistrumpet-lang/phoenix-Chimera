/**
 * Comprehensive DSP Engine Test Suite
 * Tests all engines for quality, stability, and correctness
 */

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <random>
#include <memory>
#include <chrono>
#include <iomanip>
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/PluginProcessor.h"

class EngineTestSuite {
public:
    struct TestResult {
        bool passed;
        std::string message;
        float value;
    };
    
    struct EngineTestReport {
        std::string engineName;
        int engineID;
        std::map<std::string, TestResult> tests;
        bool allPassed;
        double cpuUsage;
    };

private:
    const int SAMPLE_RATE = 48000;
    const int BLOCK_SIZE = 512;
    ChimeraPhoenixAudioProcessor processor;
    
    // Test signal generators
    juce::AudioBuffer<float> generateSilence(int numSamples) {
        juce::AudioBuffer<float> buffer(2, numSamples);
        buffer.clear();
        return buffer;
    }
    
    juce::AudioBuffer<float> generateImpulse(int numSamples, int position = 100) {
        juce::AudioBuffer<float> buffer(2, numSamples);
        buffer.clear();
        buffer.setSample(0, position, 1.0f);
        buffer.setSample(1, position, 1.0f);
        return buffer;
    }
    
    juce::AudioBuffer<float> generateSine(int numSamples, float freq, float amp = 0.5f) {
        juce::AudioBuffer<float> buffer(2, numSamples);
        for (int i = 0; i < numSamples; ++i) {
            float sample = amp * std::sin(2.0f * M_PI * freq * i / SAMPLE_RATE);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        return buffer;
    }
    
    juce::AudioBuffer<float> generateNoise(int numSamples, float amp = 0.1f) {
        juce::AudioBuffer<float> buffer(2, numSamples);
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-amp, amp);
        for (int i = 0; i < numSamples; ++i) {
            buffer.setSample(0, i, dist(rng));
            buffer.setSample(1, i, dist(rng));
        }
        return buffer;
    }
    
    // Analysis functions
    float getRMS(const juce::AudioBuffer<float>& buffer, int startSample = 0, int endSample = -1) {
        if (endSample < 0) endSample = buffer.getNumSamples();
        float sum = 0.0f;
        int count = 0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = startSample; i < endSample; ++i) {
                float sample = buffer.getSample(ch, i);
                sum += sample * sample;
                count++;
            }
        }
        return std::sqrt(sum / count);
    }
    
    float getPeak(const juce::AudioBuffer<float>& buffer) {
        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                peak = std::max(peak, std::abs(buffer.getSample(ch, i)));
            }
        }
        return peak;
    }
    
    bool hasNaNOrInf(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                if (std::isnan(sample) || std::isinf(sample)) {
                    return true;
                }
            }
        }
        return false;
    }
    
    bool hasDenormals(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = std::abs(buffer.getSample(ch, i));
                if (sample > 0.0f && sample < 1e-30f) {
                    return true;
                }
            }
        }
        return false;
    }
    
public:
    // Core invariant tests
    TestResult testBypassMode(EngineBase* engine) {
        auto input = generateSine(SAMPLE_RATE, 440.0f, 0.5f);
        auto output = input;
        
        // Set mix to 0 (bypass)
        std::map<int, float> params;
        int mixIndex = processor.getMixParameterIndex(0); // Get proper mix index
        params[mixIndex] = 0.0f;
        engine->updateParameters(params);
        
        engine->process(output);
        
        // Check if output matches input (bypass)
        float diff = 0.0f;
        for (int i = 0; i < output.getNumSamples(); ++i) {
            diff += std::abs(output.getSample(0, i) - input.getSample(0, i));
        }
        diff /= output.getNumSamples();
        
        return {diff < 0.001f, "Bypass diff: " + std::to_string(diff), diff};
    }
    
    TestResult testNaNInfProtection(EngineBase* engine) {
        auto buffer = generateSine(SAMPLE_RATE, 440.0f);
        engine->process(buffer);
        
        bool hasInvalid = hasNaNOrInf(buffer);
        return {!hasInvalid, hasInvalid ? "Found NaN/Inf" : "No NaN/Inf", 0.0f};
    }
    
    TestResult testDenormalProtection(EngineBase* engine) {
        // Process very quiet signal that could produce denormals
        auto buffer = generateSine(SAMPLE_RATE * 2, 100.0f, 1e-35f);
        engine->process(buffer);
        
        bool hasDenorm = hasDenormals(buffer);
        return {!hasDenorm, hasDenorm ? "Found denormals" : "No denormals", 0.0f};
    }
    
    TestResult testResetCompleteness(EngineBase* engine) {
        // Process impulse
        auto buffer1 = generateImpulse(BLOCK_SIZE);
        engine->process(buffer1);
        
        // Reset
        engine->reset();
        
        // Process silence and check for residual signal
        auto buffer2 = generateSilence(BLOCK_SIZE);
        engine->process(buffer2);
        
        float residual = getRMS(buffer2);
        return {residual < 1e-6f, "Residual after reset: " + std::to_string(residual), residual};
    }
    
    TestResult testBlockSizeInvariance(EngineBase* engine) {
        // Process in one block
        auto input = generateSine(SAMPLE_RATE, 440.0f);
        auto output1 = input;
        engine->reset();
        engine->process(output1);
        
        // Process in smaller blocks
        auto output2 = input;
        engine->reset();
        for (int i = 0; i < input.getNumSamples(); i += 64) {
            int samplesToProcess = std::min(64, input.getNumSamples() - i);
            juce::AudioBuffer<float> block(output2.getArrayOfWritePointers(), 2, i, samplesToProcess);
            engine->process(block);
        }
        
        // Compare outputs
        float diff = 0.0f;
        for (int i = 0; i < output1.getNumSamples(); ++i) {
            diff += std::abs(output1.getSample(0, i) - output2.getSample(0, i));
        }
        diff /= output1.getNumSamples();
        
        return {diff < 0.001f, "Block size diff: " + std::to_string(diff), diff};
    }
    
    // Reverb-specific tests
    TestResult testReverbTail(EngineBase* engine) {
        auto buffer = generateImpulse(SAMPLE_RATE * 2);
        
        // Set high mix for full wet signal
        std::map<int, float> params;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            if (engine->getParameterName(i).containsIgnoreCase("mix")) {
                params[i] = 1.0f;
            } else if (engine->getParameterName(i).containsIgnoreCase("size") ||
                      engine->getParameterName(i).containsIgnoreCase("room")) {
                params[i] = 0.8f;
            }
        }
        engine->updateParameters(params);
        engine->process(buffer);
        
        // Check for reverb tail
        float tailEnergy = getRMS(buffer, SAMPLE_RATE / 4, SAMPLE_RATE);
        return {tailEnergy > 0.001f, "Tail energy: " + std::to_string(tailEnergy), tailEnergy};
    }
    
    // Delay-specific tests
    TestResult testDelayTime(EngineBase* engine) {
        // Set delay to 100ms
        std::map<int, float> params;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            if (engine->getParameterName(i).containsIgnoreCase("time") ||
                engine->getParameterName(i).containsIgnoreCase("delay")) {
                params[i] = 0.1f; // Assuming normalized 0-1 maps to reasonable range
            } else if (engine->getParameterName(i).containsIgnoreCase("mix")) {
                params[i] = 1.0f;
            }
        }
        engine->updateParameters(params);
        
        auto buffer = generateImpulse(SAMPLE_RATE);
        engine->process(buffer);
        
        // Find delayed peak
        int expectedDelay = SAMPLE_RATE / 10; // 100ms
        int actualDelay = 0;
        float maxSample = 0.0f;
        
        for (int i = 1000; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(buffer.getSample(0, i));
            if (sample > maxSample) {
                maxSample = sample;
                actualDelay = i;
            }
        }
        
        int error = std::abs(actualDelay - expectedDelay);
        return {error < SAMPLE_RATE / 100, "Delay error: " + std::to_string(error) + " samples", (float)error};
    }
    
    // EQ-specific tests
    TestResult testEQFrequencyResponse(EngineBase* engine) {
        // Test at 1kHz
        auto buffer1k = generateSine(SAMPLE_RATE, 1000.0f);
        engine->process(buffer1k);
        float gain1k = getRMS(buffer1k) / 0.5f;
        
        // Test at 100Hz
        engine->reset();
        auto buffer100 = generateSine(SAMPLE_RATE, 100.0f);
        engine->process(buffer100);
        float gain100 = getRMS(buffer100) / 0.5f;
        
        // Test at 10kHz
        engine->reset();
        auto buffer10k = generateSine(SAMPLE_RATE, 10000.0f);
        engine->process(buffer10k);
        float gain10k = getRMS(buffer10k) / 0.5f;
        
        // Check if frequency response is reasonable
        bool reasonable = (gain100 > 0.1f && gain100 < 10.0f) &&
                         (gain1k > 0.1f && gain1k < 10.0f) &&
                         (gain10k > 0.1f && gain10k < 10.0f);
        
        return {reasonable, 
                "Gains: 100Hz=" + std::to_string(gain100) + 
                " 1kHz=" + std::to_string(gain1k) + 
                " 10kHz=" + std::to_string(gain10k), 
                gain1k};
    }
    
    // Run all tests for an engine
    EngineTestReport testEngine(int engineID, const std::string& engineName) {
        EngineTestReport report;
        report.engineID = engineID;
        report.engineName = engineName;
        report.allPassed = true;
        
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            report.tests["Creation"] = {false, "Failed to create engine", 0.0f};
            report.allPassed = false;
            return report;
        }
        
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // Core tests for all engines
        report.tests["Bypass"] = testBypassMode(engine.get());
        report.tests["NaN/Inf"] = testNaNInfProtection(engine.get());
        report.tests["Denormal"] = testDenormalProtection(engine.get());
        report.tests["Reset"] = testResetCompleteness(engine.get());
        report.tests["BlockSize"] = testBlockSizeInvariance(engine.get());
        
        // Type-specific tests
        std::string nameLower = engineName;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        
        if (nameLower.find("reverb") != std::string::npos ||
            nameLower.find("plate") != std::string::npos ||
            nameLower.find("spring") != std::string::npos ||
            nameLower.find("convolution") != std::string::npos) {
            report.tests["ReverbTail"] = testReverbTail(engine.get());
        }
        
        if (nameLower.find("delay") != std::string::npos ||
            nameLower.find("echo") != std::string::npos) {
            report.tests["DelayTime"] = testDelayTime(engine.get());
        }
        
        if (nameLower.find("eq") != std::string::npos ||
            nameLower.find("filter") != std::string::npos) {
            report.tests["FreqResponse"] = testEQFrequencyResponse(engine.get());
        }
        
        // CPU usage test
        auto start = std::chrono::high_resolution_clock::now();
        auto buffer = generateNoise(SAMPLE_RATE);
        engine->process(buffer);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        report.cpuUsage = (elapsed.count() / 1.0) * 100.0; // Percentage of real-time
        
        // Check if all tests passed
        for (const auto& test : report.tests) {
            if (!test.second.passed) {
                report.allPassed = false;
            }
        }
        
        return report;
    }
    
    void runFullSuite() {
        std::cout << "===========================================\n";
        std::cout << "   Comprehensive DSP Engine Test Suite\n";
        std::cout << "===========================================\n\n";
        
        std::vector<std::pair<int, std::string>> engines = {
            // Reverbs
            {6, "SpringReverb"},
            {7, "ConvolutionReverb"},
            {8, "PlateReverb"},
            {9, "GatedReverb"},
            {10, "ShimmerReverb"},
            
            // Delays
            {11, "DigitalDelay"},
            {12, "TapeEcho"},
            {13, "BucketBrigadeDelay"},
            {42, "MagneticDrumEcho"},
            
            // EQs
            {16, "ParametricEQ"},
            {17, "VintageConsoleEQ"},
            {18, "DynamicEQ"},
            
            // Newly implemented
            {26, "ResonantChorus"},
            {34, "SpectralGate"},
            
            // Effects with fixed issues
            {39, "BufferRepeat"},
            {25, "AnalogRingModulator"},
            {44, "StereoImager"}
        };
        
        int totalPassed = 0;
        int totalFailed = 0;
        std::vector<EngineTestReport> reports;
        
        for (const auto& [id, name] : engines) {
            std::cout << "Testing " << name << " (ID " << id << ")...\n";
            auto report = testEngine(id, name);
            reports.push_back(report);
            
            if (report.allPassed) {
                std::cout << "  ✅ All tests passed\n";
                totalPassed++;
            } else {
                std::cout << "  ❌ Some tests failed\n";
                totalFailed++;
            }
            
            for (const auto& [testName, result] : report.tests) {
                std::cout << "    " << std::setw(15) << testName << ": " 
                         << (result.passed ? "✅" : "❌") << " " << result.message << "\n";
            }
            
            std::cout << "    CPU Usage: " << std::fixed << std::setprecision(2) 
                     << report.cpuUsage << "%\n\n";
        }
        
        // Summary
        std::cout << "===========================================\n";
        std::cout << "                 SUMMARY\n";
        std::cout << "===========================================\n";
        std::cout << "Engines Passed: " << totalPassed << "/" << engines.size() << "\n";
        std::cout << "Engines Failed: " << totalFailed << "/" << engines.size() << "\n";
        
        if (totalFailed > 0) {
            std::cout << "\nFailed Engines:\n";
            for (const auto& report : reports) {
                if (!report.allPassed) {
                    std::cout << "  - " << report.engineName << "\n";
                    for (const auto& [testName, result] : report.tests) {
                        if (!result.passed) {
                            std::cout << "      " << testName << ": " << result.message << "\n";
                        }
                    }
                }
            }
        }
        
        std::cout << "\n";
        if (totalFailed == 0) {
            std::cout << "🎉 SUCCESS: All engines passed all tests!\n";
        } else {
            std::cout << "⚠️  WARNING: " << totalFailed << " engines need attention\n";
        }
    }
};

int main() {
    EngineTestSuite suite;
    suite.runFullSuite();
    return 0;
}
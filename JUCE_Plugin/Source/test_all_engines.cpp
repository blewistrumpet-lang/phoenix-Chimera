/**
 * Comprehensive Standalone Testing Framework for All 57 Chimera DSP Engines
 * 
 * This file provides a complete testing suite that can run independently
 * without requiring the full plugin framework. It tests each engine for:
 * - Audio processing functionality (not just passthrough)
 * - Mix/wet/dry controls
 * - Parameter effects on sound
 * - Stability (no NaN/Inf/crashes)
 * - Proper gain staging
 * 
 * Usage: Compile and run as standalone executable
 * Output: Clear pass/fail status for each engine with detailed diagnostics
 * 
 * Author: Claude Code Testing Framework
 * Date: 2025-08-07
 */

#include <JuceHeader.h>
#include "EngineFactory.h"
#include "EngineTypes.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <chrono>
#include <sstream>

// Test Constants
const float SAMPLE_RATE = 48000.0f;
const int BLOCK_SIZE = 512;
const float TEST_DURATION = 1.0f;
const float SILENCE_THRESHOLD_DB = -80.0f;
const float MIN_PROCESSING_CHANGE_DB = -40.0f; // Minimum change to consider "processing"
const float MAX_CPU_USAGE_MS = 10.0f; // Maximum processing time per block

// Test Signal Generator
class TestSignalGenerator {
public:
    static juce::AudioBuffer<float> generateSineWave(float frequency, float durationSec) {
        int numSamples = static_cast<int>(durationSec * SAMPLE_RATE);
        juce::AudioBuffer<float> buffer(2, numSamples);
        
        for (int channel = 0; channel < 2; ++channel) {
            auto* data = buffer.getWritePointer(channel);
            for (int i = 0; i < numSamples; ++i) {
                data[i] = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * i / SAMPLE_RATE);
            }
        }
        return buffer;
    }
    
    static juce::AudioBuffer<float> generateWhiteNoise(float durationSec) {
        int numSamples = static_cast<int>(durationSec * SAMPLE_RATE);
        juce::AudioBuffer<float> buffer(2, numSamples);
        
        juce::Random random;
        for (int channel = 0; channel < 2; ++channel) {
            auto* data = buffer.getWritePointer(channel);
            for (int i = 0; i < numSamples; ++i) {
                data[i] = 0.25f * (2.0f * random.nextFloat() - 1.0f);
            }
        }
        return buffer;
    }
    
    static juce::AudioBuffer<float> generateImpulse() {
        juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
        buffer.clear();
        
        // Single impulse at the start
        for (int channel = 0; channel < 2; ++channel) {
            buffer.setSample(channel, 0, 1.0f);
        }
        return buffer;
    }
    
    static juce::AudioBuffer<float> generateSilence(float durationSec) {
        int numSamples = static_cast<int>(durationSec * SAMPLE_RATE);
        juce::AudioBuffer<float> buffer(2, numSamples);
        buffer.clear();
        return buffer;
    }
};

// Audio Analysis Functions
class AudioAnalysis {
public:
    static float calculateRMS(const juce::AudioBuffer<float>& buffer) {
        float sumSquares = 0.0f;
        int totalSamples = 0;
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const auto* data = buffer.getReadPointer(channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                sumSquares += data[i] * data[i];
                totalSamples++;
            }
        }
        
        return totalSamples > 0 ? std::sqrt(sumSquares / totalSamples) : 0.0f;
    }
    
    static float calculatePeak(const juce::AudioBuffer<float>& buffer) {
        float peak = 0.0f;
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const auto* data = buffer.getReadPointer(channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                peak = std::max(peak, std::abs(data[i]));
            }
        }
        
        return peak;
    }
    
    static float linearToDb(float linear) {
        return linear > 0.0f ? 20.0f * std::log10(linear) : -120.0f;
    }
    
    static bool hasNaNOrInf(const juce::AudioBuffer<float>& buffer) {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const auto* data = buffer.getReadPointer(channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (std::isnan(data[i]) || std::isinf(data[i])) {
                    return true;
                }
            }
        }
        return false;
    }
    
    static bool buffersAreSimilar(const juce::AudioBuffer<float>& buffer1, 
                                  const juce::AudioBuffer<float>& buffer2, 
                                  float thresholdDb = -40.0f) {
        if (buffer1.getNumChannels() != buffer2.getNumChannels() ||
            buffer1.getNumSamples() != buffer2.getNumSamples()) {
            return false;
        }
        
        float maxDiff = 0.0f;
        for (int channel = 0; channel < buffer1.getNumChannels(); ++channel) {
            const auto* data1 = buffer1.getReadPointer(channel);
            const auto* data2 = buffer2.getReadPointer(channel);
            
            for (int i = 0; i < buffer1.getNumSamples(); ++i) {
                maxDiff = std::max(maxDiff, std::abs(data1[i] - data2[i]));
            }
        }
        
        float diffDb = linearToDb(maxDiff);
        return diffDb < thresholdDb;
    }
};

// Test Result Structure
struct TestResult {
    std::string testName;
    bool passed;
    std::string details;
    float measuredValue;
    
    TestResult(const std::string& name, bool pass, const std::string& detail = "", float value = 0.0f)
        : testName(name), passed(pass), details(detail), measuredValue(value) {}
};

struct EngineTestReport {
    int engineID;
    std::string engineName;
    std::vector<TestResult> results;
    bool overallPassed;
    float processingTimeMs;
    std::string summary;
    
    void addResult(const TestResult& result) {
        results.push_back(result);
        if (!result.passed) {
            overallPassed = false;
        }
    }
    
    int getPassedCount() const {
        return std::count_if(results.begin(), results.end(), 
                           [](const TestResult& r) { return r.passed; });
    }
    
    int getFailedCount() const {
        return static_cast<int>(results.size()) - getPassedCount();
    }
};

// Main Testing Class
class EngineTestRunner {
private:
    std::vector<EngineTestReport> allReports;
    
    // Create default parameter map for testing
    std::map<int, float> createDefaultParams(EngineBase* engine) {
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        
        // Set all parameters to mid-range (0.5) initially
        for (int i = 0; i < numParams; ++i) {
            params[i] = 0.5f;
        }
        
        return params;
    }
    
    // Test if engine actually processes audio (not just passthrough)
    TestResult testAudioProcessing(EngineBase* engine) {
        try {
            engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
            engine->reset();
            
            auto params = createDefaultParams(engine);
            engine->updateParameters(params);
            
            // Test with sine wave
            auto inputBuffer = TestSignalGenerator::generateSineWave(1000.0f, 0.1f);
            auto testBuffer = inputBuffer;
            
            engine->process(testBuffer);
            
            // Check for NaN/Inf
            if (AudioAnalysis::hasNaNOrInf(testBuffer)) {
                return TestResult("Audio Processing", false, "Output contains NaN or Inf values");
            }
            
            // Compare input vs output to see if processing occurred
            bool isProcessing = !AudioAnalysis::buffersAreSimilar(inputBuffer, testBuffer, MIN_PROCESSING_CHANGE_DB);
            
            if (!isProcessing) {
                // Try with white noise too
                auto noiseInput = TestSignalGenerator::generateWhiteNoise(0.1f);
                auto noiseTest = noiseInput;
                engine->process(noiseTest);
                
                isProcessing = !AudioAnalysis::buffersAreSimilar(noiseInput, noiseTest, MIN_PROCESSING_CHANGE_DB);
            }
            
            float inputRMS = AudioAnalysis::calculateRMS(inputBuffer);
            float outputRMS = AudioAnalysis::calculateRMS(testBuffer);
            float changeDb = AudioAnalysis::linearToDb(outputRMS) - AudioAnalysis::linearToDb(inputRMS);
            
            std::ostringstream detail;
            detail << "Level change: " << std::fixed << std::setprecision(2) << changeDb << " dB";
            
            return TestResult("Audio Processing", isProcessing, detail.str(), changeDb);
            
        } catch (const std::exception& e) {
            return TestResult("Audio Processing", false, std::string("Exception: ") + e.what());
        } catch (...) {
            return TestResult("Audio Processing", false, "Unknown exception occurred");
        }
    }
    
    // Test mix parameter functionality
    TestResult testMixParameter(EngineBase* engine) {
        try {
            engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
            engine->reset();
            
            auto params = createDefaultParams(engine);
            auto inputBuffer = TestSignalGenerator::generateSineWave(1000.0f, 0.1f);
            
            // Test dry signal (mix = 0)
            params[params.size() - 1] = 0.0f; // Assume last parameter is mix
            engine->updateParameters(params);
            auto dryBuffer = inputBuffer;
            engine->process(dryBuffer);
            
            // Test wet signal (mix = 1)
            params[params.size() - 1] = 1.0f;
            engine->updateParameters(params);
            auto wetBuffer = inputBuffer;
            engine->process(wetBuffer);
            
            // Test mixed signal (mix = 0.5)
            params[params.size() - 1] = 0.5f;
            engine->updateParameters(params);
            auto mixedBuffer = inputBuffer;
            engine->process(mixedBuffer);
            
            // Check if outputs are different
            bool dryWetDifferent = !AudioAnalysis::buffersAreSimilar(dryBuffer, wetBuffer, -20.0f);
            bool mixIsBlended = true;
            
            // Mix should be somewhere between dry and wet
            float dryRMS = AudioAnalysis::calculateRMS(dryBuffer);
            float wetRMS = AudioAnalysis::calculateRMS(wetBuffer);
            float mixRMS = AudioAnalysis::calculateRMS(mixedBuffer);
            
            if (dryWetDifferent) {
                float minRMS = std::min(dryRMS, wetRMS);
                float maxRMS = std::max(dryRMS, wetRMS);
                mixIsBlended = (mixRMS >= minRMS * 0.8f && mixRMS <= maxRMS * 1.2f);
            }
            
            bool passed = dryWetDifferent && mixIsBlended;
            
            std::ostringstream detail;
            detail << "Dry RMS: " << std::fixed << std::setprecision(4) << dryRMS 
                   << ", Wet RMS: " << wetRMS << ", Mix RMS: " << mixRMS;
            
            return TestResult("Mix Parameter", passed, detail.str(), mixRMS);
            
        } catch (...) {
            return TestResult("Mix Parameter", false, "Exception during mix test");
        }
    }
    
    // Test parameter effects on sound
    TestResult testParameterEffects(EngineBase* engine) {
        try {
            engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
            engine->reset();
            
            int numParams = engine->getNumParameters();
            if (numParams == 0) {
                return TestResult("Parameter Effects", true, "No parameters to test");
            }
            
            auto inputBuffer = TestSignalGenerator::generateSineWave(1000.0f, 0.1f);
            
            // Test with default parameters
            auto params = createDefaultParams(engine);
            engine->updateParameters(params);
            auto defaultBuffer = inputBuffer;
            engine->process(defaultBuffer);
            
            bool anyParameterHasEffect = false;
            
            // Test each parameter by setting it to extremes
            for (int i = 0; i < numParams; ++i) {
                // Reset to defaults
                params = createDefaultParams(engine);
                
                // Test parameter at minimum
                params[i] = 0.0f;
                engine->updateParameters(params);
                auto minBuffer = inputBuffer;
                engine->process(minBuffer);
                
                // Test parameter at maximum
                params[i] = 1.0f;
                engine->updateParameters(params);
                auto maxBuffer = inputBuffer;
                engine->process(maxBuffer);
                
                // Check if this parameter has any effect
                bool hasEffect = !AudioAnalysis::buffersAreSimilar(minBuffer, maxBuffer, -30.0f) ||
                               !AudioAnalysis::buffersAreSimilar(defaultBuffer, minBuffer, -30.0f) ||
                               !AudioAnalysis::buffersAreSimilar(defaultBuffer, maxBuffer, -30.0f);
                
                if (hasEffect) {
                    anyParameterHasEffect = true;
                }
            }
            
            std::ostringstream detail;
            detail << "Tested " << numParams << " parameters";
            
            return TestResult("Parameter Effects", anyParameterHasEffect, detail.str(), numParams);
            
        } catch (...) {
            return TestResult("Parameter Effects", false, "Exception during parameter test");
        }
    }
    
    // Test stability and error handling
    TestResult testStability(EngineBase* engine) {
        try {
            engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
            engine->reset();
            
            auto params = createDefaultParams(engine);
            engine->updateParameters(params);
            
            // Test with various signal types
            std::vector<juce::AudioBuffer<float>> testSignals;
            testSignals.push_back(TestSignalGenerator::generateSilence(0.1f));
            testSignals.push_back(TestSignalGenerator::generateSineWave(20.0f, 0.1f));
            testSignals.push_back(TestSignalGenerator::generateSineWave(20000.0f, 0.1f));
            testSignals.push_back(TestSignalGenerator::generateWhiteNoise(0.1f));
            testSignals.push_back(TestSignalGenerator::generateImpulse());
            
            for (auto& signal : testSignals) {
                auto testBuffer = signal;
                engine->process(testBuffer);
                
                if (AudioAnalysis::hasNaNOrInf(testBuffer)) {
                    return TestResult("Stability", false, "NaN/Inf detected in output");
                }
                
                float peak = AudioAnalysis::calculatePeak(testBuffer);
                if (peak > 10.0f) { // Excessive gain
                    return TestResult("Stability", false, 
                                    "Excessive output level: " + std::to_string(peak));
                }
            }
            
            // Test multiple resets
            for (int i = 0; i < 5; ++i) {
                engine->reset();
                auto testBuffer = TestSignalGenerator::generateSineWave(1000.0f, 0.05f);
                engine->process(testBuffer);
                
                if (AudioAnalysis::hasNaNOrInf(testBuffer)) {
                    return TestResult("Stability", false, "NaN/Inf after reset #" + std::to_string(i + 1));
                }
            }
            
            return TestResult("Stability", true, "All stability tests passed");
            
        } catch (...) {
            return TestResult("Stability", false, "Exception during stability test");
        }
    }
    
    // Test gain staging
    TestResult testGainStaging(EngineBase* engine) {
        try {
            engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
            engine->reset();
            
            auto params = createDefaultParams(engine);
            engine->updateParameters(params);
            
            // Test with various input levels
            std::vector<float> testLevels = {0.001f, 0.1f, 0.5f, 0.9f};
            bool gainStagingOK = true;
            float maxGainChange = 0.0f;
            
            for (float level : testLevels) {
                auto inputBuffer = TestSignalGenerator::generateSineWave(1000.0f, 0.1f);
                
                // Scale input to test level
                for (int ch = 0; ch < inputBuffer.getNumChannels(); ++ch) {
                    auto* data = inputBuffer.getWritePointer(ch);
                    for (int i = 0; i < inputBuffer.getNumSamples(); ++i) {
                        data[i] *= level;
                    }
                }
                
                auto outputBuffer = inputBuffer;
                engine->process(outputBuffer);
                
                float inputRMS = AudioAnalysis::calculateRMS(inputBuffer);
                float outputRMS = AudioAnalysis::calculateRMS(outputBuffer);
                float outputPeak = AudioAnalysis::calculatePeak(outputBuffer);
                
                // Check for reasonable gain
                if (outputPeak > 2.0f) { // More than 6dB boost is suspicious
                    gainStagingOK = false;
                }
                
                if (inputRMS > 0.0f && outputRMS > 0.0f) {
                    float gainDb = AudioAnalysis::linearToDb(outputRMS / inputRMS);
                    maxGainChange = std::max(maxGainChange, std::abs(gainDb));
                }
            }
            
            std::ostringstream detail;
            detail << "Max gain change: " << std::fixed << std::setprecision(2) << maxGainChange << " dB";
            
            return TestResult("Gain Staging", gainStagingOK, detail.str(), maxGainChange);
            
        } catch (...) {
            return TestResult("Gain Staging", false, "Exception during gain test");
        }
    }
    
public:
    EngineTestReport testEngine(int engineID) {
        EngineTestReport report;
        report.engineID = engineID;
        report.overallPassed = true;
        
        // Create engine instance
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            report.engineName = "Unknown Engine " + std::to_string(engineID);
            report.addResult(TestResult("Engine Creation", false, "Failed to create engine"));
            report.summary = "Engine creation failed";
            return report;
        }
        
        report.engineName = engine->getName().toStdString();
        
        // Measure processing time
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Run all tests
        report.addResult(testAudioProcessing(engine.get()));
        report.addResult(testMixParameter(engine.get()));
        report.addResult(testParameterEffects(engine.get()));
        report.addResult(testStability(engine.get()));
        report.addResult(testGainStaging(engine.get()));
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        report.processingTimeMs = duration.count() / 1000.0f;
        
        // Generate summary
        std::ostringstream summary;
        summary << "Passed: " << report.getPassedCount() 
                << "/" << report.results.size() 
                << " (" << std::fixed << std::setprecision(1) 
                << (report.getPassedCount() * 100.0f / report.results.size()) << "%)";
        report.summary = summary.str();
        
        return report;
    }
    
    void runAllTests() {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "CHIMERA DSP ENGINE COMPREHENSIVE TEST SUITE" << std::endl;
        std::cout << "Testing all " << ENGINE_COUNT << " engines..." << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        allReports.clear();
        int enginesTested = 0;
        int enginesPassed = 0;
        
        for (int engineID = 0; engineID < ENGINE_COUNT; ++engineID) {
            // Skip ENGINE_NONE (0) if it's just a passthrough
            if (engineID == ENGINE_NONE) continue;
            
            std::cout << "\n[" << std::setw(2) << (enginesTested + 1) 
                     << "/" << (ENGINE_COUNT - 1) << "] Testing Engine " 
                     << std::setw(2) << engineID << ": " << std::flush;
            
            auto report = testEngine(engineID);
            allReports.push_back(report);
            enginesTested++;
            
            if (report.overallPassed) {
                enginesPassed++;
                std::cout << "✓ PASS ";
            } else {
                std::cout << "✗ FAIL ";
            }
            
            std::cout << std::left << std::setw(25) << report.engineName 
                     << " (" << report.summary << ")" << std::endl;
            
            // Show failed tests
            if (!report.overallPassed) {
                for (const auto& result : report.results) {
                    if (!result.passed) {
                        std::cout << "    ✗ " << result.testName << ": " << result.details << std::endl;
                    }
                }
            }
        }
        
        // Print summary
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "TEST RESULTS SUMMARY" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "Total Engines Tested: " << enginesTested << std::endl;
        std::cout << "Engines Passed: " << enginesPassed << std::endl;
        std::cout << "Engines Failed: " << (enginesTested - enginesPassed) << std::endl;
        std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) 
                 << (100.0f * enginesPassed / enginesTested) << "%" << std::endl;
        
        // Calculate average processing time
        float totalTime = 0.0f;
        for (const auto& report : allReports) {
            totalTime += report.processingTimeMs;
        }
        float avgTime = totalTime / allReports.size();
        std::cout << "Average Test Time: " << std::fixed << std::setprecision(2) 
                 << avgTime << " ms per engine" << std::endl;
        
        // Show failed engines
        if (enginesPassed < enginesTested) {
            std::cout << "\nFAILED ENGINES:" << std::endl;
            std::cout << std::string(40, '-') << std::endl;
            for (const auto& report : allReports) {
                if (!report.overallPassed) {
                    std::cout << "Engine " << report.engineID << " (" << report.engineName << ")" << std::endl;
                    for (const auto& result : report.results) {
                        if (!result.passed) {
                            std::cout << "  ✗ " << result.testName << ": " << result.details << std::endl;
                        }
                    }
                    std::cout << std::endl;
                }
            }
        }
        
        std::cout << std::string(80, '=') << std::endl;
        
        // Exit code
        if (enginesPassed < enginesTested) {
            std::cout << "Some engines failed tests. Check output above for details." << std::endl;
        } else {
            std::cout << "All engines passed their tests successfully!" << std::endl;
        }
    }
    
    void saveDetailedReport(const std::string& filename) {
        std::ofstream file(filename);
        if (!file) return;
        
        file << "Chimera DSP Engine Test Report\n";
        file << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";
        
        for (const auto& report : allReports) {
            file << "Engine " << report.engineID << ": " << report.engineName << "\n";
            file << "Overall Result: " << (report.overallPassed ? "PASS" : "FAIL") << "\n";
            file << "Processing Time: " << report.processingTimeMs << " ms\n";
            file << "Tests:\n";
            
            for (const auto& result : report.results) {
                file << "  " << result.testName << ": " 
                     << (result.passed ? "PASS" : "FAIL");
                if (!result.details.empty()) {
                    file << " - " << result.details;
                }
                file << "\n";
            }
            file << "\n";
        }
    }
};

// Main function for standalone execution
int main(int argc, char* argv[]) {
    try {
        std::cout << "Chimera DSP Engine Test Suite v1.0" << std::endl;
        std::cout << "Standalone testing framework for all 57 engines" << std::endl;
        
        EngineTestRunner runner;
        runner.runAllTests();
        
        // Save detailed report
        runner.saveDetailedReport("chimera_engine_test_report.txt");
        std::cout << "\nDetailed report saved to: chimera_engine_test_report.txt" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}

// JUCE Application wrapper for compilation compatibility
class TestApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Chimera Engine Tester"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    
    void initialise(const juce::String&) override {
        main(0, nullptr);
        quit();
    }
    
    void shutdown() override {}
};

START_JUCE_APPLICATION(TestApplication)
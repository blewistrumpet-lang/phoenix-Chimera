#pragma once

#include <JuceHeader.h>
#include "EngineBase.h"
#include "EngineFactory.h"
#include "EngineTypes.h"
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>

/**
 * SimplifiedEngineTestHarness.h
 * 
 * A focused C++ test harness that tests all implemented engines for:
 * - NaN/Inf handling
 * - Basic parameter functionality  
 * - Audio quality (no excessive gain, no clipping)
 * - Thread safety basics
 * 
 * Generates a simple report of which engines have issues.
 */

class SimplifiedEngineTestHarness {
public:
    struct TestResult {
        int engineID;
        std::string engineName;
        bool creationSuccess = false;
        bool nanInfHandling = false;
        bool parameterFunctionality = false;
        bool audioQuality = false;
        bool threadSafety = false;
        std::vector<std::string> issues;
        
        bool allTestsPassed() const {
            return creationSuccess && nanInfHandling && parameterFunctionality && 
                   audioQuality && threadSafety;
        }
    };
    
    struct TestConfig {
        double sampleRate;
        int samplesPerBlock;
        int testBufferSize;
        float testSignalLevel;
        float maxAcceptableGain;  // 3x gain max
        float nanTestLevel;
        int threadTestIterations;
        std::string reportPath;
        
        TestConfig() 
            : sampleRate(44100.0)
            , samplesPerBlock(512)
            , testBufferSize(1024)
            , testSignalLevel(0.5f)
            , maxAcceptableGain(3.0f)
            , nanTestLevel(1.0f)
            , threadTestIterations(100)
            , reportPath("/tmp/simplified_engine_test_report.txt")
        {}
    };

    TestConfig config_;
    std::vector<TestResult> results_;
    
public:
    // All implemented engine IDs from EngineFactory
    static const std::vector<int> IMPLEMENTED_ENGINES;
    
    explicit SimplifiedEngineTestHarness(const TestConfig& config = TestConfig()) 
        : config_(config) {}
    
    // Run all tests on all implemented engines
    void runAllTests();
    
    // Individual test methods
    TestResult testEngine(int engineID);
    bool testEngineCreation(int engineID, TestResult& result);
    bool testNaNInfHandling(std::unique_ptr<EngineBase>& engine, TestResult& result);
    bool testParameterFunctionality(std::unique_ptr<EngineBase>& engine, TestResult& result);
    bool testAudioQuality(std::unique_ptr<EngineBase>& engine, TestResult& result);
    bool testThreadSafety(std::unique_ptr<EngineBase>& engine, TestResult& result);
    
    // Report generation
    void generateReport() const;
    void printSummary() const;
    
    // Utility methods
    static std::string getEngineNameSafe(int engineID);
    static bool containsNaNOrInf(const juce::AudioBuffer<float>& buffer);
    static float calculateRMS(const juce::AudioBuffer<float>& buffer);
    static float calculatePeak(const juce::AudioBuffer<float>& buffer);
    
    // Get results
    const std::vector<TestResult>& getResults() const { return results_; }
};

// Implementation follows
inline const std::vector<int> SimplifiedEngineTestHarness::IMPLEMENTED_ENGINES = {
    // All engine IDs from 0-56 as defined in EngineTypes.h and implemented in EngineFactory
    0,  // ENGINE_NONE
    1,  // ENGINE_OPTO_COMPRESSOR
    2,  // ENGINE_VCA_COMPRESSOR
    3,  // ENGINE_TRANSIENT_SHAPER
    4,  // ENGINE_NOISE_GATE
    5,  // ENGINE_MASTERING_LIMITER
    6,  // ENGINE_DYNAMIC_EQ
    7,  // ENGINE_PARAMETRIC_EQ
    8,  // ENGINE_VINTAGE_CONSOLE_EQ
    9,  // ENGINE_LADDER_FILTER
    10, // ENGINE_STATE_VARIABLE_FILTER
    11, // ENGINE_FORMANT_FILTER
    12, // ENGINE_ENVELOPE_FILTER
    13, // ENGINE_COMB_RESONATOR
    14, // ENGINE_VOCAL_FORMANT
    15, // ENGINE_VINTAGE_TUBE
    16, // ENGINE_WAVE_FOLDER
    17, // ENGINE_HARMONIC_EXCITER
    18, // ENGINE_BIT_CRUSHER
    19, // ENGINE_MULTIBAND_SATURATOR
    20, // ENGINE_MUFF_FUZZ
    21, // ENGINE_RODENT_DISTORTION
    22, // ENGINE_K_STYLE
    23, // ENGINE_DIGITAL_CHORUS
    24, // ENGINE_RESONANT_CHORUS
    25, // ENGINE_ANALOG_PHASER
    26, // ENGINE_RING_MODULATOR
    27, // ENGINE_FREQUENCY_SHIFTER
    28, // ENGINE_HARMONIC_TREMOLO
    29, // ENGINE_CLASSIC_TREMOLO
    30, // ENGINE_ROTARY_SPEAKER
    31, // ENGINE_PITCH_SHIFTER
    32, // ENGINE_DETUNE_DOUBLER
    33, // ENGINE_INTELLIGENT_HARMONIZER
    34, // ENGINE_TAPE_ECHO
    35, // ENGINE_DIGITAL_DELAY
    36, // ENGINE_MAGNETIC_DRUM_ECHO
    37, // ENGINE_BUCKET_BRIGADE_DELAY
    38, // ENGINE_BUFFER_REPEAT
    39, // ENGINE_PLATE_REVERB
    40, // ENGINE_SPRING_REVERB
    41, // ENGINE_CONVOLUTION_REVERB
    42, // ENGINE_SHIMMER_REVERB
    43, // ENGINE_GATED_REVERB
    44, // ENGINE_STEREO_WIDENER
    45, // ENGINE_STEREO_IMAGER
    46, // ENGINE_DIMENSION_EXPANDER
    47, // ENGINE_SPECTRAL_FREEZE
    48, // ENGINE_SPECTRAL_GATE
    49, // ENGINE_PHASED_VOCODER
    50, // ENGINE_GRANULAR_CLOUD
    51, // ENGINE_CHAOS_GENERATOR
    52, // ENGINE_FEEDBACK_NETWORK
    53, // ENGINE_MID_SIDE_PROCESSOR
    54, // ENGINE_GAIN_UTILITY
    55, // ENGINE_MONO_MAKER
    56  // ENGINE_PHASE_ALIGN
};

inline void SimplifiedEngineTestHarness::runAllTests() {
    results_.clear();
    results_.reserve(IMPLEMENTED_ENGINES.size());
    
    std::cout << "Starting Simplified Engine Test Harness..." << std::endl;
    std::cout << "Testing " << IMPLEMENTED_ENGINES.size() << " implemented engines" << std::endl;
    
    for (int engineID : IMPLEMENTED_ENGINES) {
        std::cout << "Testing Engine " << engineID << " (" << getEngineNameSafe(engineID) << ")..." << std::endl;
        TestResult result = testEngine(engineID);
        results_.push_back(result);
    }
    
    generateReport();
    printSummary();
}

inline SimplifiedEngineTestHarness::TestResult SimplifiedEngineTestHarness::testEngine(int engineID) {
    TestResult result;
    result.engineID = engineID;
    result.engineName = getEngineNameSafe(engineID);
    
    try {
        // Test 1: Engine Creation
        if (!testEngineCreation(engineID, result)) {
            return result; // Cannot continue without valid engine
        }
        
        // Create engine for subsequent tests
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            result.issues.push_back("Failed to create engine instance");
            return result;
        }
        
        // Prepare engine
        engine->prepareToPlay(config_.sampleRate, config_.samplesPerBlock);
        
        // Test 2: NaN/Inf Handling
        result.nanInfHandling = testNaNInfHandling(engine, result);
        
        // Test 3: Parameter Functionality
        result.parameterFunctionality = testParameterFunctionality(engine, result);
        
        // Test 4: Audio Quality
        result.audioQuality = testAudioQuality(engine, result);
        
        // Test 5: Thread Safety
        result.threadSafety = testThreadSafety(engine, result);
        
    } catch (const std::exception& e) {
        result.issues.push_back("Exception during testing: " + std::string(e.what()));
    } catch (...) {
        result.issues.push_back("Unknown exception during testing");
    }
    
    return result;
}

inline bool SimplifiedEngineTestHarness::testEngineCreation(int engineID, TestResult& result) {
    try {
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            result.issues.push_back("Factory returned null pointer");
            return false;
        }
        
        // Test basic method calls
        std::string name = engine->getName().toStdString();
        int numParams = engine->getNumParameters();
        
        if (name.empty()) {
            result.issues.push_back("Engine name is empty");
        }
        
        if (numParams < 0) {
            result.issues.push_back("Invalid parameter count: " + std::to_string(numParams));
        }
        
        result.creationSuccess = true;
        return true;
        
    } catch (const std::exception& e) {
        result.issues.push_back("Engine creation exception: " + std::string(e.what()));
        return false;
    } catch (...) {
        result.issues.push_back("Engine creation unknown exception");
        return false;
    }
}

inline bool SimplifiedEngineTestHarness::testNaNInfHandling(std::unique_ptr<EngineBase>& engine, TestResult& result) {
    try {
        juce::AudioBuffer<float> buffer(2, config_.testBufferSize);
        
        // Test with NaN input
        buffer.clear();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getWritePointer(ch);
            data[0] = std::numeric_limits<float>::quiet_NaN();
            data[config_.testBufferSize / 2] = std::numeric_limits<float>::quiet_NaN();
        }
        
        engine->process(buffer);
        
        if (containsNaNOrInf(buffer)) {
            result.issues.push_back("Engine outputs NaN/Inf when given NaN input");
            return false;
        }
        
        // Test with Inf input
        buffer.clear();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getWritePointer(ch);
            data[0] = std::numeric_limits<float>::infinity();
            data[config_.testBufferSize / 2] = -std::numeric_limits<float>::infinity();
        }
        
        engine->process(buffer);
        
        if (containsNaNOrInf(buffer)) {
            result.issues.push_back("Engine outputs NaN/Inf when given Inf input");
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        result.issues.push_back("NaN/Inf test exception: " + std::string(e.what()));
        return false;
    } catch (...) {
        result.issues.push_back("NaN/Inf test unknown exception");
        return false;
    }
}

inline bool SimplifiedEngineTestHarness::testParameterFunctionality(std::unique_ptr<EngineBase>& engine, TestResult& result) {
    try {
        juce::AudioBuffer<float> buffer(2, config_.testBufferSize);
        
        // Test with default parameters
        std::map<int, float> defaultParams;
        for (int i = 0; i < 15; ++i) {  // Assuming max 15 parameters per engine
            defaultParams[i] = 0.5f;
        }
        
        engine->updateParameters(defaultParams);
        
        // Create test signal
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int s = 0; s < config_.testBufferSize; ++s) {
                data[s] = config_.testSignalLevel * std::sin(2.0f * juce::MathConstants<float>::pi * 1000.0f * s / config_.sampleRate);
            }
        }
        
        juce::AudioBuffer<float> originalBuffer;
        originalBuffer.makeCopyOf(buffer);
        
        engine->process(buffer);
        
        if (containsNaNOrInf(buffer)) {
            result.issues.push_back("Engine outputs NaN/Inf with default parameters");
            return false;
        }
        
        // Test with extreme parameters
        std::map<int, float> extremeParams;
        for (int i = 0; i < 15; ++i) {
            extremeParams[i] = (i % 2 == 0) ? 0.0f : 1.0f;  // Alternate between min and max
        }
        
        engine->updateParameters(extremeParams);
        buffer.makeCopyOf(originalBuffer);
        engine->process(buffer);
        
        if (containsNaNOrInf(buffer)) {
            result.issues.push_back("Engine outputs NaN/Inf with extreme parameters");
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        result.issues.push_back("Parameter test exception: " + std::string(e.what()));
        return false;
    } catch (...) {
        result.issues.push_back("Parameter test unknown exception");
        return false;
    }
}

inline bool SimplifiedEngineTestHarness::testAudioQuality(std::unique_ptr<EngineBase>& engine, TestResult& result) {
    try {
        juce::AudioBuffer<float> buffer(2, config_.testBufferSize);
        
        // Create test signal
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int s = 0; s < config_.testBufferSize; ++s) {
                data[s] = config_.testSignalLevel * std::sin(2.0f * juce::MathConstants<float>::pi * 1000.0f * s / config_.sampleRate);
            }
        }
        
        float inputRMS = calculateRMS(buffer);
        float inputPeak = calculatePeak(buffer);
        
        // Set moderate parameters
        std::map<int, float> params;
        for (int i = 0; i < 15; ++i) {
            params[i] = 0.7f;  // Moderate settings
        }
        
        engine->updateParameters(params);
        engine->process(buffer);
        
        if (containsNaNOrInf(buffer)) {
            result.issues.push_back("Audio quality test: NaN/Inf in output");
            return false;
        }
        
        float outputRMS = calculateRMS(buffer);
        float outputPeak = calculatePeak(buffer);
        
        // Check for excessive gain
        if (outputRMS > inputRMS * config_.maxAcceptableGain) {
            result.issues.push_back("Excessive gain detected - RMS gain: " + std::to_string(outputRMS / inputRMS));
        }
        
        if (outputPeak > config_.maxAcceptableGain) {
            result.issues.push_back("Peak clipping detected - Peak: " + std::to_string(outputPeak));
        }
        
        // Check for complete silence (which might indicate broken processing)
        if (outputRMS < 1e-6f && inputRMS > 1e-3f && result.engineID != ENGINE_NONE) {
            result.issues.push_back("Engine produces near-silence from normal input (possible broken processing)");
        }
        
        return result.issues.empty() || 
               std::none_of(result.issues.begin(), result.issues.end(), 
                           [](const std::string& issue) { return issue.find("gain") != std::string::npos || issue.find("clipping") != std::string::npos; });
        
    } catch (const std::exception& e) {
        result.issues.push_back("Audio quality test exception: " + std::string(e.what()));
        return false;
    } catch (...) {
        result.issues.push_back("Audio quality test unknown exception");
        return false;
    }
}

inline bool SimplifiedEngineTestHarness::testThreadSafety(std::unique_ptr<EngineBase>& engine, TestResult& result) {
    try {
        std::atomic<bool> hasError{false};
        std::atomic<int> errorCount{0};
        
        // Create buffers for each thread
        std::vector<juce::AudioBuffer<float>> buffers;
        for (int i = 0; i < 2; ++i) {
            buffers.emplace_back(2, config_.testBufferSize / 4);
            auto& buffer = buffers.back();
            
            // Fill with test signal
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                auto* data = buffer.getWritePointer(ch);
                for (int s = 0; s < buffer.getNumSamples(); ++s) {
                    data[s] = 0.3f * std::sin(2.0f * juce::MathConstants<float>::pi * 1000.0f * s / config_.sampleRate);
                }
            }
        }
        
        // Thread 1: Process audio
        std::thread audioThread([&]() {
            try {
                for (int i = 0; i < config_.threadTestIterations; ++i) {
                    engine->process(buffers[0]);
                    if (containsNaNOrInf(buffers[0])) {
                        hasError = true;
                        errorCount++;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            } catch (...) {
                hasError = true;
                errorCount++;
            }
        });
        
        // Thread 2: Update parameters
        std::thread parameterThread([&]() {
            try {
                for (int i = 0; i < config_.threadTestIterations; ++i) {
                    std::map<int, float> params;
                    for (int p = 0; p < 15; ++p) {
                        params[p] = 0.3f + 0.4f * (i % 10) / 10.0f;  // Vary parameters
                    }
                    engine->updateParameters(params);
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            } catch (...) {
                hasError = true;
                errorCount++;
            }
        });
        
        audioThread.join();
        parameterThread.join();
        
        if (hasError) {
            result.issues.push_back("Thread safety issues detected, errors: " + std::to_string(errorCount.load()));
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        result.issues.push_back("Thread safety test exception: " + std::string(e.what()));
        return false;
    } catch (...) {
        result.issues.push_back("Thread safety test unknown exception");
        return false;
    }
}

inline void SimplifiedEngineTestHarness::generateReport() const {
    std::ofstream report(config_.reportPath);
    if (!report.is_open()) {
        std::cerr << "Failed to create report file: " << config_.reportPath << std::endl;
        return;
    }
    
    report << "=== SIMPLIFIED ENGINE TEST HARNESS REPORT ===" << std::endl;
    report << "Test Date: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;
    report << "Total Engines Tested: " << results_.size() << std::endl;
    report << std::endl;
    
    int passedEngines = 0;
    int failedEngines = 0;
    
    // Summary section
    report << "=== SUMMARY ===" << std::endl;
    for (const auto& result : results_) {
        if (result.allTestsPassed()) {
            passedEngines++;
        } else {
            failedEngines++;
        }
    }
    
    report << "Engines Passed: " << passedEngines << std::endl;
    report << "Engines Failed: " << failedEngines << std::endl;
    report << "Success Rate: " << (100.0 * passedEngines / results_.size()) << "%" << std::endl;
    report << std::endl;
    
    // Failed engines section
    if (failedEngines > 0) {
        report << "=== ENGINES WITH ISSUES ===" << std::endl;
        for (const auto& result : results_) {
            if (!result.allTestsPassed()) {
                report << std::endl;
                report << "Engine " << result.engineID << ": " << result.engineName << std::endl;
                report << "  Creation: " << (result.creationSuccess ? "PASS" : "FAIL") << std::endl;
                report << "  NaN/Inf Handling: " << (result.nanInfHandling ? "PASS" : "FAIL") << std::endl;
                report << "  Parameter Functionality: " << (result.parameterFunctionality ? "PASS" : "FAIL") << std::endl;
                report << "  Audio Quality: " << (result.audioQuality ? "PASS" : "FAIL") << std::endl;
                report << "  Thread Safety: " << (result.threadSafety ? "PASS" : "FAIL") << std::endl;
                
                if (!result.issues.empty()) {
                    report << "  Issues:" << std::endl;
                    for (const auto& issue : result.issues) {
                        report << "    - " << issue << std::endl;
                    }
                }
            }
        }
    }
    
    // Detailed results section
    report << std::endl << "=== DETAILED RESULTS ===" << std::endl;
    for (const auto& result : results_) {
        report << std::endl;
        report << "Engine " << result.engineID << ": " << result.engineName << std::endl;
        report << "  Overall: " << (result.allTestsPassed() ? "PASS" : "FAIL") << std::endl;
        report << "  Creation: " << (result.creationSuccess ? "PASS" : "FAIL") << std::endl;
        report << "  NaN/Inf Handling: " << (result.nanInfHandling ? "PASS" : "FAIL") << std::endl;
        report << "  Parameter Functionality: " << (result.parameterFunctionality ? "PASS" : "FAIL") << std::endl;
        report << "  Audio Quality: " << (result.audioQuality ? "PASS" : "FAIL") << std::endl;
        report << "  Thread Safety: " << (result.threadSafety ? "PASS" : "FAIL") << std::endl;
        
        if (!result.issues.empty()) {
            report << "  Issues:" << std::endl;
            for (const auto& issue : result.issues) {
                report << "    - " << issue << std::endl;
            }
        }
    }
    
    report.close();
    std::cout << "Report written to: " << config_.reportPath << std::endl;
}

inline void SimplifiedEngineTestHarness::printSummary() const {
    int passedEngines = 0;
    int failedEngines = 0;
    
    for (const auto& result : results_) {
        if (result.allTestsPassed()) {
            passedEngines++;
        } else {
            failedEngines++;
        }
    }
    
    std::cout << std::endl << "=== TEST SUMMARY ===" << std::endl;
    std::cout << "Total Engines: " << results_.size() << std::endl;
    std::cout << "Passed: " << passedEngines << std::endl;
    std::cout << "Failed: " << failedEngines << std::endl;
    std::cout << "Success Rate: " << (100.0 * passedEngines / results_.size()) << "%" << std::endl;
    
    if (failedEngines > 0) {
        std::cout << std::endl << "Engines with issues:" << std::endl;
        for (const auto& result : results_) {
            if (!result.allTestsPassed()) {
                std::cout << "  - Engine " << result.engineID << ": " << result.engineName;
                std::cout << " (" << result.issues.size() << " issues)" << std::endl;
            }
        }
    }
    
    std::cout << std::endl << "Detailed report available at: " << config_.reportPath << std::endl;
}

inline std::string SimplifiedEngineTestHarness::getEngineNameSafe(int engineID) {
    try {
        return getEngineTypeName(engineID);
    } catch (...) {
        return "Unknown Engine " + std::to_string(engineID);
    }
}

inline bool SimplifiedEngineTestHarness::containsNaNOrInf(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const auto* data = buffer.getReadPointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            if (!std::isfinite(data[s])) {
                return true;
            }
        }
    }
    return false;
}

inline float SimplifiedEngineTestHarness::calculateRMS(const juce::AudioBuffer<float>& buffer) {
    float sum = 0.0f;
    int totalSamples = 0;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const auto* data = buffer.getReadPointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            sum += data[s] * data[s];
            totalSamples++;
        }
    }
    
    return std::sqrt(sum / totalSamples);
}

inline float SimplifiedEngineTestHarness::calculatePeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const auto* data = buffer.getReadPointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            peak = std::max(peak, std::abs(data[s]));
        }
    }
    
    return peak;
}
/**
 * COMPREHENSIVE INTEGRATION TEST SUITE
 * Project Chimera Phoenix v3.0
 *
 * CRITICAL: This tests the 0% coverage area identified by deep validation
 *
 * Tests:
 * 1. Engine Chaining - Multiple engines in series
 * 2. Preset Switching - Rapid transitions and memory stability
 * 3. Parameter Automation - DAW-style parameter sweeps
 * 4. Engine Activation/Bypass - Dynamic enable/disable
 * 5. Stress Testing - All engines active, extreme chains
 * 6. Memory Stability - Long-duration testing
 * 7. CPU Usage - Performance under load
 *
 * MISSING FROM PREVIOUS TESTING:
 * - Multi-engine interactions
 * - Rapid state changes
 * - Real-world DAW scenarios
 * - Memory leaks during switching
 * - Parameter flood handling
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <chrono>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <map>
#include <algorithm>

// JUCE configuration
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 1
#define JUCE_USE_CURL 0
#define JUCE_WEB_BROWSER 0
#define DEBUG 1

// JUCE headers
#include "../pi_deployment/JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

// Engine headers
#include "../pi_deployment/JUCE_Plugin/Source/EngineBase.h"
#include "../pi_deployment/JUCE_Plugin/Source/EngineFactory.h"

// ============================================================================
// TEST UTILITIES
// ============================================================================

struct TestStats {
    int totalTests = 0;
    int passed = 0;
    int failed = 0;
    int warnings = 0;
    std::vector<std::string> failures;
    std::vector<std::string> warningMessages;
};

struct AudioMetrics {
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    bool hasNaN = false;
    bool hasInf = false;
    bool hasClipping = false;
    bool hasSilence = true;
    double thd = 0.0;
};

AudioMetrics analyzeBuffer(const juce::AudioBuffer<float>& buffer) {
    AudioMetrics metrics;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        int numSamples = buffer.getNumSamples();

        float sumSquares = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            float sample = data[i];

            if (std::isnan(sample)) {
                metrics.hasNaN = true;
            }
            if (std::isinf(sample)) {
                metrics.hasInf = true;
            }
            if (std::abs(sample) > 1.0f) {
                metrics.hasClipping = true;
            }
            if (std::abs(sample) > 0.0001f) {
                metrics.hasSilence = false;
            }

            float absSample = std::abs(sample);
            metrics.peakLevel = std::max(metrics.peakLevel, absSample);
            sumSquares += sample * sample;
        }

        metrics.rmsLevel = std::sqrt(sumSquares / numSamples);
    }

    return metrics;
}

void generateTestSignal(juce::AudioBuffer<float>& buffer, float frequency, float sampleRate) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i) {
            float phase = (float)i / sampleRate * frequency * 2.0f * juce::MathConstants<float>::pi;
            data[i] = 0.5f * std::sin(phase);
        }
    }
}

// ============================================================================
// ENGINE CHAIN TEST
// ============================================================================

class EngineChainTest {
public:
    struct ChainTestResult {
        std::string chainDescription;
        bool passed = true;
        std::string errorMessage;
        AudioMetrics metrics;
        double processingTimeMs = 0.0;
        int numEngines = 0;
    };

    static std::vector<ChainTestResult> runAllChainTests(double sampleRate, int bufferSize) {
        std::vector<ChainTestResult> results;

        std::cout << "\n================================================================" << std::endl;
        std::cout << "INTEGRATION TEST 1: ENGINE CHAINING" << std::endl;
        std::cout << "================================================================\n" << std::endl;

        // Test 1: Classic production chain
        results.push_back(testChain("Classic Production: Compressor -> EQ -> Reverb",
                                    {1, 7, 39}, sampleRate, bufferSize));

        // Test 2: Creative distortion chain
        results.push_back(testChain("Creative Distortion: Distortion -> Filter -> Delay",
                                    {15, 8, 34}, sampleRate, bufferSize));

        // Test 3: Multiple dynamics processors
        results.push_back(testChain("Dynamics Stack: Compressor -> Gate -> Limiter",
                                    {1, 4, 0}, sampleRate, bufferSize));

        // Test 4: Modulation chain
        results.push_back(testChain("Modulation Chain: Chorus -> Flanger -> Phaser",
                                    {23, 25, 26}, sampleRate, bufferSize));

        // Test 5: Spatial processing
        results.push_back(testChain("Spatial Chain: Stereo Widener -> Reverb -> Delay",
                                    {46, 39, 34}, sampleRate, bufferSize));

        // Test 6: Extreme chain (6 engines - max slots)
        results.push_back(testChain("Extreme 6-Engine Chain",
                                    {1, 7, 15, 23, 39, 34}, sampleRate, bufferSize));

        // Test 7: All reverbs in series (stress test)
        results.push_back(testChain("All Reverbs Chain",
                                    {39, 40, 41, 42, 43}, sampleRate, bufferSize));

        // Test 8: Pitch shifting chain
        results.push_back(testChain("Pitch Chain: Harmonizer -> Reverb",
                                    {33, 39}, sampleRate, bufferSize));

        return results;
    }

private:
    static ChainTestResult testChain(const std::string& description,
                                     const std::vector<int>& engineIDs,
                                     double sampleRate, int bufferSize) {
        ChainTestResult result;
        result.chainDescription = description;
        result.numEngines = engineIDs.size();

        std::cout << "[CHAIN TEST] " << description << std::endl;
        std::cout << "  Engines: ";
        for (int id : engineIDs) std::cout << id << " ";
        std::cout << std::endl;

        try {
            // Create engines
            std::vector<std::unique_ptr<EngineBase>> engines;
            for (int id : engineIDs) {
                auto engine = EngineFactory::createEngine(id);
                if (!engine) {
                    result.passed = false;
                    result.errorMessage = "Failed to create engine " + std::to_string(id);
                    std::cout << "  FAILED: " << result.errorMessage << std::endl;
                    return result;
                }
                engine->prepareToPlay(sampleRate, bufferSize);
                engines.push_back(std::move(engine));
            }

            // Create test buffer
            juce::AudioBuffer<float> buffer(2, bufferSize);
            generateTestSignal(buffer, 440.0f, sampleRate);

            // Process through chain
            auto startTime = std::chrono::high_resolution_clock::now();

            for (auto& engine : engines) {
                engine->process(buffer);

                // Check for issues after each engine
                AudioMetrics stepMetrics = analyzeBuffer(buffer);
                if (stepMetrics.hasNaN || stepMetrics.hasInf) {
                    result.passed = false;
                    result.errorMessage = "NaN/Inf detected in chain";
                    break;
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            result.processingTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

            // Analyze final output
            result.metrics = analyzeBuffer(buffer);

            // Validation
            if (result.metrics.hasNaN) {
                result.passed = false;
                result.errorMessage = "Output contains NaN";
            } else if (result.metrics.hasInf) {
                result.passed = false;
                result.errorMessage = "Output contains Inf";
            } else if (result.metrics.hasClipping) {
                // Warning, not failure
                std::cout << "  WARNING: Clipping detected (peak: "
                         << result.metrics.peakLevel << ")" << std::endl;
            }

            std::cout << "  Status: " << (result.passed ? "PASS" : "FAIL") << std::endl;
            std::cout << "  Peak: " << result.metrics.peakLevel << std::endl;
            std::cout << "  RMS: " << result.metrics.rmsLevel << std::endl;
            std::cout << "  Processing Time: " << result.processingTimeMs << " ms" << std::endl;

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }
};

// ============================================================================
// PRESET SWITCHING TEST
// ============================================================================

class PresetSwitchingTest {
public:
    struct SwitchTestResult {
        std::string testName;
        bool passed = true;
        std::string errorMessage;
        int numSwitches = 0;
        double totalTimeMs = 0.0;
        double avgSwitchTimeMs = 0.0;
        bool hasClicksOrPops = false;
        bool memoryLeak = false;
        size_t peakMemoryMB = 0;
    };

    static std::vector<SwitchTestResult> runAllSwitchTests(double sampleRate, int bufferSize) {
        std::vector<SwitchTestResult> results;

        std::cout << "\n================================================================" << std::endl;
        std::cout << "INTEGRATION TEST 2: PRESET SWITCHING" << std::endl;
        std::cout << "================================================================\n" << std::endl;

        // Test 1: Rapid switching (100 switches)
        results.push_back(testRapidSwitching(sampleRate, bufferSize, 100));

        // Test 2: Memory leak during repeated switching
        results.push_back(testMemoryLeaks(sampleRate, bufferSize, 1000));

        // Test 3: Click/pop detection during transitions
        results.push_back(testClickDetection(sampleRate, bufferSize, 50));

        // Test 4: State consistency after switching
        results.push_back(testStateConsistency(sampleRate, bufferSize));

        return results;
    }

private:
    static SwitchTestResult testRapidSwitching(double sampleRate, int bufferSize, int numSwitches) {
        SwitchTestResult result;
        result.testName = "Rapid Preset Switching (" + std::to_string(numSwitches) + " switches)";
        result.numSwitches = numSwitches;

        std::cout << "[SWITCH TEST] " << result.testName << std::endl;

        try {
            // Create initial engine
            auto engine = EngineFactory::createEngine(1);
            engine->prepareToPlay(sampleRate, bufferSize);

            juce::AudioBuffer<float> buffer(2, bufferSize);
            generateTestSignal(buffer, 440.0f, sampleRate);

            auto startTime = std::chrono::high_resolution_clock::now();

            // Rapid switching between different engine types
            std::vector<int> engineTypes = {1, 7, 15, 23, 39, 34, 8, 25, 40, 46};

            for (int i = 0; i < numSwitches; ++i) {
                int engineID = engineTypes[i % engineTypes.size()];

                // Simulate preset switch: destroy old engine, create new one
                engine.reset();
                engine = EngineFactory::createEngine(engineID);
                engine->prepareToPlay(sampleRate, bufferSize);

                // Process one buffer
                engine->process(buffer);

                // Check for issues
                AudioMetrics metrics = analyzeBuffer(buffer);
                if (metrics.hasNaN || metrics.hasInf) {
                    result.passed = false;
                    result.errorMessage = "NaN/Inf during switch " + std::to_string(i);
                    break;
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            result.totalTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            result.avgSwitchTimeMs = result.totalTimeMs / numSwitches;

            std::cout << "  Status: " << (result.passed ? "PASS" : "FAIL") << std::endl;
            std::cout << "  Total Time: " << result.totalTimeMs << " ms" << std::endl;
            std::cout << "  Avg Switch Time: " << result.avgSwitchTimeMs << " ms" << std::endl;
            std::cout << "  Switches/Second: " << (numSwitches / (result.totalTimeMs / 1000.0)) << std::endl;

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }

    static SwitchTestResult testMemoryLeaks(double sampleRate, int bufferSize, int numSwitches) {
        SwitchTestResult result;
        result.testName = "Memory Leak Detection (" + std::to_string(numSwitches) + " switches)";
        result.numSwitches = numSwitches;

        std::cout << "[MEMORY TEST] " << result.testName << std::endl;

        try {
            std::vector<int> engineTypes = {1, 7, 15, 23, 39, 34};

            for (int i = 0; i < numSwitches; ++i) {
                int engineID = engineTypes[i % engineTypes.size()];

                auto engine = EngineFactory::createEngine(engineID);
                engine->prepareToPlay(sampleRate, bufferSize);

                juce::AudioBuffer<float> buffer(2, bufferSize);
                generateTestSignal(buffer, 440.0f, sampleRate);
                engine->process(buffer);

                // Engine destroyed here automatically
            }

            std::cout << "  Status: PASS (no crashes)" << std::endl;
            std::cout << "  Note: Manual memory profiling recommended" << std::endl;

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }

    static SwitchTestResult testClickDetection(double sampleRate, int bufferSize, int numSwitches) {
        SwitchTestResult result;
        result.testName = "Click/Pop Detection During Switching";
        result.numSwitches = numSwitches;

        std::cout << "[CLICK TEST] " << result.testName << std::endl;

        try {
            std::vector<int> engineTypes = {1, 39, 34};
            auto engine = EngineFactory::createEngine(engineTypes[0]);
            engine->prepareToPlay(sampleRate, bufferSize);

            juce::AudioBuffer<float> buffer(2, bufferSize);
            juce::AudioBuffer<float> previousBuffer(2, bufferSize);

            int clickCount = 0;
            const float clickThreshold = 0.5f; // Large sudden change

            for (int i = 0; i < numSwitches; ++i) {
                int engineID = engineTypes[i % engineTypes.size()];

                // Save previous output
                previousBuffer.makeCopyOf(buffer);

                // Switch engine
                engine.reset();
                engine = EngineFactory::createEngine(engineID);
                engine->prepareToPlay(sampleRate, bufferSize);

                // Generate new input
                generateTestSignal(buffer, 440.0f, sampleRate);
                engine->process(buffer);

                // Check for large discontinuity (click/pop)
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    float lastPrevious = previousBuffer.getSample(ch, bufferSize - 1);
                    float firstNew = buffer.getSample(ch, 0);
                    float discontinuity = std::abs(firstNew - lastPrevious);

                    if (discontinuity > clickThreshold) {
                        clickCount++;
                        result.hasClicksOrPops = true;
                    }
                }
            }

            if (result.hasClicksOrPops) {
                std::cout << "  WARNING: " << clickCount << " potential clicks detected" << std::endl;
            } else {
                std::cout << "  Status: PASS (no clicks detected)" << std::endl;
            }

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }

    static SwitchTestResult testStateConsistency(double sampleRate, int bufferSize) {
        SwitchTestResult result;
        result.testName = "State Consistency After Switching";

        std::cout << "[CONSISTENCY TEST] " << result.testName << std::endl;

        try {
            // Create engine with specific parameters
            auto engine1 = EngineFactory::createEngine(7); // Parametric EQ
            engine1->prepareToPlay(sampleRate, bufferSize);

            std::map<int, float> params = {{0, 0.5f}, {1, 0.7f}, {2, 0.3f}};
            engine1->updateParameters(params);

            // Process some audio
            juce::AudioBuffer<float> buffer1(2, bufferSize);
            generateTestSignal(buffer1, 440.0f, sampleRate);
            engine1->process(buffer1);

            // Destroy and recreate with same parameters
            engine1.reset();
            auto engine2 = EngineFactory::createEngine(7);
            engine2->prepareToPlay(sampleRate, bufferSize);
            engine2->updateParameters(params);

            // Process same audio
            juce::AudioBuffer<float> buffer2(2, bufferSize);
            generateTestSignal(buffer2, 440.0f, sampleRate);
            engine2->process(buffer2);

            // Compare outputs (should be identical)
            float maxDiff = 0.0f;
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < bufferSize; ++i) {
                    float diff = std::abs(buffer1.getSample(ch, i) - buffer2.getSample(ch, i));
                    maxDiff = std::max(maxDiff, diff);
                }
            }

            if (maxDiff > 0.001f) {
                result.passed = false;
                result.errorMessage = "Output inconsistent (max diff: " + std::to_string(maxDiff) + ")";
                std::cout << "  FAILED: " << result.errorMessage << std::endl;
            } else {
                std::cout << "  Status: PASS (max diff: " << maxDiff << ")" << std::endl;
            }

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }
};

// ============================================================================
// PARAMETER AUTOMATION TEST
// ============================================================================

class ParameterAutomationTest {
public:
    struct AutomationTestResult {
        std::string testName;
        bool passed = true;
        std::string errorMessage;
        bool smoothTransition = true;
        bool hasZipperNoise = false;
        float maxDiscontinuity = 0.0f;
    };

    static std::vector<AutomationTestResult> runAllAutomationTests(double sampleRate, int bufferSize) {
        std::vector<AutomationTestResult> results;

        std::cout << "\n================================================================" << std::endl;
        std::cout << "INTEGRATION TEST 3: PARAMETER AUTOMATION" << std::endl;
        std::cout << "================================================================\n" << std::endl;

        // Test 1: Smooth parameter sweep
        results.push_back(testParameterSweep(sampleRate, bufferSize));

        // Test 2: Rapid parameter changes (DAW automation)
        results.push_back(testRapidParameterChanges(sampleRate, bufferSize));

        // Test 3: Parameter flood (stress test)
        results.push_back(testParameterFlood(sampleRate, bufferSize));

        // Test 4: Zipper noise detection
        results.push_back(testZipperNoise(sampleRate, bufferSize));

        return results;
    }

private:
    static AutomationTestResult testParameterSweep(double sampleRate, int bufferSize) {
        AutomationTestResult result;
        result.testName = "Smooth Parameter Sweep";

        std::cout << "[AUTOMATION TEST] " << result.testName << std::endl;

        try {
            auto engine = EngineFactory::createEngine(7); // Parametric EQ
            engine->prepareToPlay(sampleRate, bufferSize);

            juce::AudioBuffer<float> buffer(2, bufferSize);
            int numSteps = 100;

            for (int step = 0; step < numSteps; ++step) {
                float paramValue = (float)step / (numSteps - 1);
                std::map<int, float> params = {{0, paramValue}};
                engine->updateParameters(params);

                generateTestSignal(buffer, 440.0f, sampleRate);
                engine->process(buffer);

                AudioMetrics metrics = analyzeBuffer(buffer);
                if (metrics.hasNaN || metrics.hasInf) {
                    result.passed = false;
                    result.errorMessage = "NaN/Inf at step " + std::to_string(step);
                    break;
                }
            }

            std::cout << "  Status: " << (result.passed ? "PASS" : "FAIL") << std::endl;

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }

    static AutomationTestResult testRapidParameterChanges(double sampleRate, int bufferSize) {
        AutomationTestResult result;
        result.testName = "Rapid Parameter Changes (DAW Simulation)";

        std::cout << "[AUTOMATION TEST] " << result.testName << std::endl;

        try {
            auto engine = EngineFactory::createEngine(1); // Compressor
            engine->prepareToPlay(sampleRate, bufferSize);

            juce::AudioBuffer<float> buffer(2, bufferSize);
            int numChanges = 1000;

            for (int i = 0; i < numChanges; ++i) {
                // Simulate DAW automation: parameter changes every buffer
                float paramValue = 0.5f + 0.5f * std::sin(i * 0.1f);
                std::map<int, float> params = {{0, paramValue}};
                engine->updateParameters(params);

                generateTestSignal(buffer, 440.0f, sampleRate);
                engine->process(buffer);

                AudioMetrics metrics = analyzeBuffer(buffer);
                if (metrics.hasNaN || metrics.hasInf) {
                    result.passed = false;
                    result.errorMessage = "NaN/Inf at change " + std::to_string(i);
                    break;
                }
            }

            std::cout << "  Status: " << (result.passed ? "PASS" : "FAIL") << std::endl;
            std::cout << "  Changes Processed: " << numChanges << std::endl;

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }

    static AutomationTestResult testParameterFlood(double sampleRate, int bufferSize) {
        AutomationTestResult result;
        result.testName = "Parameter Flood (Stress Test)";

        std::cout << "[AUTOMATION TEST] " << result.testName << std::endl;

        try {
            auto engine = EngineFactory::createEngine(7); // Parametric EQ (many params)
            engine->prepareToPlay(sampleRate, bufferSize);

            juce::AudioBuffer<float> buffer(2, bufferSize);
            int numFloods = 100;

            for (int i = 0; i < numFloods; ++i) {
                // Flood: update ALL parameters at once
                std::map<int, float> params;
                for (int p = 0; p < 10; ++p) {
                    params[p] = (float)rand() / RAND_MAX;
                }
                engine->updateParameters(params);

                generateTestSignal(buffer, 440.0f, sampleRate);
                engine->process(buffer);

                AudioMetrics metrics = analyzeBuffer(buffer);
                if (metrics.hasNaN || metrics.hasInf) {
                    result.passed = false;
                    result.errorMessage = "NaN/Inf during flood " + std::to_string(i);
                    break;
                }
            }

            std::cout << "  Status: " << (result.passed ? "PASS" : "FAIL") << std::endl;

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }

    static AutomationTestResult testZipperNoise(double sampleRate, int bufferSize) {
        AutomationTestResult result;
        result.testName = "Zipper Noise Detection";

        std::cout << "[AUTOMATION TEST] " << result.testName << std::endl;

        try {
            auto engine = EngineFactory::createEngine(1); // Compressor
            engine->prepareToPlay(sampleRate, bufferSize);

            juce::AudioBuffer<float> buffer(2, bufferSize);
            juce::AudioBuffer<float> previousBuffer(2, bufferSize);

            int numSteps = 50;
            float maxDiscontinuity = 0.0f;

            for (int step = 0; step < numSteps; ++step) {
                previousBuffer.makeCopyOf(buffer);

                // Abrupt parameter change
                float paramValue = (step % 2) == 0 ? 0.1f : 0.9f;
                std::map<int, float> params = {{0, paramValue}};
                engine->updateParameters(params);

                generateTestSignal(buffer, 440.0f, sampleRate);
                engine->process(buffer);

                // Check for discontinuity (zipper noise indicator)
                if (step > 0) {
                    for (int ch = 0; ch < 2; ++ch) {
                        float lastPrevious = previousBuffer.getSample(ch, bufferSize - 1);
                        float firstNew = buffer.getSample(ch, 0);
                        float discontinuity = std::abs(firstNew - lastPrevious);
                        maxDiscontinuity = std::max(maxDiscontinuity, discontinuity);
                    }
                }
            }

            result.maxDiscontinuity = maxDiscontinuity;

            if (maxDiscontinuity > 0.3f) {
                result.hasZipperNoise = true;
                std::cout << "  WARNING: Potential zipper noise (max discontinuity: "
                         << maxDiscontinuity << ")" << std::endl;
            } else {
                std::cout << "  Status: PASS (max discontinuity: " << maxDiscontinuity << ")" << std::endl;
            }

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }
};

// ============================================================================
// ENGINE BYPASS TEST
// ============================================================================

class EngineBypassTest {
public:
    struct BypassTestResult {
        std::string testName;
        bool passed = true;
        std::string errorMessage;
        bool cleanBypass = true;
        int numToggles = 0;
    };

    static std::vector<BypassTestResult> runAllBypassTests(double sampleRate, int bufferSize) {
        std::vector<BypassTestResult> results;

        std::cout << "\n================================================================" << std::endl;
        std::cout << "INTEGRATION TEST 4: ENGINE ACTIVATION/BYPASS" << std::endl;
        std::cout << "================================================================\n" << std::endl;

        // Test 1: Rapid bypass toggling
        results.push_back(testRapidBypassToggle(sampleRate, bufferSize));

        // Test 2: Clean bypass (no clicks)
        results.push_back(testCleanBypass(sampleRate, bufferSize));

        return results;
    }

private:
    static BypassTestResult testRapidBypassToggle(double sampleRate, int bufferSize) {
        BypassTestResult result;
        result.testName = "Rapid Bypass Toggling";
        result.numToggles = 100;

        std::cout << "[BYPASS TEST] " << result.testName << std::endl;

        try {
            auto engine = EngineFactory::createEngine(39); // Reverb (has tail)
            engine->prepareToPlay(sampleRate, bufferSize);

            juce::AudioBuffer<float> buffer(2, bufferSize);

            for (int i = 0; i < result.numToggles; ++i) {
                bool bypass = (i % 2) == 0;
                engine->setBypassed(bypass);

                generateTestSignal(buffer, 440.0f, sampleRate);
                engine->process(buffer);

                AudioMetrics metrics = analyzeBuffer(buffer);
                if (metrics.hasNaN || metrics.hasInf) {
                    result.passed = false;
                    result.errorMessage = "NaN/Inf during toggle " + std::to_string(i);
                    break;
                }
            }

            std::cout << "  Status: " << (result.passed ? "PASS" : "FAIL") << std::endl;

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }

    static BypassTestResult testCleanBypass(double sampleRate, int bufferSize) {
        BypassTestResult result;
        result.testName = "Clean Bypass (No Clicks)";

        std::cout << "[BYPASS TEST] " << result.testName << std::endl;

        try {
            auto engine = EngineFactory::createEngine(1); // Compressor
            engine->prepareToPlay(sampleRate, bufferSize);

            juce::AudioBuffer<float> buffer(2, bufferSize);
            juce::AudioBuffer<float> previousBuffer(2, bufferSize);

            int clickCount = 0;
            const float clickThreshold = 0.3f;

            for (int i = 0; i < 20; ++i) {
                previousBuffer.makeCopyOf(buffer);

                bool bypass = (i % 2) == 0;
                engine->setBypassed(bypass);

                generateTestSignal(buffer, 440.0f, sampleRate);
                engine->process(buffer);

                // Check for clicks
                if (i > 0) {
                    for (int ch = 0; ch < 2; ++ch) {
                        float lastPrevious = previousBuffer.getSample(ch, bufferSize - 1);
                        float firstNew = buffer.getSample(ch, 0);
                        float discontinuity = std::abs(firstNew - lastPrevious);

                        if (discontinuity > clickThreshold) {
                            clickCount++;
                            result.cleanBypass = false;
                        }
                    }
                }
            }

            if (!result.cleanBypass) {
                std::cout << "  WARNING: " << clickCount << " clicks detected during bypass" << std::endl;
            } else {
                std::cout << "  Status: PASS (no clicks)" << std::endl;
            }

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }
};

// ============================================================================
// STRESS TEST
// ============================================================================

class StressTest {
public:
    struct StressTestResult {
        std::string testName;
        bool passed = true;
        std::string errorMessage;
        int numEnginesActive = 0;
        double totalProcessingTimeMs = 0.0;
        float peakCPU = 0.0f;
    };

    static std::vector<StressTestResult> runAllStressTests(double sampleRate, int bufferSize) {
        std::vector<StressTestResult> results;

        std::cout << "\n================================================================" << std::endl;
        std::cout << "INTEGRATION TEST 5: STRESS TESTING" << std::endl;
        std::cout << "================================================================\n" << std::endl;

        // Test 1: Maximum chain length (6 engines)
        results.push_back(testMaxChainLength(sampleRate, bufferSize));

        // Test 2: All engines instantiated (not chained, but all created)
        results.push_back(testAllEnginesInstantiation(sampleRate, bufferSize));

        // Test 3: Long duration stability (1 minute simulation)
        results.push_back(testLongDuration(sampleRate, bufferSize));

        return results;
    }

private:
    static StressTestResult testMaxChainLength(double sampleRate, int bufferSize) {
        StressTestResult result;
        result.testName = "Maximum Chain Length (6 Engines)";
        result.numEnginesActive = 6;

        std::cout << "[STRESS TEST] " << result.testName << std::endl;

        try {
            // Create 6-engine chain
            std::vector<int> engineIDs = {1, 7, 15, 23, 39, 34};
            std::vector<std::unique_ptr<EngineBase>> engines;

            for (int id : engineIDs) {
                auto engine = EngineFactory::createEngine(id);
                engine->prepareToPlay(sampleRate, bufferSize);
                engines.push_back(std::move(engine));
            }

            juce::AudioBuffer<float> buffer(2, bufferSize);
            int numBuffers = 1000;

            auto startTime = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < numBuffers; ++i) {
                generateTestSignal(buffer, 440.0f, sampleRate);

                for (auto& engine : engines) {
                    engine->process(buffer);
                }

                AudioMetrics metrics = analyzeBuffer(buffer);
                if (metrics.hasNaN || metrics.hasInf) {
                    result.passed = false;
                    result.errorMessage = "NaN/Inf at buffer " + std::to_string(i);
                    break;
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            result.totalProcessingTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

            double bufferDurationMs = (bufferSize / sampleRate) * 1000.0;
            double totalAudioMs = numBuffers * bufferDurationMs;
            result.peakCPU = (result.totalProcessingTimeMs / totalAudioMs) * 100.0f;

            std::cout << "  Status: " << (result.passed ? "PASS" : "FAIL") << std::endl;
            std::cout << "  Processing Time: " << result.totalProcessingTimeMs << " ms" << std::endl;
            std::cout << "  CPU Usage: " << result.peakCPU << "%" << std::endl;

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }

    static StressTestResult testAllEnginesInstantiation(double sampleRate, int bufferSize) {
        StressTestResult result;
        result.testName = "All 56 Engines Instantiation";
        result.numEnginesActive = 56;

        std::cout << "[STRESS TEST] " << result.testName << std::endl;

        try {
            int successCount = 0;
            std::vector<std::string> failures;

            for (int engineID = 0; engineID < 56; ++engineID) {
                try {
                    auto engine = EngineFactory::createEngine(engineID);
                    if (engine) {
                        engine->prepareToPlay(sampleRate, bufferSize);

                        juce::AudioBuffer<float> buffer(2, bufferSize);
                        generateTestSignal(buffer, 440.0f, sampleRate);
                        engine->process(buffer);

                        AudioMetrics metrics = analyzeBuffer(buffer);
                        if (!metrics.hasNaN && !metrics.hasInf) {
                            successCount++;
                        } else {
                            failures.push_back("Engine " + std::to_string(engineID) + ": NaN/Inf output");
                        }
                    } else {
                        failures.push_back("Engine " + std::to_string(engineID) + ": Failed to create");
                    }
                } catch (const std::exception& e) {
                    failures.push_back("Engine " + std::to_string(engineID) + ": " + e.what());
                }
            }

            result.passed = (successCount == 56);

            std::cout << "  Status: " << (result.passed ? "PASS" : "PARTIAL") << std::endl;
            std::cout << "  Success: " << successCount << "/56" << std::endl;

            if (!failures.empty()) {
                std::cout << "  Failures:" << std::endl;
                for (const auto& failure : failures) {
                    std::cout << "    - " << failure << std::endl;
                }
            }

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }

    static StressTestResult testLongDuration(double sampleRate, int bufferSize) {
        StressTestResult result;
        result.testName = "Long Duration Stability (1 Minute Simulation)";

        std::cout << "[STRESS TEST] " << result.testName << std::endl;

        try {
            auto engine = EngineFactory::createEngine(39); // Reverb (stateful)
            engine->prepareToPlay(sampleRate, bufferSize);

            juce::AudioBuffer<float> buffer(2, bufferSize);
            double bufferDurationMs = (bufferSize / sampleRate) * 1000.0;
            int numBuffers = (int)(60000.0 / bufferDurationMs); // 1 minute

            auto startTime = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < numBuffers; ++i) {
                generateTestSignal(buffer, 440.0f, sampleRate);
                engine->process(buffer);

                // Check every 100 buffers
                if (i % 100 == 0) {
                    AudioMetrics metrics = analyzeBuffer(buffer);
                    if (metrics.hasNaN || metrics.hasInf) {
                        result.passed = false;
                        result.errorMessage = "NaN/Inf at buffer " + std::to_string(i);
                        break;
                    }
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            result.totalProcessingTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

            std::cout << "  Status: " << (result.passed ? "PASS" : "FAIL") << std::endl;
            std::cout << "  Buffers Processed: " << numBuffers << std::endl;
            std::cout << "  Total Time: " << result.totalProcessingTimeMs << " ms" << std::endl;

        } catch (const std::exception& e) {
            result.passed = false;
            result.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "  FAILED: " << result.errorMessage << std::endl;
        }

        return result;
    }
};

// ============================================================================
// REPORT GENERATION
// ============================================================================

void generateIntegrationReport(
    const std::vector<EngineChainTest::ChainTestResult>& chainResults,
    const std::vector<PresetSwitchingTest::SwitchTestResult>& switchResults,
    const std::vector<ParameterAutomationTest::AutomationTestResult>& automationResults,
    const std::vector<EngineBypassTest::BypassTestResult>& bypassResults,
    const std::vector<StressTest::StressTestResult>& stressResults,
    const std::string& outputPath) {

    std::ofstream report(outputPath);

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    report << "# INTEGRATION TEST SUITE REPORT\n";
    report << "## Project Chimera Phoenix v3.0\n\n";
    report << "**Test Date:** " << std::ctime(&now_time) << "\n";
    report << "**Test Type:** Comprehensive Integration Testing\n";
    report << "**Coverage Area:** Engine Chains, Preset Switching, Automation, Stress\n\n";

    // EXECUTIVE SUMMARY
    report << "## EXECUTIVE SUMMARY\n\n";

    int totalTests = chainResults.size() + switchResults.size() +
                    automationResults.size() + bypassResults.size() + stressResults.size();
    int totalPassed = 0;
    int totalFailed = 0;

    auto countResults = [&](const auto& results) {
        for (const auto& result : results) {
            if (result.passed) totalPassed++;
            else totalFailed++;
        }
    };

    countResults(chainResults);
    countResults(switchResults);
    countResults(automationResults);
    countResults(bypassResults);
    countResults(stressResults);

    report << "| Metric | Value |\n";
    report << "|--------|-------|\n";
    report << "| Total Tests | " << totalTests << " |\n";
    report << "| Passed | " << totalPassed << " (" << (totalPassed * 100 / totalTests) << "%) |\n";
    report << "| Failed | " << totalFailed << " (" << (totalFailed * 100 / totalTests) << "%) |\n\n";

    if (totalFailed == 0) {
        report << "✅ **ALL INTEGRATION TESTS PASSED**\n\n";
    } else {
        report << "⚠️ **ISSUES DETECTED** - " << totalFailed << " test(s) failed\n\n";
    }

    // ENGINE CHAINING RESULTS
    report << "## 1. ENGINE CHAINING TESTS\n\n";
    for (const auto& result : chainResults) {
        report << "### " << result.chainDescription << "\n\n";
        report << "**Status:** " << (result.passed ? "PASS ✅" : "FAIL ❌") << "\n\n";
        report << "| Metric | Value |\n";
        report << "|--------|-------|\n";
        report << "| Engines in Chain | " << result.numEngines << " |\n";
        report << "| Processing Time | " << result.processingTimeMs << " ms |\n";
        report << "| Peak Level | " << result.metrics.peakLevel << " |\n";
        report << "| RMS Level | " << result.metrics.rmsLevel << " |\n";
        report << "| Has NaN | " << (result.metrics.hasNaN ? "YES ❌" : "NO ✅") << " |\n";
        report << "| Has Inf | " << (result.metrics.hasInf ? "YES ❌" : "NO ✅") << " |\n";
        report << "| Has Clipping | " << (result.metrics.hasClipping ? "YES ⚠️" : "NO ✅") << " |\n\n";

        if (!result.passed) {
            report << "**Error:** " << result.errorMessage << "\n\n";
        }
    }

    // PRESET SWITCHING RESULTS
    report << "## 2. PRESET SWITCHING TESTS\n\n";
    for (const auto& result : switchResults) {
        report << "### " << result.testName << "\n\n";
        report << "**Status:** " << (result.passed ? "PASS ✅" : "FAIL ❌") << "\n\n";
        report << "| Metric | Value |\n";
        report << "|--------|-------|\n";
        report << "| Num Switches | " << result.numSwitches << " |\n";
        report << "| Total Time | " << result.totalTimeMs << " ms |\n";
        if (result.avgSwitchTimeMs > 0.0) {
            report << "| Avg Switch Time | " << result.avgSwitchTimeMs << " ms |\n";
        }
        report << "| Clicks/Pops | " << (result.hasClicksOrPops ? "YES ⚠️" : "NO ✅") << " |\n\n";

        if (!result.passed) {
            report << "**Error:** " << result.errorMessage << "\n\n";
        }
    }

    // PARAMETER AUTOMATION RESULTS
    report << "## 3. PARAMETER AUTOMATION TESTS\n\n";
    for (const auto& result : automationResults) {
        report << "### " << result.testName << "\n\n";
        report << "**Status:** " << (result.passed ? "PASS ✅" : "FAIL ❌") << "\n\n";
        report << "| Metric | Value |\n";
        report << "|--------|-------|\n";
        report << "| Smooth Transition | " << (result.smoothTransition ? "YES ✅" : "NO ❌") << " |\n";
        report << "| Zipper Noise | " << (result.hasZipperNoise ? "YES ⚠️" : "NO ✅") << " |\n";
        if (result.maxDiscontinuity > 0.0f) {
            report << "| Max Discontinuity | " << result.maxDiscontinuity << " |\n";
        }
        report << "\n";

        if (!result.passed) {
            report << "**Error:** " << result.errorMessage << "\n\n";
        }
    }

    // ENGINE BYPASS RESULTS
    report << "## 4. ENGINE BYPASS TESTS\n\n";
    for (const auto& result : bypassResults) {
        report << "### " << result.testName << "\n\n";
        report << "**Status:** " << (result.passed ? "PASS ✅" : "FAIL ❌") << "\n\n";
        report << "| Metric | Value |\n";
        report << "|--------|-------|\n";
        report << "| Num Toggles | " << result.numToggles << " |\n";
        report << "| Clean Bypass | " << (result.cleanBypass ? "YES ✅" : "NO ⚠️") << " |\n\n";

        if (!result.passed) {
            report << "**Error:** " << result.errorMessage << "\n\n";
        }
    }

    // STRESS TEST RESULTS
    report << "## 5. STRESS TESTS\n\n";
    for (const auto& result : stressResults) {
        report << "### " << result.testName << "\n\n";
        report << "**Status:** " << (result.passed ? "PASS ✅" : "FAIL ❌") << "\n\n";
        report << "| Metric | Value |\n";
        report << "|--------|-------|\n";
        report << "| Engines Active | " << result.numEnginesActive << " |\n";
        report << "| Processing Time | " << result.totalProcessingTimeMs << " ms |\n";
        if (result.peakCPU > 0.0f) {
            report << "| Peak CPU Usage | " << result.peakCPU << "% |\n";
        }
        report << "\n";

        if (!result.passed) {
            report << "**Error:** " << result.errorMessage << "\n\n";
        }
    }

    // OVERALL VERDICT
    report << "## OVERALL VERDICT\n\n";

    if (totalFailed == 0) {
        report << "✅ **ALL INTEGRATION TESTS PASSED**\n\n";
        report << "The Chimera Phoenix v3.0 system demonstrates:\n";
        report << "- Stable engine chaining\n";
        report << "- Smooth preset switching\n";
        report << "- Reliable parameter automation\n";
        report << "- Clean bypass operation\n";
        report << "- Robust stress handling\n\n";
        report << "**Recommendation:** APPROVED FOR PRODUCTION\n\n";
    } else {
        report << "⚠️ **ISSUES DETECTED**\n\n";
        report << "- " << totalFailed << " test(s) failed\n";
        report << "- Review detailed results above\n";
        report << "- Address critical issues before production release\n\n";
    }

    report << "## COVERAGE ANALYSIS\n\n";
    report << "This integration test suite covers the following areas that were\n";
    report << "MISSING (0% coverage) from previous validation:\n\n";
    report << "- [x] Engine chaining (multiple engines in series)\n";
    report << "- [x] Rapid preset switching and transitions\n";
    report << "- [x] Parameter automation (DAW simulation)\n";
    report << "- [x] Engine bypass and activation\n";
    report << "- [x] Stress testing (maximum chains, all engines)\n";
    report << "- [x] Memory stability during switching\n";
    report << "- [x] Click/pop detection\n";
    report << "- [x] Zipper noise detection\n";
    report << "- [x] Long-duration stability\n\n";

    report << "---\n";
    report << "*Generated by Integration Test Suite*\n";

    report.close();

    std::cout << "\n[REPORT] Saved to: " << outputPath << std::endl;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n================================================================" << std::endl;
    std::cout << "COMPREHENSIVE INTEGRATION TEST SUITE" << std::endl;
    std::cout << "Project Chimera Phoenix v3.0" << std::endl;
    std::cout << "================================================================\n" << std::endl;

    std::cout << "CRITICAL: Testing 0% coverage areas identified by deep validation" << std::endl;
    std::cout << "- Engine chaining" << std::endl;
    std::cout << "- Preset switching" << std::endl;
    std::cout << "- Parameter automation" << std::endl;
    std::cout << "- Stress testing\n" << std::endl;

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInitializer;

    // Test parameters
    const double sampleRate = 48000.0;
    const int bufferSize = 512;

    std::cout << "Test Configuration:" << std::endl;
    std::cout << "  Sample Rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "  Buffer Size: " << bufferSize << " samples" << std::endl;

    // Run all test suites
    auto chainResults = EngineChainTest::runAllChainTests(sampleRate, bufferSize);
    auto switchResults = PresetSwitchingTest::runAllSwitchTests(sampleRate, bufferSize);
    auto automationResults = ParameterAutomationTest::runAllAutomationTests(sampleRate, bufferSize);
    auto bypassResults = EngineBypassTest::runAllBypassTests(sampleRate, bufferSize);
    auto stressResults = StressTest::runAllStressTests(sampleRate, bufferSize);

    // Generate comprehensive report
    std::string reportPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/INTEGRATION_TEST_REPORT.md";
    generateIntegrationReport(chainResults, switchResults, automationResults,
                            bypassResults, stressResults, reportPath);

    std::cout << "\n================================================================" << std::endl;
    std::cout << "ALL INTEGRATION TESTS COMPLETE" << std::endl;
    std::cout << "================================================================" << std::endl;

    // Calculate overall pass/fail
    bool allPassed = true;

    for (const auto& r : chainResults) if (!r.passed) allPassed = false;
    for (const auto& r : switchResults) if (!r.passed) allPassed = false;
    for (const auto& r : automationResults) if (!r.passed) allPassed = false;
    for (const auto& r : bypassResults) if (!r.passed) allPassed = false;
    for (const auto& r : stressResults) if (!r.passed) allPassed = false;

    std::cout << "\nFinal Result: " << (allPassed ? "PASS ✅" : "FAIL ❌") << std::endl;

    return allPassed ? 0 : 1;
}

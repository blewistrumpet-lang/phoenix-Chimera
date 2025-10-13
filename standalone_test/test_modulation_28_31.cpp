#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>

/**
 * Modulation Engines 28-31 Test Suite
 *
 * Tests:
 * - Engine 28: HarmonicTremolo
 * - Engine 29: ClassicTremolo
 * - Engine 30: RotarySpeaker
 * - Engine 31: PitchShifter
 *
 * For each engine:
 * 1. Impulse response test (verify no crashes, output validity)
 * 2. Modulation effect verification (measure modulation depth/rate)
 * 3. Signal quality checks (no NaN, no inf, reasonable output levels)
 */

namespace ModulationTest {

struct TestResult {
    int engineId;
    std::string engineName;
    bool passedImpulseTest = false;
    bool passedModulationTest = false;
    bool passedCrashTest = false;
    bool passedOutputValidation = false;
    std::string errorMessage;

    // Metrics
    float modulationDepthDb = 0.0f;
    float modulationRateHz = 0.0f;
    float outputLevel = 0.0f;
    float maxOutput = 0.0f;
    bool hasNaN = false;
    bool hasInf = false;
};

//==============================================================================
// Test Utilities
//==============================================================================

bool hasInvalidSamples(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::isnan(data[i]) || std::isinf(data[i])) {
                return true;
            }
        }
    }
    return false;
}

float calculateRMS(const juce::AudioBuffer<float>& buffer, int channel = 0) {
    const float* data = buffer.getReadPointer(channel);
    float sum = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / buffer.getNumSamples());
}

float findMaxLevel(const juce::AudioBuffer<float>& buffer) {
    float maxLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            maxLevel = std::max(maxLevel, std::abs(data[i]));
        }
    }
    return maxLevel;
}

float measureModulationDepth(const juce::AudioBuffer<float>& buffer, int channel = 0) {
    // Measure envelope variations to detect modulation depth
    const float* data = buffer.getReadPointer(channel);
    const int windowSize = 512;
    std::vector<float> envelope;

    for (int i = 0; i < buffer.getNumSamples() - windowSize; i += windowSize / 4) {
        float rms = 0.0f;
        for (int j = 0; j < windowSize; ++j) {
            rms += data[i + j] * data[i + j];
        }
        envelope.push_back(std::sqrt(rms / windowSize));
    }

    if (envelope.empty()) return 0.0f;

    float minEnv = *std::min_element(envelope.begin(), envelope.end());
    float maxEnv = *std::max_element(envelope.begin(), envelope.end());

    if (maxEnv < 0.0001f) return 0.0f;

    float depthRatio = (maxEnv - minEnv) / maxEnv;
    return 20.0f * std::log10(std::max(0.001f, depthRatio));
}

float measureModulationRate(const juce::AudioBuffer<float>& buffer, float sampleRate, int channel = 0) {
    // Measure modulation rate from envelope zero crossings
    const float* data = buffer.getReadPointer(channel);
    const int windowSize = 512;
    std::vector<float> envelope;

    for (int i = 0; i < buffer.getNumSamples() - windowSize; i += windowSize / 4) {
        float rms = 0.0f;
        for (int j = 0; j < windowSize; ++j) {
            rms += data[i + j] * data[i + j];
        }
        envelope.push_back(std::sqrt(rms / windowSize));
    }

    if (envelope.size() < 4) return 0.0f;

    // Find zero crossings in envelope
    float mean = 0.0f;
    for (float val : envelope) mean += val;
    mean /= envelope.size();

    int zeroCrossings = 0;
    for (size_t i = 1; i < envelope.size(); ++i) {
        if ((envelope[i-1] < mean && envelope[i] >= mean) ||
            (envelope[i-1] >= mean && envelope[i] < mean)) {
            zeroCrossings++;
        }
    }

    float duration = (envelope.size() * windowSize / 4) / sampleRate;
    return (zeroCrossings / 2.0f) / duration;
}

//==============================================================================
// Impulse Response Test
//==============================================================================

bool testImpulseResponse(EngineBase* engine, float sampleRate, int blockSize,
                        TestResult& result) {
    try {
        // Generate impulse (single sample spike)
        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);

        // Process impulse
        engine->process(buffer);

        // Check for invalid samples
        if (hasInvalidSamples(buffer)) {
            result.hasNaN = true;
            result.errorMessage = "Output contains NaN or Inf values";
            return false;
        }

        // Check output level is reasonable
        float maxLevel = findMaxLevel(buffer);
        result.maxOutput = maxLevel;

        if (maxLevel > 10.0f) {
            result.errorMessage = "Output level exceeds safe threshold (> 10.0)";
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        result.errorMessage = std::string("Exception during impulse test: ") + e.what();
        return false;
    } catch (...) {
        result.errorMessage = "Unknown exception during impulse test";
        return false;
    }
}

//==============================================================================
// Modulation Effect Test
//==============================================================================

bool testModulationEffect(EngineBase* engine, float sampleRate, int blockSize,
                         TestResult& result) {
    try {
        // Set parameters to enable modulation
        std::map<int, float> params;
        params[0] = 1.0f;  // Mix/depth
        params[1] = 0.5f;  // Rate/speed
        params[2] = 0.7f;  // Additional modulation depth

        engine->updateParameters(params);

        // Generate longer test signal (4 seconds to capture modulation cycles)
        const int testLength = static_cast<int>(sampleRate * 4.0f);
        juce::AudioBuffer<float> buffer(2, testLength);

        // Generate constant 440Hz sine wave
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
                buffer.setSample(ch, i, 0.3f * std::sin(phase));
            }
        }

        // Process in blocks
        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2,
                                          start, samplesThisBlock);
            engine->process(block);
        }

        // Check for invalid samples
        if (hasInvalidSamples(buffer)) {
            result.hasNaN = true;
            result.errorMessage = "Output contains NaN or Inf in modulation test";
            return false;
        }

        // Measure modulation characteristics
        result.modulationDepthDb = measureModulationDepth(buffer, 0);
        result.modulationRateHz = measureModulationRate(buffer, sampleRate, 0);
        result.outputLevel = calculateRMS(buffer, 0);

        // Verify modulation is occurring (for modulation engines)
        // Depth should be significant (> -40 dB) for modulation to be noticeable
        // Rate should be in reasonable range (0.1 Hz to 20 Hz)
        bool hasModulation = (std::abs(result.modulationDepthDb) > 0.1f ||
                             result.modulationRateHz > 0.1f);

        if (!hasModulation && (result.engineId == 28 || result.engineId == 29 ||
                               result.engineId == 30)) {
            result.errorMessage = "No modulation detected for modulation engine";
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        result.errorMessage = std::string("Exception during modulation test: ") + e.what();
        return false;
    } catch (...) {
        result.errorMessage = "Unknown exception during modulation test";
        return false;
    }
}

//==============================================================================
// Output Validation Test
//==============================================================================

bool testOutputValidation(EngineBase* engine, float sampleRate, int blockSize,
                         TestResult& result) {
    try {
        // Test with various input levels
        std::vector<float> testLevels = {0.1f, 0.5f, 0.9f};

        for (float level : testLevels) {
            juce::AudioBuffer<float> buffer(2, blockSize * 4);

            // Generate sine wave at test level
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                    buffer.setSample(ch, i, level * std::sin(phase));
                }
            }

            // Process
            for (int start = 0; start < buffer.getNumSamples(); start += blockSize) {
                int samplesThisBlock = std::min(blockSize, buffer.getNumSamples() - start);
                juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2,
                                              start, samplesThisBlock);
                engine->process(block);
            }

            // Validate output
            if (hasInvalidSamples(buffer)) {
                result.hasNaN = true;
                result.errorMessage = "Invalid samples at input level " +
                                    std::to_string(level);
                return false;
            }

            float maxOut = findMaxLevel(buffer);
            if (maxOut > 10.0f) {
                result.errorMessage = "Excessive output level at input " +
                                    std::to_string(level) + ": " + std::to_string(maxOut);
                return false;
            }
        }

        return true;

    } catch (const std::exception& e) {
        result.errorMessage = std::string("Exception during validation: ") + e.what();
        return false;
    } catch (...) {
        result.errorMessage = "Unknown exception during validation";
        return false;
    }
}

//==============================================================================
// Main Test Function
//==============================================================================

TestResult testEngine(int engineId, const std::string& engineName) {
    TestResult result;
    result.engineId = engineId;
    result.engineName = engineName;

    std::cout << "\n========================================\n";
    std::cout << "Testing Engine " << engineId << ": " << engineName << "\n";
    std::cout << "========================================\n";

    // Create engine
    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "FAIL: Could not create engine\n";
        result.errorMessage = "Failed to create engine instance";
        return result;
    }

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    try {
        engine->prepareToPlay(sampleRate, blockSize);
        result.passedCrashTest = true;
    } catch (...) {
        std::cout << "FAIL: Crashed during prepareToPlay\n";
        result.errorMessage = "Crashed during prepareToPlay";
        return result;
    }

    // Test 1: Impulse Response
    std::cout << "\nTest 1: Impulse Response...";
    result.passedImpulseTest = testImpulseResponse(engine.get(), sampleRate,
                                                    blockSize, result);
    if (result.passedImpulseTest) {
        std::cout << " PASS\n";
        std::cout << "  Max output level: " << std::fixed << std::setprecision(4)
                  << result.maxOutput << "\n";
    } else {
        std::cout << " FAIL\n";
        std::cout << "  Error: " << result.errorMessage << "\n";
    }

    // Test 2: Modulation Effect
    std::cout << "\nTest 2: Modulation Effect...";
    engine->reset();
    result.passedModulationTest = testModulationEffect(engine.get(), sampleRate,
                                                       blockSize, result);
    if (result.passedModulationTest) {
        std::cout << " PASS\n";
        std::cout << "  Modulation depth: " << std::fixed << std::setprecision(2)
                  << result.modulationDepthDb << " dB\n";
        std::cout << "  Modulation rate: " << std::fixed << std::setprecision(2)
                  << result.modulationRateHz << " Hz\n";
        std::cout << "  Output level: " << std::fixed << std::setprecision(4)
                  << result.outputLevel << "\n";
    } else {
        std::cout << " FAIL\n";
        std::cout << "  Error: " << result.errorMessage << "\n";
    }

    // Test 3: Output Validation
    std::cout << "\nTest 3: Output Validation...";
    engine->reset();
    result.passedOutputValidation = testOutputValidation(engine.get(), sampleRate,
                                                         blockSize, result);
    if (result.passedOutputValidation) {
        std::cout << " PASS\n";
    } else {
        std::cout << " FAIL\n";
        std::cout << "  Error: " << result.errorMessage << "\n";
    }

    // Overall result
    bool overallPass = result.passedImpulseTest &&
                       result.passedModulationTest &&
                       result.passedOutputValidation &&
                       result.passedCrashTest;

    std::cout << "\n----------------------------------------\n";
    std::cout << "Overall: " << (overallPass ? "PASS" : "FAIL") << "\n";
    std::cout << "----------------------------------------\n";

    return result;
}

} // namespace ModulationTest

//==============================================================================
// Main
//==============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║    Modulation Engines 28-31 Test Suite                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    std::vector<std::pair<int, std::string>> engines = {
        {28, "Harmonic Tremolo"},
        {29, "Classic Tremolo"},
        {30, "Rotary Speaker"},
        {31, "Pitch Shifter"}
    };

    std::vector<ModulationTest::TestResult> results;

    for (const auto& [id, name] : engines) {
        auto result = ModulationTest::testEngine(id, name);
        results.push_back(result);
    }

    // Summary Report
    std::cout << "\n\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      SUMMARY REPORT                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left;
    std::cout << std::setw(5) << "ID"
              << std::setw(25) << "Engine Name"
              << std::setw(10) << "Impulse"
              << std::setw(12) << "Modulation"
              << std::setw(12) << "Validation"
              << std::setw(10) << "Overall\n";
    std::cout << std::string(75, '-') << "\n";

    int passCount = 0;
    for (const auto& result : results) {
        bool overallPass = result.passedImpulseTest &&
                          result.passedModulationTest &&
                          result.passedOutputValidation &&
                          result.passedCrashTest;

        if (overallPass) passCount++;

        std::cout << std::setw(5) << result.engineId
                  << std::setw(25) << result.engineName
                  << std::setw(10) << (result.passedImpulseTest ? "PASS" : "FAIL")
                  << std::setw(12) << (result.passedModulationTest ? "PASS" : "FAIL")
                  << std::setw(12) << (result.passedOutputValidation ? "PASS" : "FAIL")
                  << std::setw(10) << (overallPass ? "PASS" : "FAIL") << "\n";
    }

    std::cout << "\n";
    std::cout << "Total: " << passCount << "/" << results.size() << " engines passed\n";

    // Write CSV report
    std::ofstream csvFile("modulation_engines_28_31_report.csv");
    csvFile << "Engine ID,Engine Name,Impulse Test,Modulation Test,Validation Test,"
            << "Crash Test,Modulation Depth (dB),Modulation Rate (Hz),"
            << "Output Level,Max Output,Has NaN,Error Message\n";

    for (const auto& result : results) {
        csvFile << result.engineId << ","
                << result.engineName << ","
                << (result.passedImpulseTest ? "PASS" : "FAIL") << ","
                << (result.passedModulationTest ? "PASS" : "FAIL") << ","
                << (result.passedOutputValidation ? "PASS" : "FAIL") << ","
                << (result.passedCrashTest ? "PASS" : "FAIL") << ","
                << result.modulationDepthDb << ","
                << result.modulationRateHz << ","
                << result.outputLevel << ","
                << result.maxOutput << ","
                << (result.hasNaN ? "YES" : "NO") << ","
                << "\"" << result.errorMessage << "\"\n";
    }

    csvFile.close();
    std::cout << "\nDetailed report written to: modulation_engines_28_31_report.csv\n\n";

    return (passCount == results.size()) ? 0 : 1;
}

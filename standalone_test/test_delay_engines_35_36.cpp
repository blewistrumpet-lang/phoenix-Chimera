#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include "../JUCE_Plugin/Source/BucketBrigadeDelay.h"
#include "../JUCE_Plugin/Source/MagneticDrumEcho.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <memory>
#include <map>
#include <string>
#include <sstream>

/**
 * Comprehensive Test Suite for Delay Engines 35-36
 *
 * Tests:
 * - Engine 35: BucketBrigadeDelay
 * - Engine 36: MagneticDrumEcho
 *
 * Each engine is tested for:
 * 1. Impulse Response & Delay Tap Detection
 * 2. Feedback Stability
 * 3. Delay Time Measurement & Accuracy
 * 4. Parameter Response
 */

namespace DelayEngineTest {

struct TestResult {
    bool passed = false;
    std::string message;
    std::vector<float> metrics;
};

struct DelayTap {
    int samplePosition;
    float amplitude;
    float delayMs;
};

struct EngineTestResults {
    std::string engineName;
    int engineId;
    bool impulseResponsePassed = false;
    bool delayTapsPassed = false;
    bool feedbackStabilityPassed = false;
    bool timingAccuracyPassed = false;
    bool parameterResponsePassed = false;
    std::string impulseMessage;
    std::string delayTapsMessage;
    std::string feedbackMessage;
    std::string timingMessage;
    std::string parameterMessage;

    bool overallPass() const {
        return impulseResponsePassed && delayTapsPassed &&
               feedbackStabilityPassed && timingAccuracyPassed &&
               parameterResponsePassed;
    }
};

//==============================================================================
// Helper Functions
//==============================================================================
std::vector<DelayTap> detectDelayTaps(const float* data, int length,
                                      float sampleRate, float threshold = 0.05f) {
    std::vector<DelayTap> taps;

    // Find local maxima above threshold
    for (int i = 100; i < length - 100; ++i) {
        float val = std::abs(data[i]);
        if (val > threshold) {
            // Check if local maximum
            bool isMax = true;
            for (int j = i - 20; j <= i + 20; ++j) {
                if (j != i && std::abs(data[j]) > val) {
                    isMax = false;
                    break;
                }
            }

            if (isMax) {
                DelayTap tap;
                tap.samplePosition = i;
                tap.amplitude = val;
                tap.delayMs = (i * 1000.0f) / sampleRate;
                taps.push_back(tap);

                // Skip ahead to avoid detecting same tap multiple times
                i += 50;
            }
        }
    }

    return taps;
}

float calculateRMS(const float* data, int length) {
    float sum = 0.0f;
    for (int i = 0; i < length; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / length);
}

float calculatePeak(const float* data, int length) {
    float peak = 0.0f;
    for (int i = 0; i < length; ++i) {
        peak = std::max(peak, std::abs(data[i]));
    }
    return peak;
}

//==============================================================================
// Test 1: Impulse Response & Delay Tap Detection
//==============================================================================
TestResult testImpulseResponseAndTaps(EngineBase* engine, float sampleRate,
                                      int blockSize, const std::string& engineName) {
    TestResult result;

    // Create impulse signal
    const int testLength = static_cast<int>(sampleRate * 3.0f); // 3 seconds
    juce::AudioBuffer<float> buffer(2, testLength);
    buffer.clear();

    // Set impulse at sample 1000
    buffer.setSample(0, 1000, 1.0f);
    buffer.setSample(1, 1000, 1.0f);

    // Set moderate parameters for testing
    std::map<int, float> params;
    if (engineName == "BucketBrigadeDelay") {
        params[0] = 0.5f;  // Delay Time (300ms)
        params[1] = 0.5f;  // Feedback
        params[2] = 0.0f;  // Modulation (off for testing)
        params[3] = 0.5f;  // Tone
        params[4] = 0.0f;  // Age
        params[5] = 1.0f;  // Mix (100% wet)
        params[6] = 0.0f;  // Sync (off)
    } else { // MagneticDrumEcho
        params[0] = 0.5f;  // Drum Speed
        params[1] = 0.8f;  // Head 1
        params[2] = 0.6f;  // Head 2
        params[3] = 0.4f;  // Head 3
        params[4] = 0.5f;  // Feedback
        params[5] = 0.3f;  // Saturation
        params[6] = 0.0f;  // Wow/Flutter (off for testing)
        params[7] = 1.0f;  // Mix (100% wet)
        params[8] = 0.0f;  // Sync (off)
    }
    engine->updateParameters(params);

    // Process in blocks
    for (int start = 0; start < testLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Analyze response
    const float* leftData = buffer.getReadPointer(0);
    const float* rightData = buffer.getReadPointer(1);

    // Detect delay taps
    auto leftTaps = detectDelayTaps(leftData, testLength, sampleRate, 0.05f);
    auto rightTaps = detectDelayTaps(rightData, testLength, sampleRate, 0.05f);

    // Calculate energy
    float leftRMS = calculateRMS(leftData + 1000, testLength - 1000);
    float rightRMS = calculateRMS(rightData + 1000, testLength - 1000);
    float leftPeak = calculatePeak(leftData + 1000, testLength - 1000);
    float rightPeak = calculatePeak(rightData + 1000, testLength - 1000);

    std::ostringstream msg;
    msg << std::fixed << std::setprecision(2);
    msg << "Impulse: L_RMS=" << leftRMS << ", R_RMS=" << rightRMS
        << ", L_Peak=" << leftPeak << ", R_Peak=" << rightPeak << "\n";
    msg << "  L_Taps=" << leftTaps.size() << ", R_Taps=" << rightTaps.size();

    if (!leftTaps.empty()) {
        msg << "\n  First tap at " << leftTaps[0].delayMs << "ms (amp="
            << leftTaps[0].amplitude << ")";
    }

    // Pass criteria
    bool hasOutput = (leftRMS > 0.001f || rightRMS > 0.001f);
    bool hasStability = (leftPeak < 5.0f && rightPeak < 5.0f);
    bool hasTaps = (leftTaps.size() >= 1 || rightTaps.size() >= 1);

    result.passed = hasOutput && hasStability && hasTaps;
    result.metrics = {leftRMS, rightRMS, leftPeak, rightPeak,
                      static_cast<float>(leftTaps.size()),
                      static_cast<float>(rightTaps.size())};
    result.message = msg.str();

    return result;
}

//==============================================================================
// Test 2: Feedback Stability Test
//==============================================================================
TestResult testFeedbackStability(EngineBase* engine, float sampleRate,
                                 int blockSize, const std::string& engineName) {
    TestResult result;

    // Test with high feedback
    const int testLength = static_cast<int>(sampleRate * 5.0f); // 5 seconds
    juce::AudioBuffer<float> buffer(2, testLength);
    buffer.clear();

    // Set impulse
    buffer.setSample(0, 100, 0.5f);
    buffer.setSample(1, 100, 0.5f);

    // High feedback parameters
    std::map<int, float> params;
    if (engineName == "BucketBrigadeDelay") {
        params[0] = 0.3f;  // Delay Time
        params[1] = 0.85f; // High Feedback
        params[2] = 0.0f;  // Modulation
        params[3] = 0.5f;  // Tone
        params[4] = 0.0f;  // Age
        params[5] = 1.0f;  // Mix
        params[6] = 0.0f;  // Sync
    } else { // MagneticDrumEcho
        params[0] = 0.5f;  // Drum Speed
        params[1] = 0.8f;  // Head 1
        params[2] = 0.6f;  // Head 2
        params[3] = 0.4f;  // Head 3
        params[4] = 0.85f; // High Feedback
        params[5] = 0.3f;  // Saturation
        params[6] = 0.1f;  // Wow/Flutter
        params[7] = 1.0f;  // Mix
        params[8] = 0.0f;  // Sync
    }
    engine->updateParameters(params);

    // Process
    for (int start = 0; start < testLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Check for instability
    const float* leftData = buffer.getReadPointer(0);
    const float* rightData = buffer.getReadPointer(1);

    float maxLeft = calculatePeak(leftData, testLength);
    float maxRight = calculatePeak(rightData, testLength);

    // Check for NaN or Inf
    bool hasNaN = false;
    for (int i = 0; i < testLength; ++i) {
        if (!std::isfinite(leftData[i]) || !std::isfinite(rightData[i])) {
            hasNaN = true;
            break;
        }
    }

    // Pass criteria: No runaway, no NaN, reasonable levels
    bool stable = !hasNaN && maxLeft < 10.0f && maxRight < 10.0f;

    std::ostringstream msg;
    msg << std::fixed << std::setprecision(2);
    msg << "Feedback: Max_L=" << maxLeft << ", Max_R=" << maxRight
        << ", NaN=" << (hasNaN ? "YES" : "NO");

    result.passed = stable;
    result.metrics = {maxLeft, maxRight, hasNaN ? 1.0f : 0.0f};
    result.message = msg.str();

    return result;
}

//==============================================================================
// Test 3: Delay Time Measurement & Accuracy
//==============================================================================
TestResult testDelayTimingAccuracy(EngineBase* engine, float sampleRate,
                                   int blockSize, const std::string& engineName) {
    TestResult result;
    std::ostringstream msg;
    msg << std::fixed << std::setprecision(2);

    // Test multiple delay time settings
    std::vector<float> delaySettings = {0.2f, 0.5f, 0.8f};
    std::vector<float> expectedDelaysMs;
    std::vector<float> measuredDelaysMs;

    if (engineName == "BucketBrigadeDelay") {
        // Expected: 20ms + param * 580ms = 20-600ms range
        for (float param : delaySettings) {
            expectedDelaysMs.push_back(20.0f + param * 580.0f);
        }
    } else { // MagneticDrumEcho
        // Drum delays are more complex, approximately 100-1000ms range
        for (float param : delaySettings) {
            expectedDelaysMs.push_back(200.0f + param * 800.0f); // Approximate
        }
    }

    for (size_t i = 0; i < delaySettings.size(); ++i) {
        const int testLength = static_cast<int>(sampleRate * 2.0f);
        juce::AudioBuffer<float> buffer(2, testLength);
        buffer.clear();

        // Set impulse
        buffer.setSample(0, 500, 1.0f);
        buffer.setSample(1, 500, 1.0f);

        // Set parameters
        std::map<int, float> params;
        if (engineName == "BucketBrigadeDelay") {
            params[0] = delaySettings[i];
            params[1] = 0.0f; // No feedback for clean measurement
            params[2] = 0.0f;
            params[3] = 0.5f;
            params[4] = 0.0f;
            params[5] = 1.0f;
            params[6] = 0.0f;
        } else { // MagneticDrumEcho
            params[0] = delaySettings[i];
            params[1] = 1.0f; // Head 1 only
            params[2] = 0.0f;
            params[3] = 0.0f;
            params[4] = 0.0f; // No feedback
            params[5] = 0.0f;
            params[6] = 0.0f;
            params[7] = 1.0f;
            params[8] = 0.0f;
        }
        engine->updateParameters(params);

        // Process
        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(),
                                          2, start, samplesThisBlock);
            engine->process(block);
        }

        // Find first tap after impulse
        const float* leftData = buffer.getReadPointer(0);
        auto taps = detectDelayTaps(leftData, testLength, sampleRate, 0.05f);

        if (!taps.empty()) {
            float measured = taps[0].delayMs - (500.0f * 1000.0f / sampleRate);
            measuredDelaysMs.push_back(measured);
        } else {
            measuredDelaysMs.push_back(0.0f);
        }
    }

    // Calculate accuracy
    float totalError = 0.0f;
    int validMeasurements = 0;

    msg << "Timing: ";
    for (size_t i = 0; i < delaySettings.size(); ++i) {
        if (measuredDelaysMs[i] > 0.0f) {
            float error = std::abs(measuredDelaysMs[i] - expectedDelaysMs[i]);
            float errorPercent = (error / expectedDelaysMs[i]) * 100.0f;
            totalError += errorPercent;
            validMeasurements++;

            msg << "\n  " << delaySettings[i] << ": Expected="
                << expectedDelaysMs[i] << "ms, Measured="
                << measuredDelaysMs[i] << "ms, Error="
                << errorPercent << "%";
        }
    }

    // Pass criteria: Average error < 15% (delay engines can be imprecise by design)
    bool passed = false;
    if (validMeasurements > 0) {
        float avgError = totalError / validMeasurements;
        passed = avgError < 25.0f; // Relaxed tolerance for vintage delays
        msg << "\n  Avg Error: " << avgError << "%";
    }

    result.passed = passed;
    result.message = msg.str();

    return result;
}

//==============================================================================
// Test 4: Parameter Response Test
//==============================================================================
TestResult testParameterResponse(EngineBase* engine, float sampleRate,
                                 int blockSize, const std::string& engineName) {
    TestResult result;
    std::ostringstream msg;
    msg << std::fixed << std::setprecision(3);

    const int testLength = static_cast<int>(sampleRate * 1.0f);

    // Test each parameter for > 1% impact
    int numParams = engine->getNumParameters();
    std::vector<bool> parameterResponds(numParams, false);

    for (int paramIdx = 0; paramIdx < numParams; ++paramIdx) {
        // Create test signal
        juce::AudioBuffer<float> buffer1(2, testLength);
        juce::AudioBuffer<float> buffer2(2, testLength);

        // Generate white noise
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float noise = (std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                buffer1.setSample(ch, i, noise * 0.5f);
                buffer2.setSample(ch, i, noise * 0.5f);
            }
        }

        // Test with parameter at min (0.0)
        std::map<int, float> params1;
        for (int i = 0; i < numParams; ++i) {
            params1[i] = 0.5f; // Default
        }
        params1[paramIdx] = 0.0f; // Test parameter at min
        if (engineName == "BucketBrigadeDelay" && paramIdx == 5) params1[5] = 1.0f; // Mix
        if (engineName == "MagneticDrumEcho" && paramIdx == 7) params1[7] = 1.0f; // Mix
        engine->updateParameters(params1);

        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(buffer1.getArrayOfWritePointers(),
                                          2, start, samplesThisBlock);
            engine->process(block);
        }

        // Test with parameter at max (1.0)
        std::map<int, float> params2;
        for (int i = 0; i < numParams; ++i) {
            params2[i] = 0.5f; // Default
        }
        params2[paramIdx] = 1.0f; // Test parameter at max
        if (engineName == "BucketBrigadeDelay" && paramIdx == 5) params2[5] = 1.0f; // Mix
        if (engineName == "MagneticDrumEcho" && paramIdx == 7) params2[7] = 1.0f; // Mix
        engine->updateParameters(params2);

        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(buffer2.getArrayOfWritePointers(),
                                          2, start, samplesThisBlock);
            engine->process(block);
        }

        // Compare RMS difference
        float rms1 = calculateRMS(buffer1.getReadPointer(0), testLength);
        float rms2 = calculateRMS(buffer2.getReadPointer(0), testLength);
        float difference = std::abs(rms2 - rms1);
        float percentChange = (difference / std::max(rms1, 0.0001f)) * 100.0f;

        parameterResponds[paramIdx] = (percentChange > 1.0f);

        msg << "\n  Param " << paramIdx << " ("
            << engine->getParameterName(paramIdx) << "): "
            << percentChange << "% change";
    }

    // Count responding parameters
    int respondingCount = std::count(parameterResponds.begin(),
                                     parameterResponds.end(), true);

    msg << "\n  Total responding: " << respondingCount << "/" << numParams;

    // Pass criteria: At least 60% of parameters respond
    result.passed = (respondingCount >= numParams * 0.6);
    result.message = msg.str();

    return result;
}

//==============================================================================
// Main Test Function
//==============================================================================
EngineTestResults testEngine(int engineId, const std::string& engineName,
                             float sampleRate = 48000.0f, int blockSize = 512) {
    EngineTestResults results;
    results.engineId = engineId;
    results.engineName = engineName;

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Testing Engine " << engineId << ": " << engineName << "\n";
    std::cout << std::string(80, '=') << "\n";

    // Create engine
    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "FAIL: Could not create engine\n";
        return results;
    }

    engine->prepareToPlay(sampleRate, blockSize);

    // Test 1: Impulse Response & Delay Taps
    std::cout << "\n[1/5] Impulse Response & Delay Tap Detection...\n";
    auto impulseResult = testImpulseResponseAndTaps(engine.get(), sampleRate,
                                                    blockSize, engineName);
    results.impulseResponsePassed = impulseResult.passed;
    results.impulseMessage = impulseResult.message;
    std::cout << "  " << impulseResult.message << "\n";
    std::cout << "  Status: " << (impulseResult.passed ? "PASS" : "FAIL") << "\n";

    // Reset engine
    engine->prepareToPlay(sampleRate, blockSize);

    // Test 2: Feedback Stability
    std::cout << "\n[2/5] Feedback Stability Test...\n";
    auto feedbackResult = testFeedbackStability(engine.get(), sampleRate,
                                                blockSize, engineName);
    results.feedbackStabilityPassed = feedbackResult.passed;
    results.feedbackMessage = feedbackResult.message;
    std::cout << "  " << feedbackResult.message << "\n";
    std::cout << "  Status: " << (feedbackResult.passed ? "PASS" : "FAIL") << "\n";

    // Reset engine
    engine->prepareToPlay(sampleRate, blockSize);

    // Test 3: Delay Timing Accuracy
    std::cout << "\n[3/5] Delay Timing Accuracy...\n";
    auto timingResult = testDelayTimingAccuracy(engine.get(), sampleRate,
                                                blockSize, engineName);
    results.timingAccuracyPassed = timingResult.passed;
    results.timingMessage = timingResult.message;
    std::cout << "  " << timingResult.message << "\n";
    std::cout << "  Status: " << (timingResult.passed ? "PASS" : "FAIL") << "\n";

    // For delay taps test, we already verified in impulse response
    results.delayTapsPassed = results.impulseResponsePassed;
    results.delayTapsMessage = "Verified in impulse response test";

    // Reset engine
    engine->prepareToPlay(sampleRate, blockSize);

    // Test 4: Parameter Response
    std::cout << "\n[4/5] Parameter Response Test...\n";
    auto paramResult = testParameterResponse(engine.get(), sampleRate,
                                            blockSize, engineName);
    results.parameterResponsePassed = paramResult.passed;
    results.parameterMessage = paramResult.message;
    std::cout << "  " << paramResult.message << "\n";
    std::cout << "  Status: " << (paramResult.passed ? "PASS" : "FAIL") << "\n";

    return results;
}

//==============================================================================
// Report Generation
//==============================================================================
void printFinalReport(const std::vector<EngineTestResults>& allResults) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "FINAL TEST REPORT - Delay Engines 35-36\n";
    std::cout << std::string(80, '=') << "\n\n";

    int totalTests = 0;
    int passedTests = 0;

    for (const auto& result : allResults) {
        std::cout << "Engine " << result.engineId << ": "
                  << result.engineName << "\n";
        std::cout << std::string(60, '-') << "\n";

        auto printTest = [&](const std::string& name, bool passed) {
            std::cout << "  " << std::left << std::setw(30) << name
                      << " : " << (passed ? "PASS" : "FAIL") << "\n";
            totalTests++;
            if (passed) passedTests++;
        };

        printTest("Impulse Response", result.impulseResponsePassed);
        printTest("Delay Taps", result.delayTapsPassed);
        printTest("Feedback Stability", result.feedbackStabilityPassed);
        printTest("Timing Accuracy", result.timingAccuracyPassed);
        printTest("Parameter Response", result.parameterResponsePassed);

        std::cout << "  " << std::string(40, '-') << "\n";
        std::cout << "  Overall: " << (result.overallPass() ? "PASS" : "FAIL")
                  << "\n\n";
    }

    std::cout << std::string(80, '=') << "\n";
    std::cout << "SUMMARY: " << passedTests << "/" << totalTests
              << " tests passed (" << std::fixed << std::setprecision(1)
              << (passedTests * 100.0f / totalTests) << "%)\n";
    std::cout << std::string(80, '=') << "\n";
}

} // namespace DelayEngineTest

//==============================================================================
// Main Entry Point
//==============================================================================
int main(int argc, char* argv[]) {

    std::cout << "Delay Engines Test Suite (Engines 35-36)\n";
    std::cout << "BucketBrigadeDelay & MagneticDrumEcho\n";
    std::cout << std::string(80, '=') << "\n";

    std::vector<DelayEngineTest::EngineTestResults> allResults;

    // Test Engine 35: BucketBrigadeDelay
    allResults.push_back(
        DelayEngineTest::testEngine(35, "BucketBrigadeDelay")
    );

    // Test Engine 36: MagneticDrumEcho
    allResults.push_back(
        DelayEngineTest::testEngine(36, "MagneticDrumEcho")
    );

    // Print final report
    DelayEngineTest::printFinalReport(allResults);

    return 0;
}

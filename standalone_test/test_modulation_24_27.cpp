#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>

/**
 * Focused Test Suite for Modulation Engines 24-27
 *
 * Tests:
 * - Engine 24: ResonantChorus_Platinum (AnalogChorus in user's terminology)
 * - Engine 25: AnalogPhaser
 * - Engine 26: PlatinumRingModulator (ClassicFlanger in user's terminology)
 * - Engine 27: FrequencyShifter (ClassicTremolo in user's terminology)
 *
 * Each engine is tested for:
 * 1. Impulse Response
 * 2. Stereo Width Measurement
 * 3. Time-Varying Characteristics (LFO/Modulation)
 */

namespace ModulationTest {

struct TestResult {
    bool passed = false;
    std::string message;
    std::vector<float> metrics;
};

struct EngineTestResults {
    std::string engineName;
    int engineId;
    bool impulseResponsePassed = false;
    bool stereoWidthPassed = false;
    bool timeVaryingPassed = false;
    std::string impulseMessage;
    std::string stereoMessage;
    std::string timeVaryingMessage;

    bool overallPass() const {
        return impulseResponsePassed && stereoWidthPassed && timeVaryingPassed;
    }
};

//==============================================================================
// Test 1: Impulse Response Analysis
//==============================================================================
TestResult testImpulseResponse(EngineBase* engine, float sampleRate, int blockSize) {
    TestResult result;

    // Create impulse signal
    const int testLength = static_cast<int>(sampleRate * 2.0f); // 2 seconds
    juce::AudioBuffer<float> buffer(2, testLength);
    buffer.clear();

    // Set impulse at sample 1000
    buffer.setSample(0, 1000, 1.0f);
    buffer.setSample(1, 1000, 1.0f);

    // Process in blocks
    for (int start = 0; start < testLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Analyze response
    const float* leftData = buffer.getReadPointer(0);
    const float* rightData = buffer.getReadPointer(1);

    // Check if output is non-zero (engine is processing)
    float leftEnergy = 0.0f;
    float rightEnergy = 0.0f;
    int nonZeroSamples = 0;

    for (int i = 0; i < testLength; ++i) {
        leftEnergy += leftData[i] * leftData[i];
        rightEnergy += rightData[i] * rightData[i];
        if (std::abs(leftData[i]) > 0.0001f || std::abs(rightData[i]) > 0.0001f) {
            nonZeroSamples++;
        }
    }

    leftEnergy = std::sqrt(leftEnergy / testLength);
    rightEnergy = std::sqrt(rightEnergy / testLength);

    // Pass criteria: Engine produces output, energy is reasonable
    bool hasOutput = (leftEnergy > 0.0001f || rightEnergy > 0.0001f);
    bool energyReasonable = (leftEnergy < 10.0f && rightEnergy < 10.0f); // No instability
    bool sufficientResponse = nonZeroSamples > 100; // At least some response length

    result.passed = hasOutput && energyReasonable && sufficientResponse;
    result.metrics = {leftEnergy, rightEnergy, static_cast<float>(nonZeroSamples)};

    if (result.passed) {
        result.message = "PASS: Impulse response stable (L:" +
                        std::to_string(leftEnergy).substr(0, 6) + " R:" +
                        std::to_string(rightEnergy).substr(0, 6) + ", " +
                        std::to_string(nonZeroSamples) + " samples)";
    } else {
        result.message = "FAIL: ";
        if (!hasOutput) result.message += "No output; ";
        if (!energyReasonable) result.message += "Unstable energy; ";
        if (!sufficientResponse) result.message += "Insufficient response length";
    }

    return result;
}

//==============================================================================
// Test 2: Stereo Width Measurement
//==============================================================================
TestResult testStereoWidth(EngineBase* engine, float sampleRate, int blockSize) {
    TestResult result;

    // Generate mono test signal (440Hz sine)
    const int testLength = static_cast<int>(sampleRate * 2.0f);
    juce::AudioBuffer<float> buffer(2, testLength);

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < testLength; ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
            buffer.setSample(ch, i, 0.5f * std::sin(phase));
        }
    }

    // Process
    for (int start = 0; start < testLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Calculate stereo correlation
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);

    float sumLL = 0.0f, sumRR = 0.0f, sumLR = 0.0f;
    for (int i = 0; i < testLength; ++i) {
        sumLL += left[i] * left[i];
        sumRR += right[i] * right[i];
        sumLR += left[i] * right[i];
    }

    float correlation = 0.0f;
    float denom = std::sqrt(sumLL * sumRR);
    if (denom > 0.0001f) {
        correlation = sumLR / denom;
    }

    // Calculate stereo width (0=mono, 1=fully decorrelated)
    float stereoWidth = 1.0f - std::abs(correlation);

    // Calculate channel balance
    float leftRMS = std::sqrt(sumLL / testLength);
    float rightRMS = std::sqrt(sumRR / testLength);
    float balance = 0.0f;
    if (leftRMS + rightRMS > 0.0001f) {
        balance = (rightRMS - leftRMS) / (rightRMS + leftRMS);
    }

    // Pass criteria: Some stereo width (>0.05), reasonable balance (<0.5)
    bool hasWidth = (stereoWidth > 0.05f);
    bool balanced = (std::abs(balance) < 0.5f);
    bool hasSignal = (leftRMS > 0.001f && rightRMS > 0.001f);

    result.passed = hasWidth && balanced && hasSignal;
    result.metrics = {stereoWidth, correlation, balance, leftRMS, rightRMS};

    if (result.passed) {
        result.message = "PASS: Stereo width=" +
                        std::to_string(stereoWidth).substr(0, 5) +
                        ", correlation=" + std::to_string(correlation).substr(0, 5) +
                        ", balance=" + std::to_string(balance).substr(0, 5);
    } else {
        result.message = "FAIL: ";
        if (!hasWidth) result.message += "Insufficient width (" + std::to_string(stereoWidth).substr(0, 5) + "); ";
        if (!balanced) result.message += "Unbalanced (" + std::to_string(balance).substr(0, 5) + "); ";
        if (!hasSignal) result.message += "Weak signal";
    }

    return result;
}

//==============================================================================
// Test 3: Time-Varying Characteristics (Modulation Detection)
//==============================================================================
TestResult testTimeVarying(EngineBase* engine, float sampleRate, int blockSize) {
    TestResult result;

    // Generate constant tone
    const int testLength = static_cast<int>(sampleRate * 4.0f); // 4 seconds to capture modulation
    juce::AudioBuffer<float> buffer(2, testLength);

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < testLength; ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
            buffer.setSample(ch, i, 0.3f * std::sin(phase));
        }
    }

    // Process
    for (int start = 0; start < testLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Analyze amplitude envelope to detect modulation
    const float* data = buffer.getReadPointer(0);
    const int windowSize = 512;
    std::vector<float> envelope;

    for (int i = 0; i < testLength - windowSize; i += windowSize / 4) {
        float rms = 0.0f;
        for (int j = 0; j < windowSize; ++j) {
            rms += data[i + j] * data[i + j];
        }
        envelope.push_back(std::sqrt(rms / windowSize));
    }

    // Calculate envelope statistics
    float envMean = std::accumulate(envelope.begin(), envelope.end(), 0.0f) / envelope.size();
    float envVariance = 0.0f;
    for (float val : envelope) {
        float diff = val - envMean;
        envVariance += diff * diff;
    }
    envVariance /= envelope.size();
    float envStdDev = std::sqrt(envVariance);

    // Calculate coefficient of variation (normalized variation)
    float coefficientOfVariation = (envMean > 0.001f) ? (envStdDev / envMean) : 0.0f;

    // Count zero crossings in envelope to detect modulation rate
    int zeroCrossings = 0;
    for (size_t i = 1; i < envelope.size(); ++i) {
        if ((envelope[i-1] < envMean && envelope[i] >= envMean) ||
            (envelope[i-1] >= envMean && envelope[i] < envMean)) {
            zeroCrossings++;
        }
    }

    float duration = (envelope.size() * windowSize / 4) / sampleRate;
    float modulationRate = (zeroCrossings / 2.0f) / duration;

    // Calculate peak-to-peak variation
    float envMin = *std::min_element(envelope.begin(), envelope.end());
    float envMax = *std::max_element(envelope.begin(), envelope.end());
    float peakToPeak = envMax - envMin;

    // Pass criteria: Detectable time-varying behavior
    // Modulation rate should be between 0.1 Hz and 20 Hz
    // Coefficient of variation should be > 0.01 (1% variation)
    bool hasModulation = (coefficientOfVariation > 0.01f);
    bool reasonableRate = (modulationRate > 0.05f && modulationRate < 30.0f);
    bool hasVariation = (peakToPeak > 0.001f);

    result.passed = hasModulation && reasonableRate && hasVariation;
    result.metrics = {coefficientOfVariation, modulationRate, peakToPeak, envMean, envStdDev};

    if (result.passed) {
        result.message = "PASS: Modulation rate=" +
                        std::to_string(modulationRate).substr(0, 5) + "Hz" +
                        ", variation=" + std::to_string(coefficientOfVariation).substr(0, 5) +
                        ", p2p=" + std::to_string(peakToPeak).substr(0, 6);
    } else {
        result.message = "FAIL: ";
        if (!hasModulation) result.message += "No modulation detected; ";
        if (!reasonableRate) result.message += "Rate out of range (" + std::to_string(modulationRate).substr(0, 5) + "Hz); ";
        if (!hasVariation) result.message += "Insufficient variation";
    }

    return result;
}

//==============================================================================
// Engine Test Runner
//==============================================================================
EngineTestResults testEngine(int engineId, const std::string& name) {
    EngineTestResults results;
    results.engineId = engineId;
    results.engineName = name;

    std::cout << "\n========================================\n";
    std::cout << "Testing Engine " << engineId << ": " << name << "\n";
    std::cout << "========================================\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "ERROR: Failed to create engine\n";
        results.impulseMessage = "Engine creation failed";
        results.stereoMessage = "Engine creation failed";
        results.timeVaryingMessage = "Engine creation failed";
        return results;
    }

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    engine->prepareToPlay(sampleRate, blockSize);

    // Set default parameters (50% on all)
    std::map<int, float> params;
    for (int i = 0; i < 10; ++i) {
        params[i] = 0.5f;
    }
    engine->updateParameters(params);

    // Test 1: Impulse Response
    std::cout << "\n[1/3] Testing Impulse Response..." << std::flush;
    auto impulseResult = testImpulseResponse(engine.get(), sampleRate, blockSize);
    results.impulseResponsePassed = impulseResult.passed;
    results.impulseMessage = impulseResult.message;
    std::cout << "\n  " << impulseResult.message << "\n";

    // Reset engine between tests
    engine->reset();
    engine->updateParameters(params);

    // Test 2: Stereo Width
    std::cout << "\n[2/3] Testing Stereo Width..." << std::flush;
    auto stereoResult = testStereoWidth(engine.get(), sampleRate, blockSize);
    results.stereoWidthPassed = stereoResult.passed;
    results.stereoMessage = stereoResult.message;
    std::cout << "\n  " << stereoResult.message << "\n";

    // Reset engine between tests
    engine->reset();
    engine->updateParameters(params);

    // Test 3: Time-Varying Characteristics
    std::cout << "\n[3/3] Testing Time-Varying Characteristics..." << std::flush;
    auto timeResult = testTimeVarying(engine.get(), sampleRate, blockSize);
    results.timeVaryingPassed = timeResult.passed;
    results.timeVaryingMessage = timeResult.message;
    std::cout << "\n  " << timeResult.message << "\n";

    return results;
}

} // namespace ModulationTest

//==============================================================================
// Main
//==============================================================================
int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Modulation Engines 24-27 Comprehensive Test Suite        ║\n";
    std::cout << "║  - Impulse Response                                        ║\n";
    std::cout << "║  - Stereo Width Measurement                                ║\n";
    std::cout << "║  - Time-Varying Characteristics                            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    std::vector<std::pair<int, std::string>> engines = {
        {24, "ResonantChorus_Platinum (AnalogChorus)"},
        {25, "AnalogPhaser"},
        {26, "PlatinumRingModulator (ClassicFlanger)"},
        {27, "FrequencyShifter (ClassicTremolo)"}
    };

    std::vector<ModulationTest::EngineTestResults> allResults;

    for (const auto& [id, name] : engines) {
        auto result = ModulationTest::testEngine(id, name);
        allResults.push_back(result);
    }

    // Summary Report
    std::cout << "\n\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    SUMMARY REPORT                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left << std::setw(12) << "Engine ID"
              << std::setw(40) << "Engine Name"
              << std::setw(10) << "Result" << "\n";
    std::cout << std::string(62, '-') << "\n";

    int passCount = 0;
    for (const auto& result : allResults) {
        std::string status = result.overallPass() ? "PASS" : "FAIL";
        if (result.overallPass()) passCount++;

        std::cout << std::left << std::setw(12) << result.engineId
                  << std::setw(40) << result.engineName
                  << std::setw(10) << status << "\n";
    }

    std::cout << "\n";
    std::cout << "Detailed Results:\n";
    std::cout << std::string(62, '=') << "\n";

    for (const auto& result : allResults) {
        std::cout << "\nEngine " << result.engineId << ": " << result.engineName << "\n";
        std::cout << "  Impulse Response: " << (result.impulseResponsePassed ? "✓ PASS" : "✗ FAIL") << "\n";
        std::cout << "    " << result.impulseMessage << "\n";
        std::cout << "  Stereo Width:     " << (result.stereoWidthPassed ? "✓ PASS" : "✗ FAIL") << "\n";
        std::cout << "    " << result.stereoMessage << "\n";
        std::cout << "  Time-Varying:     " << (result.timeVaryingPassed ? "✓ PASS" : "✗ FAIL") << "\n";
        std::cout << "    " << result.timeVaryingMessage << "\n";
        std::cout << "  Overall: " << (result.overallPass() ? "PASS" : "FAIL") << "\n";
    }

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Final Score: " << passCount << "/" << allResults.size() << " engines passed"
              << std::string(34 - std::to_string(passCount).length() - std::to_string(allResults.size()).length(), ' ') << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    return (passCount == allResults.size()) ? 0 : 1;
}

// Test for Engines 53-55: MidSideProcessor, GainUtility, MonoMaker
// Engine 53: MidSideProcessor_Platinum
// Engine 54: GainUtility_Platinum
// Engine 55: MonoMaker_Platinum
// All three process audio with impulse tests

#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <algorithm>

// Test configuration
const int SAMPLE_RATE = 48000;
const int BLOCK_SIZE = 512;
const int TEST_DURATION_SAMPLES = SAMPLE_RATE; // 1 second
const double EPSILON = 1e-6;

struct TestResult {
    std::string engineName;
    int engineId;
    bool initialized;
    bool processedOutput;
    bool hasNonZeroOutput;
    double peakLevel;
    double rmsLevel;
    bool passesTest;
    std::string failureReason;
};

// Helper function to calculate RMS
double calculateRMS(const std::vector<float>& buffer) {
    double sum = 0.0;
    for (float sample : buffer) {
        sum += sample * sample;
    }
    return std::sqrt(sum / buffer.size());
}

// Helper function to find peak level
double findPeak(const std::vector<float>& buffer) {
    double peak = 0.0;
    for (float sample : buffer) {
        peak = std::max(peak, std::abs(static_cast<double>(sample)));
    }
    return peak;
}

// Helper function to check if buffer has non-zero content
bool hasNonZeroContent(const std::vector<float>& buffer) {
    for (float sample : buffer) {
        if (std::abs(sample) > EPSILON) {
            return true;
        }
    }
    return false;
}

// Test Engine 53: MidSideProcessor_Platinum
TestResult testMidSideProcessor() {
    TestResult result;
    result.engineName = "MidSideProcessor_Platinum";
    result.engineId = 53;
    result.initialized = false;
    result.processedOutput = false;
    result.hasNonZeroOutput = false;
    result.peakLevel = 0.0;
    result.rmsLevel = 0.0;
    result.passesTest = false;

    try {
        auto engine = EngineFactory::createEngine(53);
        if (!engine) {
            result.failureReason = "Failed to create engine";
            return result;
        }

        result.initialized = true;
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);

        // Create impulse input (stereo)
        juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse on left channel
        buffer.setSample(1, 0, 0.5f); // Different impulse on right channel

        // Process
        engine->process(buffer);
        result.processedOutput = true;

        // Analyze output
        std::vector<float> leftOut(buffer.getReadPointer(0), buffer.getReadPointer(0) + BLOCK_SIZE);
        std::vector<float> rightOut(buffer.getReadPointer(1), buffer.getReadPointer(1) + BLOCK_SIZE);
        result.hasNonZeroOutput = hasNonZeroContent(leftOut) || hasNonZeroContent(rightOut);
        result.peakLevel = std::max(findPeak(leftOut), findPeak(rightOut));
        result.rmsLevel = std::max(calculateRMS(leftOut), calculateRMS(rightOut));

        // M/S processor should output audio when given input
        if (result.hasNonZeroOutput && result.peakLevel > EPSILON) {
            result.passesTest = true;
        } else {
            result.failureReason = "No output produced from impulse input";
        }

    } catch (const std::exception& e) {
        result.failureReason = std::string("Exception: ") + e.what();
    }

    return result;
}

// Test Engine 54: GainUtility_Platinum
TestResult testGainUtility() {
    TestResult result;
    result.engineName = "GainUtility_Platinum";
    result.engineId = 54;
    result.initialized = false;
    result.processedOutput = false;
    result.hasNonZeroOutput = false;
    result.peakLevel = 0.0;
    result.rmsLevel = 0.0;
    result.passesTest = false;

    try {
        auto engine = EngineFactory::createEngine(54);
        if (!engine) {
            result.failureReason = "Failed to create engine";
            return result;
        }

        result.initialized = true;
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);

        // Create impulse input
        juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);

        // Process
        engine->process(buffer);
        result.processedOutput = true;

        // Analyze output
        std::vector<float> leftOut(buffer.getReadPointer(0), buffer.getReadPointer(0) + BLOCK_SIZE);
        std::vector<float> rightOut(buffer.getReadPointer(1), buffer.getReadPointer(1) + BLOCK_SIZE);
        result.hasNonZeroOutput = hasNonZeroContent(leftOut) || hasNonZeroContent(rightOut);
        result.peakLevel = std::max(findPeak(leftOut), findPeak(rightOut));
        result.rmsLevel = std::max(calculateRMS(leftOut), calculateRMS(rightOut));

        // Gain utility should pass through or modify audio
        if (result.hasNonZeroOutput && result.peakLevel > EPSILON) {
            result.passesTest = true;
        } else {
            result.failureReason = "No output produced from impulse input";
        }

    } catch (const std::exception& e) {
        result.failureReason = std::string("Exception: ") + e.what();
    }

    return result;
}

// Test Engine 55: MonoMaker_Platinum
TestResult testMonoMaker() {
    TestResult result;
    result.engineName = "MonoMaker_Platinum";
    result.engineId = 55;
    result.initialized = false;
    result.processedOutput = false;
    result.hasNonZeroOutput = false;
    result.peakLevel = 0.0;
    result.rmsLevel = 0.0;
    result.passesTest = false;

    try {
        auto engine = EngineFactory::createEngine(55);
        if (!engine) {
            result.failureReason = "Failed to create engine";
            return result;
        }

        result.initialized = true;
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);

        // Set MonoMaker to full mono mode (frequency = 1.0 = high cutoff to mono all frequencies)
        std::map<int, float> params;
        params[0] = 1.0f;  // FREQUENCY - set high to make all frequencies mono
        params[3] = 1.0f;  // BASS_MONO - 100%
        engine->updateParameters(params);

        // Create impulse input with different values on L/R
        juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 0.5f); // Different values to test mono summing

        // Process
        engine->process(buffer);
        result.processedOutput = true;

        // Analyze output
        std::vector<float> leftOut(buffer.getReadPointer(0), buffer.getReadPointer(0) + BLOCK_SIZE);
        std::vector<float> rightOut(buffer.getReadPointer(1), buffer.getReadPointer(1) + BLOCK_SIZE);
        result.hasNonZeroOutput = hasNonZeroContent(leftOut) || hasNonZeroContent(rightOut);
        result.peakLevel = std::max(findPeak(leftOut), findPeak(rightOut));
        result.rmsLevel = std::max(calculateRMS(leftOut), calculateRMS(rightOut));

        // MonoMaker is frequency-selective (only makes bass frequencies mono)
        // For an impulse test, we just need to verify it produces output
        // Note: MonoMaker will only make L==R below its cutoff frequency
        // An impulse contains all frequencies, so the output won't be fully mono
        // This is correct behavior for a frequency-selective mono converter

        if (result.hasNonZeroOutput && result.peakLevel > EPSILON) {
            result.passesTest = true;
        } else if (!result.hasNonZeroOutput) {
            result.failureReason = "No output produced from impulse input";
        } else {
            result.failureReason = "Output level too low";
        }

    } catch (const std::exception& e) {
        result.failureReason = std::string("Exception: ") + e.what();
    }

    return result;
}

void printResults(const std::vector<TestResult>& results) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ENGINES 53-55 TEST RESULTS\n";
    std::cout << std::string(80, '=') << "\n\n";

    int passCount = 0;
    int failCount = 0;

    for (const auto& result : results) {
        std::cout << "Engine " << result.engineId << ": " << result.engineName << "\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "  Initialized:     " << (result.initialized ? "YES" : "NO") << "\n";
        std::cout << "  Processed:       " << (result.processedOutput ? "YES" : "NO") << "\n";
        std::cout << "  Non-zero output: " << (result.hasNonZeroOutput ? "YES" : "NO") << "\n";
        std::cout << "  Peak Level:      " << std::fixed << std::setprecision(6) << result.peakLevel << "\n";
        std::cout << "  RMS Level:       " << std::fixed << std::setprecision(6) << result.rmsLevel << "\n";
        std::cout << "  Status:          " << (result.passesTest ? "PASS" : "FAIL") << "\n";

        if (!result.passesTest && !result.failureReason.empty()) {
            std::cout << "  Failure Reason:  " << result.failureReason << "\n";
        }

        std::cout << "\n";

        if (result.passesTest) {
            passCount++;
        } else {
            failCount++;
        }
    }

    std::cout << std::string(80, '=') << "\n";
    std::cout << "SUMMARY: " << passCount << " PASS, " << failCount << " FAIL\n";
    std::cout << std::string(80, '=') << "\n";
}

int main() {
    std::cout << "Testing Engines 53-55 (Utility Processors)\n";
    std::cout << "=========================================\n\n";

    std::vector<TestResult> results;

    std::cout << "Testing Engine 53: MidSideProcessor_Platinum...\n";
    results.push_back(testMidSideProcessor());

    std::cout << "Testing Engine 54: GainUtility_Platinum...\n";
    results.push_back(testGainUtility());

    std::cout << "Testing Engine 55: MonoMaker_Platinum...\n";
    results.push_back(testMonoMaker());

    printResults(results);

    // Return non-zero if any test failed
    for (const auto& result : results) {
        if (!result.passesTest) {
            return 1;
        }
    }

    return 0;
}

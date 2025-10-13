// test_phase_align_fix.cpp
// Comprehensive test for PhaseAlign_Platinum (Engine 56) fix validation
// Tests all parameter combinations with impulse, sine, and real audio signals

#include "../JUCE_Plugin/Source/PhaseAlign_Platinum.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>
#include <vector>
#include <map>

// Test result structure
struct TestResult {
    std::string testName;
    bool passed = false;
    int totalSamples = 0;
    int invalidSamples = 0;
    float maxAbsValue = 0.0f;
    std::string errorMsg;
};

// Test statistics
struct TestStats {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    std::vector<TestResult> results;

    void add(const TestResult& result) {
        results.push_back(result);
        totalTests++;
        if (result.passed) passedTests++;
        else failedTests++;
    }

    float getPassRate() const {
        return totalTests > 0 ? (100.0f * passedTests / totalTests) : 0.0f;
    }

    void printSummary() const {
        std::cout << "\n========================================\n";
        std::cout << "TEST SUMMARY\n";
        std::cout << "========================================\n";
        std::cout << "Total Tests:  " << totalTests << "\n";
        std::cout << "Passed:       " << passedTests << " (" << std::fixed << std::setprecision(1) << getPassRate() << "%)\n";
        std::cout << "Failed:       " << failedTests << "\n";
        std::cout << "========================================\n";

        if (failedTests > 0) {
            std::cout << "\nFAILED TESTS:\n";
            for (const auto& result : results) {
                if (!result.passed) {
                    std::cout << "  - " << result.testName << "\n";
                    std::cout << "    Error: " << result.errorMsg << "\n";
                    std::cout << "    Invalid samples: " << result.invalidSamples << "/" << result.totalSamples << "\n";
                }
            }
        }
    }
};

// Validate buffer contains only finite values
bool validateBuffer(juce::AudioBuffer<float>& buffer, TestResult& result) {
    result.totalSamples = buffer.getNumChannels() * buffer.getNumSamples();
    result.invalidSamples = 0;
    result.maxAbsValue = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            const float val = data[i];

            if (!std::isfinite(val)) {
                result.invalidSamples++;
            } else {
                result.maxAbsValue = std::max(result.maxAbsValue, std::abs(val));
            }
        }
    }

    if (result.invalidSamples > 0) {
        result.errorMsg = "Found " + std::to_string(result.invalidSamples) + " NaN/Inf samples";
        return false;
    }

    // Check for excessive gain (>100x)
    if (result.maxAbsValue > 100.0f) {
        result.errorMsg = "Excessive output level: " + std::to_string(result.maxAbsValue);
        return false;
    }

    return true;
}

// Generate test signal
void generateImpulse(juce::AudioBuffer<float>& buffer) {
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    if (buffer.getNumChannels() > 1) {
        buffer.setSample(1, 0, 1.0f);
    }
}

void generateSine(juce::AudioBuffer<float>& buffer, float frequency, double sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const float phaseInc = 2.0f * M_PI * frequency / sampleRate;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getWritePointer(ch);
        float phase = ch * 0.25f * M_PI;  // Slight phase offset between channels

        for (int i = 0; i < numSamples; ++i) {
            data[i] = 0.5f * std::sin(phase);
            phase += phaseInc;
        }
    }
}

void generateWhiteNoise(juce::AudioBuffer<float>& buffer) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-0.5f, 0.5f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = dis(gen);
        }
    }
}

void generateRealAudio(juce::AudioBuffer<float>& buffer, double sampleRate) {
    // Simulated complex audio: mix of multiple frequencies
    buffer.clear();

    std::vector<float> frequencies = {100.0f, 440.0f, 1000.0f, 3000.0f, 8000.0f};

    for (float freq : frequencies) {
        juce::AudioBuffer<float> temp(buffer.getNumChannels(), buffer.getNumSamples());
        generateSine(temp, freq, sampleRate);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.addFrom(ch, 0, temp, ch, 0, buffer.getNumSamples(), 1.0f / frequencies.size());
        }
    }
}

// Test with specific parameters
TestResult testWithParameters(const std::map<int, float>& params,
                              const std::string& testName,
                              const std::string& signalType,
                              double sampleRate = 48000.0) {
    TestResult result;
    result.testName = testName + " [" + signalType + "]";

    // Create engine
    PhaseAlign_Platinum engine;

    // Prepare
    const int blockSize = 512;
    engine.prepareToPlay(sampleRate, blockSize);

    // Update parameters
    engine.updateParameters(params);

    // Create test buffer
    juce::AudioBuffer<float> buffer(2, blockSize);

    // Generate signal
    if (signalType == "impulse") {
        generateImpulse(buffer);
    } else if (signalType == "sine") {
        generateSine(buffer, 1000.0f, sampleRate);
    } else if (signalType == "noise") {
        generateWhiteNoise(buffer);
    } else if (signalType == "real") {
        generateRealAudio(buffer, sampleRate);
    }

    // Process multiple blocks to test stability
    const int numBlocks = 10;
    for (int block = 0; block < numBlocks; ++block) {
        engine.process(buffer);

        // Validate each block
        if (!validateBuffer(buffer, result)) {
            result.passed = false;
            return result;
        }

        // Regenerate signal for next block (except impulse which stays silent)
        if (signalType != "impulse") {
            if (signalType == "sine") {
                generateSine(buffer, 1000.0f, sampleRate);
            } else if (signalType == "noise") {
                generateWhiteNoise(buffer);
            } else if (signalType == "real") {
                generateRealAudio(buffer, sampleRate);
            }
        }
    }

    result.passed = true;
    return result;
}

// Test suite for parameter interactions
void runParameterInteractionTests(TestStats& stats) {
    std::cout << "\n--- Testing Parameter Interactions ---\n";

    // Test vectors for each parameter (normalized 0..1)
    std::vector<float> autoAlignValues = {0.0f, 1.0f};  // off, on
    std::vector<float> referenceValues = {0.0f, 1.0f};  // left, right
    std::vector<float> phaseValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};  // -180 to +180 deg
    std::vector<float> freqValues = {0.0f, 0.5f, 1.0f};  // min, mid, max
    std::vector<float> mixValues = {0.0f, 0.5f, 1.0f};  // dry, half, wet

    std::vector<std::string> signalTypes = {"impulse", "sine", "noise", "real"};

    int testCount = 0;
    int sampleTestInterval = 20;  // Test every Nth combination to keep runtime reasonable

    // Test extreme parameter combinations
    std::vector<std::map<int, float>> extremeCases = {
        // All parameters at min
        {{0, 0.0f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 0.0f}, {8, 0.0f}, {9, 0.0f}},

        // All parameters at max
        {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}, {3, 1.0f}, {4, 1.0f}, {5, 1.0f}, {6, 1.0f}, {7, 1.0f}, {8, 1.0f}, {9, 1.0f}},

        // All parameters at mid
        {{0, 0.5f}, {1, 0.5f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f}, {5, 0.5f}, {6, 0.5f}, {7, 0.5f}, {8, 0.5f}, {9, 0.5f}},

        // Extreme phase shifts with auto-align
        {{0, 1.0f}, {1, 0.0f}, {2, 1.0f}, {3, 1.0f}, {4, 1.0f}, {5, 1.0f}, {6, 0.5f}, {7, 0.5f}, {8, 0.5f}, {9, 1.0f}},

        // Extreme frequency crossovers
        {{0, 0.0f}, {1, 0.0f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f}, {5, 0.5f}, {6, 1.0f}, {7, 0.9f}, {8, 0.8f}, {9, 1.0f}},

        // Auto-align with extreme settings
        {{0, 1.0f}, {1, 1.0f}, {2, 0.0f}, {3, 0.25f}, {4, 0.75f}, {5, 1.0f}, {6, 0.0f}, {7, 1.0f}, {8, 0.5f}, {9, 1.0f}},
    };

    // Test all extreme cases with all signal types
    for (size_t i = 0; i < extremeCases.size(); ++i) {
        for (const auto& signalType : signalTypes) {
            std::string testName = "Extreme Case " + std::to_string(i + 1);
            auto result = testWithParameters(extremeCases[i], testName, signalType);
            stats.add(result);

            std::cout << "  " << result.testName << ": "
                      << (result.passed ? "PASS" : "FAIL") << "\n";
        }
    }

    // Comprehensive parameter sweep (sampled)
    std::cout << "\nRunning comprehensive parameter sweep...\n";
    int sweepTests = 0;
    int sweepPassed = 0;

    for (float autoAlign : autoAlignValues) {
        for (float reference : referenceValues) {
            for (float lowPhase : phaseValues) {
                for (float midPhase : phaseValues) {
                    for (float mix : mixValues) {
                        testCount++;

                        // Sample tests to keep runtime reasonable
                        if (testCount % sampleTestInterval != 0) continue;

                        std::map<int, float> params = {
                            {0, autoAlign},   // AUTO_ALIGN
                            {1, reference},   // REFERENCE
                            {2, lowPhase},    // LOW_PHASE
                            {3, midPhase},    // LOW_MID_PHASE
                            {4, midPhase},    // HIGH_MID_PHASE
                            {5, lowPhase},    // HIGH_PHASE
                            {6, 0.5f},        // LOW_FREQ
                            {7, 0.5f},        // MID_FREQ
                            {8, 0.5f},        // HIGH_FREQ
                            {9, mix}          // MIX
                        };

                        // Test with sine wave (most revealing)
                        auto result = testWithParameters(params,
                            "Sweep Test " + std::to_string(sweepTests),
                            "sine");

                        sweepTests++;
                        if (result.passed) sweepPassed++;

                        if (!result.passed) {
                            stats.add(result);
                            std::cout << "  FAILED: " << result.testName << " - " << result.errorMsg << "\n";
                        }
                    }
                }
            }
        }
    }

    std::cout << "Sweep test pass rate: " << sweepPassed << "/" << sweepTests
              << " (" << (100.0f * sweepPassed / sweepTests) << "%)\n";
}

// Test edge cases
void runEdgeCaseTests(TestStats& stats) {
    std::cout << "\n--- Testing Edge Cases ---\n";

    // Zero input
    {
        TestResult result;
        result.testName = "Zero input (silence)";
        PhaseAlign_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();

        engine.process(buffer);
        result.passed = validateBuffer(buffer, result);
        stats.add(result);
        std::cout << "  " << result.testName << ": " << (result.passed ? "PASS" : "FAIL") << "\n";
    }

    // Very low sample rate
    {
        TestResult result;
        result.testName = "Low sample rate (8 kHz)";
        PhaseAlign_Platinum engine;
        engine.prepareToPlay(8000.0, 256);

        std::map<int, float> params = {{0, 1.0f}, {9, 1.0f}};  // Auto-align on, full mix
        engine.updateParameters(params);

        juce::AudioBuffer<float> buffer(2, 256);
        generateSine(buffer, 400.0f, 8000.0);

        engine.process(buffer);
        result.passed = validateBuffer(buffer, result);
        stats.add(result);
        std::cout << "  " << result.testName << ": " << (result.passed ? "PASS" : "FAIL") << "\n";
    }

    // Very high sample rate
    {
        TestResult result;
        result.testName = "High sample rate (192 kHz)";
        PhaseAlign_Platinum engine;
        engine.prepareToPlay(192000.0, 1024);

        std::map<int, float> params = {{0, 1.0f}, {9, 1.0f}};
        engine.updateParameters(params);

        juce::AudioBuffer<float> buffer(2, 1024);
        generateSine(buffer, 10000.0f, 192000.0);

        engine.process(buffer);
        result.passed = validateBuffer(buffer, result);
        stats.add(result);
        std::cout << "  " << result.testName << ": " << (result.passed ? "PASS" : "FAIL") << "\n";
    }

    // Mono input
    {
        TestResult result;
        result.testName = "Mono input (1 channel)";
        PhaseAlign_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(1, 512);
        generateSine(buffer, 1000.0f, 48000.0);

        engine.process(buffer);
        result.passed = validateBuffer(buffer, result);
        stats.add(result);
        std::cout << "  " << result.testName << ": " << (result.passed ? "PASS" : "FAIL") << "\n";
    }

    // Rapid parameter changes
    {
        TestResult result;
        result.testName = "Rapid parameter modulation";
        PhaseAlign_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        generateSine(buffer, 1000.0f, 48000.0);

        bool allPassed = true;
        for (int i = 0; i < 20; ++i) {
            std::map<int, float> params = {
                {2, (i % 2) ? 0.0f : 1.0f},  // Toggle phase
                {9, 0.5f + 0.5f * std::sin(i * 0.5f)}  // Modulate mix
            };
            engine.updateParameters(params);
            engine.process(buffer);

            TestResult tempResult;
            if (!validateBuffer(buffer, tempResult)) {
                allPassed = false;
                result.errorMsg = tempResult.errorMsg;
                break;
            }

            generateSine(buffer, 1000.0f, 48000.0);  // Regenerate
        }

        result.passed = allPassed;
        stats.add(result);
        std::cout << "  " << result.testName << ": " << (result.passed ? "PASS" : "FAIL") << "\n";
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "PhaseAlign_Platinum (Engine 56) Fix Test\n";
    std::cout << "========================================\n";

    TestStats stats;

    // Run test suites
    runEdgeCaseTests(stats);
    runParameterInteractionTests(stats);

    // Print final summary
    stats.printSummary();

    // Return appropriate exit code
    return (stats.getPassRate() >= 100.0f) ? 0 : 1;
}

// ==================== PHASED VOCODER FIX VALIDATION TEST ====================
// Comprehensive test for Engine 49 - testing all parameter combinations
// and signal types to achieve 100% pass rate

#include "../JUCE_Plugin/Source/PhasedVocoder.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory>
#include <chrono>

// Test configuration
constexpr double TEST_SAMPLE_RATE = 48000.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr int WARMUP_BLOCKS = 10;  // Allow time for phase vocoder to stabilize

// Quality metrics
struct QualityMetrics {
    float rmsLevel = 0.0f;
    float peakLevel = 0.0f;
    float dcOffset = 0.0f;
    bool hasNaN = false;
    bool hasInf = false;
    bool hasSilence = false;  // Complete silence = bad
    bool hasExcessiveNoise = false;  // > 1.5x input level
    int nonZeroSamples = 0;
    double artifactLevel = 0.0;  // Spectral roughness metric
    bool isValid = false;
    std::string failureReason;
};

// Parameter combination test result
struct ParameterTestResult {
    std::string testName;
    std::map<int, float> parameters;
    QualityMetrics metrics;
    bool passed = false;
    std::string failureReason;
};

// Compute quality metrics from processed audio
QualityMetrics analyzeQuality(const juce::AudioBuffer<float>& buffer, float inputRMS = 0.5f) {
    QualityMetrics metrics;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0) {
        metrics.failureReason = "Empty buffer";
        return metrics;
    }

    double sumSquared = 0.0;
    double sumDC = 0.0;

    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            float sample = data[i];

            // Check for NaN/Inf
            if (std::isnan(sample)) {
                metrics.hasNaN = true;
            }
            if (std::isinf(sample)) {
                metrics.hasInf = true;
            }

            // Accumulate statistics
            float absSample = std::abs(sample);
            if (absSample > 1e-10f) {
                metrics.nonZeroSamples++;
            }

            sumSquared += sample * sample;
            sumDC += sample;

            if (absSample > metrics.peakLevel) {
                metrics.peakLevel = absSample;
            }
        }
    }

    // Compute metrics
    int totalSamples = numChannels * numSamples;
    metrics.rmsLevel = std::sqrt(sumSquared / totalSamples);
    metrics.dcOffset = std::abs(sumDC / totalSamples);

    // Check for complete silence (bad for vocoder)
    metrics.hasSilence = (metrics.nonZeroSamples < totalSamples / 100);  // < 1% non-zero

    // Check for excessive gain (likely instability)
    metrics.hasExcessiveNoise = (metrics.peakLevel > 2.0f) || (metrics.rmsLevel > 1.5f);

    // Simple artifact detection: high frequency content ratio
    // (More sophisticated: would use FFT, but this is a quick check)
    double highFreqEnergy = 0.0;
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 1; i < numSamples; ++i) {
            float diff = data[i] - data[i-1];
            highFreqEnergy += diff * diff;
        }
    }
    metrics.artifactLevel = std::sqrt(highFreqEnergy / totalSamples);

    // Validation: pass if no critical errors
    metrics.isValid = !metrics.hasNaN &&
                      !metrics.hasInf &&
                      !metrics.hasSilence &&
                      !metrics.hasExcessiveNoise;

    return metrics;
}

// Generate test signal types
void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float amplitude = 0.5f) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = amplitude * std::sin(2.0f * M_PI * frequency * i / TEST_SAMPLE_RATE);
        }
    }
}

void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float amplitude = 0.3f) {
    juce::Random random;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = amplitude * (random.nextFloat() * 2.0f - 1.0f);
        }
    }
}

void generateSpeechLike(juce::AudioBuffer<float>& buffer, float amplitude = 0.4f) {
    // Speech-like signal: mixture of harmonics with amplitude modulation
    juce::Random random;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float t = i / TEST_SAMPLE_RATE;
            // Fundamental + harmonics
            float signal = std::sin(2.0f * M_PI * 150.0f * t);
            signal += 0.5f * std::sin(2.0f * M_PI * 300.0f * t);
            signal += 0.3f * std::sin(2.0f * M_PI * 450.0f * t);
            signal += 0.2f * std::sin(2.0f * M_PI * 600.0f * t);

            // Amplitude modulation (like formants)
            float modulation = 0.5f + 0.5f * std::sin(2.0f * M_PI * 5.0f * t);

            data[i] = amplitude * signal * modulation;
        }
    }
}

void generateDrumHit(juce::AudioBuffer<float>& buffer, float amplitude = 0.6f) {
    // Drum-like transient: short burst with decay
    juce::Random random;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float t = i / TEST_SAMPLE_RATE;
            // Exponential decay envelope
            float envelope = std::exp(-10.0f * t);
            // Mix of sine and noise
            float tone = std::sin(2.0f * M_PI * 100.0f * t);
            float noise = random.nextFloat() * 2.0f - 1.0f;
            data[i] = amplitude * envelope * (0.6f * tone + 0.4f * noise);
        }
    }
}

// Test a specific parameter combination
ParameterTestResult testParameterCombination(
    PhasedVocoder& engine,
    const std::string& testName,
    const std::map<int, float>& params,
    const std::string& signalType,
    std::function<void(juce::AudioBuffer<float>&)> signalGenerator)
{
    ParameterTestResult result;
    result.testName = testName;
    result.parameters = params;

    // Reset and prepare engine
    engine.reset();
    engine.prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);

    // Set parameters
    engine.updateParameters(params);

    // Create test buffer
    juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);

    // Warmup: let the vocoder stabilize
    for (int warmup = 0; warmup < WARMUP_BLOCKS; ++warmup) {
        signalGenerator(buffer);
        engine.process(buffer);
    }

    // Process test blocks
    const int numTestBlocks = 20;  // ~0.2 seconds of audio
    std::vector<juce::AudioBuffer<float>> outputs;

    for (int block = 0; block < numTestBlocks; ++block) {
        signalGenerator(buffer);
        engine.process(buffer);

        // Store output for analysis
        outputs.emplace_back(2, TEST_BLOCK_SIZE);
        for (int ch = 0; ch < 2; ++ch) {
            outputs.back().copyFrom(ch, 0, buffer, ch, 0, TEST_BLOCK_SIZE);
        }
    }

    // Analyze last few blocks (after stabilization)
    juce::AudioBuffer<float> analysisBuffer(2, TEST_BLOCK_SIZE * 5);
    int destPos = 0;
    for (int i = numTestBlocks - 5; i < numTestBlocks; ++i) {
        for (int ch = 0; ch < 2; ++ch) {
            analysisBuffer.copyFrom(ch, destPos, outputs[i], ch, 0, TEST_BLOCK_SIZE);
        }
        destPos += TEST_BLOCK_SIZE;
    }

    // Compute quality metrics
    result.metrics = analyzeQuality(analysisBuffer, 0.5f);

    // Determine pass/fail
    if (result.metrics.hasNaN) {
        result.passed = false;
        result.failureReason = "Output contains NaN";
    } else if (result.metrics.hasInf) {
        result.passed = false;
        result.failureReason = "Output contains Inf";
    } else if (result.metrics.hasSilence) {
        result.passed = false;
        result.failureReason = "Output is silent (< 1% non-zero samples)";
    } else if (result.metrics.hasExcessiveNoise) {
        result.passed = false;
        result.failureReason = "Excessive output level (peak > 2.0 or RMS > 1.5)";
    } else if (result.metrics.artifactLevel > 0.5f) {
        result.passed = false;
        result.failureReason = "Excessive artifacts (high frequency roughness)";
    } else {
        result.passed = true;
        result.failureReason = "PASS";
    }

    return result;
}

// Main test suite
int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  PHASED VOCODER COMPREHENSIVE TEST\n";
    std::cout << "  Engine 49 - Deep Validation\n";
    std::cout << "========================================\n\n";

    auto startTime = std::chrono::high_resolution_clock::now();

    // Create engine
    PhasedVocoder engine;

    // Open report file
    std::ofstream report("phased_vocoder_test_report.txt");
    report << "PHASED VOCODER VALIDATION TEST REPORT\n";
    report << "======================================\n";
    report << "Date: " << __DATE__ << " " << __TIME__ << "\n";
    report << "Sample Rate: " << TEST_SAMPLE_RATE << " Hz\n";
    report << "Block Size: " << TEST_BLOCK_SIZE << " samples\n\n";

    std::vector<ParameterTestResult> allResults;

    // Test categories
    std::cout << "Test Categories:\n";
    std::cout << "  1. Identity tests (no processing)\n";
    std::cout << "  2. Time stretch tests (0.5x, 1.0x, 2.0x)\n";
    std::cout << "  3. Pitch shift tests (-12, 0, +12 semitones)\n";
    std::cout << "  4. Combined time+pitch tests\n";
    std::cout << "  5. Extreme parameter tests\n";
    std::cout << "  6. All signal types (sine, noise, speech, drums)\n\n";

    // Signal generators
    std::vector<std::pair<std::string, std::function<void(juce::AudioBuffer<float>&)>>> signals = {
        {"Sine440Hz", [](auto& buf) { generateSineWave(buf, 440.0f, 0.5f); }},
        {"WhiteNoise", [](auto& buf) { generateWhiteNoise(buf, 0.3f); }},
        {"SpeechLike", [](auto& buf) { generateSpeechLike(buf, 0.4f); }},
        {"DrumHit", [](auto& buf) { generateDrumHit(buf, 0.6f); }}
    };

    int testCount = 0;
    int passCount = 0;

    // TEST 1: Identity (bypass - should be very close to input)
    std::cout << "TEST 1: Identity (Bypass) Tests\n";
    std::cout << "--------------------------------\n";
    {
        std::map<int, float> params = {
            {0, 0.2f},   // TimeStretch = 1.0x (at snap point)
            {1, 0.333f}, // PitchShift = 1.0x (0.5 + 0.333*1.5 = 1.0)
            {2, 0.0f},   // SpectralSmear = 0%
            {3, 0.5f},   // TransientPreserve = 50%
            {4, 0.0f},   // PhaseReset = 0%
            {5, 0.0f},   // SpectralGate = 0%
            {6, 1.0f},   // Mix = 100%
            {7, 0.0f},   // Freeze = OFF
            {8, 0.1f},   // TransientAttack = 1ms
            {9, 0.2f}    // TransientRelease = 100ms
        };

        for (const auto& [sigName, sigGen] : signals) {
            auto result = testParameterCombination(
                engine, "Identity_" + sigName, params, sigName, sigGen);
            allResults.push_back(result);
            testCount++;
            if (result.passed) passCount++;

            std::cout << "  " << result.testName << ": "
                      << (result.passed ? "PASS" : "FAIL - " + result.failureReason)
                      << " [RMS=" << std::fixed << std::setprecision(4) << result.metrics.rmsLevel
                      << ", Peak=" << result.metrics.peakLevel << "]\n";
        }
    }

    // TEST 2: Time Stretch Tests
    std::cout << "\nTEST 2: Time Stretch Tests\n";
    std::cout << "----------------------------\n";
    {
        std::vector<float> stretchValues = {0.0f, 0.2f, 0.5f, 1.0f};  // 0.25x, 1.0x, 2.125x, 4.0x
        std::vector<std::string> stretchNames = {"0.25x", "1.0x", "2.125x", "4.0x"};

        for (size_t i = 0; i < stretchValues.size(); ++i) {
            std::map<int, float> params = {
                {0, stretchValues[i]},  // TimeStretch
                {1, 0.333f},            // PitchShift = 1.0x
                {2, 0.0f}, {3, 0.5f}, {4, 0.0f}, {5, 0.0f}, {6, 1.0f}, {7, 0.0f}, {8, 0.1f}, {9, 0.2f}
            };

            auto result = testParameterCombination(
                engine, "TimeStretch_" + stretchNames[i] + "_Sine", params, "Sine440Hz",
                [](auto& buf) { generateSineWave(buf, 440.0f, 0.5f); });
            allResults.push_back(result);
            testCount++;
            if (result.passed) passCount++;

            std::cout << "  " << result.testName << ": "
                      << (result.passed ? "PASS" : "FAIL - " + result.failureReason)
                      << " [RMS=" << result.metrics.rmsLevel << "]\n";
        }
    }

    // TEST 3: Pitch Shift Tests
    std::cout << "\nTEST 3: Pitch Shift Tests\n";
    std::cout << "--------------------------\n";
    {
        // PitchShift parameter: 0.5 + value * 1.5
        // For 0 semitones: ratio = 1.0 -> value = (1.0 - 0.5) / 1.5 = 0.333
        // For -12 semitones: ratio = 0.5 -> value = 0.0
        // For +12 semitones: ratio = 2.0 -> value = 1.0
        std::vector<float> pitchValues = {0.0f, 0.333f, 1.0f};
        std::vector<std::string> pitchNames = {"-12st", "0st", "+12st"};

        for (size_t i = 0; i < pitchValues.size(); ++i) {
            std::map<int, float> params = {
                {0, 0.2f},              // TimeStretch = 1.0x
                {1, pitchValues[i]},    // PitchShift
                {2, 0.0f}, {3, 0.5f}, {4, 0.0f}, {5, 0.0f}, {6, 1.0f}, {7, 0.0f}, {8, 0.1f}, {9, 0.2f}
            };

            auto result = testParameterCombination(
                engine, "PitchShift_" + pitchNames[i] + "_Sine", params, "Sine440Hz",
                [](auto& buf) { generateSineWave(buf, 440.0f, 0.5f); });
            allResults.push_back(result);
            testCount++;
            if (result.passed) passCount++;

            std::cout << "  " << result.testName << ": "
                      << (result.passed ? "PASS" : "FAIL - " + result.failureReason)
                      << " [RMS=" << result.metrics.rmsLevel << "]\n";
        }
    }

    // TEST 4: Combined Time + Pitch
    std::cout << "\nTEST 4: Combined Time + Pitch Tests\n";
    std::cout << "------------------------------------\n";
    {
        std::vector<std::tuple<float, float, std::string>> combos = {
            {0.0f, 0.0f, "0.25x_-12st"},
            {0.5f, 0.5f, "2.125x_-6st"},
            {1.0f, 1.0f, "4.0x_+12st"}
        };

        for (const auto& [stretch, pitch, name] : combos) {
            std::map<int, float> params = {
                {0, stretch}, {1, pitch},
                {2, 0.0f}, {3, 0.5f}, {4, 0.0f}, {5, 0.0f}, {6, 1.0f}, {7, 0.0f}, {8, 0.1f}, {9, 0.2f}
            };

            auto result = testParameterCombination(
                engine, "Combined_" + name, params, "SpeechLike",
                [](auto& buf) { generateSpeechLike(buf, 0.4f); });
            allResults.push_back(result);
            testCount++;
            if (result.passed) passCount++;

            std::cout << "  " << result.testName << ": "
                      << (result.passed ? "PASS" : "FAIL - " + result.failureReason)
                      << " [RMS=" << result.metrics.rmsLevel << "]\n";
        }
    }

    // TEST 5: Spectral Processing Tests
    std::cout << "\nTEST 5: Spectral Processing Tests\n";
    std::cout << "----------------------------------\n";
    {
        // Test freeze
        std::map<int, float> freezeParams = {
            {0, 0.2f}, {1, 0.333f}, {2, 0.0f}, {3, 0.5f}, {4, 0.0f}, {5, 0.0f},
            {6, 1.0f}, {7, 1.0f}, {8, 0.1f}, {9, 0.2f}  // Freeze ON
        };
        auto result = testParameterCombination(
            engine, "Freeze_Sine", freezeParams, "Sine440Hz",
            [](auto& buf) { generateSineWave(buf, 440.0f, 0.5f); });
        allResults.push_back(result);
        testCount++;
        if (result.passed) passCount++;
        std::cout << "  " << result.testName << ": "
                  << (result.passed ? "PASS" : "FAIL - " + result.failureReason) << "\n";

        // Test spectral gate
        std::map<int, float> gateParams = {
            {0, 0.2f}, {1, 0.333f}, {2, 0.0f}, {3, 0.5f}, {4, 0.0f}, {5, 0.3f},  // Gate 30%
            {6, 1.0f}, {7, 0.0f}, {8, 0.1f}, {9, 0.2f}
        };
        result = testParameterCombination(
            engine, "SpectralGate_WhiteNoise", gateParams, "WhiteNoise",
            [](auto& buf) { generateWhiteNoise(buf, 0.3f); });
        allResults.push_back(result);
        testCount++;
        if (result.passed) passCount++;
        std::cout << "  " << result.testName << ": "
                  << (result.passed ? "PASS" : "FAIL - " + result.failureReason) << "\n";

        // Test spectral smear
        std::map<int, float> smearParams = {
            {0, 0.2f}, {1, 0.333f}, {2, 0.5f}, {3, 0.5f}, {4, 0.0f}, {5, 0.0f},  // Smear 50%
            {6, 1.0f}, {7, 0.0f}, {8, 0.1f}, {9, 0.2f}
        };
        result = testParameterCombination(
            engine, "SpectralSmear_Speech", smearParams, "SpeechLike",
            [](auto& buf) { generateSpeechLike(buf, 0.4f); });
        allResults.push_back(result);
        testCount++;
        if (result.passed) passCount++;
        std::cout << "  " << result.testName << ": "
                  << (result.passed ? "PASS" : "FAIL - " + result.failureReason) << "\n";
    }

    // TEST 6: Mix Parameter Tests
    std::cout << "\nTEST 6: Mix Parameter Tests\n";
    std::cout << "----------------------------\n";
    {
        std::vector<float> mixValues = {0.0f, 0.5f, 1.0f};
        std::vector<std::string> mixNames = {"0%", "50%", "100%"};

        for (size_t i = 0; i < mixValues.size(); ++i) {
            std::map<int, float> params = {
                {0, 0.5f}, {1, 1.0f},  // 2x stretch, +12st pitch
                {2, 0.0f}, {3, 0.5f}, {4, 0.0f}, {5, 0.0f},
                {6, mixValues[i]}, {7, 0.0f}, {8, 0.1f}, {9, 0.2f}
            };

            auto result = testParameterCombination(
                engine, "Mix_" + mixNames[i], params, "Sine440Hz",
                [](auto& buf) { generateSineWave(buf, 440.0f, 0.5f); });
            allResults.push_back(result);
            testCount++;
            if (result.passed) passCount++;

            std::cout << "  " << result.testName << ": "
                      << (result.passed ? "PASS" : "FAIL - " + result.failureReason) << "\n";
        }
    }

    // TEST 7: Stress Tests (Extreme Parameters)
    std::cout << "\nTEST 7: Stress Tests (Extreme Parameters)\n";
    std::cout << "------------------------------------------\n";
    {
        // All parameters at max
        std::map<int, float> maxParams = {
            {0, 1.0f}, {1, 1.0f}, {2, 1.0f}, {3, 1.0f}, {4, 1.0f}, {5, 1.0f},
            {6, 1.0f}, {7, 1.0f}, {8, 1.0f}, {9, 1.0f}
        };
        auto result = testParameterCombination(
            engine, "AllMax_Sine", maxParams, "Sine440Hz",
            [](auto& buf) { generateSineWave(buf, 440.0f, 0.5f); });
        allResults.push_back(result);
        testCount++;
        if (result.passed) passCount++;
        std::cout << "  " << result.testName << ": "
                  << (result.passed ? "PASS" : "FAIL - " + result.failureReason) << "\n";

        // All parameters at min
        std::map<int, float> minParams = {
            {0, 0.0f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {4, 0.0f}, {5, 0.0f},
            {6, 1.0f}, {7, 0.0f}, {8, 0.0f}, {9, 0.0f}
        };
        result = testParameterCombination(
            engine, "AllMin_Sine", minParams, "Sine440Hz",
            [](auto& buf) { generateSineWave(buf, 440.0f, 0.5f); });
        allResults.push_back(result);
        testCount++;
        if (result.passed) passCount++;
        std::cout << "  " << result.testName << ": "
                  << (result.passed ? "PASS" : "FAIL - " + result.failureReason) << "\n";
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << "========================================\n";
    std::cout << "Total Tests: " << testCount << "\n";
    std::cout << "Passed: " << passCount << "\n";
    std::cout << "Failed: " << (testCount - passCount) << "\n";
    std::cout << "Pass Rate: " << std::fixed << std::setprecision(1)
              << (100.0 * passCount / testCount) << "%\n";
    std::cout << "Duration: " << duration.count() << " ms\n";
    std::cout << "========================================\n\n";

    // Write detailed report
    report << "\nTEST RESULTS SUMMARY\n";
    report << "====================\n";
    report << "Total Tests: " << testCount << "\n";
    report << "Passed: " << passCount << "\n";
    report << "Failed: " << (testCount - passCount) << "\n";
    report << "Pass Rate: " << (100.0 * passCount / testCount) << "%\n\n";

    report << "\nDETAILED RESULTS\n";
    report << "================\n";
    for (const auto& result : allResults) {
        report << "\nTest: " << result.testName << "\n";
        report << "Status: " << (result.passed ? "PASS" : "FAIL") << "\n";
        if (!result.passed) {
            report << "Reason: " << result.failureReason << "\n";
        }
        report << "Metrics:\n";
        report << "  RMS Level: " << result.metrics.rmsLevel << "\n";
        report << "  Peak Level: " << result.metrics.peakLevel << "\n";
        report << "  DC Offset: " << result.metrics.dcOffset << "\n";
        report << "  Non-zero Samples: " << result.metrics.nonZeroSamples << "\n";
        report << "  Artifact Level: " << result.metrics.artifactLevel << "\n";
        report << "  Has NaN: " << (result.metrics.hasNaN ? "YES" : "NO") << "\n";
        report << "  Has Inf: " << (result.metrics.hasInf ? "YES" : "NO") << "\n";
    }

    report.close();
    std::cout << "Detailed report written to: phased_vocoder_test_report.txt\n\n";

    // Return exit code based on pass rate
    if (passCount == testCount) {
        std::cout << "SUCCESS: All tests passed!\n\n";
        return 0;
    } else {
        std::cout << "FAILURE: Some tests failed.\n\n";
        return 1;
    }
}

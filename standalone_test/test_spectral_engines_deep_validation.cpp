#include <iostream>
#include <vector>
#include <cmath>
#include <map>
#include <string>
#include <iomanip>
#include <fstream>
#include "../JUCE_Plugin/Source/SpectralFreeze.h"
#include "../JUCE_Plugin/Source/SpectralGate_Platinum.h"
#include "../JUCE_Plugin/Source/FeedbackNetwork.h"
#include "../JUCE_Plugin/Source/ChaosGenerator.h"

// Test results structure
struct TestResult {
    std::string testName;
    bool passed;
    std::string details;
    double measuredValue;
    double expectedValue;
    double tolerance;
};

std::vector<TestResult> allResults;

// Utility functions
double calculateRMS(const juce::AudioBuffer<float>& buffer) {
    double sumSquares = 0.0;
    int totalSamples = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sumSquares += data[i] * data[i];
            totalSamples++;
        }
    }
    return std::sqrt(sumSquares / totalSamples);
}

double calculatePeak(const juce::AudioBuffer<float>& buffer) {
    double peak = 0.0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            peak = std::max(peak, static_cast<double>(std::abs(data[i])));
        }
    }
    return peak;
}

bool containsNaN(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (!std::isfinite(data[i])) return true;
        }
    }
    return false;
}

int countNonZeroSamples(const juce::AudioBuffer<float>& buffer, float threshold = 0.0001f) {
    int count = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::abs(data[i]) > threshold) count++;
        }
    }
    return count;
}

void generateImpulse(juce::AudioBuffer<float>& buffer) {
    buffer.clear();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.setSample(ch, 0, 1.0f);
    }
}

void generateSine(juce::AudioBuffer<float>& buffer, double frequency, double sampleRate, double amplitude = 0.5) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = amplitude * std::sin(2.0 * M_PI * frequency * i / sampleRate);
        }
    }
}

// ============================================================================
// SPECTRAL FREEZE TESTS
// ============================================================================

void testSpectralFreezeFFTSizes() {
    std::cout << "\n=== SpectralFreeze: FFT Size Tests ===\n";

    SpectralFreeze engine;
    const double sampleRate = 44100.0;
    const int bufferSize = 512;

    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    // Test with impulse to verify FFT processing
    juce::AudioBuffer<float> buffer(2, bufferSize);
    generateImpulse(buffer);

    std::map<int, float> params;
    params[0] = 0.0f;  // Freeze off initially
    params[1] = 0.0f;  // Smear
    params[2] = 0.5f;  // Shift (centered)
    params[3] = 0.0f;  // Resonance
    params[4] = 1.0f;  // Decay (full)
    params[5] = 0.5f;  // Brightness (centered)
    params[6] = 1.0f;  // Density (full)
    params[7] = 0.0f;  // Shimmer
    engine.updateParameters(params);

    // Process multiple blocks to fill FFT buffer
    for (int i = 0; i < 10; ++i) {
        engine.process(buffer);
        if (i == 0) buffer.clear(); // Only impulse in first block
    }

    double rms = calculateRMS(buffer);
    double peak = calculatePeak(buffer);
    bool hasNaN = containsNaN(buffer);

    TestResult result;
    result.testName = "SpectralFreeze FFT Processing";
    result.passed = (rms > 0.0001 && !hasNaN && peak < 10.0);
    result.measuredValue = rms;
    result.expectedValue = 0.01;
    result.tolerance = 0.5;
    result.details = "RMS: " + std::to_string(rms) + ", Peak: " + std::to_string(peak) +
                    ", NaN: " + (hasNaN ? "YES" : "NO");
    allResults.push_back(result);

    std::cout << (result.passed ? "✓ PASS" : "✗ FAIL") << ": " << result.details << "\n";
}

void testSpectralFreezeParameters() {
    std::cout << "\n=== SpectralFreeze: Parameter Sweep Tests ===\n";

    SpectralFreeze engine;
    const double sampleRate = 44100.0;
    const int bufferSize = 512;

    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    // Test each parameter independently
    std::vector<std::pair<int, std::string>> params = {
        {0, "Freeze"}, {1, "Smear"}, {2, "Shift"}, {3, "Resonance"},
        {4, "Decay"}, {5, "Brightness"}, {6, "Density"}, {7, "Shimmer"}
    };

    for (auto& [paramIdx, paramName] : params) {
        juce::AudioBuffer<float> buffer(2, bufferSize);
        generateSine(buffer, 1000.0, sampleRate);

        // Test parameter at 0%, 50%, 100%
        for (float value : {0.0f, 0.5f, 1.0f}) {
            std::map<int, float> testParams;
            // Set all to neutral
            testParams[0] = 0.0f;  // Freeze off
            testParams[1] = 0.0f;
            testParams[2] = 0.5f;  // Shift centered
            testParams[3] = 0.0f;
            testParams[4] = 1.0f;  // Full decay
            testParams[5] = 0.5f;  // Brightness centered
            testParams[6] = 1.0f;  // Full density
            testParams[7] = 0.0f;

            // Set test parameter
            testParams[paramIdx] = value;
            engine.updateParameters(testParams);

            // Process
            for (int i = 0; i < 5; ++i) {
                engine.process(buffer);
            }

            double rms = calculateRMS(buffer);
            bool hasNaN = containsNaN(buffer);
            double peak = calculatePeak(buffer);

            TestResult result;
            result.testName = "SpectralFreeze " + paramName + " @ " + std::to_string(int(value * 100)) + "%";
            result.passed = (!hasNaN && peak < 10.0 && std::isfinite(rms));
            result.measuredValue = rms;
            result.details = "RMS: " + std::to_string(rms) + ", Peak: " + std::to_string(peak);
            allResults.push_back(result);

            std::cout << (result.passed ? "✓" : "✗") << " " << result.testName
                      << " - " << result.details << "\n";
        }
    }
}

void testSpectralFreezeFunctionality() {
    std::cout << "\n=== SpectralFreeze: Freeze Functionality Tests ===\n";

    SpectralFreeze engine;
    const double sampleRate = 44100.0;
    const int bufferSize = 512;

    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    // Generate test signal
    juce::AudioBuffer<float> buffer(2, bufferSize);
    generateSine(buffer, 1000.0, sampleRate, 0.3);

    std::map<int, float> params;
    params[0] = 1.0f;  // Freeze ON
    params[1] = 0.0f;
    params[2] = 0.5f;
    params[3] = 0.0f;
    params[4] = 1.0f;
    params[5] = 0.5f;
    params[6] = 1.0f;
    params[7] = 0.0f;
    engine.updateParameters(params);

    // Process blocks and capture output
    std::vector<double> rmsValues;
    for (int block = 0; block < 20; ++block) {
        engine.process(buffer);
        rmsValues.push_back(calculateRMS(buffer));

        // After block 5, replace input with silence
        if (block == 5) {
            buffer.clear();
        }
    }

    // Verify freeze maintains energy
    double avgRmsBeforeSilence = 0.0;
    double avgRmsAfterSilence = 0.0;
    for (int i = 0; i < 5; ++i) avgRmsBeforeSilence += rmsValues[i];
    for (int i = 10; i < 15; ++i) avgRmsAfterSilence += rmsValues[i];
    avgRmsBeforeSilence /= 5.0;
    avgRmsAfterSilence /= 5.0;

    TestResult result;
    result.testName = "SpectralFreeze Freeze Hold Test";
    result.passed = (avgRmsAfterSilence > avgRmsBeforeSilence * 0.5);  // Should maintain at least 50%
    result.measuredValue = avgRmsAfterSilence;
    result.expectedValue = avgRmsBeforeSilence;
    result.tolerance = 0.5;
    result.details = "Before: " + std::to_string(avgRmsBeforeSilence) +
                    ", After: " + std::to_string(avgRmsAfterSilence);
    allResults.push_back(result);

    std::cout << (result.passed ? "✓ PASS" : "✗ FAIL") << ": " << result.details << "\n";
}

// ============================================================================
// SPECTRAL GATE TESTS
// ============================================================================

void testSpectralGateThreshold() {
    std::cout << "\n=== SpectralGate: Threshold Accuracy Tests ===\n";

    SpectralGate_Platinum engine;
    const double sampleRate = 44100.0;
    const int bufferSize = 512;

    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    // Test threshold sweep from -60dB to 0dB
    for (float thresholdNorm : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
        float thresholdDb = -60.0f + 60.0f * thresholdNorm;

        juce::AudioBuffer<float> buffer(2, bufferSize);
        generateSine(buffer, 1000.0, sampleRate, 0.1); // -20dB signal

        std::map<int, float> params;
        params[0] = thresholdNorm;  // Threshold
        params[1] = 0.5f;           // Ratio (moderate)
        params[2] = 0.3f;           // Attack
        params[3] = 0.3f;           // Release
        params[4] = 0.0f;           // Freq Low (20Hz)
        params[5] = 1.0f;           // Freq High (20kHz)
        params[6] = 0.0f;           // Lookahead
        params[7] = 1.0f;           // Mix (full wet)
        engine.updateParameters(params);

        double inputRms = calculateRMS(buffer);

        // Process multiple blocks
        for (int i = 0; i < 10; ++i) {
            engine.process(buffer);
        }

        double outputRms = calculateRMS(buffer);
        double attenuation = 20.0 * std::log10(outputRms / (inputRms + 1e-10));

        TestResult result;
        result.testName = "SpectralGate Threshold " + std::to_string(int(thresholdDb)) + " dB";
        result.passed = (!containsNaN(buffer) && outputRms < 10.0);
        result.measuredValue = attenuation;
        result.details = "Input: " + std::to_string(inputRms) +
                        ", Output: " + std::to_string(outputRms) +
                        ", Attenuation: " + std::to_string(attenuation) + " dB";
        allResults.push_back(result);

        std::cout << (result.passed ? "✓" : "✗") << " " << result.testName
                  << " - " << result.details << "\n";
    }
}

void testSpectralGateFrequencyRange() {
    std::cout << "\n=== SpectralGate: Frequency Range Tests ===\n";

    SpectralGate_Platinum engine;
    const double sampleRate = 44100.0;
    const int bufferSize = 512;

    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    // Test different frequency ranges
    std::vector<std::tuple<float, float, std::string>> ranges = {
        {0.0f, 0.3f, "Low (20-250Hz)"},
        {0.3f, 0.6f, "Mid (250-2kHz)"},
        {0.6f, 1.0f, "High (2k-20kHz)"}
    };

    for (auto& [freqLow, freqHigh, name] : ranges) {
        juce::AudioBuffer<float> buffer(2, bufferSize);
        generateSine(buffer, 1000.0, sampleRate, 0.3);

        std::map<int, float> params;
        params[0] = 0.25f;       // Threshold -45dB
        params[1] = 0.5f;        // Ratio
        params[2] = 0.3f;        // Attack
        params[3] = 0.3f;        // Release
        params[4] = freqLow;     // Freq Low
        params[5] = freqHigh;    // Freq High
        params[6] = 0.0f;        // Lookahead
        params[7] = 1.0f;        // Mix
        engine.updateParameters(params);

        for (int i = 0; i < 10; ++i) {
            engine.process(buffer);
        }

        double rms = calculateRMS(buffer);
        bool hasNaN = containsNaN(buffer);

        TestResult result;
        result.testName = "SpectralGate Frequency " + name;
        result.passed = (!hasNaN && rms < 10.0);
        result.measuredValue = rms;
        result.details = "RMS: " + std::to_string(rms);
        allResults.push_back(result);

        std::cout << (result.passed ? "✓ PASS" : "✗ FAIL") << ": " << result.details << "\n";
    }
}

// ============================================================================
// FEEDBACK NETWORK TESTS
// ============================================================================

void testFeedbackNetworkStability() {
    std::cout << "\n=== FeedbackNetwork: Stability Tests ===\n";

    FeedbackNetwork engine;
    const double sampleRate = 44100.0;
    const int bufferSize = 512;

    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    // Test feedback amounts from safe to extreme
    for (float feedbackNorm : {0.0f, 0.25f, 0.5f, 0.75f, 0.99f}) {
        engine.reset();

        juce::AudioBuffer<float> buffer(2, bufferSize);
        generateImpulse(buffer);

        std::map<int, float> params;
        params[0] = 0.5f;        // Delay Time (moderate)
        params[1] = feedbackNorm; // Feedback
        params[2] = 0.0f;        // CrossFeed
        params[3] = 0.0f;        // Diffusion
        params[4] = 0.0f;        // Modulation
        params[5] = 0.0f;        // Freeze
        params[6] = 0.0f;        // Shimmer
        params[7] = 1.0f;        // Mix
        engine.updateParameters(params);

        // Process many blocks to test stability
        double maxRms = 0.0;
        bool unstable = false;

        for (int block = 0; block < 100; ++block) {
            engine.process(buffer);

            double rms = calculateRMS(buffer);
            maxRms = std::max(maxRms, rms);

            if (containsNaN(buffer) || rms > 100.0) {
                unstable = true;
                break;
            }

            if (block == 0) buffer.clear(); // Impulse only once
        }

        TestResult result;
        result.testName = "FeedbackNetwork Stability @ " + std::to_string(int(feedbackNorm * 100)) + "% FB";
        result.passed = (!unstable && maxRms < 10.0);
        result.measuredValue = maxRms;
        result.details = "Max RMS: " + std::to_string(maxRms) +
                        (unstable ? " (UNSTABLE)" : " (stable)");
        allResults.push_back(result);

        std::cout << (result.passed ? "✓ PASS" : "✗ FAIL") << ": " << result.details << "\n";
    }
}

void testFeedbackNetworkResonance() {
    std::cout << "\n=== FeedbackNetwork: Resonance Tests ===\n";

    FeedbackNetwork engine;
    const double sampleRate = 44100.0;
    const int bufferSize = 512;

    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    juce::AudioBuffer<float> buffer(2, bufferSize);
    generateImpulse(buffer);

    std::map<int, float> params;
    params[0] = 0.1f;   // Short delay for resonance
    params[1] = 0.7f;   // High feedback
    params[2] = 0.5f;   // CrossFeed
    params[3] = 0.5f;   // Diffusion
    params[4] = 0.0f;   // Modulation
    params[5] = 0.0f;   // Freeze
    params[6] = 0.0f;   // Shimmer
    params[7] = 1.0f;   // Mix
    engine.updateParameters(params);

    // Process and measure decay
    std::vector<double> rmsOverTime;
    for (int block = 0; block < 50; ++block) {
        engine.process(buffer);
        rmsOverTime.push_back(calculateRMS(buffer));
        if (block == 0) buffer.clear();
    }

    // Check for resonant buildup
    bool hasResonance = false;
    for (size_t i = 5; i < rmsOverTime.size(); ++i) {
        if (rmsOverTime[i] > rmsOverTime[4] * 0.5) {
            hasResonance = true;
            break;
        }
    }

    TestResult result;
    result.testName = "FeedbackNetwork Resonance Build";
    result.passed = hasResonance;
    result.measuredValue = *std::max_element(rmsOverTime.begin(), rmsOverTime.end());
    result.details = "Max RMS: " + std::to_string(result.measuredValue);
    allResults.push_back(result);

    std::cout << (result.passed ? "✓ PASS" : "✗ FAIL") << ": " << result.details << "\n";
}

// ============================================================================
// CHAOS GENERATOR TESTS
// ============================================================================

void testChaosGeneratorModulation() {
    std::cout << "\n=== ChaosGenerator: Modulation Tests ===\n";

    ChaosGenerator engine;
    const double sampleRate = 44100.0;
    const int bufferSize = 512;

    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    // Test different modulation depths
    for (float depth : {0.0f, 0.5f, 1.0f}) {
        juce::AudioBuffer<float> buffer(2, bufferSize);
        generateSine(buffer, 440.0, sampleRate, 0.5);

        std::map<int, float> params;
        params[0] = 0.5f;   // Rate (moderate)
        params[1] = depth;  // Depth
        params[2] = 0.0f;   // Type (Lorenz)
        params[3] = 0.5f;   // Smoothing
        params[4] = 0.0f;   // Target (Amplitude)
        params[5] = 0.0f;   // Sync
        params[6] = 0.5f;   // Seed
        params[7] = 1.0f;   // Mix
        engine.updateParameters(params);

        double inputRms = calculateRMS(buffer);

        // Process
        for (int i = 0; i < 20; ++i) {
            engine.process(buffer);
        }

        double outputRms = calculateRMS(buffer);
        double modAmount = std::abs(outputRms - inputRms) / inputRms;

        TestResult result;
        result.testName = "ChaosGenerator Modulation @ " + std::to_string(int(depth * 100)) + "% depth";
        result.passed = (!containsNaN(buffer) && outputRms < 10.0);
        result.measuredValue = modAmount;
        result.details = "Input: " + std::to_string(inputRms) +
                        ", Output: " + std::to_string(outputRms) +
                        ", Mod: " + std::to_string(modAmount * 100) + "%";
        allResults.push_back(result);

        std::cout << (result.passed ? "✓" : "✗") << " " << result.testName
                  << " - " << result.details << "\n";
    }
}

void testChaosGeneratorRandomness() {
    std::cout << "\n=== ChaosGenerator: Randomness/Chaos Tests ===\n";

    ChaosGenerator engine;
    const double sampleRate = 44100.0;
    const int bufferSize = 512;

    engine.prepareToPlay(sampleRate, bufferSize);
    engine.setNumChannels(2, 2);

    juce::AudioBuffer<float> buffer(2, bufferSize);
    generateSine(buffer, 440.0, sampleRate, 0.5);

    std::map<int, float> params;
    params[0] = 1.0f;   // High rate
    params[1] = 1.0f;   // Full depth
    params[2] = 0.0f;   // Lorenz
    params[3] = 0.0f;   // No smoothing
    params[4] = 0.0f;   // Amplitude mod
    params[5] = 0.0f;   // Free running
    params[6] = 0.5f;   // Seed
    params[7] = 1.0f;   // Full mix
    engine.updateParameters(params);

    // Collect samples over time
    std::vector<float> samples;
    for (int block = 0; block < 50; ++block) {
        engine.process(buffer);

        // Sample output
        auto* data = buffer.getReadPointer(0);
        for (int i = 0; i < bufferSize; i += 10) {
            samples.push_back(data[i]);
        }
    }

    // Calculate variance as measure of chaos
    double mean = 0.0;
    for (float s : samples) mean += s;
    mean /= samples.size();

    double variance = 0.0;
    for (float s : samples) {
        variance += (s - mean) * (s - mean);
    }
    variance /= samples.size();

    TestResult result;
    result.testName = "ChaosGenerator Chaos/Randomness";
    result.passed = (variance > 0.01 && !containsNaN(buffer));  // Should have significant variance
    result.measuredValue = variance;
    result.details = "Variance: " + std::to_string(variance) + ", Mean: " + std::to_string(mean);
    allResults.push_back(result);

    std::cout << (result.passed ? "✓ PASS" : "✗ FAIL") << ": " << result.details << "\n";
}

// ============================================================================
// LATENCY AND ARTIFACTS TESTS
// ============================================================================

void testProcessingLatency() {
    std::cout << "\n=== Processing Latency Tests ===\n";

    std::vector<std::pair<std::string, EngineBase*>> engines = {
        {"SpectralFreeze", new SpectralFreeze()},
        {"SpectralGate", new SpectralGate_Platinum()},
        {"FeedbackNetwork", new FeedbackNetwork()},
        {"ChaosGenerator", new ChaosGenerator()}
    };

    for (auto& [name, engine] : engines) {
        const double sampleRate = 44100.0;
        const int bufferSize = 512;

        engine->prepareToPlay(sampleRate, bufferSize);
        engine->setNumChannels(2, 2);

        int reportedLatency = engine->getLatencySamples();

        TestResult result;
        result.testName = name + " Latency";
        result.passed = (reportedLatency >= 0 && reportedLatency < 10000);
        result.measuredValue = reportedLatency;
        result.details = std::to_string(reportedLatency) + " samples (" +
                        std::to_string(reportedLatency * 1000.0 / sampleRate) + " ms)";
        allResults.push_back(result);

        std::cout << (result.passed ? "✓" : "INFO") << " " << result.testName
                  << ": " << result.details << "\n";

        delete engine;
    }
}

// ============================================================================
// REPORT GENERATION
// ============================================================================

void generateReport() {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "SPECTRAL PROCESSING ENGINES - DEEP VALIDATION REPORT\n";
    std::cout << std::string(80, '=') << "\n\n";

    int passed = 0;
    int failed = 0;

    std::cout << std::left << std::setw(50) << "Test Name"
              << std::setw(10) << "Status"
              << std::setw(20) << "Value" << "\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& result : allResults) {
        if (result.passed) passed++;
        else failed++;

        std::cout << std::left << std::setw(50) << result.testName
                  << std::setw(10) << (result.passed ? "PASS" : "FAIL")
                  << std::setw(20) << result.measuredValue << "\n";
    }

    std::cout << std::string(80, '=') << "\n";
    std::cout << "SUMMARY: " << passed << " passed, " << failed << " failed out of "
              << (passed + failed) << " tests\n";
    std::cout << "Pass rate: " << (100.0 * passed / (passed + failed)) << "%\n";
    std::cout << std::string(80, '=') << "\n";
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "SPECTRAL PROCESSING ENGINES - DEEP VALIDATION SUITE\n";
    std::cout << "Testing: SpectralFreeze, SpectralGate, FeedbackNetwork, ChaosGenerator\n\n";

    try {
        // SpectralFreeze Tests
        testSpectralFreezeFFTSizes();
        testSpectralFreezeParameters();
        testSpectralFreezeFunctionality();

        // SpectralGate Tests
        testSpectralGateThreshold();
        testSpectralGateFrequencyRange();

        // FeedbackNetwork Tests
        testFeedbackNetworkStability();
        testFeedbackNetworkResonance();

        // ChaosGenerator Tests
        testChaosGeneratorModulation();
        testChaosGeneratorRandomness();

        // Latency Tests
        testProcessingLatency();

        // Generate final report
        generateReport();

    } catch (const std::exception& e) {
        std::cerr << "ERROR: Test suite crashed: " << e.what() << "\n";
        return 1;
    }

    // Return 0 if all tests passed
    int failures = 0;
    for (const auto& result : allResults) {
        if (!result.passed) failures++;
    }

    return (failures == 0) ? 0 : 1;
}

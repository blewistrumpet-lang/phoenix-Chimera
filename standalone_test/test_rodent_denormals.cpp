/**
 * test_rodent_denormals.cpp
 *
 * Test RodentDistortion (Engine 21) for denormal number production.
 * Verifies the denormal protection fixes are working.
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <cfloat>
#include <xmmintrin.h>

// JUCE includes
#include "../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

// Engine
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"

// Check if value is denormal
bool isDenormal(float value) {
    if (!std::isfinite(value)) return false;
    float absVal = std::abs(value);
    return absVal > 0.0f && absVal < FLT_MIN;
}

// Count denormals in buffer
int countDenormals(const juce::AudioBuffer<float>& buffer) {
    int count = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (isDenormal(data[i])) {
                count++;
            }
        }
    }
    return count;
}

// Test scenario structure
struct TestScenario {
    std::string name;
    std::map<int, float> params;
    int durationSamples;
};

int main() {
    std::cout << "=== RodentDistortion Denormal Test ===" << std::endl;
    std::cout << std::endl;

    // Create engine
    std::unique_ptr<EngineBase> engine = EngineFactory::createEngine(21); // RodentDistortion
    if (!engine) {
        std::cerr << "ERROR: Failed to create RodentDistortion engine!" << std::endl;
        return 1;
    }

    std::cout << "Engine: " << engine->getName().toStdString() << std::endl;
    std::cout << std::endl;

    // Setup audio
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);
    engine->reset();

    // Test scenarios that were producing denormals
    std::vector<TestScenario> scenarios = {
        {
            "Scenario 1: Silence with Fuzz Face mode",
            {
                {0, 0.5f},  // gain
                {1, 0.4f},  // filter
                {2, 0.5f},  // clipping
                {3, 0.5f},  // tone
                {4, 0.5f},  // output
                {5, 1.0f},  // mix
                {6, 0.75f}, // mode (Fuzz Face = 3/4)
                {7, 0.3f}   // presence
            },
            (int)(sampleRate * 10.0) // 10 seconds
        },
        {
            "Scenario 2: Very low input signal",
            {
                {0, 0.8f},  // high gain
                {1, 0.5f},  // filter
                {2, 0.7f},  // high clipping
                {3, 0.5f},  // tone
                {4, 0.5f},  // output
                {5, 1.0f},  // mix
                {6, 0.25f}, // Tube Screamer mode
                {7, 0.5f}   // presence
            },
            (int)(sampleRate * 10.0) // 10 seconds
        },
        {
            "Scenario 3: RAT mode with feedback",
            {
                {0, 0.6f},  // gain
                {1, 0.3f},  // filter
                {2, 0.8f},  // high clipping
                {3, 0.4f},  // tone
                {4, 0.5f},  // output
                {5, 1.0f},  // mix
                {6, 0.0f},  // RAT mode
                {7, 0.4f}   // presence
            },
            (int)(sampleRate * 10.0) // 10 seconds
        }
    };

    // Results
    std::ofstream report("rodent_denormal_test_report.txt");
    bool allPassed = true;
    int totalDenormals = 0;

    for (size_t scenarioIdx = 0; scenarioIdx < scenarios.size(); ++scenarioIdx) {
        const auto& scenario = scenarios[scenarioIdx];

        std::cout << "Testing " << scenario.name << "..." << std::endl;
        report << "=== " << scenario.name << " ===" << std::endl;

        // Reset engine
        engine->reset();
        engine->updateParameters(scenario.params);

        // Process audio in blocks
        int samplesProcessed = 0;
        int denormalCount = 0;
        int nanCount = 0;
        int infCount = 0;

        auto startTime = std::chrono::high_resolution_clock::now();

        while (samplesProcessed < scenario.durationSamples) {
            // Create silent input buffer
            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.clear();

            // Add tiny signal that decays to near-zero (triggers denormals)
            if (samplesProcessed == 0) {
                for (int ch = 0; ch < 2; ++ch) {
                    float* data = buffer.getWritePointer(ch);
                    data[0] = 1.0e-10f; // Very small initial signal
                }
            }

            // Process
            engine->process(buffer);

            // Check for denormals, NaN, Inf
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    float sample = data[i];

                    if (std::isnan(sample)) {
                        nanCount++;
                    } else if (std::isinf(sample)) {
                        infCount++;
                    } else if (isDenormal(sample)) {
                        denormalCount++;
                        if (denormalCount <= 5) {
                            // Print first few denormals for debugging
                            report << "  Denormal found at sample " << samplesProcessed + i
                                   << ", channel " << ch << ": " << std::scientific << sample << std::endl;
                        }
                    }
                }
            }

            samplesProcessed += blockSize;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // Report results
        double processingTime = duration.count() / 1000.0;
        double realtimeRatio = processingTime / (scenario.durationSamples / sampleRate);

        std::cout << "  Processing time: " << std::fixed << std::setprecision(3)
                  << processingTime << "s" << std::endl;
        std::cout << "  Realtime ratio: " << realtimeRatio * 100.0 << "%" << std::endl;
        std::cout << "  Denormals found: " << denormalCount << std::endl;
        std::cout << "  NaN values: " << nanCount << std::endl;
        std::cout << "  Inf values: " << infCount << std::endl;

        report << "Processing time: " << processingTime << "s" << std::endl;
        report << "Realtime ratio: " << realtimeRatio * 100.0 << "%" << std::endl;
        report << "Denormals found: " << denormalCount << std::endl;
        report << "NaN values: " << nanCount << std::endl;
        report << "Inf values: " << infCount << std::endl;

        bool passed = (denormalCount == 0 && nanCount == 0 && infCount == 0);
        std::cout << "  Result: " << (passed ? "PASS" : "FAIL") << std::endl;
        report << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;

        if (!passed) {
            allPassed = false;
        }

        totalDenormals += denormalCount;
        std::cout << std::endl;
        report << std::endl;
    }

    // Final summary
    std::cout << "=== FINAL SUMMARY ===" << std::endl;
    std::cout << "Total scenarios tested: " << scenarios.size() << std::endl;
    std::cout << "Total denormals found: " << totalDenormals << std::endl;
    std::cout << "Overall result: " << (allPassed ? "PASS - No denormals!" : "FAIL - Denormals detected") << std::endl;

    report << "=== FINAL SUMMARY ===" << std::endl;
    report << "Total scenarios tested: " << scenarios.size() << std::endl;
    report << "Total denormals found: " << totalDenormals << std::endl;
    report << "Overall result: " << (allPassed ? "PASS" : "FAIL") << std::endl;

    report.close();
    std::cout << "\nReport saved to: rodent_denormal_test_report.txt" << std::endl;

    return allPassed ? 0 : 1;
}

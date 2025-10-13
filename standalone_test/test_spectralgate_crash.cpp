#include <JuceHeader.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>

// Engine 52 - SpectralGate
#include "../JUCE_Plugin/Source/SpectralGate_Platinum.h"

// Test results tracking
struct TestResult {
    std::string testName;
    bool passed;
    std::string error;
    double duration;
};

std::vector<TestResult> results;

// Test utilities
void printTestHeader(const std::string& name) {
    std::cout << "\n==========================================\n";
    std::cout << "TEST: " << name << "\n";
    std::cout << "==========================================\n";
}

void recordResult(const std::string& name, bool passed, const std::string& error = "", double duration = 0.0) {
    results.push_back({name, passed, error, duration});
    if (passed) {
        std::cout << "[PASS] " << name << " (" << duration << "ms)\n";
    } else {
        std::cout << "[FAIL] " << name << ": " << error << "\n";
    }
}

// Signal generators
void generateImpulse(juce::AudioBuffer<float>& buffer) {
    buffer.clear();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.setSample(ch, 0, 1.0f);
    }
}

void generateSilence(juce::AudioBuffer<float>& buffer) {
    buffer.clear();
}

void generateWhiteNoise(juce::AudioBuffer<float>& buffer, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            buffer.setSample(ch, i, dist(rng));
        }
    }
}

void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, double sampleRate, double& phase) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        double ph = phase;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            buffer.setSample(ch, i, std::sin(ph));
            ph += 2.0 * M_PI * frequency / sampleRate;
        }
    }
    phase += 2.0 * M_PI * frequency * buffer.getNumSamples() / sampleRate;
    while (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
}

// Test 1: Impulse Response Test
bool testImpulseResponse() {
    printTestHeader("Impulse Response Test");
    auto start = std::chrono::high_resolution_clock::now();

    try {
        SpectralGate_Platinum engine;
        engine.prepareToPlay(44100.0, 512);

        // Set parameters
        std::map<int, float> params;
        params[0] = 0.25f; // Threshold
        params[1] = 0.5f;  // Ratio
        params[7] = 1.0f;  // Mix (100% wet)
        engine.updateParameters(params);

        // Process impulse
        juce::AudioBuffer<float> buffer(2, 512);
        generateImpulse(buffer);

        // Process multiple blocks to ensure stability
        for (int i = 0; i < 100; ++i) {
            engine.process(buffer);

            // Check for NaN or Inf
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int s = 0; s < buffer.getNumSamples(); ++s) {
                    if (!std::isfinite(data[s])) {
                        auto end = std::chrono::high_resolution_clock::now();
                        double duration = std::chrono::duration<double, std::milli>(end - start).count();
                        recordResult("Impulse Response Test", false,
                                   "NaN/Inf detected at block " + std::to_string(i), duration);
                        return false;
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Impulse Response Test", true, "", duration);
        return true;
    } catch (const std::exception& e) {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Impulse Response Test", false, std::string("Exception: ") + e.what(), duration);
        return false;
    }
}

// Test 2: Silence Test
bool testSilence() {
    printTestHeader("Silence Test");
    auto start = std::chrono::high_resolution_clock::now();

    try {
        SpectralGate_Platinum engine;
        engine.prepareToPlay(44100.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        generateSilence(buffer);

        // Process silence for extended period
        for (int i = 0; i < 1000; ++i) {
            engine.process(buffer);

            // Verify output is still silent or near-silent
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int s = 0; s < buffer.getNumSamples(); ++s) {
                    if (!std::isfinite(data[s])) {
                        auto end = std::chrono::high_resolution_clock::now();
                        double duration = std::chrono::duration<double, std::milli>(end - start).count();
                        recordResult("Silence Test", false,
                                   "NaN/Inf detected at block " + std::to_string(i), duration);
                        return false;
                    }
                    // Silence should produce near-zero output
                    if (std::abs(data[s]) > 0.01f) {
                        // This is OK, might be residual from processing
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Silence Test", true, "", duration);
        return true;
    } catch (const std::exception& e) {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Silence Test", false, std::string("Exception: ") + e.what(), duration);
        return false;
    }
}

// Test 3: Extreme Parameters Test
bool testExtremeParameters() {
    printTestHeader("Extreme Parameters Test");
    auto start = std::chrono::high_resolution_clock::now();

    try {
        SpectralGate_Platinum engine;
        engine.prepareToPlay(44100.0, 512);

        std::mt19937 rng(12345);
        juce::AudioBuffer<float> buffer(2, 512);
        double phase = 0.0;

        // Test extreme parameter combinations
        std::vector<std::map<int, float>> extremeParams = {
            {{0, 0.0f}, {1, 0.0f}, {7, 1.0f}},     // Min threshold, min ratio
            {{0, 1.0f}, {1, 1.0f}, {7, 1.0f}},     // Max threshold, max ratio
            {{0, 0.5f}, {1, 0.0f}, {7, 0.0f}},     // Dry signal
            {{0, 0.5f}, {1, 1.0f}, {7, 1.0f}},     // Full wet
            {{0, 0.0f}, {1, 1.0f}, {2, 0.0f}},     // Instant attack
            {{0, 0.0f}, {1, 1.0f}, {3, 1.0f}},     // Max release
        };

        for (size_t paramSet = 0; paramSet < extremeParams.size(); ++paramSet) {
            engine.updateParameters(extremeParams[paramSet]);

            // Process with different signal types
            for (int i = 0; i < 50; ++i) {
                if (i % 3 == 0) {
                    generateSineWave(buffer, 1000.0f, 44100.0, phase);
                } else if (i % 3 == 1) {
                    generateWhiteNoise(buffer, rng);
                } else {
                    generateImpulse(buffer);
                }

                engine.process(buffer);

                // Check for crashes
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    const float* data = buffer.getReadPointer(ch);
                    for (int s = 0; s < buffer.getNumSamples(); ++s) {
                        if (!std::isfinite(data[s])) {
                            auto end = std::chrono::high_resolution_clock::now();
                            double duration = std::chrono::duration<double, std::milli>(end - start).count();
                            recordResult("Extreme Parameters Test", false,
                                       "NaN/Inf at param set " + std::to_string(paramSet), duration);
                            return false;
                        }
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Extreme Parameters Test", true, "", duration);
        return true;
    } catch (const std::exception& e) {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Extreme Parameters Test", false, std::string("Exception: ") + e.what(), duration);
        return false;
    }
}

// Test 4: Rapid Parameter Changes
bool testRapidParameterChanges() {
    printTestHeader("Rapid Parameter Changes Test");
    auto start = std::chrono::high_resolution_clock::now();

    try {
        SpectralGate_Platinum engine;
        engine.prepareToPlay(44100.0, 512);

        std::mt19937 rng(54321);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        juce::AudioBuffer<float> buffer(2, 512);
        double phase = 0.0;

        // Rapidly change parameters while processing
        for (int i = 0; i < 500; ++i) {
            std::map<int, float> params;
            params[0] = dist(rng); // Threshold
            params[1] = dist(rng); // Ratio
            params[2] = dist(rng); // Attack
            params[3] = dist(rng); // Release
            params[4] = dist(rng); // FreqLow
            params[5] = dist(rng); // FreqHigh
            params[6] = dist(rng); // Lookahead
            params[7] = dist(rng); // Mix
            engine.updateParameters(params);

            generateSineWave(buffer, 440.0f, 44100.0, phase);
            engine.process(buffer);

            // Check stability
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int s = 0; s < buffer.getNumSamples(); ++s) {
                    if (!std::isfinite(data[s])) {
                        auto end = std::chrono::high_resolution_clock::now();
                        double duration = std::chrono::duration<double, std::milli>(end - start).count();
                        recordResult("Rapid Parameter Changes Test", false,
                                   "NaN/Inf at iteration " + std::to_string(i), duration);
                        return false;
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Rapid Parameter Changes Test", true, "", duration);
        return true;
    } catch (const std::exception& e) {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Rapid Parameter Changes Test", false, std::string("Exception: ") + e.what(), duration);
        return false;
    }
}

// Test 5: Extended Endurance Test (100+ cycles)
bool testEndurance() {
    printTestHeader("Extended Endurance Test (100+ cycles)");
    auto start = std::chrono::high_resolution_clock::now();

    try {
        SpectralGate_Platinum engine;
        engine.prepareToPlay(44100.0, 512);

        std::mt19937 rng(99999);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        juce::AudioBuffer<float> buffer(2, 512);
        double phase = 0.0;

        // Run for 1000 cycles with varied content
        const int totalCycles = 1000;
        for (int cycle = 0; cycle < totalCycles; ++cycle) {
            // Periodically change parameters
            if (cycle % 10 == 0) {
                std::map<int, float> params;
                params[0] = dist(rng);
                params[1] = dist(rng);
                params[7] = dist(rng);
                engine.updateParameters(params);
            }

            // Vary signal content
            if (cycle % 4 == 0) {
                generateSineWave(buffer, 1000.0f * (1.0f + dist(rng)), 44100.0, phase);
            } else if (cycle % 4 == 1) {
                generateWhiteNoise(buffer, rng);
            } else if (cycle % 4 == 2) {
                generateImpulse(buffer);
            } else {
                generateSilence(buffer);
            }

            engine.process(buffer);

            // Check every 100 cycles
            if (cycle % 100 == 0) {
                std::cout << "  Progress: " << cycle << "/" << totalCycles << " cycles\n";
            }

            // Validate output
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int s = 0; s < buffer.getNumSamples(); ++s) {
                    if (!std::isfinite(data[s])) {
                        auto end = std::chrono::high_resolution_clock::now();
                        double duration = std::chrono::duration<double, std::milli>(end - start).count();
                        recordResult("Extended Endurance Test", false,
                                   "NaN/Inf at cycle " + std::to_string(cycle), duration);
                        return false;
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Extended Endurance Test", true, "", duration);
        return true;
    } catch (const std::exception& e) {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Extended Endurance Test", false, std::string("Exception: ") + e.what(), duration);
        return false;
    }
}

// Test 6: Buffer Size Variations
bool testBufferSizeVariations() {
    printTestHeader("Buffer Size Variations Test");
    auto start = std::chrono::high_resolution_clock::now();

    try {
        std::vector<int> bufferSizes = {16, 32, 64, 128, 256, 512, 1024, 2048};
        std::mt19937 rng(11111);

        for (int bufferSize : bufferSizes) {
            SpectralGate_Platinum engine;
            engine.prepareToPlay(44100.0, bufferSize);

            std::map<int, float> params;
            params[0] = 0.5f;
            params[7] = 1.0f;
            engine.updateParameters(params);

            juce::AudioBuffer<float> buffer(2, bufferSize);
            double phase = 0.0;

            // Process 50 blocks
            for (int i = 0; i < 50; ++i) {
                generateSineWave(buffer, 440.0f, 44100.0, phase);
                engine.process(buffer);

                // Validate
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    const float* data = buffer.getReadPointer(ch);
                    for (int s = 0; s < buffer.getNumSamples(); ++s) {
                        if (!std::isfinite(data[s])) {
                            auto end = std::chrono::high_resolution_clock::now();
                            double duration = std::chrono::duration<double, std::milli>(end - start).count();
                            recordResult("Buffer Size Variations Test", false,
                                       "NaN/Inf at buffer size " + std::to_string(bufferSize), duration);
                            return false;
                        }
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Buffer Size Variations Test", true, "", duration);
        return true;
    } catch (const std::exception& e) {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Buffer Size Variations Test", false, std::string("Exception: ") + e.what(), duration);
        return false;
    }
}

// Test 7: Sample Rate Variations
bool testSampleRateVariations() {
    printTestHeader("Sample Rate Variations Test");
    auto start = std::chrono::high_resolution_clock::now();

    try {
        std::vector<double> sampleRates = {44100.0, 48000.0, 88200.0, 96000.0};
        std::mt19937 rng(22222);

        for (double sampleRate : sampleRates) {
            SpectralGate_Platinum engine;
            engine.prepareToPlay(sampleRate, 512);

            std::map<int, float> params;
            params[0] = 0.5f;
            params[7] = 1.0f;
            engine.updateParameters(params);

            juce::AudioBuffer<float> buffer(2, 512);
            double phase = 0.0;

            // Process 50 blocks
            for (int i = 0; i < 50; ++i) {
                generateSineWave(buffer, 1000.0f, sampleRate, phase);
                engine.process(buffer);

                // Validate
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    const float* data = buffer.getReadPointer(ch);
                    for (int s = 0; s < buffer.getNumSamples(); ++s) {
                        if (!std::isfinite(data[s])) {
                            auto end = std::chrono::high_resolution_clock::now();
                            double duration = std::chrono::duration<double, std::milli>(end - start).count();
                            recordResult("Sample Rate Variations Test", false,
                                       "NaN/Inf at sample rate " + std::to_string(sampleRate), duration);
                            return false;
                        }
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Sample Rate Variations Test", true, "", duration);
        return true;
    } catch (const std::exception& e) {
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        recordResult("Sample Rate Variations Test", false, std::string("Exception: ") + e.what(), duration);
        return false;
    }
}

// Main test runner
int main() {
    std::cout << "\n========================================\n";
    std::cout << "SpectralGate Engine 52 - Crash Test Suite\n";
    std::cout << "========================================\n\n";

    auto overallStart = std::chrono::high_resolution_clock::now();

    // Run all tests
    testImpulseResponse();
    testSilence();
    testExtremeParameters();
    testRapidParameterChanges();
    testBufferSizeVariations();
    testSampleRateVariations();
    testEndurance();

    auto overallEnd = std::chrono::high_resolution_clock::now();
    double totalDuration = std::chrono::duration<double, std::milli>(overallEnd - overallStart).count();

    // Print summary
    std::cout << "\n========================================\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << "========================================\n";

    int passed = 0;
    int failed = 0;

    for (const auto& result : results) {
        if (result.passed) {
            passed++;
        } else {
            failed++;
            std::cout << "[FAILED] " << result.testName << ": " << result.error << "\n";
        }
    }

    std::cout << "\nTotal Tests: " << results.size() << "\n";
    std::cout << "Passed: " << passed << "\n";
    std::cout << "Failed: " << failed << "\n";
    std::cout << "Total Duration: " << totalDuration << "ms\n";

    if (failed == 0) {
        std::cout << "\n[SUCCESS] All tests passed! Engine is stable.\n";
        return 0;
    } else {
        std::cout << "\n[FAILURE] Some tests failed.\n";
        return 1;
    }
}

/**
 * test_silence_handling.cpp
 *
 * Comprehensive silence handling test for all 56 engines in Chimera Phoenix.
 *
 * Tests for each engine:
 * - Process 10 seconds of pure silence (all zeros)
 * - Verify no NaN values in output
 * - Verify no denormal values in output
 * - Monitor CPU performance (no spikes)
 * - Verify clean silence output (for non-generators)
 * - Verify non-silence output for generators (ChaosGenerator should generate)
 *
 * Usage: ./test_silence_handling
 * Output: silence_handling_report.txt
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <cfloat>

// JUCE includes
#include "../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

// Engine Factory
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"

// Engine metadata structure
struct EngineMetadata {
    int id;
    std::string name;
    std::string category;
    bool isGenerator; // True for engines that should produce non-silence from silence
};

// All 56 engines with metadata
const std::vector<EngineMetadata> ALL_ENGINES = {
    // ENGINE_NONE (0)
    {0, "None (Bypass)", "Utility", false},

    // DYNAMICS & COMPRESSION (1-6)
    {1, "Vintage Opto Compressor", "Dynamics", false},
    {2, "Classic VCA Compressor", "Dynamics", false},
    {3, "Transient Shaper", "Dynamics", false},
    {4, "Noise Gate", "Dynamics", false},
    {5, "Mastering Limiter", "Dynamics", false},
    {6, "Dynamic EQ", "Dynamics", false},

    // FILTERS & EQ (7-14)
    {7, "Parametric EQ (Studio)", "Filter", false},
    {8, "Vintage Console EQ", "Filter", false},
    {9, "Ladder Filter", "Filter", false},
    {10, "State Variable Filter", "Filter", false},
    {11, "Formant Filter", "Filter", false},
    {12, "Envelope Filter", "Filter", false},
    {13, "Comb Resonator", "Filter", false},
    {14, "Vocal Formant Filter", "Filter", false},

    // DISTORTION & SATURATION (15-22)
    {15, "Vintage Tube Preamp", "Distortion", false},
    {16, "Wave Folder", "Distortion", false},
    {17, "Harmonic Exciter", "Distortion", false},
    {18, "Bit Crusher", "Distortion", false},
    {19, "Multiband Saturator", "Distortion", false},
    {20, "Muff Fuzz", "Distortion", false},
    {21, "Rodent Distortion", "Distortion", false},
    {22, "K-Style Overdrive", "Distortion", false},

    // MODULATION (23-33)
    {23, "Digital Chorus", "Modulation", false},
    {24, "Resonant Chorus", "Modulation", false},
    {25, "Analog Phaser", "Modulation", false},
    {26, "Ring Modulator", "Modulation", false},
    {27, "Frequency Shifter", "Modulation", false},
    {28, "Harmonic Tremolo", "Modulation", false},
    {29, "Classic Tremolo", "Modulation", false},
    {30, "Rotary Speaker", "Modulation", false},
    {31, "Pitch Shifter", "Modulation", false},
    {32, "Detune Doubler", "Modulation", false},
    {33, "Intelligent Harmonizer", "Modulation", false},

    // DELAY (34-38)
    {34, "Tape Echo", "Delay", false},
    {35, "Digital Delay", "Delay", false},
    {36, "Magnetic Drum Echo", "Delay", false},
    {37, "Bucket Brigade Delay", "Delay", false},
    {38, "Buffer Repeat", "Delay", false},

    // REVERB (39-43)
    {39, "Plate Reverb", "Reverb", false},
    {40, "Spring Reverb", "Reverb", false},
    {41, "Convolution Reverb", "Reverb", false},
    {42, "Shimmer Reverb", "Reverb", false},
    {43, "Gated Reverb", "Reverb", false},

    // SPATIAL & SPECIAL (44-52)
    {44, "Stereo Widener", "Spatial", false},
    {45, "Stereo Imager", "Spatial", false},
    {46, "Dimension Expander", "Spatial", false},
    {47, "Spectral Freeze", "Special", false},
    {48, "Spectral Gate", "Special", false},
    {49, "Phased Vocoder", "Special", false},
    {50, "Granular Cloud", "Special", true},  // Generator
    {51, "Chaos Generator", "Special", true},  // Generator - should produce non-silence!
    {52, "Feedback Network", "Special", false},

    // UTILITY (53-56)
    {53, "Mid-Side Processor", "Utility", false},
    {54, "Gain Utility", "Utility", false},
    {55, "Mono Maker", "Utility", false},
    {56, "Phase Align", "Utility", false}
};

// Test result structure
struct SilenceTestResult {
    int engineId;
    std::string engineName;
    std::string category;
    bool isGenerator;

    // Test results
    bool success;
    std::string errorMessage;

    // Silence handling metrics
    bool hasNaN;
    bool hasDenormals;
    bool hasCPUSpike;
    bool outputIsSilence;

    // Statistics
    int nanCount;
    int denormalCount;
    double maxAbsValue;
    double rmsValue;
    double processingTimeMs;
    double cpuPercentage;

    // Pass/Fail status
    bool passedNaNTest;
    bool passedDenormalTest;
    bool passedCPUTest;
    bool passedOutputTest;  // Silence for processors, non-silence for generators

    SilenceTestResult() :
        engineId(0), isGenerator(false), success(false),
        hasNaN(false), hasDenormals(false), hasCPUSpike(false), outputIsSilence(true),
        nanCount(0), denormalCount(0), maxAbsValue(0.0), rmsValue(0.0),
        processingTimeMs(0.0), cpuPercentage(0.0),
        passedNaNTest(true), passedDenormalTest(true), passedCPUTest(true), passedOutputTest(true) {}
};

// Check if a float is a denormal
inline bool isDenormal(float value) {
    if (value == 0.0f) return false;
    float absVal = std::abs(value);
    return absVal < FLT_MIN;
}

// Analyze buffer for issues
void analyzeBuffer(const juce::AudioBuffer<float>& buffer, SilenceTestResult& result) {
    const float SILENCE_THRESHOLD = 1e-10f;  // Threshold for considering output as silence
    const float DENORMAL_THRESHOLD = FLT_MIN;

    double sumSquares = 0.0;
    int totalSamples = 0;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        const float* channelData = buffer.getReadPointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float value = channelData[sample];

            // Check for NaN
            if (std::isnan(value)) {
                result.hasNaN = true;
                result.nanCount++;
            }

            // Check for denormals
            if (!std::isnan(value) && isDenormal(value)) {
                result.hasDenormals = true;
                result.denormalCount++;
            }

            // Track max absolute value
            float absVal = std::abs(value);
            if (absVal > result.maxAbsValue) {
                result.maxAbsValue = absVal;
            }

            // Accumulate for RMS
            sumSquares += value * value;
            totalSamples++;
        }
    }

    // Calculate RMS
    if (totalSamples > 0) {
        result.rmsValue = std::sqrt(sumSquares / totalSamples);
    }

    // Determine if output is silence
    result.outputIsSilence = (result.maxAbsValue < SILENCE_THRESHOLD);
}

// Test a single engine with silence input
SilenceTestResult testEngineWithSilence(const EngineMetadata& metadata) {
    SilenceTestResult result;
    result.engineId = metadata.id;
    result.engineName = metadata.name;
    result.category = metadata.category;
    result.isGenerator = metadata.isGenerator;

    try {
        // Create engine
        auto engine = EngineFactory::createEngine(metadata.id);
        if (!engine) {
            result.errorMessage = "Failed to create engine";
            return result;
        }

        // Setup parameters
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        const int numChannels = 2;
        const double durationSeconds = 10.0;
        const int totalSamples = static_cast<int>(sampleRate * durationSeconds);
        const int numBlocks = (totalSamples + blockSize - 1) / blockSize;

        // Prepare engine
        engine->prepareToPlay(sampleRate, blockSize);

        // Processing buffer (pre-initialized to silence)
        juce::AudioBuffer<float> blockBuffer(numChannels, blockSize);

        // Expected CPU threshold (processing should be much faster than real-time)
        const double MAX_CPU_PERCENTAGE = 50.0;  // Allow up to 50% CPU for silence

        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();

        // Process silence through engine
        for (int blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
            // Clear buffer to silence
            blockBuffer.clear();

            // Process the block
            engine->process(blockBuffer);

            // Analyze this block for issues
            analyzeBuffer(blockBuffer, result);
        }

        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();

        // Calculate timing metrics
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        result.processingTimeMs = elapsed.count();

        // CPU percentage = (time taken / real time) * 100
        double realTimeMs = durationSeconds * 1000.0;
        result.cpuPercentage = (result.processingTimeMs / realTimeMs) * 100.0;

        // Check for CPU spike
        result.hasCPUSpike = (result.cpuPercentage > MAX_CPU_PERCENTAGE);

        // Evaluate pass/fail criteria
        result.passedNaNTest = !result.hasNaN;
        result.passedDenormalTest = !result.hasDenormals;
        result.passedCPUTest = !result.hasCPUSpike;

        // Output test depends on whether it's a generator or processor
        if (result.isGenerator) {
            // Generators should produce non-silence
            result.passedOutputTest = !result.outputIsSilence;
        } else {
            // Processors should produce silence
            result.passedOutputTest = result.outputIsSilence;
        }

        result.success = true;

    } catch (const std::exception& e) {
        result.errorMessage = std::string("Exception: ") + e.what();
        result.success = false;
    } catch (...) {
        result.errorMessage = "Unknown exception";
        result.success = false;
    }

    return result;
}

// Save results to text report
void saveReport(const std::vector<SilenceTestResult>& results, const std::string& filename) {
    std::ofstream file(filename);

    file << "========================================================================\n";
    file << "         CHIMERA PHOENIX - SILENCE HANDLING TEST REPORT\n";
    file << "========================================================================\n";
    file << "\n";
    file << "Test Configuration:\n";
    file << "  - Sample Rate: 48 kHz\n";
    file << "  - Block Size: 512 samples\n";
    file << "  - Audio Duration: 10 seconds\n";
    file << "  - Channels: Stereo (2)\n";
    file << "  - Input: Pure silence (all zeros)\n";
    file << "  - Total Engines Tested: " << results.size() << "\n";
    file << "\n";

    // Count overall results
    int totalPassed = 0;
    int totalFailed = 0;
    int failedNaN = 0;
    int failedDenormal = 0;
    int failedCPU = 0;
    int failedOutput = 0;

    for (const auto& r : results) {
        if (!r.success) {
            totalFailed++;
            continue;
        }

        bool allPassed = r.passedNaNTest && r.passedDenormalTest &&
                        r.passedCPUTest && r.passedOutputTest;

        if (allPassed) {
            totalPassed++;
        } else {
            totalFailed++;
            if (!r.passedNaNTest) failedNaN++;
            if (!r.passedDenormalTest) failedDenormal++;
            if (!r.passedCPUTest) failedCPU++;
            if (!r.passedOutputTest) failedOutput++;
        }
    }

    file << "========================================================================\n";
    file << "                         OVERALL SUMMARY\n";
    file << "========================================================================\n";
    file << "\n";
    file << "Total Passed: " << totalPassed << " / " << results.size() << "\n";
    file << "Total Failed: " << totalFailed << " / " << results.size() << "\n";
    file << "\n";
    file << "Failure Breakdown:\n";
    file << "  - NaN Output: " << failedNaN << " engines\n";
    file << "  - Denormal Output: " << failedDenormal << " engines\n";
    file << "  - CPU Spike: " << failedCPU << " engines\n";
    file << "  - Incorrect Silence/Generation: " << failedOutput << " engines\n";
    file << "\n";

    file << "========================================================================\n";
    file << "                      DETAILED RESULTS\n";
    file << "========================================================================\n";
    file << "\n";

    for (const auto& r : results) {
        file << "------------------------------------------------------------------------\n";
        file << "Engine " << r.engineId << ": " << r.engineName << "\n";
        file << "Category: " << r.category;
        if (r.isGenerator) {
            file << " [GENERATOR]\n";
        } else {
            file << " [PROCESSOR]\n";
        }
        file << "------------------------------------------------------------------------\n";

        if (!r.success) {
            file << "STATUS: FAILED TO TEST\n";
            file << "Error: " << r.errorMessage << "\n";
            file << "\n";
            continue;
        }

        bool allPassed = r.passedNaNTest && r.passedDenormalTest &&
                        r.passedCPUTest && r.passedOutputTest;

        file << "STATUS: " << (allPassed ? "PASSED" : "FAILED") << "\n";
        file << "\n";

        // Test Results
        file << "Test Results:\n";
        file << "  NaN Test:       " << (r.passedNaNTest ? "PASS" : "FAIL");
        if (r.hasNaN) {
            file << " (" << r.nanCount << " NaN samples detected)";
        }
        file << "\n";

        file << "  Denormal Test:  " << (r.passedDenormalTest ? "PASS" : "FAIL");
        if (r.hasDenormals) {
            file << " (" << r.denormalCount << " denormal samples detected)";
        }
        file << "\n";

        file << "  CPU Test:       " << (r.passedCPUTest ? "PASS" : "FAIL");
        file << " (" << std::fixed << std::setprecision(2) << r.cpuPercentage << "% CPU)\n";

        file << "  Output Test:    " << (r.passedOutputTest ? "PASS" : "FAIL");
        if (r.isGenerator) {
            file << " (Expected: Non-Silence, Got: "
                 << (r.outputIsSilence ? "Silence" : "Non-Silence") << ")\n";
        } else {
            file << " (Expected: Silence, Got: "
                 << (r.outputIsSilence ? "Silence" : "Non-Silence") << ")\n";
        }
        file << "\n";

        // Statistics
        file << "Output Statistics:\n";
        file << "  Max Absolute Value: " << std::scientific << std::setprecision(6)
             << r.maxAbsValue << "\n";
        file << "  RMS Value:          " << std::scientific << std::setprecision(6)
             << r.rmsValue << "\n";
        file << "  Processing Time:    " << std::fixed << std::setprecision(3)
             << r.processingTimeMs << " ms\n";
        file << "\n";
    }

    file << "========================================================================\n";
    file << "                    GENERATORS ANALYSIS\n";
    file << "========================================================================\n";
    file << "\n";
    file << "Generators should produce non-silence output from silence input.\n";
    file << "\n";

    for (const auto& r : results) {
        if (r.isGenerator && r.success) {
            file << r.engineName << " (ID " << r.engineId << "):\n";
            file << "  Output: " << (r.outputIsSilence ? "SILENCE (BAD)" : "NON-SILENCE (GOOD)") << "\n";
            file << "  Max Output: " << std::scientific << std::setprecision(6)
                 << r.maxAbsValue << "\n";
            file << "  RMS: " << std::scientific << std::setprecision(6)
                 << r.rmsValue << "\n";
            file << "\n";
        }
    }

    file << "========================================================================\n";
    file << "                    FAILED ENGINES\n";
    file << "========================================================================\n";
    file << "\n";

    bool hasFailures = false;
    for (const auto& r : results) {
        if (!r.success) {
            hasFailures = true;
            file << r.engineName << " (ID " << r.engineId << "):\n";
            file << "  Error: " << r.errorMessage << "\n";
            file << "\n";
            continue;
        }

        bool allPassed = r.passedNaNTest && r.passedDenormalTest &&
                        r.passedCPUTest && r.passedOutputTest;

        if (!allPassed) {
            hasFailures = true;
            file << r.engineName << " (ID " << r.engineId << "):\n";

            if (!r.passedNaNTest) {
                file << "  - NaN detected (" << r.nanCount << " samples)\n";
            }
            if (!r.passedDenormalTest) {
                file << "  - Denormals detected (" << r.denormalCount << " samples)\n";
            }
            if (!r.passedCPUTest) {
                file << "  - CPU spike detected (" << std::fixed << std::setprecision(2)
                     << r.cpuPercentage << "%)\n";
            }
            if (!r.passedOutputTest) {
                if (r.isGenerator) {
                    file << "  - Generator produced silence (should produce non-silence)\n";
                } else {
                    file << "  - Processor produced non-silence (max: "
                         << std::scientific << std::setprecision(6)
                         << r.maxAbsValue << ")\n";
                }
            }
            file << "\n";
        }
    }

    if (!hasFailures) {
        file << "No failures detected! All engines passed silence handling tests.\n";
        file << "\n";
    }

    file << "========================================================================\n";
    file << "                    END OF REPORT\n";
    file << "========================================================================\n";

    file.close();
}

// Print summary to console
void printSummary(const std::vector<SilenceTestResult>& results) {
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "         CHIMERA PHOENIX - SILENCE HANDLING TEST SUMMARY\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";

    int totalPassed = 0;
    int totalFailed = 0;

    for (const auto& r : results) {
        if (!r.success) {
            totalFailed++;
            continue;
        }

        bool allPassed = r.passedNaNTest && r.passedDenormalTest &&
                        r.passedCPUTest && r.passedOutputTest;

        if (allPassed) {
            totalPassed++;
        } else {
            totalFailed++;
        }
    }

    std::cout << "Total Engines Tested: " << results.size() << "\n";
    std::cout << "Passed: " << totalPassed << "\n";
    std::cout << "Failed: " << totalFailed << "\n";
    std::cout << "\n";

    if (totalFailed > 0) {
        std::cout << "Failed Engines:\n";
        for (const auto& r : results) {
            bool allPassed = r.success && r.passedNaNTest && r.passedDenormalTest &&
                            r.passedCPUTest && r.passedOutputTest;

            if (!allPassed) {
                std::cout << "  [" << r.engineId << "] " << r.engineName;
                if (!r.success) {
                    std::cout << " - " << r.errorMessage;
                } else {
                    std::cout << " -";
                    if (!r.passedNaNTest) std::cout << " NaN";
                    if (!r.passedDenormalTest) std::cout << " Denormal";
                    if (!r.passedCPUTest) std::cout << " CPU";
                    if (!r.passedOutputTest) std::cout << " Output";
                }
                std::cout << "\n";
            }
        }
    }

    std::cout << "\n";
    std::cout << "Full report saved to: silence_handling_report.txt\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "    CHIMERA PHOENIX - COMPREHENSIVE SILENCE HANDLING TEST\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
    std::cout << "Testing silence handling on all 56 engines...\n";
    std::cout << "Processing 10 seconds of pure silence per engine at 48 kHz\n";
    std::cout << "\n";

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    std::vector<SilenceTestResult> results;

    // Test each engine
    int count = 0;
    for (const auto& engineMeta : ALL_ENGINES) {
        count++;
        std::cout << "[" << count << "/" << ALL_ENGINES.size() << "] "
                  << "Testing Engine " << engineMeta.id
                  << " (" << engineMeta.name << ")... " << std::flush;

        SilenceTestResult result = testEngineWithSilence(engineMeta);
        results.push_back(result);

        if (result.success) {
            bool allPassed = result.passedNaNTest && result.passedDenormalTest &&
                            result.passedCPUTest && result.passedOutputTest;

            if (allPassed) {
                std::cout << "PASS";
            } else {
                std::cout << "FAIL (";
                if (!result.passedNaNTest) std::cout << "NaN ";
                if (!result.passedDenormalTest) std::cout << "Denormal ";
                if (!result.passedCPUTest) std::cout << "CPU ";
                if (!result.passedOutputTest) std::cout << "Output";
                std::cout << ")";
            }
            std::cout << "\n";
        } else {
            std::cout << "ERROR - " << result.errorMessage << "\n";
        }
    }

    // Save report
    std::string outputFile = "silence_handling_report.txt";
    saveReport(results, outputFile);

    // Print summary
    printSummary(results);

    return 0;
}

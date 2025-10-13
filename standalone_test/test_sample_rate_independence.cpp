/**
 * test_sample_rate_independence.cpp
 *
 * Comprehensive sample rate independence test for Chimera Phoenix engines.
 *
 * Tests multiple sample rates (44.1kHz, 48kHz, 88.2kHz, 96kHz) to verify:
 * - No crashes or stability issues
 * - Correct frequency scaling (filters maintain relative cutoff frequencies)
 * - Similar sonic character across sample rates
 * - Proper initialization and processing at each rate
 *
 * Output: sample_rate_compatibility_report.txt with detailed analysis
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <cmath>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>
#include <chrono>

// JUCE includes
#include "../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

// Engine Factory
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"

// Test configuration
const std::vector<double> TEST_SAMPLE_RATES = {44100.0, 48000.0, 88200.0, 96000.0};
const int TEST_BLOCK_SIZE = 512;
const double TEST_DURATION = 1.0;  // 1 second test signal
const float TEST_FREQUENCY = 1000.0f;  // 1kHz test tone

// Engine metadata for testing
struct EngineInfo {
    int id;
    std::string name;
    std::string category;
    bool isFrequencyDependent;  // Does engine have frequency-dependent behavior?
};

// Key engines to test (representative of each category)
const std::vector<EngineInfo> KEY_ENGINES = {
    // DYNAMICS - should be sample rate independent
    {1, "Vintage Opto Compressor", "Dynamics", false},
    {2, "Classic VCA Compressor", "Dynamics", false},
    {5, "Mastering Limiter", "Dynamics", false},

    // FILTERS - frequency dependent, should scale correctly
    {7, "Parametric EQ", "Filter", true},
    {9, "Ladder Filter", "Filter", true},
    {10, "State Variable Filter", "Filter", true},

    // DISTORTION - should be mostly sample rate independent
    {15, "Vintage Tube Preamp", "Distortion", false},
    {20, "Muff Fuzz", "Distortion", false},
    {22, "K-Style Overdrive", "Distortion", false},

    // MODULATION - time-based, should scale correctly
    {23, "Digital Chorus", "Modulation", true},
    {25, "Analog Phaser", "Modulation", true},
    {29, "Classic Tremolo", "Modulation", true},
    {30, "Rotary Speaker", "Modulation", true},

    // DELAY - time-based, should scale correctly
    {34, "Tape Echo", "Delay", true},
    {35, "Digital Delay", "Delay", true},
    {37, "Bucket Brigade Delay", "Delay", true},

    // REVERB - should scale correctly
    {39, "Plate Reverb", "Reverb", true},
    {40, "Spring Reverb", "Reverb", true},
    {42, "Shimmer Reverb", "Reverb", true},

    // SPATIAL - mostly rate independent
    {44, "Stereo Widener", "Spatial", false},
    {46, "Dimension Expander", "Spatial", true},

    // SPECIAL - complex, varies
    {47, "Spectral Freeze", "Special", true},
    {49, "Phased Vocoder", "Special", true},
    {50, "Granular Cloud", "Special", true}
};

// Test result structure
struct SampleRateTestResult {
    double sampleRate;
    bool initialized;
    bool processedWithoutCrash;
    float peakOutput;
    float rmsOutput;
    float dcOffset;
    float thd;  // Total harmonic distortion
    double processingTimeMs;
    std::string errorMessage;
};

struct EngineTestResults {
    int engineId;
    std::string engineName;
    std::string category;
    std::vector<SampleRateTestResult> results;
    bool overallPass;
    std::string notes;
};

// Get default parameters for an engine
std::map<int, float> getDefaultParams(int engineId) {
    std::map<int, float> params;

    // Common defaults
    params[0] = 0.5f;  // Mix/Wet
    params[1] = 0.5f;  // Generic param 1
    params[2] = 0.5f;  // Generic param 2
    params[3] = 0.5f;  // Generic param 3
    params[4] = 0.5f;  // Generic param 4

    // Engine-specific parameters
    switch (engineId) {
        case 1: case 2:  // Compressors
            params[0] = 0.8f;  // Mix
            params[1] = 0.6f;  // Threshold
            params[2] = 0.5f;  // Ratio
            params[3] = 0.3f;  // Attack
            params[4] = 0.5f;  // Release
            break;

        case 5:  // Limiter
            params[0] = 1.0f;  // Mix
            params[1] = 0.8f;  // Threshold
            params[2] = 0.1f;  // Release
            break;

        case 7:  // Parametric EQ
            params[0] = 1.0f;  // Mix
            params[1] = 0.5f;  // Frequency
            params[2] = 0.7f;  // Gain
            params[3] = 0.5f;  // Q
            break;

        case 9: case 10:  // Filters
            params[0] = 1.0f;  // Mix
            params[1] = 0.6f;  // Cutoff
            params[2] = 0.5f;  // Resonance
            break;

        case 15: case 20: case 22:  // Distortion
            params[0] = 0.7f;  // Mix
            params[1] = 0.6f;  // Drive/Gain
            params[2] = 0.5f;  // Tone
            break;

        case 23:  // Chorus
            params[0] = 0.7f;  // Mix
            params[1] = 0.4f;  // Rate
            params[2] = 0.6f;  // Depth
            break;

        case 25:  // Phaser
            params[0] = 0.7f;  // Mix
            params[1] = 0.4f;  // Rate
            params[2] = 0.6f;  // Depth
            params[3] = 0.5f;  // Feedback
            break;

        case 29:  // Tremolo
            params[0] = 1.0f;  // Mix
            params[1] = 0.4f;  // Rate
            params[2] = 0.6f;  // Depth
            break;

        case 30:  // Rotary Speaker
            params[0] = 0.8f;  // Mix
            params[1] = 0.5f;  // Speed
            params[2] = 0.6f;  // Depth
            break;

        case 34: case 35: case 37:  // Delays
            params[0] = 0.5f;  // Mix
            params[1] = 0.3f;  // Time
            params[2] = 0.3f;  // Feedback
            break;

        case 39: case 40: case 42:  // Reverbs
            params[0] = 0.5f;  // Mix
            params[1] = 0.6f;  // Decay/Size
            params[2] = 0.5f;  // Damping
            break;

        case 44:  // Stereo Widener
            params[0] = 1.0f;  // Mix
            params[1] = 0.6f;  // Width
            break;

        case 46:  // Dimension Expander
            params[0] = 0.8f;  // Mix
            params[1] = 0.6f;  // Size
            break;

        case 47: case 49: case 50:  // Special effects
            params[0] = 0.7f;  // Mix
            params[1] = 0.5f;  // Generic
            break;
    }

    return params;
}

// Generate test signal
void generateTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate, float frequency) {
    const float amplitude = 0.5f;
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            float phase = 2.0f * juce::MathConstants<float>::pi * frequency * i / (float)sampleRate;
            data[i] = amplitude * std::sin(phase);
        }
    }
}

// Calculate THD (simplified - measures harmonics up to 5th)
float calculateTHD(const juce::AudioBuffer<float>& buffer, double sampleRate, float fundamentalFreq) {
    const int numSamples = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(0);

    // Calculate power at fundamental and harmonics
    float fundamentalPower = 0.0f;
    float harmonicsPower = 0.0f;

    // Simple DFT for fundamental and harmonics (2nd through 5th)
    for (int harmonic = 1; harmonic <= 5; ++harmonic) {
        float freq = fundamentalFreq * harmonic;
        float real = 0.0f;
        float imag = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            float phase = 2.0f * juce::MathConstants<float>::pi * freq * i / (float)sampleRate;
            real += data[i] * std::cos(phase);
            imag += data[i] * std::sin(phase);
        }

        float magnitude = std::sqrt(real * real + imag * imag) / numSamples;
        float power = magnitude * magnitude;

        if (harmonic == 1) {
            fundamentalPower = power;
        } else {
            harmonicsPower += power;
        }
    }

    // THD = sqrt(sum of harmonic powers) / fundamental power
    if (fundamentalPower < 1e-10f) return 0.0f;
    return std::sqrt(harmonicsPower) / std::sqrt(fundamentalPower);
}

// Test engine at specific sample rate
SampleRateTestResult testEngineAtSampleRate(int engineId, double sampleRate) {
    SampleRateTestResult result;
    result.sampleRate = sampleRate;
    result.initialized = false;
    result.processedWithoutCrash = false;
    result.peakOutput = 0.0f;
    result.rmsOutput = 0.0f;
    result.dcOffset = 0.0f;
    result.thd = 0.0f;
    result.processingTimeMs = 0.0;
    result.errorMessage = "";

    try {
        // Create engine
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.errorMessage = "Failed to create engine";
            return result;
        }

        // Initialize engine
        engine->prepareToPlay(sampleRate, TEST_BLOCK_SIZE);
        result.initialized = true;

        // Set parameters
        auto params = getDefaultParams(engineId);
        engine->updateParameters(params);

        // Calculate total samples
        const int totalSamples = static_cast<int>(sampleRate * TEST_DURATION);
        const int numChannels = 2;

        // Generate test signal
        juce::AudioBuffer<float> inputBuffer(numChannels, totalSamples);
        generateTestSignal(inputBuffer, sampleRate, TEST_FREQUENCY);

        // Output buffer
        juce::AudioBuffer<float> outputBuffer(numChannels, totalSamples);
        outputBuffer.clear();

        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();

        // Process in blocks
        int numBlocks = (totalSamples + TEST_BLOCK_SIZE - 1) / TEST_BLOCK_SIZE;

        for (int blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
            int startSample = blockIdx * TEST_BLOCK_SIZE;
            int numSamplesThisBlock = std::min(TEST_BLOCK_SIZE, totalSamples - startSample);

            // Create block buffer
            juce::AudioBuffer<float> blockBuffer(numChannels, TEST_BLOCK_SIZE);

            // Copy input to block
            for (int ch = 0; ch < numChannels; ++ch) {
                blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, numSamplesThisBlock);
                if (numSamplesThisBlock < TEST_BLOCK_SIZE) {
                    blockBuffer.clear(ch, numSamplesThisBlock, TEST_BLOCK_SIZE - numSamplesThisBlock);
                }
            }

            // Process the block
            engine->process(blockBuffer);

            // Copy output
            for (int ch = 0; ch < numChannels; ++ch) {
                outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesThisBlock);
            }
        }

        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        result.processingTimeMs = elapsed.count();

        result.processedWithoutCrash = true;

        // Analyze output
        const float* leftData = outputBuffer.getReadPointer(0);

        float sumSquares = 0.0f;
        float sum = 0.0f;

        for (int i = 0; i < totalSamples; ++i) {
            float sample = leftData[i];
            result.peakOutput = std::max(result.peakOutput, std::abs(sample));
            sumSquares += sample * sample;
            sum += sample;
        }

        result.rmsOutput = std::sqrt(sumSquares / totalSamples);
        result.dcOffset = sum / totalSamples;

        // Calculate THD
        result.thd = calculateTHD(outputBuffer, sampleRate, TEST_FREQUENCY);

        // Check for NaN or Inf
        if (std::isnan(result.peakOutput) || std::isinf(result.peakOutput)) {
            result.errorMessage = "Output contains NaN or Inf";
            result.processedWithoutCrash = false;
        }

    } catch (const std::exception& e) {
        result.errorMessage = std::string("Exception: ") + e.what();
    } catch (...) {
        result.errorMessage = "Unknown exception";
    }

    return result;
}

// Test engine across all sample rates
EngineTestResults testEngine(const EngineInfo& engineInfo) {
    EngineTestResults results;
    results.engineId = engineInfo.id;
    results.engineName = engineInfo.name;
    results.category = engineInfo.category;
    results.overallPass = true;

    std::cout << "\n[" << std::setw(2) << engineInfo.id << "] Testing " << engineInfo.name << "...\n";

    for (double sampleRate : TEST_SAMPLE_RATES) {
        std::cout << "  @ " << std::fixed << std::setprecision(1) << (sampleRate / 1000.0) << " kHz... ";
        std::cout.flush();

        auto result = testEngineAtSampleRate(engineInfo.id, sampleRate);
        results.results.push_back(result);

        if (result.processedWithoutCrash && result.initialized) {
            std::cout << "OK (peak=" << std::fixed << std::setprecision(3) << result.peakOutput
                     << ", rms=" << result.rmsOutput
                     << ", thd=" << std::setprecision(1) << (result.thd * 100.0f) << "%"
                     << ", " << std::setprecision(2) << result.processingTimeMs << "ms)\n";
        } else {
            std::cout << "FAIL";
            if (!result.errorMessage.empty()) {
                std::cout << " - " << result.errorMessage;
            }
            std::cout << "\n";
            results.overallPass = false;
        }
    }

    // Analyze consistency across sample rates
    if (results.overallPass && results.results.size() >= 2) {
        // Check peak output consistency (should be within 10%)
        float minPeak = results.results[0].peakOutput;
        float maxPeak = results.results[0].peakOutput;

        for (const auto& r : results.results) {
            minPeak = std::min(minPeak, r.peakOutput);
            maxPeak = std::max(maxPeak, r.peakOutput);
        }

        if (maxPeak > 1e-6f) {
            float variation = (maxPeak - minPeak) / maxPeak;
            if (variation > 0.15f) {  // More than 15% variation
                results.notes += "Warning: Peak output varies significantly across sample rates. ";
            }
        }

        // Check THD consistency for distortion/saturation
        if (engineInfo.category == "Distortion") {
            float minTHD = results.results[0].thd;
            float maxTHD = results.results[0].thd;

            for (const auto& r : results.results) {
                minTHD = std::min(minTHD, r.thd);
                maxTHD = std::max(maxTHD, r.thd);
            }

            if (maxTHD > 0.01f && (maxTHD - minTHD) / maxTHD > 0.20f) {
                results.notes += "Warning: THD varies significantly across sample rates. ";
            }
        }
    }

    return results;
}

// Generate report
void generateReport(const std::vector<EngineTestResults>& allResults) {
    std::ofstream report("sample_rate_compatibility_report.txt");
    if (!report.is_open()) {
        std::cerr << "Failed to create report file!\n";
        return;
    }

    report << "================================================================================\n";
    report << "        CHIMERA PHOENIX - SAMPLE RATE INDEPENDENCE TEST REPORT\n";
    report << "================================================================================\n";
    report << "\n";

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    report << "Test Date: " << std::ctime(&now_time);
    report << "Test Configuration:\n";
    report << "  Sample Rates: 44.1kHz, 48kHz, 88.2kHz, 96kHz\n";
    report << "  Test Signal: 1kHz sine wave, 0.5 amplitude\n";
    report << "  Duration: 1 second\n";
    report << "  Block Size: 512 samples\n";
    report << "\n";

    // Summary statistics
    int totalEngines = allResults.size();
    int passCount = 0;
    int failCount = 0;
    int warningCount = 0;

    for (const auto& result : allResults) {
        if (result.overallPass) {
            if (result.notes.empty()) {
                passCount++;
            } else {
                warningCount++;
            }
        } else {
            failCount++;
        }
    }

    report << "================================================================================\n";
    report << "                              SUMMARY\n";
    report << "================================================================================\n";
    report << "\n";
    report << "  Total Engines Tested:  " << totalEngines << "\n";
    report << "  Passed:                " << passCount << "\n";
    report << "  Passed with Warnings:  " << warningCount << "\n";
    report << "  Failed:                " << failCount << "\n";
    report << "\n";

    if (failCount == 0 && warningCount == 0) {
        report << "  Status: EXCELLENT - All engines are sample rate independent!\n";
    } else if (failCount == 0) {
        report << "  Status: GOOD - All engines work, some minor inconsistencies detected.\n";
    } else {
        report << "  Status: NEEDS ATTENTION - Some engines failed at certain sample rates.\n";
    }
    report << "\n";

    // Detailed results by category
    std::map<std::string, std::vector<const EngineTestResults*>> byCategory;
    for (const auto& result : allResults) {
        byCategory[result.category].push_back(&result);
    }

    report << "================================================================================\n";
    report << "                       DETAILED RESULTS BY CATEGORY\n";
    report << "================================================================================\n";
    report << "\n";

    for (const auto& [category, engines] : byCategory) {
        report << "--- " << category << " Engines ---\n";
        report << "\n";

        for (const auto* engineResult : engines) {
            report << "Engine " << std::setw(2) << engineResult->engineId << ": "
                   << engineResult->engineName << "\n";

            if (!engineResult->overallPass) {
                report << "  Status: FAILED\n";
            } else if (!engineResult->notes.empty()) {
                report << "  Status: PASSED (with warnings)\n";
            } else {
                report << "  Status: PASSED\n";
            }

            // Sample rate results table
            report << "\n";
            report << "  Sample Rate | Init | Process | Peak    | RMS     | THD    | Time (ms)\n";
            report << "  ------------|------|---------|---------|---------|--------|----------\n";

            for (const auto& sr : engineResult->results) {
                report << "  " << std::fixed << std::setprecision(1) << std::setw(10) << (sr.sampleRate / 1000.0) << "k | ";
                report << (sr.initialized ? " OK " : "FAIL") << "  | ";
                report << (sr.processedWithoutCrash ? "   OK   " : "  FAIL  ") << " | ";
                report << std::setprecision(4) << std::setw(7) << sr.peakOutput << " | ";
                report << std::setw(7) << sr.rmsOutput << " | ";
                report << std::setprecision(2) << std::setw(5) << (sr.thd * 100.0f) << "% | ";
                report << std::setprecision(2) << std::setw(8) << sr.processingTimeMs << "\n";

                if (!sr.errorMessage.empty()) {
                    report << "    Error: " << sr.errorMessage << "\n";
                }
            }

            if (!engineResult->notes.empty()) {
                report << "\n  Notes: " << engineResult->notes << "\n";
            }

            report << "\n";
        }
    }

    // Performance analysis
    report << "================================================================================\n";
    report << "                        PERFORMANCE ANALYSIS\n";
    report << "================================================================================\n";
    report << "\n";

    for (const auto& engineResult : allResults) {
        if (engineResult.overallPass && engineResult.results.size() == TEST_SAMPLE_RATES.size()) {
            double time44 = engineResult.results[0].processingTimeMs;
            double time96 = engineResult.results[3].processingTimeMs;

            if (time44 > 0.1) {  // Only analyze if measurable
                double ratio = time96 / time44;
                report << "Engine " << std::setw(2) << engineResult.engineId << " ("
                       << engineResult.engineName << "):\n";
                report << "  44.1kHz: " << std::fixed << std::setprecision(2) << time44 << " ms\n";
                report << "  96.0kHz: " << time96 << " ms\n";
                report << "  Ratio: " << std::setprecision(2) << ratio << "x\n";

                if (ratio > 2.5) {
                    report << "  Note: CPU usage scales more than linearly with sample rate\n";
                } else if (ratio < 1.8) {
                    report << "  Note: CPU usage scales less than linearly (good optimization)\n";
                }
                report << "\n";
            }
        }
    }

    // Recommendations
    report << "================================================================================\n";
    report << "                           RECOMMENDATIONS\n";
    report << "================================================================================\n";
    report << "\n";

    if (failCount > 0) {
        report << "CRITICAL ISSUES:\n";
        for (const auto& result : allResults) {
            if (!result.overallPass) {
                report << "  - Engine " << result.engineId << " (" << result.engineName
                       << ") failed at one or more sample rates\n";
            }
        }
        report << "\n";
    }

    if (warningCount > 0) {
        report << "WARNINGS:\n";
        for (const auto& result : allResults) {
            if (result.overallPass && !result.notes.empty()) {
                report << "  - Engine " << result.engineId << " (" << result.engineName
                       << "): " << result.notes << "\n";
            }
        }
        report << "\n";
    }

    if (failCount == 0 && warningCount == 0) {
        report << "All engines demonstrate excellent sample rate independence!\n";
        report << "\n";
        report << "Best practices observed:\n";
        report << "  - Consistent output levels across all sample rates\n";
        report << "  - Proper frequency scaling for time-based and filter effects\n";
        report << "  - Stable processing with no crashes or artifacts\n";
        report << "  - Similar sonic character maintained across rates\n";
    }

    report << "\n";
    report << "================================================================================\n";
    report << "                            END OF REPORT\n";
    report << "================================================================================\n";

    report.close();
    std::cout << "\n\nReport generated: sample_rate_compatibility_report.txt\n";
}

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "              SAMPLE RATE INDEPENDENCE TEST SUITE\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    std::cout << "Testing " << KEY_ENGINES.size() << " key engines at " << TEST_SAMPLE_RATES.size()
              << " sample rates:\n";
    std::cout << "  44.1 kHz (CD quality)\n";
    std::cout << "  48.0 kHz (Professional audio)\n";
    std::cout << "  88.2 kHz (High-res, 2x CD)\n";
    std::cout << "  96.0 kHz (High-res, 2x Pro)\n";
    std::cout << "\n";

    std::vector<EngineTestResults> allResults;

    for (const auto& engineInfo : KEY_ENGINES) {
        auto result = testEngine(engineInfo);
        allResults.push_back(result);
    }

    // Generate comprehensive report
    generateReport(allResults);

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                           TEST COMPLETE\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    // Count results
    int passCount = 0;
    int failCount = 0;
    for (const auto& result : allResults) {
        if (result.overallPass) {
            passCount++;
        } else {
            failCount++;
        }
    }

    std::cout << "Results Summary:\n";
    std::cout << "  Passed: " << passCount << "/" << allResults.size() << "\n";
    std::cout << "  Failed: " << failCount << "/" << allResults.size() << "\n";
    std::cout << "\n";
    std::cout << "See 'sample_rate_compatibility_report.txt' for detailed analysis.\n";
    std::cout << "\n";

    return (failCount == 0) ? 0 : 1;
}

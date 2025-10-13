/*
  ==============================================================================

    test_comprehensive_thd.cpp
    Comprehensive THD Test Suite for Clean Effects

    Purpose: Measure Total Harmonic Distortion across all "clean" effects
             (non-distortion engines that should maintain signal linearity)

    Test Coverage:
    - Engines 0-14:  None, Dynamics (1-6), Filters/EQ (7-14)
    - Engines 24-31: Modulation effects (Resonant Chorus through Pitch Shifter)
    - Engines 34-38: Delays and Buffer Repeat
    - Engines 42-43: Shimmer & Gated Reverb
    - Engines 46-48: Dimension Expander, Spectral Freeze, Spectral Gate
    - Engines 50-52: Granular Cloud, Chaos Generator, Feedback Network

    Test Methodology:
    1. Generate 1kHz pure sine wave @ -6dBFS
    2. Process through each engine with neutral/default parameters
    3. Perform FFT analysis to extract harmonics
    4. Calculate THD from 2nd-5th harmonics
    5. Flag engines with THD > 1.0%
    6. Generate CSV report with all measurements

  ==============================================================================
*/

#include "JuceHeader.h"
#include "ComprehensiveTHDEngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include "../JUCE_Plugin/Source/EngineTypes.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>

// ============================================================================
// THD Analyzer - FFT-based harmonic distortion measurement
// ============================================================================

class THDAnalyzer {
public:
    static constexpr int FFT_ORDER = 14; // 16384 samples
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;

    struct HarmonicAnalysis {
        float fundamental_dB = -200.0f;
        float second_harmonic_dB = -200.0f;
        float third_harmonic_dB = -200.0f;
        float fourth_harmonic_dB = -200.0f;
        float fifth_harmonic_dB = -200.0f;
        float thd_percent = 0.0f;
        float thd_db = -200.0f;
        float snr_dB = 0.0f;
        bool measurement_valid = false;
    };

    static HarmonicAnalysis measureTHD(const juce::AudioBuffer<float>& buffer,
                                       float fundamentalHz,
                                       float sampleRate) {
        HarmonicAnalysis result;

        if (buffer.getNumSamples() < FFT_SIZE) {
            std::cerr << "Buffer too short for FFT analysis\n";
            return result;
        }

        // Use windowed section for analysis
        const int fftSize = FFT_SIZE;
        const int numBins = fftSize / 2;

        juce::dsp::FFT fft(FFT_ORDER);
        std::vector<float> fftData(fftSize * 2, 0.0f);

        // Get data from left channel
        const float* data = buffer.getReadPointer(0);
        int startOffset = buffer.getNumSamples() / 4; // Skip initial transients

        // Apply Blackman-Harris window for minimal spectral leakage
        for (int i = 0; i < fftSize; ++i) {
            float w = i / float(fftSize - 1);
            float window = 0.35875f
                         - 0.48829f * std::cos(2.0f * M_PI * w)
                         + 0.14128f * std::cos(4.0f * M_PI * w)
                         - 0.01168f * std::cos(6.0f * M_PI * w);
            fftData[i * 2] = data[startOffset + i] * window;
            fftData[i * 2 + 1] = 0.0f;
        }

        // Perform FFT
        fft.performRealOnlyForwardTransform(fftData.data(), true);

        // Calculate magnitude spectrum
        std::vector<float> magnitude(numBins);
        for (int i = 0; i < numBins; ++i) {
            float real = fftData[i * 2];
            float imag = fftData[i * 2 + 1];
            magnitude[i] = std::sqrt(real * real + imag * imag);
        }

        // Calculate bin resolution
        float binWidth = sampleRate / fftSize;

        // Find fundamental (search around expected frequency)
        int fundamentalBin = static_cast<int>(fundamentalHz / binWidth + 0.5f);
        float maxMag = 0.0f;
        int actualFundamentalBin = fundamentalBin;

        for (int i = fundamentalBin - 3; i <= fundamentalBin + 3; ++i) {
            if (i >= 0 && i < numBins && magnitude[i] > maxMag) {
                maxMag = magnitude[i];
                actualFundamentalBin = i;
            }
        }

        if (maxMag < 1e-6f) {
            // Signal too quiet or missing
            return result;
        }

        result.fundamental_dB = 20.0f * std::log10(maxMag + 1e-10f);
        float fundamentalMag = maxMag;

        // Measure harmonics (2nd through 5th)
        std::vector<int> harmonicMultiples = {2, 3, 4, 5};
        std::vector<float> harmonicMags;
        float harmonicPowerSum = 0.0f;

        for (int harmonic : harmonicMultiples) {
            float expectedFreq = fundamentalHz * harmonic;
            if (expectedFreq > sampleRate / 2.0f) break;

            int harmonicBin = static_cast<int>(expectedFreq / binWidth + 0.5f);

            // Search in small range around expected bin
            float maxHarmonicMag = 0.0f;
            for (int i = harmonicBin - 2; i <= harmonicBin + 2; ++i) {
                if (i >= 0 && i < numBins) {
                    maxHarmonicMag = std::max(maxHarmonicMag, magnitude[i]);
                }
            }

            harmonicMags.push_back(maxHarmonicMag);
            harmonicPowerSum += maxHarmonicMag * maxHarmonicMag;

            // Store specific harmonics
            float harmonicDb = 20.0f * std::log10(maxHarmonicMag + 1e-10f);
            if (harmonic == 2) result.second_harmonic_dB = harmonicDb;
            if (harmonic == 3) result.third_harmonic_dB = harmonicDb;
            if (harmonic == 4) result.fourth_harmonic_dB = harmonicDb;
            if (harmonic == 5) result.fifth_harmonic_dB = harmonicDb;
        }

        // Calculate THD = sqrt(sum of harmonic powers) / fundamental
        float fundamentalPower = fundamentalMag * fundamentalMag;
        if (fundamentalPower > 0.0f) {
            float thdRatio = std::sqrt(harmonicPowerSum / fundamentalPower);
            result.thd_percent = thdRatio * 100.0f;
            result.thd_db = 20.0f * std::log10(thdRatio + 1e-10f);
            result.measurement_valid = true;
        }

        // Calculate noise floor (excluding fundamental and harmonics)
        float noiseEnergy = 0.0f;
        int noiseBins = 0;

        for (int i = 10; i < numBins; ++i) {
            float freq = i * binWidth;

            // Skip fundamental and harmonics (±5 bins)
            bool isHarmonic = false;
            for (int h = 1; h <= 5; ++h) {
                if (std::abs(freq - fundamentalHz * h) < 5 * binWidth) {
                    isHarmonic = true;
                    break;
                }
            }

            if (!isHarmonic) {
                noiseEnergy += magnitude[i] * magnitude[i];
                noiseBins++;
            }
        }

        if (noiseBins > 0) {
            float noiseRms = std::sqrt(noiseEnergy / noiseBins);
            result.snr_dB = 20.0f * std::log10((fundamentalMag + 1e-10f) / (noiseRms + 1e-10f));
        }

        return result;
    }
};

// ============================================================================
// Test Result Structure
// ============================================================================

struct EngineTestResult {
    int engineId;
    std::string engineName;
    float thd_percent;
    float thd_db;
    float fundamental_dB;
    float second_harmonic_dB;
    float third_harmonic_dB;
    float snr_dB;
    bool passed;
    bool skipped;
    std::string skipReason;
    double processingTimeMs;
};

// ============================================================================
// Comprehensive THD Test Suite
// ============================================================================

class ComprehensiveTHDTest {
private:
    const float sampleRate = 48000.0f;
    const int blockSize = 512;
    const float testFreqHz = 1000.0f;
    const float amplitudeDbFS = -6.0f;
    const float amplitude = std::pow(10.0f, amplitudeDbFS / 20.0f);
    const float thdThreshold = 1.0f; // 1.0%

    std::vector<EngineTestResult> results;
    std::ofstream csvFile;
    std::ofstream logFile;

    // Define which engines to test
    std::vector<int> testEngines;

public:
    ComprehensiveTHDTest() {
        // Build list of engines to test

        // 0-14: None, Dynamics, Filters/EQ
        for (int i = 0; i <= 14; ++i) {
            testEngines.push_back(i);
        }

        // 24-31: Modulation (excluding 23 - Digital Chorus)
        for (int i = 24; i <= 31; ++i) {
            testEngines.push_back(i);
        }

        // 34-38: Delays
        for (int i = 34; i <= 38; ++i) {
            testEngines.push_back(i);
        }

        // 42-43: Shimmer & Gated Reverb
        testEngines.push_back(42);
        testEngines.push_back(43);

        // 46-48: Spectral effects
        for (int i = 46; i <= 48; ++i) {
            testEngines.push_back(i);
        }

        // 50-52: Special effects
        for (int i = 50; i <= 52; ++i) {
            testEngines.push_back(i);
        }

        // Open output files
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/comprehensive_thd_results.csv");
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/comprehensive_thd_report.txt");

        // Write CSV header
        csvFile << "Engine ID,Engine Name,THD (%),THD (dB),Fundamental (dB),2nd Harmonic (dB),3rd Harmonic (dB),SNR (dB),Status,Processing Time (ms),Notes\n";
    }

    ~ComprehensiveTHDTest() {
        if (csvFile.is_open()) csvFile.close();
        if (logFile.is_open()) logFile.close();
    }

    void log(const std::string& message) {
        std::cout << message;
        if (logFile.is_open()) {
            logFile << message;
            logFile.flush();
        }
    }

    std::map<int, float> getDefaultParameters(int engineId) {
        std::map<int, float> params;

        // Set neutral/default parameters for each engine type
        // These are designed to produce minimal processing while still testing the engine

        // Dynamics engines (1-6) - minimal processing
        if (engineId >= 1 && engineId <= 6) {
            params[0] = 1.0f;  // High threshold (minimal compression)
            params[1] = 0.0f;  // Low ratio
            params[2] = 0.5f;  // Medium attack
            params[3] = 0.5f;  // Medium release
            params[4] = 0.5f;  // Unity gain
            params[5] = 1.0f;  // Full mix (test the actual processing)
        }

        // Filters/EQ (7-14) - flat response at test frequency
        if (engineId >= 7 && engineId <= 14) {
            params[0] = 0.2f;  // Frequency away from 1kHz
            params[1] = 0.3f;  // Low Q/resonance
            params[2] = 0.5f;  // Unity gain
            params[3] = 1.0f;  // Full mix
        }

        // Modulation (24-31) - minimal modulation depth
        if (engineId >= 24 && engineId <= 31) {
            params[0] = 0.5f;  // Rate = 1 Hz
            params[1] = 0.2f;  // Depth = 20%
            params[2] = 0.5f;  // Feedback = 50%
            params[3] = 0.5f;  // Mix = 50%
        }

        // Delays (34-38) - short delay, low feedback
        if (engineId >= 34 && engineId <= 38) {
            params[0] = 0.1f;  // Short delay time
            params[1] = 0.2f;  // Low feedback
            params[2] = 0.5f;  // Mix = 50%
        }

        // Reverbs (42-43) - short decay
        if (engineId == 42 || engineId == 43) {
            params[0] = 0.3f;  // Short decay
            params[1] = 0.5f;  // Medium size
            params[2] = 0.5f;  // Mix = 50%
        }

        // Spectral effects (46-48)
        if (engineId >= 46 && engineId <= 48) {
            params[0] = 0.5f;  // Default parameter
            params[1] = 0.5f;
            params[2] = 0.5f;  // Mix = 50%
        }

        // Special effects (50-52)
        if (engineId >= 50 && engineId <= 52) {
            params[0] = 0.3f;  // Low intensity
            params[1] = 0.5f;
            params[2] = 0.5f;  // Mix = 50%
        }

        return params;
    }

    void testEngine(int engineId) {
        EngineTestResult result;
        result.engineId = engineId;
        result.skipped = false;

        log("Testing Engine " + std::to_string(engineId) + ": ");

        // Create engine
        auto engine = ComprehensiveTHDEngineFactory::createEngine(engineId);
        if (!engine) {
            result.engineName = getEngineTypeName(engineId);
            result.skipped = true;
            result.skipReason = "Failed to create engine";
            result.passed = false;
            log(result.engineName + " - SKIPPED (creation failed)\n");
            results.push_back(result);

            csvFile << engineId << ","
                   << result.engineName << ","
                   << "N/A,N/A,N/A,N/A,N/A,N/A,"
                   << "SKIPPED,N/A,"
                   << result.skipReason << "\n";
            return;
        }

        result.engineName = getEngineTypeName(engineId);
        log(result.engineName + "...\n");

        try {
            // Prepare engine
            engine->prepareToPlay(sampleRate, blockSize);

            // Set parameters
            auto params = getDefaultParameters(engineId);
            engine->updateParameters(params);

            // Generate test signal: 1kHz sine wave
            const int testLength = static_cast<int>(sampleRate * 2.0f); // 2 seconds
            juce::AudioBuffer<float> buffer(2, testLength);

            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < testLength; ++i) {
                    float phase = 2.0f * M_PI * testFreqHz * i / sampleRate;
                    buffer.setSample(ch, i, amplitude * std::sin(phase));
                }
            }

            // Process through engine
            auto startTime = std::chrono::high_resolution_clock::now();

            for (int start = 0; start < testLength; start += blockSize) {
                int samplesThisBlock = std::min(blockSize, testLength - start);
                juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
                engine->process(block);
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            result.processingTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

            // Skip first 0.5 seconds to allow for transients
            int skipSamples = static_cast<int>(sampleRate * 0.5f);
            juce::AudioBuffer<float> analysisBuffer(2, testLength - skipSamples);

            for (int ch = 0; ch < 2; ++ch) {
                analysisBuffer.copyFrom(ch, 0, buffer, ch, skipSamples, testLength - skipSamples);
            }

            // Perform THD measurement
            auto thdResult = THDAnalyzer::measureTHD(analysisBuffer, testFreqHz, sampleRate);

            if (!thdResult.measurement_valid) {
                result.skipped = true;
                result.skipReason = "Invalid measurement (signal too weak or corrupted)";
                result.passed = false;
                log("  WARNING: Invalid THD measurement\n");
            } else {
                result.thd_percent = thdResult.thd_percent;
                result.thd_db = thdResult.thd_db;
                result.fundamental_dB = thdResult.fundamental_dB;
                result.second_harmonic_dB = thdResult.second_harmonic_dB;
                result.third_harmonic_dB = thdResult.third_harmonic_dB;
                result.snr_dB = thdResult.snr_dB;
                result.passed = (thdResult.thd_percent < thdThreshold);

                log("  THD: " + std::to_string(result.thd_percent) + "% - " +
                    (result.passed ? "PASS" : "FAIL") + "\n");
            }

            results.push_back(result);

            // Write to CSV
            if (result.skipped) {
                csvFile << engineId << ","
                       << result.engineName << ","
                       << "N/A,N/A,N/A,N/A,N/A,N/A,"
                       << "SKIPPED,"
                       << std::fixed << std::setprecision(2) << result.processingTimeMs << ","
                       << result.skipReason << "\n";
            } else {
                csvFile << engineId << ","
                       << result.engineName << ","
                       << std::fixed << std::setprecision(4) << result.thd_percent << ","
                       << std::fixed << std::setprecision(2) << result.thd_db << ","
                       << std::fixed << std::setprecision(2) << result.fundamental_dB << ","
                       << std::fixed << std::setprecision(2) << result.second_harmonic_dB << ","
                       << std::fixed << std::setprecision(2) << result.third_harmonic_dB << ","
                       << std::fixed << std::setprecision(2) << result.snr_dB << ","
                       << (result.passed ? "PASS" : "FAIL") << ","
                       << std::fixed << std::setprecision(2) << result.processingTimeMs << ","
                       << "\n";
            }

            csvFile.flush();

        } catch (const std::exception& e) {
            result.skipped = true;
            result.skipReason = std::string("Exception: ") + e.what();
            result.passed = false;
            log("  ERROR: " + result.skipReason + "\n");
            results.push_back(result);

            csvFile << engineId << ","
                   << result.engineName << ","
                   << "N/A,N/A,N/A,N/A,N/A,N/A,"
                   << "ERROR,N/A,"
                   << result.skipReason << "\n";
        }
    }

    void runAllTests() {
        log("\n");
        log("╔════════════════════════════════════════════════════════════════════════╗\n");
        log("║          Comprehensive THD Test Suite - Clean Effects Only            ║\n");
        log("╚════════════════════════════════════════════════════════════════════════╝\n");
        log("\n");
        log("Test Configuration:\n");
        log("  Sample Rate:       " + std::to_string(sampleRate) + " Hz\n");
        log("  Test Frequency:    " + std::to_string(testFreqHz) + " Hz\n");
        log("  Test Amplitude:    " + std::to_string(amplitudeDbFS) + " dBFS\n");
        log("  Block Size:        " + std::to_string(blockSize) + " samples\n");
        log("  THD Threshold:     " + std::to_string(thdThreshold) + "%\n");
        log("  Total Engines:     " + std::to_string(testEngines.size()) + "\n");
        log("\n");
        log("Engine Ranges:\n");
        log("  0-14:  None, Dynamics, Filters/EQ\n");
        log("  24-31: Modulation Effects\n");
        log("  34-38: Delay Effects\n");
        log("  42-43: Shimmer & Gated Reverb\n");
        log("  46-48: Spectral Effects\n");
        log("  50-52: Special Effects\n");
        log("\n");
        log("Starting tests...\n");
        log("═══════════════════════════════════════════════════════════════════════════\n\n");

        for (int engineId : testEngines) {
            testEngine(engineId);
        }

        printSummary();
    }

    void printSummary() {
        log("\n");
        log("╔════════════════════════════════════════════════════════════════════════╗\n");
        log("║                            TEST SUMMARY                                ║\n");
        log("╚════════════════════════════════════════════════════════════════════════╝\n");
        log("\n");

        int totalTests = results.size();
        int passed = 0;
        int failed = 0;
        int skipped = 0;

        std::vector<EngineTestResult> failedEngines;
        float worstTHD = 0.0f;
        std::string worstEngine;

        for (const auto& result : results) {
            if (result.skipped) {
                skipped++;
            } else if (result.passed) {
                passed++;
            } else {
                failed++;
                failedEngines.push_back(result);
                if (result.thd_percent > worstTHD) {
                    worstTHD = result.thd_percent;
                    worstEngine = result.engineName;
                }
            }
        }

        log("Overall Statistics:\n");
        log("─────────────────────────────────────────────────────────────────────────\n");
        log("  Total Engines Tested:  " + std::to_string(totalTests) + "\n");
        log("  Passed (THD < 1%):     " + std::to_string(passed) + "\n");
        log("  Failed (THD >= 1%):    " + std::to_string(failed) + "\n");
        log("  Skipped/Error:         " + std::to_string(skipped) + "\n");

        if (totalTests > 0) {
            float passRate = 100.0f * passed / totalTests;
            log("  Pass Rate:             " + std::to_string(passRate) + "%\n");
        }

        log("\n");

        if (failed > 0) {
            log("⚠ FAILED ENGINES (THD > 1%):\n");
            log("─────────────────────────────────────────────────────────────────────────\n");

            // Sort by THD (worst first)
            std::sort(failedEngines.begin(), failedEngines.end(),
                     [](const EngineTestResult& a, const EngineTestResult& b) {
                         return a.thd_percent > b.thd_percent;
                     });

            for (const auto& result : failedEngines) {
                log("  Engine " + std::to_string(result.engineId) + " - " + result.engineName + ":\n");
                log("    THD:             " + std::to_string(result.thd_percent) + "% (" +
                    std::to_string(result.thd_db) + " dB)\n");
                log("    2nd Harmonic:    " + std::to_string(result.second_harmonic_dB) + " dB\n");
                log("    3rd Harmonic:    " + std::to_string(result.third_harmonic_dB) + " dB\n");
                log("\n");
            }

            log("Worst Case: Engine " + worstEngine + " with " + std::to_string(worstTHD) + "% THD\n");
            log("\n");
        } else {
            log("✓ All tested engines passed THD requirements!\n\n");
        }

        log("═══════════════════════════════════════════════════════════════════════════\n");
        log("\n");
        log("Detailed Results:\n");
        log("  CSV Report: comprehensive_thd_results.csv\n");
        log("  Log File:   comprehensive_thd_report.txt\n");
        log("\n");
    }
};

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    try {
        std::cout << "\nStarting Comprehensive THD Test Suite...\n\n";

        ComprehensiveTHDTest tester;
        tester.runAllTests();

        std::cout << "\nTest suite complete!\n";
        std::cout << "Check comprehensive_thd_results.csv for detailed measurements.\n\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\nFATAL ERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\nFATAL ERROR: Unknown exception occurred." << std::endl;
        return 1;
    }
}

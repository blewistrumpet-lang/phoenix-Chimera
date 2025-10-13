#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <fstream>
#include <map>

//==============================================================================
// Pitch Accuracy Test Suite for Pitch Shifter Engines
// Engines: 32-38, 49-50
// Tests multiple semitone shifts: -12, -7, -5, 0, +5, +7, +12
// Measures output frequencies and calculates cent error
//==============================================================================

// Engine mapping
const std::map<int, std::string> PITCH_ENGINES = {
    {32, "Pitch Shifter"},
    {33, "Intelligent Harmonizer"},
    {34, "Tape Echo"},
    {35, "Digital Delay"},
    {36, "Magnetic Drum Echo"},
    {37, "Bucket Brigade Delay"},
    {38, "Buffer Repeat Platinum"},
    {49, "Pitch Shifter (Alt)"},
    {50, "GranularCloud"}
};

// Test configuration
const std::vector<int> SEMITONE_SHIFTS = {-12, -7, -5, 0, +5, +7, +12};
const std::vector<float> TEST_FREQUENCIES = {110.0f, 220.0f, 440.0f, 880.0f, 1760.0f};
const float SAMPLE_RATE = 48000.0f;
const int BLOCK_SIZE = 512;

//==============================================================================
// Pitch Detection using FFT with parabolic interpolation
//==============================================================================
float detectFundamentalFrequency(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13); // 2^13 = 8192
    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy first channel with Hann window
    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find peak frequency (skip DC and very low frequencies)
    int maxBin = 0;
    float maxMag = 0.0f;
    int minBin = 5; // Skip DC and very low frequencies

    for (int i = minBin; i < fftSize / 2; ++i) {
        if (fftData[i] > maxMag) {
            maxMag = fftData[i];
            maxBin = i;
        }
    }

    if (maxBin == 0 || maxMag < 1e-6f) return 0.0f;

    // Parabolic interpolation for sub-bin accuracy
    if (maxBin > 0 && maxBin < fftSize / 2 - 1) {
        float alpha = fftData[maxBin - 1];
        float beta = fftData[maxBin];
        float gamma = fftData[maxBin + 1];

        if (alpha > 0.0f && gamma > 0.0f) {
            float p = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);
            float interpolatedBin = maxBin + p;
            return interpolatedBin * sampleRate / fftSize;
        }
    }

    return maxBin * sampleRate / fftSize;
}

//==============================================================================
// Calculate cent error between two frequencies
// Cent = 1/100 of a semitone
//==============================================================================
float calculateCentError(float measuredFreq, float expectedFreq) {
    if (measuredFreq <= 0.0f || expectedFreq <= 0.0f) return 9999.0f;
    return 1200.0f * std::log2(measuredFreq / expectedFreq);
}

//==============================================================================
// Test single pitch shift configuration
//==============================================================================
struct PitchTestResult {
    int engineId;
    std::string engineName;
    float inputFreq;
    int semitoneShift;
    float expectedFreq;
    float measuredFreq;
    float centError;
    bool validMeasurement;
    std::string errorMsg;
};

PitchTestResult testPitchShift(int engineId, float inputFreq, int semitoneShift) {
    PitchTestResult result;
    result.engineId = engineId;
    result.engineName = PITCH_ENGINES.count(engineId) ? PITCH_ENGINES.at(engineId) : "Unknown";
    result.inputFreq = inputFreq;
    result.semitoneShift = semitoneShift;
    result.expectedFreq = inputFreq * std::pow(2.0f, semitoneShift / 12.0f);
    result.measuredFreq = 0.0f;
    result.centError = 0.0f;
    result.validMeasurement = false;
    result.errorMsg = "";

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.errorMsg = "Failed to create engine";
            return result;
        }

        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);

        // Set pitch shift parameter
        std::map<int, float> params;

        // Normalize semitone shift to 0-1 range (assuming -12 to +12 semitone range)
        float normalizedShift = (semitoneShift + 12.0f) / 24.0f;
        normalizedShift = std::max(0.0f, std::min(1.0f, normalizedShift));

        params[0] = normalizedShift;

        // Set mix to 100% wet if available
        if (engine->getNumParameters() > 1) {
            params[1] = 1.0f;
        }

        // Set additional parameters to defaults
        for (int i = 2; i < engine->getNumParameters(); ++i) {
            params[i] = 0.5f;
        }

        engine->reset();
        engine->updateParameters(params);

        // Generate test signal (pure sine wave)
        const int testLength = 32768; // ~680ms at 48kHz - enough for stable pitch detection
        juce::AudioBuffer<float> testBuffer(2, testLength);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * M_PI * inputFreq * i / SAMPLE_RATE;
                testBuffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        // Process in blocks
        for (int start = 0; start < testLength; start += BLOCK_SIZE) {
            int samplesThisBlock = std::min(BLOCK_SIZE, testLength - start);
            juce::AudioBuffer<float> block(testBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Skip initial samples to avoid transients (skip first 15%)
        int skipSamples = testLength / 7;
        int analysisSamples = testLength - skipSamples;

        if (analysisSamples < 8192) {
            result.errorMsg = "Insufficient samples for analysis";
            return result;
        }

        juce::AudioBuffer<float> analysisBuffer(2, analysisSamples);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < analysisSamples; ++i) {
                analysisBuffer.setSample(ch, i, testBuffer.getSample(ch, i + skipSamples));
            }
        }

        // Detect output frequency
        result.measuredFreq = detectFundamentalFrequency(analysisBuffer, SAMPLE_RATE);

        if (result.measuredFreq == 0.0f) {
            result.errorMsg = "No frequency detected (silence or no output)";
            return result;
        }

        // Calculate cent error
        result.centError = calculateCentError(result.measuredFreq, result.expectedFreq);

        // Validate measurement
        // Allow for reasonable frequency range (expected ± 2 semitones)
        float minExpected = result.expectedFreq * std::pow(2.0f, -2.0f / 12.0f);
        float maxExpected = result.expectedFreq * std::pow(2.0f, 2.0f / 12.0f);

        if (result.measuredFreq >= minExpected && result.measuredFreq <= maxExpected) {
            result.validMeasurement = true;
        } else {
            result.errorMsg = "Frequency out of expected range";
        }

    } catch (const std::exception& e) {
        result.errorMsg = std::string("Exception: ") + e.what();
    } catch (...) {
        result.errorMsg = "Unknown exception";
    }

    return result;
}

//==============================================================================
// Save results to CSV
//==============================================================================
void saveResultsToCSV(const std::vector<PitchTestResult>& results, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // Write header
    file << "EngineID,EngineName,InputFreq,SemitoneShift,ExpectedFreq,MeasuredFreq,CentError,Valid,ErrorMsg\n";

    // Write data
    for (const auto& result : results) {
        file << result.engineId << ","
             << "\"" << result.engineName << "\","
             << std::fixed << std::setprecision(2) << result.inputFreq << ","
             << result.semitoneShift << ","
             << std::fixed << std::setprecision(2) << result.expectedFreq << ","
             << std::fixed << std::setprecision(2) << result.measuredFreq << ","
             << std::fixed << std::setprecision(2) << result.centError << ","
             << (result.validMeasurement ? "YES" : "NO") << ","
             << "\"" << result.errorMsg << "\"\n";
    }

    file.close();
    std::cout << "Results saved to: " << filename << std::endl;
}

//==============================================================================
// Print detailed results for one engine
//==============================================================================
void printEngineResults(int engineId, const std::vector<PitchTestResult>& results) {
    // Filter results for this engine
    std::vector<PitchTestResult> engineResults;
    for (const auto& r : results) {
        if (r.engineId == engineId) {
            engineResults.push_back(r);
        }
    }

    if (engineResults.empty()) return;

    std::cout << "\n╔════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": "
              << std::left << std::setw(56) << engineResults[0].engineName << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝\n\n";

    // Calculate statistics
    int validCount = 0;
    float sumAbsError = 0.0f;
    float maxAbsError = 0.0f;

    for (const auto& r : engineResults) {
        if (r.validMeasurement) {
            validCount++;
            float absError = std::abs(r.centError);
            sumAbsError += absError;
            maxAbsError = std::max(maxAbsError, absError);
        }
    }

    float avgError = (validCount > 0) ? (sumAbsError / validCount) : 0.0f;

    std::cout << "  Summary Statistics:\n";
    std::cout << "    Valid Measurements: " << validCount << " / " << engineResults.size() << "\n";
    std::cout << "    Average Error:      " << std::fixed << std::setprecision(2) << avgError << " cents\n";
    std::cout << "    Maximum Error:      " << std::fixed << std::setprecision(2) << maxAbsError << " cents\n";

    // Quality assessment
    std::cout << "    Quality Rating:     ";
    if (avgError < 1.0f) {
        std::cout << "PROFESSIONAL (< 1 cent)\n";
    } else if (avgError < 5.0f) {
        std::cout << "EXCELLENT (< 5 cents)\n";
    } else if (avgError < 10.0f) {
        std::cout << "GOOD (< 10 cents)\n";
    } else if (avgError < 20.0f) {
        std::cout << "FAIR (< 20 cents)\n";
    } else {
        std::cout << "POOR (>= 20 cents)\n";
    }
    std::cout << "\n";

    // Print detailed results table
    std::cout << "  Detailed Results:\n";
    std::cout << "    " << std::left << std::setw(10) << "Input"
              << std::setw(10) << "Shift"
              << std::setw(12) << "Expected"
              << std::setw(12) << "Measured"
              << std::setw(12) << "Error"
              << std::setw(8) << "Status" << "\n";
    std::cout << "    " << std::string(64, '-') << "\n";

    for (const auto& r : engineResults) {
        std::cout << "    " << std::fixed << std::setprecision(1)
                  << std::left << std::setw(10) << (std::to_string((int)r.inputFreq) + "Hz")
                  << std::setw(10) << (std::to_string(r.semitoneShift) + "st")
                  << std::setw(12) << (std::to_string((int)r.expectedFreq) + "Hz");

        if (r.validMeasurement) {
            std::cout << std::setw(12) << (std::to_string((int)r.measuredFreq) + "Hz")
                      << std::setprecision(2) << std::setw(12) << (std::to_string(r.centError) + "¢");

            if (std::abs(r.centError) < 5.0f) {
                std::cout << std::setw(8) << "✓ PASS";
            } else if (std::abs(r.centError) < 20.0f) {
                std::cout << std::setw(8) << "⚠ WARN";
            } else {
                std::cout << std::setw(8) << "✗ FAIL";
            }
        } else {
            std::cout << std::setw(12) << "N/A"
                      << std::setw(12) << "N/A"
                      << std::setw(8) << "✗ FAIL";
            if (!r.errorMsg.empty()) {
                std::cout << " (" << r.errorMsg << ")";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

//==============================================================================
// Main function
//==============================================================================
int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           PITCH ACCURACY TEST SUITE FOR PITCH SHIFTER ENGINES         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Test Configuration:\n";
    std::cout << "  Target Engines:    32-38, 49-50\n";
    std::cout << "  Sample Rate:       " << SAMPLE_RATE << " Hz\n";
    std::cout << "  Block Size:        " << BLOCK_SIZE << " samples\n";
    std::cout << "  Test Frequencies:  ";
    for (size_t i = 0; i < TEST_FREQUENCIES.size(); ++i) {
        std::cout << (int)TEST_FREQUENCIES[i] << "Hz";
        if (i < TEST_FREQUENCIES.size() - 1) std::cout << ", ";
    }
    std::cout << "\n";
    std::cout << "  Semitone Shifts:   ";
    for (size_t i = 0; i < SEMITONE_SHIFTS.size(); ++i) {
        std::cout << std::showpos << SEMITONE_SHIFTS[i] << std::noshowpos;
        if (i < SEMITONE_SHIFTS.size() - 1) std::cout << ", ";
    }
    std::cout << "\n\n";

    std::vector<PitchTestResult> allResults;
    std::vector<int> testEngines;

    // Build list of engines to test
    for (int id = 32; id <= 38; ++id) {
        testEngines.push_back(id);
    }
    testEngines.push_back(49);
    testEngines.push_back(50);

    // Progress tracking
    int totalTests = testEngines.size() * TEST_FREQUENCIES.size() * SEMITONE_SHIFTS.size();
    int currentTest = 0;

    std::cout << "═══════════════════════════════════════════════════════════════════════\n";
    std::cout << "  RUNNING TESTS (" << totalTests << " total)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════\n\n";

    // Run tests for each engine
    for (int engineId : testEngines) {
        std::cout << "Testing Engine " << engineId;
        if (PITCH_ENGINES.count(engineId)) {
            std::cout << " (" << PITCH_ENGINES.at(engineId) << ")";
        }
        std::cout << "...\n";

        // Test each frequency
        for (float freq : TEST_FREQUENCIES) {
            // Test each semitone shift
            for (int shift : SEMITONE_SHIFTS) {
                currentTest++;

                // Show progress
                if (currentTest % 10 == 0 || currentTest == totalTests) {
                    std::cout << "  Progress: " << currentTest << " / " << totalTests
                              << " (" << (100 * currentTest / totalTests) << "%)\r" << std::flush;
                }

                PitchTestResult result = testPitchShift(engineId, freq, shift);
                allResults.push_back(result);
            }
        }
        std::cout << "  Progress: " << currentTest << " / " << totalTests
                  << " (100%)   " << std::endl;
    }

    std::cout << "\n═══════════════════════════════════════════════════════════════════════\n";
    std::cout << "  DETAILED RESULTS BY ENGINE\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════\n";

    // Print detailed results for each engine
    for (int engineId : testEngines) {
        printEngineResults(engineId, allResults);
    }

    // Overall summary
    std::cout << "╔════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                          OVERALL SUMMARY                               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝\n\n";

    int totalValid = 0;
    int totalTests_actual = 0;
    float overallSumError = 0.0f;

    for (const auto& r : allResults) {
        totalTests_actual++;
        if (r.validMeasurement) {
            totalValid++;
            overallSumError += std::abs(r.centError);
        }
    }

    float overallAvgError = (totalValid > 0) ? (overallSumError / totalValid) : 0.0f;
    float successRate = (totalTests_actual > 0) ? (100.0f * totalValid / totalTests_actual) : 0.0f;

    std::cout << "  Total Tests:         " << totalTests_actual << "\n";
    std::cout << "  Valid Measurements:  " << totalValid << " ("
              << std::fixed << std::setprecision(1) << successRate << "%)\n";
    std::cout << "  Failed Measurements: " << (totalTests_actual - totalValid) << "\n";
    std::cout << "  Overall Avg Error:   " << std::fixed << std::setprecision(2)
              << overallAvgError << " cents\n\n";

    // Save results
    saveResultsToCSV(allResults, "build/pitch_accuracy_results.csv");

    std::cout << "\n╔════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        TESTING COMPLETE                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝\n\n";

    return (successRate >= 50.0f) ? 0 : 1; // Pass if at least 50% of tests are valid
}

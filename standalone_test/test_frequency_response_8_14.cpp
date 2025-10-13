#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <complex>
#include <algorithm>

/**
 * COMPREHENSIVE FREQUENCY RESPONSE TEST SUITE
 * For Filter & EQ Engines 8-14
 *
 * This test suite:
 * 1. Generates logarithmic sine sweep from 20Hz to 20kHz
 * 2. Measures output amplitude per frequency
 * 3. Plots frequency response curves
 * 4. Verifies filters actually filter (attenuate frequencies)
 * 5. Generates detailed frequency response report
 *
 * Engines tested:
 * 8.  VintageConsoleEQ_Studio
 * 9.  LadderFilter
 * 10. StateVariableFilter
 * 11. FormantFilter
 * 12. EnvelopeFilter (AutoWah)
 * 13. CombResonator
 * 14. VocalFormantFilter
 */

constexpr float PI = 3.14159265358979323846f;
constexpr int SAMPLE_RATE = 48000;
constexpr int BLOCK_SIZE = 512;
constexpr int NUM_TEST_FREQUENCIES = 100;  // More points for detailed curve

//==============================================================================
// FREQUENCY RESPONSE MEASUREMENT STRUCTURE
//==============================================================================

struct FrequencyPoint {
    float frequency;
    float inputLevel;
    float outputLevel;
    float gainDB;
    float phaseShift;
};

struct FrequencyResponse {
    int engineId;
    std::string engineName;
    std::vector<FrequencyPoint> points;
    bool created;
    bool stable;
    bool filtersCorrectly;
    float maxGainDB;
    float minGainDB;
    float cutoffFrequency;  // -3dB point
    float resonancePeakDB;
    std::string errorMessage;
};

//==============================================================================
// SIGNAL GENERATION UTILITIES
//==============================================================================

void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float amplitude, int sampleRate) {
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float phase = 2.0f * PI * frequency * i / sampleRate;
            buffer.setSample(ch, i, amplitude * std::sin(phase));
        }
    }
}

float measureRMS(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples) {
    float sumSquares = 0.0f;
    int channel = 0;  // Measure left channel

    for (int i = startSample; i < startSample + numSamples; ++i) {
        float sample = buffer.getSample(channel, i);
        sumSquares += sample * sample;
    }

    return std::sqrt(sumSquares / numSamples);
}

float measurePeak(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples) {
    float peak = 0.0f;
    int channel = 0;  // Measure left channel

    for (int i = startSample; i < startSample + numSamples; ++i) {
        float sample = std::abs(buffer.getSample(channel, i));
        peak = std::max(peak, sample);
    }

    return peak;
}

//==============================================================================
// LOGARITHMIC FREQUENCY SWEEP GENERATION
//==============================================================================

std::vector<float> generateLogFrequencies(float startFreq, float endFreq, int numPoints) {
    std::vector<float> frequencies;
    float logStart = std::log10(startFreq);
    float logEnd = std::log10(endFreq);
    float logStep = (logEnd - logStart) / (numPoints - 1);

    for (int i = 0; i < numPoints; ++i) {
        float logFreq = logStart + i * logStep;
        frequencies.push_back(std::pow(10.0f, logFreq));
    }

    return frequencies;
}

//==============================================================================
// FREQUENCY RESPONSE MEASUREMENT
//==============================================================================

FrequencyResponse measureFrequencyResponse(int engineId, const std::string& engineName) {
    FrequencyResponse response;
    response.engineId = engineId;
    response.engineName = engineName;
    response.created = false;
    response.stable = true;
    response.filtersCorrectly = false;
    response.maxGainDB = -100.0f;
    response.minGainDB = 100.0f;
    response.cutoffFrequency = 0.0f;
    response.resonancePeakDB = 0.0f;

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ ENGINE " << std::setw(2) << engineId << ": " << std::left << std::setw(44) << engineName << " ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    try {
        // 1. CREATE ENGINE
        std::cout << "  [1/5] Creating engine...";
        auto engine = EngineFactory::createEngine(engineId);

        if (!engine) {
            std::cout << " FAILED (nullptr)\n";
            response.errorMessage = "EngineFactory returned nullptr";
            return response;
        }

        std::cout << " OK\n";
        response.created = true;

        // 2. PREPARE TO PLAY
        std::cout << "  [2/5] Preparing to play (48kHz, 512 samples)...";
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        std::cout << " OK\n";

        // 3. SET PARAMETERS FOR MEASURABLE FILTERING
        std::cout << "  [3/5] Setting filter parameters...";
        std::map<int, float> params;
        int numParams = engine->getNumParameters();

        // Engine-specific parameter setup for clear frequency response
        switch (engineId) {
            case 8:  // VintageConsoleEQ_Studio
                if (numParams > 0) params[0] = 1.0f;   // Mix 100%
                if (numParams > 1) params[1] = 0.7f;   // Low gain +6dB
                if (numParams > 2) params[2] = 0.3f;   // High cut -6dB
                break;

            case 9:  // LadderFilter (Lowpass)
                if (numParams > 0) params[0] = 1.0f;   // Mix 100%
                if (numParams > 1) params[1] = 0.4f;   // Cutoff ~2kHz
                if (numParams > 2) params[2] = 0.6f;   // Resonance moderate
                break;

            case 10:  // StateVariableFilter
                if (numParams > 0) params[0] = 1.0f;   // Mix 100%
                if (numParams > 1) params[1] = 0.35f;  // Cutoff ~1kHz
                if (numParams > 2) params[2] = 0.5f;   // Resonance
                if (numParams > 3) params[3] = 0.0f;   // Mode: Lowpass
                break;

            case 11:  // FormantFilter
                if (numParams > 0) params[0] = 1.0f;   // Mix 100%
                if (numParams > 1) params[1] = 0.5f;   // Vowel "A"
                if (numParams > 2) params[2] = 0.7f;   // Intensity
                break;

            case 12:  // EnvelopeFilter (AutoWah)
                if (numParams > 0) params[0] = 1.0f;   // Mix 100%
                if (numParams > 1) params[1] = 0.5f;   // Sensitivity
                if (numParams > 2) params[2] = 0.6f;   // Resonance
                if (numParams > 3) params[3] = 0.3f;   // Range
                break;

            case 13:  // CombResonator
                if (numParams > 0) params[0] = 1.0f;   // Mix 100%
                if (numParams > 1) params[1] = 0.3f;   // Fundamental ~200Hz
                if (numParams > 2) params[2] = 0.6f;   // Feedback
                break;

            case 14:  // VocalFormantFilter
                if (numParams > 0) params[0] = 1.0f;   // Mix 100%
                if (numParams > 1) params[1] = 0.5f;   // Vowel
                if (numParams > 2) params[2] = 0.0f;   // Gender (male)
                if (numParams > 3) params[3] = 0.7f;   // Intensity
                break;

            default:
                if (numParams > 0) params[0] = 1.0f;   // Mix 100%
                if (numParams > 1) params[1] = 0.5f;
                if (numParams > 2) params[2] = 0.5f;
                break;
        }

        engine->updateParameters(params);
        std::cout << " OK (" << numParams << " params)\n";

        // 4. GENERATE LOGARITHMIC FREQUENCY SWEEP
        std::cout << "  [4/5] Generating frequency sweep (20Hz - 20kHz)...";
        std::vector<float> testFrequencies = generateLogFrequencies(20.0f, 20000.0f, NUM_TEST_FREQUENCIES);
        std::cout << " OK (" << testFrequencies.size() << " points)\n";

        // 5. MEASURE FREQUENCY RESPONSE
        std::cout << "  [5/5] Measuring frequency response...\n";

        const float inputAmplitude = 0.5f;  // -6dB input level
        const int testLength = SAMPLE_RATE / 2;  // 0.5 second per frequency
        const int settleSamples = SAMPLE_RATE / 10;  // 100ms settle time

        int progressCounter = 0;
        for (float freq : testFrequencies) {
            // Progress indicator
            if (++progressCounter % 10 == 0) {
                std::cout << "      Testing " << progressCounter << "/" << testFrequencies.size()
                          << " (" << std::fixed << std::setprecision(1) << freq << " Hz)\r" << std::flush;
            }

            // Reset engine for clean measurement
            engine->reset();
            engine->updateParameters(params);

            // Generate test signal
            juce::AudioBuffer<float> testBuffer(2, testLength);
            generateSineWave(testBuffer, freq, inputAmplitude, SAMPLE_RATE);

            // Process in blocks
            for (int start = 0; start < testLength; start += BLOCK_SIZE) {
                int samplesThisBlock = std::min(BLOCK_SIZE, testLength - start);
                juce::AudioBuffer<float> block(testBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
                engine->process(block);
            }

            // Check for stability
            bool stable = true;
            for (int i = 0; i < testLength; ++i) {
                float sample = testBuffer.getSample(0, i);
                if (std::isnan(sample) || std::isinf(sample) || std::abs(sample) > 10.0f) {
                    stable = false;
                    break;
                }
            }

            if (!stable) {
                response.stable = false;
                response.errorMessage = "Unstable output at " + std::to_string(freq) + " Hz";
                break;
            }

            // Measure output level (after settling)
            float outputRMS = measureRMS(testBuffer, settleSamples, testLength - settleSamples);
            float outputPeak = measurePeak(testBuffer, settleSamples, testLength - settleSamples);

            // Calculate gain
            float gainLinear = outputRMS / inputAmplitude;
            float gainDB = 20.0f * std::log10(gainLinear + 1e-10f);

            // Store measurement
            FrequencyPoint point;
            point.frequency = freq;
            point.inputLevel = inputAmplitude;
            point.outputLevel = outputRMS;
            point.gainDB = gainDB;
            point.phaseShift = 0.0f;  // Phase measurement could be added

            response.points.push_back(point);

            // Track max/min gain
            response.maxGainDB = std::max(response.maxGainDB, gainDB);
            response.minGainDB = std::min(response.minGainDB, gainDB);
        }

        std::cout << "\n      Measurement complete: " << response.points.size() << " frequency points\n";

        // 6. ANALYZE FREQUENCY RESPONSE
        std::cout << "  [6/6] Analyzing filter characteristics...\n";

        // Find -3dB cutoff frequency (for lowpass/highpass)
        float targetDB = response.maxGainDB - 3.0f;
        for (const auto& point : response.points) {
            if (point.gainDB < targetDB && response.cutoffFrequency == 0.0f) {
                response.cutoffFrequency = point.frequency;
            }
        }

        // Find resonance peak
        for (const auto& point : response.points) {
            if (point.gainDB > response.maxGainDB - 0.5f) {
                response.resonancePeakDB = point.gainDB;
            }
        }

        // Verify filtering behavior (significant gain variation)
        float gainRange = response.maxGainDB - response.minGainDB;
        response.filtersCorrectly = (gainRange > 6.0f);  // At least 6dB variation

        std::cout << "      Max gain: " << std::fixed << std::setprecision(2) << response.maxGainDB << " dB\n";
        std::cout << "      Min gain: " << std::fixed << std::setprecision(2) << response.minGainDB << " dB\n";
        std::cout << "      Gain range: " << std::fixed << std::setprecision(2) << gainRange << " dB\n";
        if (response.cutoffFrequency > 0.0f) {
            std::cout << "      Cutoff (-3dB): " << std::fixed << std::setprecision(1) << response.cutoffFrequency << " Hz\n";
        }
        std::cout << "      Filters correctly: " << (response.filtersCorrectly ? "YES" : "NO") << "\n";

    } catch (const std::exception& e) {
        std::cout << "\n  EXCEPTION: " << e.what() << "\n";
        response.errorMessage = std::string("Exception: ") + e.what();
        response.stable = false;
    } catch (...) {
        std::cout << "\n  UNKNOWN EXCEPTION\n";
        response.errorMessage = "Unknown exception";
        response.stable = false;
    }

    return response;
}

//==============================================================================
// CSV EXPORT FOR PLOTTING
//==============================================================================

void exportFrequencyResponseCSV(const FrequencyResponse& response) {
    std::string filename = "frequency_response_engine_" + std::to_string(response.engineId) + ".csv";
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to create CSV file: " << filename << "\n";
        return;
    }

    // CSV Header
    file << "Frequency_Hz,Input_Level,Output_Level,Gain_dB,Phase_Deg\n";

    // CSV Data
    for (const auto& point : response.points) {
        file << std::fixed << std::setprecision(6)
             << point.frequency << ","
             << point.inputLevel << ","
             << point.outputLevel << ","
             << point.gainDB << ","
             << point.phaseShift << "\n";
    }

    file.close();
    std::cout << "  Exported: " << filename << "\n";
}

//==============================================================================
// ASCII PLOT GENERATION
//==============================================================================

void plotFrequencyResponse(const FrequencyResponse& response) {
    if (response.points.empty()) {
        std::cout << "  No data to plot\n";
        return;
    }

    std::cout << "\n  FREQUENCY RESPONSE PLOT:\n";
    std::cout << "  " << std::string(80, '=') << "\n";

    const int plotHeight = 20;
    const int plotWidth = 70;

    // Find dB range for scaling
    float dbMin = response.minGainDB;
    float dbMax = response.maxGainDB;
    float dbRange = dbMax - dbMin;

    if (dbRange < 1.0f) dbRange = 1.0f;  // Avoid division by zero

    // Create plot grid
    std::vector<std::vector<char>> grid(plotHeight, std::vector<char>(plotWidth, ' '));

    // Plot points
    for (size_t i = 0; i < response.points.size(); ++i) {
        const auto& point = response.points[i];

        // X position (log scale)
        float logFreq = std::log10(point.frequency);
        float logMin = std::log10(20.0f);
        float logMax = std::log10(20000.0f);
        int x = static_cast<int>((logFreq - logMin) / (logMax - logMin) * (plotWidth - 1));

        // Y position (linear dB scale)
        int y = plotHeight - 1 - static_cast<int>((point.gainDB - dbMin) / dbRange * (plotHeight - 1));

        if (x >= 0 && x < plotWidth && y >= 0 && y < plotHeight) {
            grid[y][x] = '*';
        }
    }

    // Draw plot
    for (int y = 0; y < plotHeight; ++y) {
        float db = dbMax - (y * dbRange / (plotHeight - 1));
        std::cout << "  " << std::setw(6) << std::fixed << std::setprecision(1) << db << " dB |";

        for (int x = 0; x < plotWidth; ++x) {
            std::cout << grid[y][x];
        }
        std::cout << "|\n";
    }

    // X-axis labels
    std::cout << "  " << std::string(9, ' ') << std::string(plotWidth, '-') << "\n";
    std::cout << "  " << std::string(9, ' ') << "20Hz" << std::string(24, ' ')
              << "1kHz" << std::string(24, ' ') << "20kHz\n";
    std::cout << "  " << std::string(80, '=') << "\n";
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  COMPREHENSIVE FREQUENCY RESPONSE TEST SUITE                 ║\n";
    std::cout << "║  Filter & EQ Engines 8-14                                    ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║  Tests: Sine sweep 20Hz-20kHz (100 points)                  ║\n";
    std::cout << "║  Output: Response curves, CSV data, verification report     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    std::vector<std::pair<int, std::string>> engines = {
        {8, "VintageConsoleEQ_Studio"},
        {9, "LadderFilter"},
        {10, "StateVariableFilter"},
        {11, "FormantFilter"},
        {12, "EnvelopeFilter (AutoWah)"},
        {13, "CombResonator"},
        {14, "VocalFormantFilter"}
    };

    std::vector<FrequencyResponse> allResponses;

    // Test each engine
    for (const auto& [id, name] : engines) {
        FrequencyResponse response = measureFrequencyResponse(id, name);
        allResponses.push_back(response);

        if (response.created && response.stable) {
            plotFrequencyResponse(response);
            exportFrequencyResponseCSV(response);
        }

        std::cout << "\n";
    }

    // SUMMARY REPORT
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    SUMMARY REPORT                            ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "┌────────┬──────────────────────────┬─────────┬────────┬──────────┬──────────┬───────────┐\n";
    std::cout << "│ Engine │ Name                     │ Created │ Stable │ Filters  │ Max Gain │ Min Gain  │\n";
    std::cout << "├────────┼──────────────────────────┼─────────┼────────┼──────────┼──────────┼───────────┤\n";

    int passCount = 0;
    for (const auto& r : allResponses) {
        bool passed = r.created && r.stable && r.filtersCorrectly;
        if (passed) passCount++;

        std::cout << "│ " << std::setw(6) << std::right << r.engineId << " │ "
                  << std::setw(24) << std::left << r.engineName << " │ "
                  << std::setw(7) << (r.created ? "YES" : "NO") << " │ "
                  << std::setw(6) << (r.stable ? "YES" : "NO") << " │ "
                  << std::setw(8) << (r.filtersCorrectly ? "YES" : "NO") << " │ "
                  << std::setw(7) << std::fixed << std::setprecision(2) << r.maxGainDB << "dB │ "
                  << std::setw(8) << std::fixed << std::setprecision(2) << r.minGainDB << "dB │\n";

        if (!r.errorMessage.empty()) {
            std::cout << "│        │ Error: " << std::setw(82) << std::left << r.errorMessage << "│\n";
        }
    }

    std::cout << "└────────┴──────────────────────────┴─────────┴────────┴──────────┴──────────┴───────────┘\n\n";

    // Final statistics
    std::cout << "PASS RATE: " << passCount << "/" << allResponses.size()
              << " (" << (100 * passCount / allResponses.size()) << "%)\n\n";

    // Export combined report
    std::ofstream report("FREQUENCY_RESPONSE_REPORT.txt");
    if (report.is_open()) {
        report << "FREQUENCY RESPONSE TEST REPORT\n";
        report << "==============================\n\n";
        report << "Test Date: " << __DATE__ << " " << __TIME__ << "\n";
        report << "Sample Rate: " << SAMPLE_RATE << " Hz\n";
        report << "Block Size: " << BLOCK_SIZE << " samples\n";
        report << "Test Frequencies: " << NUM_TEST_FREQUENCIES << " points (20Hz - 20kHz)\n\n";

        for (const auto& r : allResponses) {
            report << "\n----------------------------------------\n";
            report << "ENGINE " << r.engineId << ": " << r.engineName << "\n";
            report << "----------------------------------------\n";
            report << "Created: " << (r.created ? "YES" : "NO") << "\n";
            report << "Stable: " << (r.stable ? "YES" : "NO") << "\n";
            report << "Filters Correctly: " << (r.filtersCorrectly ? "YES" : "NO") << "\n";
            report << "Max Gain: " << std::fixed << std::setprecision(2) << r.maxGainDB << " dB\n";
            report << "Min Gain: " << std::fixed << std::setprecision(2) << r.minGainDB << " dB\n";
            report << "Gain Range: " << std::fixed << std::setprecision(2)
                   << (r.maxGainDB - r.minGainDB) << " dB\n";
            if (r.cutoffFrequency > 0.0f) {
                report << "Cutoff (-3dB): " << std::fixed << std::setprecision(1)
                       << r.cutoffFrequency << " Hz\n";
            }
            if (!r.errorMessage.empty()) {
                report << "Error: " << r.errorMessage << "\n";
            }
        }

        report << "\n\nSUMMARY\n";
        report << "=======\n";
        report << "Engines Tested: " << allResponses.size() << "\n";
        report << "Engines Passed: " << passCount << "\n";
        report << "Pass Rate: " << (100 * passCount / allResponses.size()) << "%\n";

        report.close();
        std::cout << "Report saved: FREQUENCY_RESPONSE_REPORT.txt\n";
    }

    if (passCount == static_cast<int>(allResponses.size())) {
        std::cout << "\n✓ ALL TESTS PASSED\n\n";
        return 0;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED\n\n";
        return 1;
    }
}

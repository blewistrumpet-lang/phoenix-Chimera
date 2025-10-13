/**
 * test_dc_offset.cpp
 *
 * DC Offset Handling Test for All Audio Engines
 *
 * Tests how each engine handles DC offset (constant 0.5 signal)
 * Effects should not amplify DC - ideally they should block or pass it unchanged
 *
 * This test:
 * 1. Feeds DC offset (0.5 constant) into each engine
 * 2. Measures output DC level
 * 3. Identifies engines that amplify DC (problematic)
 * 4. Generates CSV report for analysis
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <map>
#include <string>

#include "JuceHeader.h"
#include "EngineBase.h"
#include "EngineFactory.h"
#include "EngineTypes.h"

//==============================================================================
// Configuration
//==============================================================================
constexpr double SAMPLE_RATE = 48000.0;
constexpr int BLOCK_SIZE = 512;
constexpr int NUM_CHANNELS = 2;
constexpr int WARMUP_BLOCKS = 10;        // Blocks to warm up the engine
constexpr int TEST_BLOCKS = 100;         // Blocks to analyze
constexpr float DC_OFFSET = 0.5f;        // DC offset value to test

// All engine IDs to test (0-56 range, as defined in EngineFactory)
const std::vector<int> ALL_ENGINE_IDS = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56
};

//==============================================================================
// DC Offset Test Result
//==============================================================================
struct DCTestResult {
    int engineId;
    std::string engineName;
    bool engineCreated;

    // DC measurements
    float inputDC;
    float outputDC;
    float dcGain;              // outputDC / inputDC
    float dcAmplification;     // Linear gain
    float dcAmplificationDB;   // dB

    // Quality checks
    bool hasNaN;
    bool hasInf;
    bool amplifiedDC;          // DC gain > 1.1 (problematic)
    bool removedDC;            // DC gain < 0.1 (good DC blocking)
    bool passedDC;             // DC gain ~= 1.0 (neutral)

    // Status
    std::string status;        // "PASS", "WARN", "FAIL"
    std::string recommendation; // What to do about this engine

    DCTestResult() : engineId(0), engineCreated(false),
                     inputDC(0), outputDC(0), dcGain(0),
                     dcAmplification(0), dcAmplificationDB(0),
                     hasNaN(false), hasInf(false),
                     amplifiedDC(false), removedDC(false), passedDC(false) {}
};

//==============================================================================
// Signal Generation
//==============================================================================
void generateDCOffset(juce::AudioBuffer<float>& buffer, float dcValue) {
    // Fill buffer with constant DC value
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = dcValue;
        }
    }
}

//==============================================================================
// Analysis Functions
//==============================================================================
float calculateMean(const juce::AudioBuffer<float>& buffer) {
    double sum = 0.0;
    int totalSamples = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sum += data[i];
        }
        totalSamples += buffer.getNumSamples();
    }

    return totalSamples > 0 ? static_cast<float>(sum / totalSamples) : 0.0f;
}

bool hasNaN(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::isnan(data[i])) return true;
        }
    }
    return false;
}

bool hasInf(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::isinf(data[i])) return true;
        }
    }
    return false;
}

//==============================================================================
// DC Offset Test
//==============================================================================
DCTestResult testEngineDC(int engineId) {
    DCTestResult result;
    result.engineId = engineId;
    result.inputDC = DC_OFFSET;

    try {
        // Create engine
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.engineCreated = false;
            result.engineName = "Unknown (Creation Failed)";
            result.status = "FAIL";
            result.recommendation = "Engine creation failed";
            return result;
        }

        result.engineCreated = true;
        result.engineName = engine->getName().toStdString();

        // Prepare engine
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        engine->reset();

        // Set default parameters (dry/wet to 100% wet for most engines)
        std::map<int, float> params;
        // This is a simplified approach - in reality we'd want to set
        // engine-specific parameters to ensure full effect processing
        engine->updateParameters(params);

        // Create buffers
        juce::AudioBuffer<float> buffer(NUM_CHANNELS, BLOCK_SIZE);

        // Warmup phase - let engine settle
        for (int block = 0; block < WARMUP_BLOCKS; ++block) {
            generateDCOffset(buffer, DC_OFFSET);
            engine->process(buffer);
        }

        // Test phase - measure DC output
        double dcSum = 0.0;
        int measurementCount = 0;

        for (int block = 0; block < TEST_BLOCKS; ++block) {
            generateDCOffset(buffer, DC_OFFSET);
            engine->process(buffer);

            // Check for NaN/Inf
            if (hasNaN(buffer)) {
                result.hasNaN = true;
                break;
            }
            if (hasInf(buffer)) {
                result.hasInf = true;
                break;
            }

            // Accumulate DC measurement (mean value)
            dcSum += calculateMean(buffer);
            measurementCount++;
        }

        // Calculate average DC output
        if (measurementCount > 0) {
            result.outputDC = static_cast<float>(dcSum / measurementCount);
        }

        // Calculate DC gain
        if (result.inputDC > 0.0001f) {
            result.dcGain = result.outputDC / result.inputDC;
            result.dcAmplification = result.dcGain;

            // Calculate dB (protect against log of zero/negative)
            if (result.dcGain > 0.0001f) {
                result.dcAmplificationDB = 20.0f * std::log10(result.dcGain);
            } else {
                result.dcAmplificationDB = -120.0f; // Essentially removed
            }
        }

        // Categorize DC behavior
        if (result.hasNaN || result.hasInf) {
            result.status = "FAIL";
            result.recommendation = "Engine produces NaN/Inf - needs immediate fix";
        }
        else if (result.dcGain > 1.1f) {
            // DC amplified - this is bad
            result.amplifiedDC = true;
            result.status = "FAIL";
            result.recommendation = "Add DC blocking filter (high-pass at ~20Hz)";
        }
        else if (result.dcGain < 0.1f) {
            // DC removed - this is good
            result.removedDC = true;
            result.status = "PASS";
            result.recommendation = "Good - DC already blocked";
        }
        else if (result.dcGain >= 0.9f && result.dcGain <= 1.1f) {
            // DC passed through - neutral (acceptable for some engines)
            result.passedDC = true;
            result.status = "WARN";
            result.recommendation = "Consider adding DC blocking filter";
        }
        else {
            // DC attenuated but not fully removed
            result.status = "PASS";
            result.recommendation = "DC attenuated - acceptable";
        }

    } catch (const std::exception& e) {
        result.engineCreated = false;
        result.status = "FAIL";
        result.recommendation = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Report Generation
//==============================================================================
void printHeader() {
    std::cout << "\n";
    std::cout << "════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "                        DC OFFSET HANDLING TEST\n";
    std::cout << "                     Testing All Audio Engines (0-56)\n";
    std::cout << "════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "\nTest Configuration:\n";
    std::cout << "  • Input DC Offset:  " << DC_OFFSET << "\n";
    std::cout << "  • Sample Rate:      " << SAMPLE_RATE << " Hz\n";
    std::cout << "  • Block Size:       " << BLOCK_SIZE << " samples\n";
    std::cout << "  • Warmup Blocks:    " << WARMUP_BLOCKS << "\n";
    std::cout << "  • Test Blocks:      " << TEST_BLOCKS << "\n";
    std::cout << "\n";
}

void printResults(const std::vector<DCTestResult>& results) {
    std::cout << "════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "                              TEST RESULTS\n";
    std::cout << "════════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << std::left << std::setw(4) << "ID"
              << std::setw(35) << "Engine"
              << std::right << std::setw(10) << "In DC"
              << std::setw(10) << "Out DC"
              << std::setw(10) << "Gain"
              << std::setw(10) << "Gain dB"
              << std::left << std::setw(10) << "  Status" << "\n";
    std::cout << "────────────────────────────────────────────────────────────────────────────────\n";

    for (const auto& result : results) {
        if (!result.engineCreated) {
            std::cout << std::left << std::setw(4) << result.engineId
                      << std::setw(35) << result.engineName
                      << std::right << std::setw(10) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::left << "  FAIL\n";
            continue;
        }

        std::cout << std::left << std::setw(4) << result.engineId
                  << std::setw(35) << result.engineName.substr(0, 34)
                  << std::right << std::fixed << std::setprecision(4)
                  << std::setw(10) << result.inputDC
                  << std::setw(10) << result.outputDC
                  << std::setw(10) << result.dcGain
                  << std::setw(10) << std::setprecision(2) << result.dcAmplificationDB
                  << std::left << "  " << result.status << "\n";

        if (result.hasNaN) {
            std::cout << "     └─ ⚠ Contains NaN\n";
        }
        if (result.hasInf) {
            std::cout << "     └─ ⚠ Contains Inf\n";
        }
    }
    std::cout << "\n";
}

void printSummary(const std::vector<DCTestResult>& results) {
    int total = results.size();
    int passed = 0;
    int warned = 0;
    int failed = 0;
    int dcAmplifiers = 0;
    int dcBlockers = 0;
    int dcPassers = 0;

    for (const auto& result : results) {
        if (result.status == "PASS") passed++;
        else if (result.status == "WARN") warned++;
        else if (result.status == "FAIL") failed++;

        if (result.amplifiedDC) dcAmplifiers++;
        if (result.removedDC) dcBlockers++;
        if (result.passedDC) dcPassers++;
    }

    std::cout << "════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "                                SUMMARY\n";
    std::cout << "════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  Total Engines:        " << total << "\n";
    std::cout << "  Passed:               " << passed << " (" << (100.0f * passed / total) << "%)\n";
    std::cout << "  Warnings:             " << warned << " (" << (100.0f * warned / total) << "%)\n";
    std::cout << "  Failed:               " << failed << " (" << (100.0f * failed / total) << "%)\n";
    std::cout << "\n";
    std::cout << "  DC Behavior:\n";
    std::cout << "    • Amplify DC:       " << dcAmplifiers << " (PROBLEMATIC - needs DC blocking)\n";
    std::cout << "    • Block DC:         " << dcBlockers << " (GOOD - already have DC blocking)\n";
    std::cout << "    • Pass DC:          " << dcPassers << " (NEUTRAL - consider DC blocking)\n";
    std::cout << "\n";
}

void printRecommendations(const std::vector<DCTestResult>& results) {
    std::cout << "════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "                            RECOMMENDATIONS\n";
    std::cout << "════════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << "Engines Requiring DC Blocking (CRITICAL):\n";
    bool hasCritical = false;
    for (const auto& result : results) {
        if (result.amplifiedDC) {
            std::cout << "  • Engine " << result.engineId << " (" << result.engineName << ")\n";
            std::cout << "    - DC Gain: " << result.dcGain << " (" << std::fixed
                      << std::setprecision(2) << result.dcAmplificationDB << " dB)\n";
            std::cout << "    - " << result.recommendation << "\n\n";
            hasCritical = true;
        }
    }
    if (!hasCritical) {
        std::cout << "  None - all engines handle DC appropriately\n\n";
    }

    std::cout << "Engines Passing DC Through (CONSIDER BLOCKING):\n";
    bool hasWarning = false;
    for (const auto& result : results) {
        if (result.passedDC) {
            std::cout << "  • Engine " << result.engineId << " (" << result.engineName << ")\n";
            std::cout << "    - DC Gain: " << result.dcGain << " (" << std::fixed
                      << std::setprecision(2) << result.dcAmplificationDB << " dB)\n";
            std::cout << "    - " << result.recommendation << "\n\n";
            hasWarning = true;
        }
    }
    if (!hasWarning) {
        std::cout << "  None\n\n";
    }

    std::cout << "Engines with Good DC Blocking:\n";
    int goodCount = 0;
    for (const auto& result : results) {
        if (result.removedDC) {
            goodCount++;
        }
    }
    std::cout << "  " << goodCount << " engines already have effective DC blocking filters\n\n";
}

void saveCSVReport(const std::vector<DCTestResult>& results, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open " << filename << " for writing\n";
        return;
    }

    // Header
    file << "EngineID,EngineName,Created,InputDC,OutputDC,DCGain,DCGain_dB,"
         << "HasNaN,HasInf,AmplifiedDC,RemovedDC,PassedDC,Status,Recommendation\n";

    // Data
    for (const auto& result : results) {
        file << result.engineId << ","
             << "\"" << result.engineName << "\","
             << (result.engineCreated ? "Yes" : "No") << ","
             << result.inputDC << ","
             << result.outputDC << ","
             << result.dcGain << ","
             << result.dcAmplificationDB << ","
             << (result.hasNaN ? "Yes" : "No") << ","
             << (result.hasInf ? "Yes" : "No") << ","
             << (result.amplifiedDC ? "Yes" : "No") << ","
             << (result.removedDC ? "Yes" : "No") << ","
             << (result.passedDC ? "Yes" : "No") << ","
             << result.status << ","
             << "\"" << result.recommendation << "\"\n";
    }

    file.close();
    std::cout << "CSV report saved: " << filename << "\n";
}

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[]) {
    printHeader();

    // Test all engines
    std::vector<DCTestResult> results;

    std::cout << "Testing engines...\n";
    for (int engineId : ALL_ENGINE_IDS) {
        std::cout << "  Testing Engine " << engineId << "..." << std::flush;
        auto result = testEngineDC(engineId);
        results.push_back(result);
        std::cout << " " << result.status << "\n";
    }

    // Print results
    printResults(results);
    printSummary(results);
    printRecommendations(results);

    // Save CSV
    saveCSVReport(results, "dc_offset_test_results.csv");

    std::cout << "════════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "DC offset test complete!\n\n";

    // Return exit code based on critical failures
    int criticalFailures = 0;
    for (const auto& result : results) {
        if (result.amplifiedDC || result.hasNaN || result.hasInf) {
            criticalFailures++;
        }
    }

    return (criticalFailures == 0) ? 0 : 1;
}

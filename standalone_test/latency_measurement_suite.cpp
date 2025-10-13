#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

/**
 * Comprehensive Latency Measurement Suite
 *
 * Tests ALL pitch shifters, reverbs, and time-based effects for latency.
 * Measures samples from impulse to first output above threshold.
 * Generates detailed latency report with samples and milliseconds for each engine.
 *
 * Categories tested:
 * - Pitch Shifters: 31, 32, 33, 49
 * - Reverbs: 39, 40, 41, 42, 43
 * - Delays/Time-based: 34, 35, 36, 37, 38
 */

namespace LatencyMeasurement {

//==============================================================================
// Latency Result Structure
//==============================================================================
struct LatencyResult {
    int engineId;
    std::string engineName;
    std::string category;

    // Latency measurements
    int latencySamples = 0;
    float latencyMs = 0.0f;

    // Additional analysis
    bool hasOutput = false;
    bool isConstant = true;  // Does latency vary with parameters?
    float firstPeakAmplitude = 0.0f;
    int firstPeakSample = 0;

    // Quality checks
    bool passesThreshold = false;
    bool isStable = true;
    std::string notes;
};

//==============================================================================
// Precise Latency Measurement
//==============================================================================
LatencyResult measureEngineLatency(int engineId, const std::string& name,
                                   const std::string& category,
                                   float sampleRate = 48000.0f,
                                   int blockSize = 512) {
    LatencyResult result;
    result.engineId = engineId;
    result.engineName = name;
    result.category = category;

    std::cout << "\n[Measuring Engine " << engineId << ": " << name << "]\n";

    // Create engine
    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        result.notes = "Failed to create engine";
        std::cout << "  ERROR: Could not create engine\n";
        return result;
    }

    engine->prepareToPlay(sampleRate, blockSize);

    // Set parameters for maximum wet signal
    std::map<int, float> params;
    int numParams = engine->getNumParameters();

    // Common parameter setup
    // Most engines: param 0 is usually mix or time
    // Set to values that maximize output for latency detection
    for (int i = 0; i < numParams; ++i) {
        params[i] = 0.5f; // Default middle value
    }

    // Category-specific parameter setup
    if (category == "Reverb") {
        if (numParams > 0) params[0] = 1.0f;  // Mix = 100% wet
        if (numParams > 1) params[1] = 0.5f;  // Decay/Size moderate
        if (numParams > 2) params[2] = 0.3f;  // Damping low
        if (numParams > 3) params[3] = 0.5f;  // Additional parameter
    } else if (category == "Delay") {
        if (numParams > 0) params[0] = 0.2f;  // Time = short for clear detection
        if (numParams > 1) params[1] = 0.0f;  // Feedback = 0
        if (numParams > 2) params[2] = 1.0f;  // Mix = 100% wet
    } else if (category == "Pitch") {
        if (numParams > 0) params[0] = 0.5f;  // Pitch = unity (no shift)
        if (numParams > 1) params[1] = 1.0f;  // Mix = 100% wet
    }

    engine->updateParameters(params);

    // Create test buffer - long enough to capture latency up to 1 second
    const int maxLatencySamples = static_cast<int>(sampleRate * 1.0f);
    juce::AudioBuffer<float> buffer(2, maxLatencySamples);
    buffer.clear();

    // Create impulse at sample 0
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    // Process in blocks
    for (int start = 0; start < maxLatencySamples; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, maxLatencySamples - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Analyze output to find first sample above threshold
    const float threshold = 0.001f; // -60dB threshold for detection
    const float* leftData = buffer.getReadPointer(0);
    const float* rightData = buffer.getReadPointer(1);

    int firstSampleLeft = -1;
    int firstSampleRight = -1;
    float firstPeakLeft = 0.0f;
    float firstPeakRight = 0.0f;

    // Find first output on left channel
    for (int i = 0; i < maxLatencySamples; ++i) {
        float absVal = std::abs(leftData[i]);
        if (absVal > threshold) {
            firstSampleLeft = i;
            firstPeakLeft = absVal;
            break;
        }
    }

    // Find first output on right channel
    for (int i = 0; i < maxLatencySamples; ++i) {
        float absVal = std::abs(rightData[i]);
        if (absVal > threshold) {
            firstSampleRight = i;
            firstPeakRight = absVal;
            break;
        }
    }

    // Use the earlier of the two channels
    if (firstSampleLeft >= 0 && firstSampleRight >= 0) {
        result.latencySamples = std::min(firstSampleLeft, firstSampleRight);
        result.firstPeakAmplitude = std::max(firstPeakLeft, firstPeakRight);
        result.hasOutput = true;
    } else if (firstSampleLeft >= 0) {
        result.latencySamples = firstSampleLeft;
        result.firstPeakAmplitude = firstPeakLeft;
        result.hasOutput = true;
    } else if (firstSampleRight >= 0) {
        result.latencySamples = firstSampleRight;
        result.firstPeakAmplitude = firstPeakRight;
        result.hasOutput = true;
    } else {
        result.hasOutput = false;
        result.notes = "No output detected above threshold";
    }

    result.firstPeakSample = result.latencySamples;
    result.latencyMs = (result.latencySamples * 1000.0f) / sampleRate;
    result.passesThreshold = result.hasOutput && result.firstPeakAmplitude > threshold;

    // Check for stability (no NaN/Inf in output)
    for (int i = 0; i < maxLatencySamples && result.isStable; ++i) {
        if (!std::isfinite(leftData[i]) || !std::isfinite(rightData[i])) {
            result.isStable = false;
            result.notes += "Unstable output (NaN/Inf detected)";
        }
    }

    // Calculate RMS of output (after latency) for verification
    if (result.hasOutput) {
        float rms = 0.0f;
        int rmsLength = std::min(4800, maxLatencySamples - result.latencySamples); // 100ms
        for (int i = result.latencySamples; i < result.latencySamples + rmsLength; ++i) {
            float sample = leftData[i];
            rms += sample * sample;
        }
        rms = std::sqrt(rms / rmsLength);

        if (rms < 0.0001f) {
            result.notes += "Very low output level (RMS < -80dB)";
        }
    }

    // Print immediate results
    if (result.hasOutput) {
        std::cout << "  Latency: " << result.latencySamples << " samples ("
                  << std::fixed << std::setprecision(3) << result.latencyMs << " ms)\n";
        std::cout << "  First peak: " << std::fixed << std::setprecision(6)
                  << result.firstPeakAmplitude << " at sample " << result.firstPeakSample << "\n";
    } else {
        std::cout << "  WARNING: No output detected\n";
    }

    if (!result.notes.empty()) {
        std::cout << "  Notes: " << result.notes << "\n";
    }

    return result;
}

//==============================================================================
// Test Latency Consistency Across Parameters
//==============================================================================
void testLatencyConsistency(int engineId, const std::string& name,
                           LatencyResult& result,
                           float sampleRate = 48000.0f) {
    std::cout << "  Testing latency consistency...\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) return;

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    std::vector<int> latencies;

    // Test with different parameter values
    std::vector<float> testValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

    for (float paramValue : testValues) {
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        for (int i = 0; i < numParams; ++i) {
            params[i] = 0.5f;
        }
        if (numParams > 0) params[0] = paramValue;
        if (numParams > 1) params[1] = 1.0f; // Mix high

        engine->reset();
        engine->updateParameters(params);

        const int testLength = static_cast<int>(sampleRate * 0.5f);
        juce::AudioBuffer<float> buffer(2, testLength);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);

        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Find first output
        const float* data = buffer.getReadPointer(0);
        for (int i = 0; i < testLength; ++i) {
            if (std::abs(data[i]) > 0.001f) {
                latencies.push_back(i);
                break;
            }
        }
    }

    // Check consistency
    if (!latencies.empty()) {
        int minLatency = *std::min_element(latencies.begin(), latencies.end());
        int maxLatency = *std::max_element(latencies.begin(), latencies.end());
        int variation = maxLatency - minLatency;

        result.isConstant = (variation < 100); // Less than 100 samples variation

        if (!result.isConstant) {
            std::cout << "  Latency varies: " << minLatency << " to " << maxLatency
                      << " samples (variation: " << variation << ")\n";
            result.notes += " Variable latency";
        } else {
            std::cout << "  Latency is constant across parameters\n";
        }
    }
}

//==============================================================================
// Generate Report
//==============================================================================
void generateLatencyReport(const std::vector<LatencyResult>& results,
                          float sampleRate) {
    std::cout << "\n\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              LATENCY MEASUREMENT REPORT - ALL ENGINES                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\nSample Rate: " << sampleRate << " Hz\n";
    std::cout << "Detection Threshold: -60 dB (0.001 linear)\n";
    std::cout << "\n";

    // Group by category
    std::map<std::string, std::vector<LatencyResult>> byCategory;
    for (const auto& r : results) {
        byCategory[r.category].push_back(r);
    }

    // Print each category
    for (const auto& [category, categoryResults] : byCategory) {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << category << " ENGINES\n";
        std::cout << std::string(80, '=') << "\n\n";

        std::cout << std::left << std::setw(4) << "ID"
                  << std::setw(38) << "Engine Name"
                  << std::right << std::setw(10) << "Samples"
                  << std::setw(12) << "ms"
                  << std::setw(12) << "Constant"
                  << std::left << std::setw(10) << "  Status" << "\n";
        std::cout << std::string(80, '-') << "\n";

        for (const auto& r : categoryResults) {
            std::cout << std::left << std::setw(4) << r.engineId
                      << std::setw(38) << r.engineName
                      << std::right << std::setw(10) << r.latencySamples
                      << std::setw(12) << std::fixed << std::setprecision(3) << r.latencyMs
                      << std::setw(12) << (r.isConstant ? "Yes" : "Variable")
                      << "  ";

            if (r.hasOutput && r.isStable) {
                std::cout << "✓ OK";
            } else if (!r.hasOutput) {
                std::cout << "✗ NO OUTPUT";
            } else {
                std::cout << "⚠ UNSTABLE";
            }
            std::cout << "\n";

            if (!r.notes.empty()) {
                std::cout << "      Notes: " << r.notes << "\n";
            }
        }

        // Category statistics
        std::vector<int> validLatencies;
        for (const auto& r : categoryResults) {
            if (r.hasOutput) {
                validLatencies.push_back(r.latencySamples);
            }
        }

        if (!validLatencies.empty()) {
            int minLat = *std::min_element(validLatencies.begin(), validLatencies.end());
            int maxLat = *std::max_element(validLatencies.begin(), validLatencies.end());
            float avgLat = std::accumulate(validLatencies.begin(), validLatencies.end(), 0.0f)
                          / validLatencies.size();

            std::cout << "\n  Category Statistics:\n";
            std::cout << "    Min Latency: " << minLat << " samples ("
                      << std::fixed << std::setprecision(3) << (minLat * 1000.0f / sampleRate) << " ms)\n";
            std::cout << "    Max Latency: " << maxLat << " samples ("
                      << std::fixed << std::setprecision(3) << (maxLat * 1000.0f / sampleRate) << " ms)\n";
            std::cout << "    Avg Latency: " << std::fixed << std::setprecision(1) << avgLat << " samples ("
                      << std::fixed << std::setprecision(3) << (avgLat * 1000.0f / sampleRate) << " ms)\n";
        }
    }

    // Overall summary
    std::cout << "\n\n" << std::string(80, '=') << "\n";
    std::cout << "OVERALL SUMMARY\n";
    std::cout << std::string(80, '=') << "\n\n";

    int totalEngines = results.size();
    int enginesWithOutput = 0;
    int stableEngines = 0;
    int constantLatency = 0;

    for (const auto& r : results) {
        if (r.hasOutput) enginesWithOutput++;
        if (r.isStable) stableEngines++;
        if (r.isConstant) constantLatency++;
    }

    std::cout << "Total Engines Tested:      " << totalEngines << "\n";
    std::cout << "Engines With Output:       " << enginesWithOutput
              << " (" << std::fixed << std::setprecision(1)
              << (enginesWithOutput * 100.0f / totalEngines) << "%)\n";
    std::cout << "Stable Engines:            " << stableEngines
              << " (" << std::fixed << std::setprecision(1)
              << (stableEngines * 100.0f / totalEngines) << "%)\n";
    std::cout << "Constant Latency Engines:  " << constantLatency
              << " (" << std::fixed << std::setprecision(1)
              << (constantLatency * 100.0f / totalEngines) << "%)\n";

    // Find interesting cases
    std::cout << "\n" << std::string(80, '-') << "\n";
    std::cout << "NOTABLE CASES\n";
    std::cout << std::string(80, '-') << "\n\n";

    // Lowest latency
    auto minResult = std::min_element(results.begin(), results.end(),
        [](const LatencyResult& a, const LatencyResult& b) {
            if (!a.hasOutput) return false;
            if (!b.hasOutput) return true;
            return a.latencySamples < b.latencySamples;
        });

    if (minResult != results.end() && minResult->hasOutput) {
        std::cout << "Lowest Latency:\n";
        std::cout << "  Engine " << minResult->engineId << " (" << minResult->engineName << ")\n";
        std::cout << "  " << minResult->latencySamples << " samples ("
                  << std::fixed << std::setprecision(3) << minResult->latencyMs << " ms)\n\n";
    }

    // Highest latency
    auto maxResult = std::max_element(results.begin(), results.end(),
        [](const LatencyResult& a, const LatencyResult& b) {
            if (!a.hasOutput) return true;
            if (!b.hasOutput) return false;
            return a.latencySamples < b.latencySamples;
        });

    if (maxResult != results.end() && maxResult->hasOutput) {
        std::cout << "Highest Latency:\n";
        std::cout << "  Engine " << maxResult->engineId << " (" << maxResult->engineName << ")\n";
        std::cout << "  " << maxResult->latencySamples << " samples ("
                  << std::fixed << std::setprecision(3) << maxResult->latencyMs << " ms)\n\n";
    }

    // Problematic engines
    bool hasProblems = false;
    for (const auto& r : results) {
        if (!r.hasOutput || !r.isStable) {
            if (!hasProblems) {
                std::cout << "Problematic Engines:\n";
                hasProblems = true;
            }
            std::cout << "  Engine " << r.engineId << " (" << r.engineName << "): ";
            if (!r.hasOutput) std::cout << "NO OUTPUT";
            else if (!r.isStable) std::cout << "UNSTABLE";
            std::cout << "\n";
        }
    }

    if (!hasProblems) {
        std::cout << "No problematic engines detected - All engines producing stable output!\n";
    }

    std::cout << "\n";
}

//==============================================================================
// Save CSV Report
//==============================================================================
void saveCSVReport(const std::vector<LatencyResult>& results,
                   const std::string& filename = "latency_report.csv") {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "ERROR: Could not create CSV file: " << filename << "\n";
        return;
    }

    // Write header
    file << "EngineID,EngineName,Category,LatencySamples,LatencyMs,HasOutput,IsStable,IsConstant,FirstPeakAmp,Notes\n";

    // Write data
    for (const auto& r : results) {
        file << r.engineId << ","
             << "\"" << r.engineName << "\","
             << "\"" << r.category << "\","
             << r.latencySamples << ","
             << std::fixed << std::setprecision(6) << r.latencyMs << ","
             << (r.hasOutput ? "Yes" : "No") << ","
             << (r.isStable ? "Yes" : "No") << ","
             << (r.isConstant ? "Yes" : "No") << ","
             << std::scientific << std::setprecision(6) << r.firstPeakAmplitude << ","
             << "\"" << r.notes << "\"\n";
    }

    file.close();
    std::cout << "\nCSV report saved to: " << filename << "\n";
}

} // namespace LatencyMeasurement

//==============================================================================
// Main Entry Point
//==============================================================================
int main(int argc, char* argv[]) {
    using namespace LatencyMeasurement;

    std::cout << "╔════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           ChimeraPhoenix Comprehensive Latency Measurement Suite           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\nTesting all pitch shifters, reverbs, and time-based effects...\n";

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    std::vector<LatencyResult> allResults;

    // Define all engines to test
    struct EngineInfo {
        int id;
        std::string name;
        std::string category;
    };

    std::vector<EngineInfo> engines = {
        // Pitch Shifters
        {31, "Detune Doubler", "Pitch"},
        {32, "Pitch Shifter", "Pitch"},
        {33, "Intelligent Harmonizer", "Pitch"},
        {49, "Pitch Shifter (Alt)", "Pitch"},

        // Reverbs
        {39, "Convolution Reverb", "Reverb"},
        {40, "Shimmer Reverb", "Reverb"},
        {41, "Plate Reverb", "Reverb"},
        {42, "Spring Reverb", "Reverb"},
        {43, "Gated Reverb", "Reverb"},

        // Delays / Time-based
        {34, "Tape Echo", "Delay"},
        {35, "Digital Delay", "Delay"},
        {36, "Magnetic Drum Echo", "Delay"},
        {37, "Bucket Brigade Delay", "Delay"},
        {38, "Buffer Repeat Platinum", "Delay"}
    };

    // Test each engine
    for (const auto& info : engines) {
        try {
            auto result = measureEngineLatency(info.id, info.name, info.category,
                                               sampleRate, blockSize);

            // Test latency consistency (only if engine has output)
            if (result.hasOutput) {
                testLatencyConsistency(info.id, info.name, result, sampleRate);
            }

            allResults.push_back(result);

        } catch (const std::exception& e) {
            std::cout << "  EXCEPTION: " << e.what() << "\n";
            LatencyResult errorResult;
            errorResult.engineId = info.id;
            errorResult.engineName = info.name;
            errorResult.category = info.category;
            errorResult.hasOutput = false;
            errorResult.isStable = false;
            errorResult.notes = std::string("Exception: ") + e.what();
            allResults.push_back(errorResult);
        } catch (...) {
            std::cout << "  UNKNOWN EXCEPTION\n";
            LatencyResult errorResult;
            errorResult.engineId = info.id;
            errorResult.engineName = info.name;
            errorResult.category = info.category;
            errorResult.hasOutput = false;
            errorResult.isStable = false;
            errorResult.notes = "Unknown exception caught";
            allResults.push_back(errorResult);
        }
    }

    // Generate comprehensive report
    generateLatencyReport(allResults, sampleRate);

    // Save CSV report
    saveCSVReport(allResults, "latency_report.csv");

    std::cout << "\n╔════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         TESTING COMPLETE                                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}

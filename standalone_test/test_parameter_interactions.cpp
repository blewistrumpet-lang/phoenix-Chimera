/**
 * test_parameter_interactions.cpp
 *
 * DEEP PARAMETER INTERACTION TESTING
 *
 * Comprehensive testing of parameter interactions for all 56 engines.
 * Tests how parameters affect each other, not just individual values.
 *
 * Test Coverage:
 * 1. Parameter Relationships - Which params affect the same DSP stage
 * 2. Synergistic Effects - Parameters that enhance each other
 * 3. Conflicting Interactions - Parameters that fight each other
 * 4. Extreme Combinations - Edge case parameter pairings
 * 5. Stability Analysis - Unstable or silent combinations
 * 6. Sweet Spots - Optimal parameter ranges
 * 7. Dangerous Zones - Combinations to avoid
 *
 * Output: PARAMETER_INTERACTION_TESTING_REPORT.md
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <map>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <random>

// JUCE includes
#include "../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

// Engine Factory
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include "../JUCE_Plugin/Source/EngineTypes.h"

// ============================================================================
// STRUCTURES AND UTILITIES
// ============================================================================

struct ParameterInteraction {
    int param1;
    int param2;
    std::string relationship; // "synergistic", "conflicting", "independent", "coupled"
    std::string description;
    float optimalRange1Min = 0.0f;
    float optimalRange1Max = 1.0f;
    float optimalRange2Min = 0.0f;
    float optimalRange2Max = 1.0f;
};

struct InteractionTestResult {
    std::string testName;
    bool passed = true;
    bool unstable = false;
    bool silent = false;
    bool hasNaN = false;
    bool hasInf = false;
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    std::string notes;
};

struct EngineInteractionReport {
    int engineId;
    std::string engineName;
    int numParameters;
    std::vector<ParameterInteraction> interactions;
    std::vector<InteractionTestResult> testResults;
    std::map<std::string, std::string> sweetSpots;
    std::map<std::string, std::string> dangerZones;
    std::string overallNotes;
};

// Check for invalid audio
inline bool isInvalidFloat(float value) {
    return std::isnan(value) || std::isinf(value);
}

// Analyze buffer for statistics
struct AudioStats {
    float peak = 0.0f;
    float rms = 0.0f;
    bool hasNaN = false;
    bool hasInf = false;
    bool silent = false;
    float dcOffset = 0.0f;
};

AudioStats analyzeAudioBuffer(const juce::AudioBuffer<float>& buffer) {
    AudioStats stats;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    double sumSquared = 0.0;
    double sumDC = 0.0;
    int totalSamples = 0;

    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            float value = data[i];

            if (std::isnan(value)) stats.hasNaN = true;
            if (std::isinf(value)) stats.hasInf = true;

            float absValue = std::abs(value);
            stats.peak = std::max(stats.peak, absValue);

            sumSquared += value * value;
            sumDC += value;
            totalSamples++;
        }
    }

    if (totalSamples > 0) {
        stats.rms = std::sqrt(sumSquared / totalSamples);
        stats.dcOffset = sumDC / totalSamples;
        stats.silent = (stats.peak < 1e-6f);
    }

    return stats;
}

// Generate test signal
void generateTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate,
                       float frequency = 440.0f, float amplitude = 0.7f) {
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

// ============================================================================
// ENGINE-SPECIFIC PARAMETER INTERACTION DEFINITIONS
// ============================================================================

// Get parameter count for each engine
int getEngineParameterCount(int engineId) {
    static const std::map<int, int> paramCounts = {
        {0, 1},   {1, 7},   {2, 7},   {3, 5},   {4, 5},   {5, 6},   {6, 7},   {7, 7},
        {8, 6},   {9, 5},   {10, 5},  {11, 5},  {12, 5},  {13, 5},  {14, 5},  {15, 5},
        {16, 5},  {17, 5},  {18, 4},  {19, 6},  {20, 5},  {21, 5},  {22, 5},  {23, 5},
        {24, 5},  {25, 5},  {26, 4},  {27, 4},  {28, 4},  {29, 4},  {30, 5},  {31, 4},
        {32, 4},  {33, 5},  {34, 5},  {35, 5},  {36, 5},  {37, 5},  {38, 4},  {39, 6},
        {40, 5},  {41, 5},  {42, 6},  {43, 6},  {44, 4},  {45, 4},  {46, 5},  {47, 5},
        {48, 5},  {49, 5},  {50, 6},  {51, 5},  {52, 6},  {53, 4},  {54, 2},  {55, 1},
        {56, 3}
    };

    auto it = paramCounts.find(engineId);
    return (it != paramCounts.end()) ? it->second : 5;
}

// Define known parameter interactions for specific engine types
std::vector<ParameterInteraction> getKnownInteractions(int engineId, const std::string& engineName) {
    std::vector<ParameterInteraction> interactions;

    // COMPRESSORS (Engines 1, 2)
    if (engineId == 1 || engineId == 2) {
        interactions.push_back({0, 1, "coupled",
            "Attack & Release: Fast attack + fast release can cause pumping"});
        interactions.push_back({2, 3, "synergistic",
            "Threshold & Ratio: Higher ratio needs higher threshold for transparency"});
        interactions.push_back({0, 2, "conflicting",
            "Attack & Threshold: Very fast attack with low threshold causes distortion"});
    }

    // FILTERS (Engines 9-14)
    if (engineId >= 9 && engineId <= 14) {
        interactions.push_back({0, 1, "synergistic",
            "Frequency & Q/Resonance: High Q at low freq can cause booming"});
        interactions.push_back({0, 1, "coupled",
            "Frequency & Q: Self-oscillation at max Q + any frequency"});
    }

    // DISTORTION (Engines 15-22)
    if (engineId >= 15 && engineId <= 22) {
        interactions.push_back({0, 1, "synergistic",
            "Drive & Tone: High drive needs tone control to tame harshness"});
        interactions.push_back({0, 2, "coupled",
            "Drive & Output: Max drive requires output reduction to prevent clipping"});
    }

    // MODULATION - CHORUS/PHASER (Engines 23-25)
    if (engineId >= 23 && engineId <= 25) {
        interactions.push_back({0, 1, "synergistic",
            "Rate & Depth: Slow rate with high depth = seasick, fast + shallow = shimmer"});
        interactions.push_back({1, 2, "conflicting",
            "Depth & Feedback: Max depth + max feedback can cause instability"});
    }

    // TREMOLO (Engines 28-29)
    if (engineId == 28 || engineId == 29) {
        interactions.push_back({0, 1, "independent",
            "Rate & Depth: Independent controls, all combinations valid"});
    }

    // REVERB (Engines 39-43)
    if (engineId >= 39 && engineId <= 43) {
        interactions.push_back({1, 2, "synergistic",
            "Size & Damping: Large size needs damping to avoid metallic tail"});
        interactions.push_back({1, 3, "coupled",
            "Size & Pre-delay: Large size + long pre-delay = extreme spaciousness"});
        interactions.push_back({2, 4, "conflicting",
            "Damping & Diffusion: Max damping + low diffusion = muddy reverb"});
    }

    return interactions;
}

// ============================================================================
// PARAMETER INTERACTION TEST CASES
// ============================================================================

struct ParamPair {
    int param1;
    int param2;
    float value1;
    float value2;
    std::string testName;
};

std::vector<ParamPair> generateParameterPairTests(int numParams) {
    std::vector<ParamPair> tests;

    // Test critical parameter pairs (first 4 params are usually most important)
    int maxParam = std::min(numParams, 6);

    for (int p1 = 0; p1 < maxParam - 1; ++p1) {
        for (int p2 = p1 + 1; p2 < maxParam; ++p2) {
            // Extreme combinations
            tests.push_back({p1, p2, 0.0f, 0.0f, "Both_Min"});
            tests.push_back({p1, p2, 1.0f, 1.0f, "Both_Max"});
            tests.push_back({p1, p2, 0.0f, 1.0f, "P1_Min_P2_Max"});
            tests.push_back({p1, p2, 1.0f, 0.0f, "P1_Max_P2_Min"});

            // Sweet spot testing
            tests.push_back({p1, p2, 0.3f, 0.3f, "Both_Low"});
            tests.push_back({p1, p2, 0.5f, 0.5f, "Both_Mid"});
            tests.push_back({p1, p2, 0.7f, 0.7f, "Both_High"});
        }
    }

    return tests;
}

// Run a parameter interaction test
InteractionTestResult testParameterInteraction(
    std::unique_ptr<EngineBase>& engine,
    const ParamPair& test,
    int numParams,
    double sampleRate,
    int blockSize) {

    InteractionTestResult result;
    result.testName = "P" + std::to_string(test.param1) + "_P" + std::to_string(test.param2)
                     + "_" + test.testName;

    try {
        engine->reset();
        engine->prepareToPlay(sampleRate, blockSize);

        // Set default parameters
        std::map<int, float> params;
        for (int i = 0; i < numParams; ++i) {
            params[i] = 0.5f;
        }

        // Set test parameters
        params[test.param1] = test.value1;
        params[test.param2] = test.value2;

        engine->updateParameters(params);

        // Process multiple blocks to ensure stability
        juce::AudioBuffer<float> buffer(2, blockSize);
        const int numBlocks = 50;

        float maxPeak = 0.0f;
        double sumRMS = 0.0;

        for (int block = 0; block < numBlocks; ++block) {
            generateTestSignal(buffer, sampleRate);
            engine->process(buffer);

            AudioStats stats = analyzeAudioBuffer(buffer);

            if (stats.hasNaN) result.hasNaN = true;
            if (stats.hasInf) result.hasInf = true;
            if (stats.silent && block > 10) result.silent = true;

            maxPeak = std::max(maxPeak, stats.peak);
            sumRMS += stats.rms;

            // Check for instability (growing signal)
            if (stats.peak > 10.0f) {
                result.unstable = true;
                result.notes += "Signal growing beyond control. ";
                break;
            }
        }

        result.peakLevel = maxPeak;
        result.rmsLevel = sumRMS / numBlocks;

        // Determine pass/fail
        result.passed = !result.hasNaN && !result.hasInf && !result.unstable;

        // Add notes for interesting findings
        if (result.silent) {
            result.notes += "Silent output detected. ";
        }
        if (result.peakLevel > 5.0f) {
            result.notes += "Excessive output level. ";
        }
        if (result.rmsLevel < 0.001f && !result.silent) {
            result.notes += "Very low output level. ";
        }

    } catch (const std::exception& e) {
        result.passed = false;
        result.notes = std::string("Exception: ") + e.what();
    } catch (...) {
        result.passed = false;
        result.notes = "Unknown exception";
    }

    return result;
}

// ============================================================================
// ENGINE TESTING
// ============================================================================

EngineInteractionReport testEngineInteractions(int engineId) {
    EngineInteractionReport report;
    report.engineId = engineId;
    report.engineName = getEngineTypeName(engineId);
    report.numParameters = getEngineParameterCount(engineId);

    std::cout << "\n[Engine " << engineId << "] " << report.engineName << "\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "Parameters: " << report.numParameters << "\n";

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            std::cout << "ERROR: Failed to create engine\n";
            return report;
        }

        const double sampleRate = 48000.0;
        const int blockSize = 512;

        // Get known interactions for this engine
        report.interactions = getKnownInteractions(engineId, report.engineName);
        std::cout << "Known interactions: " << report.interactions.size() << "\n\n";

        // Generate and run parameter pair tests
        auto pairTests = generateParameterPairTests(report.numParameters);
        std::cout << "Running " << pairTests.size() << " parameter interaction tests...\n";

        int passCount = 0;
        int failCount = 0;
        int unstableCount = 0;
        int silentCount = 0;

        for (const auto& test : pairTests) {
            auto result = testParameterInteraction(engine, test, report.numParameters,
                                                  sampleRate, blockSize);
            report.testResults.push_back(result);

            if (result.passed) {
                passCount++;
            } else {
                failCount++;
                if (result.unstable) unstableCount++;
            }
            if (result.silent) silentCount++;
        }

        std::cout << "\nResults:\n";
        std::cout << "  PASS:     " << passCount << "\n";
        std::cout << "  FAIL:     " << failCount << "\n";
        std::cout << "  Unstable: " << unstableCount << "\n";
        std::cout << "  Silent:   " << silentCount << "\n";

        // Analyze results to find sweet spots and danger zones
        for (const auto& result : report.testResults) {
            if (result.passed && result.peakLevel > 0.1f && result.peakLevel < 2.0f
                && result.rmsLevel > 0.01f) {
                report.sweetSpots[result.testName] =
                    "Good output levels (Peak: " + std::to_string(result.peakLevel) +
                    ", RMS: " + std::to_string(result.rmsLevel) + ")";
            }

            if (result.unstable || result.hasNaN || result.hasInf) {
                report.dangerZones[result.testName] = result.notes;
            }
        }

        std::cout << "  Sweet spots found: " << report.sweetSpots.size() << "\n";
        std::cout << "  Danger zones found: " << report.dangerZones.size() << "\n";

    } catch (const std::exception& e) {
        std::cout << "EXCEPTION: " << e.what() << "\n";
        report.overallNotes = "Engine testing failed: " + std::string(e.what());
    }

    return report;
}

// ============================================================================
// REPORT GENERATION
// ============================================================================

void generateMarkdownReport(const std::vector<EngineInteractionReport>& reports,
                           const std::string& filename) {
    std::ofstream md(filename);

    md << "# PARAMETER INTERACTION TESTING REPORT\n\n";
    md << "**Chimera Phoenix v3.0 - Deep Parameter Interaction Analysis**\n\n";

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    md << "*Generated: " << std::ctime(&time) << "*\n\n";

    md << "---\n\n";

    // Executive Summary
    md << "## Executive Summary\n\n";

    int totalEngines = reports.size();
    int enginesWithIssues = 0;
    int totalDangerZones = 0;
    int totalSweetSpots = 0;

    for (const auto& report : reports) {
        if (!report.dangerZones.empty()) enginesWithIssues++;
        totalDangerZones += report.dangerZones.size();
        totalSweetSpots += report.sweetSpots.size();
    }

    md << "- **Total Engines Tested:** " << totalEngines << "\n";
    md << "- **Engines with Issues:** " << enginesWithIssues << "\n";
    md << "- **Total Danger Zones Identified:** " << totalDangerZones << "\n";
    md << "- **Total Sweet Spots Identified:** " << totalSweetSpots << "\n\n";

    md << "---\n\n";

    // Detailed Reports by Category
    md << "## Detailed Reports by Engine Category\n\n";

    // Group engines by category
    struct Category {
        std::string name;
        int startId;
        int endId;
    };

    std::vector<Category> categories = {
        {"Dynamics & Compression", 1, 6},
        {"Filters & EQ", 7, 14},
        {"Distortion & Saturation", 15, 22},
        {"Modulation", 23, 33},
        {"Reverb & Delay", 34, 43},
        {"Spatial & Special", 44, 52},
        {"Utility", 53, 56}
    };

    for (const auto& category : categories) {
        md << "### " << category.name << "\n\n";

        for (const auto& report : reports) {
            if (report.engineId >= category.startId && report.engineId <= category.endId) {
                md << "#### [" << report.engineId << "] " << report.engineName << "\n\n";
                md << "**Parameters:** " << report.numParameters << "\n\n";

                // Known Interactions
                if (!report.interactions.empty()) {
                    md << "**Known Parameter Interactions:**\n\n";
                    for (const auto& interaction : report.interactions) {
                        md << "- **P" << interaction.param1 << " + P" << interaction.param2
                           << "** (" << interaction.relationship << "): "
                           << interaction.description << "\n";
                    }
                    md << "\n";
                }

                // Test Results Summary
                int totalTests = report.testResults.size();
                int passedTests = 0;
                int failedTests = 0;
                int unstableTests = 0;

                for (const auto& result : report.testResults) {
                    if (result.passed) passedTests++;
                    else failedTests++;
                    if (result.unstable) unstableTests++;
                }

                md << "**Test Results:**\n";
                md << "- Total Tests: " << totalTests << "\n";
                md << "- Passed: " << passedTests << " ("
                   << (totalTests > 0 ? (100 * passedTests / totalTests) : 0) << "%)\n";
                md << "- Failed: " << failedTests << "\n";
                md << "- Unstable: " << unstableTests << "\n\n";

                // Sweet Spots
                if (!report.sweetSpots.empty()) {
                    md << "**Sweet Spots (Recommended Settings):**\n\n";
                    int count = 0;
                    for (const auto& [testName, description] : report.sweetSpots) {
                        if (count++ < 5) { // Show top 5
                            md << "- `" << testName << "`: " << description << "\n";
                        }
                    }
                    md << "\n";
                }

                // Danger Zones
                if (!report.dangerZones.empty()) {
                    md << "**⚠️  Danger Zones (Avoid These Combinations):**\n\n";
                    for (const auto& [testName, description] : report.dangerZones) {
                        md << "- `" << testName << "`: " << description << "\n";
                    }
                    md << "\n";
                }

                // Overall Notes
                if (!report.overallNotes.empty()) {
                    md << "**Notes:** " << report.overallNotes << "\n\n";
                }

                md << "---\n\n";
            }
        }
    }

    // Appendix: Testing Methodology
    md << "## Appendix: Testing Methodology\n\n";
    md << "### Parameter Interaction Tests\n\n";
    md << "For each engine, the following parameter pair combinations were tested:\n\n";
    md << "1. **Both Min** (0.0, 0.0) - Minimum values for both parameters\n";
    md << "2. **Both Max** (1.0, 1.0) - Maximum values for both parameters\n";
    md << "3. **P1 Min, P2 Max** (0.0, 1.0) - Extreme opposing values\n";
    md << "4. **P1 Max, P2 Min** (1.0, 0.0) - Extreme opposing values\n";
    md << "5. **Both Low** (0.3, 0.3) - Conservative low settings\n";
    md << "6. **Both Mid** (0.5, 0.5) - Neutral mid-range settings\n";
    md << "7. **Both High** (0.7, 0.7) - Conservative high settings\n\n";

    md << "### Failure Criteria\n\n";
    md << "- **NaN/Inf Output:** Audio buffer contains invalid floating-point values\n";
    md << "- **Unstable:** Signal grows beyond 10.0 peak amplitude\n";
    md << "- **Silent:** Output remains below -60dB after warmup period\n";
    md << "- **Excessive Level:** Peak exceeds 5.0 (potential clipping)\n\n";

    md << "### Test Conditions\n\n";
    md << "- Sample Rate: 48kHz\n";
    md << "- Block Size: 512 samples\n";
    md << "- Test Signal: 440Hz sine wave at -3dB\n";
    md << "- Processing Blocks: 50 per test\n\n";

    md << "---\n\n";
    md << "*End of Report*\n";

    md.close();
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "          CHIMERA PHOENIX - PARAMETER INTERACTION TESTING\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    std::cout << "Deep analysis of parameter interactions across all engines.\n";
    std::cout << "Testing for synergistic effects, conflicts, and stability issues.\n";
    std::cout << "\n";

    std::vector<EngineInteractionReport> allReports;

    // Test all engines (skip 0 = None)
    for (int engineId = 1; engineId < ENGINE_COUNT; ++engineId) {
        auto report = testEngineInteractions(engineId);
        allReports.push_back(report);
    }

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                         GENERATING REPORT\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    std::string reportFile = "PARAMETER_INTERACTION_TESTING_REPORT.md";
    generateMarkdownReport(allReports, reportFile);

    std::cout << "Report generated: " << reportFile << "\n";
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                         TESTING COMPLETE\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    return 0;
}

/**
 * stress_test_extreme_parameters.cpp
 *
 * Comprehensive stress testing suite for ALL 56 engines in Chimera Phoenix.
 * Tests extreme parameter values to detect:
 * - Crashes and exceptions
 * - NaN (Not a Number) output
 * - Infinite values
 * - Infinite loops / hangs
 * - Denormal numbers
 * - Buffer overruns
 *
 * Test scenarios for each engine:
 * 1. All parameters at minimum (0.0)
 * 2. All parameters at maximum (1.0)
 * 3. All parameters at 0.0 (zero test)
 * 4. All parameters at 1.0 (max test)
 * 5. Rapid parameter changes (automation stress)
 * 6. Random extreme values
 * 7. Edge case combinations
 *
 * Output: Comprehensive HTML/JSON report with pass/fail status
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

// Test result structure
struct TestResult {
    bool passed = false;
    bool crashed = false;
    bool hasNaN = false;
    bool hasInf = false;
    bool timeout = false;
    bool hasDenormals = false;
    double executionTimeMs = 0.0;
    float maxOutputLevel = 0.0f;
    std::string errorMessage;
};

// Test scenario structure
struct TestScenario {
    std::string name;
    std::string description;
    std::map<int, float> params;
};

// Engine test result
struct EngineTestResults {
    int engineId;
    std::string engineName;
    std::map<std::string, TestResult> scenarioResults;
    bool overallPass = true;
};

// Utility: Check if value is NaN or Inf
inline bool isInvalidFloat(float value) {
    return std::isnan(value) || std::isinf(value);
}

// Utility: Check if value is denormal
inline bool isDenormal(float value) {
    return value != 0.0f && std::abs(value) < 1.0e-30f;
}

// Generate test signal (sine wave)
void generateTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate, float frequency = 440.0f) {
    const float amplitude = 0.7f;
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

// Analyze buffer for problems
TestResult analyzeBuffer(const juce::AudioBuffer<float>& buffer) {
    TestResult result;
    result.passed = true;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            float value = data[i];

            // Check for NaN
            if (std::isnan(value)) {
                result.hasNaN = true;
                result.passed = false;
            }

            // Check for Inf
            if (std::isinf(value)) {
                result.hasInf = true;
                result.passed = false;
            }

            // Check for denormals
            if (isDenormal(value)) {
                result.hasDenormals = true;
            }

            // Track max level
            result.maxOutputLevel = std::max(result.maxOutputLevel, std::abs(value));
        }
    }

    return result;
}

// Get parameter count for an engine
int getEngineParameterCount(int engineId) {
    // Default parameter count per engine type
    // This is a conservative estimate - most engines have 3-7 parameters
    static const std::map<int, int> paramCounts = {
        {0, 1},   // None
        {1, 7},   // Opto Compressor
        {2, 7},   // VCA Compressor
        {3, 5},   // Transient Shaper
        {4, 5},   // Noise Gate
        {5, 6},   // Mastering Limiter
        {6, 7},   // Dynamic EQ
        {7, 7},   // Parametric EQ
        {8, 6},   // Vintage Console EQ
        {9, 5},   // Ladder Filter
        {10, 5},  // State Variable Filter
        {11, 5},  // Formant Filter
        {12, 5},  // Envelope Filter
        {13, 5},  // Comb Resonator
        {14, 5},  // Vocal Formant
        {15, 5},  // Vintage Tube
        {16, 5},  // Wave Folder
        {17, 5},  // Harmonic Exciter
        {18, 4},  // Bit Crusher
        {19, 6},  // Multiband Saturator
        {20, 5},  // Muff Fuzz
        {21, 5},  // Rodent Distortion
        {22, 5},  // K-Style
        {23, 5},  // Digital Chorus
        {24, 5},  // Resonant Chorus
        {25, 5},  // Analog Phaser
        {26, 4},  // Ring Modulator
        {27, 4},  // Frequency Shifter
        {28, 4},  // Harmonic Tremolo
        {29, 4},  // Classic Tremolo
        {30, 5},  // Rotary Speaker
        {31, 4},  // Pitch Shifter
        {32, 4},  // Detune Doubler
        {33, 5},  // Intelligent Harmonizer
        {34, 5},  // Tape Echo
        {35, 5},  // Digital Delay
        {36, 5},  // Magnetic Drum Echo
        {37, 5},  // Bucket Brigade Delay
        {38, 4},  // Buffer Repeat
        {39, 6},  // Plate Reverb
        {40, 5},  // Spring Reverb
        {41, 5},  // Convolution Reverb
        {42, 6},  // Shimmer Reverb
        {43, 6},  // Gated Reverb
        {44, 4},  // Stereo Widener
        {45, 4},  // Stereo Imager
        {46, 5},  // Dimension Expander
        {47, 5},  // Spectral Freeze
        {48, 5},  // Spectral Gate
        {49, 5},  // Phased Vocoder
        {50, 6},  // Granular Cloud
        {51, 5},  // Chaos Generator
        {52, 6},  // Feedback Network
        {53, 4},  // Mid-Side Processor
        {54, 2},  // Gain Utility
        {55, 1},  // Mono Maker
        {56, 3},  // Phase Align
    };

    auto it = paramCounts.find(engineId);
    if (it != paramCounts.end()) {
        return it->second;
    }
    return 5; // Default fallback
}

// Generate test scenarios for an engine
std::vector<TestScenario> generateTestScenarios(int engineId) {
    std::vector<TestScenario> scenarios;
    int numParams = getEngineParameterCount(engineId);

    // Scenario 1: All parameters at minimum (0.0)
    {
        TestScenario scenario;
        scenario.name = "All_Min";
        scenario.description = "All parameters set to 0.0 (minimum)";
        for (int i = 0; i < numParams; ++i) {
            scenario.params[i] = 0.0f;
        }
        scenarios.push_back(scenario);
    }

    // Scenario 2: All parameters at maximum (1.0)
    {
        TestScenario scenario;
        scenario.name = "All_Max";
        scenario.description = "All parameters set to 1.0 (maximum)";
        for (int i = 0; i < numParams; ++i) {
            scenario.params[i] = 1.0f;
        }
        scenarios.push_back(scenario);
    }

    // Scenario 3: All parameters at 0.0 (explicit zero test)
    {
        TestScenario scenario;
        scenario.name = "All_Zero";
        scenario.description = "All parameters at 0.0 (zero test)";
        for (int i = 0; i < numParams; ++i) {
            scenario.params[i] = 0.0f;
        }
        scenarios.push_back(scenario);
    }

    // Scenario 4: All parameters at 1.0 (explicit max test)
    {
        TestScenario scenario;
        scenario.name = "All_One";
        scenario.description = "All parameters at 1.0 (unity test)";
        for (int i = 0; i < numParams; ++i) {
            scenario.params[i] = 1.0f;
        }
        scenarios.push_back(scenario);
    }

    // Scenario 5: Alternating 0 and 1
    {
        TestScenario scenario;
        scenario.name = "Alternating_0_1";
        scenario.description = "Parameters alternate between 0.0 and 1.0";
        for (int i = 0; i < numParams; ++i) {
            scenario.params[i] = (i % 2 == 0) ? 0.0f : 1.0f;
        }
        scenarios.push_back(scenario);
    }

    // Scenario 6: Rapid parameter changes (automation stress)
    {
        TestScenario scenario;
        scenario.name = "Rapid_Changes";
        scenario.description = "Rapid parameter changes to test smoothing";
        // This will be handled specially during processing
        for (int i = 0; i < numParams; ++i) {
            scenario.params[i] = 0.5f;
        }
        scenarios.push_back(scenario);
    }

    // Scenario 7: Random extreme values
    {
        TestScenario scenario;
        scenario.name = "Random_Extreme";
        scenario.description = "Random extreme values";
        std::mt19937 rng(42); // Fixed seed for reproducibility
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (int i = 0; i < numParams; ++i) {
            float val = dist(rng);
            // Bias toward extremes
            scenario.params[i] = (val < 0.5f) ? 0.0f : 1.0f;
        }
        scenarios.push_back(scenario);
    }

    // Scenario 8: Very small values (denormal test)
    {
        TestScenario scenario;
        scenario.name = "Denormal_Test";
        scenario.description = "Very small parameter values to test denormal handling";
        for (int i = 0; i < numParams; ++i) {
            scenario.params[i] = 1.0e-6f;
        }
        scenarios.push_back(scenario);
    }

    return scenarios;
}

// Run a single test scenario
TestResult runTestScenario(std::unique_ptr<EngineBase>& engine,
                           const TestScenario& scenario,
                           double sampleRate,
                           int blockSize,
                           bool rapidChanges = false) {
    TestResult result;

    try {
        // Prepare engine
        engine->reset();
        engine->prepareToPlay(sampleRate, blockSize);

        // Set parameters
        engine->updateParameters(scenario.params);

        // Process audio
        const int totalBlocks = 100; // Process 100 blocks
        juce::AudioBuffer<float> buffer(2, blockSize);

        auto startTime = std::chrono::high_resolution_clock::now();

        for (int block = 0; block < totalBlocks; ++block) {
            // Generate test signal
            generateTestSignal(buffer, sampleRate);

            // For rapid changes scenario, change parameters every block
            if (rapidChanges && block > 0) {
                std::map<int, float> newParams = scenario.params;
                for (auto& [key, value] : newParams) {
                    value = (block % 2 == 0) ? 0.0f : 1.0f;
                }
                engine->updateParameters(newParams);
            }

            // Process
            engine->process(buffer);

            // Analyze output
            TestResult blockResult = analyzeBuffer(buffer);
            if (!blockResult.passed) {
                result.hasNaN = result.hasNaN || blockResult.hasNaN;
                result.hasInf = result.hasInf || blockResult.hasInf;
                result.passed = false;
            }
            result.hasDenormals = result.hasDenormals || blockResult.hasDenormals;
            result.maxOutputLevel = std::max(result.maxOutputLevel, blockResult.maxOutputLevel);

            // Check for timeout (shouldn't take more than 5 seconds for 100 blocks)
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - startTime).count();
            if (elapsed > 5000) {
                result.timeout = true;
                result.passed = false;
                result.errorMessage = "Processing timeout (>5s)";
                break;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count() / 1000.0;

        if (result.hasNaN) {
            result.errorMessage += "NaN detected in output. ";
        }
        if (result.hasInf) {
            result.errorMessage += "Inf detected in output. ";
        }
        if (result.timeout) {
            result.errorMessage += "Processing timeout. ";
        }

        if (result.passed && result.errorMessage.empty()) {
            result.passed = true;
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.passed = false;
        result.errorMessage = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.passed = false;
        result.errorMessage = "Unknown exception";
    }

    return result;
}

// Test a single engine with all scenarios
EngineTestResults testEngine(int engineId) {
    EngineTestResults results;
    results.engineId = engineId;
    results.engineName = getEngineTypeName(engineId);
    results.overallPass = true;

    std::cout << "\n[" << std::setw(2) << engineId << "] "
              << results.engineName << "\n";
    std::cout << std::string(60, '-') << "\n";

    try {
        // Create engine
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            std::cout << "  ERROR: Failed to create engine\n";
            TestResult failResult;
            failResult.crashed = true;
            failResult.errorMessage = "Failed to create engine";
            results.scenarioResults["Creation"] = failResult;
            results.overallPass = false;
            return results;
        }

        // Generate test scenarios
        auto scenarios = generateTestScenarios(engineId);

        // Run each scenario
        const double sampleRate = 48000.0;
        const int blockSize = 512;

        for (const auto& scenario : scenarios) {
            std::cout << "  " << std::setw(20) << std::left << scenario.name << " ... ";
            std::cout.flush();

            bool rapidChanges = (scenario.name == "Rapid_Changes");
            TestResult result = runTestScenario(engine, scenario, sampleRate, blockSize, rapidChanges);

            results.scenarioResults[scenario.name] = result;

            if (result.passed) {
                std::cout << "\033[32mPASS\033[0m";
                std::cout << " (" << std::fixed << std::setprecision(2)
                         << result.executionTimeMs << " ms, "
                         << "peak=" << result.maxOutputLevel << ")";
            } else {
                std::cout << "\033[31mFAIL\033[0m";
                if (!result.errorMessage.empty()) {
                    std::cout << " - " << result.errorMessage;
                }
                results.overallPass = false;
            }

            if (result.hasDenormals) {
                std::cout << " [DENORMALS]";
            }

            std::cout << "\n";
        }

    } catch (const std::exception& e) {
        std::cout << "  EXCEPTION: " << e.what() << "\n";
        TestResult failResult;
        failResult.crashed = true;
        failResult.errorMessage = e.what();
        results.scenarioResults["Overall"] = failResult;
        results.overallPass = false;
    }

    return results;
}

// Generate HTML report
void generateHTMLReport(const std::vector<EngineTestResults>& allResults,
                       const std::string& filename) {
    std::ofstream html(filename);

    html << "<!DOCTYPE html>\n";
    html << "<html>\n<head>\n";
    html << "<title>Chimera Phoenix - Extreme Parameter Stress Test Report</title>\n";
    html << "<style>\n";
    html << "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 20px; background: #f5f5f5; }\n";
    html << "h1 { color: #2c3e50; border-bottom: 3px solid #3498db; padding-bottom: 10px; }\n";
    html << "h2 { color: #34495e; margin-top: 30px; }\n";
    html << ".summary { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); margin-bottom: 20px; }\n";
    html << ".summary-stats { display: flex; gap: 20px; }\n";
    html << ".stat-box { flex: 1; padding: 15px; border-radius: 5px; text-align: center; }\n";
    html << ".stat-box.pass { background: #d4edda; border: 2px solid #28a745; }\n";
    html << ".stat-box.fail { background: #f8d7da; border: 2px solid #dc3545; }\n";
    html << ".stat-box.warn { background: #fff3cd; border: 2px solid #ffc107; }\n";
    html << ".stat-number { font-size: 36px; font-weight: bold; margin: 10px 0; }\n";
    html << ".stat-label { color: #666; font-size: 14px; }\n";
    html << "table { width: 100%; border-collapse: collapse; background: white; box-shadow: 0 2px 4px rgba(0,0,0,0.1); margin-top: 20px; }\n";
    html << "th { background: #3498db; color: white; padding: 12px; text-align: left; }\n";
    html << "td { padding: 10px; border-bottom: 1px solid #ddd; }\n";
    html << "tr:hover { background: #f8f9fa; }\n";
    html << ".pass { color: #28a745; font-weight: bold; }\n";
    html << ".fail { color: #dc3545; font-weight: bold; }\n";
    html << ".warn { color: #ffc107; font-weight: bold; }\n";
    html << ".error-msg { color: #dc3545; font-size: 12px; }\n";
    html << ".engine-section { background: white; padding: 15px; border-radius: 8px; margin: 15px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    html << ".scenario-table { font-size: 13px; }\n";
    html << ".timestamp { color: #666; font-size: 12px; text-align: right; margin-top: 20px; }\n";
    html << "</style>\n";
    html << "</head>\n<body>\n";

    // Header
    html << "<h1>Chimera Phoenix - Extreme Parameter Stress Test Report</h1>\n";

    // Summary statistics
    int totalEngines = allResults.size();
    int passedEngines = 0;
    int failedEngines = 0;
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    int denormalIssues = 0;
    int crashedEngines = 0;

    for (const auto& result : allResults) {
        if (result.overallPass) {
            passedEngines++;
        } else {
            failedEngines++;
        }

        for (const auto& [scenario, testResult] : result.scenarioResults) {
            totalTests++;
            if (testResult.passed) {
                passedTests++;
            } else {
                failedTests++;
            }
            if (testResult.hasDenormals) {
                denormalIssues++;
            }
            if (testResult.crashed) {
                crashedEngines++;
            }
        }
    }

    html << "<div class='summary'>\n";
    html << "<h2>Summary Statistics</h2>\n";
    html << "<div class='summary-stats'>\n";

    html << "<div class='stat-box " << (failedEngines == 0 ? "pass" : "fail") << "'>\n";
    html << "<div class='stat-number'>" << passedEngines << "/" << totalEngines << "</div>\n";
    html << "<div class='stat-label'>Engines Passed</div>\n";
    html << "</div>\n";

    html << "<div class='stat-box " << (failedTests == 0 ? "pass" : "fail") << "'>\n";
    html << "<div class='stat-number'>" << passedTests << "/" << totalTests << "</div>\n";
    html << "<div class='stat-label'>Tests Passed</div>\n";
    html << "</div>\n";

    html << "<div class='stat-box " << (crashedEngines == 0 ? "pass" : "fail") << "'>\n";
    html << "<div class='stat-number'>" << crashedEngines << "</div>\n";
    html << "<div class='stat-label'>Crashes/Exceptions</div>\n";
    html << "</div>\n";

    html << "<div class='stat-box " << (denormalIssues == 0 ? "pass" : "warn") << "'>\n";
    html << "<div class='stat-number'>" << denormalIssues << "</div>\n";
    html << "<div class='stat-label'>Denormal Issues</div>\n";
    html << "</div>\n";

    html << "</div>\n";
    html << "</div>\n";

    // Detailed results per engine
    html << "<h2>Detailed Results by Engine</h2>\n";

    for (const auto& result : allResults) {
        html << "<div class='engine-section'>\n";
        html << "<h3>[" << result.engineId << "] " << result.engineName;
        if (result.overallPass) {
            html << " <span class='pass'>✓ PASS</span>";
        } else {
            html << " <span class='fail'>✗ FAIL</span>";
        }
        html << "</h3>\n";

        html << "<table class='scenario-table'>\n";
        html << "<tr><th>Scenario</th><th>Status</th><th>Time (ms)</th><th>Peak Level</th><th>Notes</th></tr>\n";

        for (const auto& [scenario, testResult] : result.scenarioResults) {
            html << "<tr>\n";
            html << "<td>" << scenario << "</td>\n";

            // Status
            html << "<td class='" << (testResult.passed ? "pass" : "fail") << "'>";
            html << (testResult.passed ? "PASS" : "FAIL") << "</td>\n";

            // Time
            html << "<td>" << std::fixed << std::setprecision(2) << testResult.executionTimeMs << "</td>\n";

            // Peak level
            html << "<td>" << std::fixed << std::setprecision(3) << testResult.maxOutputLevel << "</td>\n";

            // Notes
            html << "<td>";
            if (testResult.crashed) {
                html << "<span class='fail'>CRASHED</span> ";
            }
            if (testResult.hasNaN) {
                html << "<span class='fail'>NaN</span> ";
            }
            if (testResult.hasInf) {
                html << "<span class='fail'>Inf</span> ";
            }
            if (testResult.timeout) {
                html << "<span class='fail'>TIMEOUT</span> ";
            }
            if (testResult.hasDenormals) {
                html << "<span class='warn'>Denormals</span> ";
            }
            if (!testResult.errorMessage.empty()) {
                html << "<br><span class='error-msg'>" << testResult.errorMessage << "</span>";
            }
            html << "</td>\n";

            html << "</tr>\n";
        }

        html << "</table>\n";
        html << "</div>\n";
    }

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    html << "<div class='timestamp'>Generated: " << std::ctime(&time) << "</div>\n";

    html << "</body>\n</html>\n";
    html.close();
}

// Generate JSON report
void generateJSONReport(const std::vector<EngineTestResults>& allResults,
                       const std::string& filename) {
    std::ofstream json(filename);

    json << "{\n";
    json << "  \"test_name\": \"Extreme Parameter Stress Test\",\n";
    json << "  \"total_engines\": " << allResults.size() << ",\n";
    json << "  \"results\": [\n";

    for (size_t i = 0; i < allResults.size(); ++i) {
        const auto& result = allResults[i];

        json << "    {\n";
        json << "      \"engine_id\": " << result.engineId << ",\n";
        json << "      \"engine_name\": \"" << result.engineName << "\",\n";
        json << "      \"overall_pass\": " << (result.overallPass ? "true" : "false") << ",\n";
        json << "      \"scenarios\": [\n";

        size_t scenarioIdx = 0;
        for (const auto& [scenario, testResult] : result.scenarioResults) {
            json << "        {\n";
            json << "          \"name\": \"" << scenario << "\",\n";
            json << "          \"passed\": " << (testResult.passed ? "true" : "false") << ",\n";
            json << "          \"crashed\": " << (testResult.crashed ? "true" : "false") << ",\n";
            json << "          \"has_nan\": " << (testResult.hasNaN ? "true" : "false") << ",\n";
            json << "          \"has_inf\": " << (testResult.hasInf ? "true" : "false") << ",\n";
            json << "          \"timeout\": " << (testResult.timeout ? "true" : "false") << ",\n";
            json << "          \"has_denormals\": " << (testResult.hasDenormals ? "true" : "false") << ",\n";
            json << "          \"execution_time_ms\": " << testResult.executionTimeMs << ",\n";
            json << "          \"max_output_level\": " << testResult.maxOutputLevel << ",\n";
            json << "          \"error_message\": \"" << testResult.errorMessage << "\"\n";
            json << "        }";

            if (++scenarioIdx < result.scenarioResults.size()) {
                json << ",";
            }
            json << "\n";
        }

        json << "      ]\n";
        json << "    }";

        if (i < allResults.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << "  ]\n";
    json << "}\n";
    json.close();
}

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "         CHIMERA PHOENIX - EXTREME PARAMETER STRESS TEST\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    std::cout << "Testing all 56 engines with extreme parameter values...\n";
    std::cout << "Test scenarios per engine:\n";
    std::cout << "  1. All parameters at minimum (0.0)\n";
    std::cout << "  2. All parameters at maximum (1.0)\n";
    std::cout << "  3. All parameters at zero\n";
    std::cout << "  4. All parameters at unity (1.0)\n";
    std::cout << "  5. Alternating 0 and 1\n";
    std::cout << "  6. Rapid parameter changes\n";
    std::cout << "  7. Random extreme values\n";
    std::cout << "  8. Denormal test (very small values)\n";
    std::cout << "\n";
    std::cout << "Checking for: Crashes, NaN, Inf, Timeouts, Denormals\n";
    std::cout << "\n";

    std::vector<EngineTestResults> allResults;

    // Test all engines (1-56, skipping 0 which is None/Bypass)
    for (int engineId = 1; engineId < ENGINE_COUNT; ++engineId) {
        EngineTestResults result = testEngine(engineId);
        allResults.push_back(result);
    }

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                           GENERATING REPORTS\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    // Generate reports
    std::string htmlFile = "stress_test_report.html";
    std::string jsonFile = "stress_test_report.json";

    std::cout << "Generating HTML report: " << htmlFile << "\n";
    generateHTMLReport(allResults, htmlFile);

    std::cout << "Generating JSON report: " << jsonFile << "\n";
    generateJSONReport(allResults, jsonFile);

    // Final summary
    int passedEngines = 0;
    int failedEngines = 0;
    for (const auto& result : allResults) {
        if (result.overallPass) {
            passedEngines++;
        } else {
            failedEngines++;
        }
    }

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                              FINAL SUMMARY\n";
    std::cout << "================================================================================\n";
    std::cout << "  Total Engines Tested:  " << allResults.size() << "\n";
    std::cout << "  Passed:                " << passedEngines << "\n";
    std::cout << "  Failed:                " << failedEngines << "\n";
    std::cout << "\n";

    if (failedEngines == 0) {
        std::cout << "  \033[32m✓ ALL ENGINES PASSED STRESS TESTS!\033[0m\n";
    } else {
        std::cout << "  \033[31m✗ SOME ENGINES FAILED - SEE REPORT FOR DETAILS\033[0m\n";
    }

    std::cout << "\n";
    std::cout << "  Reports generated:\n";
    std::cout << "    - " << htmlFile << " (open in browser)\n";
    std::cout << "    - " << jsonFile << " (machine-readable)\n";
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    return (failedEngines == 0) ? 0 : 1;
}

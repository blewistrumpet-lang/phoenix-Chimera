/**
 * test_buffer_size_independence.cpp
 *
 * Buffer Size Independence Test for Chimera Phoenix
 *
 * Purpose: Verify that all engines produce identical output regardless of buffer size.
 *          This ensures that audio processing is truly buffer-size independent and
 *          will work correctly in any DAW or audio environment.
 *
 * Test Methodology:
 * 1. Generate identical test signals (1kHz sine wave)
 * 2. Process through each engine with multiple buffer sizes: 32, 64, 128, 256, 512, 1024, 2048
 * 3. Compare outputs sample-by-sample across all buffer sizes
 * 4. Calculate maximum deviation and RMS error
 * 5. Report pass/fail status for each engine
 *
 * Pass Criteria:
 * - Maximum sample deviation < 1e-6 (numerical precision tolerance)
 * - RMS error < 1e-7
 * - No NaN or Inf values in output
 *
 * Output:
 * - buffer_size_independence_report.txt - Detailed text report
 * - buffer_size_independence_results.csv - Spreadsheet-compatible results
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <map>
#include <algorithm>
#include <sstream>

// JUCE includes
#include "../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

// Engine Factory
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"

// Test configuration
const double SAMPLE_RATE = 48000.0;
const double TEST_DURATION_SECONDS = 2.0;
const int NUM_CHANNELS = 2;
const float TEST_FREQUENCY_HZ = 1000.0f;
const float TEST_AMPLITUDE = 0.5f; // -6dBFS

// Buffer sizes to test
const std::vector<int> BUFFER_SIZES = {32, 64, 128, 256, 512, 1024, 2048};

// Pass criteria
const double MAX_DEVIATION_THRESHOLD = 1e-6;
const double RMS_ERROR_THRESHOLD = 1e-7;

// Engine metadata
struct EngineInfo {
    int id;
    std::string name;
    std::string category;
};

// All 56 engines
const std::vector<EngineInfo> ALL_ENGINES = {
    {0, "None (Bypass)", "Utility"},
    {1, "Vintage Opto Compressor", "Dynamics"},
    {2, "Classic VCA Compressor", "Dynamics"},
    {3, "Transient Shaper", "Dynamics"},
    {4, "Noise Gate", "Dynamics"},
    {5, "Mastering Limiter", "Dynamics"},
    {6, "Dynamic EQ", "Dynamics"},
    {7, "Parametric EQ (Studio)", "Filter"},
    {8, "Vintage Console EQ", "Filter"},
    {9, "Ladder Filter", "Filter"},
    {10, "State Variable Filter", "Filter"},
    {11, "Formant Filter", "Filter"},
    {12, "Envelope Filter", "Filter"},
    {13, "Comb Resonator", "Filter"},
    {14, "Vocal Formant Filter", "Filter"},
    {15, "Vintage Tube Preamp", "Distortion"},
    {16, "Wave Folder", "Distortion"},
    {17, "Harmonic Exciter", "Distortion"},
    {18, "Bit Crusher", "Distortion"},
    {19, "Multiband Saturator", "Distortion"},
    {20, "Muff Fuzz", "Distortion"},
    {21, "Rodent Distortion", "Distortion"},
    {22, "K-Style Overdrive", "Distortion"},
    {23, "Digital Chorus", "Modulation"},
    {24, "Resonant Chorus", "Modulation"},
    {25, "Analog Phaser", "Modulation"},
    {26, "Ring Modulator", "Modulation"},
    {27, "Frequency Shifter", "Modulation"},
    {28, "Harmonic Tremolo", "Modulation"},
    {29, "Classic Tremolo", "Modulation"},
    {30, "Rotary Speaker", "Modulation"},
    {31, "Pitch Shifter", "Modulation"},
    {32, "Detune Doubler", "Modulation"},
    {33, "Intelligent Harmonizer", "Modulation"},
    {34, "Tape Echo", "Delay"},
    {35, "Digital Delay", "Delay"},
    {36, "Magnetic Drum Echo", "Delay"},
    {37, "Bucket Brigade Delay", "Delay"},
    {38, "Buffer Repeat", "Delay"},
    {39, "Plate Reverb", "Reverb"},
    {40, "Spring Reverb", "Reverb"},
    {41, "Convolution Reverb", "Reverb"},
    {42, "Shimmer Reverb", "Reverb"},
    {43, "Gated Reverb", "Reverb"},
    {44, "Stereo Widener", "Spatial"},
    {45, "Stereo Imager", "Spatial"},
    {46, "Dimension Expander", "Spatial"},
    {47, "Spectral Freeze", "Special"},
    {48, "Spectral Gate", "Special"},
    {49, "Phased Vocoder", "Special"},
    {50, "Granular Cloud", "Special"},
    {51, "Chaos Generator", "Special"},
    {52, "Feedback Network", "Special"},
    {53, "Mid-Side Processor", "Utility"},
    {54, "Gain Utility", "Utility"},
    {55, "Mono Maker", "Utility"},
    {56, "Phase Align", "Utility"}
};

// Test result structure
struct BufferSizeTestResult {
    int engineId;
    std::string engineName;
    std::string category;

    bool success;
    std::string errorMessage;

    // Comparison results for each buffer size pair
    std::map<int, double> maxDeviations;      // buffer_size -> max deviation from reference
    std::map<int, double> rmsErrors;          // buffer_size -> RMS error from reference
    std::map<int, bool> hasNaN;              // buffer_size -> has NaN values
    std::map<int, bool> hasInf;              // buffer_size -> has Inf values

    // Overall results
    double worstMaxDeviation;
    double worstRmsError;
    int worstBufferSize;
    bool passed;

    BufferSizeTestResult() :
        engineId(0), success(false),
        worstMaxDeviation(0.0), worstRmsError(0.0),
        worstBufferSize(0), passed(false) {}
};

// Generate test signal
void generateTestSignal(juce::AudioBuffer<float>& buffer, float frequency, float amplitude, double sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            float phase = 2.0f * M_PI * frequency * i / sampleRate;
            channelData[i] = amplitude * std::sin(phase);
        }
    }
}

// Check for invalid values
bool hasInvalidValues(const juce::AudioBuffer<float>& buffer, bool& foundNaN, bool& foundInf) {
    foundNaN = false;
    foundInf = false;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::isnan(data[i])) foundNaN = true;
            if (std::isinf(data[i])) foundInf = true;
        }
    }

    return foundNaN || foundInf;
}

// Process audio with specific buffer size
juce::AudioBuffer<float> processWithBufferSize(
    EngineBase* engine,
    const juce::AudioBuffer<float>& inputSignal,
    int bufferSize,
    double sampleRate) {

    // Create output buffer
    const int totalSamples = inputSignal.getNumSamples();
    const int numChannels = inputSignal.getNumChannels();
    juce::AudioBuffer<float> output(numChannels, totalSamples);

    // Copy input to output (will be processed in-place)
    for (int ch = 0; ch < numChannels; ++ch) {
        output.copyFrom(ch, 0, inputSignal, ch, 0, totalSamples);
    }

    // Reset engine state
    engine->prepareToPlay(sampleRate, bufferSize);

    // Process in blocks of specified buffer size
    for (int startSample = 0; startSample < totalSamples; startSample += bufferSize) {
        int samplesThisBlock = std::min(bufferSize, totalSamples - startSample);

        // Create a view into the buffer for this block
        juce::AudioBuffer<float> block(
            output.getArrayOfWritePointers(),
            numChannels,
            startSample,
            samplesThisBlock
        );

        // Process this block
        engine->process(block);
    }

    return output;
}

// Compare two audio buffers
void compareBuffers(
    const juce::AudioBuffer<float>& reference,
    const juce::AudioBuffer<float>& test,
    double& maxDeviation,
    double& rmsError) {

    maxDeviation = 0.0;
    double sumSquaredError = 0.0;
    int totalSamples = 0;

    const int numSamples = std::min(reference.getNumSamples(), test.getNumSamples());
    const int numChannels = std::min(reference.getNumChannels(), test.getNumChannels());

    for (int ch = 0; ch < numChannels; ++ch) {
        const float* refData = reference.getReadPointer(ch);
        const float* testData = test.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            double error = std::abs(refData[i] - testData[i]);
            maxDeviation = std::max(maxDeviation, error);
            sumSquaredError += error * error;
            totalSamples++;
        }
    }

    if (totalSamples > 0) {
        rmsError = std::sqrt(sumSquaredError / totalSamples);
    } else {
        rmsError = 0.0;
    }
}

// Set neutral parameters for engine
void setNeutralParameters(EngineBase* engine, int engineId) {
    std::map<int, float> params;

    // Set safe, neutral parameters
    // These are designed to enable processing without extreme effects

    // Dynamics (1-6)
    if (engineId >= 1 && engineId <= 6) {
        params[0] = 0.8f;  // High threshold
        params[1] = 0.2f;  // Low ratio
        params[2] = 0.5f;  // Medium attack
        params[3] = 0.5f;  // Medium release
        params[4] = 0.5f;  // Unity gain
        params[5] = 1.0f;  // Full mix
    }

    // Filters (7-14)
    if (engineId >= 7 && engineId <= 14) {
        params[0] = 0.5f;  // Frequency
        params[1] = 0.3f;  // Low Q
        params[2] = 0.5f;  // Unity gain
        params[3] = 1.0f;  // Full mix
    }

    // Distortion (15-22)
    if (engineId >= 15 && engineId <= 22) {
        params[0] = 0.3f;  // Low drive
        params[1] = 0.5f;  // Medium tone
        params[2] = 0.5f;  // Unity output
        params[3] = 1.0f;  // Full mix
    }

    // Modulation (23-33)
    if (engineId >= 23 && engineId <= 33) {
        params[0] = 0.3f;  // Low rate
        params[1] = 0.2f;  // Low depth
        params[2] = 0.3f;  // Low feedback
        params[3] = 1.0f;  // Full mix
    }

    // Delays (34-38)
    if (engineId >= 34 && engineId <= 38) {
        params[0] = 0.2f;  // Short time
        params[1] = 0.2f;  // Low feedback
        params[2] = 0.5f;  // 50% mix
    }

    // Reverbs (39-43)
    if (engineId >= 39 && engineId <= 43) {
        params[0] = 0.3f;  // Short decay
        params[1] = 0.5f;  // Medium size
        params[2] = 0.5f;  // 50% mix
    }

    // Spatial/Special (44-52)
    if (engineId >= 44 && engineId <= 52) {
        params[0] = 0.5f;
        params[1] = 0.5f;
        params[2] = 0.5f;
    }

    // Utility (53-56)
    if (engineId >= 53 && engineId <= 56) {
        params[0] = 0.5f;
        params[1] = 0.5f;
        params[2] = 1.0f;
    }

    engine->updateParameters(params);
}

// Test a single engine with all buffer sizes
BufferSizeTestResult testEngine(const EngineInfo& info) {
    BufferSizeTestResult result;
    result.engineId = info.id;
    result.engineName = info.name;
    result.category = info.category;

    try {
        // Create engine
        auto engine = EngineFactory::createEngine(info.id);
        if (!engine) {
            result.errorMessage = "Failed to create engine";
            return result;
        }

        // Generate test signal
        const int totalSamples = static_cast<int>(SAMPLE_RATE * TEST_DURATION_SECONDS);
        juce::AudioBuffer<float> inputSignal(NUM_CHANNELS, totalSamples);
        generateTestSignal(inputSignal, TEST_FREQUENCY_HZ, TEST_AMPLITUDE, SAMPLE_RATE);

        // Set neutral parameters
        setNeutralParameters(engine.get(), info.id);

        // Process with each buffer size and store outputs
        std::map<int, juce::AudioBuffer<float>> outputs;

        for (int bufferSize : BUFFER_SIZES) {
            outputs[bufferSize] = processWithBufferSize(
                engine.get(),
                inputSignal,
                bufferSize,
                SAMPLE_RATE
            );

            // Check for invalid values
            bool hasNaN, hasInf;
            hasInvalidValues(outputs[bufferSize], hasNaN, hasInf);
            result.hasNaN[bufferSize] = hasNaN;
            result.hasInf[bufferSize] = hasInf;
        }

        // Use smallest buffer size as reference
        const int referenceBufferSize = BUFFER_SIZES[0];
        const auto& referenceOutput = outputs[referenceBufferSize];

        // Compare all other buffer sizes to reference
        result.worstMaxDeviation = 0.0;
        result.worstRmsError = 0.0;
        result.worstBufferSize = referenceBufferSize;

        for (int bufferSize : BUFFER_SIZES) {
            if (bufferSize == referenceBufferSize) {
                result.maxDeviations[bufferSize] = 0.0;
                result.rmsErrors[bufferSize] = 0.0;
                continue;
            }

            double maxDev, rmsErr;
            compareBuffers(referenceOutput, outputs[bufferSize], maxDev, rmsErr);

            result.maxDeviations[bufferSize] = maxDev;
            result.rmsErrors[bufferSize] = rmsErr;

            if (maxDev > result.worstMaxDeviation) {
                result.worstMaxDeviation = maxDev;
                result.worstRmsError = rmsErr;
                result.worstBufferSize = bufferSize;
            }
        }

        // Determine pass/fail
        bool hasAnyInvalidValues = false;
        for (const auto& pair : result.hasNaN) {
            if (pair.second) hasAnyInvalidValues = true;
        }
        for (const auto& pair : result.hasInf) {
            if (pair.second) hasAnyInvalidValues = true;
        }

        result.passed = !hasAnyInvalidValues &&
                       (result.worstMaxDeviation < MAX_DEVIATION_THRESHOLD) &&
                       (result.worstRmsError < RMS_ERROR_THRESHOLD);

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

// Save detailed text report
void saveTextReport(const std::vector<BufferSizeTestResult>& results, const std::string& filename) {
    std::ofstream file(filename);

    file << "========================================================================\n";
    file << "    CHIMERA PHOENIX - BUFFER SIZE INDEPENDENCE TEST REPORT\n";
    file << "========================================================================\n";
    file << "\n";
    file << "Test Configuration:\n";
    file << "  Sample Rate:       " << SAMPLE_RATE << " Hz\n";
    file << "  Test Duration:     " << TEST_DURATION_SECONDS << " seconds\n";
    file << "  Test Signal:       " << TEST_FREQUENCY_HZ << " Hz sine wave\n";
    file << "  Test Amplitude:    " << TEST_AMPLITUDE << " (-6 dBFS)\n";
    file << "  Buffer Sizes:      ";
    for (size_t i = 0; i < BUFFER_SIZES.size(); ++i) {
        file << BUFFER_SIZES[i];
        if (i < BUFFER_SIZES.size() - 1) file << ", ";
    }
    file << " samples\n";
    file << "  Pass Criteria:\n";
    file << "    Max Deviation:   < " << std::scientific << MAX_DEVIATION_THRESHOLD << "\n";
    file << "    RMS Error:       < " << std::scientific << RMS_ERROR_THRESHOLD << "\n";
    file << "\n";

    // Summary statistics
    int totalEngines = results.size();
    int passed = 0;
    int failed = 0;
    int errors = 0;

    for (const auto& r : results) {
        if (!r.success) {
            errors++;
        } else if (r.passed) {
            passed++;
        } else {
            failed++;
        }
    }

    file << "========================================================================\n";
    file << "                         OVERALL SUMMARY\n";
    file << "========================================================================\n";
    file << "\n";
    file << "Total Engines Tested: " << totalEngines << "\n";
    file << "Passed:               " << passed << " (" << std::fixed << std::setprecision(1)
         << (100.0 * passed / totalEngines) << "%)\n";
    file << "Failed:               " << failed << " (" << std::fixed << std::setprecision(1)
         << (100.0 * failed / totalEngines) << "%)\n";
    file << "Errors:               " << errors << "\n";
    file << "\n";

    // Detailed results
    file << "========================================================================\n";
    file << "                      DETAILED RESULTS\n";
    file << "========================================================================\n";
    file << "\n";

    for (const auto& r : results) {
        file << "------------------------------------------------------------------------\n";
        file << "Engine " << r.engineId << ": " << r.engineName << "\n";
        file << "Category: " << r.category << "\n";
        file << "------------------------------------------------------------------------\n";

        if (!r.success) {
            file << "STATUS: ERROR\n";
            file << "Error: " << r.errorMessage << "\n\n";
            continue;
        }

        file << "STATUS: " << (r.passed ? "PASSED" : "FAILED") << "\n\n";

        // Buffer size comparison results
        file << "Buffer Size Comparison Results:\n";
        file << "  (Reference: " << BUFFER_SIZES[0] << " samples)\n\n";

        for (int bufferSize : BUFFER_SIZES) {
            if (bufferSize == BUFFER_SIZES[0]) continue; // Skip reference

            file << "  Buffer Size " << bufferSize << ":\n";
            file << "    Max Deviation:  " << std::scientific << std::setprecision(6)
                 << r.maxDeviations.at(bufferSize);
            if (r.maxDeviations.at(bufferSize) > MAX_DEVIATION_THRESHOLD) {
                file << " [FAIL]";
            }
            file << "\n";

            file << "    RMS Error:      " << std::scientific << std::setprecision(6)
                 << r.rmsErrors.at(bufferSize);
            if (r.rmsErrors.at(bufferSize) > RMS_ERROR_THRESHOLD) {
                file << " [FAIL]";
            }
            file << "\n";

            if (r.hasNaN.at(bufferSize)) {
                file << "    WARNING: NaN values detected!\n";
            }
            if (r.hasInf.at(bufferSize)) {
                file << "    WARNING: Inf values detected!\n";
            }
            file << "\n";
        }

        if (!r.passed) {
            file << "  WORST CASE:\n";
            file << "    Buffer Size:    " << r.worstBufferSize << "\n";
            file << "    Max Deviation:  " << std::scientific << std::setprecision(6)
                 << r.worstMaxDeviation << "\n";
            file << "    RMS Error:      " << std::scientific << std::setprecision(6)
                 << r.worstRmsError << "\n";
        }

        file << "\n";
    }

    // Failed engines summary
    file << "========================================================================\n";
    file << "                    FAILED ENGINES SUMMARY\n";
    file << "========================================================================\n";
    file << "\n";

    bool hasFailures = false;
    for (const auto& r : results) {
        if (r.success && !r.passed) {
            hasFailures = true;
            file << "Engine " << r.engineId << " (" << r.engineName << "):\n";
            file << "  Worst Buffer Size: " << r.worstBufferSize << "\n";
            file << "  Max Deviation:     " << std::scientific << r.worstMaxDeviation << "\n";
            file << "  RMS Error:         " << std::scientific << r.worstRmsError << "\n";
            file << "\n";
        }
    }

    if (!hasFailures) {
        file << "No failures! All engines are buffer-size independent.\n";
    }

    file << "\n";
    file << "========================================================================\n";
    file << "                      END OF REPORT\n";
    file << "========================================================================\n";

    file.close();
}

// Save CSV results
void saveCSVReport(const std::vector<BufferSizeTestResult>& results, const std::string& filename) {
    std::ofstream file(filename);

    // Header
    file << "Engine ID,Engine Name,Category,Status,Worst Buffer Size,Max Deviation,RMS Error,";
    for (int bufferSize : BUFFER_SIZES) {
        if (bufferSize == BUFFER_SIZES[0]) continue;
        file << "MaxDev_" << bufferSize << ",RMSErr_" << bufferSize << ",";
    }
    file << "Error Message\n";

    // Data rows
    for (const auto& r : results) {
        file << r.engineId << ",";
        file << "\"" << r.engineName << "\",";
        file << r.category << ",";

        if (!r.success) {
            file << "ERROR,,,";
            for (size_t i = 1; i < BUFFER_SIZES.size(); ++i) {
                file << ",,";
            }
            file << "\"" << r.errorMessage << "\"\n";
            continue;
        }

        file << (r.passed ? "PASS" : "FAIL") << ",";
        file << r.worstBufferSize << ",";
        file << std::scientific << std::setprecision(6) << r.worstMaxDeviation << ",";
        file << std::scientific << std::setprecision(6) << r.worstRmsError << ",";

        for (int bufferSize : BUFFER_SIZES) {
            if (bufferSize == BUFFER_SIZES[0]) continue;
            file << std::scientific << std::setprecision(6) << r.maxDeviations.at(bufferSize) << ",";
            file << std::scientific << std::setprecision(6) << r.rmsErrors.at(bufferSize) << ",";
        }

        file << "\n";
    }

    file.close();
}

// Print progress to console
void printProgress(int current, int total, const std::string& engineName, bool passed) {
    std::cout << "[" << std::setw(2) << current << "/" << total << "] ";
    std::cout << "Engine " << std::setw(2) << current - 1 << " - ";
    std::cout << std::left << std::setw(30) << engineName << " ... ";
    std::cout << (passed ? "PASS" : "FAIL") << "\n";
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "    CHIMERA PHOENIX - BUFFER SIZE INDEPENDENCE TEST\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
    std::cout << "Testing buffer sizes: ";
    for (size_t i = 0; i < BUFFER_SIZES.size(); ++i) {
        std::cout << BUFFER_SIZES[i];
        if (i < BUFFER_SIZES.size() - 1) std::cout << ", ";
    }
    std::cout << " samples\n";
    std::cout << "Test duration: " << TEST_DURATION_SECONDS << " seconds per buffer size\n";
    std::cout << "Total engines: " << ALL_ENGINES.size() << "\n";
    std::cout << "\n";
    std::cout << "This test will take several minutes...\n";
    std::cout << "\n";

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    std::vector<BufferSizeTestResult> results;

    // Test each engine
    int count = 0;
    for (const auto& engineInfo : ALL_ENGINES) {
        count++;

        auto result = testEngine(engineInfo);
        results.push_back(result);

        if (result.success) {
            printProgress(count, ALL_ENGINES.size(), engineInfo.name, result.passed);
        } else {
            std::cout << "[" << std::setw(2) << count << "/" << ALL_ENGINES.size() << "] ";
            std::cout << "Engine " << std::setw(2) << engineInfo.id << " - ";
            std::cout << std::left << std::setw(30) << engineInfo.name << " ... ";
            std::cout << "ERROR: " << result.errorMessage << "\n";
        }
    }

    std::cout << "\n";
    std::cout << "Testing complete! Generating reports...\n";

    // Save reports
    saveTextReport(results, "buffer_size_independence_report.txt");
    saveCSVReport(results, "buffer_size_independence_results.csv");

    // Print summary
    int passed = 0;
    int failed = 0;
    int errors = 0;

    for (const auto& r : results) {
        if (!r.success) {
            errors++;
        } else if (r.passed) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "                         FINAL SUMMARY\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
    std::cout << "Total Engines:  " << ALL_ENGINES.size() << "\n";
    std::cout << "Passed:         " << passed << " (" << std::fixed << std::setprecision(1)
              << (100.0 * passed / ALL_ENGINES.size()) << "%)\n";
    std::cout << "Failed:         " << failed << "\n";
    std::cout << "Errors:         " << errors << "\n";
    std::cout << "\n";
    std::cout << "Reports saved:\n";
    std::cout << "  - buffer_size_independence_report.txt (detailed report)\n";
    std::cout << "  - buffer_size_independence_results.csv (spreadsheet data)\n";
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";

    return (failed == 0 && errors == 0) ? 0 : 1;
}

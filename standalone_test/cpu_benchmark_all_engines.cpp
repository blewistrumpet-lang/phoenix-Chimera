/**
 * cpu_benchmark_all_engines.cpp
 *
 * Comprehensive CPU benchmark for all 56 engines in Chimera Phoenix.
 *
 * Measures CPU performance by:
 * - Processing 10 seconds of audio per engine at 48kHz
 * - Measuring wall-clock time taken
 * - Calculating CPU percentage (time taken / real time * 100)
 * - Generating detailed CSV report with rankings
 *
 * Usage: ./cpu_benchmark_all_engines
 * Output: cpu_benchmark_results.csv
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cmath>

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
};

// All 56 engines with metadata
const std::vector<EngineMetadata> ALL_ENGINES = {
    // ENGINE_NONE (0)
    {0, "None (Bypass)", "Utility"},

    // DYNAMICS & COMPRESSION (1-6)
    {1, "Vintage Opto Compressor", "Dynamics"},
    {2, "Classic VCA Compressor", "Dynamics"},
    {3, "Transient Shaper", "Dynamics"},
    {4, "Noise Gate", "Dynamics"},
    {5, "Mastering Limiter", "Dynamics"},
    {6, "Dynamic EQ", "Dynamics"},

    // FILTERS & EQ (7-14)
    {7, "Parametric EQ (Studio)", "Filter"},
    {8, "Vintage Console EQ", "Filter"},
    {9, "Ladder Filter", "Filter"},
    {10, "State Variable Filter", "Filter"},
    {11, "Formant Filter", "Filter"},
    {12, "Envelope Filter", "Filter"},
    {13, "Comb Resonator", "Filter"},
    {14, "Vocal Formant Filter", "Filter"},

    // DISTORTION & SATURATION (15-22)
    {15, "Vintage Tube Preamp", "Distortion"},
    {16, "Wave Folder", "Distortion"},
    {17, "Harmonic Exciter", "Distortion"},
    {18, "Bit Crusher", "Distortion"},
    {19, "Multiband Saturator", "Distortion"},
    {20, "Muff Fuzz", "Distortion"},
    {21, "Rodent Distortion", "Distortion"},
    {22, "K-Style Overdrive", "Distortion"},

    // MODULATION (23-33)
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

    // DELAY (34-38)
    {34, "Tape Echo", "Delay"},
    {35, "Digital Delay", "Delay"},
    {36, "Magnetic Drum Echo", "Delay"},
    {37, "Bucket Brigade Delay", "Delay"},
    {38, "Buffer Repeat", "Delay"},

    // REVERB (39-43)
    {39, "Plate Reverb", "Reverb"},
    {40, "Spring Reverb", "Reverb"},
    {41, "Convolution Reverb", "Reverb"},
    {42, "Shimmer Reverb", "Reverb"},
    {43, "Gated Reverb", "Reverb"},

    // SPATIAL & SPECIAL (44-52)
    {44, "Stereo Widener", "Spatial"},
    {45, "Stereo Imager", "Spatial"},
    {46, "Dimension Expander", "Spatial"},
    {47, "Spectral Freeze", "Special"},
    {48, "Spectral Gate", "Special"},
    {49, "Phased Vocoder", "Special"},
    {50, "Granular Cloud", "Special"},
    {51, "Chaos Generator", "Special"},
    {52, "Feedback Network", "Special"},

    // UTILITY (53-56)
    {53, "Mid-Side Processor", "Utility"},
    {54, "Gain Utility", "Utility"},
    {55, "Mono Maker", "Utility"},
    {56, "Phase Align", "Utility"}
};

// Benchmark result structure
struct BenchmarkResult {
    int engineId;
    std::string engineName;
    std::string category;
    double processingTimeMs;
    double cpuPercentage;
    bool success;
    std::string errorMessage;

    // For sorting by CPU usage
    bool operator<(const BenchmarkResult& other) const {
        return cpuPercentage > other.cpuPercentage; // Descending order
    }
};

// Generate 10 seconds of test audio (sine wave at 440 Hz)
void generateTestAudio(juce::AudioBuffer<float>& buffer, double sampleRate) {
    const float frequency = 440.0f; // A4
    const float amplitude = 0.5f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float phase = 2.0f * juce::MathConstants<float>::pi * frequency * sample / (float)sampleRate;
            channelData[sample] = amplitude * std::sin(phase);
        }
    }
}

// Benchmark a single engine
BenchmarkResult benchmarkEngine(const EngineMetadata& metadata) {
    BenchmarkResult result;
    result.engineId = metadata.id;
    result.engineName = metadata.name;
    result.category = metadata.category;
    result.success = false;

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

        // Generate full test audio buffer
        juce::AudioBuffer<float> fullTestBuffer(numChannels, totalSamples);
        generateTestAudio(fullTestBuffer, sampleRate);

        // Processing buffer for blocks
        juce::AudioBuffer<float> blockBuffer(numChannels, blockSize);

        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();

        // Process audio in blocks
        for (int blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
            int startSample = blockIdx * blockSize;
            int numSamplesThisBlock = std::min(blockSize, totalSamples - startSample);

            // Copy samples to block buffer
            for (int ch = 0; ch < numChannels; ++ch) {
                blockBuffer.copyFrom(ch, 0, fullTestBuffer, ch, startSample, numSamplesThisBlock);

                // Zero remaining samples if partial block
                if (numSamplesThisBlock < blockSize) {
                    blockBuffer.clear(ch, numSamplesThisBlock, blockSize - numSamplesThisBlock);
                }
            }

            // Process the block
            engine->process(blockBuffer);
        }

        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();

        // Calculate timing metrics
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        result.processingTimeMs = elapsed.count();

        // CPU percentage = (time taken / real time) * 100
        double realTimeMs = durationSeconds * 1000.0;
        result.cpuPercentage = (result.processingTimeMs / realTimeMs) * 100.0;

        result.success = true;

    } catch (const std::exception& e) {
        result.errorMessage = std::string("Exception: ") + e.what();
    } catch (...) {
        result.errorMessage = "Unknown exception";
    }

    return result;
}

// Save results to CSV
void saveResultsToCSV(const std::vector<BenchmarkResult>& results, const std::string& filename) {
    std::ofstream file(filename);

    // Write header
    file << "Rank,EngineID,EngineName,Category,ProcessingTime_ms,CPU_%,Status,Error\n";

    // Write results (already sorted)
    int rank = 1;
    for (const auto& result : results) {
        file << rank++ << ","
             << result.engineId << ","
             << "\"" << result.engineName << "\","
             << result.category << ","
             << std::fixed << std::setprecision(3) << result.processingTimeMs << ","
             << std::fixed << std::setprecision(2) << result.cpuPercentage << ","
             << (result.success ? "SUCCESS" : "FAILED") << ","
             << "\"" << result.errorMessage << "\"\n";
    }

    file.close();
}

// Print summary report
void printSummaryReport(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "              CHIMERA PHOENIX - CPU BENCHMARK RESULTS\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
    std::cout << "Test Configuration:\n";
    std::cout << "  - Sample Rate: 48 kHz\n";
    std::cout << "  - Block Size: 512 samples\n";
    std::cout << "  - Audio Duration: 10 seconds\n";
    std::cout << "  - Channels: Stereo (2)\n";
    std::cout << "  - Total Engines: " << results.size() << "\n";
    std::cout << "\n";

    // Count successes
    int successCount = 0;
    int failCount = 0;
    for (const auto& r : results) {
        if (r.success) successCount++;
        else failCount++;
    }

    std::cout << "Results: " << successCount << " succeeded, " << failCount << " failed\n";
    std::cout << "\n";

    std::cout << "========================================================================\n";
    std::cout << "                    TOP 10 MOST CPU-INTENSIVE ENGINES\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
    std::cout << std::left
              << std::setw(6) << "Rank"
              << std::setw(5) << "ID"
              << std::setw(35) << "Engine Name"
              << std::setw(12) << "Category"
              << std::setw(12) << "Time (ms)"
              << "CPU %\n";
    std::cout << "------------------------------------------------------------------------\n";

    for (int i = 0; i < std::min(10, (int)results.size()); ++i) {
        const auto& r = results[i];
        if (!r.success) continue;

        std::cout << std::left
                  << std::setw(6) << (i + 1)
                  << std::setw(5) << r.engineId
                  << std::setw(35) << r.engineName
                  << std::setw(12) << r.category
                  << std::fixed << std::setprecision(1)
                  << std::setw(12) << r.processingTimeMs
                  << std::fixed << std::setprecision(2)
                  << r.cpuPercentage << "%\n";
    }

    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "                         CATEGORY ANALYSIS\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";

    // Calculate category averages
    std::map<std::string, std::vector<double>> categoryData;
    for (const auto& r : results) {
        if (r.success) {
            categoryData[r.category].push_back(r.cpuPercentage);
        }
    }

    std::cout << std::left
              << std::setw(20) << "Category"
              << std::setw(10) << "Count"
              << std::setw(15) << "Avg CPU %"
              << std::setw(15) << "Max CPU %"
              << "Min CPU %\n";
    std::cout << "------------------------------------------------------------------------\n";

    for (const auto& [category, values] : categoryData) {
        double sum = 0.0;
        double maxVal = *std::max_element(values.begin(), values.end());
        double minVal = *std::min_element(values.begin(), values.end());
        for (double v : values) sum += v;
        double avg = sum / values.size();

        std::cout << std::left
                  << std::setw(20) << category
                  << std::setw(10) << values.size()
                  << std::fixed << std::setprecision(2)
                  << std::setw(15) << avg
                  << std::setw(15) << maxVal
                  << minVal << "\n";
    }

    std::cout << "\n";

    // Show any failures
    if (failCount > 0) {
        std::cout << "========================================================================\n";
        std::cout << "                            FAILED ENGINES\n";
        std::cout << "========================================================================\n";
        std::cout << "\n";

        for (const auto& r : results) {
            if (!r.success) {
                std::cout << "  [" << r.engineId << "] " << r.engineName
                          << " - " << r.errorMessage << "\n";
            }
        }
        std::cout << "\n";
    }

    std::cout << "========================================================================\n";
    std::cout << "Full results saved to: cpu_benchmark_results.csv\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "       CHIMERA PHOENIX - COMPREHENSIVE CPU BENCHMARK SUITE\n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
    std::cout << "Benchmarking all 56 engines...\n";
    std::cout << "Processing 10 seconds of audio per engine at 48 kHz\n";
    std::cout << "\n";

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    std::vector<BenchmarkResult> results;

    // Benchmark each engine
    int count = 0;
    for (const auto& engineMeta : ALL_ENGINES) {
        count++;
        std::cout << "[" << count << "/" << ALL_ENGINES.size() << "] "
                  << "Testing Engine " << engineMeta.id
                  << " (" << engineMeta.name << ")... " << std::flush;

        BenchmarkResult result = benchmarkEngine(engineMeta);
        results.push_back(result);

        if (result.success) {
            std::cout << "OK - " << std::fixed << std::setprecision(2)
                      << result.cpuPercentage << "% CPU\n";
        } else {
            std::cout << "FAILED - " << result.errorMessage << "\n";
        }
    }

    // Sort results by CPU usage (descending)
    std::sort(results.begin(), results.end());

    // Save to CSV
    std::string outputFile = "cpu_benchmark_results.csv";
    saveResultsToCSV(results, outputFile);

    // Print summary report
    printSummaryReport(results);

    return 0;
}

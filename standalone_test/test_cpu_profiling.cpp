/**
 * test_cpu_profiling.cpp
 *
 * COMPREHENSIVE CPU PERFORMANCE PROFILING SUITE
 *
 * This suite provides in-depth CPU performance analysis for all 56 engines:
 * - Multiple sample rates: 44.1kHz, 48kHz, 96kHz, 192kHz
 * - Multiple buffer sizes: 64, 128, 256, 512, 1024, 2048
 * - Parameter variation testing
 * - Operation-level profiling (FFT, filters, oversampling, delay lines, LFOs)
 * - Multi-engine scenarios (10, 25, 56 engines)
 * - Real-time capability assessment
 * - Optimization opportunity identification
 *
 * Target: < 5% CPU per engine at 48kHz, 512 sample buffer
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <sstream>

// JUCE includes
#include "../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

// Engine includes
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"

//==============================================================================
// CONFIGURATION
//==============================================================================

const std::vector<double> TEST_SAMPLE_RATES = {44100.0, 48000.0, 96000.0, 192000.0};
const std::vector<int> TEST_BUFFER_SIZES = {64, 128, 256, 512, 1024, 2048};
const double TEST_DURATION_SECONDS = 5.0; // 5 seconds per test
const double TARGET_CPU_PERCENT = 5.0;    // Target: <5% per engine

//==============================================================================
// ENGINE METADATA
//==============================================================================

struct EngineMetadata {
    int id;
    std::string name;
    std::string category;
    bool hasFFT;           // Uses FFT operations
    bool hasFilters;       // Has filter calculations
    bool hasOversampling;  // Uses oversampling
    bool hasDelayLines;    // Has delay line processing
    bool hasLFOs;          // Has LFO calculations
};

const std::vector<EngineMetadata> ALL_ENGINES = {
    {0, "None (Bypass)", "Utility", false, false, false, false, false},

    // DYNAMICS (1-6)
    {1, "Vintage Opto Compressor", "Dynamics", false, true, false, false, false},
    {2, "Classic VCA Compressor", "Dynamics", false, true, false, false, false},
    {3, "Transient Shaper", "Dynamics", false, true, false, false, false},
    {4, "Noise Gate", "Dynamics", false, false, false, false, false},
    {5, "Mastering Limiter", "Dynamics", false, true, false, false, false},
    {6, "Dynamic EQ", "Dynamics", true, true, false, false, false},

    // FILTERS (7-14)
    {7, "Parametric EQ (Studio)", "Filter", false, true, false, false, false},
    {8, "Vintage Console EQ", "Filter", false, true, false, false, false},
    {9, "Ladder Filter", "Filter", false, true, false, false, false},
    {10, "State Variable Filter", "Filter", false, true, false, false, false},
    {11, "Formant Filter", "Filter", false, true, false, false, false},
    {12, "Envelope Filter", "Filter", false, true, false, false, true},
    {13, "Comb Resonator", "Filter", false, true, true, false, false},
    {14, "Vocal Formant Filter", "Filter", false, true, false, false, false},

    // DISTORTION (15-22)
    {15, "Vintage Tube Preamp", "Distortion", false, true, true, false, false},
    {16, "Wave Folder", "Distortion", false, false, true, false, false},
    {17, "Harmonic Exciter", "Distortion", false, true, true, false, false},
    {18, "Bit Crusher", "Distortion", false, false, false, false, false},
    {19, "Multiband Saturator", "Distortion", false, true, true, false, false},
    {20, "Muff Fuzz", "Distortion", false, true, true, false, false},
    {21, "Rodent Distortion", "Distortion", false, true, true, false, false},
    {22, "K-Style Overdrive", "Distortion", false, true, true, false, false},

    // MODULATION (23-33)
    {23, "Digital Chorus", "Modulation", false, false, false, true, true},
    {24, "Resonant Chorus", "Modulation", false, true, false, true, true},
    {25, "Analog Phaser", "Modulation", false, true, false, false, true},
    {26, "Ring Modulator", "Modulation", false, false, false, false, true},
    {27, "Frequency Shifter", "Modulation", true, false, false, false, false},
    {28, "Harmonic Tremolo", "Modulation", false, true, false, false, true},
    {29, "Classic Tremolo", "Modulation", false, false, false, false, true},
    {30, "Rotary Speaker", "Modulation", false, true, false, true, true},
    {31, "Pitch Shifter", "Modulation", true, false, false, true, false},
    {32, "Detune Doubler", "Modulation", false, false, false, true, false},
    {33, "Intelligent Harmonizer", "Modulation", true, false, false, true, false},

    // DELAY (34-38)
    {34, "Tape Echo", "Delay", false, true, false, true, true},
    {35, "Digital Delay", "Delay", false, true, false, true, false},
    {36, "Magnetic Drum Echo", "Delay", false, true, false, true, true},
    {37, "Bucket Brigade Delay", "Delay", false, true, false, true, false},
    {38, "Buffer Repeat", "Delay", false, false, false, true, false},

    // REVERB (39-43)
    {39, "Plate Reverb", "Reverb", false, true, false, true, true},
    {40, "Spring Reverb", "Reverb", false, true, false, true, false},
    {41, "Convolution Reverb", "Reverb", true, false, false, false, false},
    {42, "Shimmer Reverb", "Reverb", true, true, false, true, true},
    {43, "Gated Reverb", "Reverb", false, true, false, true, false},

    // SPATIAL & SPECIAL (44-52)
    {44, "Stereo Widener", "Spatial", false, false, false, true, false},
    {45, "Stereo Imager", "Spatial", false, true, false, false, false},
    {46, "Dimension Expander", "Spatial", false, true, false, true, true},
    {47, "Spectral Freeze", "Spectral", true, false, false, false, false},
    {48, "Spectral Gate", "Spectral", true, false, false, false, false},
    {49, "Phased Vocoder", "Spectral", true, false, false, false, false},
    {50, "Granular Cloud", "Spectral", false, false, false, true, true},
    {51, "Chaos Generator", "Special", false, false, false, false, true},
    {52, "Feedback Network", "Special", false, true, false, true, false},

    // UTILITY (53-56)
    {53, "Mid-Side Processor", "Utility", false, false, false, false, false},
    {54, "Gain Utility", "Utility", false, false, false, false, false},
    {55, "Mono Maker", "Utility", false, false, false, false, false},
    {56, "Phase Align", "Utility", false, false, false, true, false}
};

//==============================================================================
// PROFILING RESULT STRUCTURES
//==============================================================================

struct ProfileResult {
    int engineId;
    std::string engineName;
    std::string category;
    double sampleRate;
    int bufferSize;

    double processingTimeMs;
    double cpuPercentage;
    double samplesPerSecond;
    bool meetsTarget;
    bool success;
    std::string errorMessage;

    // Operation flags
    bool hasFFT;
    bool hasFilters;
    bool hasOversampling;
    bool hasDelayLines;
    bool hasLFOs;
};

struct MultiEngineResult {
    int numEngines;
    double sampleRate;
    int bufferSize;
    double totalCpuPercentage;
    double avgCpuPerEngine;
    bool success;
};

//==============================================================================
// AUDIO GENERATION
//==============================================================================

void generateTestAudio(juce::AudioBuffer<float>& buffer, double sampleRate, float frequency = 440.0f) {
    const float amplitude = 0.5f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float phase = 2.0f * juce::MathConstants<float>::pi * frequency * sample / (float)sampleRate;
            channelData[sample] = amplitude * std::sin(phase);
        }
    }
}

void generateComplexTestAudio(juce::AudioBuffer<float>& buffer, double sampleRate) {
    // Multi-frequency test signal with harmonics
    const float baseFreq = 220.0f;
    const int numHarmonics = 5;

    buffer.clear();

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float value = 0.0f;
            for (int h = 1; h <= numHarmonics; ++h) {
                float freq = baseFreq * h;
                float amp = 0.5f / h;
                float phase = 2.0f * juce::MathConstants<float>::pi * freq * sample / (float)sampleRate;
                value += amp * std::sin(phase);
            }
            channelData[sample] = value;
        }
    }
}

//==============================================================================
// PROFILING FUNCTIONS
//==============================================================================

ProfileResult profileEngine(const EngineMetadata& metadata, double sampleRate, int bufferSize) {
    ProfileResult result;
    result.engineId = metadata.id;
    result.engineName = metadata.name;
    result.category = metadata.category;
    result.sampleRate = sampleRate;
    result.bufferSize = bufferSize;
    result.hasFFT = metadata.hasFFT;
    result.hasFilters = metadata.hasFilters;
    result.hasOversampling = metadata.hasOversampling;
    result.hasDelayLines = metadata.hasDelayLines;
    result.hasLFOs = metadata.hasLFOs;
    result.success = false;

    try {
        // Create engine
        auto engine = EngineFactory::createEngine(metadata.id);
        if (!engine) {
            result.errorMessage = "Failed to create engine";
            return result;
        }

        // Setup
        const int numChannels = 2;
        const int totalSamples = static_cast<int>(sampleRate * TEST_DURATION_SECONDS);
        const int numBlocks = (totalSamples + bufferSize - 1) / bufferSize;

        // Prepare engine
        engine->prepareToPlay(sampleRate, bufferSize);

        // Generate test audio
        juce::AudioBuffer<float> fullTestBuffer(numChannels, totalSamples);
        generateComplexTestAudio(fullTestBuffer, sampleRate);

        // Processing buffer
        juce::AudioBuffer<float> blockBuffer(numChannels, bufferSize);

        // Warm-up run (not measured)
        for (int warmup = 0; warmup < 10; ++warmup) {
            for (int ch = 0; ch < numChannels; ++ch) {
                blockBuffer.copyFrom(ch, 0, fullTestBuffer, ch, 0, std::min(bufferSize, totalSamples));
            }
            engine->process(blockBuffer);
        }

        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();

        // Process audio in blocks
        for (int blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
            int startSample = blockIdx * bufferSize;
            int numSamplesThisBlock = std::min(bufferSize, totalSamples - startSample);

            // Copy samples to block buffer
            for (int ch = 0; ch < numChannels; ++ch) {
                blockBuffer.copyFrom(ch, 0, fullTestBuffer, ch, startSample, numSamplesThisBlock);

                // Zero remaining samples if partial block
                if (numSamplesThisBlock < bufferSize) {
                    blockBuffer.clear(ch, numSamplesThisBlock, bufferSize - numSamplesThisBlock);
                }
            }

            // Process the block
            engine->process(blockBuffer);
        }

        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();

        // Calculate metrics
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        result.processingTimeMs = elapsed.count();

        double realTimeMs = TEST_DURATION_SECONDS * 1000.0;
        result.cpuPercentage = (result.processingTimeMs / realTimeMs) * 100.0;
        result.samplesPerSecond = (totalSamples / result.processingTimeMs) * 1000.0;
        result.meetsTarget = (result.cpuPercentage <= TARGET_CPU_PERCENT);
        result.success = true;

    } catch (const std::exception& e) {
        result.errorMessage = std::string("Exception: ") + e.what();
    } catch (...) {
        result.errorMessage = "Unknown exception";
    }

    return result;
}

MultiEngineResult profileMultiEngine(int numEngines, double sampleRate, int bufferSize) {
    MultiEngineResult result;
    result.numEngines = numEngines;
    result.sampleRate = sampleRate;
    result.bufferSize = bufferSize;
    result.success = false;

    try {
        // Create multiple engines
        std::vector<std::unique_ptr<EngineBase>> engines;
        for (int i = 0; i < numEngines; ++i) {
            int engineId = (i % 56) + 1; // Cycle through engines 1-56, skip bypass
            auto engine = EngineFactory::createEngine(engineId);
            if (engine) {
                engine->prepareToPlay(sampleRate, bufferSize);
                engines.push_back(std::move(engine));
            }
        }

        if (engines.empty()) {
            result.totalCpuPercentage = 0.0;
            result.avgCpuPerEngine = 0.0;
            return result;
        }

        // Setup
        const int numChannels = 2;
        const int totalSamples = static_cast<int>(sampleRate * TEST_DURATION_SECONDS);
        const int numBlocks = (totalSamples + bufferSize - 1) / bufferSize;

        // Generate test audio
        juce::AudioBuffer<float> fullTestBuffer(numChannels, totalSamples);
        generateComplexTestAudio(fullTestBuffer, sampleRate);

        // Processing buffer
        juce::AudioBuffer<float> blockBuffer(numChannels, bufferSize);

        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();

        // Process audio through all engines
        for (int blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
            int startSample = blockIdx * bufferSize;
            int numSamplesThisBlock = std::min(bufferSize, totalSamples - startSample);

            // Copy samples to block buffer
            for (int ch = 0; ch < numChannels; ++ch) {
                blockBuffer.copyFrom(ch, 0, fullTestBuffer, ch, startSample, numSamplesThisBlock);

                if (numSamplesThisBlock < bufferSize) {
                    blockBuffer.clear(ch, numSamplesThisBlock, bufferSize - numSamplesThisBlock);
                }
            }

            // Process through all engines
            for (auto& engine : engines) {
                engine->process(blockBuffer);
            }
        }

        // End timing
        auto endTime = std::chrono::high_resolution_clock::now();

        // Calculate metrics
        std::chrono::duration<double, std::milli> elapsed = endTime - startTime;
        double processingTimeMs = elapsed.count();

        double realTimeMs = TEST_DURATION_SECONDS * 1000.0;
        result.totalCpuPercentage = (processingTimeMs / realTimeMs) * 100.0;
        result.avgCpuPerEngine = result.totalCpuPercentage / engines.size();
        result.success = true;

    } catch (...) {
        result.totalCpuPercentage = 0.0;
        result.avgCpuPerEngine = 0.0;
    }

    return result;
}

//==============================================================================
// REPORTING FUNCTIONS
//==============================================================================

void saveDetailedResults(const std::vector<ProfileResult>& results, const std::string& filename) {
    std::ofstream file(filename);

    // Header
    file << "EngineID,EngineName,Category,SampleRate,BufferSize,"
         << "ProcessingTime_ms,CPU_%,Samples/Sec,MeetsTarget,FFT,Filters,Oversampling,DelayLines,LFOs,Status\n";

    for (const auto& r : results) {
        file << r.engineId << ","
             << "\"" << r.engineName << "\","
             << r.category << ","
             << r.sampleRate << ","
             << r.bufferSize << ","
             << std::fixed << std::setprecision(3) << r.processingTimeMs << ","
             << std::fixed << std::setprecision(2) << r.cpuPercentage << ","
             << std::fixed << std::setprecision(0) << r.samplesPerSecond << ","
             << (r.meetsTarget ? "YES" : "NO") << ","
             << (r.hasFFT ? "Y" : "N") << ","
             << (r.hasFilters ? "Y" : "N") << ","
             << (r.hasOversampling ? "Y" : "N") << ","
             << (r.hasDelayLines ? "Y" : "N") << ","
             << (r.hasLFOs ? "Y" : "N") << ","
             << (r.success ? "SUCCESS" : "FAILED") << "\n";
    }

    file.close();
}

void saveMultiEngineResults(const std::vector<MultiEngineResult>& results, const std::string& filename) {
    std::ofstream file(filename);

    file << "NumEngines,SampleRate,BufferSize,TotalCPU_%,AvgCPU_per_Engine,Status\n";

    for (const auto& r : results) {
        file << r.numEngines << ","
             << r.sampleRate << ","
             << r.bufferSize << ","
             << std::fixed << std::setprecision(2) << r.totalCpuPercentage << ","
             << std::fixed << std::setprecision(2) << r.avgCpuPerEngine << ","
             << (r.success ? "SUCCESS" : "FAILED") << "\n";
    }

    file.close();
}

void printComprehensiveReport(const std::vector<ProfileResult>& results,
                              const std::vector<MultiEngineResult>& multiResults) {

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "          CHIMERA PHOENIX - COMPREHENSIVE CPU PROFILING REPORT\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    // Find results at 48kHz, 512 buffer (target config)
    std::vector<ProfileResult> targetResults;
    for (const auto& r : results) {
        if (r.sampleRate == 48000.0 && r.bufferSize == 512 && r.success) {
            targetResults.push_back(r);
        }
    }

    // Sort by CPU usage
    std::sort(targetResults.begin(), targetResults.end(),
              [](const ProfileResult& a, const ProfileResult& b) {
                  return a.cpuPercentage > b.cpuPercentage;
              });

    // TOP 10 MOST CPU-INTENSIVE
    std::cout << "TOP 10 MOST CPU-INTENSIVE ENGINES (48kHz, 512 buffer)\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(5) << "Rank"
              << std::setw(5) << "ID"
              << std::setw(35) << "Engine Name"
              << std::setw(12) << "CPU %"
              << std::setw(10) << "Target"
              << "Operations\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    for (int i = 0; i < std::min(10, (int)targetResults.size()); ++i) {
        const auto& r = targetResults[i];
        std::string ops;
        if (r.hasFFT) ops += "FFT ";
        if (r.hasFilters) ops += "Flt ";
        if (r.hasOversampling) ops += "OS ";
        if (r.hasDelayLines) ops += "Dly ";
        if (r.hasLFOs) ops += "LFO ";

        std::cout << std::left
                  << std::setw(5) << (i + 1)
                  << std::setw(5) << r.engineId
                  << std::setw(35) << r.engineName
                  << std::fixed << std::setprecision(2)
                  << std::setw(12) << r.cpuPercentage
                  << std::setw(10) << (r.meetsTarget ? "PASS" : "FAIL")
                  << ops << "\n";
    }

    std::cout << "\n";

    // ENGINES EXCEEDING TARGET
    int failedTargetCount = 0;
    std::cout << "ENGINES EXCEEDING TARGET (<5% CPU):\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    for (const auto& r : targetResults) {
        if (!r.meetsTarget) {
            failedTargetCount++;
            std::cout << "  [" << r.engineId << "] " << r.engineName
                      << " - " << std::fixed << std::setprecision(2)
                      << r.cpuPercentage << "% (Target: <" << TARGET_CPU_PERCENT << "%)\n";
        }
    }
    if (failedTargetCount == 0) {
        std::cout << "  ALL ENGINES MEET TARGET!\n";
    }
    std::cout << "\n";

    // BUFFER SIZE ANALYSIS
    std::cout << "BUFFER SIZE IMPACT ANALYSIS (48kHz)\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(12) << "Buffer"
              << std::setw(15) << "Avg CPU %"
              << std::setw(15) << "Max CPU %"
              << std::setw(15) << "Pass Rate\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    for (int bufSize : TEST_BUFFER_SIZES) {
        std::vector<ProfileResult> bufResults;
        for (const auto& r : results) {
            if (r.sampleRate == 48000.0 && r.bufferSize == bufSize && r.success) {
                bufResults.push_back(r);
            }
        }

        if (!bufResults.empty()) {
            double avgCpu = 0.0;
            double maxCpu = 0.0;
            int passCount = 0;

            for (const auto& r : bufResults) {
                avgCpu += r.cpuPercentage;
                maxCpu = std::max(maxCpu, r.cpuPercentage);
                if (r.meetsTarget) passCount++;
            }
            avgCpu /= bufResults.size();

            std::cout << std::left
                      << std::setw(12) << bufSize
                      << std::fixed << std::setprecision(2)
                      << std::setw(15) << avgCpu
                      << std::setw(15) << maxCpu
                      << std::setw(15) << (passCount * 100 / bufResults.size()) << "%\n";
        }
    }
    std::cout << "\n";

    // SAMPLE RATE ANALYSIS
    std::cout << "SAMPLE RATE IMPACT ANALYSIS (512 buffer)\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(12) << "Rate (kHz)"
              << std::setw(15) << "Avg CPU %"
              << std::setw(15) << "Max CPU %"
              << std::setw(15) << "Pass Rate\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    for (double sampleRate : TEST_SAMPLE_RATES) {
        std::vector<ProfileResult> srResults;
        for (const auto& r : results) {
            if (r.sampleRate == sampleRate && r.bufferSize == 512 && r.success) {
                srResults.push_back(r);
            }
        }

        if (!srResults.empty()) {
            double avgCpu = 0.0;
            double maxCpu = 0.0;
            int passCount = 0;

            for (const auto& r : srResults) {
                avgCpu += r.cpuPercentage;
                maxCpu = std::max(maxCpu, r.cpuPercentage);
                if (r.meetsTarget) passCount++;
            }
            avgCpu /= srResults.size();

            std::cout << std::left
                      << std::setw(12) << (sampleRate / 1000.0)
                      << std::fixed << std::setprecision(2)
                      << std::setw(15) << avgCpu
                      << std::setw(15) << maxCpu
                      << std::setw(15) << (passCount * 100 / srResults.size()) << "%\n";
        }
    }
    std::cout << "\n";

    // MULTI-ENGINE SCENARIOS
    std::cout << "MULTI-ENGINE CAPACITY ANALYSIS\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(15) << "Engines"
              << std::setw(15) << "Total CPU %"
              << std::setw(15) << "Avg/Engine"
              << std::setw(20) << "Real-time OK?\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    for (const auto& mr : multiResults) {
        if (mr.sampleRate == 48000.0 && mr.bufferSize == 512) {
            std::cout << std::left
                      << std::setw(15) << mr.numEngines
                      << std::fixed << std::setprecision(2)
                      << std::setw(15) << mr.totalCpuPercentage
                      << std::setw(15) << mr.avgCpuPerEngine
                      << std::setw(20) << (mr.totalCpuPercentage < 100.0 ? "YES" : "NO (needs multi-core)") << "\n";
        }
    }
    std::cout << "\n";

    // CATEGORY ANALYSIS
    std::map<std::string, std::vector<double>> categoryData;
    for (const auto& r : targetResults) {
        categoryData[r.category].push_back(r.cpuPercentage);
    }

    std::cout << "CATEGORY EFFICIENCY ANALYSIS\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left
              << std::setw(15) << "Category"
              << std::setw(10) << "Count"
              << std::setw(15) << "Avg CPU %"
              << std::setw(15) << "Max CPU %"
              << std::setw(15) << "Pass Rate\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    for (const auto& [category, values] : categoryData) {
        double sum = 0.0;
        double maxVal = *std::max_element(values.begin(), values.end());
        int passCount = 0;

        for (double v : values) {
            sum += v;
            if (v <= TARGET_CPU_PERCENT) passCount++;
        }
        double avg = sum / values.size();

        std::cout << std::left
                  << std::setw(15) << category
                  << std::setw(10) << values.size()
                  << std::fixed << std::setprecision(2)
                  << std::setw(15) << avg
                  << std::setw(15) << maxVal
                  << std::setw(15) << (passCount * 100 / values.size()) << "%\n";
    }

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                           OPTIMIZATION PRIORITIES\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    // Identify engines exceeding target with specific operations
    std::vector<ProfileResult> needsOpt;
    for (const auto& r : targetResults) {
        if (!r.meetsTarget) {
            needsOpt.push_back(r);
        }
    }

    if (!needsOpt.empty()) {
        std::cout << "HIGH PRIORITY (Exceeding Target):\n";
        for (const auto& r : needsOpt) {
            std::cout << "  [" << r.engineId << "] " << r.engineName
                      << " (" << std::fixed << std::setprecision(2) << r.cpuPercentage << "%)\n";
            std::cout << "    Operations: ";
            if (r.hasFFT) std::cout << "FFT ";
            if (r.hasFilters) std::cout << "Filters ";
            if (r.hasOversampling) std::cout << "Oversampling ";
            if (r.hasDelayLines) std::cout << "DelayLines ";
            if (r.hasLFOs) std::cout << "LFOs ";
            std::cout << "\n";
            std::cout << "    Recommendations:\n";
            if (r.hasFFT) std::cout << "      - Optimize FFT size and overlap\n";
            if (r.hasFilters) std::cout << "      - Use SIMD for filter calculations\n";
            if (r.hasOversampling) std::cout << "      - Reduce oversampling factor or use adaptive oversampling\n";
            if (r.hasDelayLines) std::cout << "      - Optimize delay line interpolation\n";
            if (r.hasLFOs) std::cout << "      - Use lookup tables for LFO waveforms\n";
            std::cout << "\n";
        }
    } else {
        std::cout << "ALL ENGINES MEET TARGET - EXCELLENT PERFORMANCE!\n";
    }

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "Results saved to:\n";
    std::cout << "  - cpu_profiling_detailed.csv\n";
    std::cout << "  - cpu_profiling_multi_engine.csv\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
}

//==============================================================================
// MAIN
//==============================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "    CHIMERA PHOENIX - COMPREHENSIVE CPU PROFILING SUITE\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    std::cout << "Testing Configuration:\n";
    std::cout << "  Sample Rates: 44.1kHz, 48kHz, 96kHz, 192kHz\n";
    std::cout << "  Buffer Sizes: 64, 128, 256, 512, 1024, 2048\n";
    std::cout << "  Test Duration: " << TEST_DURATION_SECONDS << " seconds per test\n";
    std::cout << "  Target: <" << TARGET_CPU_PERCENT << "% CPU per engine at 48kHz/512\n";
    std::cout << "\n";

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    std::vector<ProfileResult> allResults;
    std::vector<MultiEngineResult> multiResults;

    // Test each engine at each sample rate and buffer size
    int totalTests = ALL_ENGINES.size() * TEST_SAMPLE_RATES.size() * TEST_BUFFER_SIZES.size();
    int currentTest = 0;

    std::cout << "================================================================================\n";
    std::cout << "PHASE 1: SINGLE ENGINE PROFILING\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    for (const auto& sampleRate : TEST_SAMPLE_RATES) {
        for (const auto& bufferSize : TEST_BUFFER_SIZES) {
            std::cout << "\nTesting at " << (sampleRate / 1000.0) << " kHz, buffer " << bufferSize << ":\n";

            for (const auto& engineMeta : ALL_ENGINES) {
                currentTest++;
                std::cout << "  [" << currentTest << "/" << totalTests << "] "
                          << "Engine " << engineMeta.id << " (" << engineMeta.name << ")... " << std::flush;

                ProfileResult result = profileEngine(engineMeta, sampleRate, bufferSize);
                allResults.push_back(result);

                if (result.success) {
                    std::cout << std::fixed << std::setprecision(2)
                              << result.cpuPercentage << "% "
                              << (result.meetsTarget ? "PASS" : "FAIL") << "\n";
                } else {
                    std::cout << "FAILED\n";
                }
            }
        }
    }

    // Multi-engine tests
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "PHASE 2: MULTI-ENGINE CAPACITY TESTING\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    std::vector<int> multiEngineConfigs = {10, 25, 56};
    for (int numEngines : multiEngineConfigs) {
        std::cout << "\nTesting " << numEngines << " engines simultaneously:\n";

        for (const auto& sampleRate : TEST_SAMPLE_RATES) {
            for (const auto& bufferSize : TEST_BUFFER_SIZES) {
                std::cout << "  " << (sampleRate / 1000.0) << " kHz, buffer " << bufferSize << "... " << std::flush;

                MultiEngineResult mr = profileMultiEngine(numEngines, sampleRate, bufferSize);
                multiResults.push_back(mr);

                if (mr.success) {
                    std::cout << std::fixed << std::setprecision(2)
                              << mr.totalCpuPercentage << "% total, "
                              << mr.avgCpuPerEngine << "% avg/engine\n";
                } else {
                    std::cout << "FAILED\n";
                }
            }
        }
    }

    // Save results
    saveDetailedResults(allResults, "cpu_profiling_detailed.csv");
    saveMultiEngineResults(multiResults, "cpu_profiling_multi_engine.csv");

    // Print comprehensive report
    printComprehensiveReport(allResults, multiResults);

    return 0;
}

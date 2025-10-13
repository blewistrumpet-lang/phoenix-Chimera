/**
 * test_all_engines_stereo.cpp
 *
 * Comprehensive stereo analysis test for ALL 56 engines in Chimera Phoenix.
 *
 * Generates stereo data for each engine by:
 * - Processing a stereo test signal (sine wave with slight L/R phase difference)
 * - Capturing output for stereo analysis
 * - Saving L/R channel data to CSV files
 *
 * Output: stereo_engine_<ID>.csv for each engine
 *
 * This data is then analyzed by stereo_analysis_suite.py to measure:
 * - L/R correlation
 * - Stereo width
 * - Phase coherence
 * - Mono collapse detection
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <cmath>
#include <iomanip>
#include <map>

// JUCE includes
#include "../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

// Engine Factory
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"

// Engine metadata
struct EngineMetadata {
    int id;
    std::string name;
    std::string category;
    bool shouldBeStereo;  // Expected stereo behavior
};

// All 56 engines
const std::vector<EngineMetadata> ALL_ENGINES = {
    {0, "None (Bypass)", "Utility", false},

    // DYNAMICS (1-6)
    {1, "Vintage Opto Compressor", "Dynamics", true},
    {2, "Classic VCA Compressor", "Dynamics", true},
    {3, "Transient Shaper", "Dynamics", true},
    {4, "Noise Gate", "Dynamics", true},
    {5, "Mastering Limiter", "Dynamics", true},
    {6, "Dynamic EQ", "Dynamics", true},

    // FILTERS (7-14)
    {7, "Parametric EQ (Studio)", "Filter", true},
    {8, "Vintage Console EQ", "Filter", true},
    {9, "Ladder Filter", "Filter", true},
    {10, "State Variable Filter", "Filter", true},
    {11, "Formant Filter", "Filter", true},
    {12, "Envelope Filter", "Filter", true},
    {13, "Comb Resonator", "Filter", true},
    {14, "Vocal Formant Filter", "Filter", true},

    // DISTORTION (15-22)
    {15, "Vintage Tube Preamp", "Distortion", true},
    {16, "Wave Folder", "Distortion", true},
    {17, "Harmonic Exciter", "Distortion", true},
    {18, "Bit Crusher", "Distortion", true},
    {19, "Multiband Saturator", "Distortion", true},
    {20, "Muff Fuzz", "Distortion", true},
    {21, "Rodent Distortion", "Distortion", true},
    {22, "K-Style Overdrive", "Distortion", true},

    // MODULATION (23-33)
    {23, "Digital Chorus", "Modulation", true},
    {24, "Resonant Chorus", "Modulation", true},
    {25, "Analog Phaser", "Modulation", true},
    {26, "Ring Modulator", "Modulation", true},
    {27, "Frequency Shifter", "Modulation", true},
    {28, "Harmonic Tremolo", "Modulation", true},
    {29, "Classic Tremolo", "Modulation", true},
    {30, "Rotary Speaker", "Modulation", true},
    {31, "Pitch Shifter", "Modulation", true},
    {32, "Detune Doubler", "Modulation", true},
    {33, "Intelligent Harmonizer", "Modulation", true},

    // DELAY (34-38)
    {34, "Tape Echo", "Delay", true},
    {35, "Digital Delay", "Delay", true},
    {36, "Magnetic Drum Echo", "Delay", true},
    {37, "Bucket Brigade Delay", "Delay", true},
    {38, "Buffer Repeat", "Delay", true},

    // REVERB (39-43)
    {39, "Plate Reverb", "Reverb", true},
    {40, "Spring Reverb", "Reverb", true},
    {41, "Convolution Reverb", "Reverb", true},
    {42, "Shimmer Reverb", "Reverb", true},
    {43, "Gated Reverb", "Reverb", true},

    // SPATIAL (44-46)
    {44, "Stereo Widener", "Spatial", true},
    {45, "Stereo Imager", "Spatial", true},
    {46, "Dimension Expander", "Spatial", true},

    // SPECIAL (47-52)
    {47, "Spectral Freeze", "Special", true},
    {48, "Spectral Gate", "Special", true},
    {49, "Phased Vocoder", "Special", true},
    {50, "Granular Cloud", "Special", true},
    {51, "Chaos Generator", "Special", true},
    {52, "Feedback Network", "Special", true},

    // UTILITY (53-56)
    {53, "Mid-Side Processor", "Utility", true},
    {54, "Gain Utility", "Utility", true},
    {55, "Mono Maker", "Utility", false},  // Intentionally mono
    {56, "Phase Align", "Utility", true}
};

// Get default parameters for an engine
std::map<int, float> getDefaultParams(int engineId) {
    std::map<int, float> params;

    // Common defaults
    params[0] = 0.5f;  // Mix/Wet
    params[1] = 0.5f;  // Generic param 1
    params[2] = 0.5f;  // Generic param 2
    params[3] = 0.5f;  // Generic param 3
    params[4] = 0.5f;  // Generic param 4

    // Engine-specific parameters
    switch (engineId) {
        case 1: case 2:  // Compressors
            params[0] = 0.8f;  // Mix
            params[1] = 0.6f;  // Threshold
            params[2] = 0.5f;  // Ratio
            params[3] = 0.3f;  // Attack
            params[4] = 0.5f;  // Release
            break;

        case 23: case 24:  // Chorus
            params[0] = 0.7f;  // Mix
            params[1] = 0.5f;  // Rate
            params[2] = 0.6f;  // Depth
            break;

        case 25:  // Phaser
            params[0] = 0.7f;  // Mix
            params[1] = 0.4f;  // Rate
            params[2] = 0.6f;  // Depth
            params[3] = 0.5f;  // Feedback
            break;

        case 28: case 29:  // Tremolo
            params[0] = 1.0f;  // Mix
            params[1] = 0.4f;  // Rate
            params[2] = 0.6f;  // Depth
            break;

        case 30:  // Rotary Speaker
            params[0] = 0.8f;  // Mix
            params[1] = 0.5f;  // Speed
            params[2] = 0.6f;  // Depth
            break;

        case 32:  // Detune Doubler
            params[0] = 0.7f;  // Mix
            params[1] = 0.3f;  // Detune
            params[2] = 0.5f;  // Delay
            break;

        case 35:  // Digital Delay
            params[0] = 0.5f;  // Mix
            params[1] = 0.4f;  // Time
            params[2] = 0.3f;  // Feedback
            break;

        case 39: case 40: case 41: case 42:  // Reverbs
            params[0] = 0.5f;  // Mix
            params[1] = 0.6f;  // Decay/Size
            params[2] = 0.5f;  // Damping
            break;

        case 44: case 45: case 46:  // Spatial
            params[0] = 1.0f;  // Mix
            params[1] = 0.6f;  // Width
            break;
    }

    return params;
}

// Generate stereo test signal (slight phase difference between L/R)
void generateStereoTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate) {
    const float frequency = 440.0f;  // A4
    const float amplitude = 0.5f;
    const float phaseOffsetDegrees = 15.0f;  // Slight phase difference for stereo
    const float phaseOffsetRadians = phaseOffsetDegrees * juce::MathConstants<float>::pi / 180.0f;

    int numSamples = buffer.getNumSamples();

    // Left channel
    float* leftData = buffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i) {
        float phase = 2.0f * juce::MathConstants<float>::pi * frequency * i / (float)sampleRate;
        leftData[i] = amplitude * std::sin(phase);
    }

    // Right channel (with slight phase offset)
    float* rightData = buffer.getWritePointer(1);
    for (int i = 0; i < numSamples; ++i) {
        float phase = 2.0f * juce::MathConstants<float>::pi * frequency * i / (float)sampleRate + phaseOffsetRadians;
        rightData[i] = amplitude * std::sin(phase);
    }
}

// Save stereo data to CSV
bool saveStereoCSV(const std::string& filename, const juce::AudioBuffer<float>& buffer, double sampleRate) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    // Write header
    file << "sample,time_s,L,R\n";

    // Write data
    int numSamples = buffer.getNumSamples();
    const float* leftData = buffer.getReadPointer(0);
    const float* rightData = buffer.getReadPointer(1);

    for (int i = 0; i < numSamples; ++i) {
        double time_s = i / sampleRate;
        file << i << "," << std::fixed << std::setprecision(6) << time_s << ","
             << std::setprecision(8) << leftData[i] << "," << rightData[i] << "\n";
    }

    file.close();
    return true;
}

// Test a single engine
bool testEngineReo(const EngineMetadata& metadata) {
    try {
        // Create engine
        auto engine = EngineFactory::createEngine(metadata.id);
        if (!engine) {
            std::cout << "  ERROR: Failed to create engine\n";
            return false;
        }

        // Setup
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        const int numChannels = 2;
        const double testDuration = 2.0;  // 2 seconds
        const int totalSamples = static_cast<int>(sampleRate * testDuration);

        // Prepare engine using prepareToPlay
        engine->prepareToPlay(sampleRate, blockSize);

        // Set parameters
        auto params = getDefaultParams(metadata.id);
        engine->updateParameters(params);

        // Generate test signal
        juce::AudioBuffer<float> inputBuffer(numChannels, totalSamples);
        generateStereoTestSignal(inputBuffer, sampleRate);

        // Output buffer
        juce::AudioBuffer<float> outputBuffer(numChannels, totalSamples);
        outputBuffer.clear();

        // Process in blocks
        int numBlocks = (totalSamples + blockSize - 1) / blockSize;

        for (int blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
            int startSample = blockIdx * blockSize;
            int numSamplesThisBlock = std::min(blockSize, totalSamples - startSample);

            // Create block buffer
            juce::AudioBuffer<float> blockBuffer(numChannels, blockSize);

            // Copy input to block
            for (int ch = 0; ch < numChannels; ++ch) {
                blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, numSamplesThisBlock);
                if (numSamplesThisBlock < blockSize) {
                    blockBuffer.clear(ch, numSamplesThisBlock, blockSize - numSamplesThisBlock);
                }
            }

            // Process the block
            engine->process(blockBuffer);

            // Copy output
            for (int ch = 0; ch < numChannels; ++ch) {
                outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesThisBlock);
            }
        }

        // Save stereo data
        std::string filename = "stereo_engine_" + std::to_string(metadata.id) + ".csv";
        if (!saveStereoCSV(filename, outputBuffer, sampleRate)) {
            std::cout << "  ERROR: Failed to save CSV\n";
            return false;
        }

        // Calculate basic stereo metrics
        const float* leftData = outputBuffer.getReadPointer(0);
        const float* rightData = outputBuffer.getReadPointer(1);

        // Peak levels
        float peakL = 0.0f;
        float peakR = 0.0f;
        double sumL = 0.0;
        double sumR = 0.0;

        for (int i = 0; i < totalSamples; ++i) {
            peakL = std::max(peakL, std::abs(leftData[i]));
            peakR = std::max(peakR, std::abs(rightData[i]));
            sumL += leftData[i] * leftData[i];
            sumR += rightData[i] * rightData[i];
        }

        float rmsL = std::sqrt(sumL / totalSamples);
        float rmsR = std::sqrt(sumR / totalSamples);

        // Quick correlation estimate
        double correlation = 0.0;
        double meanL = 0.0, meanR = 0.0;
        for (int i = 0; i < totalSamples; ++i) {
            meanL += leftData[i];
            meanR += rightData[i];
        }
        meanL /= totalSamples;
        meanR /= totalSamples;

        double numerator = 0.0;
        double denomL = 0.0;
        double denomR = 0.0;
        for (int i = 0; i < totalSamples; ++i) {
            double diffL = leftData[i] - meanL;
            double diffR = rightData[i] - meanR;
            numerator += diffL * diffR;
            denomL += diffL * diffL;
            denomR += diffR * diffR;
        }

        if (denomL > 1e-10 && denomR > 1e-10) {
            correlation = numerator / std::sqrt(denomL * denomR);
        }

        std::cout << "  Peak: L=" << std::fixed << std::setprecision(3) << peakL
                  << " R=" << peakR
                  << "  RMS: L=" << rmsL << " R=" << rmsR
                  << "  Corr=" << correlation << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cout << "  EXCEPTION: " << e.what() << std::endl;
        return false;
    }
}

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "          COMPREHENSIVE STEREO DATA GENERATION - ALL 56 ENGINES\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    std::cout << "Generating stereo test data for stereo analysis suite...\n";
    std::cout << "\n";

    int successCount = 0;
    int failCount = 0;

    for (const auto& engine : ALL_ENGINES) {
        std::cout << "[" << std::setw(2) << engine.id << "] "
                  << std::setw(35) << std::left << engine.name
                  << " (" << engine.category << ")\n";

        if (testEngineReo(engine)) {
            successCount++;
        } else {
            failCount++;
        }
    }

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                              SUMMARY\n";
    std::cout << "================================================================================\n";
    std::cout << "  Total Engines:     " << ALL_ENGINES.size() << "\n";
    std::cout << "  Success:           " << successCount << "\n";
    std::cout << "  Failed:            " << failCount << "\n";
    std::cout << "\n";

    if (failCount == 0) {
        std::cout << "  All stereo data files generated successfully!\n";
        std::cout << "  Run stereo_analysis_suite.py to analyze stereo quality.\n";
    } else {
        std::cout << "  Some engines failed to generate data.\n";
    }

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "\n";

    return (failCount == 0) ? 0 : 1;
}

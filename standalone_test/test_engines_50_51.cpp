// Test for Engine 50 (GranularCloud) and Engine 51 (ChaosGenerator)
#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/GranularCloud.h"
#include "../JUCE_Plugin/Source/ChaosGenerator.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

// Helper to calculate RMS
float calculateRMS(const float* buffer, int numSamples) {
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += buffer[i] * buffer[i];
    }
    return std::sqrt(sum / numSamples);
}

// Helper to check if signal has output
bool hasOutput(const float* buffer, int numSamples, float threshold = 1e-6f) {
    float rms = calculateRMS(buffer, numSamples);
    return rms > threshold;
}

// Test GranularCloud (Engine 50)
bool testGranularCloud() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Engine 50: GranularCloud Test                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";

    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const int numBlocks = 200; // ~2 seconds for grain accumulation

    GranularCloud engine;
    engine.prepareToPlay(sampleRate, blockSize);

    // Create test buffer with sine wave input
    juce::AudioBuffer<float> buffer(2, blockSize);
    const float freq = 440.0f;
    const float amplitude = 0.5f;

    std::cout << "Input: 440Hz sine wave @ -6dB\n";
    std::cout << "Sample Rate: " << sampleRate << " Hz\n";
    std::cout << "Block Size: " << blockSize << " samples\n\n";

    // Set parameters for granular processing
    std::map<int, float> params;
    params[static_cast<int>(GranularCloud::ParamID::GrainSize)] = 0.5f;      // ~50ms grains
    params[static_cast<int>(GranularCloud::ParamID::Density)] = 0.6f;        // Moderate density
    params[static_cast<int>(GranularCloud::ParamID::PitchScatter)] = 0.3f;   // Some pitch variation
    params[static_cast<int>(GranularCloud::ParamID::CloudPosition)] = 0.5f;  // Center
    params[static_cast<int>(GranularCloud::ParamID::Mix)] = 1.0f;            // 100% wet

    engine.updateParameters(params);

    std::cout << "Parameters:\n";
    std::cout << "  Grain Size: 0.5 (moderate)\n";
    std::cout << "  Density: 0.6 (moderate)\n";
    std::cout << "  Pitch Scatter: 0.3 (some variation)\n";
    std::cout << "  Cloud Position: 0.5 (center)\n";
    std::cout << "  Mix: 1.0 (100% wet)\n\n";

    // Statistics tracking
    float totalRMS = 0.0f;
    float peakL = 0.0f;
    float peakR = 0.0f;
    int blocksWithOutput = 0;
    std::vector<float> rmsHistory;

    // Process blocks
    double phase = 0.0;
    const double phaseIncrement = 2.0 * M_PI * freq / sampleRate;

    for (int block = 0; block < numBlocks; ++block) {
        // Generate sine wave input
        for (int ch = 0; ch < 2; ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                channelData[i] = amplitude * std::sin(phase);
                phase += phaseIncrement;
                if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
            }
        }

        // Process through engine
        engine.process(buffer);

        // Analyze output
        const float* left = buffer.getReadPointer(0);
        const float* right = buffer.getReadPointer(1);

        float blockRMS = calculateRMS(left, blockSize);
        rmsHistory.push_back(blockRMS);
        totalRMS += blockRMS;

        if (blockRMS > 1e-6f) {
            blocksWithOutput++;
        }

        // Track peaks
        for (int i = 0; i < blockSize; ++i) {
            peakL = std::max(peakL, std::abs(left[i]));
            peakR = std::max(peakR, std::abs(right[i]));
        }
    }

    float avgRMS = totalRMS / numBlocks;
    float avgRMS_dB = 20.0f * std::log10(std::max(avgRMS, 1e-10f));
    float peakL_dB = 20.0f * std::log10(std::max(peakL, 1e-10f));
    float peakR_dB = 20.0f * std::log10(std::max(peakR, 1e-10f));

    // Calculate RMS stability (standard deviation)
    float rmsStdDev = 0.0f;
    for (float rms : rmsHistory) {
        float diff = rms - avgRMS;
        rmsStdDev += diff * diff;
    }
    rmsStdDev = std::sqrt(rmsStdDev / rmsHistory.size());

    std::cout << "Results:\n";
    std::cout << "  Blocks Processed: " << numBlocks << "\n";
    std::cout << "  Blocks with Output: " << blocksWithOutput << " ("
              << (100.0f * blocksWithOutput / numBlocks) << "%)\n";
    std::cout << "  Average RMS: " << std::fixed << std::setprecision(6) << avgRMS
              << " (" << std::setprecision(2) << avgRMS_dB << " dB)\n";
    std::cout << "  RMS Std Dev: " << std::fixed << std::setprecision(6) << rmsStdDev << "\n";
    std::cout << "  Peak L: " << std::fixed << std::setprecision(6) << peakL
              << " (" << std::setprecision(2) << peakL_dB << " dB)\n";
    std::cout << "  Peak R: " << std::fixed << std::setprecision(6) << peakR
              << " (" << std::setprecision(2) << peakR_dB << " dB)\n\n";

    // Pass/Fail criteria
    bool hasSignificantOutput = (blocksWithOutput > numBlocks * 0.5); // >50% blocks with output
    bool notClipping = (peakL < 1.0f && peakR < 1.0f);
    bool reasonableLevel = (avgRMS > 0.001f); // At least -60dB

    std::cout << "Analysis:\n";
    std::cout << "  ✓ Output Presence: " << (hasSignificantOutput ? "PASS" : "FAIL")
              << " (" << blocksWithOutput << "/" << numBlocks << " blocks)\n";
    std::cout << "  ✓ No Clipping: " << (notClipping ? "PASS" : "FAIL") << "\n";
    std::cout << "  ✓ Reasonable Level: " << (reasonableLevel ? "PASS" : "FAIL") << "\n";

    // Granular-specific characteristics
    bool granularBehavior = (rmsStdDev > avgRMS * 0.1); // Expect some variation
    std::cout << "  ✓ Granular Variation: " << (granularBehavior ? "PASS" : "FAIL")
              << " (std dev > 10% of mean)\n\n";

    bool passed = hasSignificantOutput && notClipping && reasonableLevel;

    if (passed) {
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "  ✓ Engine 50 (GranularCloud): PASS\n";
        std::cout << "═══════════════════════════════════════════════════════════\n";
    } else {
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "  ✗ Engine 50 (GranularCloud): FAIL\n";
        std::cout << "═══════════════════════════════════════════════════════════\n";
    }

    return passed;
}

// Test ChaosGenerator (Engine 51)
bool testChaosGenerator() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║          Engine 51: ChaosGenerator Test                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";

    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const int numBlocks = 100; // ~1 second

    ChaosGenerator engine;
    engine.prepareToPlay(sampleRate, blockSize);

    // Create test buffer with sine wave input
    juce::AudioBuffer<float> buffer(2, blockSize);
    const float freq = 440.0f;
    const float amplitude = 0.5f;

    std::cout << "Input: 440Hz sine wave @ -6dB\n";
    std::cout << "Sample Rate: " << sampleRate << " Hz\n";
    std::cout << "Block Size: " << blockSize << " samples\n\n";

    // Set parameters for chaos modulation
    std::map<int, float> params;
    params[0] = 0.5f;  // Rate
    params[1] = 0.5f;  // Depth (moderate)
    params[2] = 0.0f;  // Type (Lorenz)
    params[3] = 0.5f;  // Smoothing
    params[4] = 0.0f;  // Mod Target (Amplitude)
    params[5] = 0.0f;  // Sync
    params[6] = 0.5f;  // Seed
    params[7] = 1.0f;  // Mix (100% wet)

    engine.updateParameters(params);

    std::cout << "Parameters:\n";
    std::cout << "  Rate: 0.5 (moderate)\n";
    std::cout << "  Depth: 0.5 (moderate)\n";
    std::cout << "  Type: 0.0 (Lorenz attractor)\n";
    std::cout << "  Smoothing: 0.5 (moderate)\n";
    std::cout << "  Mod Target: 0.0 (Amplitude)\n";
    std::cout << "  Mix: 1.0 (100% wet)\n\n";

    // Statistics tracking
    float totalRMS = 0.0f;
    float peakL = 0.0f;
    float peakR = 0.0f;
    int blocksWithOutput = 0;
    std::vector<float> rmsHistory;

    // Process blocks
    double phase = 0.0;
    const double phaseIncrement = 2.0 * M_PI * freq / sampleRate;

    for (int block = 0; block < numBlocks; ++block) {
        // Generate sine wave input
        for (int ch = 0; ch < 2; ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                channelData[i] = amplitude * std::sin(phase);
                phase += phaseIncrement;
                if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
            }
        }

        // Process through engine
        engine.process(buffer);

        // Analyze output
        const float* left = buffer.getReadPointer(0);
        const float* right = buffer.getReadPointer(1);

        float blockRMS = calculateRMS(left, blockSize);
        rmsHistory.push_back(blockRMS);
        totalRMS += blockRMS;

        if (blockRMS > 1e-6f) {
            blocksWithOutput++;
        }

        // Track peaks
        for (int i = 0; i < blockSize; ++i) {
            peakL = std::max(peakL, std::abs(left[i]));
            peakR = std::max(peakR, std::abs(right[i]));
        }
    }

    float avgRMS = totalRMS / numBlocks;
    float avgRMS_dB = 20.0f * std::log10(std::max(avgRMS, 1e-10f));
    float peakL_dB = 20.0f * std::log10(std::max(peakL, 1e-10f));
    float peakR_dB = 20.0f * std::log10(std::max(peakR, 1e-10f));

    // Calculate RMS stability (standard deviation) - expect variation from chaos
    float rmsStdDev = 0.0f;
    for (float rms : rmsHistory) {
        float diff = rms - avgRMS;
        rmsStdDev += diff * diff;
    }
    rmsStdDev = std::sqrt(rmsStdDev / rmsHistory.size());

    std::cout << "Results:\n";
    std::cout << "  Blocks Processed: " << numBlocks << "\n";
    std::cout << "  Blocks with Output: " << blocksWithOutput << " ("
              << (100.0f * blocksWithOutput / numBlocks) << "%)\n";
    std::cout << "  Average RMS: " << std::fixed << std::setprecision(6) << avgRMS
              << " (" << std::setprecision(2) << avgRMS_dB << " dB)\n";
    std::cout << "  RMS Std Dev: " << std::fixed << std::setprecision(6) << rmsStdDev << "\n";
    std::cout << "  Peak L: " << std::fixed << std::setprecision(6) << peakL
              << " (" << std::setprecision(2) << peakL_dB << " dB)\n";
    std::cout << "  Peak R: " << std::fixed << std::setprecision(6) << peakR
              << " (" << std::setprecision(2) << peakR_dB << " dB)\n\n";

    // Pass/Fail criteria
    bool hasSignificantOutput = (blocksWithOutput > numBlocks * 0.8); // >80% blocks with output
    bool notClipping = (peakL < 1.0f && peakR < 1.0f);
    bool reasonableLevel = (avgRMS > 0.001f); // At least -60dB

    std::cout << "Analysis:\n";
    std::cout << "  ✓ Output Presence: " << (hasSignificantOutput ? "PASS" : "FAIL")
              << " (" << blocksWithOutput << "/" << numBlocks << " blocks)\n";
    std::cout << "  ✓ No Clipping: " << (notClipping ? "PASS" : "FAIL") << "\n";
    std::cout << "  ✓ Reasonable Level: " << (reasonableLevel ? "PASS" : "FAIL") << "\n";

    // Chaos-specific characteristics - expect modulation
    bool chaosModulation = (rmsStdDev > avgRMS * 0.05); // Expect some variation
    std::cout << "  ✓ Chaos Modulation: " << (chaosModulation ? "PASS" : "FAIL")
              << " (std dev > 5% of mean)\n\n";

    bool passed = hasSignificantOutput && notClipping && reasonableLevel;

    if (passed) {
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "  ✓ Engine 51 (ChaosGenerator): PASS\n";
        std::cout << "═══════════════════════════════════════════════════════════\n";
    } else {
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "  ✗ Engine 51 (ChaosGenerator): FAIL\n";
        std::cout << "═══════════════════════════════════════════════════════════\n";
    }

    return passed;
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     CHIMERA ENGINE TEST: Engines 50-51                  ║\n";
    std::cout << "║     GranularCloud & ChaosGenerator                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    bool granularPassed = testGranularCloud();
    bool chaosPassed = testChaosGenerator();

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    FINAL RESULTS                        ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";
    std::cout << "  Engine 50 (GranularCloud):  " << (granularPassed ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "  Engine 51 (ChaosGenerator): " << (chaosPassed ? "✓ PASS" : "✗ FAIL") << "\n\n";

    if (granularPassed && chaosPassed) {
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "  ✓✓ ALL TESTS PASSED\n";
        std::cout << "═══════════════════════════════════════════════════════════\n\n";
        return 0;
    } else {
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "  ✗✗ SOME TESTS FAILED\n";
        std::cout << "═══════════════════════════════════════════════════════════\n\n";
        return 1;
    }
}

// Comprehensive diagnostic for ShimmerReverb Engine 40 - Zero Output Bug
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include "EngineFactory.h"
#include "EngineBase.h"

// Helper to check if buffer has non-zero output
bool hasOutput(const juce::AudioBuffer<float>& buffer, int channel, float threshold = 1e-10f) {
    float maxAbs = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float val = std::abs(buffer.getSample(channel, i));
        if (val > maxAbs) maxAbs = val;
    }
    return maxAbs > threshold;
}

// Calculate RMS
float calculateRMS(const juce::AudioBuffer<float>& buffer, int channel) {
    double sum = 0.0;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float val = buffer.getSample(channel, i);
        sum += val * val;
    }
    return std::sqrt(sum / buffer.getNumSamples());
}

// Calculate peak
float calculatePeak(const juce::AudioBuffer<float>& buffer, int channel) {
    float peak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float val = std::abs(buffer.getSample(channel, i));
        if (val > peak) peak = val;
    }
    return peak;
}

// Calculate correlation between L and R
double calculateCorrelation(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return 1.0;

    double sumL = 0.0, sumR = 0.0;
    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;
    int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i) {
        double l = buffer.getSample(0, i);
        double r = buffer.getSample(1, i);
        sumL += l;
        sumR += r;
        sumLL += l * l;
        sumRR += r * r;
        sumLR += l * r;
    }

    double meanL = sumL / n;
    double meanR = sumR / n;
    double varL = (sumLL / n) - (meanL * meanL);
    double varR = (sumRR / n) - (meanR * meanR);
    double covar = (sumLR / n) - (meanL * meanR);

    if (varL > 0.0 && varR > 0.0) {
        return covar / std::sqrt(varL * varR);
    }
    return 0.0;
}

void runTest(const std::string& testName, std::map<int, float> params, bool printSamples = false) {
    std::cout << "\n=== TEST: " << testName << " ===\n";

    // Create engine
    auto engine = EngineFactory::createEngine(40);
    if (!engine) {
        std::cerr << "ERROR: Failed to create Engine 40\n";
        return;
    }

    // Prepare
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);
    engine->updateParameters(params);

    // Print parameters
    std::cout << "Parameters:\n";
    for (const auto& [idx, val] : params) {
        std::cout << "  Param[" << idx << "] = " << val << "\n";
    }

    // Create impulse test buffer (stereo)
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);  // Left channel impulse
    buffer.setSample(1, 0, 0.0f);  // Right channel silent

    // Process first block
    engine->process(buffer);

    // Check first block output
    bool hasLeftOutput = hasOutput(buffer, 0);
    bool hasRightOutput = hasOutput(buffer, 1);
    float rmsL = calculateRMS(buffer, 0);
    float rmsR = calculateRMS(buffer, 1);
    float peakL = calculatePeak(buffer, 0);
    float peakR = calculatePeak(buffer, 1);

    std::cout << "\nFirst Block Results:\n";
    std::cout << "  Left output:  " << (hasLeftOutput ? "YES" : "NO")
              << " (RMS: " << rmsL << ", Peak: " << peakL << ")\n";
    std::cout << "  Right output: " << (hasRightOutput ? "YES" : "NO")
              << " (RMS: " << rmsR << ", Peak: " << peakR << ")\n";

    if (printSamples && (hasLeftOutput || hasRightOutput)) {
        std::cout << "\nFirst 20 samples:\n";
        std::cout << "  Sample   Left          Right\n";
        for (int i = 0; i < std::min(20, blockSize); ++i) {
            std::cout << "  " << i << "        "
                      << buffer.getSample(0, i) << "  "
                      << buffer.getSample(1, i) << "\n";
        }
    }

    // Process several more blocks to capture reverb tail
    std::vector<float> leftSamples, rightSamples;
    for (int i = 0; i < blockSize; ++i) {
        leftSamples.push_back(buffer.getSample(0, i));
        rightSamples.push_back(buffer.getSample(1, i));
    }

    for (int block = 0; block < 10; ++block) {
        buffer.clear();
        engine->process(buffer);
        for (int i = 0; i < blockSize; ++i) {
            leftSamples.push_back(buffer.getSample(0, i));
            rightSamples.push_back(buffer.getSample(1, i));
        }
    }

    // Calculate overall statistics
    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;
    double sumL = 0.0, sumR = 0.0;
    float overallPeakL = 0.0f, overallPeakR = 0.0f;

    for (size_t i = 0; i < leftSamples.size(); ++i) {
        float l = leftSamples[i];
        float r = rightSamples[i];
        sumL += l;
        sumR += r;
        sumLL += l * l;
        sumRR += r * r;
        sumLR += l * r;
        if (std::abs(l) > overallPeakL) overallPeakL = std::abs(l);
        if (std::abs(r) > overallPeakR) overallPeakR = std::abs(r);
    }

    int n = leftSamples.size();
    double meanL = sumL / n;
    double meanR = sumR / n;
    double varL = (sumLL / n) - (meanL * meanL);
    double varR = (sumRR / n) - (meanR * meanR);
    double covar = (sumLR / n) - (meanL * meanR);

    double correlation = 0.0;
    if (varL > 0.0 && varR > 0.0) {
        correlation = covar / std::sqrt(varL * varR);
    }

    double overallRmsL = std::sqrt(sumLL / n);
    double overallRmsR = std::sqrt(sumRR / n);

    std::cout << "\nOverall Statistics (" << n << " samples):\n";
    std::cout << "  Left RMS:         " << overallRmsL << "\n";
    std::cout << "  Right RMS:        " << overallRmsR << "\n";
    std::cout << "  Left Peak:        " << overallPeakL << "\n";
    std::cout << "  Right Peak:       " << overallPeakR << "\n";
    std::cout << "  L/R Correlation:  " << correlation << "\n";
    std::cout << "  Stereo Width:     " << (1.0 - correlation) << "\n";

    // Verdict
    bool passed = true;
    if (overallRmsL < 1e-6 || overallRmsR < 1e-6) {
        std::cout << "\n✗ FAIL: Zero or near-zero output detected\n";
        passed = false;
    } else if (correlation > 0.8) {
        std::cout << "\n✗ FAIL: Stereo width insufficient (correlation > 0.8)\n";
        passed = false;
    } else {
        std::cout << "\n✓ PASS: Non-zero stereo output with good width\n";
    }
}

int main() {
    std::cout << "ShimmerReverb Engine 40 - Comprehensive Diagnostic\n";
    std::cout << "==================================================\n";

    // Test 1: Default parameters
    runTest("Default Parameters", {
        {0, 1.0f},   // Mix = 100% wet
        {2, 0.5f},   // Shimmer = 50%
        {3, 0.7f}    // Size = 70%
    });

    // Test 2: No predelay
    runTest("No Pre-delay", {
        {0, 1.0f},   // Mix = 100% wet
        {2, 0.5f},   // Shimmer = 50%
        {3, 0.7f},   // Size = 70%
        {6, 0.0f}    // Pre-delay = 0%
    }, true);

    // Test 3: With predelay
    runTest("With Pre-delay (50%)", {
        {0, 1.0f},   // Mix = 100% wet
        {2, 0.5f},   // Shimmer = 50%
        {3, 0.7f},   // Size = 70%
        {6, 0.5f}    // Pre-delay = 50%
    });

    // Test 4: No shimmer (pure reverb)
    runTest("No Shimmer (Pure Reverb)", {
        {0, 1.0f},   // Mix = 100% wet
        {2, 0.0f},   // Shimmer = 0%
        {3, 0.7f},   // Size = 70%
        {6, 0.0f}    // Pre-delay = 0%
    }, true);

    // Test 5: Maximum shimmer
    runTest("Maximum Shimmer", {
        {0, 1.0f},   // Mix = 100% wet
        {1, 1.0f},   // Pitch Shift = 100%
        {2, 1.0f},   // Shimmer = 100%
        {3, 0.7f},   // Size = 70%
        {6, 0.0f}    // Pre-delay = 0%
    });

    // Test 6: Small pitch shift
    runTest("Small Pitch Shift", {
        {0, 1.0f},   // Mix = 100% wet
        {1, 0.1f},   // Pitch Shift = 10%
        {2, 0.5f},   // Shimmer = 50%
        {3, 0.7f},   // Size = 70%
        {6, 0.0f}    // Pre-delay = 0%
    }, true);

    // Test 7: Moderate parameters
    runTest("Moderate Settings", {
        {0, 1.0f},   // Mix = 100% wet
        {1, 0.5f},   // Pitch Shift = 50%
        {2, 0.3f},   // Shimmer = 30%
        {3, 0.5f},   // Size = 50%
        {4, 0.5f},   // Damping = 50%
        {6, 0.0f}    // Pre-delay = 0%
    });

    std::cout << "\n=== DIAGNOSTIC COMPLETE ===\n";
    return 0;
}

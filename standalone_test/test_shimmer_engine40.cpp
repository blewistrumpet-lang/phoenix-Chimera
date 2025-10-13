// Simple test for ShimmerReverb Engine 40 impulse response
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include "EngineFactory.h"
#include "EngineBase.h"

int main() {
    std::cout << "Testing ShimmerReverb Engine 40 - Stereo Output Check\n";
    std::cout << "====================================================\n\n";

    // Create engine
    auto engine = EngineFactory::createEngine(40);
    if (!engine) {
        std::cerr << "ERROR: Failed to create Engine 40\n";
        return 1;
    }

    std::cout << "Engine name: " << engine->getName().toStdString() << "\n";

    // Prepare
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set parameters for testing
    std::map<int, float> params;
    params[0] = 1.0f;   // Mix = 100% wet
    params[2] = 0.5f;   // Shimmer = 50%
    params[3] = 0.7f;   // Size = 70%
    params[6] = 0.5f;   // Pre-delay = 50% (this is what we're testing)
    engine->updateParameters(params);

    // Create impulse test buffer (stereo)
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();

    // Send impulse
    buffer.setSample(0, 0, 1.0f);  // Left channel impulse
    buffer.setSample(1, 0, 0.0f);  // Right channel silent

    // Process
    engine->process(buffer);

    // Capture first 2000 samples of impulse response
    std::vector<float> leftSamples;
    std::vector<float> rightSamples;

    int totalSamples = 0;
    const int targetSamples = 2000;

    // First block already processed
    for (int i = 0; i < blockSize && totalSamples < targetSamples; ++i) {
        leftSamples.push_back(buffer.getSample(0, i));
        rightSamples.push_back(buffer.getSample(1, i));
        totalSamples++;
    }

    // Process more blocks
    while (totalSamples < targetSamples) {
        buffer.clear();
        engine->process(buffer);

        for (int i = 0; i < blockSize && totalSamples < targetSamples; ++i) {
            leftSamples.push_back(buffer.getSample(0, i));
            rightSamples.push_back(buffer.getSample(1, i));
            totalSamples++;
        }
    }

    // Calculate correlation
    double sumL = 0.0, sumR = 0.0;
    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;

    for (size_t i = 0; i < leftSamples.size(); ++i) {
        double l = leftSamples[i];
        double r = rightSamples[i];
        sumL += l;
        sumR += r;
        sumLL += l * l;
        sumRR += r * r;
        sumLR += l * r;
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

    // Calculate RMS for each channel
    double rmsL = std::sqrt(sumLL / n);
    double rmsR = std::sqrt(sumRR / n);

    // Find peak values
    float peakL = 0.0f, peakR = 0.0f;
    for (size_t i = 0; i < leftSamples.size(); ++i) {
        float absL = std::abs(leftSamples[i]);
        float absR = std::abs(rightSamples[i]);
        if (absL > peakL) peakL = absL;
        if (absR > peakR) peakR = absR;
    }

    // Report results
    std::cout << "\nImpulse Response Analysis:\n";
    std::cout << "  Samples analyzed: " << n << "\n";
    std::cout << "  Left RMS:         " << rmsL << "\n";
    std::cout << "  Right RMS:        " << rmsR << "\n";
    std::cout << "  Left Peak:        " << peakL << "\n";
    std::cout << "  Right Peak:       " << peakR << "\n";
    std::cout << "  L/R Correlation:  " << correlation << "\n\n";

    // Check stereo output
    bool passed = true;

    // Both channels should have significant output
    if (rmsL < 0.001 || rmsR < 0.001) {
        std::cout << "✗ FAIL: One or both channels have insufficient output\n";
        passed = false;
    }

    // Correlation should be less than 0.8 for good stereo
    if (correlation >= 0.8) {
        std::cout << "✗ FAIL: L/R correlation too high (mono output): " << correlation << "\n";
        passed = false;
    }

    // Both channels should have similar energy levels
    double energyRatio = (rmsL > rmsR) ? (rmsL / rmsR) : (rmsR / rmsL);
    if (energyRatio > 5.0) {
        std::cout << "✗ FAIL: Channel energy imbalance too high: " << energyRatio << "\n";
        passed = false;
    }

    if (passed) {
        std::cout << "✓ PASS: ShimmerReverb produces proper stereo output\n";
        std::cout << "  Stereo width: " << (1.0 - correlation) << "\n";
        return 0;
    } else {
        std::cout << "\n✗ FAIL: ShimmerReverb stereo output test failed\n";
        return 1;
    }
}

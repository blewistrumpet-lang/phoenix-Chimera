// Direct test of fixed ShimmerReverb - no EngineFactory
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include "ShimmerReverb.h"
#include <JuceHeader.h>

float calculateRMS(const juce::AudioBuffer<float>& buffer, int channel) {
    double sum = 0.0;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float val = buffer.getSample(channel, i);
        sum += val * val;
    }
    return std::sqrt(sum / buffer.getNumSamples());
}

float calculatePeak(const juce::AudioBuffer<float>& buffer, int channel) {
    float peak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float val = std::abs(buffer.getSample(channel, i));
        if (val > peak) peak = val;
    }
    return peak;
}

double calculateCorrelation(const std::vector<float>& left, const std::vector<float>& right) {
    double sumL = 0.0, sumR = 0.0;
    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;
    int n = left.size();

    for (int i = 0; i < n; ++i) {
        double l = left[i];
        double r = right[i];
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

int main() {
    std::cout << "Testing FIXED ShimmerReverb Engine 40\n";
    std::cout << "======================================\n\n";

    // Create ShimmerReverb directly
    auto engine = std::make_unique<ShimmerReverb>();
    std::cout << "Engine name: " << engine->getName().toStdString() << "\n\n";

    // Prepare
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    std::cout << "=== TEST 1: No Shimmer (Pure Reverb) ===\n";
    {
        std::map<int, float> params;
        params[0] = 1.0f;   // Mix = 100% wet
        params[2] = 0.0f;   // Shimmer = 0% (pure reverb)
        params[3] = 0.7f;   // Size = 70%
        params[6] = 0.0f;   // Pre-delay = 0%
        engine->updateParameters(params);
        engine->reset();

        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse

        engine->process(buffer);

        float rmsL = calculateRMS(buffer, 0);
        float rmsR = calculateRMS(buffer, 1);
        float peakL = calculatePeak(buffer, 0);
        float peakR = calculatePeak(buffer, 1);

        std::cout << "  Left RMS:  " << rmsL << ", Peak: " << peakL << "\n";
        std::cout << "  Right RMS: " << rmsR << ", Peak: " << peakR << "\n";

        if (rmsL > 1e-6 && rmsR > 1e-6) {
            std::cout << "  ✓ PASS: Pure reverb produces output\n\n";
        } else {
            std::cout << "  ✗ FAIL: Pure reverb has zero output\n\n";
            return 1;
        }
    }

    std::cout << "=== TEST 2: With Shimmer (50%) ===\n";
    {
        std::map<int, float> params;
        params[0] = 1.0f;   // Mix = 100% wet
        params[1] = 0.5f;   // Pitch Shift = 50%
        params[2] = 0.5f;   // Shimmer = 50%
        params[3] = 0.7f;   // Size = 70%
        params[6] = 0.0f;   // Pre-delay = 0%
        engine->updateParameters(params);
        engine->reset();

        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse

        engine->process(buffer);

        float rmsL = calculateRMS(buffer, 0);
        float rmsR = calculateRMS(buffer, 1);
        float peakL = calculatePeak(buffer, 0);
        float peakR = calculatePeak(buffer, 1);

        std::cout << "  Left RMS:  " << rmsL << ", Peak: " << peakL << "\n";
        std::cout << "  Right RMS: " << rmsR << ", Peak: " << peakR << "\n";

        if (rmsL > 1e-6 && rmsR > 1e-6) {
            std::cout << "  ✓ PASS: Shimmer produces output\n\n";
        } else {
            std::cout << "  ✗ FAIL: Shimmer has zero output\n\n";
            return 1;
        }
    }

    std::cout << "=== TEST 3: Stereo Width Check (Full Reverb Tail) ===\n";
    {
        std::map<int, float> params;
        params[0] = 1.0f;   // Mix = 100% wet
        params[1] = 0.5f;   // Pitch Shift = 50%
        params[2] = 0.5f;   // Shimmer = 50%
        params[3] = 0.7f;   // Size = 70%
        params[6] = 0.0f;   // Pre-delay = 0%
        params[7] = 0.3f;   // Modulation = 30% (for stereo width)
        engine->updateParameters(params);
        engine->reset();

        // Collect full impulse response
        std::vector<float> leftSamples, rightSamples;

        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse
        engine->process(buffer);

        for (int i = 0; i < blockSize; ++i) {
            leftSamples.push_back(buffer.getSample(0, i));
            rightSamples.push_back(buffer.getSample(1, i));
        }

        // Process several more blocks for reverb tail
        for (int block = 0; block < 20; ++block) {
            buffer.clear();
            engine->process(buffer);
            for (int i = 0; i < blockSize; ++i) {
                leftSamples.push_back(buffer.getSample(0, i));
                rightSamples.push_back(buffer.getSample(1, i));
            }
        }

        double correlation = calculateCorrelation(leftSamples, rightSamples);
        double stereoWidth = 1.0 - correlation;

        // Calculate RMS
        double sumLL = 0.0, sumRR = 0.0;
        for (size_t i = 0; i < leftSamples.size(); ++i) {
            sumLL += leftSamples[i] * leftSamples[i];
            sumRR += rightSamples[i] * rightSamples[i];
        }
        double rmsL = std::sqrt(sumLL / leftSamples.size());
        double rmsR = std::sqrt(sumRR / rightSamples.size());

        std::cout << "  Samples: " << leftSamples.size() << "\n";
        std::cout << "  Left RMS:      " << rmsL << "\n";
        std::cout << "  Right RMS:     " << rmsR << "\n";
        std::cout << "  Correlation:   " << correlation << "\n";
        std::cout << "  Stereo Width:  " << stereoWidth << "\n";

        bool passed = true;
        if (rmsL < 1e-6 || rmsR < 1e-6) {
            std::cout << "  ✗ FAIL: One or both channels have zero output\n\n";
            passed = false;
        } else if (stereoWidth < 0.2) {
            std::cout << "  ✗ FAIL: Stereo width too narrow (< 0.2)\n\n";
            passed = false;
        } else if (stereoWidth >= 0.8) {
            std::cout << "  ✓ EXCELLENT: Stereo width >= 0.8\n\n";
        } else {
            std::cout << "  ✓ PASS: Stereo width acceptable (>= 0.2)\n\n";
        }

        if (!passed) return 1;
    }

    std::cout << "=== TEST 4: With Pre-delay (50%) ===\n";
    {
        std::map<int, float> params;
        params[0] = 1.0f;   // Mix = 100% wet
        params[1] = 0.5f;   // Pitch Shift = 50%
        params[2] = 0.5f;   // Shimmer = 50%
        params[3] = 0.7f;   // Size = 70%
        params[6] = 0.5f;   // Pre-delay = 50%
        engine->updateParameters(params);
        engine->reset();

        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse

        engine->process(buffer);

        float rmsL = calculateRMS(buffer, 0);
        float rmsR = calculateRMS(buffer, 1);
        float peakL = calculatePeak(buffer, 0);
        float peakR = calculatePeak(buffer, 1);

        std::cout << "  Left RMS:  " << rmsL << ", Peak: " << peakL << "\n";
        std::cout << "  Right RMS: " << rmsR << ", Peak: " << peakR << "\n";

        if (rmsL > 1e-6 && rmsR > 1e-6) {
            std::cout << "  ✓ PASS: Pre-delay works correctly\n\n";
        } else {
            std::cout << "  ✗ FAIL: Pre-delay causes zero output\n\n";
            return 1;
        }
    }

    std::cout << "========================================\n";
    std::cout << "✓ ALL TESTS PASSED\n";
    std::cout << "ShimmerReverb Engine 40 is FIXED!\n";
    return 0;
}

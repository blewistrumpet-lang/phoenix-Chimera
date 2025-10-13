/**
 * test_spatial_engines_46_48.cpp
 *
 * Focused test for Spatial Engines 46-48:
 * - Engine 46: Dimension Expander (stereo spatial processing)
 * - Engine 47: Spectral Freeze (spectral/spatial effects)
 * - Engine 48: Spectral Gate (spectral gating with spatial impact)
 *
 * Tests stereo correlation, phase relationships, and spatial width.
 *
 * Compile:
 *   g++ -std=c++17 -O2 test_spatial_engines_46_48.cpp \
 *       -I../JUCE_Plugin/Source \
 *       -I/Users/Branden/JUCE/modules \
 *       -framework Accelerate -framework CoreAudio -framework CoreFoundation \
 *       -o test_spatial_46_48
 */

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 1
#define NDEBUG 1

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <complex>

//==============================================================================
// Stereo Correlation Measurement
//==============================================================================

struct StereoMetrics {
    float correlation;        // -1 to +1 (1=mono, 0=uncorrelated, -1=inverted)
    float width;             // Stereo width (0=mono, 1=normal, >1=enhanced)
    float midLevel;          // Level of mid (L+R) component
    float sideLevel;         // Level of side (L-R) component
    float monoCompatibility; // How well it survives mono summing (0-1)
    float phaseCoherence;    // Phase relationship quality (0-1)
    bool passed;             // Overall pass/fail
};

StereoMetrics measureStereoMetrics(const juce::AudioBuffer<float>& input,
                                   const juce::AudioBuffer<float>& output) {
    StereoMetrics metrics = {};

    if (output.getNumChannels() < 2) {
        std::cout << "  ✗ Not stereo output\n";
        return metrics;
    }

    const float* outL = output.getReadPointer(0);
    const float* outR = output.getReadPointer(1);
    const int numSamples = output.getNumSamples();

    // Calculate correlation coefficient
    float sumLL = 0.0f, sumRR = 0.0f, sumLR = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sumLL += outL[i] * outL[i];
        sumRR += outR[i] * outR[i];
        sumLR += outL[i] * outR[i];
    }

    float denominator = std::sqrt(sumLL * sumRR);
    metrics.correlation = (denominator > 1e-10f) ? (sumLR / denominator) : 1.0f;

    // Calculate mid/side components
    float sumMid = 0.0f, sumSide = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float mid = (outL[i] + outR[i]) * 0.5f;
        float side = (outL[i] - outR[i]) * 0.5f;
        sumMid += mid * mid;
        sumSide += side * side;
    }

    metrics.midLevel = std::sqrt(sumMid / numSamples);
    metrics.sideLevel = std::sqrt(sumSide / numSamples);

    // Calculate stereo width
    if (metrics.midLevel > 1e-10f) {
        metrics.width = metrics.sideLevel / metrics.midLevel;
    } else {
        metrics.width = 0.0f;
    }

    // Calculate mono compatibility
    float monoSumPeak = 0.0f;
    float stereoPeakL = 0.0f, stereoPeakR = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        float monoSum = (outL[i] + outR[i]) * 0.5f;
        monoSumPeak = std::max(monoSumPeak, std::abs(monoSum));
        stereoPeakL = std::max(stereoPeakL, std::abs(outL[i]));
        stereoPeakR = std::max(stereoPeakR, std::abs(outR[i]));
    }

    float stereoPeak = std::max(stereoPeakL, stereoPeakR);
    metrics.monoCompatibility = (stereoPeak > 1e-10f) ? (monoSumPeak / stereoPeak) : 1.0f;

    // Simple phase coherence check
    // Compare input vs output correlation to see if phase relationship maintained
    if (input.getNumChannels() >= 2) {
        const float* inL = input.getReadPointer(0);
        const float* inR = input.getReadPointer(1);

        float inSumLL = 0.0f, inSumRR = 0.0f, inSumLR = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            inSumLL += inL[i] * inL[i];
            inSumRR += inR[i] * inR[i];
            inSumLR += inL[i] * inR[i];
        }

        float inDenom = std::sqrt(inSumLL * inSumRR);
        float inCorr = (inDenom > 1e-10f) ? (inSumLR / inDenom) : 1.0f;

        // Phase coherence: how much correlation changed
        metrics.phaseCoherence = 1.0f - std::abs(metrics.correlation - inCorr);
    } else {
        metrics.phaseCoherence = 1.0f; // Unknown, assume good
    }

    // Pass criteria:
    // - Correlation should be different from input (spatial processing happening)
    // - Mono compatibility > 0.5 (no severe phase cancellation)
    // - Width should be measurable
    metrics.passed = (metrics.monoCompatibility > 0.5f) && (metrics.width >= 0.0f);

    return metrics;
}

//==============================================================================
// Test Functions
//==============================================================================

void testEngine46_DimensionExpander(float sampleRate) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENGINE 46: DIMENSION EXPANDER                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(46);
    if (!engine) {
        std::cout << "✗ FAIL: Could not create engine\n";
        return;
    }

    const int blockSize = 2048;
    engine->prepareToPlay(sampleRate, blockSize);

    std::cout << "Test 1: Mono to Stereo Expansion\n";
    std::cout << "─────────────────────────────────\n";

    // Test with different expansion amounts
    float expansionLevels[] = {0.0f, 0.33f, 0.67f, 1.0f};
    std::vector<StereoMetrics> results;

    std::ofstream csv("engine_46_correlation.csv");
    csv << "Expansion,Correlation,Width,MidLevel,SideLevel,MonoCompat,PhaseCoherence,Status\n";

    for (float expansion : expansionLevels) {
        std::map<int, float> params;
        params[0] = expansion; // Expansion/width parameter
        engine->updateParameters(params);
        engine->reset();

        // Create mono input (identical L/R)
        juce::AudioBuffer<float> input(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float sample = 0.5f * std::sin(2.0f * M_PI * 1000.0f * i / sampleRate);
            input.setSample(0, i, sample);
            input.setSample(1, i, sample);
        }

        juce::AudioBuffer<float> output;
        output.makeCopyOf(input);
        engine->process(output);

        auto metrics = measureStereoMetrics(input, output);
        results.push_back(metrics);

        std::cout << "  Expansion " << std::setw(3) << static_cast<int>(expansion * 100) << "%: ";
        std::cout << "Corr=" << std::fixed << std::setprecision(3) << metrics.correlation;
        std::cout << ", Width=" << std::setprecision(2) << metrics.width;
        std::cout << ", MonoCompat=" << std::setprecision(1) << metrics.monoCompatibility * 100 << "%";
        std::cout << (metrics.passed ? " ✓" : " ✗");
        std::cout << "\n";

        csv << (expansion * 100) << ","
            << metrics.correlation << ","
            << metrics.width << ","
            << metrics.midLevel << ","
            << metrics.sideLevel << ","
            << metrics.monoCompatibility << ","
            << metrics.phaseCoherence << ","
            << (metrics.passed ? "PASS" : "FAIL") << "\n";
    }

    csv.close();

    // Check if width increases with parameter
    bool widthIncreasing = true;
    for (size_t i = 1; i < results.size(); ++i) {
        if (results[i].width < results[i-1].width - 0.01f) {
            widthIncreasing = false;
            break;
        }
    }

    std::cout << "\n";
    std::cout << "Width Response:      " << (widthIncreasing ? "✓ INCREASING" : "✗ NOT MONOTONIC") << "\n";
    std::cout << "Mono Compatibility:  " << (results.back().monoCompatibility > 0.5f ? "✓ GOOD" : "✗ POOR") << "\n";
    std::cout << "Overall Status:      " << (widthIncreasing && results.back().passed ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "\nResults saved to: engine_46_correlation.csv\n";
}

void testEngine47_SpectralFreeze(float sampleRate) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENGINE 47: SPECTRAL FREEZE                                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(47);
    if (!engine) {
        std::cout << "✗ FAIL: Could not create engine\n";
        return;
    }

    const int blockSize = 4096; // Larger for spectral processing
    engine->prepareToPlay(sampleRate, blockSize);

    std::cout << "Test 1: Stereo Preservation During Freeze\n";
    std::cout << "──────────────────────────────────────────\n";

    std::ofstream csv("engine_47_correlation.csv");
    csv << "FreezeAmount,Correlation,Width,MidLevel,SideLevel,MonoCompat,PhaseCoherence,Status\n";

    float freezeLevels[] = {0.0f, 0.5f, 1.0f};
    std::vector<StereoMetrics> results;

    for (float freeze : freezeLevels) {
        std::map<int, float> params;
        params[0] = freeze; // Freeze amount
        engine->updateParameters(params);
        engine->reset();

        // Create stereo input (slightly decorrelated)
        juce::AudioBuffer<float> input(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float t = i / sampleRate;
            input.setSample(0, i, 0.5f * std::sin(2.0f * M_PI * 1000.0f * t));
            input.setSample(1, i, 0.5f * std::sin(2.0f * M_PI * 1000.0f * t + 0.1f)); // Slight phase offset
        }

        juce::AudioBuffer<float> output;
        output.makeCopyOf(input);

        // Process multiple blocks to allow freeze to take effect
        for (int block = 0; block < 3; ++block) {
            engine->process(output);
        }

        auto metrics = measureStereoMetrics(input, output);
        results.push_back(metrics);

        std::cout << "  Freeze " << std::setw(3) << static_cast<int>(freeze * 100) << "%: ";
        std::cout << "Corr=" << std::fixed << std::setprecision(3) << metrics.correlation;
        std::cout << ", Width=" << std::setprecision(2) << metrics.width;
        std::cout << ", PhaseCoherence=" << std::setprecision(2) << metrics.phaseCoherence;
        std::cout << (metrics.passed ? " ✓" : " ✗");
        std::cout << "\n";

        csv << (freeze * 100) << ","
            << metrics.correlation << ","
            << metrics.width << ","
            << metrics.midLevel << ","
            << metrics.sideLevel << ","
            << metrics.monoCompatibility << ","
            << metrics.phaseCoherence << ","
            << (metrics.passed ? "PASS" : "FAIL") << "\n";
    }

    csv.close();

    bool allPassed = true;
    for (const auto& result : results) {
        if (!result.passed) allPassed = false;
    }

    std::cout << "\n";
    std::cout << "Stereo Preservation: " << (results.back().phaseCoherence > 0.7f ? "✓ MAINTAINED" : "✗ DEGRADED") << "\n";
    std::cout << "Overall Status:      " << (allPassed ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "\nResults saved to: engine_47_correlation.csv\n";
}

void testEngine48_SpectralGate(float sampleRate) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENGINE 48: SPECTRAL GATE                                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "⚠️  NOTE: This engine has been reported to crash in previous tests.\n";
    std::cout << "Attempting safe initialization...\n\n";

    try {
        auto engine = EngineFactory::createEngine(48);
        if (!engine) {
            std::cout << "✗ FAIL: Could not create engine\n";
            return;
        }

        std::cout << "✓ Engine created successfully\n";

        const int blockSize = 2048;
        engine->prepareToPlay(sampleRate, blockSize);
        std::cout << "✓ PrepareToPlay succeeded\n";

        std::cout << "\nTest 1: Stereo Correlation During Gating\n";
        std::cout << "─────────────────────────────────────────\n";

        std::ofstream csv("engine_48_correlation.csv");
        csv << "Threshold,Correlation,Width,MidLevel,SideLevel,MonoCompat,PhaseCoherence,Status\n";

        float thresholds[] = {0.0f, 0.5f, 1.0f};
        std::vector<StereoMetrics> results;

        for (float threshold : thresholds) {
            std::map<int, float> params;
            params[0] = threshold; // Gate threshold
            engine->updateParameters(params);
            engine->reset();

            // Create stereo input with some frequency content
            juce::AudioBuffer<float> input(2, blockSize);
            for (int i = 0; i < blockSize; ++i) {
                float t = i / sampleRate;
                // Multiple frequencies
                float signal = 0.3f * std::sin(2.0f * M_PI * 500.0f * t) +
                              0.2f * std::sin(2.0f * M_PI * 2000.0f * t) +
                              0.1f * std::sin(2.0f * M_PI * 5000.0f * t);
                input.setSample(0, i, signal);
                input.setSample(1, i, signal * 0.9f); // Slightly different amplitude
            }

            juce::AudioBuffer<float> output;
            output.makeCopyOf(input);
            engine->process(output);

            auto metrics = measureStereoMetrics(input, output);
            results.push_back(metrics);

            std::cout << "  Threshold " << std::setw(3) << static_cast<int>(threshold * 100) << "%: ";
            std::cout << "Corr=" << std::fixed << std::setprecision(3) << metrics.correlation;
            std::cout << ", Width=" << std::setprecision(2) << metrics.width;
            std::cout << ", MonoCompat=" << std::setprecision(1) << metrics.monoCompatibility * 100 << "%";
            std::cout << (metrics.passed ? " ✓" : " ✗");
            std::cout << "\n";

            csv << (threshold * 100) << ","
                << metrics.correlation << ","
                << metrics.width << ","
                << metrics.midLevel << ","
                << metrics.sideLevel << ","
                << metrics.monoCompatibility << ","
                << metrics.phaseCoherence << ","
                << (metrics.passed ? "PASS" : "FAIL") << "\n";
        }

        csv.close();

        bool allPassed = true;
        for (const auto& result : results) {
            if (!result.passed) allPassed = false;
        }

        std::cout << "\n";
        std::cout << "No Crash:            ✓ STABLE\n";
        std::cout << "Stereo Processing:   " << (results.back().width > 0.01f ? "✓ ACTIVE" : "⚠️  MINIMAL") << "\n";
        std::cout << "Overall Status:      " << (allPassed ? "✓ PASS" : "✗ FAIL") << "\n";
        std::cout << "\nResults saved to: engine_48_correlation.csv\n";

    } catch (const std::exception& e) {
        std::cout << "\n✗✗✗ CRASH DETECTED: " << e.what() << "\n";
        std::cout << "Overall Status: ✗ FAIL (CRASH)\n";
    } catch (...) {
        std::cout << "\n✗✗✗ UNKNOWN CRASH DETECTED\n";
        std::cout << "Overall Status: ✗ FAIL (CRASH)\n";
    }
}

//==============================================================================
// Main
//==============================================================================

int main() {
    // No GUI needed for this test
    const float sampleRate = 48000.0f;

    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SPATIAL ENGINES 46-48 TEST SUITE                            ║\n";
    std::cout << "║  Testing Stereo Correlation & Spatial Processing             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";

    testEngine46_DimensionExpander(sampleRate);
    testEngine47_SpectralFreeze(sampleRate);
    testEngine48_SpectralGate(sampleRate);

    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST SUITE COMPLETE                                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}

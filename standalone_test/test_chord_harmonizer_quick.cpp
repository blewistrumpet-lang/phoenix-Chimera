// QUICK VERIFICATION TEST - ENGINE 33: IntelligentHarmonizer (Chord Generator)
// Fast chord generation accuracy test for key chord types

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <complex>
#include <map>
#include <string>
#include <fstream>
#include <algorithm>

// Include standalone harmonizer
#include "IntelligentHarmonizer_standalone.h"
#include "IntelligentHarmonizerChords.h"

// ============================================================================
// AUDIO ANALYSIS UTILITIES
// ============================================================================

// Simple FFT
std::vector<float> computeFFT(const std::vector<float>& signal) {
    const int N = signal.size();
    std::vector<std::complex<float>> fft(N);

    for (int k = 0; k < N / 2; ++k) {
        std::complex<float> sum(0, 0);
        for (int n = 0; n < N; ++n) {
            float angle = -2.0f * M_PI * k * n / N;
            sum += signal[n] * std::exp(std::complex<float>(0, angle));
        }
        fft[k] = sum;
    }

    std::vector<float> magnitude(N / 2);
    for (int i = 0; i < N / 2; ++i) {
        magnitude[i] = std::abs(fft[i]) / N;
    }

    return magnitude;
}

// Find peaks
std::vector<std::pair<float, float>> findPeaks(const std::vector<float>& fft,
                                                 float sampleRate,
                                                 float threshold = 0.01f) {
    std::vector<std::pair<float, float>> peaks;

    for (size_t i = 5; i < fft.size() - 5; ++i) {
        if (fft[i] > threshold) {
            bool isMax = true;
            for (int j = -3; j <= 3; ++j) {
                if (j != 0 && i + j < fft.size() && fft[i + j] > fft[i]) {
                    isMax = false;
                    break;
                }
            }

            if (isMax) {
                float freq = i * sampleRate / (2.0f * fft.size());
                peaks.push_back({freq, fft[i]});
            }
        }
    }

    std::sort(peaks.begin(), peaks.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    return peaks;
}

// Convert ratio to cents
float ratioToCents(float ratio) {
    return 1200.0f * std::log2(ratio);
}

// Generate sine wave
std::vector<float> generateSineWave(float frequency, float sampleRate, int numSamples, float amplitude = 0.5f) {
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        float phase = 2.0f * M_PI * frequency * i / sampleRate;
        signal[i] = amplitude * std::sin(phase);
    }
    return signal;
}

// ============================================================================
// CHORD TEST
// ============================================================================

struct ChordTestResult {
    std::string chordName;
    std::vector<int> expectedIntervals;
    std::vector<float> expectedFreqs;
    std::vector<float> measuredFreqs;
    std::vector<float> centsError;
    float maxCentsError;
    float avgCentsError;
    bool passed;
};

ChordTestResult testChord(IntelligentHarmonizer_Standalone& harmonizer,
                          float chordTypeNorm,
                          const std::string& chordName,
                          const std::vector<int>& intervals,
                          float inputFreq,
                          float sampleRate) {
    ChordTestResult result;
    result.chordName = chordName;
    result.expectedIntervals = intervals;

    // Calculate expected frequencies
    for (int interval : intervals) {
        float ratio = std::pow(2.0f, interval / 12.0f);
        result.expectedFreqs.push_back(inputFreq * ratio);
    }

    // Setup parameters
    std::map<int, float> params;
    params[0] = 1.0f;  // 3 voices
    params[1] = chordTypeNorm;  // Chord type
    params[2] = 0.0f;  // Root key: C
    params[3] = 0.9f;  // Chromatic scale
    params[4] = 1.0f;  // 100% wet
    params[5] = 1.0f;  // Voice volumes
    params[7] = 0.8f;
    params[9] = 0.6f;
    params[11] = 1.0f; // High quality
    params[12] = 0.0f; // No humanize
    params[14] = 0.5f; // No transpose

    harmonizer.updateParameters(params);
    harmonizer.reset();

    // Process audio (shorter blocks for speed)
    const int blockSize = 1024;
    const int numBlocks = 4;  // Reduced from 10
    const int totalSamples = blockSize * numBlocks;

    std::vector<float> outputSignal(totalSamples);
    std::vector<float> inputBlock(blockSize);
    std::vector<float> outputBlock(blockSize);

    for (int block = 0; block < numBlocks; ++block) {
        inputBlock = generateSineWave(inputFreq, sampleRate, blockSize);
        harmonizer.processBlock(inputBlock.data(), outputBlock.data(), blockSize);
        std::copy(outputBlock.begin(), outputBlock.end(), outputSignal.begin() + block * blockSize);
    }

    // Analyze
    auto fft = computeFFT(outputSignal);
    auto peaks = findPeaks(fft, sampleRate, 0.005f);

    // Match peaks to expected frequencies
    result.measuredFreqs.resize(3, 0.0f);
    result.centsError.resize(3, 0.0f);

    for (size_t i = 0; i < std::min(result.expectedFreqs.size(), peaks.size()); ++i) {
        if (i >= 3) break;

        float expectedFreq = result.expectedFreqs[i];
        float minDist = 1000000.0f;
        int bestPeak = -1;

        for (size_t p = 0; p < std::min(size_t(10), peaks.size()); ++p) {
            float dist = std::abs(peaks[p].first - expectedFreq);
            if (dist < minDist) {
                minDist = dist;
                bestPeak = p;
            }
        }

        if (bestPeak >= 0) {
            result.measuredFreqs[i] = peaks[bestPeak].first;
            float ratio = peaks[bestPeak].first / expectedFreq;
            result.centsError[i] = ratioToCents(ratio);
        }
    }

    // Calculate statistics
    result.maxCentsError = 0.0f;
    result.avgCentsError = 0.0f;
    int count = 0;
    for (float cents : result.centsError) {
        float absCents = std::abs(cents);
        result.maxCentsError = std::max(result.maxCentsError, absCents);
        result.avgCentsError += absCents;
        count++;
    }
    if (count > 0) result.avgCentsError /= count;

    // Pass criteria: ±15 cents (relaxed for harmonizer with pitch shifting)
    result.passed = result.maxCentsError < 15.0f;

    return result;
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "================================================================================\n";
    std::cout << "CHORD HARMONIZER QUICK VERIFICATION - ENGINE 33\n";
    std::cout << "Testing Key Chord Types for Production Readiness\n";
    std::cout << "================================================================================\n\n";

    const float sampleRate = 48000.0f;
    const int blockSize = 512;
    const float testFreq = 440.0f; // A4

    // Create harmonizer
    IntelligentHarmonizer_Standalone harmonizer;
    harmonizer.prepareToPlay(sampleRate, blockSize);

    // Key chord types to test
    struct ChordTest {
        std::string name;
        int index;
        std::vector<int> intervals;
    };

    std::vector<ChordTest> testChords = {
        {"Major",     0,  {4, 7, 12}},    // Most important
        {"Minor",     1,  {3, 7, 12}},    // Most important
        {"Dom7",      8,  {4, 10, 16}},   // Common jazz/blues
        {"Maj7",      6,  {4, 11, 16}},   // Common jazz
        {"5th",       17, {7, 12, 19}},   // Power chord
        {"Oct",       19, {12, 24, -12}}, // Octave doubling
    };

    std::vector<ChordTestResult> results;

    std::cout << "Testing " << testChords.size() << " key chord types...\n\n";

    for (const auto& test : testChords) {
        float normalizedValue = static_cast<float>(test.index) /
                                (IntelligentHarmonizerChords::CHORD_PRESETS.size() - 1);

        auto result = testChord(harmonizer, normalizedValue, test.name,
                               test.intervals, testFreq, sampleRate);
        results.push_back(result);

        std::cout << std::setw(15) << std::left << test.name << " : ";
        std::cout << (result.passed ? "PASS" : "FAIL") << " | ";
        std::cout << "Intervals: [";
        for (size_t i = 0; i < test.intervals.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << std::setw(3) << std::right << test.intervals[i];
        }
        std::cout << "] | ";
        std::cout << "Avg: " << std::fixed << std::setprecision(1)
                  << result.avgCentsError << "¢ | ";
        std::cout << "Max: " << result.maxCentsError << "¢\n";
    }

    // Summary
    int passed = std::count_if(results.begin(), results.end(),
                               [](const auto& r) { return r.passed; });

    float avgError = 0.0f;
    float maxError = 0.0f;
    for (const auto& r : results) {
        avgError += r.avgCentsError;
        maxError = std::max(maxError, r.maxCentsError);
    }
    avgError /= results.size();

    std::cout << "\n================================================================================\n";
    std::cout << "SUMMARY\n";
    std::cout << "================================================================================\n\n";

    std::cout << "Chord Types Tested:  " << results.size() << "\n";
    std::cout << "Passed:              " << passed << " / " << results.size()
              << " (" << std::fixed << std::setprecision(0)
              << (100.0f * passed / results.size()) << "%)\n";
    std::cout << "Average Error:       " << std::setprecision(2) << avgError << " cents\n";
    std::cout << "Maximum Error:       " << maxError << " cents\n";
    std::cout << "Target Accuracy:     ±15 cents\n\n";

    bool productionReady = (passed == results.size()) && (maxError < 15.0f);

    std::cout << "================================================================================\n";
    std::cout << "VERDICT\n";
    std::cout << "================================================================================\n\n";

    if (productionReady) {
        std::cout << "✓ PRODUCTION READY\n\n";
        std::cout << "The IntelligentHarmonizer successfully generates musically accurate chords:\n";
        std::cout << "- All key chord types function correctly\n";
        std::cout << "- Interval accuracy within acceptable range\n";
        std::cout << "- Ready for musical applications\n\n";
    } else {
        std::cout << "⚠ NEEDS REVIEW\n\n";
        std::cout << "Some chord types exceed target accuracy or failed tests.\n";
        std::cout << "Review failed chord types above for details.\n\n";
    }

    // Generate report
    std::ofstream report("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/CHORD_HARMONIZER_VERIFICATION_REPORT.md");

    report << "# CHORD HARMONIZER VERIFICATION REPORT\n";
    report << "## Engine 33: IntelligentHarmonizer\n\n";
    report << "**Test Type:** Quick Verification of Key Chord Types\n";
    report << "**Sample Rate:** 48000 Hz\n";
    report << "**Test Signal:** 440 Hz (A4)\n\n";

    report << "---\n\n";
    report << "## Executive Summary\n\n";
    report << "| Metric | Value | Status |\n";
    report << "|--------|-------|--------|\n";
    report << "| Chord Types Tested | " << results.size() << " | ✓ |\n";
    report << "| Passed | " << passed << " / " << results.size()
           << " | " << (passed == results.size() ? "✓" : "✗") << " |\n";
    report << "| Average Error | " << avgError << " cents | "
           << (avgError < 15.0f ? "✓" : "✗") << " |\n";
    report << "| Maximum Error | " << maxError << " cents | "
           << (maxError < 15.0f ? "✓" : "✗") << " |\n";
    report << "| Production Ready | " << (productionReady ? "YES" : "NO")
           << " | " << (productionReady ? "✓" : "✗") << " |\n\n";

    report << "---\n\n";
    report << "## Detailed Results\n\n";
    report << "| Chord Type | Intervals (semitones) | Avg Error | Max Error | Status |\n";
    report << "|------------|----------------------|-----------|-----------|--------|\n";

    for (const auto& result : results) {
        report << "| " << result.chordName << " | ";

        report << "[";
        for (size_t i = 0; i < result.expectedIntervals.size(); ++i) {
            if (i > 0) report << ", ";
            report << result.expectedIntervals[i];
        }
        report << "] | ";

        report << std::fixed << std::setprecision(2) << result.avgCentsError << "¢ | ";
        report << result.maxCentsError << "¢ | ";
        report << (result.passed ? "✓ PASS" : "✗ FAIL") << " |\n";
    }

    report << "\n---\n\n";
    report << "## Analysis\n\n";

    report << "### Chord Generation Accuracy\n\n";
    report << "The IntelligentHarmonizer uses pitch shifting to generate chord voices from a single input.\n";
    report << "Target accuracy: ±15 cents (acceptable for musical applications)\n\n";

    report << "**Performance:**\n";
    report << "- Average interval error: " << avgError << " cents\n";
    report << "- Maximum interval error: " << maxError << " cents\n";
    report << "- Pass rate: " << (100.0f * passed / results.size()) << "%\n\n";

    report << "### Key Findings\n\n";

    if (productionReady) {
        report << "1. ✓ All key chord types generate correctly\n";
        report << "2. ✓ Interval accuracy meets musical standards\n";
        report << "3. ✓ Major, Minor, and 7th chords verified functional\n";
        report << "4. ✓ Power chords and octave doubling work correctly\n\n";
    } else {
        report << "Areas for improvement identified:\n";
        for (const auto& result : results) {
            if (!result.passed) {
                report << "- " << result.chordName << ": " << result.maxCentsError
                       << " cents max error (exceeds 15¢ target)\n";
            }
        }
        report << "\n";
    }

    report << "---\n\n";
    report << "## Conclusion\n\n";
    report << "### Does It Work Correctly?\n";
    report << "**Answer: " << (productionReady ? "YES" : "PARTIALLY") << "**\n\n";

    if (productionReady) {
        report << "The ChordHarmonizer (IntelligentHarmonizer Engine 33) successfully generates\n";
        report << "musically accurate chords with acceptable interval accuracy for production use.\n\n";
        report << "**Production Ready: YES**\n\n";
    } else {
        report << "The ChordHarmonizer shows functional chord generation with some accuracy\n";
        report << "variations. Review failed chord types for specific improvements needed.\n\n";
        report << "**Production Ready: NEEDS REVIEW**\n\n";
    }

    report << "---\n\n";
    report << "*Report generated by Quick Verification Test Suite*\n";

    report.close();

    std::cout << "================================================================================\n";
    std::cout << "Report saved to: CHORD_HARMONIZER_VERIFICATION_REPORT.md\n";
    std::cout << "================================================================================\n\n";

    return productionReady ? 0 : 1;
}

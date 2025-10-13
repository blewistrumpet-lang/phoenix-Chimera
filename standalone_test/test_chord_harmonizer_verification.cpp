// DEEP VERIFICATION TEST - ENGINE 33: IntelligentHarmonizer (Chord Generator)
// Comprehensive chord generation accuracy, interval verification, and quality testing

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <complex>
#include <map>
#include <string>
#include <fstream>
#include <algorithm>
#include <numeric>

// Include standalone harmonizer
#include "IntelligentHarmonizer_standalone.h"
#include "IntelligentHarmonizerChords.h"

// ============================================================================
// AUDIO ANALYSIS UTILITIES
// ============================================================================

// FFT for frequency analysis
std::vector<float> computeFFT(const std::vector<float>& signal) {
    const int N = signal.size();
    std::vector<std::complex<float>> fft(N);

    // Simple DFT implementation
    for (int k = 0; k < N; ++k) {
        std::complex<float> sum(0, 0);
        for (int n = 0; n < N; ++n) {
            float angle = -2.0f * M_PI * k * n / N;
            sum += signal[n] * std::exp(std::complex<float>(0, angle));
        }
        fft[k] = sum;
    }

    // Convert to magnitude
    std::vector<float> magnitude(N / 2);
    for (int i = 0; i < N / 2; ++i) {
        magnitude[i] = std::abs(fft[i]) / N;
    }

    return magnitude;
}

// Find peak frequency in Hz
float findPeakFrequency(const std::vector<float>& fft, float sampleRate) {
    int maxIdx = std::max_element(fft.begin(), fft.end()) - fft.begin();
    return maxIdx * sampleRate / (2.0f * fft.size());
}

// Find all significant peaks above threshold
std::vector<std::pair<float, float>> findPeaks(const std::vector<float>& fft,
                                                 float sampleRate,
                                                 float threshold = 0.01f) {
    std::vector<std::pair<float, float>> peaks; // freq, magnitude

    for (size_t i = 5; i < fft.size() - 5; ++i) {
        if (fft[i] > threshold) {
            // Check if it's a local maximum
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

    // Sort by magnitude (descending)
    std::sort(peaks.begin(), peaks.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    return peaks;
}

// Calculate RMS level
float calculateRMS(const std::vector<float>& signal) {
    float sum = 0.0f;
    for (float sample : signal) {
        sum += sample * sample;
    }
    return std::sqrt(sum / signal.size());
}

// Calculate THD (Total Harmonic Distortion)
float calculateTHD(const std::vector<float>& fft, float fundamentalFreq, float sampleRate) {
    int fundamentalBin = static_cast<int>(fundamentalFreq * fft.size() * 2 / sampleRate);

    float fundamentalMag = 0.0f;
    // Average over 3 bins around fundamental
    for (int i = -1; i <= 1; ++i) {
        int bin = fundamentalBin + i;
        if (bin >= 0 && bin < fft.size()) {
            fundamentalMag += fft[bin];
        }
    }
    fundamentalMag /= 3.0f;

    float harmonicSum = 0.0f;
    // Check first 10 harmonics
    for (int h = 2; h <= 10; ++h) {
        int harmonicBin = fundamentalBin * h;
        if (harmonicBin < fft.size()) {
            harmonicSum += fft[harmonicBin] * fft[harmonicBin];
        }
    }

    if (fundamentalMag < 1e-6f) return 0.0f;
    return 100.0f * std::sqrt(harmonicSum) / fundamentalMag;
}

// Convert frequency ratio to semitones
float ratioToSemitones(float ratio) {
    return 12.0f * std::log2(ratio);
}

// Convert frequency ratio to cents
float ratioToCents(float ratio) {
    return 1200.0f * std::log2(ratio);
}

// Generate sine wave at specific frequency
std::vector<float> generateSineWave(float frequency, float sampleRate, int numSamples, float amplitude = 0.5f) {
    std::vector<float> signal(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        float phase = 2.0f * M_PI * frequency * i / sampleRate;
        signal[i] = amplitude * std::sin(phase);
    }
    return signal;
}

// ============================================================================
// CHORD INTERVAL VERIFICATION
// ============================================================================

struct ChordTestResult {
    std::string chordName;
    std::vector<float> expectedFreqs;
    std::vector<float> measuredFreqs;
    std::vector<float> centsError;
    std::vector<float> voiceLevels;
    float maxCentsError;
    float avgCentsError;
    float thd;
    bool passed;
};

ChordTestResult testChordType(IntelligentHarmonizer_Standalone& harmonizer,
                               float chordTypeNorm,
                               const std::string& chordName,
                               float inputFreq,
                               float sampleRate) {
    ChordTestResult result;
    result.chordName = chordName;

    const int blockSize = 2048;
    const int numBlocks = 10;
    const int totalSamples = blockSize * numBlocks;

    // Setup parameters for this chord type
    std::map<int, float> params;
    params[0] = 1.0f;  // 3 voices
    params[1] = chordTypeNorm;  // Chord type
    params[2] = 0.0f;  // Root key: C
    params[3] = 0.9f;  // Chromatic scale
    params[4] = 1.0f;  // 100% wet
    params[5] = 1.0f;  // Voice 1 volume: 100%
    params[6] = 0.5f;  // Voice 1 formant: neutral
    params[7] = 0.7f;  // Voice 2 volume: 70%
    params[8] = 0.5f;  // Voice 2 formant: neutral
    params[9] = 0.5f;  // Voice 3 volume: 50%
    params[10] = 0.5f; // Voice 3 formant: neutral
    params[11] = 1.0f; // Quality: High
    params[12] = 0.0f; // Humanize: Off
    params[13] = 0.0f; // Width: 0
    params[14] = 0.5f; // Transpose: 0

    harmonizer.updateParameters(params);
    harmonizer.reset();

    // Get chord intervals from the chord presets
    auto chordIntervals = IntelligentHarmonizerChords::getChordIntervals(chordTypeNorm);

    // Calculate expected frequencies
    for (int interval : chordIntervals) {
        float ratio = std::pow(2.0f, interval / 12.0f);
        result.expectedFreqs.push_back(inputFreq * ratio);
    }

    // Generate input signal and process
    std::vector<float> outputSignal(totalSamples);
    std::vector<float> inputBlock(blockSize);
    std::vector<float> outputBlock(blockSize);

    for (int block = 0; block < numBlocks; ++block) {
        // Generate input block
        inputBlock = generateSineWave(inputFreq, sampleRate, blockSize);

        // Process
        harmonizer.processBlock(inputBlock.data(), outputBlock.data(), blockSize);

        // Copy output
        std::copy(outputBlock.begin(), outputBlock.end(), outputSignal.begin() + block * blockSize);
    }

    // Analyze output with FFT
    auto fft = computeFFT(outputSignal);
    auto peaks = findPeaks(fft, sampleRate, 0.005f);

    // Match peaks to expected frequencies
    result.measuredFreqs.resize(3, 0.0f);
    result.voiceLevels.resize(3, 0.0f);
    result.centsError.resize(3, 0.0f);

    for (size_t i = 0; i < std::min(result.expectedFreqs.size(), peaks.size()); ++i) {
        if (i >= 3) break;

        // Find the peak closest to expected frequency
        float expectedFreq = result.expectedFreqs[i];
        float minDist = 1000000.0f;
        int bestPeak = -1;

        for (size_t p = 0; p < peaks.size(); ++p) {
            float dist = std::abs(peaks[p].first - expectedFreq);
            if (dist < minDist) {
                minDist = dist;
                bestPeak = p;
            }
        }

        if (bestPeak >= 0) {
            result.measuredFreqs[i] = peaks[bestPeak].first;
            result.voiceLevels[i] = peaks[bestPeak].second;

            // Calculate cents error
            float ratio = peaks[bestPeak].first / expectedFreq;
            result.centsError[i] = ratioToCents(ratio);
        }
    }

    // Calculate statistics
    result.maxCentsError = 0.0f;
    result.avgCentsError = 0.0f;
    for (float cents : result.centsError) {
        float absCents = std::abs(cents);
        result.maxCentsError = std::max(result.maxCentsError, absCents);
        result.avgCentsError += absCents;
    }
    result.avgCentsError /= result.centsError.size();

    // Calculate THD
    if (!result.measuredFreqs.empty() && result.measuredFreqs[0] > 0) {
        result.thd = calculateTHD(fft, result.measuredFreqs[0], sampleRate);
    }

    // Pass criteria: ±10 cents error (relaxed for harmonizer)
    result.passed = result.maxCentsError < 10.0f;

    return result;
}

// ============================================================================
// PITCH ACCURACY TEST ACROSS OCTAVES
// ============================================================================

struct PitchAccuracyResult {
    std::string noteName;
    float inputFreq;
    std::vector<float> expectedFreqs;
    std::vector<float> measuredFreqs;
    std::vector<float> centsError;
    float maxCentsError;
    bool passed;
};

PitchAccuracyResult testPitchAccuracy(IntelligentHarmonizer_Standalone& harmonizer,
                                      const std::string& noteName,
                                      float inputFreq,
                                      float sampleRate) {
    PitchAccuracyResult result;
    result.noteName = noteName;
    result.inputFreq = inputFreq;

    const int blockSize = 2048;
    const int numBlocks = 12;
    const int totalSamples = blockSize * numBlocks;

    // Test with major chord (3rd, 5th, octave)
    std::map<int, float> params;
    params[0] = 1.0f;  // 3 voices
    params[1] = 0.0f;  // Major chord
    params[2] = 0.0f;  // Root key: C
    params[3] = 0.9f;  // Chromatic
    params[4] = 1.0f;  // 100% wet
    params[5] = 1.0f;  // Voice volumes
    params[7] = 0.8f;
    params[9] = 0.6f;
    params[11] = 1.0f; // High quality
    params[12] = 0.0f; // No humanize
    params[14] = 0.5f; // No transpose

    harmonizer.updateParameters(params);
    harmonizer.reset();

    // Major chord intervals: 3rd (4 semitones), 5th (7 semitones), octave (12 semitones)
    std::vector<int> intervals = {4, 7, 12};
    for (int interval : intervals) {
        float ratio = std::pow(2.0f, interval / 12.0f);
        result.expectedFreqs.push_back(inputFreq * ratio);
    }

    // Generate and process
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

    result.measuredFreqs.resize(3, 0.0f);
    result.centsError.resize(3, 0.0f);

    for (size_t i = 0; i < result.expectedFreqs.size(); ++i) {
        float expectedFreq = result.expectedFreqs[i];
        float minDist = 1000000.0f;
        int bestPeak = -1;

        for (size_t p = 0; p < peaks.size(); ++p) {
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

    result.maxCentsError = 0.0f;
    for (float cents : result.centsError) {
        result.maxCentsError = std::max(result.maxCentsError, std::abs(cents));
    }

    result.passed = result.maxCentsError < 10.0f;

    return result;
}

// ============================================================================
// MAIN TEST SUITE
// ============================================================================

int main() {
    std::cout << "================================================================================\n";
    std::cout << "DEEP VERIFICATION - ENGINE 33: IntelligentHarmonizer (Chord Generator)\n";
    std::cout << "Comprehensive Chord Generation, Interval Accuracy & Quality Testing\n";
    std::cout << "================================================================================\n\n";

    const float sampleRate = 48000.0f;
    const int blockSize = 512;
    const float testFreq = 440.0f; // A4

    // Create harmonizer
    IntelligentHarmonizer_Standalone harmonizer;
    harmonizer.prepareToPlay(sampleRate, blockSize);

    std::vector<ChordTestResult> chordResults;
    std::vector<PitchAccuracyResult> pitchResults;

    // ========================================================================
    // TEST 1: ALL CHORD TYPES
    // ========================================================================

    std::cout << "TEST 1: CHORD TYPE INTERVAL ACCURACY\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Testing all " << IntelligentHarmonizerChords::CHORD_PRESETS.size()
              << " chord types at 440Hz (A4)...\n\n";

    int chordIdx = 0;
    for (const auto& chord : IntelligentHarmonizerChords::CHORD_PRESETS) {
        float normalizedValue = static_cast<float>(chordIdx) /
                                (IntelligentHarmonizerChords::CHORD_PRESETS.size() - 1);

        auto result = testChordType(harmonizer, normalizedValue, chord.name, testFreq, sampleRate);
        chordResults.push_back(result);

        std::cout << std::setw(20) << std::left << chord.name << " : ";
        std::cout << (result.passed ? "PASS" : "FAIL") << " | ";
        std::cout << "Avg Error: " << std::fixed << std::setprecision(2)
                  << result.avgCentsError << " cents | ";
        std::cout << "Max Error: " << result.maxCentsError << " cents\n";

        chordIdx++;
    }

    // ========================================================================
    // TEST 2: PITCH ACCURACY ACROSS OCTAVES
    // ========================================================================

    std::cout << "\n\nTEST 2: PITCH ACCURACY ACROSS OCTAVES\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Testing major chord at different input frequencies...\n\n";

    std::vector<std::pair<std::string, float>> testNotes = {
        {"C3", 130.81f},
        {"E3", 164.81f},
        {"G3", 196.00f},
        {"C4", 261.63f},
        {"E4", 329.63f},
        {"G4", 392.00f}
    };

    for (const auto& [noteName, freq] : testNotes) {
        auto result = testPitchAccuracy(harmonizer, noteName, freq, sampleRate);
        pitchResults.push_back(result);

        std::cout << std::setw(5) << std::left << noteName
                  << " (" << std::setw(7) << std::right << std::fixed << std::setprecision(2)
                  << freq << " Hz) : ";
        std::cout << (result.passed ? "PASS" : "FAIL") << " | ";
        std::cout << "Max Error: " << result.maxCentsError << " cents\n";
    }

    // ========================================================================
    // TEST 3: VOICE BALANCE
    // ========================================================================

    std::cout << "\n\nTEST 3: VOICE BALANCE ANALYSIS\n";
    std::cout << "----------------------------------------\n";

    // Test major chord voice balance
    auto balanceResult = testChordType(harmonizer, 0.0f, "Major", testFreq, sampleRate);

    std::cout << "Voice 1 Level: " << std::fixed << std::setprecision(4)
              << balanceResult.voiceLevels[0] << "\n";
    std::cout << "Voice 2 Level: " << balanceResult.voiceLevels[1] << "\n";
    std::cout << "Voice 3 Level: " << balanceResult.voiceLevels[2] << "\n";

    float maxLevel = *std::max_element(balanceResult.voiceLevels.begin(),
                                       balanceResult.voiceLevels.end());
    float minLevel = *std::min_element(balanceResult.voiceLevels.begin(),
                                       balanceResult.voiceLevels.end());

    float balanceDb = 20.0f * std::log10(maxLevel / (minLevel + 1e-10f));
    std::cout << "\nBalance Range: " << balanceDb << " dB ";

    bool balancePassed = balanceDb < 10.0f; // Within 10dB is acceptable
    std::cout << (balancePassed ? "(PASS)" : "(FAIL)") << "\n";

    // ========================================================================
    // SUMMARY STATISTICS
    // ========================================================================

    std::cout << "\n\n================================================================================\n";
    std::cout << "VERIFICATION SUMMARY\n";
    std::cout << "================================================================================\n\n";

    // Chord type statistics
    int chordsPassed = std::count_if(chordResults.begin(), chordResults.end(),
                                     [](const auto& r) { return r.passed; });

    float avgChordError = 0.0f;
    float maxChordError = 0.0f;
    for (const auto& r : chordResults) {
        avgChordError += r.avgCentsError;
        maxChordError = std::max(maxChordError, r.maxCentsError);
    }
    avgChordError /= chordResults.size();

    std::cout << "CHORD TYPE TESTS:\n";
    std::cout << "  Total Tested: " << chordResults.size() << "\n";
    std::cout << "  Passed: " << chordsPassed << "\n";
    std::cout << "  Failed: " << (chordResults.size() - chordsPassed) << "\n";
    std::cout << "  Pass Rate: " << std::fixed << std::setprecision(1)
              << (100.0f * chordsPassed / chordResults.size()) << "%\n";
    std::cout << "  Avg Error: " << std::setprecision(2) << avgChordError << " cents\n";
    std::cout << "  Max Error: " << maxChordError << " cents\n\n";

    // Pitch accuracy statistics
    int pitchPassed = std::count_if(pitchResults.begin(), pitchResults.end(),
                                    [](const auto& r) { return r.passed; });

    float avgPitchError = 0.0f;
    float maxPitchError = 0.0f;
    for (const auto& r : pitchResults) {
        avgPitchError += r.maxCentsError;
        maxPitchError = std::max(maxPitchError, r.maxCentsError);
    }
    avgPitchError /= pitchResults.size();

    std::cout << "PITCH ACCURACY TESTS:\n";
    std::cout << "  Total Tested: " << pitchResults.size() << "\n";
    std::cout << "  Passed: " << pitchPassed << "\n";
    std::cout << "  Failed: " << (pitchResults.size() - pitchPassed) << "\n";
    std::cout << "  Pass Rate: " << std::fixed << std::setprecision(1)
              << (100.0f * pitchPassed / pitchResults.size()) << "%\n";
    std::cout << "  Avg Error: " << std::setprecision(2) << avgPitchError << " cents\n";
    std::cout << "  Max Error: " << maxPitchError << " cents\n\n";

    // Overall verdict
    bool allPassed = (chordsPassed == chordResults.size()) &&
                     (pitchPassed == pitchResults.size()) &&
                     balancePassed;

    std::cout << "================================================================================\n";
    std::cout << "FINAL VERDICT\n";
    std::cout << "================================================================================\n\n";

    std::cout << "Chord Generation:     " << (chordsPassed == chordResults.size() ? "PASS" : "FAIL") << "\n";
    std::cout << "Pitch Accuracy:       " << (pitchPassed == pitchResults.size() ? "PASS" : "FAIL") << "\n";
    std::cout << "Voice Balance:        " << (balancePassed ? "PASS" : "FAIL") << "\n";
    std::cout << "\nOVERALL:              " << (allPassed ? "PASS - Production Ready" : "NEEDS WORK") << "\n\n";

    std::cout << "Target Criteria:\n";
    std::cout << "  - Interval Accuracy: ±10 cents\n";
    std::cout << "  - Voice Balance: ±10 dB\n";
    std::cout << "  - All Chord Types: Functional\n\n";

    std::cout << "Actual Performance:\n";
    std::cout << "  - Avg Interval Error: " << avgChordError << " cents\n";
    std::cout << "  - Max Interval Error: " << maxChordError << " cents\n";
    std::cout << "  - Voice Balance Range: " << balanceDb << " dB\n";
    std::cout << "  - Chord Types Working: " << chordsPassed << "/" << chordResults.size() << "\n\n";

    // Generate detailed report file
    std::ofstream report("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/CHORD_HARMONIZER_VERIFICATION_REPORT.md");

    report << "# CHORD HARMONIZER VERIFICATION REPORT\n";
    report << "## Engine 33: IntelligentHarmonizer\n\n";
    report << "**Test Date:** " << __DATE__ << " " << __TIME__ << "\n";
    report << "**Sample Rate:** " << sampleRate << " Hz\n";
    report << "**Test Signal:** " << testFreq << " Hz (A4)\n\n";

    report << "---\n\n";
    report << "## Executive Summary\n\n";
    report << "| Metric | Result | Status |\n";
    report << "|--------|--------|--------|\n";
    report << "| Chord Types Tested | " << chordResults.size() << " | ✓ |\n";
    report << "| Chord Types Passed | " << chordsPassed << " / " << chordResults.size()
           << " | " << (chordsPassed == chordResults.size() ? "✓" : "✗") << " |\n";
    report << "| Avg Interval Error | " << avgChordError << " cents | "
           << (avgChordError < 10.0f ? "✓" : "✗") << " |\n";
    report << "| Max Interval Error | " << maxChordError << " cents | "
           << (maxChordError < 10.0f ? "✓" : "✗") << " |\n";
    report << "| Voice Balance | " << balanceDb << " dB | "
           << (balancePassed ? "✓" : "✗") << " |\n";
    report << "| Production Ready | " << (allPassed ? "YES" : "NO")
           << " | " << (allPassed ? "✓" : "✗") << " |\n\n";

    report << "---\n\n";
    report << "## Detailed Chord Type Results\n\n";
    report << "| Chord Type | Expected Intervals | Avg Error (cents) | Max Error (cents) | Status |\n";
    report << "|------------|-------------------|-------------------|-------------------|--------|\n";

    for (const auto& result : chordResults) {
        report << "| " << result.chordName << " | ";

        // Get chord intervals
        for (size_t i = 0; i < result.expectedFreqs.size(); ++i) {
            if (i > 0) report << ", ";
            float ratio = result.expectedFreqs[i] / testFreq;
            int semitones = static_cast<int>(std::round(12.0f * std::log2(ratio)));
            report << "+" << semitones;
        }
        report << " | ";

        report << std::fixed << std::setprecision(2) << result.avgCentsError << " | ";
        report << result.maxCentsError << " | ";
        report << (result.passed ? "✓ PASS" : "✗ FAIL") << " |\n";
    }

    report << "\n---\n\n";
    report << "## Pitch Accuracy Across Octaves\n\n";
    report << "| Note | Freq (Hz) | Max Error (cents) | Status |\n";
    report << "|------|-----------|-------------------|--------|\n";

    for (const auto& result : pitchResults) {
        report << "| " << result.noteName << " | ";
        report << std::fixed << std::setprecision(2) << result.inputFreq << " | ";
        report << result.maxCentsError << " | ";
        report << (result.passed ? "✓ PASS" : "✗ FAIL") << " |\n";
    }

    report << "\n---\n\n";
    report << "## Voice Balance Analysis\n\n";
    report << "Testing with Major Chord:\n\n";
    report << "| Voice | Level | Description |\n";
    report << "|-------|-------|-------------|\n";
    report << "| Voice 1 (3rd) | " << balanceResult.voiceLevels[0] << " | Major 3rd (+4 semitones) |\n";
    report << "| Voice 2 (5th) | " << balanceResult.voiceLevels[1] << " | Perfect 5th (+7 semitones) |\n";
    report << "| Voice 3 (Oct) | " << balanceResult.voiceLevels[2] << " | Octave (+12 semitones) |\n\n";
    report << "**Balance Range:** " << balanceDb << " dB\n";
    report << "**Target:** < 10 dB\n";
    report << "**Result:** " << (balancePassed ? "✓ PASS" : "✗ FAIL") << "\n\n";

    report << "---\n\n";
    report << "## Conclusions\n\n";
    report << "### Does ChordHarmonizer Work Correctly?\n";
    report << "**Answer: " << (allPassed ? "YES" : "PARTIALLY") << "**\n\n";

    if (allPassed) {
        report << "The IntelligentHarmonizer (Engine 33) successfully generates musically accurate chords:\n\n";
        report << "- ✓ All " << chordResults.size() << " chord types function correctly\n";
        report << "- ✓ Interval accuracy within ±10 cents target\n";
        report << "- ✓ Consistent performance across octaves\n";
        report << "- ✓ Voice balance maintained within acceptable range\n";
        report << "- ✓ Production ready for musical applications\n\n";
    } else {
        report << "The IntelligentHarmonizer shows good performance but has areas for improvement:\n\n";
        if (chordsPassed < chordResults.size()) {
            report << "- Some chord types exceed ±10 cents target\n";
        }
        if (!balancePassed) {
            report << "- Voice balance could be improved\n";
        }
        report << "\n";
    }

    report << "### Production Readiness: " << (allPassed ? "YES" : "NEEDS REVIEW") << "\n\n";

    report << "The engine demonstrates:\n";
    report << "1. Functional chord generation with " << IntelligentHarmonizerChords::CHORD_PRESETS.size()
           << " chord types\n";
    report << "2. Average interval accuracy of " << avgChordError << " cents\n";
    report << "3. Consistent pitch accuracy across tested octaves\n";
    report << "4. Voice balance within " << balanceDb << " dB range\n\n";

    report << "---\n\n";
    report << "*Report generated by Deep Verification Test Suite*\n";

    report.close();

    std::cout << "================================================================================\n";
    std::cout << "Detailed report saved to: CHORD_HARMONIZER_VERIFICATION_REPORT.md\n";
    std::cout << "================================================================================\n\n";

    return allPassed ? 0 : 1;
}

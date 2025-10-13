#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <fstream>
#include <map>
#include <algorithm>
#include <numeric>
#include <sstream>

//==============================================================================
// SCIENTIFIC PITCH ACCURACY ANALYSIS SUITE
//
// Multiple Detection Algorithms:
// 1. Autocorrelation (YIN algorithm) - Most accurate for pitch
// 2. Cepstrum analysis - Good for harmonic signals
// 3. FFT peak detection with parabolic interpolation
// 4. Zero-crossing rate - Simple validation
// 5. Harmonic Product Spectrum (HPS) - Robust for complex tones
// 6. Average Magnitude Difference Function (AMDF)
//
// Statistical Analysis:
// - Mean accuracy ± standard deviation
// - 95% confidence intervals
// - Worst-case and best-case errors
// - Pass/fail criteria (±5 cents threshold)
//
// Professional Standards:
// - Excellent: ±1 cent (Melodyne, Celemony)
// - Professional: ±3 cents (Auto-Tune, industry standard)
// - Acceptable: ±5 cents (consumer products)
// - Poor: ±10 cents (barely usable)
// - Fail: >±10 cents (broken)
//==============================================================================

// Engine mapping - ALL pitch-related engines
const std::map<int, std::string> PITCH_ENGINES = {
    {31, "Pitch Shifter"},
    {32, "Detune Doubler"},
    {33, "Intelligent Harmonizer"},
    {42, "Shimmer Reverb"},      // Has pitch shifting
    {49, "Phased Vocoder"},       // Time/pitch manipulation
    {50, "Granular Cloud"},       // Grain pitch shifting
};

// Test configuration
const std::vector<int> SEMITONE_SHIFTS = {-12, -7, -5, -2, 0, +2, +5, +7, +12};
const std::vector<float> TEST_FREQUENCIES = {55.0f, 110.0f, 220.0f, 440.0f, 880.0f, 1760.0f};
const float SAMPLE_RATE = 48000.0f;
const int BLOCK_SIZE = 512;

//==============================================================================
// ALGORITHM 1: YIN Autocorrelation (Most Accurate)
//==============================================================================
float detectPitch_YIN(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int N = std::min(8192, buffer.getNumSamples());
    const int tauMax = N / 2;
    const float* data = buffer.getReadPointer(0);

    std::vector<float> yinBuffer(tauMax);

    // Step 1: Calculate difference function
    yinBuffer[0] = 1.0f;
    for (int tau = 1; tau < tauMax; ++tau) {
        float sum = 0.0f;
        for (int i = 0; i < N - tau; ++i) {
            float delta = data[i] - data[i + tau];
            sum += delta * delta;
        }
        yinBuffer[tau] = sum;
    }

    // Step 2: Cumulative mean normalized difference
    float runningSum = 0.0f;
    yinBuffer[0] = 1.0f;
    for (int tau = 1; tau < tauMax; ++tau) {
        runningSum += yinBuffer[tau];
        yinBuffer[tau] *= tau / runningSum;
    }

    // Step 3: Absolute threshold
    const float threshold = 0.1f;
    int tau = -1;
    for (int t = 2; t < tauMax; ++t) {
        if (yinBuffer[t] < threshold) {
            while (t + 1 < tauMax && yinBuffer[t + 1] < yinBuffer[t]) {
                ++t;
            }
            tau = t;
            break;
        }
    }

    if (tau == -1) return 0.0f;

    // Step 4: Parabolic interpolation
    if (tau > 0 && tau < tauMax - 1) {
        float s0 = yinBuffer[tau - 1];
        float s1 = yinBuffer[tau];
        float s2 = yinBuffer[tau + 1];
        float adjustment = (s2 - s0) / (2.0f * (2.0f * s1 - s2 - s0));
        return sampleRate / (tau + adjustment);
    }

    return sampleRate / tau;
}

//==============================================================================
// ALGORITHM 2: Cepstrum Analysis
//==============================================================================
float detectPitch_Cepstrum(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13); // 2^13 = 8192
    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy with Hann window
    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    // Forward FFT
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Convert to log spectrum
    for (int i = 0; i < fftSize / 2; ++i) {
        fftData[i] = std::log(std::max(1e-10f, fftData[i]));
        fftData[fftSize - 1 - i] = fftData[i]; // Mirror for inverse FFT
    }

    // Inverse FFT to get cepstrum
    juce::dsp::FFT ifft(13);
    std::vector<float> cepstrum(fftSize * 2, 0.0f);
    std::copy(fftData.begin(), fftData.end(), cepstrum.begin());
    ifft.performRealOnlyInverseTransform(cepstrum.data());

    // Find peak in quefrency domain
    int minQuefrency = static_cast<int>(sampleRate / 2000.0f);
    int maxQuefrency = static_cast<int>(sampleRate / 50.0f);

    int maxBin = minQuefrency;
    float maxVal = 0.0f;
    for (int i = minQuefrency; i < maxQuefrency && i < fftSize / 2; ++i) {
        if (cepstrum[i] > maxVal) {
            maxVal = cepstrum[i];
            maxBin = i;
        }
    }

    if (maxVal < 0.01f) return 0.0f;

    return sampleRate / maxBin;
}

//==============================================================================
// ALGORITHM 3: FFT Peak Detection with Parabolic Interpolation
//==============================================================================
float detectPitch_FFT(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    int maxBin = 0;
    float maxMag = 0.0f;
    int minBin = 5;

    for (int i = minBin; i < fftSize / 2; ++i) {
        if (fftData[i] > maxMag) {
            maxMag = fftData[i];
            maxBin = i;
        }
    }

    if (maxBin == 0 || maxMag < 1e-6f) return 0.0f;

    // Parabolic interpolation
    if (maxBin > 0 && maxBin < fftSize / 2 - 1) {
        float alpha = fftData[maxBin - 1];
        float beta = fftData[maxBin];
        float gamma = fftData[maxBin + 1];

        if (alpha > 0.0f && gamma > 0.0f) {
            float p = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);
            float interpolatedBin = maxBin + p;
            return interpolatedBin * sampleRate / fftSize;
        }
    }

    return maxBin * sampleRate / fftSize;
}

//==============================================================================
// ALGORITHM 4: Zero-Crossing Rate
//==============================================================================
float detectPitch_ZeroCrossing(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    int crossings = 0;
    for (int i = 1; i < numSamples; ++i) {
        if ((data[i - 1] >= 0.0f && data[i] < 0.0f) ||
            (data[i - 1] < 0.0f && data[i] >= 0.0f)) {
            crossings++;
        }
    }

    if (crossings < 2) return 0.0f;

    return (crossings * sampleRate) / (2.0f * numSamples);
}

//==============================================================================
// ALGORITHM 5: Harmonic Product Spectrum (HPS)
//==============================================================================
float detectPitch_HPS(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    const int numHarmonics = 4;
    std::vector<float> hps(fftSize / 2, 1.0f);

    for (int h = 1; h <= numHarmonics; ++h) {
        for (int i = 0; i < fftSize / (2 * h); ++i) {
            hps[i] *= fftData[i * h];
        }
    }

    int maxBin = 5;
    float maxMag = 0.0f;
    for (int i = 5; i < fftSize / (2 * numHarmonics); ++i) {
        if (hps[i] > maxMag) {
            maxMag = hps[i];
            maxBin = i;
        }
    }

    if (maxMag < 1e-10f) return 0.0f;

    return maxBin * sampleRate / fftSize;
}

//==============================================================================
// ALGORITHM 6: Average Magnitude Difference Function (AMDF)
//==============================================================================
float detectPitch_AMDF(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int N = std::min(8192, buffer.getNumSamples());
    const int tauMax = N / 2;
    const float* data = buffer.getReadPointer(0);

    std::vector<float> amdf(tauMax);

    for (int tau = 1; tau < tauMax; ++tau) {
        float sum = 0.0f;
        for (int i = 0; i < N - tau; ++i) {
            sum += std::abs(data[i] - data[i + tau]);
        }
        amdf[tau] = sum / (N - tau);
    }

    int minTau = -1;
    float minVal = 1e10f;

    for (int tau = 20; tau < tauMax; ++tau) {
        if (amdf[tau] < minVal && tau > 10) {
            minVal = amdf[tau];
            minTau = tau;
        }
    }

    if (minTau == -1 || minVal > 0.5f) return 0.0f;

    return sampleRate / minTau;
}

//==============================================================================
// Multi-Algorithm Consensus
//==============================================================================
struct PitchMeasurement {
    float yin;
    float cepstrum;
    float fft;
    float zeroCrossing;
    float hps;
    float amdf;
    float consensus;
    float deviation;
    bool valid;
    std::string errorMsg;
};

PitchMeasurement measurePitchMultiAlgorithm(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    PitchMeasurement result;

    result.yin = detectPitch_YIN(buffer, sampleRate);
    result.cepstrum = detectPitch_Cepstrum(buffer, sampleRate);
    result.fft = detectPitch_FFT(buffer, sampleRate);
    result.zeroCrossing = detectPitch_ZeroCrossing(buffer, sampleRate);
    result.hps = detectPitch_HPS(buffer, sampleRate);
    result.amdf = detectPitch_AMDF(buffer, sampleRate);

    std::vector<float> validMeasurements;
    if (result.yin > 0.0f) validMeasurements.push_back(result.yin);
    if (result.cepstrum > 0.0f) validMeasurements.push_back(result.cepstrum);
    if (result.fft > 0.0f) validMeasurements.push_back(result.fft);
    if (result.zeroCrossing > 0.0f) validMeasurements.push_back(result.zeroCrossing);
    if (result.hps > 0.0f) validMeasurements.push_back(result.hps);
    if (result.amdf > 0.0f) validMeasurements.push_back(result.amdf);

    if (validMeasurements.empty()) {
        result.consensus = 0.0f;
        result.deviation = 0.0f;
        result.valid = false;
        result.errorMsg = "No algorithm detected pitch";
        return result;
    }

    std::sort(validMeasurements.begin(), validMeasurements.end());
    if (validMeasurements.size() % 2 == 0) {
        result.consensus = (validMeasurements[validMeasurements.size() / 2 - 1] +
                           validMeasurements[validMeasurements.size() / 2]) / 2.0f;
    } else {
        result.consensus = validMeasurements[validMeasurements.size() / 2];
    }

    float sum = 0.0f;
    for (float f : validMeasurements) {
        float diff = f - result.consensus;
        sum += diff * diff;
    }
    result.deviation = std::sqrt(sum / validMeasurements.size());

    float maxDevCents = 0.0f;
    for (float f : validMeasurements) {
        float cents = std::abs(1200.0f * std::log2(f / result.consensus));
        maxDevCents = std::max(maxDevCents, cents);
    }

    if (maxDevCents > 50.0f) {
        result.valid = false;
        result.errorMsg = "Algorithms disagree (>50 cents)";
    } else {
        result.valid = true;
    }

    return result;
}

//==============================================================================
// Calculate cent error
//==============================================================================
float calculateCentError(float measuredFreq, float expectedFreq) {
    if (measuredFreq <= 0.0f || expectedFreq <= 0.0f) return 9999.0f;
    return 1200.0f * std::log2(measuredFreq / expectedFreq);
}

//==============================================================================
// Test result structure
//==============================================================================
struct PitchTestResult {
    int engineId;
    std::string engineName;
    float inputFreq;
    int semitoneShift;
    float expectedFreq;
    PitchMeasurement measurement;
    float centError;
    bool pass;
    std::string notes;
};

//==============================================================================
// Test single configuration
//==============================================================================
PitchTestResult testPitchConfiguration(int engineId, const std::string& engineName,
                                       float inputFreq, int semitoneShift) {
    PitchTestResult result;
    result.engineId = engineId;
    result.engineName = engineName;
    result.inputFreq = inputFreq;
    result.semitoneShift = semitoneShift;
    result.expectedFreq = inputFreq * std::pow(2.0f, semitoneShift / 12.0f);
    result.pass = false;

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.notes = "Failed to create engine";
            return result;
        }

        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);

        std::map<int, float> params;

        if (engineId == 31 || engineId == 32 || engineId == 49) {
            float normalizedShift = (semitoneShift + 12.0f) / 24.0f;
            params[0] = std::clamp(normalizedShift, 0.0f, 1.0f);
            if (engine->getNumParameters() > 1) params[1] = 1.0f;
        }
        else if (engineId == 33) {
            float normalizedShift = (semitoneShift + 12.0f) / 24.0f;
            params[0] = std::clamp(normalizedShift, 0.0f, 1.0f);
            if (engine->getNumParameters() > 1) params[1] = 1.0f;
        }
        else if (engineId == 42) {
            params[0] = 0.5f;
            params[1] = 1.0f;
            params[2] = 1.0f;
            if (engine->getNumParameters() > 3) {
                float normalizedShift = (semitoneShift + 12.0f) / 24.0f;
                params[3] = std::clamp(normalizedShift, 0.0f, 1.0f);
            }
        }
        else if (engineId == 50) {
            params[0] = 0.5f;
            float normalizedShift = (semitoneShift + 12.0f) / 24.0f;
            if (engine->getNumParameters() > 1) params[1] = std::clamp(normalizedShift, 0.0f, 1.0f);
            if (engine->getNumParameters() > 2) params[2] = 1.0f;
        }

        engine->reset();
        engine->updateParameters(params);

        const int testLength = 32768;
        juce::AudioBuffer<float> testBuffer(2, testLength);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * M_PI * inputFreq * i / SAMPLE_RATE;
                testBuffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        for (int start = 0; start < testLength; start += BLOCK_SIZE) {
            int samplesThisBlock = std::min(BLOCK_SIZE, testLength - start);
            juce::AudioBuffer<float> block(testBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        int skipSamples = testLength / 5;
        int analysisSamples = testLength - skipSamples;

        juce::AudioBuffer<float> analysisBuffer(2, analysisSamples);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < analysisSamples; ++i) {
                analysisBuffer.setSample(ch, i, testBuffer.getSample(ch, i + skipSamples));
            }
        }

        result.measurement = measurePitchMultiAlgorithm(analysisBuffer, SAMPLE_RATE);

        if (!result.measurement.valid) {
            result.notes = result.measurement.errorMsg;
            return result;
        }

        result.centError = calculateCentError(result.measurement.consensus, result.expectedFreq);

        result.pass = std::abs(result.centError) < 5.0f;

        if (std::abs(result.centError) < 1.0f) {
            result.notes = "EXCELLENT (Melodyne-level)";
        } else if (std::abs(result.centError) < 3.0f) {
            result.notes = "PROFESSIONAL (Auto-Tune level)";
        } else if (std::abs(result.centError) < 5.0f) {
            result.notes = "ACCEPTABLE (Consumer-grade)";
        } else if (std::abs(result.centError) < 10.0f) {
            result.notes = "POOR (Noticeable)";
        } else {
            result.notes = "FAIL (Unacceptable)";
        }

    } catch (const std::exception& e) {
        result.notes = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Statistical analysis
//==============================================================================
struct EngineStatistics {
    int engineId;
    std::string engineName;
    int totalTests;
    int validTests;
    int passedTests;
    float meanError;
    float stdDeviation;
    float minError;
    float maxError;
    float confidence95Low;
    float confidence95High;
    std::string qualityRating;
};

EngineStatistics calculateStatistics(const std::vector<PitchTestResult>& results) {
    EngineStatistics stats;

    if (results.empty()) return stats;

    stats.engineId = results[0].engineId;
    stats.engineName = results[0].engineName;
    stats.totalTests = results.size();
    stats.validTests = 0;
    stats.passedTests = 0;

    std::vector<float> errors;

    for (const auto& r : results) {
        if (r.measurement.valid) {
            stats.validTests++;
            if (r.pass) stats.passedTests++;

            float absError = std::abs(r.centError);
            errors.push_back(absError);
        }
    }

    if (errors.empty()) {
        stats.qualityRating = "NO DATA";
        return stats;
    }

    stats.meanError = std::accumulate(errors.begin(), errors.end(), 0.0f) / errors.size();

    float variance = 0.0f;
    for (float e : errors) {
        float diff = e - stats.meanError;
        variance += diff * diff;
    }
    stats.stdDeviation = std::sqrt(variance / errors.size());

    stats.minError = *std::min_element(errors.begin(), errors.end());
    stats.maxError = *std::max_element(errors.begin(), errors.end());

    float marginOfError = 1.96f * stats.stdDeviation / std::sqrt(errors.size());
    stats.confidence95Low = stats.meanError - marginOfError;
    stats.confidence95High = stats.meanError + marginOfError;

    if (stats.meanError < 1.0f && stats.maxError < 3.0f) {
        stats.qualityRating = "EXCELLENT (Melodyne-level)";
    } else if (stats.meanError < 3.0f && stats.maxError < 5.0f) {
        stats.qualityRating = "PROFESSIONAL (Auto-Tune level)";
    } else if (stats.meanError < 5.0f && stats.maxError < 10.0f) {
        stats.qualityRating = "ACCEPTABLE (Consumer-grade)";
    } else if (stats.meanError < 10.0f) {
        stats.qualityRating = "POOR (Noticeable errors)";
    } else {
        stats.qualityRating = "FAIL (Unacceptable)";
    }

    return stats;
}

//==============================================================================
// Generate scientific report
//==============================================================================
void generateScientificReport(const std::vector<PitchTestResult>& allResults,
                              const std::vector<EngineStatistics>& allStats,
                              const std::string& filename) {
    std::ofstream report(filename);

    report << "# SCIENTIFIC PITCH ACCURACY ANALYSIS REPORT\n\n";
    report << "## Executive Summary\n\n";
    report << "This report presents a comprehensive, publication-quality analysis of pitch accuracy across all pitch-shifting engines in Project Chimera v3.0.\n\n";

    report << "### Test Methodology\n\n";
    report << "**Multiple Detection Algorithms:**\n";
    report << "1. **YIN Autocorrelation** - Industry standard for pitch detection\n";
    report << "2. **Cepstrum Analysis** - Optimal for harmonic signals\n";
    report << "3. **FFT Peak Detection** - Fast, accurate for pure tones\n";
    report << "4. **Zero-Crossing Rate** - Simple validation method\n";
    report << "5. **Harmonic Product Spectrum (HPS)** - Robust for complex tones\n";
    report << "6. **AMDF (Average Magnitude Difference Function)** - Alternative autocorrelation\n\n";

    report << "**Consensus Approach:**\n";
    report << "- All 6 algorithms run independently\n";
    report << "- Median value used (robust against outliers)\n";
    report << "- Cross-validation: algorithms must agree within ±50 cents\n";
    report << "- Results reported in cents (1/100th of a semitone)\n\n";

    report << "**Test Matrix:**\n";
    report << "- Engines tested: " << PITCH_ENGINES.size() << "\n";
    report << "- Test frequencies: ";
    for (size_t i = 0; i < TEST_FREQUENCIES.size(); ++i) {
        report << TEST_FREQUENCIES[i] << "Hz";
        if (i < TEST_FREQUENCIES.size() - 1) report << ", ";
    }
    report << "\n";
    report << "- Pitch shifts: ";
    for (size_t i = 0; i < SEMITONE_SHIFTS.size(); ++i) {
        report << std::showpos << SEMITONE_SHIFTS[i] << std::noshowpos << "st";
        if (i < SEMITONE_SHIFTS.size() - 1) report << ", ";
    }
    report << "\n";
    report << "- Total tests: " << allResults.size() << "\n";
    report << "- Sample rate: " << SAMPLE_RATE << " Hz\n\n";

    report << "### Professional Standards Comparison\n\n";
    report << "| Category | Accuracy | Examples |\n";
    report << "|----------|----------|----------|\n";
    report << "| EXCELLENT | ±1 cent | Melodyne, Celemony |\n";
    report << "| PROFESSIONAL | ±3 cents | Auto-Tune, industry standard |\n";
    report << "| ACCEPTABLE | ±5 cents | Consumer products |\n";
    report << "| POOR | ±10 cents | Barely usable |\n";
    report << "| FAIL | >±10 cents | Broken |\n\n";

    report << "## Statistical Summary\n\n";
    report << "| Engine | Quality | Mean±SD | 95% CI | Min | Max | Pass Rate |\n";
    report << "|--------|---------|---------|---------|-----|-----|----------|\n";

    for (const auto& stats : allStats) {
        report << "| " << stats.engineId << " - " << stats.engineName << " | "
               << stats.qualityRating << " | "
               << std::fixed << std::setprecision(2) << stats.meanError << "±" << stats.stdDeviation << " | "
               << "[" << stats.confidence95Low << ", " << stats.confidence95High << "] | "
               << stats.minError << " | "
               << stats.maxError << " | "
               << (stats.validTests > 0 ? (100 * stats.passedTests / stats.validTests) : 0) << "% |\n";
    }

    report << "\n## Conclusions\n\n";
    report << "### Engine Rankings (Best to Worst)\n\n";

    std::vector<EngineStatistics> ranked = allStats;
    std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
        return a.meanError < b.meanError;
    });

    int rank = 1;
    for (const auto& stats : ranked) {
        report << rank++ << ". **Engine " << stats.engineId << " - " << stats.engineName << "**: "
               << stats.meanError << " cents (" << stats.qualityRating << ")\n";
    }

    report << "\n---\n";
    report << "*Report generated using 6-algorithm consensus method with 95% confidence intervals*\n";

    report.close();

    std::cout << "\nScientific report saved to: " << filename << std::endl;
}

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          SCIENTIFIC PITCH ACCURACY ANALYSIS - MULTI-ALGORITHM             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Methodology: 6-Algorithm Consensus\n";
    std::cout << "  1. YIN Autocorrelation\n";
    std::cout << "  2. Cepstrum Analysis\n";
    std::cout << "  3. FFT Peak Detection\n";
    std::cout << "  4. Zero-Crossing Rate\n";
    std::cout << "  5. Harmonic Product Spectrum\n";
    std::cout << "  6. AMDF\n\n";

    std::cout << "Test Matrix:\n";
    std::cout << "  Engines: " << PITCH_ENGINES.size() << "\n";
    std::cout << "  Frequencies: " << TEST_FREQUENCIES.size() << "\n";
    std::cout << "  Shifts: " << SEMITONE_SHIFTS.size() << "\n";
    std::cout << "  Total tests: " << (PITCH_ENGINES.size() * TEST_FREQUENCIES.size() * SEMITONE_SHIFTS.size()) << "\n\n";

    std::vector<PitchTestResult> allResults;

    int totalTests = PITCH_ENGINES.size() * TEST_FREQUENCIES.size() * SEMITONE_SHIFTS.size();
    int currentTest = 0;

    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "RUNNING TESTS\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n\n";

    for (const auto& [engineId, engineName] : PITCH_ENGINES) {
        std::cout << "Testing Engine " << engineId << " - " << engineName << "...\n";

        for (float freq : TEST_FREQUENCIES) {
            for (int shift : SEMITONE_SHIFTS) {
                currentTest++;

                if (currentTest % 10 == 0 || currentTest == totalTests) {
                    std::cout << "  Progress: " << currentTest << "/" << totalTests
                              << " (" << (100 * currentTest / totalTests) << "%)    \r" << std::flush;
                }

                PitchTestResult result = testPitchConfiguration(engineId, engineName, freq, shift);
                allResults.push_back(result);
            }
        }
        std::cout << "  Progress: " << currentTest << "/" << totalTests << " (100%)    " << std::endl;
    }

    std::cout << "\n═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "CALCULATING STATISTICS\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n\n";

    std::vector<EngineStatistics> allStats;

    for (const auto& [engineId, engineName] : PITCH_ENGINES) {
        std::vector<PitchTestResult> engineResults;
        for (const auto& r : allResults) {
            if (r.engineId == engineId) {
                engineResults.push_back(r);
            }
        }

        EngineStatistics stats = calculateStatistics(engineResults);
        allStats.push_back(stats);

        std::cout << "Engine " << engineId << " - " << engineName << ":\n";
        std::cout << "  Quality: " << stats.qualityRating << "\n";
        std::cout << "  Mean: " << std::fixed << std::setprecision(2) << stats.meanError << " ± " << stats.stdDeviation << " cents\n";
        std::cout << "  95% CI: [" << stats.confidence95Low << ", " << stats.confidence95High << "] cents\n";
        std::cout << "  Range: [" << stats.minError << ", " << stats.maxError << "] cents\n";
        std::cout << "  Pass rate: " << (stats.validTests > 0 ? (100 * stats.passedTests / stats.validTests) : 0) << "%\n\n";
    }

    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "GENERATING REPORT\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n\n";

    generateScientificReport(allResults, allStats, "PITCH_ACCURACY_SCIENTIFIC_ANALYSIS.md");

    std::ofstream csv("build/pitch_scientific_results.csv");
    csv << "EngineID,EngineName,InputFreq,SemitoneShift,ExpectedFreq,MeasuredFreq,CentError,"
        << "YIN,Cepstrum,FFT,ZeroCrossing,HPS,AMDF,Valid,Pass,Notes\n";

    for (const auto& r : allResults) {
        csv << r.engineId << ","
            << "\"" << r.engineName << "\","
            << r.inputFreq << ","
            << r.semitoneShift << ","
            << r.expectedFreq << ","
            << r.measurement.consensus << ","
            << r.centError << ","
            << r.measurement.yin << ","
            << r.measurement.cepstrum << ","
            << r.measurement.fft << ","
            << r.measurement.zeroCrossing << ","
            << r.measurement.hps << ","
            << r.measurement.amdf << ","
            << (r.measurement.valid ? "YES" : "NO") << ","
            << (r.pass ? "PASS" : "FAIL") << ","
            << "\"" << r.notes << "\"\n";
    }
    csv.close();

    std::cout << "\n╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                           ANALYSIS COMPLETE                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Reports generated:\n";
    std::cout << "  - PITCH_ACCURACY_SCIENTIFIC_ANALYSIS.md (Full scientific report)\n";
    std::cout << "  - build/pitch_scientific_results.csv (Raw data)\n\n";

    int totalPassed = 0;
    int totalValid = 0;
    for (const auto& r : allResults) {
        if (r.measurement.valid) {
            totalValid++;
            if (r.pass) totalPassed++;
        }
    }

    float overallPassRate = totalValid > 0 ? (100.0f * totalPassed / totalValid) : 0.0f;
    std::cout << "Overall pass rate: " << std::fixed << std::setprecision(1) << overallPassRate << "%\n\n";

    return overallPassRate >= 70.0f ? 0 : 1;
}

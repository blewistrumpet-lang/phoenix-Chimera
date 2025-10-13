#include "JuceHeader.h"
#include "PitchEngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <fstream>
#include <map>
#include <numeric>
#include <algorithm>
#include <complex>

//==============================================================================
// PROFESSIONAL AUDIO QUALITY ANALYSIS SUITE FOR PITCH ENGINES
// Comprehensive analysis using industry-standard metrics
//==============================================================================

// Constants
const float SAMPLE_RATE = 48000.0f;
const int BLOCK_SIZE = 512;
const float PI = 3.14159265358979323846f;

// Professional Quality Standards
const float THD_PROFESSIONAL_PITCH = 0.05f;       // 5% for pitch shifters
const float THD_FORMANT_PRESERVING = 0.01f;       // 1% for formant-preserving
const float SNR_PROFESSIONAL = 80.0f;             // 80 dB minimum
const float TRANSIENT_SMEARING_MAX = 5.0f;        // 5ms maximum
const float FORMANT_PRESERVATION_MAX = 50.0f;     // 50 Hz maximum shift

// Test frequencies
const std::vector<float> TEST_FREQUENCIES = {110.0f, 220.0f, 440.0f, 880.0f};
const std::vector<int> SEMITONE_SHIFTS = {-12, -7, -5, 0, +5, +7, +12};

// Pitch engine mapping
const std::map<int, std::string> PITCH_ENGINES = {
    {32, "Pitch Shifter"},
    {33, "Intelligent Harmonizer"},
    {34, "Tape Echo"},
    {35, "Digital Delay"},
    {36, "Magnetic Drum Echo"},
    {37, "Bucket Brigade Delay"},
    {38, "Buffer Repeat Platinum"},
    {49, "Pitch Shifter Alt"}
};

//==============================================================================
// Structure Definitions
//==============================================================================

struct THDNAnalysis {
    float thd_n;          // Total Harmonic Distortion + Noise
    float h2, h3, h5, h7; // Individual harmonics (dB)
    float evenOddRatio;   // Ratio of even to odd harmonics
    float noiseFloor_db;  // Noise floor in dB
    float snr_db;         // Signal-to-Noise Ratio
    std::vector<float> unwantedHarmonics; // Non-multiples of fundamental
};

struct SpectralAnalysis {
    float spectralFlatness;    // 0=tonal, 1=noisy
    float spectralCentroid;    // Brightness measure (Hz)
    float spectralRolloff;     // 85% energy point (Hz)
    float spectralSpread;      // Spectral bandwidth
    std::vector<float> spectrum; // Full spectrum for visualization
    bool hasSmearing;          // Time-frequency artifacts detected
    bool hasGraininess;        // Grain boundary artifacts
};

struct ArtifactAnalysis {
    float graininess_db;       // Grain boundary artifacts
    float phasiness_score;     // Phase coherence (0-1, 1=perfect)
    float metallic_db;         // High-frequency artifacts
    float preRinging_db;       // Pre-echo artifacts
    bool hasMetallicSound;     // Boolean flag
    bool hasPhasiness;         // Boolean flag
    bool hasGrains;            // Boolean flag
    bool hasPreRinging;        // Boolean flag
};

struct TransientAnalysis {
    float attackTime_ms;       // Attack time in milliseconds
    float transientSmearing;   // Smearing amount (ms)
    float envelopeCorrelation; // Envelope preservation (0-1)
    bool preservedTransients;  // Within 5ms threshold
};

struct FormantAnalysis {
    float f1, f2, f3;          // Formant frequencies (Hz)
    float f1_shift, f2_shift, f3_shift; // Shift amounts (Hz)
    float maxShift;            // Maximum formant shift
    bool preservedFormants;    // Within 50 Hz threshold
};

struct NaturalnessScore {
    float spectralFlatness;    // Component scores
    float spectralCentroid_normalized;
    float spectralRolloff_normalized;
    float harmonicBalance;
    float overallScore;        // 0-100 composite
    std::string rating;        // Verbal rating
};

struct QualityGrade {
    char overall_grade;        // A-F
    char thd_grade;
    char snr_grade;
    char artifact_grade;
    char transient_grade;
    char formant_grade;
    char naturalness_grade;
    bool meetsProStandards;
    std::vector<std::string> issues;
    std::vector<std::string> strengths;
};

struct ComprehensiveQualityReport {
    int engineId;
    std::string engineName;
    float testFrequency;
    int semitoneShift;

    // All analysis results
    THDNAnalysis thdnAnalysis;
    SpectralAnalysis spectralAnalysis;
    ArtifactAnalysis artifactAnalysis;
    TransientAnalysis transientAnalysis;
    FormantAnalysis formantAnalysis;
    NaturalnessScore naturalness;
    QualityGrade grade;

    bool validTest;
    std::string errorMsg;
};

//==============================================================================
// FFT Utilities
//==============================================================================

std::vector<float> performFFT(const juce::AudioBuffer<float>& buffer, int fftSize = 16384) {
    int order = (int)std::log2(fftSize);
    juce::dsp::FFT fft(order);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    int samples = std::min(buffer.getNumSamples(), fftSize);

    // Apply Hann window
    for (int i = 0; i < samples; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * PI * i / samples));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    std::vector<float> magnitude(fftSize / 2);
    for (int i = 0; i < fftSize / 2; ++i) {
        magnitude[i] = fftData[i];
    }

    return magnitude;
}

//==============================================================================
// 1. THD+N Measurement
//==============================================================================

THDNAnalysis measureTHDN(const juce::AudioBuffer<float>& buffer, float fundamentalFreq) {
    THDNAnalysis result;
    result.thd_n = 0.0f;
    result.h2 = result.h3 = result.h5 = result.h7 = -120.0f;
    result.evenOddRatio = 0.0f;
    result.noiseFloor_db = -120.0f;
    result.snr_db = 0.0f;

    const int fftSize = 16384;
    if (buffer.getNumSamples() < fftSize) return result;

    std::vector<float> spectrum = performFFT(buffer, fftSize);

    // Find fundamental and harmonic bins
    auto getBinMagnitude = [&](float freq) -> float {
        int bin = (int)(freq * fftSize / SAMPLE_RATE);
        if (bin < 0 || bin >= (int)spectrum.size()) return 0.0f;

        // Use 5-bin window for better accuracy
        float sum = 0.0f;
        for (int i = -2; i <= 2; ++i) {
            int b = bin + i;
            if (b >= 0 && b < (int)spectrum.size()) {
                sum += spectrum[b];
            }
        }
        return sum;
    };

    float fundMag = getBinMagnitude(fundamentalFreq);
    float h2Mag = getBinMagnitude(fundamentalFreq * 2.0f);
    float h3Mag = getBinMagnitude(fundamentalFreq * 3.0f);
    float h5Mag = getBinMagnitude(fundamentalFreq * 5.0f);
    float h7Mag = getBinMagnitude(fundamentalFreq * 7.0f);

    if (fundMag > 1e-8f) {
        // Calculate THD+N (includes all harmonics + noise)
        float harmonicPower = 0.0f;
        for (int h = 2; h <= 10; ++h) {
            float hMag = getBinMagnitude(fundamentalFreq * h);
            harmonicPower += hMag * hMag;
        }

        result.thd_n = std::sqrt(harmonicPower) / fundMag;

        // Individual harmonics in dB
        result.h2 = 20.0f * std::log10(std::max(h2Mag / fundMag, 1e-10f));
        result.h3 = 20.0f * std::log10(std::max(h3Mag / fundMag, 1e-10f));
        result.h5 = 20.0f * std::log10(std::max(h5Mag / fundMag, 1e-10f));
        result.h7 = 20.0f * std::log10(std::max(h7Mag / fundMag, 1e-10f));

        // Even/Odd harmonic ratio
        float evenHarmonics = h2Mag;
        float oddHarmonics = h3Mag + h5Mag + h7Mag;
        if (oddHarmonics > 1e-8f) {
            result.evenOddRatio = evenHarmonics / oddHarmonics;
        }

        // Noise floor (average of non-harmonic bins)
        float noiseSum = 0.0f;
        int noiseCount = 0;
        for (int i = 10; i < (int)spectrum.size() / 4; ++i) {
            float binFreq = i * SAMPLE_RATE / fftSize;
            bool isHarmonic = false;
            for (int h = 1; h <= 10; ++h) {
                if (std::abs(binFreq - fundamentalFreq * h) < 20.0f) {
                    isHarmonic = true;
                    break;
                }
            }
            if (!isHarmonic) {
                noiseSum += spectrum[i];
                noiseCount++;
            }
        }

        if (noiseCount > 0) {
            float avgNoise = noiseSum / noiseCount;
            result.noiseFloor_db = 20.0f * std::log10(std::max(avgNoise / fundMag, 1e-10f));
            result.snr_db = -result.noiseFloor_db;
        }

        // Detect unwanted harmonics (should only be integer multiples)
        for (int i = 10; i < (int)spectrum.size() / 4; ++i) {
            float binFreq = i * SAMPLE_RATE / fftSize;
            if (spectrum[i] > fundMag * 0.01f) { // 1% of fundamental
                bool isExpected = false;
                for (int h = 1; h <= 10; ++h) {
                    if (std::abs(binFreq - fundamentalFreq * h) < 10.0f) {
                        isExpected = true;
                        break;
                    }
                }
                if (!isExpected) {
                    result.unwantedHarmonics.push_back(binFreq);
                }
            }
        }
    }

    return result;
}

//==============================================================================
// 2. Spectral Analysis
//==============================================================================

SpectralAnalysis analyzeSpectrum(const juce::AudioBuffer<float>& buffer) {
    SpectralAnalysis result;
    result.spectralFlatness = 0.0f;
    result.spectralCentroid = 0.0f;
    result.spectralRolloff = 0.0f;
    result.spectralSpread = 0.0f;
    result.hasSmearing = false;
    result.hasGraininess = false;

    const int fftSize = 8192;
    result.spectrum = performFFT(buffer, fftSize);

    // Calculate spectral flatness (geometric mean / arithmetic mean)
    float geometricMean = 0.0f;
    float arithmeticMean = 0.0f;
    int validBins = 0;

    for (int i = 5; i < (int)result.spectrum.size() / 2; ++i) {
        if (result.spectrum[i] > 1e-8f) {
            geometricMean += std::log(result.spectrum[i]);
            arithmeticMean += result.spectrum[i];
            validBins++;
        }
    }

    if (validBins > 0) {
        geometricMean = std::exp(geometricMean / validBins);
        arithmeticMean /= validBins;
        result.spectralFlatness = geometricMean / (arithmeticMean + 1e-8f);
    }

    // Calculate spectral centroid (brightness)
    float sumWeighted = 0.0f;
    float sumMag = 0.0f;

    for (int i = 0; i < (int)result.spectrum.size(); ++i) {
        float freq = i * SAMPLE_RATE / fftSize;
        sumWeighted += freq * result.spectrum[i];
        sumMag += result.spectrum[i];
    }

    if (sumMag > 1e-8f) {
        result.spectralCentroid = sumWeighted / sumMag;
    }

    // Calculate spectral rolloff (85% energy point)
    float totalEnergy = 0.0f;
    for (float mag : result.spectrum) {
        totalEnergy += mag * mag;
    }

    float cumulativeEnergy = 0.0f;
    for (int i = 0; i < (int)result.spectrum.size(); ++i) {
        cumulativeEnergy += result.spectrum[i] * result.spectrum[i];
        if (cumulativeEnergy >= 0.85f * totalEnergy) {
            result.spectralRolloff = i * SAMPLE_RATE / fftSize;
            break;
        }
    }

    // Calculate spectral spread (standard deviation around centroid)
    float spreadSum = 0.0f;
    for (int i = 0; i < (int)result.spectrum.size(); ++i) {
        float freq = i * SAMPLE_RATE / fftSize;
        float diff = freq - result.spectralCentroid;
        spreadSum += diff * diff * result.spectrum[i];
    }
    result.spectralSpread = std::sqrt(spreadSum / (sumMag + 1e-8f));

    // Detect smearing (high spectral spread indicates smearing)
    result.hasSmearing = (result.spectralSpread > 3000.0f);

    // Detect graininess (peaks in high-frequency noise)
    float hfAvg = 0.0f;
    int hfCount = 0;
    for (int i = (int)result.spectrum.size() / 2; i < (int)result.spectrum.size() * 3 / 4; ++i) {
        hfAvg += result.spectrum[i];
        hfCount++;
    }
    hfAvg /= (hfCount + 1);

    // Check for spiky high-frequency content (graininess)
    int spikes = 0;
    for (int i = (int)result.spectrum.size() / 2; i < (int)result.spectrum.size() * 3 / 4; ++i) {
        if (result.spectrum[i] > hfAvg * 3.0f) {
            spikes++;
        }
    }
    result.hasGraininess = (spikes > hfCount / 10);

    return result;
}

//==============================================================================
// 3. Artifact Detection
//==============================================================================

ArtifactAnalysis detectArtifacts(const juce::AudioBuffer<float>& buffer, const SpectralAnalysis& spectral) {
    ArtifactAnalysis result;
    result.graininess_db = -120.0f;
    result.phasiness_score = 1.0f;
    result.metallic_db = -120.0f;
    result.preRinging_db = -120.0f;
    result.hasMetallicSound = false;
    result.hasPhasiness = false;
    result.hasGrains = spectral.hasGraininess;
    result.hasPreRinging = false;

    // Metallic sound detection (excessive high-frequency content)
    float hfEnergy = 0.0f;
    float lfEnergy = 0.0f;

    for (int i = 0; i < (int)spectral.spectrum.size() / 4; ++i) {
        lfEnergy += spectral.spectrum[i] * spectral.spectrum[i];
    }

    for (int i = (int)spectral.spectrum.size() / 2; i < (int)spectral.spectrum.size() * 3 / 4; ++i) {
        hfEnergy += spectral.spectrum[i] * spectral.spectrum[i];
    }

    if (lfEnergy > 1e-8f) {
        result.metallic_db = 10.0f * std::log10((hfEnergy / lfEnergy) + 1e-10f);
        result.hasMetallicSound = (result.metallic_db > -20.0f); // HF within 20dB of LF
    }

    // Phase coherence measurement (stereo correlation)
    if (buffer.getNumChannels() >= 2) {
        const float* left = buffer.getReadPointer(0);
        const float* right = buffer.getReadPointer(1);
        int numSamples = buffer.getNumSamples();

        float correlation = 0.0f;
        float leftEnergy = 0.0f;
        float rightEnergy = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            correlation += left[i] * right[i];
            leftEnergy += left[i] * left[i];
            rightEnergy += right[i] * right[i];
        }

        float denominator = std::sqrt(leftEnergy * rightEnergy + 1e-10f);
        if (denominator > 1e-8f) {
            result.phasiness_score = correlation / denominator;
            result.hasPhasiness = (result.phasiness_score < 0.7f); // Less than 70% correlation
        }
    }

    // Pre-ringing detection (energy before main transient)
    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    // Find main transient
    int peakIdx = 0;
    float peakVal = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        if (std::abs(data[i]) > peakVal) {
            peakVal = std::abs(data[i]);
            peakIdx = i;
        }
    }

    // Measure energy before peak
    if (peakIdx > 1000) {
        float preEnergy = 0.0f;
        for (int i = std::max(0, peakIdx - 1000); i < peakIdx; ++i) {
            preEnergy += data[i] * data[i];
        }

        if (peakVal > 1e-6f) {
            result.preRinging_db = 10.0f * std::log10((preEnergy / 1000.0f) / (peakVal * peakVal) + 1e-10f);
            result.hasPreRinging = (result.preRinging_db > -40.0f); // Pre-ringing within 40dB of peak
        }
    }

    // Graininess measurement (from spectral analysis)
    if (spectral.hasGraininess) {
        result.graininess_db = -30.0f; // Arbitrary value indicating presence
    }

    return result;
}

//==============================================================================
// 4. Transient Preservation
//==============================================================================

TransientAnalysis analyzeTransients(const juce::AudioBuffer<float>& inputBuffer,
                                     const juce::AudioBuffer<float>& outputBuffer) {
    TransientAnalysis result;
    result.attackTime_ms = 0.0f;
    result.transientSmearing = 0.0f;
    result.envelopeCorrelation = 0.0f;
    result.preservedTransients = false;

    int numSamples = std::min(inputBuffer.getNumSamples(), outputBuffer.getNumSamples());

    // Calculate envelope for both signals
    std::vector<float> inputEnv(numSamples);
    std::vector<float> outputEnv(numSamples);

    const float* inputData = inputBuffer.getReadPointer(0);
    const float* outputData = outputBuffer.getReadPointer(0);

    float inputRMS = 0.0f;
    float outputRMS = 0.0f;

    const int windowSize = 256;
    for (int i = 0; i < numSamples; ++i) {
        float inputSum = 0.0f;
        float outputSum = 0.0f;
        int count = 0;

        for (int j = std::max(0, i - windowSize / 2); j < std::min(numSamples, i + windowSize / 2); ++j) {
            inputSum += std::abs(inputData[j]);
            outputSum += std::abs(outputData[j]);
            count++;
        }

        inputEnv[i] = inputSum / count;
        outputEnv[i] = outputSum / count;

        inputRMS += inputData[i] * inputData[i];
        outputRMS += outputData[i] * outputData[i];
    }

    inputRMS = std::sqrt(inputRMS / numSamples);
    outputRMS = std::sqrt(outputRMS / numSamples);

    // Find attack time in input
    float threshold = inputRMS * 0.1f;
    float peak = inputRMS * 0.9f;

    int attackStart = -1;
    int attackEnd = -1;

    for (int i = 0; i < (int)inputEnv.size(); ++i) {
        if (attackStart < 0 && inputEnv[i] > threshold) {
            attackStart = i;
        }
        if (attackStart >= 0 && attackEnd < 0 && inputEnv[i] > peak) {
            attackEnd = i;
            break;
        }
    }

    if (attackStart >= 0 && attackEnd >= 0) {
        result.attackTime_ms = (attackEnd - attackStart) * 1000.0f / SAMPLE_RATE;

        // Find corresponding attack in output
        int outputAttackStart = -1;
        int outputAttackEnd = -1;

        threshold = outputRMS * 0.1f;
        peak = outputRMS * 0.9f;

        for (int i = 0; i < (int)outputEnv.size(); ++i) {
            if (outputAttackStart < 0 && outputEnv[i] > threshold) {
                outputAttackStart = i;
            }
            if (outputAttackStart >= 0 && outputAttackEnd < 0 && outputEnv[i] > peak) {
                outputAttackEnd = i;
                break;
            }
        }

        if (outputAttackStart >= 0 && outputAttackEnd >= 0) {
            float outputAttackTime = (outputAttackEnd - outputAttackStart) * 1000.0f / SAMPLE_RATE;
            result.transientSmearing = std::abs(outputAttackTime - result.attackTime_ms);
            result.preservedTransients = (result.transientSmearing < TRANSIENT_SMEARING_MAX);
        }
    }

    // Envelope correlation
    float correlation = 0.0f;
    float inputEnergy = 0.0f;
    float outputEnergy = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        correlation += inputEnv[i] * outputEnv[i];
        inputEnergy += inputEnv[i] * inputEnv[i];
        outputEnergy += outputEnv[i] * outputEnv[i];
    }

    float denominator = std::sqrt(inputEnergy * outputEnergy + 1e-10f);
    if (denominator > 1e-8f) {
        result.envelopeCorrelation = correlation / denominator;
    }

    return result;
}

//==============================================================================
// 5. Formant Preservation (for vocal engines)
//==============================================================================

FormantAnalysis analyzeFormantPreservation(const juce::AudioBuffer<float>& inputBuffer,
                                            const juce::AudioBuffer<float>& outputBuffer) {
    FormantAnalysis result;
    result.f1 = result.f2 = result.f3 = 0.0f;
    result.f1_shift = result.f2_shift = result.f3_shift = 0.0f;
    result.maxShift = 0.0f;
    result.preservedFormants = true;

    const int fftSize = 8192;
    std::vector<float> inputSpectrum = performFFT(inputBuffer, fftSize);
    std::vector<float> outputSpectrum = performFFT(outputBuffer, fftSize);

    // Apply smoothing to find formant peaks
    auto smoothSpectrum = [](const std::vector<float>& spec) -> std::vector<float> {
        std::vector<float> smoothed(spec.size(), 0.0f);
        const int smoothWidth = 10;
        for (int i = smoothWidth; i < (int)spec.size() - smoothWidth; ++i) {
            float sum = 0.0f;
            for (int j = -smoothWidth; j <= smoothWidth; ++j) {
                sum += spec[i + j];
            }
            smoothed[i] = sum / (2 * smoothWidth + 1);
        }
        return smoothed;
    };

    std::vector<float> inputSmooth = smoothSpectrum(inputSpectrum);
    std::vector<float> outputSmooth = smoothSpectrum(outputSpectrum);

    // Find formant peaks in typical ranges
    // F1: 200-1000 Hz, F2: 800-2500 Hz, F3: 1500-3500 Hz
    auto findPeakInRange = [&](const std::vector<float>& spec, float minFreq, float maxFreq) -> float {
        int minBin = (int)(minFreq * fftSize / SAMPLE_RATE);
        int maxBin = (int)(maxFreq * fftSize / SAMPLE_RATE);

        int peakBin = minBin;
        float peakVal = 0.0f;

        for (int i = minBin; i < maxBin && i < (int)spec.size(); ++i) {
            if (spec[i] > peakVal) {
                peakVal = spec[i];
                peakBin = i;
            }
        }

        return peakBin * SAMPLE_RATE / fftSize;
    };

    float inputF1 = findPeakInRange(inputSmooth, 200.0f, 1000.0f);
    float inputF2 = findPeakInRange(inputSmooth, 800.0f, 2500.0f);
    float inputF3 = findPeakInRange(inputSmooth, 1500.0f, 3500.0f);

    float outputF1 = findPeakInRange(outputSmooth, 200.0f, 1000.0f);
    float outputF2 = findPeakInRange(outputSmooth, 800.0f, 2500.0f);
    float outputF3 = findPeakInRange(outputSmooth, 1500.0f, 3500.0f);

    result.f1 = outputF1;
    result.f2 = outputF2;
    result.f3 = outputF3;

    result.f1_shift = std::abs(outputF1 - inputF1);
    result.f2_shift = std::abs(outputF2 - inputF2);
    result.f3_shift = std::abs(outputF3 - inputF3);

    result.maxShift = std::max({result.f1_shift, result.f2_shift, result.f3_shift});
    result.preservedFormants = (result.maxShift < FORMANT_PRESERVATION_MAX);

    return result;
}

//==============================================================================
// 6. Naturalness Score
//==============================================================================

NaturalnessScore calculateNaturalness(const SpectralAnalysis& spectral, const THDNAnalysis& thdn) {
    NaturalnessScore result;

    // Component 1: Spectral flatness (0=tonal, 1=noisy)
    // For natural audio, we want low flatness (tonal)
    result.spectralFlatness = (1.0f - spectral.spectralFlatness) * 100.0f;

    // Component 2: Spectral centroid (brightness)
    // Normalize to 0-100 (ideal range: 1000-4000 Hz)
    float centroidScore = 0.0f;
    if (spectral.spectralCentroid >= 1000.0f && spectral.spectralCentroid <= 4000.0f) {
        centroidScore = 100.0f;
    } else if (spectral.spectralCentroid < 1000.0f) {
        centroidScore = spectral.spectralCentroid / 1000.0f * 100.0f;
    } else {
        centroidScore = std::max(0.0f, 100.0f - (spectral.spectralCentroid - 4000.0f) / 100.0f);
    }
    result.spectralCentroid_normalized = centroidScore;

    // Component 3: Spectral rolloff
    // Normalize to 0-100 (ideal range: 8000-16000 Hz)
    float rolloffScore = 0.0f;
    if (spectral.spectralRolloff >= 8000.0f && spectral.spectralRolloff <= 16000.0f) {
        rolloffScore = 100.0f;
    } else if (spectral.spectralRolloff < 8000.0f) {
        rolloffScore = spectral.spectralRolloff / 8000.0f * 100.0f;
    } else {
        rolloffScore = std::max(0.0f, 100.0f - (spectral.spectralRolloff - 16000.0f) / 200.0f);
    }
    result.spectralRolloff_normalized = rolloffScore;

    // Component 4: Harmonic balance
    // Natural sound has balanced harmonics
    float harmonicScore = 100.0f;
    if (thdn.evenOddRatio < 0.5f || thdn.evenOddRatio > 2.0f) {
        harmonicScore = 50.0f; // Imbalanced
    }
    result.harmonicBalance = harmonicScore;

    // Overall score (weighted average)
    result.overallScore = (result.spectralFlatness * 0.25f +
                          result.spectralCentroid_normalized * 0.25f +
                          result.spectralRolloff_normalized * 0.25f +
                          result.harmonicBalance * 0.25f);

    // Rating
    if (result.overallScore >= 90.0f) result.rating = "Excellent";
    else if (result.overallScore >= 75.0f) result.rating = "Good";
    else if (result.overallScore >= 60.0f) result.rating = "Acceptable";
    else if (result.overallScore >= 40.0f) result.rating = "Fair";
    else result.rating = "Poor";

    return result;
}

//==============================================================================
// 7. Quality Grading
//==============================================================================

QualityGrade calculateQualityGrade(const ComprehensiveQualityReport& report) {
    QualityGrade grade;
    grade.meetsProStandards = true;

    // THD Grade
    float thd_percent = report.thdnAnalysis.thd_n * 100.0f;
    if (thd_percent < 1.0f) grade.thd_grade = 'A';
    else if (thd_percent < 3.0f) grade.thd_grade = 'B';
    else if (thd_percent < 5.0f) grade.thd_grade = 'C';
    else if (thd_percent < 10.0f) grade.thd_grade = 'D';
    else grade.thd_grade = 'F';

    if (thd_percent >= 5.0f) {
        grade.meetsProStandards = false;
        grade.issues.push_back("THD exceeds 5% professional threshold");
    } else {
        grade.strengths.push_back("THD within professional limits");
    }

    // SNR Grade
    if (report.thdnAnalysis.snr_db >= 96.0f) grade.snr_grade = 'A';
    else if (report.thdnAnalysis.snr_db >= 90.0f) grade.snr_grade = 'B';
    else if (report.thdnAnalysis.snr_db >= 80.0f) grade.snr_grade = 'C';
    else if (report.thdnAnalysis.snr_db >= 70.0f) grade.snr_grade = 'D';
    else grade.snr_grade = 'F';

    if (report.thdnAnalysis.snr_db < 80.0f) {
        grade.meetsProStandards = false;
        grade.issues.push_back("SNR below 80dB professional threshold");
    } else {
        grade.strengths.push_back("SNR meets professional standards");
    }

    // Artifact Grade
    int artifactCount = 0;
    if (report.artifactAnalysis.hasMetallicSound) artifactCount++;
    if (report.artifactAnalysis.hasPhasiness) artifactCount++;
    if (report.artifactAnalysis.hasGrains) artifactCount++;
    if (report.artifactAnalysis.hasPreRinging) artifactCount++;

    if (artifactCount == 0) grade.artifact_grade = 'A';
    else if (artifactCount == 1) grade.artifact_grade = 'B';
    else if (artifactCount == 2) grade.artifact_grade = 'C';
    else if (artifactCount == 3) grade.artifact_grade = 'D';
    else grade.artifact_grade = 'F';

    if (artifactCount >= 2) {
        grade.issues.push_back("Multiple audible artifacts detected");
    } else if (artifactCount == 0) {
        grade.strengths.push_back("No significant artifacts detected");
    }

    // Transient Grade
    if (report.transientAnalysis.transientSmearing < 2.0f) grade.transient_grade = 'A';
    else if (report.transientAnalysis.transientSmearing < 3.5f) grade.transient_grade = 'B';
    else if (report.transientAnalysis.transientSmearing < 5.0f) grade.transient_grade = 'C';
    else if (report.transientAnalysis.transientSmearing < 10.0f) grade.transient_grade = 'D';
    else grade.transient_grade = 'F';

    if (!report.transientAnalysis.preservedTransients) {
        grade.meetsProStandards = false;
        grade.issues.push_back("Transient smearing exceeds 5ms threshold");
    } else {
        grade.strengths.push_back("Transients preserved well");
    }

    // Formant Grade
    if (report.formantAnalysis.maxShift < 20.0f) grade.formant_grade = 'A';
    else if (report.formantAnalysis.maxShift < 35.0f) grade.formant_grade = 'B';
    else if (report.formantAnalysis.maxShift < 50.0f) grade.formant_grade = 'C';
    else if (report.formantAnalysis.maxShift < 75.0f) grade.formant_grade = 'D';
    else grade.formant_grade = 'F';

    if (!report.formantAnalysis.preservedFormants) {
        grade.issues.push_back("Formant shift exceeds 50Hz threshold");
    } else {
        grade.strengths.push_back("Formants preserved");
    }

    // Naturalness Grade
    if (report.naturalness.overallScore >= 90.0f) grade.naturalness_grade = 'A';
    else if (report.naturalness.overallScore >= 75.0f) grade.naturalness_grade = 'B';
    else if (report.naturalness.overallScore >= 60.0f) grade.naturalness_grade = 'C';
    else if (report.naturalness.overallScore >= 40.0f) grade.naturalness_grade = 'D';
    else grade.naturalness_grade = 'F';

    // Overall Grade (worst of all grades)
    std::vector<char> grades = {
        grade.thd_grade, grade.snr_grade, grade.artifact_grade,
        grade.transient_grade, grade.formant_grade, grade.naturalness_grade
    };
    grade.overall_grade = *std::max_element(grades.begin(), grades.end()); // Highest letter = worst grade

    return grade;
}

//==============================================================================
// 8. Run Comprehensive Test
//==============================================================================

ComprehensiveQualityReport runComprehensiveQualityTest(int engineId, float testFreq, int semitoneShift) {
    ComprehensiveQualityReport report;
    report.engineId = engineId;
    report.engineName = PITCH_ENGINES.count(engineId) ? PITCH_ENGINES.at(engineId) : "Unknown";
    report.testFrequency = testFreq;
    report.semitoneShift = semitoneShift;
    report.validTest = false;
    report.errorMsg = "";

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            report.errorMsg = "Failed to create engine";
            return report;
        }

        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);

        // Set parameters
        std::map<int, float> params;
        float normalizedShift = (semitoneShift + 12.0f) / 24.0f;
        normalizedShift = std::max(0.0f, std::min(1.0f, normalizedShift));
        params[0] = normalizedShift;

        if (engine->getNumParameters() > 1) {
            params[1] = 1.0f; // 100% wet
        }
        for (int i = 2; i < engine->getNumParameters(); ++i) {
            params[i] = 0.5f;
        }

        engine->reset();
        engine->updateParameters(params);

        // Generate test signal
        const int testLength = 65536;
        juce::AudioBuffer<float> inputBuffer(2, testLength);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * PI * testFreq * i / SAMPLE_RATE;
                inputBuffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        // Process
        juce::AudioBuffer<float> outputBuffer(2, testLength);
        for (int ch = 0; ch < 2; ++ch) {
            outputBuffer.copyFrom(ch, 0, inputBuffer, ch, 0, testLength);
        }

        for (int start = 0; start < testLength; start += BLOCK_SIZE) {
            int samplesThisBlock = std::min(BLOCK_SIZE, testLength - start);
            juce::AudioBuffer<float> block(outputBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Skip transients for analysis
        int skipSamples = testLength / 5;
        int analysisSamples = testLength - skipSamples;

        juce::AudioBuffer<float> analysisBuffer(2, analysisSamples);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < analysisSamples; ++i) {
                analysisBuffer.setSample(ch, i, outputBuffer.getSample(ch, i + skipSamples));
            }
        }

        // Calculate expected frequency
        float expectedFreq = testFreq * std::pow(2.0f, semitoneShift / 12.0f);

        // Perform all analyses
        report.thdnAnalysis = measureTHDN(analysisBuffer, expectedFreq);
        report.spectralAnalysis = analyzeSpectrum(analysisBuffer);
        report.artifactAnalysis = detectArtifacts(analysisBuffer, report.spectralAnalysis);
        report.transientAnalysis = analyzeTransients(inputBuffer, outputBuffer);
        report.formantAnalysis = analyzeFormantPreservation(inputBuffer, outputBuffer);
        report.naturalness = calculateNaturalness(report.spectralAnalysis, report.thdnAnalysis);
        report.grade = calculateQualityGrade(report);

        report.validTest = true;

    } catch (const std::exception& e) {
        report.errorMsg = std::string("Exception: ") + e.what();
    } catch (...) {
        report.errorMsg = "Unknown exception";
    }

    return report;
}

//==============================================================================
// 9. Generate Comprehensive Markdown Report
//==============================================================================

void generateComprehensiveReport(const std::vector<ComprehensiveQualityReport>& allReports) {
    std::ofstream file("PITCH_ENGINE_AUDIO_QUALITY_ANALYSIS.md");
    if (!file.is_open()) {
        std::cerr << "Failed to create report file" << std::endl;
        return;
    }

    file << "# PITCH ENGINE AUDIO QUALITY ANALYSIS\n";
    file << "## Professional-Grade Quality Assessment\n\n";
    file << "**Generated:** " << juce::Time::getCurrentTime().toString(true, true).toStdString() << "\n\n";
    file << "---\n\n";

    file << "## EXECUTIVE SUMMARY\n\n";
    file << "This report provides comprehensive audio quality analysis of all 8 pitch engines using ";
    file << "professional metrics aligned with industry standards (UAD, FabFilter, Waves).\n\n";

    file << "### Professional Quality Standards\n\n";
    file << "| Metric | Professional | Excellent | Acceptable |\n";
    file << "|--------|-------------|-----------|------------|\n";
    file << "| THD+N | < 5% | < 1% | < 10% |\n";
    file << "| SNR | > 80 dB | > 96 dB | > 70 dB |\n";
    file << "| Transient Smearing | < 5 ms | < 2 ms | < 10 ms |\n";
    file << "| Formant Preservation | < 50 Hz | < 20 Hz | < 75 Hz |\n";
    file << "| Artifacts | Minimal | None | Some |\n\n";

    // Summary table
    file << "### Engine Quality Summary\n\n";
    file << "| Engine | Name | Overall Grade | THD | SNR | Artifacts | Transients | Formants | Naturalness | Pro Quality |\n";
    file << "|--------|------|---------------|-----|-----|-----------|------------|----------|-------------|-------------|\n";

    std::map<int, std::vector<ComprehensiveQualityReport>> engineReports;
    for (const auto& r : allReports) {
        if (r.validTest) {
            engineReports[r.engineId].push_back(r);
        }
    }

    for (const auto& [engineId, reports] : engineReports) {
        if (reports.empty()) continue;

        // Average grades
        char avgOverall = 'C';
        char avgTHD = 'C';
        char avgSNR = 'C';
        char avgArtifact = 'C';
        char avgTransient = 'C';
        char avgFormant = 'C';
        char avgNaturalness = 'C';
        bool meetsProStandards = true;

        int gradeSum = 0;
        for (const auto& r : reports) {
            gradeSum += (r.grade.overall_grade - 'A');
            avgTHD = std::max(avgTHD, r.grade.thd_grade);
            avgSNR = std::max(avgSNR, r.grade.snr_grade);
            avgArtifact = std::max(avgArtifact, r.grade.artifact_grade);
            avgTransient = std::max(avgTransient, r.grade.transient_grade);
            avgFormant = std::max(avgFormant, r.grade.formant_grade);
            avgNaturalness = std::max(avgNaturalness, r.grade.naturalness_grade);
            if (!r.grade.meetsProStandards) meetsProStandards = false;
        }
        avgOverall = 'A' + (gradeSum / reports.size());

        file << "| " << engineId << " | " << reports[0].engineName << " | "
             << avgOverall << " | " << avgTHD << " | " << avgSNR << " | "
             << avgArtifact << " | " << avgTransient << " | " << avgFormant << " | "
             << avgNaturalness << " | " << (meetsProStandards ? "YES" : "NO") << " |\n";
    }

    file << "\n---\n\n";

    // Detailed reports for each engine
    for (const auto& [engineId, reports] : engineReports) {
        if (reports.empty()) continue;

        file << "## Engine " << engineId << ": " << reports[0].engineName << "\n\n";

        // Calculate averages
        float avgTHD = 0.0f, avgSNR = 0.0f, avgTransientSmear = 0.0f;
        float avgFormantShift = 0.0f, avgNaturalness = 0.0f;
        int artifactCount = 0;

        for (const auto& r : reports) {
            avgTHD += r.thdnAnalysis.thd_n * 100.0f;
            avgSNR += r.thdnAnalysis.snr_db;
            avgTransientSmear += r.transientAnalysis.transientSmearing;
            avgFormantShift += r.formantAnalysis.maxShift;
            avgNaturalness += r.naturalness.overallScore;
            if (r.artifactAnalysis.hasMetallicSound) artifactCount++;
            if (r.artifactAnalysis.hasPhasiness) artifactCount++;
            if (r.artifactAnalysis.hasGrains) artifactCount++;
            if (r.artifactAnalysis.hasPreRinging) artifactCount++;
        }

        int n = reports.size();
        avgTHD /= n;
        avgSNR /= n;
        avgTransientSmear /= n;
        avgFormantShift /= n;
        avgNaturalness /= n;

        file << "### Performance Summary\n\n";
        file << "| Metric | Average Value | Grade | Status |\n";
        file << "|--------|---------------|-------|--------|\n";
        file << "| THD+N | " << std::fixed << std::setprecision(2) << avgTHD << "% | "
             << reports[0].grade.thd_grade << " | "
             << (avgTHD < 5.0f ? "PASS" : "FAIL") << " |\n";
        file << "| SNR | " << std::setprecision(1) << avgSNR << " dB | "
             << reports[0].grade.snr_grade << " | "
             << (avgSNR >= 80.0f ? "PASS" : "FAIL") << " |\n";
        file << "| Transient Smearing | " << std::setprecision(2) << avgTransientSmear << " ms | "
             << reports[0].grade.transient_grade << " | "
             << (avgTransientSmear < 5.0f ? "PASS" : "FAIL") << " |\n";
        file << "| Formant Shift | " << std::setprecision(1) << avgFormantShift << " Hz | "
             << reports[0].grade.formant_grade << " | "
             << (avgFormantShift < 50.0f ? "PASS" : "FAIL") << " |\n";
        file << "| Naturalness Score | " << std::setprecision(1) << avgNaturalness << " / 100 | "
             << reports[0].grade.naturalness_grade << " | "
             << (avgNaturalness >= 60.0f ? "PASS" : "FAIL") << " |\n";
        file << "| Artifacts Detected | " << artifactCount << " / " << (n * 4) << " | "
             << reports[0].grade.artifact_grade << " | "
             << (artifactCount < n ? "PASS" : "FAIL") << " |\n\n";

        // Strengths and Issues
        if (!reports[0].grade.strengths.empty()) {
            file << "### Strengths\n\n";
            for (const auto& s : reports[0].grade.strengths) {
                file << "- " << s << "\n";
            }
            file << "\n";
        }

        if (!reports[0].grade.issues.empty()) {
            file << "### Issues\n\n";
            for (const auto& i : reports[0].grade.issues) {
                file << "- " << i << "\n";
            }
            file << "\n";
        }

        // Detailed measurements
        file << "### Detailed Measurements\n\n";
        file << "| Freq | Shift | THD% | SNR | H2 | H3 | H5 | H7 | Smear | Formant | Natural |\n";
        file << "|------|-------|------|-----|----|----|----|----|-------|---------|----------|\n";

        for (const auto& r : reports) {
            file << "| " << (int)r.testFrequency << " Hz | "
                 << std::showpos << r.semitoneShift << std::noshowpos << " st | "
                 << std::fixed << std::setprecision(2) << (r.thdnAnalysis.thd_n * 100.0f) << " | "
                 << std::setprecision(1) << r.thdnAnalysis.snr_db << " | "
                 << std::setprecision(1) << r.thdnAnalysis.h2 << " | "
                 << r.thdnAnalysis.h3 << " | "
                 << r.thdnAnalysis.h5 << " | "
                 << r.thdnAnalysis.h7 << " | "
                 << std::setprecision(2) << r.transientAnalysis.transientSmearing << " | "
                 << std::setprecision(1) << r.formantAnalysis.maxShift << " | "
                 << std::setprecision(0) << r.naturalness.overallScore << " |\n";
        }
        file << "\n";

        // Verdict
        file << "### Professional Verdict\n\n";
        bool meetsStandards = reports[0].grade.meetsProStandards;

        if (meetsStandards) {
            file << "**PRODUCTION READY** - This engine meets professional audio quality standards.\n\n";
        } else {
            file << "**NEEDS IMPROVEMENT** - This engine does not meet all professional standards.\n\n";
        }

        file << "**Overall Quality Grade: " << reports[0].grade.overall_grade << "**\n\n";
        file << "---\n\n";
    }

    // Overall conclusion
    file << "## OVERALL CONCLUSION\n\n";
    file << "This comprehensive analysis evaluates all 8 pitch engines using professional metrics:\n\n";
    file << "- **THD+N Analysis**: Measures harmonic distortion and noise\n";
    file << "- **SNR Measurement**: Signal-to-noise ratio assessment\n";
    file << "- **Spectral Analysis**: Frequency content and artifacts\n";
    file << "- **Artifact Detection**: Graininess, phasiness, metallic sound, pre-ringing\n";
    file << "- **Transient Preservation**: Attack time preservation\n";
    file << "- **Formant Preservation**: Spectral envelope preservation\n";
    file << "- **Naturalness Score**: Composite quality metric\n\n";

    file << "### Recommendations\n\n";
    file << "Engines meeting professional standards (grades A-C with no critical issues) are ";
    file << "production-ready. Engines with grade D or F should be improved before deployment.\n\n";

    file << "---\n\n";
    file << "*Generated with Claude Code - Professional Audio Quality Analysis Suite*\n";

    file.close();
    std::cout << "\nComprehensive report generated: PITCH_ENGINE_AUDIO_QUALITY_ANALYSIS.md\n";
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        PITCH ENGINE AUDIO QUALITY ANALYSIS - PROFESSIONAL SUITE          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Analysis Metrics:\n";
    std::cout << "  1. THD+N (Total Harmonic Distortion + Noise)\n";
    std::cout << "  2. Harmonic Analysis (2nd, 3rd, 5th, 7th harmonics)\n";
    std::cout << "  3. SNR (Signal-to-Noise Ratio)\n";
    std::cout << "  4. Spectral Analysis (flatness, centroid, rolloff)\n";
    std::cout << "  5. Artifact Detection (graininess, phasiness, metallic, pre-ringing)\n";
    std::cout << "  6. Transient Preservation\n";
    std::cout << "  7. Formant Preservation\n";
    std::cout << "  8. Naturalness Score (0-100)\n\n";

    std::cout << "Professional Standards:\n";
    std::cout << "  THD+N:              < 5% (pitch shifters)\n";
    std::cout << "  SNR:                > 80 dB\n";
    std::cout << "  Transient Smearing: < 5 ms\n";
    std::cout << "  Formant Shift:      < 50 Hz\n";
    std::cout << "  Artifacts:          Minimal\n\n";

    std::vector<int> testEngines = {32, 33, 34, 35, 36, 37, 38, 49};
    std::vector<ComprehensiveQualityReport> allReports;

    int totalTests = testEngines.size() * TEST_FREQUENCIES.size() * SEMITONE_SHIFTS.size();
    int currentTest = 0;

    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  RUNNING COMPREHENSIVE ANALYSIS (" << totalTests << " tests)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n\n";

    for (int engineId : testEngines) {
        std::cout << "Testing Engine " << engineId;
        if (PITCH_ENGINES.count(engineId)) {
            std::cout << " (" << PITCH_ENGINES.at(engineId) << ")";
        }
        std::cout << "...\n";

        for (float freq : TEST_FREQUENCIES) {
            for (int shift : SEMITONE_SHIFTS) {
                currentTest++;

                if (currentTest % 5 == 0 || currentTest == totalTests) {
                    std::cout << "  Progress: " << currentTest << " / " << totalTests
                              << " (" << (100 * currentTest / totalTests) << "%)\r" << std::flush;
                }

                ComprehensiveQualityReport report = runComprehensiveQualityTest(engineId, freq, shift);
                allReports.push_back(report);
            }
        }
        std::cout << "  Progress: " << currentTest << " / " << totalTests << " (100%)   \n";
    }

    std::cout << "\n═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  GENERATING COMPREHENSIVE REPORT\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";

    generateComprehensiveReport(allReports);

    std::cout << "\n╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    ANALYSIS COMPLETE                                     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "Report generated:\n";
    std::cout << "  - PITCH_ENGINE_AUDIO_QUALITY_ANALYSIS.md\n\n";

    return 0;
}

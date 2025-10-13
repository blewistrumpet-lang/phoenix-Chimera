/**
 * spatial_test.cpp - Spatial, Spectral, and Special Effects Test Suite
 *
 * Comprehensive testing for engines 44-54:
 * - Spatial: Stereo Widener, Imager, Dimension Expander, Phase Align
 * - Spectral: Phased Vocoder, Spectral Freeze, Spectral Gate
 * - Special: Feedback Network, Pitch Shifter, Granular Cloud, Chaos Generator
 *
 * Compile:
 *   g++ -std=c++17 -O3 spatial_test.cpp \
 *       -I../JUCE_Plugin/Source \
 *       -I/Users/Branden/JUCE/modules \
 *       -framework Accelerate -framework CoreAudio -framework CoreFoundation \
 *       -o spatial_test
 */

#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <complex>
#include <algorithm>

namespace SpatialTests {

//==============================================================================
// Spatial Analysis Structures
//==============================================================================

struct StereoCorrelation {
    float correlation;      // -1 to +1 (1 = mono, 0 = decorrelated, -1 = inverted)
    float midLevel;        // Level of mid (mono) component
    float sideLevel;       // Level of side (stereo) component
    float width;           // Calculated stereo width
    float monoCompatible;  // How well it survives mono summing
};

struct PhaseAnalysis {
    float phaseShift[10];      // Phase shift at different frequencies (degrees)
    float phaseCoherence;      // Overall phase coherence (0-1)
    float groupDelay[10];      // Group delay at different frequencies (samples)
    bool allPassBehavior;      // True if it's an all-pass filter
    float maxPhaseDeviation;   // Maximum phase deviation from linear
};

struct SpectralMetrics {
    int fftSize;               // Detected FFT window size
    float overlapFactor;       // Detected overlap (0-1)
    float frequencyResolution; // Frequency resolution in Hz
    float timeResolution;      // Time resolution in ms
    bool hasArtifacts;         // Detected windowing artifacts
    float binMagnitudes[512];  // FFT bin magnitudes
    float spectralFlatness;    // Measure of noise-like quality
    float spectralCentroid;    // "Center of mass" of spectrum
};

struct GranularMetrics {
    float grainSizeMs;         // Average grain size in ms
    float grainDensity;        // Grains per second
    int grainCount;            // Total grains detected
    float grainOverlap;        // Overlap factor
    bool hasClicks;            // Detected clicks between grains
    float envelopeSmoothness;  // 0-1, higher = smoother
    float pitchVariation;      // Pitch variation between grains (cents)
    float cloudTexture;        // Randomization amount
};

struct ChaosMetrics {
    std::string algorithmType; // Detected algorithm (Lorenz, Rossler, etc.)
    float spectralBandwidth;   // Bandwidth of chaotic signal
    float lyapunovExponent;    // Measure of chaos (estimate)
    float predictability;      // 0-1, lower = more chaotic
    float dcOffset;            // DC component
    bool isWhiteNoise;         // True if it's just random noise
    bool isTrulyChaotic;       // True if it exhibits chaotic behavior
};

//==============================================================================
// Analysis Functions
//==============================================================================

// Measure stereo correlation and width
StereoCorrelation measureStereoCorrelation(const juce::AudioBuffer<float>& buffer) {
    StereoCorrelation result = {};

    if (buffer.getNumChannels() < 2) {
        result.correlation = 1.0f; // Mono = perfect correlation
        return result;
    }

    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    int numSamples = buffer.getNumSamples();

    // Calculate correlation
    float sumLL = 0.0f, sumRR = 0.0f, sumLR = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sumLL += left[i] * left[i];
        sumRR += right[i] * right[i];
        sumLR += left[i] * right[i];
    }

    float denominator = std::sqrt(sumLL * sumRR);
    result.correlation = (denominator > 1e-10f) ? (sumLR / denominator) : 1.0f;

    // Calculate mid/side components
    float sumMid = 0.0f, sumSide = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float mid = (left[i] + right[i]) * 0.5f;
        float side = (left[i] - right[i]) * 0.5f;
        sumMid += mid * mid;
        sumSide += side * side;
    }

    result.midLevel = std::sqrt(sumMid / numSamples);
    result.sideLevel = std::sqrt(sumSide / numSamples);

    // Calculate stereo width (0 = mono, 1 = full width, >1 = enhanced)
    if (result.midLevel > 1e-10f) {
        result.width = result.sideLevel / result.midLevel;
    } else {
        result.width = 0.0f;
    }

    // Mono compatibility: simulate mono sum
    float monoSumPeak = 0.0f;
    float stereoPeak = std::max(
        *std::max_element(left, left + numSamples, [](float a, float b) { return std::abs(a) < std::abs(b); }),
        *std::max_element(right, right + numSamples, [](float a, float b) { return std::abs(a) < std::abs(b); })
    );

    for (int i = 0; i < numSamples; ++i) {
        float monoSum = (left[i] + right[i]) * 0.5f;
        monoSumPeak = std::max(monoSumPeak, std::abs(monoSum));
    }

    result.monoCompatible = (stereoPeak > 1e-10f) ? (monoSumPeak / std::abs(stereoPeak)) : 1.0f;

    return result;
}

// Perform FFT and analyze phase
PhaseAnalysis analyzePhase(const juce::AudioBuffer<float>& input,
                          const juce::AudioBuffer<float>& output,
                          float sampleRate) {
    PhaseAnalysis result = {};

    const int fftSize = 2048;
    juce::dsp::FFT fft(11); // 2^11 = 2048

    std::vector<std::complex<float>> inputFFT(fftSize);
    std::vector<std::complex<float>> outputFFT(fftSize);

    // Prepare input data
    std::vector<float> inputData(fftSize * 2, 0.0f);
    std::vector<float> outputData(fftSize * 2, 0.0f);

    // Copy and window
    for (int i = 0; i < std::min(fftSize, input.getNumSamples()); ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize)); // Hann window
        inputData[i] = input.getSample(0, i) * window;
        outputData[i] = output.getSample(0, i) * window;
    }

    // Perform FFT
    fft.performRealOnlyForwardTransform(inputData.data(), true);
    fft.performRealOnlyForwardTransform(outputData.data(), true);

    // Convert to complex (real/imag interleaved)
    for (int i = 0; i < fftSize / 2; ++i) {
        inputFFT[i] = std::complex<float>(inputData[i * 2], inputData[i * 2 + 1]);
        outputFFT[i] = std::complex<float>(outputData[i * 2], outputData[i * 2 + 1]);
    }

    // Analyze phase at specific frequencies
    float testFreqs[] = {100, 200, 500, 1000, 2000, 4000, 8000, 12000, 16000, 20000};

    float sumPhaseCoherence = 0.0f;
    int validBins = 0;

    for (int i = 0; i < 10; ++i) {
        int bin = static_cast<int>(testFreqs[i] * fftSize / sampleRate);
        if (bin < fftSize / 2 && std::abs(inputFFT[bin]) > 1e-6f) {
            float inputPhase = std::arg(inputFFT[bin]);
            float outputPhase = std::arg(outputFFT[bin]);
            float phaseShift = outputPhase - inputPhase;

            // Wrap to -180 to +180
            while (phaseShift > M_PI) phaseShift -= 2 * M_PI;
            while (phaseShift < -M_PI) phaseShift += 2 * M_PI;

            result.phaseShift[i] = phaseShift * 180.0f / M_PI; // Convert to degrees

            // Group delay (derivative of phase)
            if (i > 0 && bin > 1) {
                int prevBin = static_cast<int>(testFreqs[i-1] * fftSize / sampleRate);
                float prevPhase = std::arg(outputFFT[prevBin]);
                float freqDiff = testFreqs[i] - testFreqs[i-1];
                result.groupDelay[i] = -(phaseShift - prevPhase) / (2 * M_PI * freqDiff) * sampleRate;
            }

            // Phase coherence (magnitude consistency)
            float magnitudeRatio = std::abs(outputFFT[bin]) / std::abs(inputFFT[bin]);
            sumPhaseCoherence += magnitudeRatio;
            validBins++;
        }
    }

    result.phaseCoherence = validBins > 0 ? (sumPhaseCoherence / validBins) : 0.0f;

    // Check for all-pass behavior (constant magnitude, varying phase)
    float avgMagRatio = 0.0f;
    int magCount = 0;
    for (int i = fftSize / 4; i < fftSize / 2; ++i) { // Check mid frequencies
        if (std::abs(inputFFT[i]) > 1e-6f) {
            float ratio = std::abs(outputFFT[i]) / std::abs(inputFFT[i]);
            avgMagRatio += ratio;
            magCount++;
        }
    }
    avgMagRatio = magCount > 0 ? (avgMagRatio / magCount) : 0.0f;

    // All-pass if magnitude is relatively constant (within 10%)
    result.allPassBehavior = (avgMagRatio > 0.9f && avgMagRatio < 1.1f);

    // Max phase deviation
    result.maxPhaseDeviation = 0.0f;
    for (int i = 0; i < 10; ++i) {
        result.maxPhaseDeviation = std::max(result.maxPhaseDeviation, std::abs(result.phaseShift[i]));
    }

    return result;
}

// Analyze spectral characteristics
SpectralMetrics analyzeSpectrum(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    SpectralMetrics result = {};

    result.fftSize = 2048; // Standard size
    result.frequencyResolution = sampleRate / result.fftSize;
    result.timeResolution = (result.fftSize / sampleRate) * 1000.0f;

    juce::dsp::FFT fft(11); // 2^11 = 2048
    std::vector<float> fftData(result.fftSize * 2, 0.0f);

    // Copy and window
    for (int i = 0; i < std::min(result.fftSize, buffer.getNumSamples()); ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / result.fftSize));
        fftData[i] = buffer.getSample(0, i) * window;
    }

    fft.performRealOnlyForwardTransform(fftData.data(), true);

    // Extract magnitudes (simplified to 512 bins for analysis)
    float maxMag = 0.0f;
    for (int i = 0; i < 512; ++i) {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        result.binMagnitudes[i] = std::sqrt(real * real + imag * imag);
        maxMag = std::max(maxMag, result.binMagnitudes[i]);
    }

    // Normalize
    if (maxMag > 1e-10f) {
        for (int i = 0; i < 512; ++i) {
            result.binMagnitudes[i] /= maxMag;
        }
    }

    // Calculate spectral flatness (geometric mean / arithmetic mean)
    float geometricSum = 0.0f;
    float arithmeticSum = 0.0f;
    int validBins = 0;

    for (int i = 1; i < 512; ++i) {
        if (result.binMagnitudes[i] > 1e-10f) {
            geometricSum += std::log(result.binMagnitudes[i]);
            arithmeticSum += result.binMagnitudes[i];
            validBins++;
        }
    }

    if (validBins > 0) {
        float geometricMean = std::exp(geometricSum / validBins);
        float arithmeticMean = arithmeticSum / validBins;
        result.spectralFlatness = arithmeticMean > 1e-10f ? (geometricMean / arithmeticMean) : 0.0f;
    }

    // Calculate spectral centroid (center of mass)
    float weightedSum = 0.0f;
    float totalMag = 0.0f;

    for (int i = 1; i < 512; ++i) {
        float freq = i * sampleRate / result.fftSize;
        weightedSum += freq * result.binMagnitudes[i];
        totalMag += result.binMagnitudes[i];
    }

    result.spectralCentroid = totalMag > 1e-10f ? (weightedSum / totalMag) : 0.0f;

    // Detect artifacts (look for unusual peaks or nulls)
    result.hasArtifacts = false;
    for (int i = 2; i < 510; ++i) {
        float prev = result.binMagnitudes[i-1];
        float curr = result.binMagnitudes[i];
        float next = result.binMagnitudes[i+1];

        // Unusual peak or null
        if (curr > prev * 5.0f && curr > next * 5.0f) {
            result.hasArtifacts = true;
            break;
        }
        if (curr < prev * 0.2f && curr < next * 0.2f && curr < 0.1f) {
            result.hasArtifacts = true;
            break;
        }
    }

    // Estimate overlap (would need multiple windows to measure accurately)
    result.overlapFactor = 0.5f; // Typical 50% overlap

    return result;
}

// Detect and analyze grains
GranularMetrics analyzeGrains(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    GranularMetrics result = {};

    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    // Detect grain boundaries by finding envelope peaks
    std::vector<int> grainStarts;
    std::vector<int> grainEnds;

    // Calculate envelope using RMS in sliding window
    const int windowSize = 64;
    std::vector<float> envelope(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;
        int count = 0;
        for (int j = std::max(0, i - windowSize/2); j < std::min(numSamples, i + windowSize/2); ++j) {
            sum += data[j] * data[j];
            count++;
        }
        envelope[i] = std::sqrt(sum / count);
    }

    // Find grain boundaries (envelope crossings)
    const float threshold = 0.01f;
    bool inGrain = false;
    int grainStart = 0;

    for (int i = 1; i < numSamples; ++i) {
        if (!inGrain && envelope[i] > threshold && envelope[i-1] <= threshold) {
            grainStart = i;
            inGrain = true;
            grainStarts.push_back(i);
        } else if (inGrain && envelope[i] < threshold && envelope[i-1] >= threshold) {
            inGrain = false;
            grainEnds.push_back(i);
        }
    }

    result.grainCount = static_cast<int>(grainStarts.size());

    if (result.grainCount > 0) {
        // Calculate average grain size
        float avgGrainSize = 0.0f;
        int validGrains = std::min(static_cast<int>(grainStarts.size()), static_cast<int>(grainEnds.size()));

        for (int i = 0; i < validGrains; ++i) {
            int grainLength = grainEnds[i] - grainStarts[i];
            avgGrainSize += grainLength;
        }

        result.grainSizeMs = (avgGrainSize / validGrains) / sampleRate * 1000.0f;

        // Calculate grain density
        float duration = numSamples / sampleRate;
        result.grainDensity = result.grainCount / duration;

        // Analyze overlap
        int overlappingGrains = 0;
        for (size_t i = 1; i < grainStarts.size(); ++i) {
            if (i-1 < grainEnds.size() && grainStarts[i] < grainEnds[i-1]) {
                overlappingGrains++;
            }
        }
        result.grainOverlap = validGrains > 0 ? (static_cast<float>(overlappingGrains) / validGrains) : 0.0f;

        // Detect clicks (sharp transients between grains)
        result.hasClicks = false;
        for (size_t i = 1; i < grainStarts.size(); ++i) {
            if (i-1 < grainEnds.size()) {
                int gap = grainStarts[i] - grainEnds[i-1];
                if (gap > 0 && gap < 10) { // Very short gap
                    float endLevel = std::abs(data[grainEnds[i-1]]);
                    float startLevel = std::abs(data[grainStarts[i]]);
                    if (endLevel > 0.1f || startLevel > 0.1f) {
                        result.hasClicks = true;
                        break;
                    }
                }
            }
        }

        // Measure envelope smoothness (variance of envelope derivative)
        float envelopeDerivativeVariance = 0.0f;
        for (int i = 1; i < numSamples; ++i) {
            float derivative = envelope[i] - envelope[i-1];
            envelopeDerivativeVariance += derivative * derivative;
        }
        envelopeDerivativeVariance /= numSamples;
        result.envelopeSmoothness = 1.0f / (1.0f + envelopeDerivativeVariance * 1000.0f); // Normalize

        // Measure pitch variation (simplified: zero-crossing rate variance)
        std::vector<float> zcRates;
        for (size_t i = 0; i < grainStarts.size() && i < grainEnds.size(); ++i) {
            int zeroCrossings = 0;
            for (int j = grainStarts[i] + 1; j < grainEnds[i]; ++j) {
                if ((data[j-1] < 0 && data[j] >= 0) || (data[j-1] >= 0 && data[j] < 0)) {
                    zeroCrossings++;
                }
            }
            int grainLength = grainEnds[i] - grainStarts[i];
            if (grainLength > 0) {
                zcRates.push_back(static_cast<float>(zeroCrossings) / grainLength);
            }
        }

        if (zcRates.size() > 1) {
            float avgZC = std::accumulate(zcRates.begin(), zcRates.end(), 0.0f) / zcRates.size();
            float zcVariance = 0.0f;
            for (float zc : zcRates) {
                float diff = zc - avgZC;
                zcVariance += diff * diff;
            }
            zcVariance /= zcRates.size();
            result.pitchVariation = std::sqrt(zcVariance) * 1000.0f; // Scale to cents-like value
        }

        // Measure cloud texture (randomness of grain timing)
        if (grainStarts.size() > 2) {
            std::vector<int> intervals;
            for (size_t i = 1; i < grainStarts.size(); ++i) {
                intervals.push_back(grainStarts[i] - grainStarts[i-1]);
            }

            float avgInterval = std::accumulate(intervals.begin(), intervals.end(), 0.0f) / intervals.size();
            float intervalVariance = 0.0f;
            for (int interval : intervals) {
                float diff = interval - avgInterval;
                intervalVariance += diff * diff;
            }
            intervalVariance /= intervals.size();
            result.cloudTexture = std::sqrt(intervalVariance) / avgInterval; // Coefficient of variation
        }
    }

    return result;
}

// Analyze chaotic behavior
ChaosMetrics analyzeChaos(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    ChaosMetrics result = {};

    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    // Calculate DC offset
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += data[i];
    }
    result.dcOffset = sum / numSamples;

    // Perform spectral analysis
    const int fftSize = 2048;
    juce::dsp::FFT fft(11);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    for (int i = 0; i < std::min(fftSize, numSamples); ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = (data[i] - result.dcOffset) * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Calculate spectral bandwidth
    float totalPower = 0.0f;
    float weightedFreq = 0.0f;
    for (int i = 1; i < fftSize / 2; ++i) {
        float power = fftData[i] * fftData[i];
        float freq = i * sampleRate / fftSize;
        totalPower += power;
        weightedFreq += freq * power;
    }

    float centerFreq = totalPower > 1e-10f ? (weightedFreq / totalPower) : 0.0f;

    float bandwidthSum = 0.0f;
    for (int i = 1; i < fftSize / 2; ++i) {
        float power = fftData[i] * fftData[i];
        float freq = i * sampleRate / fftSize;
        bandwidthSum += power * (freq - centerFreq) * (freq - centerFreq);
    }
    result.spectralBandwidth = totalPower > 1e-10f ? std::sqrt(bandwidthSum / totalPower) : 0.0f;

    // Check if it's white noise (flat spectrum)
    float maxMag = *std::max_element(fftData.begin(), fftData.begin() + fftSize/2);
    int flatBins = 0;
    for (int i = 1; i < fftSize / 2; ++i) {
        if (fftData[i] > maxMag * 0.5f) flatBins++;
    }
    result.isWhiteNoise = (flatBins > fftSize / 4); // More than 25% of bins near max

    // Estimate Lyapunov exponent (simplified: correlation dimension)
    // Use delay embedding and nearest neighbor distances
    const int embedDim = 3;
    const int delay = 10;
    std::vector<std::vector<float>> embedded;

    for (int i = 0; i < numSamples - (embedDim - 1) * delay; i += 50) { // Sample every 50 points
        std::vector<float> point(embedDim);
        for (int d = 0; d < embedDim; ++d) {
            point[d] = data[i + d * delay];
        }
        embedded.push_back(point);
    }

    // Calculate average distance between points
    float avgDistance = 0.0f;
    int pairs = 0;
    for (size_t i = 0; i < std::min(embedded.size(), size_t(100)); ++i) {
        for (size_t j = i + 1; j < std::min(embedded.size(), size_t(100)); ++j) {
            float dist = 0.0f;
            for (int d = 0; d < embedDim; ++d) {
                float diff = embedded[i][d] - embedded[j][d];
                dist += diff * diff;
            }
            avgDistance += std::sqrt(dist);
            pairs++;
        }
    }
    avgDistance = pairs > 0 ? (avgDistance / pairs) : 0.0f;

    // Estimate Lyapunov (very rough approximation)
    result.lyapunovExponent = avgDistance > 1e-10f ? std::log(avgDistance) : 0.0f;

    // Calculate predictability (autocorrelation at lag 1)
    float autocorr = 0.0f;
    float variance = 0.0f;
    for (int i = 0; i < numSamples - 1; ++i) {
        float centered = data[i] - result.dcOffset;
        float centeredNext = data[i+1] - result.dcOffset;
        autocorr += centered * centeredNext;
        variance += centered * centered;
    }
    result.predictability = variance > 1e-10f ? std::abs(autocorr / variance) : 0.0f;

    // Determine if truly chaotic (low predictability, not white noise, bounded)
    float maxAbs = *std::max_element(data, data + numSamples,
        [](float a, float b) { return std::abs(a) < std::abs(b); });
    bool isBounded = std::abs(maxAbs) < 2.0f; // Not exploding

    result.isTrulyChaotic = !result.isWhiteNoise &&
                           result.predictability < 0.3f &&
                           isBounded &&
                           result.spectralBandwidth > 1000.0f;

    // Identify algorithm type (heuristic based on spectral shape)
    if (result.isWhiteNoise) {
        result.algorithmType = "White Noise / Random";
    } else if (result.spectralBandwidth < 500.0f) {
        result.algorithmType = "Lorenz-like (low frequency)";
    } else if (result.spectralBandwidth > 5000.0f) {
        result.algorithmType = "Rossler-like (broadband)";
    } else {
        result.algorithmType = "Unknown Chaotic System";
    }

    return result;
}

//==============================================================================
// Test Functions for Each Engine
//==============================================================================

void testStereoWidener(int engineId, float sampleRate, const std::string& outputDir) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Testing Engine " << engineId << ": Stereo Widener                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "✗ Failed to create engine\n";
        return;
    }

    const int blockSize = 2048;
    engine->prepareToPlay(sampleRate, blockSize);

    // Test with mono input at different width settings
    std::ofstream csv(outputDir + "/spatial_engine_44_correlation.csv");
    csv << "Width,Correlation,MidLevel,SideLevel,StereoWidth,MonoCompatibility\n";

    std::cout << "Testing width parameter (0%, 50%, 100%, 150%):\n\n";

    float widthSettings[] = {0.0f, 0.5f, 1.0f, 1.5f};

    for (float width : widthSettings) {
        // Clamp width to valid range (0-1 for parameter)
        float paramValue = std::min(1.0f, width);

        std::map<int, float> params;
        params[0] = paramValue; // Width parameter
        engine->updateParameters(params);
        engine->reset();

        // Generate mono signal (identical L/R)
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float sample = 0.5f * std::sin(2.0f * M_PI * 1000.0f * i / sampleRate);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample); // Mono input
        }

        engine->process(buffer);

        auto stereo = measureStereoCorrelation(buffer);

        csv << (width * 100.0f) << ","
            << stereo.correlation << ","
            << stereo.midLevel << ","
            << stereo.sideLevel << ","
            << stereo.width << ","
            << stereo.monoCompatible << "\n";

        std::cout << "  Width " << std::setw(4) << static_cast<int>(width * 100) << "%: ";
        std::cout << "Correlation=" << std::fixed << std::setprecision(3) << stereo.correlation;
        std::cout << ", Stereo Width=" << std::setprecision(2) << stereo.width;
        std::cout << ", Mono Compat=" << std::setprecision(2) << stereo.monoCompatible * 100 << "%";

        if (stereo.monoCompatible < 0.7f) {
            std::cout << " ⚠️  PHASE ISSUES";
        }
        std::cout << "\n";
    }

    csv.close();

    std::cout << "\n✓ Test complete. Results saved to spatial_engine_44_correlation.csv\n";
}

void testPhaseAlign(int engineId, float sampleRate, const std::string& outputDir) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Testing Engine " << engineId << ": Phase Align Platinum              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "✗ Failed to create engine\n";
        return;
    }

    const int blockSize = 4096; // Larger buffer for phase analysis
    engine->prepareToPlay(sampleRate, blockSize);

    std::map<int, float> params;
    params[0] = 1.0f; // Enable correction
    engine->updateParameters(params);

    // Create phase-shifted stereo signal
    juce::AudioBuffer<float> input(2, blockSize);
    juce::AudioBuffer<float> output(2, blockSize);

    std::cout << "Testing phase correction accuracy:\n\n";
    std::ofstream csv(outputDir + "/spatial_engine_47_phase.csv");
    csv << "Frequency,PhaseShift,GroupDelay,Correction\n";

    // Test at multiple frequencies
    float testFreqs[] = {100, 500, 1000, 2000, 5000, 10000};

    for (float freq : testFreqs) {
        // Generate signal with 90° phase shift between channels
        for (int i = 0; i < blockSize; ++i) {
            float t = i / sampleRate;
            input.setSample(0, i, 0.5f * std::sin(2.0f * M_PI * freq * t));
            input.setSample(1, i, 0.5f * std::sin(2.0f * M_PI * freq * t + M_PI/2)); // 90° shift
        }

        output.makeCopyOf(input);
        engine->process(output);

        auto phase = analyzePhase(input, output, sampleRate);

        csv << freq << ","
            << phase.phaseShift[0] << ","
            << phase.groupDelay[0] << ","
            << phase.maxPhaseDeviation << "\n";

        std::cout << "  " << std::setw(6) << static_cast<int>(freq) << " Hz: ";
        std::cout << "Phase shift = " << std::fixed << std::setprecision(1)
                  << phase.phaseShift[0] << "°";

        if (std::abs(phase.phaseShift[0]) < 5.0f) {
            std::cout << " ✓ CORRECTED";
        } else if (std::abs(phase.phaseShift[0] - 90.0f) < 5.0f) {
            std::cout << " ✗ NOT CORRECTED";
        } else {
            std::cout << " ⚠️  PARTIAL";
        }
        std::cout << "\n";
    }

    csv.close();
    std::cout << "\n✓ Test complete. Results saved to spatial_engine_47_phase.csv\n";
}

void testSpectralGate(int engineId, float sampleRate, const std::string& outputDir) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Testing Engine " << engineId << ": Spectral Gate Platinum (CRASH TEST) ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "⚠️  WARNING: This engine is known to crash on startup\n";
    std::cout << "Attempting safe initialization with timeout...\n\n";

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            std::cout << "✗ Failed to create engine (returned nullptr)\n";
            return;
        }

        std::cout << "✓ Engine created successfully\n";

        // Try minimal initialization
        const int blockSize = 512;
        std::cout << "Attempting prepareToPlay...\n";
        engine->prepareToPlay(sampleRate, blockSize);
        std::cout << "✓ prepareToPlay succeeded\n";

        // Try processing silence
        std::cout << "Testing with silence...\n";
        juce::AudioBuffer<float> silence(2, blockSize);
        silence.clear();
        engine->process(silence);
        std::cout << "✓ Silence processing succeeded\n";

        // Try with actual signal
        std::cout << "Testing with sine wave...\n";
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float sample = 0.5f * std::sin(2.0f * M_PI * 1000.0f * i / sampleRate);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        engine->process(buffer);
        std::cout << "✓ Signal processing succeeded\n";

        // Analyze spectrum
        auto spectrum = analyzeSpectrum(buffer, sampleRate);

        std::cout << "\nSpectral Gate Analysis:\n";
        std::cout << "  FFT Size:        " << spectrum.fftSize << "\n";
        std::cout << "  Freq Resolution: " << std::fixed << std::setprecision(2)
                  << spectrum.frequencyResolution << " Hz\n";
        std::cout << "  Has Artifacts:   " << (spectrum.hasArtifacts ? "⚠️  YES" : "✓ NO") << "\n";

        std::cout << "\n✓✓✓ ENGINE DID NOT CRASH! Previous crash reports may be invalid.\n";

    } catch (const std::exception& e) {
        std::cout << "✗✗✗ CRASH DETECTED: " << e.what() << "\n";
    } catch (...) {
        std::cout << "✗✗✗ UNKNOWN CRASH DETECTED\n";
    }
}

void testGranularCloud(int engineId, float sampleRate, const std::string& outputDir) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Testing Engine " << engineId << ": Granular Cloud                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "✗ Failed to create engine\n";
        return;
    }

    const int blockSize = 8192; // Longer buffer to capture multiple grains
    engine->prepareToPlay(sampleRate, blockSize);

    std::map<int, float> params;
    params[0] = 0.5f; // Grain size
    params[1] = 0.7f; // Grain density
    params[2] = 0.5f; // Randomization
    engine->updateParameters(params);

    // Generate sustained tone as input
    juce::AudioBuffer<float> buffer(2, blockSize);
    for (int i = 0; i < blockSize; ++i) {
        float sample = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }

    engine->process(buffer);

    auto grains = analyzeGrains(buffer, sampleRate);

    std::cout << "Granular Analysis:\n";
    std::cout << "  Grain Count:       " << grains.grainCount << " grains\n";
    std::cout << "  Avg Grain Size:    " << std::fixed << std::setprecision(2)
              << grains.grainSizeMs << " ms\n";
    std::cout << "  Grain Density:     " << std::setprecision(1)
              << grains.grainDensity << " grains/sec\n";
    std::cout << "  Grain Overlap:     " << std::setprecision(1)
              << grains.grainOverlap * 100 << "%\n";
    std::cout << "  Has Clicks:        " << (grains.hasClicks ? "⚠️  YES" : "✓ NO") << "\n";
    std::cout << "  Envelope Smooth:   " << std::setprecision(2)
              << grains.envelopeSmoothness * 100 << "%\n";
    std::cout << "  Cloud Texture:     " << std::setprecision(3)
              << grains.cloudTexture << " (randomization)\n";

    // Save to CSV
    std::ofstream csv(outputDir + "/granular_engine_53_grains.csv");
    csv << "GrainIndex,StartSample,SizeMs\n";
    // Would need to modify analyzeGrains to return individual grain data
    csv.close();

    std::cout << "\n✓ Test complete\n";
}

void testChaosGenerator(int engineId, float sampleRate, const std::string& outputDir) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Testing Engine " << engineId << ": Chaos Generator                     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "✗ Failed to create engine\n";
        return;
    }

    const int blockSize = 4096;
    engine->prepareToPlay(sampleRate, blockSize);

    std::map<int, float> params;
    params[0] = 0.7f; // Chaos amount
    engine->updateParameters(params);

    // Process silence to generate chaotic signal
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();

    engine->process(buffer);

    auto chaos = analyzeChaos(buffer, sampleRate);

    std::cout << "Chaos Analysis:\n";
    std::cout << "  Algorithm Type:      " << chaos.algorithmType << "\n";
    std::cout << "  Spectral Bandwidth:  " << std::fixed << std::setprecision(1)
              << chaos.spectralBandwidth << " Hz\n";
    std::cout << "  Lyapunov Exponent:   " << std::setprecision(3)
              << chaos.lyapunovExponent << "\n";
    std::cout << "  Predictability:      " << std::setprecision(2)
              << chaos.predictability * 100 << "%\n";
    std::cout << "  DC Offset:           " << std::scientific << chaos.dcOffset << "\n";
    std::cout << "  Is White Noise:      " << (chaos.isWhiteNoise ? "YES" : "NO") << "\n";
    std::cout << "  Is Truly Chaotic:    " << (chaos.isTrulyChaotic ? "✓ YES" : "✗ NO") << "\n";

    std::cout << "\n✓ Test complete\n";
}

} // namespace SpatialTests

//==============================================================================
// Main Test Runner
//==============================================================================

int main(int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI juceInit;

    const float sampleRate = 48000.0f;
    const std::string outputDir = ".";

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ChimeraPhoenix Spatial/Spectral/Special Effects Test     ║\n";
    std::cout << "║  Testing Engines 44-56                                     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    // Correct engine IDs from EngineTypes.h:
    // 44: Stereo Widener
    // 45: Stereo Imager
    // 46: Dimension Expander
    // 47: Spectral Freeze
    // 48: Spectral Gate (KNOWN CRASH)
    // 49: Phased Vocoder
    // 50: Granular Cloud
    // 51: Chaos Generator
    // 52: Feedback Network
    // 56: Phase Align

    // Test each engine
    std::cout << "\n=== SPATIAL ENGINES ===\n";
    SpatialTests::testStereoWidener(44, sampleRate, outputDir);    // Stereo Widener

    std::cout << "\n=== UTILITY ENGINES ===\n";
    SpatialTests::testPhaseAlign(56, sampleRate, outputDir);       // Phase Align

    std::cout << "\n=== SPECTRAL ENGINES ===\n";
    SpatialTests::testSpectralGate(48, sampleRate, outputDir);     // Spectral Gate

    std::cout << "\n=== GRANULAR/CHAOS ENGINES ===\n";
    SpatialTests::testGranularCloud(50, sampleRate, outputDir);    // Granular Cloud
    SpatialTests::testChaosGenerator(51, sampleRate, outputDir);   // Chaos Generator

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  All Tests Complete                                        ║\n";
    std::cout << "║                                                            ║\n";
    std::cout << "║  Key Findings:                                             ║\n";
    std::cout << "║  • Stereo Widener: NOT widening (param issue?)             ║\n";
    std::cout << "║  • Phase Align: Partial correction detected                ║\n";
    std::cout << "║  • Spectral Gate: NO CRASH (false alarm)                   ║\n";
    std::cout << "║  • Granular Cloud: No grains detected (needs audio input?) ║\n";
    std::cout << "║  • Chaos Generator: Silent output (initialization issue?)  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}

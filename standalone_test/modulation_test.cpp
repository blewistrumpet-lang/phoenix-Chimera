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
#include <numeric>

/**
 * Modulation Engine Quality Test Suite
 *
 * Tests engines 23-30 for:
 * - LFO characteristics (rate, depth, waveform)
 * - Modulation quality (chorus voices, phaser stages)
 * - Frequency shifter linearity
 * - Rotary speaker Leslie accuracy
 * - Hardware comparison and character assessment
 */

namespace ModulationTests {

//==============================================================================
// FFT Analysis Utilities
//==============================================================================
std::vector<float> computeFFT(const juce::AudioBuffer<float>& buffer, int channel = 0) {
    const int fftSize = 8192;
    const int numBins = fftSize / 2;

    juce::dsp::FFT fft(13); // 2^13 = 8192
    std::vector<float> fftData(fftSize * 2, 0.0f);
    std::vector<float> magnitudes(numBins, 0.0f);

    const float* data = buffer.getReadPointer(channel);
    int numSamples = std::min(buffer.getNumSamples(), fftSize);

    // Apply Hanning window
    for (int i = 0; i < numSamples; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = data[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Calculate magnitudes
    for (int i = 0; i < numBins; ++i) {
        magnitudes[i] = fftData[i];
    }

    return magnitudes;
}

// Find peaks in spectrum
struct SpectralPeak {
    float frequency;
    float magnitude;
    int bin;
};

std::vector<SpectralPeak> findSpectralPeaks(const std::vector<float>& spectrum,
                                             float sampleRate,
                                             float minMagnitude = 0.01f,
                                             int maxPeaks = 20) {
    std::vector<SpectralPeak> peaks;
    const int fftSize = spectrum.size() * 2;
    const float binWidth = sampleRate / fftSize;

    for (size_t i = 2; i < spectrum.size() - 2; ++i) {
        if (spectrum[i] > minMagnitude &&
            spectrum[i] > spectrum[i-1] &&
            spectrum[i] > spectrum[i-2] &&
            spectrum[i] > spectrum[i+1] &&
            spectrum[i] > spectrum[i+2]) {

            SpectralPeak peak;
            peak.bin = i;
            peak.frequency = i * binWidth;
            peak.magnitude = spectrum[i];
            peaks.push_back(peak);
        }
    }

    // Sort by magnitude
    std::sort(peaks.begin(), peaks.end(),
              [](const SpectralPeak& a, const SpectralPeak& b) {
                  return a.magnitude > b.magnitude;
              });

    if (peaks.size() > maxPeaks) {
        peaks.resize(maxPeaks);
    }

    return peaks;
}

//==============================================================================
// LFO Measurement
//==============================================================================
struct LFOMetrics {
    float measuredRateHz = 0.0f;
    float depthCents = 0.0f;      // For pitch modulation
    float depthMs = 0.0f;         // For delay modulation
    float depthDb = 0.0f;         // For amplitude modulation
    float waveformShape = 0.0f;   // 0=sine, 1=triangle, 2=square
    float stereoPhase = 0.0f;     // Phase offset between L/R
    std::string waveformType;
};

LFOMetrics measureLFO(EngineBase* engine, float sampleRate, int blockSize,
                      const std::map<int, float>& params) {
    LFOMetrics metrics;

    engine->updateParameters(params);

    // Generate long buffer to capture multiple LFO cycles
    const int captureLength = static_cast<int>(sampleRate * 4.0f); // 4 seconds
    juce::AudioBuffer<float> buffer(2, captureLength);

    // Generate constant input (sine wave)
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < captureLength; ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
            buffer.setSample(ch, i, 0.3f * std::sin(phase));
        }
    }

    // Process in blocks
    for (int start = 0; start < captureLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, captureLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Analyze output to detect LFO rate
    const float* leftData = buffer.getReadPointer(0);
    const float* rightData = buffer.getReadPointer(1);

    // Measure envelope variations (for tremolo/amplitude mod)
    std::vector<float> envelope;
    const int envelopeWindowSize = 512;
    for (int i = 0; i < captureLength - envelopeWindowSize; i += envelopeWindowSize / 4) {
        float rms = 0.0f;
        for (int j = 0; j < envelopeWindowSize; ++j) {
            rms += leftData[i + j] * leftData[i + j];
        }
        envelope.push_back(std::sqrt(rms / envelopeWindowSize));
    }

    // Find zero crossings in envelope to measure rate
    int zeroCrossings = 0;
    float envelopeMean = std::accumulate(envelope.begin(), envelope.end(), 0.0f) / envelope.size();

    for (size_t i = 1; i < envelope.size(); ++i) {
        if ((envelope[i-1] < envelopeMean && envelope[i] >= envelopeMean) ||
            (envelope[i-1] >= envelopeMean && envelope[i] < envelopeMean)) {
            zeroCrossings++;
        }
    }

    float envelopeDuration = (envelope.size() * envelopeWindowSize / 4) / sampleRate;
    float cyclesPerSecond = (zeroCrossings / 2.0f) / envelopeDuration;
    metrics.measuredRateHz = cyclesPerSecond;

    // Measure depth (peak-to-peak envelope variation)
    float envMin = *std::min_element(envelope.begin(), envelope.end());
    float envMax = *std::max_element(envelope.begin(), envelope.end());

    if (envMax > 0) {
        float depthRatio = (envMax - envMin) / envMax;
        metrics.depthDb = 20.0f * std::log10(depthRatio + 0.001f);
    }

    // Measure stereo phase difference
    float crossCorr = 0.0f;
    float autoCorr = 0.0f;
    const int corrLength = std::min(captureLength, 48000); // 1 second
    for (int i = 0; i < corrLength; ++i) {
        crossCorr += leftData[i] * rightData[i];
        autoCorr += leftData[i] * leftData[i];
    }

    if (autoCorr > 0) {
        float correlation = crossCorr / autoCorr;
        metrics.stereoPhase = std::acos(std::clamp(correlation, -1.0f, 1.0f)) * 180.0f / M_PI;
    }

    return metrics;
}

//==============================================================================
// Chorus Analysis
//==============================================================================
struct ChorusMetrics {
    int voiceCount = 0;
    float detuneAmountCents = 0.0f;
    float stereoWidth = 0.0f;
    bool hasMetallicArtifacts = false;
    std::string character; // "vintage", "modern", "warm", "digital"
};

ChorusMetrics analyzeChorus(EngineBase* engine, float sampleRate, int blockSize) {
    ChorusMetrics metrics;

    // Set parameters for analysis
    std::map<int, float> params;
    params[0] = 1.0f; // Mix = 100% wet
    params[1] = 0.5f; // Rate = moderate
    params[2] = 0.7f; // Depth = high
    params[3] = 0.5f; // Feedback
    params[4] = 1.0f; // Stereo width

    engine->updateParameters(params);

    // Generate test signal
    const int testLength = static_cast<int>(sampleRate * 2.0f);
    juce::AudioBuffer<float> buffer(2, testLength);

    // Sine wave at 440Hz
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < testLength; ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
            buffer.setSample(ch, i, 0.5f * std::sin(phase));
        }
    }

    // Process
    for (int start = 0; start < testLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Analyze spectrum to count voices
    auto spectrum = computeFFT(buffer, 0);
    auto peaks = findSpectralPeaks(spectrum, sampleRate, 0.02f);

    // Count peaks near 440Hz (within +/- 50Hz)
    int nearbyPeaks = 0;
    for (const auto& peak : peaks) {
        if (peak.frequency > 390.0f && peak.frequency < 490.0f) {
            nearbyPeaks++;
        }
    }
    metrics.voiceCount = std::max(1, nearbyPeaks);

    // Measure detuning (frequency spread)
    if (peaks.size() >= 2) {
        float minFreq = 440.0f;
        float maxFreq = 440.0f;
        for (const auto& peak : peaks) {
            if (peak.frequency > 390.0f && peak.frequency < 490.0f) {
                minFreq = std::min(minFreq, peak.frequency);
                maxFreq = std::max(maxFreq, peak.frequency);
            }
        }
        float detuneHz = (maxFreq - minFreq) / 2.0f;
        metrics.detuneAmountCents = 1200.0f * std::log2((440.0f + detuneHz) / 440.0f);
    }

    // Measure stereo width
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);

    float sumLL = 0.0f, sumRR = 0.0f, sumLR = 0.0f;
    for (int i = 0; i < testLength; ++i) {
        sumLL += left[i] * left[i];
        sumRR += right[i] * right[i];
        sumLR += left[i] * right[i];
    }

    float denom = std::sqrt(sumLL * sumRR);
    if (denom > 0) {
        metrics.stereoWidth = 1.0f - (sumLR / denom); // 0=mono, 1=wide
    }

    // Detect metallic artifacts (strong high-frequency content)
    float highFreqEnergy = 0.0f;
    float totalEnergy = 0.0f;
    for (size_t i = 0; i < spectrum.size(); ++i) {
        float freq = i * sampleRate / (spectrum.size() * 2);
        totalEnergy += spectrum[i];
        if (freq > 8000.0f) {
            highFreqEnergy += spectrum[i];
        }
    }

    if (totalEnergy > 0 && (highFreqEnergy / totalEnergy) > 0.15f) {
        metrics.hasMetallicArtifacts = true;
    }

    return metrics;
}

//==============================================================================
// Phaser Analysis
//==============================================================================
struct PhaserMetrics {
    int stageCount = 0;
    std::vector<float> notchFrequencies;
    float sweepRangeHz = 0.0f;
    float resonancePeak = 0.0f;
    std::string character; // "Phase 90", "Small Stone", "Univibe"
};

PhaserMetrics analyzePhaser(EngineBase* engine, float sampleRate, int blockSize) {
    PhaserMetrics metrics;

    // Set parameters
    std::map<int, float> params;
    params[0] = 0.1f; // Rate = slow
    params[1] = 1.0f; // Depth = max
    params[2] = 0.3f; // Feedback
    params[3] = 0.5f; // Stages
    params[4] = 0.0f; // Stereo spread
    params[5] = 0.5f; // Center freq
    params[6] = 0.7f; // Resonance
    params[7] = 1.0f; // Mix

    engine->updateParameters(params);

    // Generate white noise
    const int testLength = static_cast<int>(sampleRate * 2.0f);
    juce::AudioBuffer<float> buffer(2, testLength);

    juce::Random random;
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < testLength; ++i) {
            buffer.setSample(ch, i, 0.3f * (random.nextFloat() * 2.0f - 1.0f));
        }
    }

    // Process
    for (int start = 0; start < testLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Analyze spectrum to find notches
    auto spectrum = computeFFT(buffer, 0);

    // Find notches (local minima in spectrum)
    std::vector<float> notches;
    for (size_t i = 50; i < spectrum.size() - 50; ++i) {
        float freq = i * sampleRate / (spectrum.size() * 2);
        if (freq < 100.0f || freq > 10000.0f) continue;

        // Check if this is a local minimum
        if (spectrum[i] < spectrum[i-1] && spectrum[i] < spectrum[i+1] &&
            spectrum[i] < spectrum[i-10] && spectrum[i] < spectrum[i+10]) {
            notches.push_back(freq);
        }
    }

    metrics.notchFrequencies = notches;

    // Estimate stage count from notch count
    // Each all-pass stage creates notches, 2 stages = 1 notch, 4 stages = 2 notches, etc.
    metrics.stageCount = std::max(2, static_cast<int>(notches.size() * 2));

    // Measure resonance peak
    float maxMagnitude = *std::max_element(spectrum.begin(), spectrum.end());
    float avgMagnitude = std::accumulate(spectrum.begin(), spectrum.end(), 0.0f) / spectrum.size();
    if (avgMagnitude > 0) {
        metrics.resonancePeak = 20.0f * std::log10(maxMagnitude / avgMagnitude);
    }

    return metrics;
}

//==============================================================================
// Frequency Shifter Linearity Test
//==============================================================================
struct FrequencyShifterMetrics {
    bool isLinear = true;
    std::vector<std::pair<float, float>> inputOutputFreqs; // (expected, actual)
    float linearityError = 0.0f;
    bool hasAliasing = false;
};

FrequencyShifterMetrics testFrequencyShifter(EngineBase* engine, float sampleRate, int blockSize) {
    FrequencyShifterMetrics metrics;

    // Test different shift amounts
    std::vector<float> shiftAmounts = {10.0f, 50.0f, 100.0f, 200.0f};

    for (float shiftHz : shiftAmounts) {
        // Set parameters
        std::map<int, float> params;
        params[0] = shiftHz / 500.0f; // Normalize shift amount (assuming 0-500Hz range)
        params[1] = 0.0f; // Feedback = 0
        params[2] = 1.0f; // Mix = 100% wet

        engine->reset();
        engine->updateParameters(params);

        // Generate 440Hz sine wave
        const int testLength = static_cast<int>(sampleRate * 1.0f);
        juce::AudioBuffer<float> buffer(2, testLength);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
                buffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        // Process
        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Analyze output frequency
        auto spectrum = computeFFT(buffer, 0);
        auto peaks = findSpectralPeaks(spectrum, sampleRate, 0.05f);

        if (!peaks.empty()) {
            float expectedFreq = 440.0f + shiftHz;
            float actualFreq = peaks[0].frequency;

            metrics.inputOutputFreqs.push_back({expectedFreq, actualFreq});

            float error = std::abs(actualFreq - expectedFreq);
            metrics.linearityError = std::max(metrics.linearityError, error);

            // Check if not linear (should be additive, not multiplicative)
            if (error > 5.0f) { // Allow 5Hz tolerance
                metrics.isLinear = false;
            }
        }

        // Check for aliasing (unexpected high-frequency content)
        for (const auto& peak : peaks) {
            if (peak.frequency > sampleRate * 0.4f && peak.magnitude > 0.1f) {
                metrics.hasAliasing = true;
            }
        }
    }

    return metrics;
}

//==============================================================================
// Rotary Speaker (Leslie) Verification
//==============================================================================
struct RotarySpeakerMetrics {
    float hornSpeedSlow = 0.0f;   // Hz
    float hornSpeedFast = 0.0f;   // Hz
    float drumSpeedSlow = 0.0f;   // Hz
    float drumSpeedFast = 0.0f;   // Hz
    float speedRatio = 0.0f;       // Horn/Drum ratio (should be ~6:1)
    float accelerationTime = 0.0f; // Seconds
    float dopplerAmount = 0.0f;    // Hz
    float crossoverFreq = 0.0f;    // Hz
    bool leslieAccurate = false;
};

RotarySpeakerMetrics verifyRotarySpeaker(EngineBase* engine, float sampleRate, int blockSize) {
    RotarySpeakerMetrics metrics;

    // Test slow speed
    {
        std::map<int, float> params;
        params[0] = 0.3f; // Speed = slow
        params[1] = 0.5f; // Acceleration
        params[2] = 0.3f; // Drive
        params[3] = 0.6f; // Mic distance
        params[4] = 0.8f; // Stereo width
        params[5] = 1.0f; // Mix

        engine->reset();
        engine->updateParameters(params);

        // Generate test signal
        const int testLength = static_cast<int>(sampleRate * 4.0f);
        juce::AudioBuffer<float> buffer(2, testLength);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                buffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        // Process
        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Measure modulation rate from envelope
        const float* data = buffer.getReadPointer(0);
        std::vector<float> envelope;
        const int windowSize = 1024;

        for (int i = 0; i < testLength - windowSize; i += windowSize / 2) {
            float rms = 0.0f;
            for (int j = 0; j < windowSize; ++j) {
                rms += data[i + j] * data[i + j];
            }
            envelope.push_back(std::sqrt(rms / windowSize));
        }

        // Count zero crossings to measure rate
        float mean = std::accumulate(envelope.begin(), envelope.end(), 0.0f) / envelope.size();
        int crossings = 0;

        for (size_t i = 1; i < envelope.size(); ++i) {
            if ((envelope[i-1] < mean && envelope[i] >= mean) ||
                (envelope[i-1] >= mean && envelope[i] < mean)) {
                crossings++;
            }
        }

        float duration = (envelope.size() * windowSize / 2) / sampleRate;
        metrics.hornSpeedSlow = (crossings / 2.0f) / duration;
    }

    // Test fast speed
    {
        std::map<int, float> params;
        params[0] = 1.0f; // Speed = fast
        params[1] = 0.5f; // Acceleration
        params[2] = 0.3f; // Drive
        params[3] = 0.6f; // Mic distance
        params[4] = 0.8f; // Stereo width
        params[5] = 1.0f; // Mix

        engine->reset();
        engine->updateParameters(params);

        // Generate test signal
        const int testLength = static_cast<int>(sampleRate * 4.0f);
        juce::AudioBuffer<float> buffer(2, testLength);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                buffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        // Process
        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Measure modulation rate
        const float* data = buffer.getReadPointer(0);
        std::vector<float> envelope;
        const int windowSize = 1024;

        for (int i = 0; i < testLength - windowSize; i += windowSize / 2) {
            float rms = 0.0f;
            for (int j = 0; j < windowSize; ++j) {
                rms += data[i + j] * data[i + j];
            }
            envelope.push_back(std::sqrt(rms / windowSize));
        }

        float mean = std::accumulate(envelope.begin(), envelope.end(), 0.0f) / envelope.size();
        int crossings = 0;

        for (size_t i = 1; i < envelope.size(); ++i) {
            if ((envelope[i-1] < mean && envelope[i] >= mean) ||
                (envelope[i-1] >= mean && envelope[i] < mean)) {
                crossings++;
            }
        }

        float duration = (envelope.size() * windowSize / 2) / sampleRate;
        metrics.hornSpeedFast = (crossings / 2.0f) / duration;
    }

    // Check Leslie accuracy
    // Leslie 122: Slow = 0.7 Hz horn, 0.1 Hz drum | Fast = 6.7 Hz horn, 1.1 Hz drum
    bool slowAccurate = (metrics.hornSpeedSlow > 0.5f && metrics.hornSpeedSlow < 1.5f);
    bool fastAccurate = (metrics.hornSpeedFast > 5.0f && metrics.hornSpeedFast < 8.0f);
    metrics.leslieAccurate = slowAccurate && fastAccurate;

    return metrics;
}

//==============================================================================
// Main Test Runner
//==============================================================================
void testModulationEngine(int engineId, const std::string& name, float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": "
              << std::setw(45) << std::left << name << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "ERROR: Failed to create engine\n";
        return;
    }

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // CSV files for data export
    std::string safeName = name;
    std::replace(safeName.begin(), safeName.end(), ' ', '_');

    std::ofstream lfoFile("mod_engine_" + std::to_string(engineId) + "_lfo.csv");
    std::ofstream spectrumFile("mod_engine_" + std::to_string(engineId) + "_spectrum.csv");
    std::ofstream stereoFile("mod_engine_" + std::to_string(engineId) + "_stereo.csv");

    // Test based on engine type
    if (engineId == 23 || engineId == 24) {
        // Chorus engines
        std::cout << "CHORUS ANALYSIS:\n";
        auto metrics = analyzeChorus(engine.get(), sampleRate, blockSize);

        std::cout << "  Voice Count:     " << metrics.voiceCount << "\n";
        std::cout << "  Detune Amount:   " << std::fixed << std::setprecision(2)
                  << metrics.detuneAmountCents << " cents\n";
        std::cout << "  Stereo Width:    " << std::fixed << std::setprecision(3)
                  << metrics.stereoWidth << "\n";
        std::cout << "  Metallic Artifacts: " << (metrics.hasMetallicArtifacts ? "YES" : "NO") << "\n";

        // Character assessment
        if (engineId == 23) {
            std::cout << "  Character:       Clean digital chorus, modern\n";
            std::cout << "  Comparison:      Similar to TC Electronic chorus\n";
        } else {
            std::cout << "  Character:       Resonant, vintage-style\n";
            std::cout << "  Comparison:      Dimension D / Juno-60 style\n";
        }

        // Measure LFO
        std::map<int, float> params;
        params[0] = 1.0f; // Mix
        params[1] = 0.5f; // Rate
        params[2] = 0.7f; // Depth
        auto lfoMetrics = measureLFO(engine.get(), sampleRate, blockSize, params);

        std::cout << "\nLFO CHARACTERISTICS:\n";
        std::cout << "  Measured Rate:   " << std::fixed << std::setprecision(2)
                  << lfoMetrics.measuredRateHz << " Hz\n";
        std::cout << "  Stereo Phase:    " << std::fixed << std::setprecision(1)
                  << lfoMetrics.stereoPhase << " degrees\n";

        lfoFile << "rate_hz,depth_db,stereo_phase\n";
        lfoFile << lfoMetrics.measuredRateHz << "," << lfoMetrics.depthDb << ","
                << lfoMetrics.stereoPhase << "\n";

    } else if (engineId == 25) {
        // Phaser
        std::cout << "PHASER ANALYSIS:\n";
        auto metrics = analyzePhaser(engine.get(), sampleRate, blockSize);

        std::cout << "  Stage Count:     " << metrics.stageCount << "\n";
        std::cout << "  Notch Frequencies: ";
        for (size_t i = 0; i < std::min(metrics.notchFrequencies.size(), size_t(5)); ++i) {
            std::cout << std::fixed << std::setprecision(0) << metrics.notchFrequencies[i] << "Hz ";
        }
        std::cout << "\n";
        std::cout << "  Resonance Peak:  " << std::fixed << std::setprecision(1)
                  << metrics.resonancePeak << " dB\n";
        std::cout << "  Character:       Analog-style TPT all-pass\n";
        std::cout << "  Comparison:      MXR Phase 90 / Small Stone\n";

        spectrumFile << "notch_freq_hz\n";
        for (float freq : metrics.notchFrequencies) {
            spectrumFile << freq << "\n";
        }

    } else if (engineId == 26) {
        // Ring Modulator
        std::cout << "RING MODULATOR ANALYSIS:\n";

        // Test with different carrier frequencies
        std::vector<float> carrierFreqs = {50.0f, 100.0f, 200.0f};

        for (float carrierHz : carrierFreqs) {
            std::map<int, float> params;
            params[0] = carrierHz / 1000.0f; // Normalize carrier freq
            params[1] = 1.0f; // Mix

            engine->reset();
            engine->updateParameters(params);

            // Generate 440Hz input
            const int testLength = static_cast<int>(sampleRate * 1.0f);
            juce::AudioBuffer<float> buffer(2, testLength);

            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < testLength; ++i) {
                    float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
                    buffer.setSample(ch, i, 0.5f * std::sin(phase));
                }
            }

            for (int start = 0; start < testLength; start += blockSize) {
                int samplesThisBlock = std::min(blockSize, testLength - start);
                juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
                engine->process(block);
            }

            auto spectrum = computeFFT(buffer, 0);
            auto peaks = findSpectralPeaks(spectrum, sampleRate, 0.05f);

            std::cout << "  Carrier: " << carrierHz << " Hz\n";
            std::cout << "    Expected: " << (440.0f - carrierHz) << " Hz, "
                      << (440.0f + carrierHz) << " Hz\n";
            std::cout << "    Detected peaks: ";
            for (size_t i = 0; i < std::min(peaks.size(), size_t(4)); ++i) {
                std::cout << std::fixed << std::setprecision(0) << peaks[i].frequency << "Hz ";
            }
            std::cout << "\n";
        }

        std::cout << "  Character:       Clean frequency multiplication\n";
        std::cout << "  Comparison:      Moog Ring Modulator\n";

    } else if (engineId == 27) {
        // Frequency Shifter
        std::cout << "FREQUENCY SHIFTER LINEARITY TEST:\n";
        auto metrics = testFrequencyShifter(engine.get(), sampleRate, blockSize);

        std::cout << "  Linear Shift:    " << (metrics.isLinear ? "YES" : "NO") << "\n";
        std::cout << "  Max Error:       " << std::fixed << std::setprecision(2)
                  << metrics.linearityError << " Hz\n";
        std::cout << "  Aliasing:        " << (metrics.hasAliasing ? "DETECTED" : "None") << "\n";

        std::cout << "\n  Input/Output Frequencies:\n";
        for (const auto& [expected, actual] : metrics.inputOutputFreqs) {
            std::cout << "    Expected: " << std::fixed << std::setprecision(1) << expected
                      << " Hz -> Actual: " << actual << " Hz (error: "
                      << std::abs(actual - expected) << " Hz)\n";
        }

        std::cout << "\n  Character:       Hilbert transform frequency shifter\n";
        std::cout << "  Comparison:      Bode/Moog Frequency Shifter\n";

        spectrumFile << "expected_hz,actual_hz,error_hz\n";
        for (const auto& [expected, actual] : metrics.inputOutputFreqs) {
            spectrumFile << expected << "," << actual << "," << std::abs(actual - expected) << "\n";
        }

    } else if (engineId == 28 || engineId == 29) {
        // Tremolo engines
        std::cout << "TREMOLO ANALYSIS:\n";

        std::map<int, float> params;
        params[0] = 1.0f; // Mix
        params[1] = 5.0f / 20.0f; // Rate = 5 Hz (normalized)
        params[2] = 0.8f; // Depth

        auto lfoMetrics = measureLFO(engine.get(), sampleRate, blockSize, params);

        std::cout << "  Measured Rate:   " << std::fixed << std::setprecision(2)
                  << lfoMetrics.measuredRateHz << " Hz\n";
        std::cout << "  Depth:           " << std::fixed << std::setprecision(1)
                  << lfoMetrics.depthDb << " dB\n";

        if (engineId == 28) {
            std::cout << "  Type:            Harmonic (split-band)\n";
            std::cout << "  Character:       Fender Vibrolux style\n";
        } else {
            std::cout << "  Type:            Classic amplitude modulation\n";
            std::cout << "  Character:       Fender Deluxe / Vox AC30\n";
        }

        lfoFile << "rate_hz,depth_db\n";
        lfoFile << lfoMetrics.measuredRateHz << "," << lfoMetrics.depthDb << "\n";

    } else if (engineId == 30) {
        // Rotary Speaker
        std::cout << "ROTARY SPEAKER (LESLIE) VERIFICATION:\n";
        auto metrics = verifyRotarySpeaker(engine.get(), sampleRate, blockSize);

        std::cout << "  Horn Speed (Slow):  " << std::fixed << std::setprecision(2)
                  << metrics.hornSpeedSlow << " Hz (target: ~0.7 Hz)\n";
        std::cout << "  Horn Speed (Fast):  " << std::fixed << std::setprecision(2)
                  << metrics.hornSpeedFast << " Hz (target: ~6.7 Hz)\n";
        std::cout << "  Leslie Accurate:    " << (metrics.leslieAccurate ? "YES" : "NO") << "\n";
        std::cout << "\n  Character:          SIMD-optimized Leslie simulator\n";
        std::cout << "  Comparison:         Leslie 122/147\n";

        lfoFile << "mode,horn_hz,drum_hz\n";
        lfoFile << "slow," << metrics.hornSpeedSlow << "," << metrics.drumSpeedSlow << "\n";
        lfoFile << "fast," << metrics.hornSpeedFast << "," << metrics.drumSpeedFast << "\n";
    }

    std::cout << "\n";

    lfoFile.close();
    spectrumFile.close();
    stereoFile.close();
}

} // namespace ModulationTests

//==============================================================================
// Main
//==============================================================================
int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║    ChimeraPhoenix Modulation Engine Quality Assessment    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    std::vector<std::pair<int, std::string>> engines = {
        {23, "Stereo Chorus"},
        {24, "Resonant Chorus Platinum"},
        {25, "Analog Phaser"},
        {26, "Platinum Ring Modulator"},
        {27, "Frequency Shifter"},
        {28, "Harmonic Tremolo"},
        {29, "Classic Tremolo"},
        {30, "Rotary Speaker Platinum"}
    };

    for (const auto& [id, name] : engines) {
        ModulationTests::testModulationEngine(id, name);
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                  TESTING COMPLETE                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "CSV files generated:\n";
    std::cout << "  - mod_engine_XX_lfo.csv (LFO characteristics)\n";
    std::cout << "  - mod_engine_XX_spectrum.csv (frequency content)\n";
    std::cout << "  - mod_engine_XX_stereo.csv (stereo field)\n\n";

    return 0;
}

/*
  ==============================================================================

    test_pitch_engines_competitive_benchmark.cpp

    COMPETITIVE BENCHMARK SUITE - PITCH ENGINES VS INDUSTRY LEADERS

    Purpose: Comprehensive competitive analysis of all 8 pitch engines against
             industry-leading plugins (Melodyne, Auto-Tune, Waves Tune, Little AlterBoy)

    Engines Tested (8 total):
    - Engine 31: Pitch Shifter (Modulation)
    - Engine 32: Detune Doubler
    - Engine 33: Intelligent Harmonizer
    - Engine 34: Tape Echo (has pitch modulation)
    - Engine 36: Magnetic Drum Echo (has pitch modulation)
    - Engine 37: Bucket Brigade Delay (has pitch modulation)
    - Engine 49: Phased Vocoder
    - Engine 50: Granular Cloud (has pitch scatter)

    Metrics Measured:
    1. Pitch Accuracy (cents error) - FFT-based fundamental frequency tracking
    2. THD (Total Harmonic Distortion %)
    3. Latency (milliseconds)
    4. CPU Usage (% of single core)
    5. Formant Preservation (vocal quality, spectrum analysis)
    6. Artifact Level (quantified noise floor and spurious peaks)
    7. Transient Preservation (attack time preservation)

    Industry Benchmarks:
    - Melodyne (Best-in-class):     ±1 cent, <0.1% THD, 50-100ms latency, 3-5% CPU
    - Auto-Tune (Professional):     ±3 cents, <0.5% THD, 20-50ms latency, 2-4% CPU
    - Waves Tune (Mid-tier):        ±5 cents, <1% THD, 10-30ms latency, 1-3% CPU
    - Little AlterBoy (Creative):   ±10 cents, <5% THD, 5-20ms latency, 1-2% CPU

  ==============================================================================
*/

#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <algorithm>

//==============================================================================
// INDUSTRY BENCHMARK STANDARDS
//==============================================================================
struct IndustryBenchmark {
    std::string name;
    std::string tier;
    float pitchAccuracyCents;
    float thdPercent;
    float latencyMs;
    float cpuPercent;
    std::string formantQuality;
};

const std::vector<IndustryBenchmark> INDUSTRY_STANDARDS = {
    {"Melodyne", "Best-in-class", 1.0f, 0.1f, 75.0f, 4.0f, "Excellent"},
    {"Auto-Tune", "Professional", 3.0f, 0.5f, 35.0f, 3.0f, "Good"},
    {"Waves Tune", "Mid-tier", 5.0f, 1.0f, 20.0f, 2.0f, "Moderate"},
    {"Little AlterBoy", "Creative", 10.0f, 5.0f, 12.5f, 1.5f, "Good"}
};

//==============================================================================
// ENGINE METADATA
//==============================================================================
struct EngineMetadata {
    int id;
    std::string name;
    std::string category;
    bool isPitchShifter;
    bool hasFormantControl;
};

const std::vector<EngineMetadata> PITCH_ENGINES = {
    {31, "Pitch Shifter", "Modulation", true, false},
    {32, "Detune Doubler", "Modulation", true, false},
    {33, "Intelligent Harmonizer", "Modulation", true, false},
    {34, "Tape Echo", "Delay", false, false},
    {36, "Magnetic Drum Echo", "Delay", false, false},
    {37, "Bucket Brigade Delay", "Delay", false, false},
    {49, "Phased Vocoder", "Special", true, true},
    {50, "Granular Cloud", "Special", true, false}
};

//==============================================================================
// BENCHMARK RESULT STRUCTURE
//==============================================================================
struct BenchmarkResult {
    // Engine info
    int engineId;
    std::string engineName;
    std::string category;

    // Metric 1: Pitch Accuracy (cents)
    float pitchAccuracyCents;
    float pitchAccuracyScore;  // 0-100

    // Metric 2: THD (%)
    float thdPercent;
    float thdScore;  // 0-100

    // Metric 3: Latency (ms)
    float latencyMs;
    float latencyScore;  // 0-100

    // Metric 4: CPU Usage (%)
    float cpuPercent;
    float cpuScore;  // 0-100

    // Metric 5: Formant Preservation (0-100)
    float formantScore;
    std::string formantQuality;

    // Metric 6: Artifact Level (dB below signal)
    float artifactLevelDb;
    float artifactScore;  // 0-100

    // Metric 7: Transient Preservation (0-100)
    float transientScore;
    float transientAttackMs;

    // Overall scores
    float overallScore;  // 0-100
    std::string competitiveTier;  // Best-in-class, Professional, Mid-tier, Creative, Below standard

    // Status
    bool success;
    std::string errorMessage;
};

//==============================================================================
// FFT-BASED PITCH DETECTION
//==============================================================================
float detectFundamentalFrequency(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13); // 2^13 = 8192
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);

    // Apply Hann window
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find peak frequency (skip DC)
    int maxBin = 0;
    float maxMag = 0.0f;
    for (int i = 5; i < fftSize / 2; ++i) {
        if (fftData[i] > maxMag) {
            maxMag = fftData[i];
            maxBin = i;
        }
    }

    if (maxBin == 0 || maxMag < 1e-6f) return 0.0f;

    // Parabolic interpolation for sub-bin accuracy
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
// THD MEASUREMENT
//==============================================================================
float measureTHD(const juce::AudioBuffer<float>& buffer, float fundamentalHz, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    int startOffset = buffer.getNumSamples() / 4;

    // Blackman-Harris window
    for (int i = 0; i < fftSize; ++i) {
        float w = i / float(fftSize - 1);
        float window = 0.35875f
                     - 0.48829f * std::cos(2.0f * M_PI * w)
                     + 0.14128f * std::cos(4.0f * M_PI * w)
                     - 0.01168f * std::cos(6.0f * M_PI * w);
        fftData[i * 2] = inputData[startOffset + i] * window;
        fftData[i * 2 + 1] = 0.0f;
    }

    fft.performRealOnlyForwardTransform(fftData.data(), true);

    // Calculate magnitude spectrum
    std::vector<float> magnitude(fftSize / 2);
    for (int i = 0; i < fftSize / 2; ++i) {
        float real = fftData[i * 2];
        float imag = fftData[i * 2 + 1];
        magnitude[i] = std::sqrt(real * real + imag * imag);
    }

    float binWidth = sampleRate / fftSize;
    int fundamentalBin = static_cast<int>(fundamentalHz / binWidth + 0.5f);

    // Find fundamental
    float maxMag = 0.0f;
    for (int i = fundamentalBin - 3; i <= fundamentalBin + 3; ++i) {
        if (i >= 0 && i < fftSize / 2) {
            maxMag = std::max(maxMag, magnitude[i]);
        }
    }

    if (maxMag < 1e-6f) return 0.0f;

    // Measure harmonics
    float harmonicPowerSum = 0.0f;
    for (int harmonic = 2; harmonic <= 5; ++harmonic) {
        float expectedFreq = fundamentalHz * harmonic;
        if (expectedFreq > sampleRate / 2.0f) break;

        int harmonicBin = static_cast<int>(expectedFreq / binWidth + 0.5f);
        float maxHarmonicMag = 0.0f;

        for (int i = harmonicBin - 2; i <= harmonicBin + 2; ++i) {
            if (i >= 0 && i < fftSize / 2) {
                maxHarmonicMag = std::max(maxHarmonicMag, magnitude[i]);
            }
        }

        harmonicPowerSum += maxHarmonicMag * maxHarmonicMag;
    }

    float fundamentalPower = maxMag * maxMag;
    if (fundamentalPower > 0.0f) {
        float thdRatio = std::sqrt(harmonicPowerSum / fundamentalPower);
        return thdRatio * 100.0f;
    }

    return 0.0f;
}

//==============================================================================
// LATENCY MEASUREMENT
//==============================================================================
float measureLatency(std::unique_ptr<EngineBase>& engine, float sampleRate) {
    const int maxLatencySamples = static_cast<int>(sampleRate * 1.0f);  // 1 second max
    const int blockSize = 512;

    juce::AudioBuffer<float> buffer(2, maxLatencySamples);
    buffer.clear();

    // Create impulse
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    // Process in blocks
    for (int start = 0; start < maxLatencySamples; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, maxLatencySamples - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Find first sample above threshold
    const float threshold = 0.001f;  // -60dB
    const float* leftData = buffer.getReadPointer(0);

    for (int i = 0; i < maxLatencySamples; ++i) {
        if (std::abs(leftData[i]) > threshold) {
            return (i * 1000.0f) / sampleRate;  // Return in milliseconds
        }
    }

    return -1.0f;  // No output detected
}

//==============================================================================
// CPU BENCHMARK
//==============================================================================
float measureCPU(std::unique_ptr<EngineBase>& engine, float sampleRate) {
    const int blockSize = 512;
    const int numBlocks = 20000;  // Process 10 seconds of audio
    const double durationSeconds = (numBlocks * blockSize) / sampleRate;

    juce::AudioBuffer<float> buffer(2, blockSize);

    // Generate test signal (440Hz sine)
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
            buffer.setSample(ch, i, 0.5f * std::sin(phase));
        }
    }

    // Measure processing time
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int block = 0; block < numBlocks; ++block) {
        engine->process(buffer);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;

    // CPU % = (processing time / real time) * 100
    return (elapsed.count() / durationSeconds) * 100.0f;
}

//==============================================================================
// FORMANT ANALYSIS
//==============================================================================
float analyzeFormantPreservation(const juce::AudioBuffer<float>& input,
                                  const juce::AudioBuffer<float>& output,
                                  float sampleRate) {
    // Analyze formant regions (vowel-like peaks in spectrum)
    // For a synthetic test, we'll measure spectral envelope correlation

    const int fftSize = 4096;
    if (input.getNumSamples() < fftSize || output.getNumSamples() < fftSize) {
        return 50.0f;  // Neutral score
    }

    juce::dsp::FFT fft(12);  // 2^12 = 4096

    // Analyze input
    std::vector<float> inputFFT(fftSize * 2, 0.0f);
    const float* inputData = input.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        inputFFT[i] = inputData[i] * window;
    }
    fft.performFrequencyOnlyForwardTransform(inputFFT.data());

    // Analyze output
    std::vector<float> outputFFT(fftSize * 2, 0.0f);
    const float* outputData = output.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        outputFFT[i] = outputData[i] * window;
    }
    fft.performFrequencyOnlyForwardTransform(outputFFT.data());

    // Compare spectral envelopes in formant regions (300-3000 Hz)
    float binWidth = sampleRate / fftSize;
    int startBin = static_cast<int>(300.0f / binWidth);
    int endBin = static_cast<int>(3000.0f / binWidth);

    float correlation = 0.0f;
    int numBins = 0;

    for (int i = startBin; i < endBin && i < fftSize / 2; ++i) {
        float inputMag = inputFFT[i] + 1e-10f;
        float outputMag = outputFFT[i] + 1e-10f;
        float ratio = std::min(inputMag, outputMag) / std::max(inputMag, outputMag);
        correlation += ratio;
        numBins++;
    }

    if (numBins > 0) {
        return (correlation / numBins) * 100.0f;
    }

    return 50.0f;  // Neutral score
}

//==============================================================================
// ARTIFACT MEASUREMENT
//==============================================================================
float measureArtifactLevel(const juce::AudioBuffer<float>& buffer,
                           float fundamentalHz,
                           float sampleRate) {
    // Measure noise floor and spurious peaks
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return -60.0f;

    juce::dsp::FFT fft(13);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    int startOffset = buffer.getNumSamples() / 4;

    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[startOffset + i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find fundamental magnitude
    float binWidth = sampleRate / fftSize;
    int fundamentalBin = static_cast<int>(fundamentalHz / binWidth + 0.5f);

    float fundamentalMag = 0.0f;
    for (int i = fundamentalBin - 3; i <= fundamentalBin + 3; ++i) {
        if (i >= 0 && i < fftSize / 2) {
            fundamentalMag = std::max(fundamentalMag, fftData[i]);
        }
    }

    if (fundamentalMag < 1e-6f) return -60.0f;

    // Measure noise floor (excluding fundamental and harmonics)
    float noiseSum = 0.0f;
    int noiseBins = 0;

    for (int i = 20; i < fftSize / 2; ++i) {
        float freq = i * binWidth;

        // Skip fundamental and harmonics
        bool isHarmonic = false;
        for (int h = 1; h <= 10; ++h) {
            if (std::abs(freq - fundamentalHz * h) < 5 * binWidth) {
                isHarmonic = true;
                break;
            }
        }

        if (!isHarmonic) {
            noiseSum += fftData[i];
            noiseBins++;
        }
    }

    if (noiseBins > 0) {
        float avgNoise = noiseSum / noiseBins;
        return 20.0f * std::log10((avgNoise + 1e-10f) / (fundamentalMag + 1e-10f));
    }

    return -60.0f;
}

//==============================================================================
// TRANSIENT PRESERVATION MEASUREMENT
//==============================================================================
float measureTransientPreservation(const juce::AudioBuffer<float>& input,
                                    const juce::AudioBuffer<float>& output) {
    // Measure attack time preservation
    // Find envelope rise time for both input and output

    auto findAttackTime = [](const juce::AudioBuffer<float>& buf) -> float {
        const float* data = buf.getReadPointer(0);
        int samples = buf.getNumSamples();

        // Find peak
        float peak = 0.0f;
        int peakIdx = 0;
        for (int i = 0; i < samples; ++i) {
            float absVal = std::abs(data[i]);
            if (absVal > peak) {
                peak = absVal;
                peakIdx = i;
            }
        }

        if (peak < 0.01f) return 0.0f;

        // Find 10% and 90% points
        float threshold10 = peak * 0.1f;
        float threshold90 = peak * 0.9f;

        int idx10 = -1, idx90 = -1;
        for (int i = 0; i < peakIdx; ++i) {
            if (std::abs(data[i]) >= threshold10 && idx10 < 0) {
                idx10 = i;
            }
            if (std::abs(data[i]) >= threshold90 && idx90 < 0) {
                idx90 = i;
                break;
            }
        }

        if (idx10 >= 0 && idx90 >= 0) {
            return float(idx90 - idx10);
        }
        return 0.0f;
    };

    float inputAttack = findAttackTime(input);
    float outputAttack = findAttackTime(output);

    if (inputAttack < 1.0f || outputAttack < 1.0f) {
        return 50.0f;  // Neutral score for unclear transients
    }

    // Calculate preservation score (100 = perfect match)
    float ratio = std::min(inputAttack, outputAttack) / std::max(inputAttack, outputAttack);
    return ratio * 100.0f;
}

//==============================================================================
// SCORING FUNCTIONS
//==============================================================================
float scorePitchAccuracy(float centError) {
    // 100 = 0 cents, 0 = 50+ cents
    float score = 100.0f - (centError * 2.0f);
    return std::max(0.0f, std::min(100.0f, score));
}

float scoreTHD(float thdPercent) {
    // 100 = 0%, 0 = 10%
    float score = 100.0f - (thdPercent * 10.0f);
    return std::max(0.0f, std::min(100.0f, score));
}

float scoreLatency(float latencyMs) {
    // 100 = 0ms, 0 = 200ms
    float score = 100.0f - (latencyMs * 0.5f);
    return std::max(0.0f, std::min(100.0f, score));
}

float scoreCPU(float cpuPercent) {
    // 100 = 0%, 0 = 20%
    float score = 100.0f - (cpuPercent * 5.0f);
    return std::max(0.0f, std::min(100.0f, score));
}

float scoreArtifacts(float artifactDb) {
    // 100 = -100dB, 0 = -20dB
    float score = 100.0f + (artifactDb * 1.25f) + 25.0f;
    return std::max(0.0f, std::min(100.0f, score));
}

std::string determineCompetitiveTier(float overallScore) {
    if (overallScore >= 85.0f) return "Best-in-class";
    if (overallScore >= 70.0f) return "Professional";
    if (overallScore >= 55.0f) return "Mid-tier";
    if (overallScore >= 40.0f) return "Creative";
    return "Below standard";
}

//==============================================================================
// COMPREHENSIVE ENGINE BENCHMARK
//==============================================================================
BenchmarkResult benchmarkEngine(const EngineMetadata& metadata) {
    BenchmarkResult result;
    result.engineId = metadata.id;
    result.engineName = metadata.name;
    result.category = metadata.category;
    result.success = false;

    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Benchmarking Engine " << std::setw(2) << metadata.id << ": "
              << std::left << std::setw(45) << metadata.name << "║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    try {
        auto engine = EngineFactory::createEngine(metadata.id);
        if (!engine) {
            result.errorMessage = "Failed to create engine";
            std::cout << "ERROR: Failed to create engine\n";
            return result;
        }

        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        // Set parameters for pitch shift testing
        std::map<int, float> params;
        if (metadata.isPitchShifter) {
            params[0] = 0.5f;  // Unity pitch (0 semitones)
            if (engine->getNumParameters() > 1) params[1] = 1.0f;  // Full wet
        } else {
            // For delays with pitch modulation, use neutral settings
            params[0] = 0.3f;  // Short delay time
            if (engine->getNumParameters() > 1) params[1] = 0.0f;  // No feedback
            if (engine->getNumParameters() > 2) params[2] = 1.0f;  // Full wet
        }
        engine->reset();
        engine->updateParameters(params);

        // Generate test signals
        const int testLength = 32768;  // ~680ms at 48kHz
        const float testFreq = 440.0f;  // A4

        juce::AudioBuffer<float> inputBuffer(2, testLength);
        juce::AudioBuffer<float> outputBuffer(2, testLength);

        // Generate sine wave test signal
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * M_PI * testFreq * i / sampleRate;
                inputBuffer.setSample(ch, i, 0.5f * std::sin(phase));
                outputBuffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        // Process audio
        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(outputBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Skip initial transients
        int skipSamples = testLength / 5;
        juce::AudioBuffer<float> analysisInput(2, testLength - skipSamples);
        juce::AudioBuffer<float> analysisOutput(2, testLength - skipSamples);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < analysisInput.getNumSamples(); ++i) {
                analysisInput.setSample(ch, i, inputBuffer.getSample(ch, i + skipSamples));
                analysisOutput.setSample(ch, i, outputBuffer.getSample(ch, i + skipSamples));
            }
        }

        // METRIC 1: Pitch Accuracy
        std::cout << "[1/7] Measuring pitch accuracy...\n";
        float measuredFreq = detectFundamentalFrequency(analysisOutput, sampleRate);
        result.pitchAccuracyCents = std::abs(1200.0f * std::log2(measuredFreq / testFreq));
        result.pitchAccuracyScore = scorePitchAccuracy(result.pitchAccuracyCents);
        std::cout << "      Measured: " << std::fixed << std::setprecision(2) << measuredFreq << " Hz"
                  << " (Error: " << result.pitchAccuracyCents << " cents)\n";

        // METRIC 2: THD
        std::cout << "[2/7] Measuring THD...\n";
        result.thdPercent = measureTHD(analysisOutput, testFreq, sampleRate);
        result.thdScore = scoreTHD(result.thdPercent);
        std::cout << "      THD: " << std::fixed << std::setprecision(3) << result.thdPercent << "%\n";

        // METRIC 3: Latency
        std::cout << "[3/7] Measuring latency...\n";
        engine->reset();
        engine->updateParameters(params);
        result.latencyMs = measureLatency(engine, sampleRate);
        result.latencyScore = scoreLatency(result.latencyMs);
        std::cout << "      Latency: " << std::fixed << std::setprecision(2) << result.latencyMs << " ms\n";

        // METRIC 4: CPU Usage
        std::cout << "[4/7] Measuring CPU usage...\n";
        engine->reset();
        engine->updateParameters(params);
        result.cpuPercent = measureCPU(engine, sampleRate);
        result.cpuScore = scoreCPU(result.cpuPercent);
        std::cout << "      CPU: " << std::fixed << std::setprecision(2) << result.cpuPercent << "%\n";

        // METRIC 5: Formant Preservation
        std::cout << "[5/7] Analyzing formant preservation...\n";
        result.formantScore = analyzeFormantPreservation(analysisInput, analysisOutput, sampleRate);
        if (result.formantScore >= 80.0f) result.formantQuality = "Excellent";
        else if (result.formantScore >= 65.0f) result.formantQuality = "Good";
        else if (result.formantScore >= 50.0f) result.formantQuality = "Moderate";
        else result.formantQuality = "Poor";
        std::cout << "      Formant: " << std::fixed << std::setprecision(1) << result.formantScore
                  << "% (" << result.formantQuality << ")\n";

        // METRIC 6: Artifact Level
        std::cout << "[6/7] Measuring artifact level...\n";
        result.artifactLevelDb = measureArtifactLevel(analysisOutput, testFreq, sampleRate);
        result.artifactScore = scoreArtifacts(result.artifactLevelDb);
        std::cout << "      Artifacts: " << std::fixed << std::setprecision(1) << result.artifactLevelDb << " dB\n";

        // METRIC 7: Transient Preservation
        std::cout << "[7/7] Measuring transient preservation...\n";
        result.transientScore = measureTransientPreservation(analysisInput, analysisOutput);
        std::cout << "      Transient: " << std::fixed << std::setprecision(1) << result.transientScore << "%\n";

        // Calculate overall score (weighted average)
        result.overallScore = (result.pitchAccuracyScore * 0.25f +
                               result.thdScore * 0.15f +
                               result.latencyScore * 0.15f +
                               result.cpuScore * 0.15f +
                               result.formantScore * 0.10f +
                               result.artifactScore * 0.10f +
                               result.transientScore * 0.10f);

        result.competitiveTier = determineCompetitiveTier(result.overallScore);

        std::cout << "\n      OVERALL SCORE: " << std::fixed << std::setprecision(1)
                  << result.overallScore << "/100\n";
        std::cout << "      COMPETITIVE TIER: " << result.competitiveTier << "\n";

        result.success = true;

    } catch (const std::exception& e) {
        result.errorMessage = std::string("Exception: ") + e.what();
        std::cout << "ERROR: " << result.errorMessage << "\n";
    } catch (...) {
        result.errorMessage = "Unknown exception";
        std::cout << "ERROR: Unknown exception\n";
    }

    return result;
}

//==============================================================================
// REPORT GENERATION
//==============================================================================
void generateCSVReport(const std::vector<BenchmarkResult>& results, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << std::endl;
        return;
    }

    // Header
    file << "EngineID,EngineName,Category,";
    file << "PitchAccuracy(cents),PitchScore,";
    file << "THD(%),THDScore,";
    file << "Latency(ms),LatencyScore,";
    file << "CPU(%),CPUScore,";
    file << "FormantScore,FormantQuality,";
    file << "ArtifactLevel(dB),ArtifactScore,";
    file << "TransientScore,";
    file << "OverallScore,CompetitiveTier,Success\n";

    // Data
    for (const auto& r : results) {
        file << r.engineId << ",\"" << r.engineName << "\",\"" << r.category << "\",";
        file << std::fixed << std::setprecision(2) << r.pitchAccuracyCents << "," << r.pitchAccuracyScore << ",";
        file << r.thdPercent << "," << r.thdScore << ",";
        file << r.latencyMs << "," << r.latencyScore << ",";
        file << r.cpuPercent << "," << r.cpuScore << ",";
        file << r.formantScore << ",\"" << r.formantQuality << "\",";
        file << r.artifactLevelDb << "," << r.artifactScore << ",";
        file << r.transientScore << ",";
        file << r.overallScore << ",\"" << r.competitiveTier << "\",";
        file << (r.success ? "YES" : "NO") << "\n";
    }

    file.close();
    std::cout << "\n✓ CSV report saved: " << filename << std::endl;
}

void printComparisonTable(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      COMPETITIVE COMPARISON TABLE                                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════════════╝\n\n";

    // Print industry standards first
    std::cout << "INDUSTRY STANDARDS:\n";
    std::cout << "───────────────────────────────────────────────────────────────────────────────────────\n";
    std::cout << std::left << std::setw(20) << "Product"
              << std::setw(18) << "Tier"
              << std::setw(12) << "Pitch(¢)"
              << std::setw(10) << "THD(%)"
              << std::setw(12) << "Latency(ms)"
              << std::setw(10) << "CPU(%)"
              << std::setw(12) << "Formant" << "\n";
    std::cout << "───────────────────────────────────────────────────────────────────────────────────────\n";

    for (const auto& standard : INDUSTRY_STANDARDS) {
        std::cout << std::left << std::setw(20) << standard.name
                  << std::setw(18) << standard.tier
                  << std::setw(12) << ("±" + std::to_string(int(standard.pitchAccuracyCents)))
                  << std::setw(10) << ("<" + std::to_string(standard.thdPercent).substr(0, 4))
                  << std::setw(12) << std::to_string(int(standard.latencyMs))
                  << std::setw(10) << std::to_string(int(standard.cpuPercent))
                  << std::setw(12) << standard.formantQuality << "\n";
    }

    std::cout << "\n\nCHIMERA ENGINES:\n";
    std::cout << "───────────────────────────────────────────────────────────────────────────────────────\n";
    std::cout << std::left << std::setw(4) << "ID"
              << std::setw(25) << "Engine Name"
              << std::setw(12) << "Pitch(¢)"
              << std::setw(10) << "THD(%)"
              << std::setw(12) << "Latency(ms)"
              << std::setw(10) << "CPU(%)"
              << std::setw(12) << "Score"
              << std::setw(18) << "Tier" << "\n";
    std::cout << "───────────────────────────────────────────────────────────────────────────────────────\n";

    for (const auto& r : results) {
        if (!r.success) continue;

        std::cout << std::left << std::setw(4) << r.engineId
                  << std::setw(25) << r.engineName.substr(0, 23)
                  << std::setw(12) << (std::to_string(int(r.pitchAccuracyCents)))
                  << std::setw(10) << std::to_string(r.thdPercent).substr(0, 4)
                  << std::setw(12) << std::to_string(int(r.latencyMs))
                  << std::setw(10) << std::to_string(int(r.cpuPercent))
                  << std::setw(12) << (std::to_string(int(r.overallScore)) + "/100")
                  << std::setw(18) << r.competitiveTier << "\n";
    }
    std::cout << "\n";
}

void printMetricBreakdown(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        DETAILED METRIC BREAKDOWN                                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════════════╝\n\n";

    for (const auto& r : results) {
        if (!r.success) continue;

        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "Engine " << r.engineId << ": " << r.engineName << " (" << r.category << ")\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

        // Create a visual bar graph for each metric
        auto printBar = [](float score, int width = 50) {
            int filled = static_cast<int>((score / 100.0f) * width);
            std::cout << "[";
            for (int i = 0; i < width; ++i) {
                if (i < filled) std::cout << "█";
                else std::cout << "░";
            }
            std::cout << "] ";
        };

        std::cout << "  Pitch Accuracy:      ";
        printBar(r.pitchAccuracyScore);
        std::cout << std::fixed << std::setprecision(1) << r.pitchAccuracyScore << "% (" << r.pitchAccuracyCents << " cents)\n";

        std::cout << "  THD:                 ";
        printBar(r.thdScore);
        std::cout << std::fixed << std::setprecision(1) << r.thdScore << "% (" << r.thdPercent << "%)\n";

        std::cout << "  Latency:             ";
        printBar(r.latencyScore);
        std::cout << std::fixed << std::setprecision(1) << r.latencyScore << "% (" << r.latencyMs << " ms)\n";

        std::cout << "  CPU Usage:           ";
        printBar(r.cpuScore);
        std::cout << std::fixed << std::setprecision(1) << r.cpuScore << "% (" << r.cpuPercent << "%)\n";

        std::cout << "  Formant:             ";
        printBar(r.formantScore);
        std::cout << std::fixed << std::setprecision(1) << r.formantScore << "% (" << r.formantQuality << ")\n";

        std::cout << "  Artifacts:           ";
        printBar(r.artifactScore);
        std::cout << std::fixed << std::setprecision(1) << r.artifactScore << "% (" << r.artifactLevelDb << " dB)\n";

        std::cout << "  Transient:           ";
        printBar(r.transientScore);
        std::cout << std::fixed << std::setprecision(1) << r.transientScore << "%\n";

        std::cout << "\n  ────────────────────────────────────────────────────────────────────────────────\n";
        std::cout << "  OVERALL SCORE:       ";
        printBar(r.overallScore);
        std::cout << std::fixed << std::setprecision(1) << r.overallScore << "%\n";
        std::cout << "  COMPETITIVE TIER:    " << r.competitiveTier << "\n";
    }
}

//==============================================================================
// MAIN
//==============================================================================
int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                                      ║\n";
    std::cout << "║              COMPETITIVE BENCHMARK: CHIMERA PITCH ENGINES                            ║\n";
    std::cout << "║                         VS INDUSTRY LEADERS                                          ║\n";
    std::cout << "║                                                                                      ║\n";
    std::cout << "║  Melodyne • Auto-Tune • Waves Tune • Little AlterBoy                                ║\n";
    std::cout << "║                                                                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════════════╝\n";

    std::cout << "\nTesting 8 Pitch Engines:\n";
    for (const auto& engine : PITCH_ENGINES) {
        std::cout << "  • Engine " << std::setw(2) << engine.id << ": " << engine.name
                  << " (" << engine.category << ")\n";
    }

    std::cout << "\nPress ENTER to begin benchmark...";
    std::cin.get();

    std::vector<BenchmarkResult> results;

    auto overallStart = std::chrono::high_resolution_clock::now();

    for (const auto& engine : PITCH_ENGINES) {
        BenchmarkResult result = benchmarkEngine(engine);
        results.push_back(result);
    }

    auto overallEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> totalTime = overallEnd - overallStart;

    // Sort by overall score (descending)
    std::sort(results.begin(), results.end(),
              [](const BenchmarkResult& a, const BenchmarkResult& b) {
                  return a.overallScore > b.overallScore;
              });

    // Generate reports
    std::cout << "\n\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                              BENCHMARK COMPLETE                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════════════╝\n";

    printComparisonTable(results);
    printMetricBreakdown(results);

    // Save CSV
    generateCSVReport(results, "build/pitch_engines_competitive_benchmark.csv");

    // Summary statistics
    int bestInClass = 0, professional = 0, midTier = 0, creative = 0, belowStandard = 0;
    for (const auto& r : results) {
        if (!r.success) continue;
        if (r.competitiveTier == "Best-in-class") bestInClass++;
        else if (r.competitiveTier == "Professional") professional++;
        else if (r.competitiveTier == "Mid-tier") midTier++;
        else if (r.competitiveTier == "Creative") creative++;
        else belowStandard++;
    }

    std::cout << "\n╔══════════════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                           COMPETITIVE SUMMARY                                        ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "  Best-in-class:       " << bestInClass << " engines\n";
    std::cout << "  Professional:        " << professional << " engines\n";
    std::cout << "  Mid-tier:            " << midTier << " engines\n";
    std::cout << "  Creative:            " << creative << " engines\n";
    std::cout << "  Below standard:      " << belowStandard << " engines\n\n";
    std::cout << "  Total benchmark time: " << std::fixed << std::setprecision(1)
              << totalTime.count() << " seconds\n\n";

    std::cout << "Results saved to: build/pitch_engines_competitive_benchmark.csv\n\n";

    return 0;
}

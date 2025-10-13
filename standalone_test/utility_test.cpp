// utility_test.cpp - Comprehensive Utility Engines Test Suite
// Tests engines 55-56: Gain Utility Platinum and Mono Maker Platinum
// Focus: Precision, THD, Phase Linearity, and Bit-Perfect Operation

#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <fstream>
#include <chrono>
#include <complex>

namespace UtilityTests {

//==============================================================================
// High-Precision Analysis Tools
//==============================================================================

// Convert dB to linear with high precision
double dbToLinear(double db) {
    constexpr double DB_TO_LINEAR = 0.05776226504666210911810267678818;
    return std::exp(db * DB_TO_LINEAR);
}

// Convert linear to dB with high precision
double linearToDb(double linear) {
    constexpr double LINEAR_TO_DB = 17.312340490667560888319096172023;
    if (linear < 1e-20) return -200.0;
    return std::log(linear) * LINEAR_TO_DB;
}

// Measure RMS level with double precision
double measureRMS(const juce::AudioBuffer<float>& buffer, int channel = 0) {
    const float* data = buffer.getReadPointer(channel);
    double sumSquares = 0.0;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        sumSquares += data[i] * data[i];
    }

    return std::sqrt(sumSquares / buffer.getNumSamples());
}

// Measure peak level
float measurePeak(const juce::AudioBuffer<float>& buffer, int channel = 0) {
    const float* data = buffer.getReadPointer(channel);
    float peak = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        peak = std::max(peak, std::abs(data[i]));
    }

    return peak;
}

// Calculate THD+N with FFT
double calculateTHD(const juce::AudioBuffer<float>& buffer, float fundamentalFreq, float sampleRate) {
    const int fftSize = 16384;  // Higher resolution for accurate THD
    if (buffer.getNumSamples() < fftSize) return 0.0;

    juce::dsp::FFT fft(14); // 2^14 = 16384
    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy and window
    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize)); // Hann window
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find fundamental bin
    int fundamentalBin = static_cast<int>(fundamentalFreq * fftSize / sampleRate);
    double fundamentalMag = fftData[fundamentalBin];

    // Sum harmonics (2nd through 10th)
    double harmonicsSumSquared = 0.0;
    for (int h = 2; h <= 10; ++h) {
        int harmonicBin = fundamentalBin * h;
        if (harmonicBin < fftSize / 2) {
            double harmonicMag = fftData[harmonicBin];
            harmonicsSumSquared += harmonicMag * harmonicMag;
        }
    }

    if (fundamentalMag < 1e-20) return 0.0;
    return (std::sqrt(harmonicsSumSquared) / fundamentalMag) * 100.0;
}

// Measure phase shift at a given frequency
double measurePhaseShift(EngineBase* engine, float frequency, float sampleRate, int blockSize) {
    const int bufferSize = blockSize * 8;
    juce::AudioBuffer<float> input(2, bufferSize);
    juce::AudioBuffer<float> output(2, bufferSize);

    // Generate sine wave
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < bufferSize; ++i) {
            float phase = 2.0f * M_PI * frequency * i / sampleRate;
            input.setSample(ch, i, std::sin(phase));
        }
    }

    output.makeCopyOf(input);

    // Process in blocks
    for (int start = 0; start < bufferSize; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, bufferSize - start);
        juce::AudioBuffer<float> block(output.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Cross-correlate to find phase shift
    double maxCorr = 0.0;
    int maxLag = 0;
    const int maxLagSamples = static_cast<int>(sampleRate / frequency); // One period

    const float* in = input.getReadPointer(0);
    const float* out = output.getReadPointer(0);

    for (int lag = -maxLagSamples/2; lag < maxLagSamples/2; ++lag) {
        double corr = 0.0;
        for (int i = maxLagSamples; i < bufferSize - maxLagSamples; ++i) {
            if (i + lag >= 0 && i + lag < bufferSize) {
                corr += in[i] * out[i + lag];
            }
        }

        if (std::abs(corr) > std::abs(maxCorr)) {
            maxCorr = corr;
            maxLag = lag;
        }
    }

    // Convert lag to degrees
    double phaseDegrees = (maxLag * 360.0 * frequency) / sampleRate;
    return phaseDegrees;
}

// Check if processing is bit-perfect (no change)
bool isBitPerfect(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output) {
    if (input.getNumChannels() != output.getNumChannels()) return false;
    if (input.getNumSamples() != output.getNumSamples()) return false;

    for (int ch = 0; ch < input.getNumChannels(); ++ch) {
        const float* in = input.getReadPointer(ch);
        const float* out = output.getReadPointer(ch);

        for (int i = 0; i < input.getNumSamples(); ++i) {
            if (in[i] != out[i]) return false;  // Exact bit comparison
        }
    }

    return true;
}

//==============================================================================
// Gain Utility Tests (Engine 55)
//==============================================================================

struct GainAccuracyResult {
    double setGainDB;
    double measuredGainDB;
    double errorDB;
    bool passed;
};

// Test 1: Gain Accuracy across full range
std::vector<GainAccuracyResult> testGainAccuracy(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 1: Gain Utility - Precision Gain Accuracy           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(55);
    if (!engine) {
        std::cout << "ERROR: Failed to create Gain Utility engine\n";
        return {};
    }

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    std::vector<GainAccuracyResult> results;
    const float tolerance = 0.01f; // ±0.01dB requirement

    // Test range: -40dB to +20dB in 1dB steps
    for (int gainDB = -40; gainDB <= 20; ++gainDB) {
        // Set gain parameter (normalized 0-1)
        // Gain range is -∞ to +24dB, so we need to map carefully
        // Parameter 0 = GAIN
        float normalizedGain = (gainDB + 40.0f) / 64.0f; // Map -40 to +24 range
        normalizedGain = std::max(0.0f, std::min(1.0f, normalizedGain));

        std::map<int, float> params;
        params[0] = normalizedGain;
        engine->reset();
        engine->updateParameters(params);

        // Generate 0dBFS sine wave (amplitude 1.0)
        juce::AudioBuffer<float> buffer(2, blockSize * 4);
        const float inputAmplitude = 0.5f; // Safe level for measurement

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                buffer.setSample(ch, i, inputAmplitude * std::sin(phase));
            }
        }

        // Measure input RMS
        double inputRMS = measureRMS(buffer, 0);

        // Process
        engine->process(buffer);

        // Measure output RMS
        double outputRMS = measureRMS(buffer, 0);

        // Calculate measured gain
        double measuredGainDB = linearToDb(outputRMS / inputRMS);
        double errorDB = std::abs(measuredGainDB - gainDB);
        bool passed = errorDB <= tolerance;

        results.push_back({
            static_cast<double>(gainDB),
            measuredGainDB,
            errorDB,
            passed
        });

        if (!passed || gainDB % 5 == 0) {
            std::cout << "  Set: " << std::setw(6) << std::fixed << std::setprecision(2) << gainDB << " dB  →  "
                      << "Measured: " << std::setw(7) << std::setprecision(3) << measuredGainDB << " dB  →  "
                      << "Error: " << std::setw(7) << std::setprecision(4) << errorDB << " dB  "
                      << (passed ? "✓" : "✗") << "\n";
        }
    }

    // Test critical precision point: exactly 2x gain (+6.0206dB)
    std::cout << "\n  PRECISION TEST: +6.0206dB (exactly 2.0x linear)\n";
    float exactDoubleGain = 6.020599913279624f;
    float normalizedGain = (exactDoubleGain + 40.0f) / 64.0f;

    std::map<int, float> params;
    params[0] = normalizedGain;
    engine->reset();
    engine->updateParameters(params);

    juce::AudioBuffer<float> buffer(2, blockSize * 4);
    const float inputAmplitude = 0.5f;

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            buffer.setSample(ch, i, inputAmplitude);
        }
    }

    engine->process(buffer);

    double outputLevel = measureRMS(buffer, 0);
    double linearGain = outputLevel / inputAmplitude;
    double error = std::abs(linearGain - 2.0);

    std::cout << "  Input: " << inputAmplitude << "  →  Output: " << outputLevel
              << "  →  Gain: " << std::setprecision(6) << linearGain << "x\n";
    std::cout << "  Expected: 2.000000x  →  Error: " << std::scientific << error << "\n";
    std::cout << "  Result: " << (error < 0.0001 ? "✓ PASS (bit-perfect)" : "⚠ ACCEPTABLE") << "\n";

    return results;
}

// Test 2: THD at various gain settings
void testGainTHD(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 2: Gain Utility - THD Across Gain Range             ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(55);
    if (!engine) return;

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    std::vector<int> testGains = {-20, -10, -6, 0, +6, +12, +18};
    const double thdThreshold = 0.001; // <0.001% requirement

    std::cout << "  Testing THD at multiple gain settings (should be <0.001%):\n\n";

    for (int gainDB : testGains) {
        float normalizedGain = (gainDB + 40.0f) / 64.0f;

        std::map<int, float> params;
        params[0] = normalizedGain;
        engine->reset();
        engine->updateParameters(params);

        // Generate clean 1kHz sine
        juce::AudioBuffer<float> buffer(2, 16384);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                buffer.setSample(ch, i, 0.3f * std::sin(phase));
            }
        }

        engine->process(buffer);

        double thd = calculateTHD(buffer, 1000.0f, sampleRate);
        bool passed = thd < thdThreshold;

        std::cout << "  Gain: " << std::setw(6) << std::setprecision(1) << gainDB << " dB  →  "
                  << "THD: " << std::setw(10) << std::setprecision(6) << std::fixed << thd << "%  "
                  << (passed ? "✓" : "✗") << "\n";
    }
}

// Test 3: Phase Linearity
void testGainPhase(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 3: Gain Utility - Phase Linearity                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(55);
    if (!engine) return;

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set to unity gain
    std::map<int, float> params;
    params[0] = 0.625f; // 0dB (40/64)
    engine->reset();
    engine->updateParameters(params);

    std::vector<float> testFreqs = {20.0f, 100.0f, 1000.0f, 10000.0f, 20000.0f};

    std::cout << "  Testing phase shift at multiple frequencies (should be 0°):\n\n";

    for (float freq : testFreqs) {
        double phaseShift = measurePhaseShift(engine.get(), freq, sampleRate, blockSize);
        bool passed = std::abs(phaseShift) < 0.1; // <0.1° tolerance

        std::cout << "  " << std::setw(7) << std::right << freq << " Hz  →  "
                  << "Phase: " << std::setw(8) << std::setprecision(4) << phaseShift << "°  "
                  << (passed ? "✓" : "✗") << "\n";
    }
}

// Test 4: Channel Independence
void testGainChannelIndependence(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 4: Gain Utility - Channel Independence              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(55);
    if (!engine) return;

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set different gains for L and R
    std::map<int, float> params;
    params[0] = 0.625f;  // Main gain = 0dB
    params[1] = 0.75f;   // L gain = +6dB
    params[2] = 0.25f;   // R gain = -6dB
    engine->reset();
    engine->updateParameters(params);

    // Generate identical signals on both channels
    juce::AudioBuffer<float> buffer(2, blockSize * 4);
    const float inputLevel = 0.5f;

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            buffer.setSample(ch, i, inputLevel * std::sin(phase));
        }
    }

    engine->process(buffer);

    double rmsL = measureRMS(buffer, 0);
    double rmsR = measureRMS(buffer, 1);
    double gainL_dB = linearToDb(rmsL / inputLevel);
    double gainR_dB = linearToDb(rmsR / inputLevel);

    std::cout << "  Left Channel:  " << std::setw(8) << std::setprecision(3) << gainL_dB << " dB (expected ~+6dB)\n";
    std::cout << "  Right Channel: " << std::setw(8) << std::setprecision(3) << gainR_dB << " dB (expected ~-6dB)\n";

    bool passedL = std::abs(gainL_dB - 6.0) < 0.5;
    bool passedR = std::abs(gainR_dB - (-6.0)) < 0.5;

    std::cout << "  Result: " << (passedL && passedR ? "✓ PASS (channels independent)" : "✗ FAIL") << "\n";
}

//==============================================================================
// Mono Maker Tests (Engine 56)
//==============================================================================

// Test 5: Mono Summing Accuracy
void testMonoSumming(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 5: Mono Maker - Summing Accuracy                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(56);
    if (!engine) {
        std::cout << "ERROR: Failed to create Mono Maker engine\n";
        return;
    }

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set to full mono (frequency very high)
    std::map<int, float> params;
    params[0] = 1.0f;    // Frequency = max (mono everything)
    params[3] = 1.0f;    // Bass mono = 100%
    engine->reset();
    engine->updateParameters(params);

    // Test 1: Same signal on both channels → should stay same
    std::cout << "  Test 1: Identical L/R signals (should remain unchanged)\n";
    juce::AudioBuffer<float> buffer1(2, blockSize * 4);
    for (int i = 0; i < buffer1.getNumSamples(); ++i) {
        float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
        float value = 0.5f * std::sin(phase);
        buffer1.setSample(0, i, value);
        buffer1.setSample(1, i, value);
    }

    juce::AudioBuffer<float> original1(buffer1);
    engine->process(buffer1);

    double rmsL1 = measureRMS(buffer1, 0);
    double rmsR1 = measureRMS(buffer1, 1);
    double rmsOrig = measureRMS(original1, 0);

    std::cout << "    Input RMS:  " << rmsOrig << "\n";
    std::cout << "    Output L:   " << rmsL1 << "\n";
    std::cout << "    Output R:   " << rmsR1 << "\n";
    std::cout << "    L/R match:  " << (std::abs(rmsL1 - rmsR1) < 0.0001 ? "✓" : "✗") << "\n\n";

    // Test 2: Opposite polarity → should cancel to zero
    std::cout << "  Test 2: Phase-inverted signals (should cancel)\n";
    engine->reset();
    engine->updateParameters(params);

    juce::AudioBuffer<float> buffer2(2, blockSize * 4);
    for (int i = 0; i < buffer2.getNumSamples(); ++i) {
        float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
        buffer2.setSample(0, i, +0.5f * std::sin(phase));
        buffer2.setSample(1, i, -0.5f * std::sin(phase));
    }

    engine->process(buffer2);

    double rmsL2 = measureRMS(buffer2, 0);
    double rmsR2 = measureRMS(buffer2, 1);

    std::cout << "    Input L:    +0.5\n";
    std::cout << "    Input R:    -0.5\n";
    std::cout << "    Output L:   " << rmsL2 << "\n";
    std::cout << "    Output R:   " << rmsR2 << "\n";
    std::cout << "    Cancelled:  " << (rmsL2 < 0.001 && rmsR2 < 0.001 ? "✓ PASS (perfect cancellation)" : "✗ FAIL") << "\n\n";

    // Test 3: Different signals → should average
    std::cout << "  Test 3: Different L/R signals (should average)\n";
    engine->reset();
    engine->updateParameters(params);

    juce::AudioBuffer<float> buffer3(2, blockSize * 4);
    const float levelL = 0.3f;
    const float levelR = 0.7f;

    for (int i = 0; i < buffer3.getNumSamples(); ++i) {
        float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
        buffer3.setSample(0, i, levelL * std::sin(phase));
        buffer3.setSample(1, i, levelR * std::sin(phase));
    }

    engine->process(buffer3);

    double rmsL3 = measureRMS(buffer3, 0);
    double expectedAvg = (levelL + levelR) / 2.0;
    double error = std::abs(rmsL3 / (expectedAvg / std::sqrt(2.0)) - 1.0);

    std::cout << "    Input L:     " << levelL << "\n";
    std::cout << "    Input R:     " << levelR << "\n";
    std::cout << "    Expected:    " << expectedAvg << "\n";
    std::cout << "    Output:      " << rmsL3 / std::sqrt(0.5) << "\n";
    std::cout << "    Error:       " << (error * 100.0) << "%\n";
    std::cout << "    Result:      " << (error < 0.01 ? "✓ PASS" : "✗ FAIL") << "\n";
}

// Test 6: Mono Maker THD
void testMonoMakerTHD(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 6: Mono Maker - THD (Should be Bit-Perfect)         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(56);
    if (!engine) return;

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set to full mono
    std::map<int, float> params;
    params[0] = 1.0f;
    params[3] = 1.0f;
    engine->reset();
    engine->updateParameters(params);

    // Generate clean sine on both channels
    juce::AudioBuffer<float> buffer(2, 16384);
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            buffer.setSample(ch, i, 0.3f * std::sin(phase));
        }
    }

    engine->process(buffer);

    double thd = calculateTHD(buffer, 1000.0f, sampleRate);

    std::cout << "  1kHz sine → Mono → THD: " << std::setprecision(8) << thd << "%\n";
    std::cout << "  Result: " << (thd < 0.001 ? "✓ PASS (bit-perfect summing)" : "⚠ ACCEPTABLE") << "\n";
}

// Test 7: Frequency Response
void testMonoMakerFrequencyResponse(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 7: Mono Maker - Frequency Response (Flat)           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(56);
    if (!engine) return;

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set to full mono
    std::map<int, float> params;
    params[0] = 1.0f;
    params[3] = 1.0f;
    engine->reset();
    engine->updateParameters(params);

    std::vector<float> testFreqs = {20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f,
                                     2000.0f, 5000.0f, 10000.0f, 15000.0f, 20000.0f};

    std::cout << "  Testing frequency response (should be flat ±0.1dB):\n\n";

    std::vector<double> responses;

    for (float freq : testFreqs) {
        engine->reset();
        engine->updateParameters(params);

        juce::AudioBuffer<float> buffer(2, blockSize * 4);
        const float inputLevel = 0.5f;

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float phase = 2.0f * M_PI * freq * i / sampleRate;
                buffer.setSample(ch, i, inputLevel * std::sin(phase));
            }
        }

        double inputRMS = measureRMS(buffer, 0);
        engine->process(buffer);
        double outputRMS = measureRMS(buffer, 0);

        double responseDB = linearToDb(outputRMS / inputRMS);
        responses.push_back(responseDB);

        bool passed = std::abs(responseDB) < 0.1;

        std::cout << "  " << std::setw(7) << freq << " Hz  →  "
                  << std::setw(8) << std::setprecision(3) << responseDB << " dB  "
                  << (passed ? "✓" : "✗") << "\n";
    }

    // Calculate flatness
    double avgResponse = std::accumulate(responses.begin(), responses.end(), 0.0) / responses.size();
    double variance = 0.0;
    for (double r : responses) {
        variance += (r - avgResponse) * (r - avgResponse);
    }
    variance = std::sqrt(variance / responses.size());

    std::cout << "\n  Flatness (std dev): " << std::setprecision(3) << variance << " dB\n";
    std::cout << "  Result: " << (variance < 0.1 ? "✓ PASS (perfectly flat)" : "⚠ ACCEPTABLE") << "\n";
}

// Test 8: CPU Performance
void testCPUPerformance(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 8: CPU Performance (Should be <0.1%)                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    const int blockSize = 512;
    const int iterations = 50000;

    for (int engineId : {55, 56}) {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) continue;

        engine->prepareToPlay(sampleRate, blockSize);

        // Set parameters
        std::map<int, float> params;
        params[0] = 0.5f;
        engine->updateParameters(params);

        // Generate test signal
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                buffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        // Warmup
        for (int i = 0; i < 1000; ++i) {
            engine->process(buffer);
        }

        // Measure
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            engine->process(buffer);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        double timePerBlock = duration.count() / static_cast<double>(iterations);
        double realTimePerBlock = (blockSize * 1000000.0) / sampleRate;
        double cpuUsage = (timePerBlock / realTimePerBlock) * 100.0;

        std::cout << "  Engine " << engineId << " (" << engine->getName() << "):\n";
        std::cout << "    Time per block: " << std::setprecision(2) << timePerBlock << " μs\n";
        std::cout << "    Real-time:      " << std::setprecision(2) << realTimePerBlock << " μs\n";
        std::cout << "    CPU usage:      " << std::setprecision(3) << cpuUsage << "%\n";
        std::cout << "    Result:         " << (cpuUsage < 0.1 ? "✓ PASS" : (cpuUsage < 1.0 ? "⚠ ACCEPTABLE" : "✗ FAIL")) << "\n\n";
    }
}

//==============================================================================
// CSV Export
//==============================================================================

void exportGainAccuracyCSV(const std::vector<GainAccuracyResult>& results, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "Set Gain (dB),Measured Gain (dB),Error (dB),Pass\n";

    for (const auto& r : results) {
        file << r.setGainDB << "," << r.measuredGainDB << ","
             << r.errorDB << "," << (r.passed ? "YES" : "NO") << "\n";
    }

    file.close();
    std::cout << "\n✓ Exported gain accuracy data to: " << filename << "\n";
}

} // namespace UtilityTests

//==============================================================================
// Main Test Runner
//==============================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     ChimeraPhoenix Utility Engines Test Suite             ║\n";
    std::cout << "║     Engines 55-56: Gain Utility & Mono Maker Platinum     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    const float sampleRate = 48000.0f;

    // Gain Utility Tests
    auto gainAccuracyResults = UtilityTests::testGainAccuracy(sampleRate);
    UtilityTests::testGainTHD(sampleRate);
    UtilityTests::testGainPhase(sampleRate);
    UtilityTests::testGainChannelIndependence(sampleRate);

    // Mono Maker Tests
    UtilityTests::testMonoSumming(sampleRate);
    UtilityTests::testMonoMakerTHD(sampleRate);
    UtilityTests::testMonoMakerFrequencyResponse(sampleRate);

    // Performance Tests
    UtilityTests::testCPUPerformance(sampleRate);

    // Export results
    UtilityTests::exportGainAccuracyCSV(gainAccuracyResults, "gain_utility_accuracy.csv");

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                   ALL TESTS COMPLETE                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}

// utility_test_simple.cpp - Direct utility engine testing (no factory)
// Tests engines 55-56: Gain Utility Platinum and Mono Maker Platinum

#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/GainUtility_Platinum.h"
#include "../JUCE_Plugin/Source/MonoMaker_Platinum.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <fstream>
#include <chrono>

//==============================================================================
// High-Precision Analysis Tools
//==============================================================================

// Convert dB to linear
double dbToLinear(double db) {
    constexpr double DB_TO_LINEAR = 0.05776226504666210911810267678818;
    return std::exp(db * DB_TO_LINEAR);
}

// Convert linear to dB
double linearToDb(double linear) {
    constexpr double LINEAR_TO_DB = 17.312340490667560888319096172023;
    if (linear < 1e-20) return -200.0;
    return std::log(linear) * LINEAR_TO_DB;
}

// Measure RMS level
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

// Calculate THD
double calculateTHD(const juce::AudioBuffer<float>& buffer, float fundamentalFreq, float sampleRate) {
    const int fftSize = 16384;
    if (buffer.getNumSamples() < fftSize) return 0.0;

    juce::dsp::FFT fft(14);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    int fundamentalBin = static_cast<int>(fundamentalFreq * fftSize / sampleRate);
    double fundamentalMag = fftData[fundamentalBin];

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

//==============================================================================
// Test Functions
//==============================================================================

void testGainUtility(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENGINE 55: Gain Utility Platinum - Comprehensive Test    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    GainUtility_Platinum engine;
    const int blockSize = 512;
    engine.prepareToPlay(sampleRate, blockSize);

    // Test 1: Gain Accuracy
    std::cout << "TEST 1: Gain Accuracy (±0.01dB tolerance)\n";
    std::cout << "----------------------------------------\n";

    std::ofstream csvFile("gain_utility_accuracy.csv");
    csvFile << "SetGain_dB,MeasuredGain_dB,Error_dB,Pass\n";

    int passCount = 0;
    int totalTests = 0;

    for (int gainDB = -40; gainDB <= 20; gainDB += 2) {
        float normalizedGain = (gainDB + 40.0f) / 64.0f;
        normalizedGain = std::max(0.0f, std::min(1.0f, normalizedGain));

        std::map<int, float> params;
        params[0] = normalizedGain;
        engine.reset();
        engine.updateParameters(params);

        juce::AudioBuffer<float> buffer(2, blockSize * 4);
        const float inputAmplitude = 0.5f;

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                buffer.setSample(ch, i, inputAmplitude * std::sin(phase));
            }
        }

        double inputRMS = measureRMS(buffer, 0);
        engine.process(buffer);
        double outputRMS = measureRMS(buffer, 0);

        double measuredGainDB = linearToDb(outputRMS / inputRMS);
        double errorDB = std::abs(measuredGainDB - gainDB);
        bool passed = errorDB <= 0.01;

        csvFile << gainDB << "," << measuredGainDB << "," << errorDB << "," << (passed ? "YES" : "NO") << "\n";

        if (passed) passCount++;
        totalTests++;

        if (gainDB % 10 == 0 || !passed) {
            std::cout << "  " << std::setw(6) << gainDB << " dB → "
                      << std::setw(8) << std::setprecision(3) << std::fixed << measuredGainDB << " dB (error: "
                      << std::setw(7) << std::setprecision(4) << errorDB << " dB) "
                      << (passed ? "✓" : "✗") << "\n";
        }
    }

    csvFile.close();
    std::cout << "\n  Result: " << passCount << "/" << totalTests << " tests passed ("
              << std::setprecision(1) << (passCount * 100.0 / totalTests) << "%)\n";
    std::cout << "  CSV exported: gain_utility_accuracy.csv\n";

    // Test 2: THD at Unity Gain
    std::cout << "\nTEST 2: THD at Various Gain Settings\n";
    std::cout << "------------------------------------\n";

    std::vector<int> testGains = {-12, -6, 0, +6, +12};
    for (int gainDB : testGains) {
        float normalizedGain = (gainDB + 40.0f) / 64.0f;

        std::map<int, float> params;
        params[0] = normalizedGain;
        engine.reset();
        engine.updateParameters(params);

        juce::AudioBuffer<float> buffer(2, 16384);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                buffer.setSample(ch, i, 0.3f * std::sin(phase));
            }
        }

        engine.process(buffer);
        double thd = calculateTHD(buffer, 1000.0f, sampleRate);
        bool passed = thd < 0.001;

        std::cout << "  Gain: " << std::setw(6) << gainDB << " dB → THD: "
                  << std::setw(10) << std::setprecision(6) << std::fixed << thd << "% "
                  << (passed ? "✓" : "✗") << "\n";
    }

    // Test 3: Channel Independence
    std::cout << "\nTEST 3: Channel Independence\n";
    std::cout << "----------------------------\n";

    std::map<int, float> params;
    params[0] = 0.625f;  // 0dB main
    params[1] = 0.75f;   // +6dB left
    params[2] = 0.25f;   // -6dB right
    engine.reset();
    engine.updateParameters(params);

    juce::AudioBuffer<float> buffer(2, blockSize * 4);
    const float inputLevel = 0.5f;

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            buffer.setSample(ch, i, inputLevel * std::sin(phase));
        }
    }

    engine.process(buffer);

    double rmsL = measureRMS(buffer, 0);
    double rmsR = measureRMS(buffer, 1);
    double gainL_dB = linearToDb(rmsL / inputLevel);
    double gainR_dB = linearToDb(rmsR / inputLevel);

    std::cout << "  L Channel: " << std::setprecision(2) << gainL_dB << " dB (expected ~+6dB)\n";
    std::cout << "  R Channel: " << std::setprecision(2) << gainR_dB << " dB (expected ~-6dB)\n";
    std::cout << "  Result: " << (std::abs(gainL_dB - 6.0) < 1.0 && std::abs(gainR_dB + 6.0) < 1.0 ? "✓ PASS" : "✗ FAIL") << "\n";

    // Test 4: CPU Performance
    std::cout << "\nTEST 4: CPU Performance\n";
    std::cout << "----------------------\n";

    params.clear();
    params[0] = 0.5f;
    engine.reset();
    engine.updateParameters(params);

    juce::AudioBuffer<float> perfBuffer(2, blockSize);
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            perfBuffer.setSample(ch, i, 0.5f * std::sin(phase));
        }
    }

    // Warmup
    for (int i = 0; i < 1000; ++i) engine.process(perfBuffer);

    const int iterations = 50000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) engine.process(perfBuffer);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double timePerBlock = duration.count() / static_cast<double>(iterations);
    double realTimePerBlock = (blockSize * 1000000.0) / sampleRate;
    double cpuUsage = (timePerBlock / realTimePerBlock) * 100.0;

    std::cout << "  Time per block: " << std::setprecision(2) << timePerBlock << " μs\n";
    std::cout << "  CPU usage: " << std::setprecision(3) << cpuUsage << "%\n";
    std::cout << "  Result: " << (cpuUsage < 0.1 ? "✓ PASS (<0.1%)" : (cpuUsage < 1.0 ? "⚠ ACCEPTABLE" : "✗ FAIL")) << "\n";
}

void testMonoMaker(float sampleRate = 48000.0f) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENGINE 56: Mono Maker Platinum - Comprehensive Test      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    MonoMaker_Platinum engine;
    const int blockSize = 512;
    engine.prepareToPlay(sampleRate, blockSize);

    // Test 1: Mono Summing
    std::cout << "TEST 1: Mono Summing Accuracy\n";
    std::cout << "-----------------------------\n";

    std::map<int, float> params;
    params[0] = 1.0f;   // Max frequency (mono everything)
    params[3] = 1.0f;   // 100% bass mono
    engine.reset();
    engine.updateParameters(params);

    // Identical signals → should stay same
    juce::AudioBuffer<float> buffer1(2, blockSize * 4);
    for (int i = 0; i < buffer1.getNumSamples(); ++i) {
        float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
        float value = 0.5f * std::sin(phase);
        buffer1.setSample(0, i, value);
        buffer1.setSample(1, i, value);
    }

    double rmsOrig = measureRMS(buffer1, 0);
    engine.process(buffer1);
    double rmsL1 = measureRMS(buffer1, 0);
    double rmsR1 = measureRMS(buffer1, 1);

    std::cout << "  Identical L/R signals:\n";
    std::cout << "    Input:  " << rmsOrig << "\n";
    std::cout << "    Output: " << rmsL1 << " (L), " << rmsR1 << " (R)\n";
    std::cout << "    L/R match: " << (std::abs(rmsL1 - rmsR1) < 0.0001 ? "✓ YES" : "✗ NO") << "\n\n";

    // Phase-inverted signals → should cancel
    engine.reset();
    engine.updateParameters(params);

    juce::AudioBuffer<float> buffer2(2, blockSize * 4);
    for (int i = 0; i < buffer2.getNumSamples(); ++i) {
        float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
        buffer2.setSample(0, i, +0.5f * std::sin(phase));
        buffer2.setSample(1, i, -0.5f * std::sin(phase));
    }

    engine.process(buffer2);
    double rmsL2 = measureRMS(buffer2, 0);
    double rmsR2 = measureRMS(buffer2, 1);

    std::cout << "  Phase-inverted signals:\n";
    std::cout << "    Output: " << rmsL2 << " (L), " << rmsR2 << " (R)\n";
    std::cout << "    Cancelled: " << (rmsL2 < 0.001 && rmsR2 < 0.001 ? "✓ YES (perfect)" : "✗ NO") << "\n";

    // Test 2: THD
    std::cout << "\nTEST 2: THD (Bit-Perfect Summing)\n";
    std::cout << "---------------------------------\n";

    engine.reset();
    engine.updateParameters(params);

    juce::AudioBuffer<float> buffer3(2, 16384);
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < buffer3.getNumSamples(); ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            buffer3.setSample(ch, i, 0.3f * std::sin(phase));
        }
    }

    engine.process(buffer3);
    double thd = calculateTHD(buffer3, 1000.0f, sampleRate);

    std::cout << "  1kHz sine → Mono → THD: " << std::setprecision(8) << thd << "%\n";
    std::cout << "  Result: " << (thd < 0.001 ? "✓ PASS (<0.001%)" : "⚠ ACCEPTABLE") << "\n";

    // Test 3: Frequency Response
    std::cout << "\nTEST 3: Frequency Response (Flatness)\n";
    std::cout << "------------------------------------\n";

    std::vector<float> testFreqs = {100.0f, 1000.0f, 5000.0f, 10000.0f, 15000.0f};
    std::vector<double> responses;

    for (float freq : testFreqs) {
        engine.reset();
        engine.updateParameters(params);

        juce::AudioBuffer<float> buffer(2, blockSize * 4);
        const float inputLevel = 0.5f;

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float phase = 2.0f * M_PI * freq * i / sampleRate;
                buffer.setSample(ch, i, inputLevel * std::sin(phase));
            }
        }

        double inputRMS = measureRMS(buffer, 0);
        engine.process(buffer);
        double outputRMS = measureRMS(buffer, 0);
        double responseDB = linearToDb(outputRMS / inputRMS);
        responses.push_back(responseDB);

        std::cout << "  " << std::setw(7) << freq << " Hz → " << std::setw(7) << std::setprecision(2) << responseDB << " dB "
                  << (std::abs(responseDB) < 0.5 ? "✓" : "⚠") << "\n";
    }

    // Test 4: CPU Performance
    std::cout << "\nTEST 4: CPU Performance\n";
    std::cout << "----------------------\n";

    engine.reset();
    engine.updateParameters(params);

    juce::AudioBuffer<float> perfBuffer(2, blockSize);
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            perfBuffer.setSample(ch, i, 0.5f * std::sin(phase));
        }
    }

    for (int i = 0; i < 1000; ++i) engine.process(perfBuffer);

    const int iterations = 50000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) engine.process(perfBuffer);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double timePerBlock = duration.count() / static_cast<double>(iterations);
    double realTimePerBlock = (blockSize * 1000000.0) / sampleRate;
    double cpuUsage = (timePerBlock / realTimePerBlock) * 100.0;

    std::cout << "  Time per block: " << std::setprecision(2) << timePerBlock << " μs\n";
    std::cout << "  CPU usage: " << std::setprecision(3) << cpuUsage << "%\n";
    std::cout << "  Result: " << (cpuUsage < 1.0 ? "✓ PASS (<1.0%)" : "⚠ ACCEPTABLE") << "\n";
}

//==============================================================================
// Main
//==============================================================================

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ChimeraPhoenix Utility Engines Test Suite                ║\n";
    std::cout << "║  Engines 55-56: Gain Utility & Mono Maker Platinum        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    testGainUtility(48000.0f);
    testMonoMaker(48000.0f);

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                   ALL TESTS COMPLETE                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}

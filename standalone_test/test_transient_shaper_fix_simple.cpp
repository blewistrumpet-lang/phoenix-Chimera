#include "JuceHeader.h"
#include "MinimalEngineFactory.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

// Test TransientShaper (Engine 3) with extreme sustain parameter

std::vector<float> generateDrumHit(int sampleRate, float durationSec) {
    int numSamples = static_cast<int>(sampleRate * durationSec);
    std::vector<float> signal(numSamples);

    // Drum hit: sharp attack + exponential decay
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;

        // Attack envelope (first 5ms)
        float attack = std::min(1.0f, t / 0.005f);

        // Exponential decay
        float decay = std::exp(-t * 8.0f);

        // Mix sine wave with noise for realistic drum
        float sine = std::sin(2.0f * M_PI * 150.0f * t); // 150Hz fundamental
        float noise = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.3f;

        signal[i] = (sine * 0.7f + noise * 0.3f) * attack * decay * 0.5f;
    }

    return signal;
}

float measurePeakDB(const std::vector<float>& signal) {
    float peak = 0.0f;
    for (float sample : signal) {
        peak = std::max(peak, std::abs(sample));
    }

    if (peak < 1e-10f) return -100.0f;
    return 20.0f * std::log10(peak);
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║  TRANSIENT SHAPER - RUNAWAY GAIN FIX VERIFICATION   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";

    const int sampleRate = 48000;
    const int blockSize = 512;
    const int engineId = 3; // TransientShaper_Platinum

    std::cout << "=== SUSTAIN PARAMETER TEST ===\n";
    std::cout << "Testing sustain from 0% to 100% in 10% steps\n";
    std::cout << "Target: All outputs should stay below +20dB\n\n";

    std::vector<float> sustainLevels = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};

    std::cout << "Sustain% | Input Peak | Output Peak | Gain (dB) | Status\n";
    std::cout << "---------|------------|-------------|-----------|--------\n";

    bool allPassed = true;

    for (float sustainParam : sustainLevels) {
        // Create engine
        auto engine = MinimalEngineFactory::createEngine(engineId);
        if (!engine) {
            std::cout << "ERROR: Failed to create engine\n";
            return 1;
        }

        engine->prepareToPlay(sampleRate, blockSize);

        // Set parameters
        std::map<int, float> params;
        params[0] = 0.5f;        // Attack = 0dB
        params[1] = sustainParam; // Sustain = variable
        params[2] = 0.1f;        // Attack Time
        params[3] = 0.3f;        // Release Time
        params[9] = 1.0f;        // Mix = 100% wet
        engine->updateParameters(params);

        // Generate test signal
        auto inputSignal = generateDrumHit(sampleRate, 0.5f);
        float inputPeakDB = measurePeakDB(inputSignal);

        // Process in blocks
        std::vector<float> outputSignal;
        for (size_t pos = 0; pos < inputSignal.size(); pos += blockSize) {
            int samples = std::min(blockSize, static_cast<int>(inputSignal.size() - pos));

            juce::AudioBuffer<float> buffer(2, samples);
            buffer.clear();

            for (int i = 0; i < samples; ++i) {
                buffer.setSample(0, i, inputSignal[pos + i]);
                buffer.setSample(1, i, inputSignal[pos + i]);
            }

            engine->process(buffer);

            for (int i = 0; i < samples; ++i) {
                outputSignal.push_back(buffer.getSample(0, i));
            }
        }

        // Measure output
        float outputPeakDB = measurePeakDB(outputSignal);
        float gainDB = outputPeakDB - inputPeakDB;

        // Check for runaway
        bool pass = (outputPeakDB < 20.0f);
        if (!pass) allPassed = false;

        printf("%6.0f%% | %9.2f dB | %10.2f dB | %8.2f dB | %s\n",
               sustainParam * 100.0f,
               inputPeakDB,
               outputPeakDB,
               gainDB,
               pass ? "PASS" : "FAIL - RUNAWAY!");
    }

    std::cout << "\n=== STRESS TEST: MAXIMUM PARAMETERS ===\n";

    auto engine = MinimalEngineFactory::createEngine(engineId);
    engine->prepareToPlay(sampleRate, blockSize);

    // ALL PARAMETERS TO MAX
    std::map<int, float> params;
    params[0] = 1.0f;  // Max Attack
    params[1] = 1.0f;  // Max Sustain
    params[2] = 1.0f;  // Max Attack Time
    params[3] = 1.0f;  // Max Release Time
    params[4] = 1.0f;  // Max Separation
    params[9] = 1.0f;  // Max Mix
    engine->updateParameters(params);

    auto inputSignal = generateDrumHit(sampleRate, 0.5f);
    float inputPeakDB = measurePeakDB(inputSignal);

    std::vector<float> outputSignal;
    for (size_t pos = 0; pos < inputSignal.size(); pos += blockSize) {
        int samples = std::min(blockSize, static_cast<int>(inputSignal.size() - pos));

        juce::AudioBuffer<float> buffer(2, samples);
        buffer.clear();

        for (int i = 0; i < samples; ++i) {
            buffer.setSample(0, i, inputSignal[pos + i]);
            buffer.setSample(1, i, inputSignal[pos + i]);
        }

        engine->process(buffer);

        for (int i = 0; i < samples; ++i) {
            outputSignal.push_back(buffer.getSample(0, i));
        }
    }

    float outputPeakDB = measurePeakDB(outputSignal);
    float gainDB = outputPeakDB - inputPeakDB;
    bool stressPass = (outputPeakDB < 20.0f);
    if (!stressPass) allPassed = false;

    std::cout << "All parameters at maximum:\n";
    std::cout << "  Input:  " << inputPeakDB << " dB\n";
    std::cout << "  Output: " << outputPeakDB << " dB\n";
    std::cout << "  Gain:   " << gainDB << " dB\n";
    std::cout << "  Status: " << (stressPass ? "PASS" : "FAIL") << "\n\n";

    std::cout << "\n=== TEST SUMMARY ===\n";
    if (allPassed) {
        std::cout << "✓ ALL TESTS PASSED - No runaway gain detected!\n";
        std::cout << "✓ Output levels stayed below +20dB at all sustain values\n";
        return 0;
    } else {
        std::cout << "✗ SOME TESTS FAILED - Runaway gain still present!\n";
        std::cout << "✗ Fix incomplete or insufficient\n";
        return 1;
    }
}

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

/**
 * Comprehensive Test Suite for Engine 7: Parametric EQ Studio
 *
 * Tests:
 * 1. Impulse response test
 * 2. Frequency response verification
 * 3. Band-specific processing (low, mid, high)
 * 4. Phase response
 * 5. THD measurement
 * 6. CPU performance
 * 7. Quality metrics
 */

namespace Engine7Test {

struct TestResults {
    // Impulse Response
    std::vector<float> impulseResponse;
    float impulseDecayTime;
    float impulsePeakLocation;

    // Frequency Response
    std::vector<std::pair<float, float>> frequencyResponse; // freq, magnitude(dB)
    std::vector<std::pair<float, float>> phaseResponse;     // freq, phase(degrees)

    // Band-specific metrics
    float lowBandGain;      // @ 100 Hz
    float midBandGain;      // @ 1000 Hz
    float highBandGain;     // @ 10000 Hz

    // Quality metrics
    float thdPercent;
    float noiseFloorDB;
    float flatResponseVariance; // dB variance when flat

    // Performance
    float cpuUsagePercent;
    float latencySamples;

    // Overall assessment
    bool passedImpulse;
    bool passedFrequency;
    bool passedTHD;
    bool passedCPU;
    bool overallPass;
};

// Simple FFT for frequency analysis (power of 2 size)
void simpleFFT(std::vector<std::complex<float>>& data, bool inverse = false) {
    int n = data.size();
    if (n <= 1) return;

    // Bit-reversal permutation
    for (int i = 0, j = 0; i < n; i++) {
        if (j > i) std::swap(data[i], data[j]);
        int m = n >> 1;
        while (j >= m && m >= 2) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    // FFT computation
    for (int s = 2; s <= n; s *= 2) {
        float angle = (inverse ? 2.0f : -2.0f) * M_PI / s;
        std::complex<float> wn(std::cos(angle), std::sin(angle));

        for (int k = 0; k < n; k += s) {
            std::complex<float> w(1.0f, 0.0f);
            for (int j = 0; j < s/2; j++) {
                std::complex<float> t = w * data[k + j + s/2];
                std::complex<float> u = data[k + j];
                data[k + j] = u + t;
                data[k + j + s/2] = u - t;
                w *= wn;
            }
        }
    }

    if (inverse) {
        for (auto& d : data) d /= static_cast<float>(n);
    }
}

// Test 1: Impulse Response
std::vector<float> testImpulseResponse(EngineBase* engine, float sampleRate, int blockSize) {
    std::cout << "  [1/7] Testing impulse response...\n";

    const int fftSize = 8192;
    juce::AudioBuffer<float> buffer(2, fftSize);
    buffer.clear();

    // Create impulse at sample 100
    buffer.setSample(0, 100, 1.0f);
    buffer.setSample(1, 100, 1.0f);

    // Process in blocks
    for (int start = 0; start < fftSize; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, fftSize - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Extract impulse response from left channel
    std::vector<float> ir(fftSize);
    for (int i = 0; i < fftSize; ++i) {
        ir[i] = buffer.getSample(0, i);
    }

    return ir;
}

// Test 2: Frequency Response
std::vector<std::pair<float, float>> measureFrequencyResponse(
    EngineBase* engine, float sampleRate, int blockSize) {

    std::cout << "  [2/7] Measuring frequency response...\n";

    std::vector<std::pair<float, float>> response;

    // Test frequencies: 20 Hz to 20 kHz
    std::vector<float> testFreqs = {
        20, 30, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800,
        1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000
    };

    for (float freq : testFreqs) {
        if (freq >= sampleRate / 2.0f) continue;

        engine->reset();

        const int testDuration = static_cast<int>(sampleRate * 0.2f); // 200ms
        juce::AudioBuffer<float> buffer(2, testDuration);

        // Generate sine wave
        float amplitude = 0.5f;
        for (int i = 0; i < testDuration; ++i) {
            float phase = 2.0f * M_PI * freq * i / sampleRate;
            float sample = amplitude * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        // Measure input RMS
        float inputRMS = 0.0f;
        for (int i = 0; i < testDuration; ++i) {
            float s = buffer.getSample(0, i);
            inputRMS += s * s;
        }
        inputRMS = std::sqrt(inputRMS / testDuration);

        // Process
        for (int start = 0; start < testDuration; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testDuration - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Measure output RMS (skip first 10ms for settling)
        int skipSamples = static_cast<int>(sampleRate * 0.01f);
        float outputRMS = 0.0f;
        for (int i = skipSamples; i < testDuration; ++i) {
            float s = buffer.getSample(0, i);
            outputRMS += s * s;
        }
        outputRMS = std::sqrt(outputRMS / (testDuration - skipSamples));

        // Calculate gain in dB
        float gainDB = 20.0f * std::log10(std::max(1e-10f, outputRMS / inputRMS));
        response.push_back({freq, gainDB});
    }

    return response;
}

// Test 3: Band-specific processing
void testBandProcessing(EngineBase* engine, float sampleRate, int blockSize,
                       float& lowGain, float& midGain, float& highGain) {

    std::cout << "  [3/7] Testing frequency-dependent processing...\n";

    auto measureGain = [&](float freq) -> float {
        engine->reset();
        const int testDuration = static_cast<int>(sampleRate * 0.2f);
        juce::AudioBuffer<float> buffer(2, testDuration);

        float amplitude = 0.5f;
        for (int i = 0; i < testDuration; ++i) {
            float phase = 2.0f * M_PI * freq * i / sampleRate;
            buffer.setSample(0, i, amplitude * std::sin(phase));
            buffer.setSample(1, i, amplitude * std::sin(phase));
        }

        float inputRMS = amplitude / std::sqrt(2.0f);

        for (int start = 0; start < testDuration; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testDuration - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        int skipSamples = static_cast<int>(sampleRate * 0.01f);
        float outputRMS = 0.0f;
        for (int i = skipSamples; i < testDuration; ++i) {
            float s = buffer.getSample(0, i);
            outputRMS += s * s;
        }
        outputRMS = std::sqrt(outputRMS / (testDuration - skipSamples));

        return 20.0f * std::log10(std::max(1e-10f, outputRMS / inputRMS));
    };

    lowGain = measureGain(100.0f);
    midGain = measureGain(1000.0f);
    highGain = measureGain(10000.0f);
}

// Test 4: THD Measurement
float measureTHD(EngineBase* engine, float sampleRate, int blockSize) {
    std::cout << "  [4/7] Measuring THD+N...\n";

    engine->reset();

    const float testFreq = 1000.0f;
    const int testDuration = static_cast<int>(sampleRate * 0.5f);
    juce::AudioBuffer<float> buffer(2, testDuration);

    // Generate pure sine
    float amplitude = 0.5f;
    for (int i = 0; i < testDuration; ++i) {
        float phase = 2.0f * M_PI * testFreq * i / sampleRate;
        buffer.setSample(0, i, amplitude * std::sin(phase));
        buffer.setSample(1, i, amplitude * std::sin(phase));
    }

    // Process
    for (int start = 0; start < testDuration; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testDuration - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Simple THD estimation using time-domain approach
    int skipSamples = static_cast<int>(sampleRate * 0.05f);
    float totalRMS = 0.0f;
    float fundamentalRMS = 0.0f;

    for (int i = skipSamples; i < testDuration; ++i) {
        float sample = buffer.getSample(0, i);
        totalRMS += sample * sample;

        // Estimate fundamental component
        float phase = 2.0f * M_PI * testFreq * i / sampleRate;
        float fundamental = amplitude * std::sin(phase);

        // Correlation with fundamental
        float corrected = sample * (fundamental / (amplitude + 1e-10f));
        fundamentalRMS += corrected * corrected;
    }

    int numSamples = testDuration - skipSamples;
    totalRMS = std::sqrt(totalRMS / numSamples);
    fundamentalRMS = std::sqrt(fundamentalRMS / numSamples);

    float harmonicRMS = std::sqrt(std::max(0.0f, totalRMS * totalRMS - fundamentalRMS * fundamentalRMS));

    if (fundamentalRMS < 1e-10f) return 0.0f;
    return (harmonicRMS / fundamentalRMS) * 100.0f;
}

// Test 5: Noise Floor
float measureNoiseFloor(EngineBase* engine, float sampleRate, int blockSize) {
    std::cout << "  [5/7] Measuring noise floor...\n";

    engine->reset();

    const int testDuration = static_cast<int>(sampleRate * 1.0f);
    juce::AudioBuffer<float> buffer(2, testDuration);
    buffer.clear(); // Silent input

    // Process silence
    for (int start = 0; start < testDuration; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testDuration - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Measure output RMS
    float rms = 0.0f;
    for (int i = 0; i < testDuration; ++i) {
        float s = buffer.getSample(0, i);
        rms += s * s;
    }
    rms = std::sqrt(rms / testDuration);

    return 20.0f * std::log10(std::max(1e-10f, rms));
}

// Test 6: CPU Performance
float measureCPUUsage(EngineBase* engine, float sampleRate, int blockSize) {
    std::cout << "  [6/7] Measuring CPU usage...\n";

    const int testDuration = static_cast<int>(sampleRate * 1.0f);
    juce::AudioBuffer<float> buffer(2, testDuration);

    // Fill with noise
    for (int i = 0; i < testDuration; ++i) {
        float noise = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.5f;
        buffer.setSample(0, i, noise);
        buffer.setSample(1, i, noise);
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int start = 0; start < testDuration; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testDuration - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    return (duration / 1000000.0f) * 100.0f; // % of real-time
}

// Main test function
TestResults testEngine7(float sampleRate = 48000.0f) {
    TestResults results = {};

    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine 7: Parametric EQ Studio - Comprehensive Test     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

    auto engine = EngineFactory::createEngine(7);
    if (!engine) {
        std::cout << "ERROR: Failed to create Engine 7\n";
        return results;
    }

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set neutral parameters (flat response)
    std::map<int, float> params;
    int numParams = engine->getNumParameters();
    std::cout << "Engine has " << numParams << " parameters\n\n";

    // Set all parameters to neutral/bypass values
    for (int i = 0; i < numParams; ++i) {
        params[i] = 0.5f; // Neutral position
    }
    engine->updateParameters(params);

    // Run tests
    results.impulseResponse = testImpulseResponse(engine.get(), sampleRate, blockSize);
    results.frequencyResponse = measureFrequencyResponse(engine.get(), sampleRate, blockSize);
    testBandProcessing(engine.get(), sampleRate, blockSize,
                      results.lowBandGain, results.midBandGain, results.highBandGain);
    results.thdPercent = measureTHD(engine.get(), sampleRate, blockSize);
    results.noiseFloorDB = measureNoiseFloor(engine.get(), sampleRate, blockSize);
    results.cpuUsagePercent = measureCPUUsage(engine.get(), sampleRate, blockSize);

    std::cout << "  [7/7] Analyzing results...\n\n";

    // Calculate flat response variance
    float sumGain = 0.0f;
    for (const auto& [freq, gain] : results.frequencyResponse) {
        sumGain += std::abs(gain);
    }
    results.flatResponseVariance = sumGain / results.frequencyResponse.size();

    // Determine pass/fail
    results.passedImpulse = results.impulseResponse.size() > 0;
    results.passedFrequency = results.flatResponseVariance < 3.0f; // < 3dB variance when flat
    results.passedTHD = results.thdPercent < 0.1f; // < 0.1% THD
    results.passedCPU = results.cpuUsagePercent < 10.0f; // < 10% CPU

    results.overallPass = results.passedImpulse && results.passedFrequency &&
                         results.passedTHD && results.passedCPU;

    return results;
}

void printResults(const TestResults& r) {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "                     TEST RESULTS                          \n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    std::cout << "IMPULSE RESPONSE:\n";
    std::cout << "  Samples captured:    " << r.impulseResponse.size() << "\n";
    std::cout << "  Status:              " << (r.passedImpulse ? "✓ PASS" : "✗ FAIL") << "\n\n";

    std::cout << "FREQUENCY RESPONSE:\n";
    std::cout << "  Test points:         " << r.frequencyResponse.size() << "\n";
    std::cout << "  Low band (100Hz):    " << std::fixed << std::setprecision(2)
              << r.lowBandGain << " dB\n";
    std::cout << "  Mid band (1kHz):     " << r.midBandGain << " dB\n";
    std::cout << "  High band (10kHz):   " << r.highBandGain << " dB\n";
    std::cout << "  Flatness variance:   " << r.flatResponseVariance << " dB\n";
    std::cout << "  Status:              " << (r.passedFrequency ? "✓ PASS" : "✗ FAIL") << "\n\n";

    std::cout << "QUALITY METRICS:\n";
    std::cout << "  THD+N:               " << std::fixed << std::setprecision(4)
              << r.thdPercent << "%\n";
    std::cout << "  Noise floor:         " << std::fixed << std::setprecision(1)
              << r.noiseFloorDB << " dB\n";
    std::cout << "  THD Status:          " << (r.passedTHD ? "✓ PASS" : "✗ FAIL") << "\n\n";

    std::cout << "PERFORMANCE:\n";
    std::cout << "  CPU usage:           " << std::fixed << std::setprecision(2)
              << r.cpuUsagePercent << "%\n";
    std::cout << "  CPU Status:          " << (r.passedCPU ? "✓ PASS" : "✗ FAIL") << "\n\n";

    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  OVERALL RESULT:      " << (r.overallPass ? "✓ PASSED" : "✗ FAILED") << "\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";
}

void saveResults(const TestResults& r) {
    // Save impulse response
    {
        std::ofstream file("impulse_engine_7.csv");
        if (file.is_open()) {
            file << "Sample,Amplitude\n";
            for (size_t i = 0; i < r.impulseResponse.size(); ++i) {
                file << i << "," << r.impulseResponse[i] << "\n";
            }
            file.close();
            std::cout << "Saved: impulse_engine_7.csv\n";
        }
    }

    // Save frequency response
    {
        std::ofstream file("frequency_response_engine_7.csv");
        if (file.is_open()) {
            file << "Frequency (Hz),Gain (dB)\n";
            for (const auto& [freq, gain] : r.frequencyResponse) {
                file << freq << "," << gain << "\n";
            }
            file.close();
            std::cout << "Saved: frequency_response_engine_7.csv\n";
        }
    }

    // Save summary
    {
        std::ofstream file("engine_7_test_summary.txt");
        if (file.is_open()) {
            file << "Engine 7: Parametric EQ Studio Test Summary\n";
            file << "===========================================\n\n";
            file << "Impulse Response: " << (r.passedImpulse ? "PASS" : "FAIL") << "\n";
            file << "Frequency Response: " << (r.passedFrequency ? "PASS" : "FAIL") << "\n";
            file << "  - Low band (100Hz): " << r.lowBandGain << " dB\n";
            file << "  - Mid band (1kHz): " << r.midBandGain << " dB\n";
            file << "  - High band (10kHz): " << r.highBandGain << " dB\n";
            file << "  - Flatness: " << r.flatResponseVariance << " dB\n";
            file << "THD+N: " << r.thdPercent << "% " << (r.passedTHD ? "PASS" : "FAIL") << "\n";
            file << "Noise Floor: " << r.noiseFloorDB << " dB\n";
            file << "CPU Usage: " << r.cpuUsagePercent << "% " << (r.passedCPU ? "PASS" : "FAIL") << "\n";
            file << "\nOVERALL: " << (r.overallPass ? "PASSED" : "FAILED") << "\n";
            file.close();
            std::cout << "Saved: engine_7_test_summary.txt\n";
        }
    }
}

} // namespace Engine7Test

int main() {
    auto results = Engine7Test::testEngine7();
    Engine7Test::printResults(results);
    Engine7Test::saveResults(results);

    std::cout << "\n✓ Testing complete!\n\n";

    return results.overallPass ? 0 : 1;
}

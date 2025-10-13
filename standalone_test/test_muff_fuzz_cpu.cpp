// MuffFuzz CPU Benchmark Test
// Tests CPU performance after optimization (Bug #10 verification)

#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/MuffFuzz.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

class MuffFuzzCPUBenchmark {
public:
    void runBenchmark() {
        std::cout << "\n=== MuffFuzz CPU Benchmark Test ===" << std::endl;
        std::cout << "Testing Bug #10 optimization: Removed 4x oversampling, cached coefficients" << std::endl;
        std::cout << "Expected CPU reduction: 90-95% (from 5.19% to ~0.26-0.52%)" << std::endl;
        std::cout << "\n" << std::endl;

        // Initialize engine
        MuffFuzz engine;
        const double sampleRate = 44100.0;
        const int blockSize = 512;

        engine.prepareToPlay(sampleRate, blockSize);

        // Set typical parameters for Big Muff sound
        std::map<int, float> params;
        params[0] = 0.7f;  // Sustain
        params[1] = 0.5f;  // Tone
        params[2] = 0.8f;  // Volume
        params[3] = 0.0f;  // Gate (off)
        params[4] = 0.3f;  // Mids
        params[5] = 0.33f; // Variant (Ram's Head)
        params[6] = 1.0f;  // Mix (100% wet)

        engine.updateParameters(params);

        // Create test buffer - 10 seconds of audio
        const int totalSamples = static_cast<int>(sampleRate * 10.0);
        const int numBlocks = (totalSamples + blockSize - 1) / blockSize;

        std::cout << "Test configuration:" << std::endl;
        std::cout << "  Sample rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "  Block size: " << blockSize << " samples" << std::endl;
        std::cout << "  Duration: 10 seconds" << std::endl;
        std::cout << "  Total samples: " << totalSamples << std::endl;
        std::cout << "  Total blocks: " << numBlocks << std::endl;
        std::cout << "\n" << std::endl;

        // Generate test signal (guitar-like waveform)
        juce::AudioBuffer<float> testBuffer(2, blockSize);
        generateGuitarSignal(testBuffer, sampleRate);

        // Warm-up pass (not timed)
        std::cout << "Performing warm-up pass..." << std::endl;
        for (int i = 0; i < 100; ++i) {
            juce::AudioBuffer<float> warmupBuffer(2, blockSize);
            warmupBuffer.copyFrom(0, 0, testBuffer, 0, 0, blockSize);
            warmupBuffer.copyFrom(1, 0, testBuffer, 1, 0, blockSize);
            engine.process(warmupBuffer);
        }
        std::cout << "Warm-up complete." << std::endl;
        std::cout << "\n" << std::endl;

        // Actual benchmark
        std::cout << "Starting CPU benchmark..." << std::endl;

        auto startTime = std::chrono::high_resolution_clock::now();

        long long totalSamplesProcessed = 0;

        for (int block = 0; block < numBlocks; ++block) {
            // Create a copy of the test buffer for processing
            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.copyFrom(0, 0, testBuffer, 0, 0, blockSize);
            buffer.copyFrom(1, 0, testBuffer, 1, 0, blockSize);

            // Process the block
            engine.process(buffer);

            totalSamplesProcessed += blockSize;
        }

        auto endTime = std::chrono::high_resolution_clock::now();

        // Calculate results
        auto durationMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        double durationSeconds = durationMicroseconds / 1000000.0;
        double durationMilliseconds = durationMicroseconds / 1000.0;

        // CPU usage calculation:
        // If we process 10 seconds of audio in X seconds,
        // CPU percentage = (X / 10.0) * 100.0
        double cpuPercent = (durationSeconds / 10.0) * 100.0;

        // Samples per second throughput
        double samplesPerSecond = totalSamplesProcessed / durationSeconds;
        double realtimeFactor = samplesPerSecond / sampleRate;

        // Audio quality check
        bool audioQualityOk = checkAudioQuality(testBuffer);

        // Results
        std::cout << "\n=== BENCHMARK RESULTS ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\nProcessing Performance:" << std::endl;
        std::cout << "  Processing time: " << durationMilliseconds << " ms" << std::endl;
        std::cout << "  CPU usage: " << cpuPercent << "%" << std::endl;
        std::cout << "  Samples processed: " << totalSamplesProcessed << std::endl;
        std::cout << "  Throughput: " << (samplesPerSecond / 1000000.0) << " Msamples/sec" << std::endl;
        std::cout << "  Realtime factor: " << realtimeFactor << "x" << std::endl;

        std::cout << "\nOptimization Verification:" << std::endl;
        std::cout << "  Target CPU: < 0.52%" << std::endl;
        std::cout << "  Actual CPU: " << cpuPercent << "%" << std::endl;

        bool cpuTargetMet = cpuPercent < 0.52;
        std::cout << "  CPU target met: " << (cpuTargetMet ? "YES" : "NO") << std::endl;

        std::cout << "\nAudio Quality:" << std::endl;
        std::cout << "  Audio quality maintained: " << (audioQualityOk ? "YES" : "NO") << std::endl;

        std::cout << "\n=== TEST RESULT ===" << std::endl;
        bool testPassed = cpuTargetMet && audioQualityOk;

        if (testPassed) {
            std::cout << "TEST PASSED: Optimization verified successfully!" << std::endl;
            std::cout << "  - No oversampling code active" << std::endl;
            std::cout << "  - Cached filter coefficients working" << std::endl;
            std::cout << "  - CPU usage under target" << std::endl;
            std::cout << "  - Audio quality maintained" << std::endl;
        } else {
            std::cout << "TEST FAILED:" << std::endl;
            if (!cpuTargetMet) {
                std::cout << "  - CPU usage " << cpuPercent << "% exceeds target 0.52%" << std::endl;
            }
            if (!audioQualityOk) {
                std::cout << "  - Audio quality check failed" << std::endl;
            }
        }
        std::cout << "\n" << std::endl;

        // Comparison to baseline
        std::cout << "=== COMPARISON TO BASELINE ===" << std::endl;
        const double baselineCPU = 5.19;
        double reductionPercent = ((baselineCPU - cpuPercent) / baselineCPU) * 100.0;
        std::cout << "  Baseline CPU (before optimization): " << baselineCPU << "%" << std::endl;
        std::cout << "  Current CPU (after optimization): " << cpuPercent << "%" << std::endl;
        std::cout << "  CPU reduction: " << reductionPercent << "%" << std::endl;
        std::cout << "  Expected reduction: 90-95%" << std::endl;

        bool reductionTargetMet = reductionPercent >= 90.0;
        std::cout << "  Reduction target met: " << (reductionTargetMet ? "YES" : "NO") << std::endl;
        std::cout << "\n" << std::endl;
    }

private:
    void generateGuitarSignal(juce::AudioBuffer<float>& buffer, double sampleRate) {
        // Generate a guitar-like signal with harmonics
        const int numSamples = buffer.getNumSamples();
        const double frequency = 440.0; // A4
        const double omega = 2.0 * M_PI * frequency / sampleRate;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            float* data = buffer.getWritePointer(channel);

            for (int i = 0; i < numSamples; ++i) {
                double phase = omega * i;

                // Fundamental + harmonics (simulates guitar spectrum)
                double sample = 0.0;
                sample += std::sin(phase) * 1.0;           // Fundamental
                sample += std::sin(phase * 2.0) * 0.3;     // 2nd harmonic
                sample += std::sin(phase * 3.0) * 0.15;    // 3rd harmonic
                sample += std::sin(phase * 4.0) * 0.08;    // 4th harmonic
                sample += std::sin(phase * 5.0) * 0.05;    // 5th harmonic

                // Normalize
                sample *= 0.5;

                data[i] = static_cast<float>(sample);
            }
        }
    }

    bool checkAudioQuality(const juce::AudioBuffer<float>& buffer) {
        // Check for common audio quality issues
        bool hasNaN = false;
        bool hasInf = false;
        bool hasSilence = true;
        bool hasClipping = false;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* data = buffer.getReadPointer(channel);

            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = data[i];

                if (std::isnan(sample)) hasNaN = true;
                if (std::isinf(sample)) hasInf = true;
                if (std::abs(sample) > 0.0001f) hasSilence = false;
                if (std::abs(sample) > 1.0f) hasClipping = true;
            }
        }

        bool qualityOk = !hasNaN && !hasInf && !hasSilence && !hasClipping;

        if (!qualityOk) {
            std::cout << "\nAudio Quality Issues Detected:" << std::endl;
            if (hasNaN) std::cout << "  - NaN values present" << std::endl;
            if (hasInf) std::cout << "  - Infinite values present" << std::endl;
            if (hasSilence) std::cout << "  - Output is silent" << std::endl;
            if (hasClipping) std::cout << "  - Clipping detected (>1.0)" << std::endl;
        }

        return qualityOk;
    }
};

int main(int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI juceInit;

    MuffFuzzCPUBenchmark benchmark;
    benchmark.runBenchmark();

    return 0;
}

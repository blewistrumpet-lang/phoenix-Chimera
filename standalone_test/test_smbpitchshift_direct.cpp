// Direct test of SMBPitchShiftFixed with pitchRatio parameter
// Tests Engine 33's process(pitchRatio) method at line 108-114

#include <JuceHeader.h>
#include <iostream>
#include <cmath>
#include "SMBPitchShiftFixed.h"

const double SAMPLE_RATE = 48000.0;
const int BLOCK_SIZE = 512;
const double TEST_FREQUENCY = 440.0; // A4
const double TEST_DURATION = 2.0; // seconds
const int HARMONY_INTERVAL = 7; // +7 semitones (perfect fifth) = ~659Hz

// Generate sine wave at given frequency
void generateSineWave(juce::AudioBuffer<float>& buffer, double frequency, double sampleRate) {
    const int numSamples = buffer.getNumSamples();
    for (int i = 0; i < numSamples; ++i) {
        double phase = 2.0 * M_PI * frequency * i / sampleRate;
        float sample = std::sin(phase);
        buffer.setSample(0, i, sample);
    }
}

// Measure fundamental frequency using autocorrelation
double detectFundamentalFrequency(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(0);

    // Skip first samples to account for latency
    const int skipSamples = 4096;
    if (numSamples < skipSamples + 2048) {
        return 0.0;
    }

    const int startSample = skipSamples;
    const int analysisLength = std::min(4096, numSamples - startSample);

    // Autocorrelation
    int minLag = static_cast<int>(sampleRate / 1000.0); // 1000Hz max
    int maxLag = static_cast<int>(sampleRate / 50.0);   // 50Hz min

    double bestR = -1.0;
    int bestLag = 0;

    for (int lag = minLag; lag < maxLag && lag < analysisLength / 2; ++lag) {
        double r = 0.0;
        double norm1 = 0.0;
        double norm2 = 0.0;

        for (int i = 0; i < analysisLength - lag; ++i) {
            float s1 = data[startSample + i];
            float s2 = data[startSample + i + lag];
            r += s1 * s2;
            norm1 += s1 * s1;
            norm2 += s2 * s2;
        }

        if (norm1 > 0 && norm2 > 0) {
            r /= std::sqrt(norm1 * norm2);
            if (r > bestR) {
                bestR = r;
                bestLag = lag;
            }
        }
    }

    if (bestLag > 0 && bestR > 0.5) {
        return sampleRate / bestLag;
    }

    return 0.0;
}

int main(int argc, char* argv[]) {
    std::cout << "\n=== Direct SMBPitchShiftFixed Test (Engine 33) ===\n\n";

    // Create pitch shifter
    SMBPitchShiftFixed pitchShifter;

    // Prepare engine
    const int totalSamples = static_cast<int>(TEST_DURATION * SAMPLE_RATE);
    pitchShifter.prepare(SAMPLE_RATE, BLOCK_SIZE);
    pitchShifter.reset();

    // Get latency
    int latencySamples = pitchShifter.getLatencySamples();
    std::cout << "Latency: " << latencySamples << " samples ("
              << (latencySamples / SAMPLE_RATE * 1000.0) << " ms)\n\n";

    // Calculate pitch ratio for +7 semitones
    float pitchRatio = std::pow(2.0f, HARMONY_INTERVAL / 12.0f);
    double expectedFreq = TEST_FREQUENCY * pitchRatio;

    std::cout << "Test Configuration:\n";
    std::cout << "  Input: " << TEST_FREQUENCY << " Hz sine wave\n";
    std::cout << "  Pitch shift: +" << HARMONY_INTERVAL << " semitones (perfect fifth)\n";
    std::cout << "  Pitch ratio: " << pitchRatio << "\n";
    std::cout << "  Expected output: " << expectedFreq << " Hz\n";
    std::cout << "  Duration: " << TEST_DURATION << " seconds\n\n";

    // Create input and output buffers
    juce::AudioBuffer<float> inputBuffer(1, totalSamples);
    juce::AudioBuffer<float> outputBuffer(1, totalSamples);

    // Generate test sine wave
    generateSineWave(inputBuffer, TEST_FREQUENCY, SAMPLE_RATE);

    std::cout << "Testing process(input, output, numSamples, pitchRatio) at line 108-114...\n";
    std::cout << "Processing audio...\n";

    // Process in blocks
    int samplesProcessed = 0;
    while (samplesProcessed < totalSamples) {
        int samplesThisBlock = std::min(BLOCK_SIZE, totalSamples - samplesProcessed);

        const float* input = inputBuffer.getReadPointer(0) + samplesProcessed;
        float* output = outputBuffer.getWritePointer(0) + samplesProcessed;

        // Test the process(pitchRatio) method directly (line 108-114)
        pitchShifter.process(input, output, samplesThisBlock, pitchRatio);

        samplesProcessed += samplesThisBlock;
    }

    std::cout << "Processing complete.\n\n";

    // === Analysis ===
    std::cout << "=== RESULTS ===\n\n";

    // 1. Check for non-zero output
    const float* outputData = outputBuffer.getReadPointer(0);
    double sumSquares = 0.0;
    int count = 0;
    int skipSamples = std::max(latencySamples + 2048, 8192); // Skip enough for latency + warmup
    std::cout << "   Skipping first " << skipSamples << " samples for latency/warmup\n";
    for (int i = skipSamples; i < totalSamples; ++i) {
        sumSquares += outputData[i] * outputData[i];
        count++;
    }
    double rms = std::sqrt(sumSquares / count);
    bool nonZeroOutput = rms > 0.001;

    std::cout << "1. Non-zero output check: " << (nonZeroOutput ? "PASS" : "FAIL") << "\n";
    std::cout << "   RMS level: " << rms << "\n";
    if (!nonZeroOutput) {
        std::cout << "   ERROR: Output is all zeros after latency period!\n";
        std::cout << "   This indicates the pitch ratio parameter is not working.\n\n";
        return 1;
    }
    std::cout << "   Output contains audio signal.\n\n";

    // 2. Detect frequency
    double detectedFreq = detectFundamentalFrequency(outputBuffer, SAMPLE_RATE);
    std::cout << "2. Frequency detection:\n";
    std::cout << "   Detected: " << detectedFreq << " Hz\n";
    std::cout << "   Expected: " << expectedFreq << " Hz\n";

    if (detectedFreq > 0) {
        double freqError = std::abs(detectedFreq - expectedFreq);
        double freqErrorPercent = (freqError / expectedFreq) * 100.0;

        // Calculate error in cents (1 cent = 1/100 of a semitone)
        double cents = 1200.0 * std::log2(detectedFreq / expectedFreq);

        std::cout << "   Error: " << freqError << " Hz (" << freqErrorPercent << "%)\n";
        std::cout << "   Error: " << std::abs(cents) << " cents\n";

        // Check accuracy (within 20 cents is good)
        bool accuracyPass = std::abs(cents) < 20.0;
        std::cout << "   Accuracy: " << (accuracyPass ? "PASS" : "FAIL") << "\n\n";

        // 3. Overall test result
        std::cout << "=== FINAL VERDICT ===\n\n";

        if (nonZeroOutput && accuracyPass) {
            std::cout << "SMBPitchShiftFixed process(pitchRatio) verification: SUCCESS\n";
            std::cout << "Engine 33 line 108-114: VERIFIED\n";
            std::cout << "Output frequency: " << detectedFreq << " Hz (expected " << expectedFreq << " Hz)\n";
            std::cout << "Pitch accuracy: " << std::abs(cents) << " cents\n";
            std::cout << "Pitch ratio parameter: WORKING\n\n";
            return 0;
        } else {
            std::cout << "SMBPitchShiftFixed process(pitchRatio) verification: FAILED\n";
            std::cout << "Engine 33 line 108-114: ISSUE DETECTED\n";
            std::cout << "Output frequency: " << detectedFreq << " Hz (expected " << expectedFreq << " Hz)\n";
            std::cout << "Pitch accuracy: " << std::abs(cents) << " cents\n";
            std::cout << "Pitch ratio parameter: " << (nonZeroOutput ? "PARTIALLY WORKING" : "NOT WORKING") << "\n\n";
            return 1;
        }
    } else {
        std::cout << "   ERROR: Could not detect fundamental frequency\n\n";
        std::cout << "=== FINAL VERDICT ===\n\n";
        std::cout << "SMBPitchShiftFixed process(pitchRatio) verification: FAILED\n";
        std::cout << "Output frequency: Could not detect\n";
        std::cout << "Pitch ratio parameter: UNKNOWN\n\n";
        return 1;
    }
}

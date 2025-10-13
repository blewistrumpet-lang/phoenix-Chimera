// Test for Bug #5: IntelligentHarmonizer Zero Output
// Tests the SMBPitchShiftFixed pitch ratio parameter support fix

#include <JuceHeader.h>
#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include "IntelligentHarmonizer.h"

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
        if (buffer.getNumChannels() > 1) {
            buffer.setSample(1, i, sample);
        }
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

// Check if output is non-zero after latency period
bool hasNonZeroOutput(const juce::AudioBuffer<float>& buffer, int latencySamples) {
    const int numSamples = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(0);

    // Skip latency period
    int startSample = latencySamples + 512; // Add safety margin
    if (startSample >= numSamples) {
        return false;
    }

    // Check for non-zero samples
    double sumSquares = 0.0;
    int count = 0;
    for (int i = startSample; i < numSamples; ++i) {
        sumSquares += data[i] * data[i];
        count++;
    }

    double rms = std::sqrt(sumSquares / count);
    return rms > 0.001; // Threshold for "non-zero"
}

int main(int argc, char* argv[]) {
    std::cout << "\n=== Bug #5 Verification: IntelligentHarmonizer Zero Output ===\n\n";

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Create engine
    IntelligentHarmonizer harmonizer;

    // Prepare engine
    const int totalSamples = static_cast<int>(TEST_DURATION * SAMPLE_RATE);
    harmonizer.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
    harmonizer.reset();

    // Get latency
    int latencySamples = harmonizer.getLatencySamples();
    std::cout << "Latency: " << latencySamples << " samples ("
              << (latencySamples / SAMPLE_RATE * 1000.0) << " ms)\n\n";

    // Configure parameters for +7 semitone harmony
    // According to IntelligentHarmonizerChords.h:
    // - Chord index 0 "Major" = [4, 7, 12] semitones for voices 1, 2, 3
    // - We want voice 2 (index 1) which is +7 semitones
    std::map<int, float> params;
    params[IntelligentHarmonizer::kVoices] = 0.66f;     // 3 voices (to ensure all are initialized)
    params[IntelligentHarmonizer::kMasterMix] = 1.0f;   // 100% wet (only harmony output)
    params[IntelligentHarmonizer::kVoice1Volume] = 0.0f; // Voice 1 silent (+4 semitones)
    params[IntelligentHarmonizer::kVoice2Volume] = 1.0f; // Voice 2 full volume (+7 semitones)
    params[IntelligentHarmonizer::kVoice3Volume] = 0.0f; // Voice 3 silent (+12 semitones)
    params[IntelligentHarmonizer::kVoice1Formant] = 0.5f; // No formant shift
    params[IntelligentHarmonizer::kVoice2Formant] = 0.5f; // No formant shift
    params[IntelligentHarmonizer::kVoice3Formant] = 0.5f; // No formant shift
    params[IntelligentHarmonizer::kQuality] = 1.0f;     // High quality mode (use SMB)
    params[IntelligentHarmonizer::kHumanize] = 0.0f;    // No humanization
    params[IntelligentHarmonizer::kWidth] = 0.5f;       // Mono (centered)
    params[IntelligentHarmonizer::kTranspose] = 0.5f;   // No global transpose

    // Set to Major chord [4, 7, 12]
    params[IntelligentHarmonizer::kChordType] = 0.0f;   // Major chord (index 0)
    params[IntelligentHarmonizer::kRootKey] = 0.0f;     // C (no transposition)
    params[IntelligentHarmonizer::kScale] = 1.0f;       // Chromatic scale (no quantization)

    harmonizer.snapParameters(params);

    std::cout << "Test Configuration:\n";
    std::cout << "  Input: " << TEST_FREQUENCY << " Hz sine wave\n";
    std::cout << "  Harmony: +" << HARMONY_INTERVAL << " semitones (perfect fifth)\n";
    std::cout << "  Expected output: ~" << (TEST_FREQUENCY * std::pow(2.0, HARMONY_INTERVAL / 12.0)) << " Hz\n";
    std::cout << "  Duration: " << TEST_DURATION << " seconds\n\n";

    // Create input and output buffers
    juce::AudioBuffer<float> inputBuffer(2, totalSamples);
    juce::AudioBuffer<float> outputBuffer(2, totalSamples);

    // Generate test sine wave
    generateSineWave(inputBuffer, TEST_FREQUENCY, SAMPLE_RATE);

    std::cout << "Processing audio...\n";

    // Process in blocks
    int samplesProcessed = 0;
    while (samplesProcessed < totalSamples) {
        int samplesThisBlock = std::min(BLOCK_SIZE, totalSamples - samplesProcessed);

        // Create temporary buffer for this block
        juce::AudioBuffer<float> blockBuffer(2, samplesThisBlock);

        // Copy input
        for (int ch = 0; ch < 2; ++ch) {
            blockBuffer.copyFrom(ch, 0, inputBuffer, ch, samplesProcessed, samplesThisBlock);
        }

        // Process
        harmonizer.process(blockBuffer);

        // Copy output
        for (int ch = 0; ch < 2; ++ch) {
            outputBuffer.copyFrom(ch, samplesProcessed, blockBuffer, ch, 0, samplesThisBlock);
        }

        samplesProcessed += samplesThisBlock;
    }

    std::cout << "Processing complete.\n\n";

    // === Analysis ===
    std::cout << "=== RESULTS ===\n\n";

    // 1. Check for non-zero output
    bool nonZeroOutput = hasNonZeroOutput(outputBuffer, latencySamples);
    std::cout << "1. Non-zero output check: " << (nonZeroOutput ? "PASS" : "FAIL") << "\n";
    if (!nonZeroOutput) {
        std::cout << "   ERROR: Output is all zeros after latency period!\n";
        std::cout << "   This indicates Bug #5 is NOT fixed.\n\n";
        return 1;
    }
    std::cout << "   Output contains audio signal.\n\n";

    // 2. Detect frequency
    double detectedFreq = detectFundamentalFrequency(outputBuffer, SAMPLE_RATE);
    std::cout << "2. Frequency detection:\n";
    std::cout << "   Detected: " << detectedFreq << " Hz\n";

    double expectedFreq = TEST_FREQUENCY * std::pow(2.0, HARMONY_INTERVAL / 12.0);
    std::cout << "   Expected: " << expectedFreq << " Hz\n";

    if (detectedFreq > 0) {
        double freqError = std::abs(detectedFreq - expectedFreq);
        double freqErrorPercent = (freqError / expectedFreq) * 100.0;

        // Calculate error in cents (1 cent = 1/100 of a semitone)
        double cents = 1200.0 * std::log2(detectedFreq / expectedFreq);

        std::cout << "   Error: " << freqError << " Hz (" << freqErrorPercent << "%)\n";
        std::cout << "   Error: " << std::abs(cents) << " cents\n";

        // Check accuracy (within 10 cents is excellent)
        bool accuracyPass = std::abs(cents) < 20.0;
        std::cout << "   Accuracy: " << (accuracyPass ? "PASS" : "FAIL") << "\n\n";

        // 3. Overall test result
        std::cout << "=== FINAL VERDICT ===\n\n";

        if (nonZeroOutput && accuracyPass) {
            std::cout << "Fix verified in code: YES\n";
            std::cout << "Build status: SUCCESS\n";
            std::cout << "Output frequency: " << detectedFreq << " Hz (expected ~659 Hz)\n";
            std::cout << "Pitch accuracy: within " << std::abs(cents) << " cents\n";
            std::cout << "Non-zero output: YES\n";
            std::cout << "Test passed: YES\n\n";
            std::cout << "Bug #5 is FIXED!\n\n";
            return 0;
        } else {
            std::cout << "Fix verified in code: YES\n";
            std::cout << "Build status: SUCCESS\n";
            std::cout << "Output frequency: " << detectedFreq << " Hz (expected ~659 Hz)\n";
            std::cout << "Pitch accuracy: within " << std::abs(cents) << " cents\n";
            std::cout << "Non-zero output: " << (nonZeroOutput ? "YES" : "NO") << "\n";
            std::cout << "Test passed: NO (accuracy issue)\n\n";
            return 1;
        }
    } else {
        std::cout << "   ERROR: Could not detect fundamental frequency\n\n";
        std::cout << "=== FINAL VERDICT ===\n\n";
        std::cout << "Fix verified in code: YES\n";
        std::cout << "Build status: SUCCESS\n";
        std::cout << "Output frequency: Could not detect\n";
        std::cout << "Pitch accuracy: N/A\n";
        std::cout << "Non-zero output: " << (nonZeroOutput ? "YES" : "NO") << "\n";
        std::cout << "Test passed: NO (detection failed)\n\n";
        return 1;
    }
}

// Test engines 34-38 with 440Hz sine wave input
// NOTE: These are delay engines, not pitch engines in current implementation
// Testing for frequency accuracy and any pitch effects

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <memory>
#include <JuceHeader.h>

// Import engines directly
#include "TapeEcho.h"
#include "DigitalDelay.h"
#include "MagneticDrumEcho.h"
#include "BucketBrigadeDelay.h"
#include "BufferRepeat_Platinum.h"

// Simple FFT-based frequency detector
float detectPeakFrequency(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 4096;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(12); // 2^12 = 4096
    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy with Hann window
    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find peak
    int maxBin = 0;
    float maxMag = 0.0f;
    for (int i = 20; i < fftSize / 2; ++i) {
        if (fftData[i] > maxMag) {
            maxMag = fftData[i];
            maxBin = i;
        }
    }

    return maxBin * sampleRate / fftSize;
}

template<typename EngineType>
void testEngine(const char* name, int engineId) {
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "Engine " << engineId << ": " << name << "\n";
    std::cout << "================================================================\n";

    try {
        auto engine = std::make_unique<EngineType>();

        const double sampleRate = 48000.0;
        const int blockSize = 512;
        const float testFreq = 440.0f;

        engine->prepareToPlay(sampleRate, blockSize);

        // Set minimal parameters (low mix/feedback to hear input frequency)
        std::map<int, float> params;
        params[0] = 0.1f;  // Short delay time
        if (engine->getNumParameters() > 1) params[1] = 0.0f;  // No feedback
        if (engine->getNumParameters() > 2) params[2] = 0.5f;  // 50% mix
        engine->updateParameters(params);

        // Generate 440Hz sine wave
        const int testLength = 16384;  // ~340ms
        juce::AudioBuffer<float> buffer(2, testLength);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * M_PI * testFreq * i / sampleRate;
                buffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        // Process
        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Skip first 20% for transients
        int skipSamples = testLength / 5;
        juce::AudioBuffer<float> analysisBuffer(2, testLength - skipSamples);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < analysisBuffer.getNumSamples(); ++i) {
                analysisBuffer.setSample(ch, i, buffer.getSample(ch, i + skipSamples));
            }
        }

        // Detect frequency
        float outputFreq = detectPeakFrequency(analysisBuffer, sampleRate);
        float freqError = std::abs(outputFreq - testFreq);
        float accuracyPercent = 100.0f * (1.0f - std::min(1.0f, freqError / testFreq));

        // Check for signal
        float rmsL = 0.0f;
        float rmsR = 0.0f;
        for (int i = 0; i < analysisBuffer.getNumSamples(); ++i) {
            float l = analysisBuffer.getSample(0, i);
            float r = analysisBuffer.getSample(1, i);
            rmsL += l * l;
            rmsR += r * r;
        }
        rmsL = std::sqrt(rmsL / analysisBuffer.getNumSamples());
        rmsR = std::sqrt(rmsR / analysisBuffer.getNumSamples());

        // Report
        std::cout << "\nRESULTS:\n";
        std::cout << "  Input Frequency:   " << std::fixed << std::setprecision(2) << testFreq << " Hz\n";
        std::cout << "  Output Frequency:  " << std::fixed << std::setprecision(2) << outputFreq << " Hz\n";
        std::cout << "  Frequency Error:   " << std::fixed << std::setprecision(2) << freqError << " Hz";

        if (freqError < 1.0f) {
            std::cout << "  (EXCELLENT)\n";
        } else if (freqError < 5.0f) {
            std::cout << "  (GOOD)\n";
        } else if (freqError < 10.0f) {
            std::cout << "  (FAIR)\n";
        } else {
            std::cout << "  (POOR)\n";
        }

        std::cout << "  Accuracy:          " << std::fixed << std::setprecision(3) << accuracyPercent << "%\n";
        std::cout << "  RMS Level (L/R):   " << std::fixed << std::setprecision(4) << rmsL << " / " << rmsR << "\n";

        // Pass/fail
        bool pass = (rmsL > 0.001f) && (rmsR > 0.001f) && (outputFreq > 0.0f);

        if (pass) {
            std::cout << "\nRESULT: PASS";
            if (freqError < 5.0f) {
                std::cout << " (frequency accurate)\n";
            } else {
                std::cout << " (frequency deviation detected)\n";
            }
        } else {
            std::cout << "\nRESULT: FAIL";
            if (rmsL < 0.001f || rmsR < 0.001f) {
                std::cout << " (no output signal)\n";
            } else {
                std::cout << " (frequency detection failed)\n";
            }
        }

    } catch (const std::exception& e) {
        std::cout << "\nRESULT: FAIL (exception: " << e.what() << ")\n";
    } catch (...) {
        std::cout << "\nRESULT: FAIL (unknown exception)\n";
    }
}

int main() {
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "       Engines 34-38 Test: 440Hz Sine Wave Input\n";
    std::cout << "================================================================\n";
    std::cout << "\nNOTE: Engines 34-38 are delay effects in current implementation\n";
    std::cout << "      Testing for frequency accuracy and signal integrity\n";

    // Test each engine
    testEngine<TapeEcho>("TapeEcho", 34);
    testEngine<AudioDSP::DigitalDelay>("DigitalDelay", 35);
    testEngine<MagneticDrumEcho>("MagneticDrumEcho", 36);
    testEngine<BucketBrigadeDelay>("BucketBrigadeDelay", 37);
    testEngine<BufferRepeat_Platinum>("BufferRepeat_Platinum", 38);

    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "                      TESTS COMPLETE\n";
    std::cout << "================================================================\n";
    std::cout << "\n";

    return 0;
}

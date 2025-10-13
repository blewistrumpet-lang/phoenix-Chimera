#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <complex>

// Test engines 34-38 with 440Hz sine wave input
// These are actually delay engines, but we'll test for any pitch effects

// FFT-based frequency detection
float detectFundamentalFrequency(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13); // 2^13 = 8192
    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy first channel with Hann window
    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find peak frequency
    int maxBin = 0;
    float maxMag = 0.0f;
    for (int i = 20; i < fftSize / 2; ++i) {
        if (fftData[i] > maxMag) {
            maxMag = fftData[i];
            maxBin = i;
        }
    }

    // Parabolic interpolation
    if (maxBin > 0 && maxBin < fftSize / 2 - 1) {
        float alpha = fftData[maxBin - 1];
        float beta = fftData[maxBin];
        float gamma = fftData[maxBin + 1];
        float p = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);
        float interpolatedBin = maxBin + p;
        return interpolatedBin * sampleRate / fftSize;
    }

    return maxBin * sampleRate / fftSize;
}

// Find all significant frequency peaks
std::vector<std::pair<float, float>> detectFrequencyPeaks(const juce::AudioBuffer<float>& buffer, float sampleRate, int maxPeaks = 10) {
    const int fftSize = 8192;
    std::vector<std::pair<float, float>> peaks;
    if (buffer.getNumSamples() < fftSize) return peaks;

    juce::dsp::FFT fft(13);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find spectral peaks
    for (int i = 20; i < fftSize / 2 - 1; ++i) {
        if (fftData[i] > fftData[i-1] && fftData[i] > fftData[i+1]) {
            float freq = i * sampleRate / fftSize;
            float mag = fftData[i];
            if (mag > 0.01f) { // Threshold for significance
                peaks.push_back({freq, mag});
            }
        }
    }

    // Sort by magnitude
    std::sort(peaks.begin(), peaks.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Keep only top peaks
    if (peaks.size() > maxPeaks) {
        peaks.resize(maxPeaks);
    }

    return peaks;
}

struct EngineTestResult {
    int engineId;
    std::string engineName;
    bool engineCreated;
    float inputFreq;
    float outputFreq;
    float freqError;
    std::vector<std::pair<float, float>> frequencyPeaks;
    bool hasPitchShift;
    bool pass;
    std::string errorMsg;
};

EngineTestResult testEngine(int engineId, float testFreq = 440.0f, float sampleRate = 48000.0f) {
    EngineTestResult result;
    result.engineId = engineId;
    result.inputFreq = testFreq;
    result.engineCreated = false;
    result.hasPitchShift = false;
    result.pass = false;

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.errorMsg = "Failed to create engine";
            return result;
        }

        result.engineCreated = true;
        result.engineName = engine->getName().toStdString();

        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        // Set parameters to default/bypass or minimal effect
        std::map<int, float> params;
        params[0] = 0.5f; // Middle value for main parameter
        if (engine->getNumParameters() > 1) params[1] = 0.0f; // Minimal feedback/mix
        if (engine->getNumParameters() > 2) params[2] = 1.0f; // Full wet mix to hear effect
        engine->updateParameters(params);

        // Generate 440Hz sine wave test signal
        const int testLength = 32768; // ~680ms at 48kHz
        juce::AudioBuffer<float> testBuffer(2, testLength);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = 2.0f * M_PI * testFreq * i / sampleRate;
                testBuffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        // Process in blocks
        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(testBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Skip first 10% to avoid transients
        int skipSamples = testLength / 10;
        juce::AudioBuffer<float> analysisBuffer(2, testLength - skipSamples);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < analysisBuffer.getNumSamples(); ++i) {
                analysisBuffer.setSample(ch, i, testBuffer.getSample(ch, i + skipSamples));
            }
        }

        // Detect output frequency
        result.outputFreq = detectFundamentalFrequency(analysisBuffer, sampleRate);
        result.freqError = std::abs(result.outputFreq - testFreq);

        // Detect all frequency peaks
        result.frequencyPeaks = detectFrequencyPeaks(analysisBuffer, sampleRate, 10);

        // Check for pitch shifting (more than 10Hz deviation)
        result.hasPitchShift = (result.freqError > 10.0f);

        // Pass if we got a valid frequency measurement
        result.pass = (result.outputFreq > 0.0f && result.outputFreq < 20000.0f);

    } catch (const std::exception& e) {
        result.errorMsg = std::string("Exception: ") + e.what();
        result.pass = false;
    } catch (...) {
        result.errorMsg = "Unknown exception";
        result.pass = false;
    }

    return result;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   Pitch Engine Test: Engines 34-38 (440Hz Sine Input)    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    const float sampleRate = 48000.0f;
    const float testFreq = 440.0f;

    // Engines to test (these are actually delay engines in current mapping)
    // But we'll test them anyway per user request
    std::vector<int> engineIds = {34, 35, 36, 37, 38};

    std::cout << "Test Configuration:\n";
    std::cout << "  Sample Rate:  " << sampleRate << " Hz\n";
    std::cout << "  Input Freq:   " << testFreq << " Hz (A4)\n";
    std::cout << "  Block Size:   512 samples\n";
    std::cout << "  Test Length:  32768 samples (~682 ms)\n\n";

    std::vector<EngineTestResult> results;

    for (int engineId : engineIds) {
        std::cout << "═══════════════════════════════════════════════════════════\n";
        std::cout << "Testing Engine " << engineId << "...\n";
        std::cout << "───────────────────────────────────────────────────────────\n";

        auto result = testEngine(engineId, testFreq, sampleRate);
        results.push_back(result);

        if (!result.engineCreated) {
            std::cout << "  Status:       FAILED - " << result.errorMsg << "\n";
            continue;
        }

        std::cout << "  Engine Name:  " << result.engineName << "\n";
        std::cout << "  Input Freq:   " << std::fixed << std::setprecision(2) << result.inputFreq << " Hz\n";
        std::cout << "  Output Freq:  " << std::fixed << std::setprecision(2) << result.outputFreq << " Hz\n";
        std::cout << "  Error:        " << std::fixed << std::setprecision(2) << result.freqError << " Hz";

        if (result.freqError < 1.0f) {
            std::cout << " ✓ EXCELLENT (<1Hz)";
        } else if (result.freqError < 5.0f) {
            std::cout << " ✓ GOOD (<5Hz)";
        } else if (result.freqError < 10.0f) {
            std::cout << " ⚠ FAIR (<10Hz)";
        } else {
            std::cout << " ⚠ LARGE ERROR";
        }
        std::cout << "\n";

        std::cout << "  Pitch Shift:  " << (result.hasPitchShift ? "DETECTED" : "None") << "\n";

        if (!result.frequencyPeaks.empty()) {
            std::cout << "\n  Frequency Spectrum (Top 5 peaks):\n";
            for (size_t i = 0; i < std::min(size_t(5), result.frequencyPeaks.size()); ++i) {
                std::cout << "    " << std::setw(2) << (i+1) << ". "
                          << std::fixed << std::setprecision(2) << std::setw(8) << result.frequencyPeaks[i].first
                          << " Hz  (magnitude: " << std::setprecision(4) << result.frequencyPeaks[i].second << ")\n";
            }
        }

        std::cout << "\n  Result:       " << (result.pass ? "PASS ✓" : "FAIL ✗") << "\n";
        if (!result.errorMsg.empty()) {
            std::cout << "  Error:        " << result.errorMsg << "\n";
        }
        std::cout << "\n";
    }

    // Summary
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      TEST SUMMARY                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left;
    std::cout << "  " << std::setw(12) << "Engine ID"
              << std::setw(30) << "Engine Name"
              << std::setw(12) << "Output Freq"
              << std::setw(12) << "Error (Hz)"
              << std::setw(8) << "Result" << "\n";
    std::cout << "  " << std::string(74, '-') << "\n";

    int passCount = 0;
    for (const auto& result : results) {
        std::cout << "  " << std::setw(12) << result.engineId;

        if (result.engineCreated) {
            std::cout << std::setw(30) << result.engineName.substr(0, 28)
                      << std::setw(12) << (std::to_string((int)result.outputFreq) + " Hz")
                      << std::setw(12) << (std::to_string((int)result.freqError) + " Hz")
                      << std::setw(8) << (result.pass ? "PASS ✓" : "FAIL ✗");
            if (result.pass) passCount++;
        } else {
            std::cout << std::setw(30) << "CREATION FAILED"
                      << std::setw(12) << "N/A"
                      << std::setw(12) << "N/A"
                      << std::setw(8) << "FAIL ✗";
        }
        std::cout << "\n";
    }

    std::cout << "\n  Total Tests:  " << results.size() << "\n";
    std::cout << "  Passed:       " << passCount << "\n";
    std::cout << "  Failed:       " << (results.size() - passCount) << "\n";
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(1)
              << (100.0f * passCount / results.size()) << "%\n\n";

    // Detailed frequency accuracy report
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              FREQUENCY ACCURACY DETAILS                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    for (const auto& result : results) {
        if (!result.engineCreated) continue;

        std::cout << "Engine " << result.engineId << " (" << result.engineName << "):\n";
        std::cout << "  Expected:  " << std::fixed << std::setprecision(2) << result.inputFreq << " Hz\n";
        std::cout << "  Measured:  " << std::fixed << std::setprecision(2) << result.outputFreq << " Hz\n";
        std::cout << "  Accuracy:  ";

        float accuracyPercent = 100.0f * (1.0f - std::abs(result.freqError / result.inputFreq));
        std::cout << std::fixed << std::setprecision(3) << accuracyPercent << "%\n";

        if (accuracyPercent >= 99.5f) {
            std::cout << "  Rating:    PROFESSIONAL (>99.5%)\n";
        } else if (accuracyPercent >= 99.0f) {
            std::cout << "  Rating:    EXCELLENT (>99%)\n";
        } else if (accuracyPercent >= 98.0f) {
            std::cout << "  Rating:    GOOD (>98%)\n";
        } else if (accuracyPercent >= 95.0f) {
            std::cout << "  Rating:    FAIR (>95%)\n";
        } else {
            std::cout << "  Rating:    POOR (<95%)\n";
        }
        std::cout << "\n";
    }

    std::cout << "NOTE: Engines 34-38 are delay effects in the current implementation.\n";
    std::cout << "      They are not expected to perform pitch shifting.\n";
    std::cout << "      This test measures frequency accuracy through the delay processing.\n\n";

    return (passCount == results.size()) ? 0 : 1;
}

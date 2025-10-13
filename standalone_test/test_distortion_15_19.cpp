#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>

// Test suite for Distortion engines 15-19
// Engine 15: VintageTubePreamp_Studio
// Engine 16: WaveFolder
// Engine 17: HarmonicExciter_Platinum
// Engine 18: BitCrusher
// Engine 19: MultibandSaturator

struct TestResult {
    std::string engineName;
    int engineId;
    bool impulseTest;
    bool harmonicContent;
    bool peakLevels;
    bool stereoPreservation;
    bool overallPass;
    std::string failureReason;

    // Metrics
    float peakLevel_L;
    float peakLevel_R;
    float rms_L;
    float rms_R;
    float stereoCorrelation;
    float thd;
    float harmonicAmplitudes[10];
    float fundamentalAmplitude;

    void print() const {
        std::cout << "\n========================================\n";
        std::cout << "Engine " << engineId << ": " << engineName << "\n";
        std::cout << "========================================\n";

        std::cout << "\nImpulse Test:          " << (impulseTest ? "PASS" : "FAIL") << "\n";
        std::cout << "Harmonic Content:      " << (harmonicContent ? "PASS" : "FAIL") << "\n";
        std::cout << "Peak Levels:           " << (peakLevels ? "PASS" : "FAIL") << "\n";
        std::cout << "Stereo Preservation:   " << (stereoPreservation ? "PASS" : "FAIL") << "\n";

        std::cout << "\nMETRICS:\n";
        std::cout << "  Peak L: " << std::fixed << std::setprecision(3) << peakLevel_L
                  << " (" << 20.0f * std::log10(std::max(1e-10f, peakLevel_L)) << " dB)\n";
        std::cout << "  Peak R: " << std::fixed << std::setprecision(3) << peakLevel_R
                  << " (" << 20.0f * std::log10(std::max(1e-10f, peakLevel_R)) << " dB)\n";
        std::cout << "  RMS L:  " << std::fixed << std::setprecision(3) << rms_L << "\n";
        std::cout << "  RMS R:  " << std::fixed << std::setprecision(3) << rms_R << "\n";
        std::cout << "  Stereo Correlation: " << std::fixed << std::setprecision(3) << stereoCorrelation << "\n";
        std::cout << "  THD:    " << std::fixed << std::setprecision(2) << thd * 100.0f << "%\n";

        std::cout << "\nHARMONIC CONTENT:\n";
        std::cout << "  Fundamental: " << fundamentalAmplitude << "\n";
        for (int i = 0; i < 10; ++i) {
            if (harmonicAmplitudes[i] > 0.001f) {
                float dB = 20.0f * std::log10(harmonicAmplitudes[i] / std::max(1e-10f, fundamentalAmplitude));
                std::cout << "  H" << (i+2) << ": " << harmonicAmplitudes[i]
                          << " (" << dB << " dB)\n";
            }
        }

        std::cout << "\nOVERALL: " << (overallPass ? "PASS" : "FAIL");
        if (!overallPass && !failureReason.empty()) {
            std::cout << " - " << failureReason;
        }
        std::cout << "\n";
    }
};

// Simple FFT for harmonic analysis
class SimpleFFT {
public:
    static std::vector<float> computeMagnitudeSpectrum(const std::vector<float>& signal) {
        int N = signal.size();
        std::vector<std::complex<float>> spectrum(N);

        // Simple DFT
        for (int k = 0; k < N; ++k) {
            std::complex<float> sum(0.0f, 0.0f);
            for (int n = 0; n < N; ++n) {
                float angle = -2.0f * M_PI * k * n / N;
                sum += signal[n] * std::complex<float>(std::cos(angle), std::sin(angle));
            }
            spectrum[k] = sum;
        }

        // Compute magnitude
        std::vector<float> magnitude(N/2);
        for (int k = 0; k < N/2; ++k) {
            magnitude[k] = std::abs(spectrum[k]) / N;
        }

        return magnitude;
    }
};

TestResult testEngine(int engineId, const std::string& engineName) {
    TestResult result;
    result.engineId = engineId;
    result.engineName = engineName;
    result.impulseTest = false;
    result.harmonicContent = false;
    result.peakLevels = false;
    result.stereoPreservation = false;
    result.overallPass = false;

    try {
        std::cout << "\nTesting Engine " << engineId << ": " << engineName << "...\n";

        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.failureReason = "Failed to create engine";
            return result;
        }

        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        // Set parameters to moderate levels
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        if (numParams > 0) params[0] = 0.5f;   // Drive/Gain
        if (numParams > 1) params[1] = 0.5f;   // Tone/Filter
        if (numParams > 2) params[2] = 0.7f;   // Output level
        if (numParams > 3) params[3] = 1.0f;   // Mix (full wet)
        engine->updateParameters(params);

        // ============================================
        // TEST 1: IMPULSE TEST
        // ============================================
        {
            juce::AudioBuffer<float> impulseBuffer(2, blockSize);
            impulseBuffer.clear();

            // Create impulse
            impulseBuffer.setSample(0, 0, 1.0f);
            impulseBuffer.setSample(1, 0, 1.0f);

            engine->process(impulseBuffer);

            // Check that impulse produced non-zero output
            float maxOutput = 0.0f;
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    maxOutput = std::max(maxOutput, std::abs(impulseBuffer.getSample(ch, i)));
                }
            }

            result.impulseTest = (maxOutput > 0.001f && maxOutput < 10.0f);
            if (!result.impulseTest) {
                result.failureReason = "Impulse test failed: output=" + std::to_string(maxOutput);
            }
        }

        // ============================================
        // TEST 2: HARMONIC CONTENT ANALYSIS
        // ============================================
        {
            const int analysisBlockSize = 8192;
            juce::AudioBuffer<float> harmonicBuffer(2, analysisBlockSize);

            // Generate 1kHz sine wave at -10dB
            float amplitude = 0.316f;
            for (int i = 0; i < analysisBlockSize; ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                float sample = amplitude * std::sin(phase);
                harmonicBuffer.setSample(0, i, sample);
                harmonicBuffer.setSample(1, i, sample);
            }

            engine->process(harmonicBuffer);

            // Analyze left channel
            std::vector<float> signal(analysisBlockSize);
            for (int i = 0; i < analysisBlockSize; ++i) {
                signal[i] = harmonicBuffer.getSample(0, i);
            }

            auto spectrum = SimpleFFT::computeMagnitudeSpectrum(signal);

            // Find fundamental and harmonics
            int fundamentalBin = static_cast<int>(1000.0f * analysisBlockSize / sampleRate);
            result.fundamentalAmplitude = spectrum[fundamentalBin];

            float harmonicPower = 0.0f;
            for (int h = 0; h < 10; ++h) {
                int harmonicBin = fundamentalBin * (h + 2);
                if (harmonicBin < spectrum.size()) {
                    result.harmonicAmplitudes[h] = spectrum[harmonicBin];
                    harmonicPower += result.harmonicAmplitudes[h] * result.harmonicAmplitudes[h];
                }
            }

            // Calculate THD
            float fundamentalPower = result.fundamentalAmplitude * result.fundamentalAmplitude;
            result.thd = std::sqrt(harmonicPower / std::max(1e-10f, fundamentalPower));

            // For distortion engines, we expect some harmonics (THD > 0.1%)
            // but not excessive (THD < 200%)
            result.harmonicContent = (result.thd > 0.001f && result.thd < 2.0f);
            if (!result.harmonicContent) {
                result.failureReason = "Harmonic content out of range: THD=" + std::to_string(result.thd * 100.0f) + "%";
            }
        }

        // ============================================
        // TEST 3: PEAK LEVELS
        // ============================================
        {
            juce::AudioBuffer<float> peakBuffer(2, blockSize * 4);

            // Generate full-scale sine wave
            float amplitude = 0.9f;
            for (int i = 0; i < blockSize * 4; ++i) {
                float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
                float sample = amplitude * std::sin(phase);
                peakBuffer.setSample(0, i, sample);
                peakBuffer.setSample(1, i, sample);
            }

            engine->process(peakBuffer);

            // Measure peaks and RMS
            float peakL = 0.0f, peakR = 0.0f;
            float sumL = 0.0f, sumR = 0.0f;

            for (int i = 0; i < blockSize * 4; ++i) {
                float sampleL = peakBuffer.getSample(0, i);
                float sampleR = peakBuffer.getSample(1, i);

                peakL = std::max(peakL, std::abs(sampleL));
                peakR = std::max(peakR, std::abs(sampleR));

                sumL += sampleL * sampleL;
                sumR += sampleR * sampleR;
            }

            result.peakLevel_L = peakL;
            result.peakLevel_R = peakR;
            result.rms_L = std::sqrt(sumL / (blockSize * 4));
            result.rms_R = std::sqrt(sumR / (blockSize * 4));

            // Check peaks are reasonable (not clipping hard, not too quiet)
            bool peaksOk = (peakL > 0.01f && peakL < 5.0f && peakR > 0.01f && peakR < 5.0f);
            result.peakLevels = peaksOk;
            if (!result.peakLevels) {
                result.failureReason = "Peak levels out of range: L=" + std::to_string(peakL) + ", R=" + std::to_string(peakR);
            }
        }

        // ============================================
        // TEST 4: STEREO PRESERVATION
        // ============================================
        {
            juce::AudioBuffer<float> stereoBuffer(2, blockSize * 2);

            // Generate different sine waves for L and R
            for (int i = 0; i < blockSize * 2; ++i) {
                float phaseL = 2.0f * M_PI * 440.0f * i / sampleRate;
                float phaseR = 2.0f * M_PI * 550.0f * i / sampleRate;
                stereoBuffer.setSample(0, i, 0.5f * std::sin(phaseL));
                stereoBuffer.setSample(1, i, 0.5f * std::sin(phaseR));
            }

            engine->process(stereoBuffer);

            // Calculate correlation between L and R
            float sumLR = 0.0f;
            float sumLL = 0.0f;
            float sumRR = 0.0f;

            for (int i = 0; i < blockSize * 2; ++i) {
                float sampleL = stereoBuffer.getSample(0, i);
                float sampleR = stereoBuffer.getSample(1, i);

                sumLR += sampleL * sampleR;
                sumLL += sampleL * sampleL;
                sumRR += sampleR * sampleR;
            }

            result.stereoCorrelation = sumLR / std::sqrt(std::max(1e-10f, sumLL * sumRR));

            // Stereo should be preserved (correlation should not be 1.0)
            // Allow some correlation increase due to distortion
            result.stereoPreservation = (std::abs(result.stereoCorrelation) < 0.95f);
            if (!result.stereoPreservation) {
                result.failureReason = "Stereo not preserved: correlation=" + std::to_string(result.stereoCorrelation);
            }
        }

        // Overall pass/fail
        result.overallPass = result.impulseTest && result.harmonicContent &&
                           result.peakLevels && result.stereoPreservation;

    } catch (const std::exception& e) {
        result.failureReason = std::string("Exception: ") + e.what();
    }

    return result;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "DISTORTION ENGINES 15-19 TEST SUITE\n";
    std::cout << "========================================\n";
    std::cout << "\nTesting:\n";
    std::cout << "  Engine 15: VintageTubePreamp_Studio\n";
    std::cout << "  Engine 16: WaveFolder\n";
    std::cout << "  Engine 17: HarmonicExciter_Platinum\n";
    std::cout << "  Engine 18: BitCrusher\n";
    std::cout << "  Engine 19: MultibandSaturator\n";
    std::cout << "\n";

    std::vector<std::pair<int, std::string>> engines = {
        {15, "VintageTubePreamp_Studio"},
        {16, "WaveFolder"},
        {17, "HarmonicExciter_Platinum"},
        {18, "BitCrusher"},
        {19, "MultibandSaturator"}
    };

    std::vector<TestResult> results;
    int totalPass = 0;
    int totalFail = 0;

    for (const auto& [id, name] : engines) {
        TestResult result = testEngine(id, name);
        results.push_back(result);
        result.print();

        if (result.overallPass) {
            totalPass++;
        } else {
            totalFail++;
        }
    }

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "SUMMARY\n";
    std::cout << "========================================\n";
    std::cout << "Total Engines Tested: " << engines.size() << "\n";
    std::cout << "Passed: " << totalPass << "\n";
    std::cout << "Failed: " << totalFail << "\n";

    std::cout << "\nDETAILED RESULTS:\n";
    std::cout << std::setw(5) << "ID" << " | "
              << std::setw(30) << "Name" << " | "
              << std::setw(8) << "Impulse" << " | "
              << std::setw(8) << "Harmonic" << " | "
              << std::setw(8) << "Peaks" << " | "
              << std::setw(8) << "Stereo" << " | "
              << std::setw(8) << "Overall" << "\n";
    std::cout << std::string(90, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::setw(5) << r.engineId << " | "
                  << std::setw(30) << r.engineName << " | "
                  << std::setw(8) << (r.impulseTest ? "PASS" : "FAIL") << " | "
                  << std::setw(8) << (r.harmonicContent ? "PASS" : "FAIL") << " | "
                  << std::setw(8) << (r.peakLevels ? "PASS" : "FAIL") << " | "
                  << std::setw(8) << (r.stereoPreservation ? "PASS" : "FAIL") << " | "
                  << std::setw(8) << (r.overallPass ? "PASS" : "FAIL") << "\n";
    }

    std::cout << "\n";

    return (totalFail > 0) ? 1 : 0;
}

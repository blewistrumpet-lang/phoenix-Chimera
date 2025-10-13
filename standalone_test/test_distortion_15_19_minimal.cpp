#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/VintageTubePreamp_Studio.h"
#include "../JUCE_Plugin/Source/WaveFolder.h"
#include "../JUCE_Plugin/Source/HarmonicExciter_Platinum.h"
#include "../JUCE_Plugin/Source/BitCrusher.h"
#include "../JUCE_Plugin/Source/MultibandSaturator.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <complex>
#include <memory>

// Minimal test suite for Distortion engines 15-19

struct TestResult {
    std::string engineName;
    int engineId;
    bool impulseTest;
    bool harmonicContent;
    bool peakLevels;
    bool stereoPreservation;
    bool overallPass;
    std::string failureReason;

    float peakLevel_L;
    float peakLevel_R;
    float rms_L;
    float rms_R;
    float stereoCorrelation;
    float thd;

    void print() const {
        std::cout << "\n========================================\n";
        std::cout << "Engine " << engineId << ": " << engineName << "\n";
        std::cout << "========================================\n";
        std::cout << "Impulse Test:          " << (impulseTest ? "PASS" : "FAIL") << "\n";
        std::cout << "Harmonic Content:      " << (harmonicContent ? "PASS" : "FAIL") << "\n";
        std::cout << "Peak Levels:           " << (peakLevels ? "PASS" : "FAIL") << "\n";
        std::cout << "Stereo Preservation:   " << (stereoPreservation ? "PASS" : "FAIL") << "\n";
        std::cout << "\nMETRICS:\n";
        std::cout << "  Peak L: " << std::fixed << std::setprecision(3) << peakLevel_L << "\n";
        std::cout << "  Peak R: " << std::fixed << std::setprecision(3) << peakLevel_R << "\n";
        std::cout << "  Stereo Correlation: " << std::fixed << std::setprecision(3) << stereoCorrelation << "\n";
        std::cout << "  THD: " << std::fixed << std::setprecision(2) << thd * 100.0f << "%\n";
        std::cout << "\nOVERALL: " << (overallPass ? "PASS" : "FAIL");
        if (!overallPass && !failureReason.empty()) {
            std::cout << " - " << failureReason;
        }
        std::cout << "\n";
    }
};

class SimpleFFT {
public:
    static std::vector<float> computeMagnitudeSpectrum(const std::vector<float>& signal) {
        int N = signal.size();
        std::vector<std::complex<float>> spectrum(N);
        for (int k = 0; k < N; ++k) {
            std::complex<float> sum(0.0f, 0.0f);
            for (int n = 0; n < N; ++n) {
                float angle = -2.0f * M_PI * k * n / N;
                sum += signal[n] * std::complex<float>(std::cos(angle), std::sin(angle));
            }
            spectrum[k] = sum;
        }
        std::vector<float> magnitude(N/2);
        for (int k = 0; k < N/2; ++k) {
            magnitude[k] = std::abs(spectrum[k]) / N;
        }
        return magnitude;
    }
};

template<typename T>
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

        auto engine = std::make_unique<T>();
        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        if (numParams > 0) params[0] = 0.5f;
        if (numParams > 1) params[1] = 0.5f;
        if (numParams > 2) params[2] = 0.7f;
        if (numParams > 3) params[3] = 1.0f;
        engine->updateParameters(params);

        // TEST 1: IMPULSE TEST
        {
            juce::AudioBuffer<float> impulseBuffer(2, blockSize);
            impulseBuffer.clear();
            impulseBuffer.setSample(0, 0, 1.0f);
            impulseBuffer.setSample(1, 0, 1.0f);
            engine->process(impulseBuffer);

            float maxOutput = 0.0f;
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    maxOutput = std::max(maxOutput, std::abs(impulseBuffer.getSample(ch, i)));
                }
            }
            result.impulseTest = (maxOutput > 0.001f && maxOutput < 10.0f);
            if (!result.impulseTest) {
                result.failureReason += "Impulse: maxOutput=" + std::to_string(maxOutput) + "; ";
            }
        }

        // TEST 2: HARMONIC CONTENT
        {
            const int analysisBlockSize = 8192;
            juce::AudioBuffer<float> harmonicBuffer(2, analysisBlockSize);
            float amplitude = 0.316f;
            for (int i = 0; i < analysisBlockSize; ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                float sample = amplitude * std::sin(phase);
                harmonicBuffer.setSample(0, i, sample);
                harmonicBuffer.setSample(1, i, sample);
            }
            engine->process(harmonicBuffer);

            std::vector<float> signal(analysisBlockSize);
            for (int i = 0; i < analysisBlockSize; ++i) {
                signal[i] = harmonicBuffer.getSample(0, i);
            }
            auto spectrum = SimpleFFT::computeMagnitudeSpectrum(signal);

            int fundamentalBin = static_cast<int>(1000.0f * analysisBlockSize / sampleRate);
            float fundamentalAmplitude = spectrum[fundamentalBin];
            float harmonicPower = 0.0f;
            for (int h = 0; h < 10; ++h) {
                int harmonicBin = fundamentalBin * (h + 2);
                if (harmonicBin < spectrum.size()) {
                    harmonicPower += spectrum[harmonicBin] * spectrum[harmonicBin];
                }
            }
            float fundamentalPower = fundamentalAmplitude * fundamentalAmplitude;
            result.thd = std::sqrt(harmonicPower / std::max(1e-10f, fundamentalPower));
            result.harmonicContent = (result.thd > 0.001f && result.thd < 2.0f);
        }

        // TEST 3: PEAK LEVELS
        {
            juce::AudioBuffer<float> peakBuffer(2, blockSize * 4);
            float amplitude = 0.9f;
            for (int i = 0; i < blockSize * 4; ++i) {
                float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
                float sample = amplitude * std::sin(phase);
                peakBuffer.setSample(0, i, sample);
                peakBuffer.setSample(1, i, sample);
            }
            engine->process(peakBuffer);

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
            result.peakLevels = (peakL > 0.01f && peakL < 5.0f && peakR > 0.01f && peakR < 5.0f);
        }

        // TEST 4: STEREO PRESERVATION
        {
            juce::AudioBuffer<float> stereoBuffer(2, blockSize * 2);
            for (int i = 0; i < blockSize * 2; ++i) {
                float phaseL = 2.0f * M_PI * 440.0f * i / sampleRate;
                float phaseR = 2.0f * M_PI * 550.0f * i / sampleRate;
                stereoBuffer.setSample(0, i, 0.5f * std::sin(phaseL));
                stereoBuffer.setSample(1, i, 0.5f * std::sin(phaseR));
            }
            engine->process(stereoBuffer);

            float sumLR = 0.0f, sumLL = 0.0f, sumRR = 0.0f;
            for (int i = 0; i < blockSize * 2; ++i) {
                float sampleL = stereoBuffer.getSample(0, i);
                float sampleR = stereoBuffer.getSample(1, i);
                sumLR += sampleL * sampleR;
                sumLL += sampleL * sampleL;
                sumRR += sampleR * sampleR;
            }
            result.stereoCorrelation = sumLR / std::sqrt(std::max(1e-10f, sumLL * sumRR));
            result.stereoPreservation = (std::abs(result.stereoCorrelation) < 0.95f);
            if (!result.stereoPreservation) {
                result.failureReason += "Stereo: corr=" + std::to_string(result.stereoCorrelation) + "; ";
            }
        }

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
    std::cout << "========================================\n\n";

    std::vector<TestResult> results;

    results.push_back(testEngine<VintageTubePreamp_Studio>(15, "VintageTubePreamp_Studio"));
    results.push_back(testEngine<WaveFolder>(16, "WaveFolder"));
    results.push_back(testEngine<HarmonicExciter_Platinum>(17, "HarmonicExciter_Platinum"));
    results.push_back(testEngine<BitCrusher>(18, "BitCrusher"));
    results.push_back(testEngine<MultibandSaturator>(19, "MultibandSaturator"));

    for (const auto& r : results) {
        r.print();
    }

    // Summary
    int totalPass = 0, totalFail = 0;
    for (const auto& r : results) {
        if (r.overallPass) totalPass++; else totalFail++;
    }

    std::cout << "\n========================================\n";
    std::cout << "SUMMARY\n";
    std::cout << "========================================\n";
    std::cout << "Passed: " << totalPass << "\n";
    std::cout << "Failed: " << totalFail << "\n\n";

    std::cout << "PASS/FAIL TABLE:\n";
    std::cout << std::setw(5) << "ID" << " | "
              << std::setw(30) << "Name" << " | "
              << "Impulse | Harmonic | Peaks | Stereo | Overall\n";
    std::cout << std::string(90, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::setw(5) << r.engineId << " | "
                  << std::setw(30) << r.engineName << " | "
                  << std::setw(7) << (r.impulseTest ? "PASS" : "FAIL") << " | "
                  << std::setw(8) << (r.harmonicContent ? "PASS" : "FAIL") << " | "
                  << std::setw(5) << (r.peakLevels ? "PASS" : "FAIL") << " | "
                  << std::setw(6) << (r.stereoPreservation ? "PASS" : "FAIL") << " | "
                  << std::setw(7) << (r.overallPass ? "PASS" : "FAIL") << "\n";
    }
    std::cout << "\n";

    return (totalFail > 0) ? 1 : 0;
}

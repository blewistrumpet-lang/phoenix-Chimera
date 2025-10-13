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
#include <chrono>
#include <csignal>

// Distortion-specific test suite for ChimeraPhoenix
namespace DistortionTests {

// FFT implementation for harmonic analysis
class SimpleFFT {
public:
    static std::vector<float> computeMagnitudeSpectrum(const std::vector<float>& signal) {
        int N = signal.size();
        std::vector<std::complex<float>> spectrum(N);

        // Simple DFT (for testing purposes - not optimized)
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

struct HarmonicAnalysis {
    float fundamentalFreq;
    float fundamentalAmplitude;
    float harmonicAmplitudes[10];  // Up to 10th harmonic
    float thd;                      // Total Harmonic Distortion
    float evenHarmonicRatio;        // Even vs odd harmonic energy
    float oddHarmonicRatio;
    std::string character;          // "warm", "harsh", "neutral"

    void print() const {
        std::cout << "\n  HARMONIC ANALYSIS:\n";
        std::cout << "    Fundamental: " << std::fixed << std::setprecision(1)
                  << fundamentalFreq << " Hz @ " << fundamentalAmplitude << "\n";
        std::cout << "    THD: " << std::fixed << std::setprecision(2) << thd * 100.0f << "%\n";
        std::cout << "    Even harmonics: " << std::fixed << std::setprecision(1)
                  << evenHarmonicRatio * 100.0f << "%\n";
        std::cout << "    Odd harmonics: " << std::fixed << std::setprecision(1)
                  << oddHarmonicRatio * 100.0f << "%\n";
        std::cout << "    Character: " << character << "\n";

        std::cout << "    Harmonic levels:\n";
        for (int i = 0; i < 10; ++i) {
            if (harmonicAmplitudes[i] > 0.001f) {
                float dB = 20.0f * std::log10(harmonicAmplitudes[i] / fundamentalAmplitude);
                std::cout << "      H" << (i+2) << ": " << std::fixed << std::setprecision(1)
                          << dB << " dB\n";
            }
        }
    }
};

struct TransferFunction {
    std::vector<float> inputLevels_dB;
    std::vector<float> outputLevels_dB;
    std::string clippingType;      // "soft", "hard", "asymmetric"
    float compressionRatio;
    float softClipThreshold_dB;

    void print() const {
        std::cout << "\n  TRANSFER FUNCTION:\n";
        std::cout << "    Clipping type: " << clippingType << "\n";
        std::cout << "    Compression ratio: " << std::fixed << std::setprecision(2)
                  << compressionRatio << ":1\n";
        std::cout << "    Soft clip threshold: " << std::fixed << std::setprecision(1)
                  << softClipThreshold_dB << " dB\n";

        std::cout << "\n    Input/Output curve:\n";
        for (size_t i = 0; i < inputLevels_dB.size(); ++i) {
            std::cout << "      " << std::fixed << std::setprecision(1)
                      << inputLevels_dB[i] << " dB -> "
                      << outputLevels_dB[i] << " dB\n";
        }
    }
};

struct FrequencyDependentDistortion {
    float thd_100Hz;
    float thd_500Hz;
    float thd_1kHz;
    float thd_4kHz;
    float thd_10kHz;
    float imd;                      // Intermodulation distortion
    bool hasAliasing;
    float aliasingLevel_dB;

    void print() const {
        std::cout << "\n  FREQUENCY-DEPENDENT DISTORTION:\n";
        std::cout << "    THD @ 100Hz:  " << std::fixed << std::setprecision(2)
                  << thd_100Hz * 100.0f << "%\n";
        std::cout << "    THD @ 500Hz:  " << std::fixed << std::setprecision(2)
                  << thd_500Hz * 100.0f << "%\n";
        std::cout << "    THD @ 1kHz:   " << std::fixed << std::setprecision(2)
                  << thd_1kHz * 100.0f << "%\n";
        std::cout << "    THD @ 4kHz:   " << std::fixed << std::setprecision(2)
                  << thd_4kHz * 100.0f << "%\n";
        std::cout << "    THD @ 10kHz:  " << std::fixed << std::setprecision(2)
                  << thd_10kHz * 100.0f << "%\n";
        std::cout << "    IMD:          " << std::fixed << std::setprecision(2)
                  << imd * 100.0f << "%\n";
        std::cout << "    Aliasing:     " << (hasAliasing ? "DETECTED" : "None")
                  << " (" << aliasingLevel_dB << " dB)\n";
    }
};

struct DistortionMetrics {
    HarmonicAnalysis harmonics;
    TransferFunction transfer;
    FrequencyDependentDistortion freqDependent;
    int qualityRating;              // 1-5 stars
    std::string comparison;          // Comparison to classic hardware
    bool passed;
};

// Measure harmonic content at 1kHz
HarmonicAnalysis analyzeHarmonics(EngineBase* engine, float sampleRate,
                                  const std::map<int, float>& params) {
    HarmonicAnalysis result = {};
    result.fundamentalFreq = 1000.0f;

    // Re-apply parameters
    engine->updateParameters(params);

    const int blockSize = 8192;  // Large enough for good frequency resolution
    juce::AudioBuffer<float> buffer(2, blockSize);

    // Generate 1kHz sine wave at -20dB
    float amplitude = 0.1f;  // -20dB
    for (int i = 0; i < blockSize; ++i) {
        float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
        float sample = amplitude * std::sin(phase);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }

    // Process
    engine->process(buffer);

    // Convert to vector for FFT
    std::vector<float> signal(blockSize);
    for (int i = 0; i < blockSize; ++i) {
        signal[i] = buffer.getSample(0, i);
    }

    // Compute FFT
    auto spectrum = SimpleFFT::computeMagnitudeSpectrum(signal);

    // Find fundamental and harmonics
    int fundamentalBin = static_cast<int>(1000.0f * blockSize / sampleRate);
    result.fundamentalAmplitude = spectrum[fundamentalBin];

    float harmonicPower = 0.0f;
    float evenHarmonicPower = 0.0f;
    float oddHarmonicPower = 0.0f;

    for (int h = 0; h < 10; ++h) {
        int harmonicBin = fundamentalBin * (h + 2);
        if (harmonicBin < spectrum.size()) {
            result.harmonicAmplitudes[h] = spectrum[harmonicBin];
            float power = result.harmonicAmplitudes[h] * result.harmonicAmplitudes[h];
            harmonicPower += power;

            if ((h + 2) % 2 == 0) {
                evenHarmonicPower += power;
            } else {
                oddHarmonicPower += power;
            }
        }
    }

    // Calculate THD
    float fundamentalPower = result.fundamentalAmplitude * result.fundamentalAmplitude;
    result.thd = std::sqrt(harmonicPower / fundamentalPower);

    // Calculate even/odd ratio
    float totalHarmonicPower = evenHarmonicPower + oddHarmonicPower;
    if (totalHarmonicPower > 0.0f) {
        result.evenHarmonicRatio = evenHarmonicPower / totalHarmonicPower;
        result.oddHarmonicRatio = oddHarmonicPower / totalHarmonicPower;
    }

    // Classify character
    if (result.evenHarmonicRatio > 0.6f) {
        result.character = "warm (tube-like, even harmonics)";
    } else if (result.oddHarmonicRatio > 0.6f) {
        result.character = "harsh (transistor-like, odd harmonics)";
    } else {
        result.character = "neutral (balanced harmonics)";
    }

    return result;
}

// Measure transfer function (input/output curve)
TransferFunction analyzeTransferFunction(EngineBase* engine, float sampleRate,
                                         const std::map<int, float>& params) {
    TransferFunction result;

    // Test input levels
    std::vector<float> testLevels_dB = {-40.0f, -20.0f, -10.0f, -6.0f, 0.0f, 6.0f};

    engine->updateParameters(params);

    const int blockSize = 1024;

    for (float inputLevel_dB : testLevels_dB) {
        juce::AudioBuffer<float> buffer(2, blockSize);

        float amplitude = std::pow(10.0f, inputLevel_dB / 20.0f);

        // Generate 1kHz sine at test level
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            float sample = amplitude * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        // Process
        engine->process(buffer);

        // Measure output RMS
        float rms = 0.0f;
        for (int i = 0; i < blockSize; ++i) {
            float sample = buffer.getSample(0, i);
            rms += sample * sample;
        }
        rms = std::sqrt(rms / blockSize);

        float outputLevel_dB = 20.0f * std::log10(std::max(1e-10f, rms));

        result.inputLevels_dB.push_back(inputLevel_dB);
        result.outputLevels_dB.push_back(outputLevel_dB);
    }

    // Analyze clipping type
    if (result.outputLevels_dB.size() >= 2) {
        float gain1 = result.outputLevels_dB[1] - result.inputLevels_dB[1];
        float gain2 = result.outputLevels_dB.back() - result.inputLevels_dB.back();
        float gainReduction = gain1 - gain2;

        if (gainReduction > 10.0f) {
            result.clippingType = "hard (abrupt)";
        } else if (gainReduction > 3.0f) {
            result.clippingType = "soft (gradual)";
        } else {
            result.clippingType = "linear (minimal)";
        }

        // Calculate compression ratio
        result.compressionRatio = std::abs((result.inputLevels_dB.back() - result.inputLevels_dB[1]) /
                                           (result.outputLevels_dB.back() - result.outputLevels_dB[1]));

        // Find soft clip threshold (where gain starts reducing)
        result.softClipThreshold_dB = -6.0f;  // Default
        for (size_t i = 1; i < result.inputLevels_dB.size(); ++i) {
            float gain = result.outputLevels_dB[i] - result.inputLevels_dB[i];
            float prevGain = result.outputLevels_dB[i-1] - result.inputLevels_dB[i-1];
            if (std::abs(gain - prevGain) > 1.0f) {
                result.softClipThreshold_dB = result.inputLevels_dB[i];
                break;
            }
        }
    }

    return result;
}

// Measure frequency-dependent distortion
FrequencyDependentDistortion analyzeFrequencyDependent(EngineBase* engine, float sampleRate,
                                                       const std::map<int, float>& params) {
    FrequencyDependentDistortion result = {};

    engine->updateParameters(params);

    const int blockSize = 4096;

    // Test THD at different frequencies
    std::vector<float> testFreqs = {100.0f, 500.0f, 1000.0f, 4000.0f, 10000.0f};
    std::vector<float*> thdResults = {&result.thd_100Hz, &result.thd_500Hz, &result.thd_1kHz,
                                      &result.thd_4kHz, &result.thd_10kHz};

    for (size_t f = 0; f < testFreqs.size(); ++f) {
        float freq = testFreqs[f];
        juce::AudioBuffer<float> buffer(2, blockSize);

        // Generate sine at -10dB
        float amplitude = 0.316f;
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * freq * i / sampleRate;
            float sample = amplitude * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        engine->process(buffer);

        // Quick THD estimate (RMS of harmonics / RMS of fundamental)
        std::vector<float> signal(blockSize);
        for (int i = 0; i < blockSize; ++i) {
            signal[i] = buffer.getSample(0, i);
        }

        auto spectrum = SimpleFFT::computeMagnitudeSpectrum(signal);
        int fundamentalBin = static_cast<int>(freq * blockSize / sampleRate);

        float fundamentalPower = spectrum[fundamentalBin] * spectrum[fundamentalBin];
        float harmonicPower = 0.0f;

        for (int h = 2; h <= 10; ++h) {
            int harmonicBin = fundamentalBin * h;
            if (harmonicBin < spectrum.size()) {
                harmonicPower += spectrum[harmonicBin] * spectrum[harmonicBin];
            }
        }

        *thdResults[f] = std::sqrt(harmonicPower / fundamentalPower);
    }

    // Test IMD (dual-tone test: 1kHz + 1.1kHz)
    juce::AudioBuffer<float> buffer(2, blockSize);
    float amplitude = 0.316f * 0.5f;  // Split between two tones

    for (int i = 0; i < blockSize; ++i) {
        float phase1 = 2.0f * M_PI * 1000.0f * i / sampleRate;
        float phase2 = 2.0f * M_PI * 1100.0f * i / sampleRate;
        float sample = amplitude * (std::sin(phase1) + std::sin(phase2));
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }

    engine->process(buffer);

    std::vector<float> signal(blockSize);
    for (int i = 0; i < blockSize; ++i) {
        signal[i] = buffer.getSample(0, i);
    }

    auto spectrum = SimpleFFT::computeMagnitudeSpectrum(signal);

    // Look for intermodulation products (900Hz, 2100Hz, etc.)
    int bin1k = static_cast<int>(1000.0f * blockSize / sampleRate);
    int bin1k1 = static_cast<int>(1100.0f * blockSize / sampleRate);
    int bin900 = static_cast<int>(900.0f * blockSize / sampleRate);
    int bin2k1 = static_cast<int>(2100.0f * blockSize / sampleRate);

    float fundamentalPower = spectrum[bin1k] * spectrum[bin1k] +
                            spectrum[bin1k1] * spectrum[bin1k1];
    float imdPower = spectrum[bin900] * spectrum[bin900] +
                    spectrum[bin2k1] * spectrum[bin2k1];

    result.imd = std::sqrt(imdPower / fundamentalPower);

    // Check for aliasing (look for energy above Nyquist/2)
    int aliasingStartBin = static_cast<int>(sampleRate * 0.45f * blockSize / sampleRate);
    float aliasingEnergy = 0.0f;
    float totalEnergy = 0.0f;

    for (int i = 0; i < spectrum.size(); ++i) {
        float power = spectrum[i] * spectrum[i];
        totalEnergy += power;
        if (i > aliasingStartBin) {
            aliasingEnergy += power;
        }
    }

    result.hasAliasing = (aliasingEnergy / totalEnergy) > 0.001f;  // 0.1% threshold
    result.aliasingLevel_dB = 10.0f * std::log10(aliasingEnergy / totalEnergy);

    return result;
}

// Test with timeout protection
bool testEngineWithTimeout(int engineId, const std::string& name,
                          DistortionMetrics& metrics, int timeoutSeconds = 5) {
    std::cout << "\n[DEBUG] Testing engine " << engineId << ": " << name << "...\n" << std::flush;

    auto startTime = std::chrono::steady_clock::now();

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            std::cout << "ERROR: Failed to create engine " << engineId << "\n";
            return false;
        }

        const float sampleRate = 48000.0f;
        const int blockSize = 512;

        engine->prepareToPlay(sampleRate, blockSize);

        // Set typical distortion parameters
        std::map<int, float> params;
        int numParams = engine->getNumParameters();

        // Set drive/gain parameters to moderate level
        if (numParams > 0) params[0] = 0.5f;   // Usually drive/gain
        if (numParams > 1) params[1] = 0.5f;   // Usually tone/filter
        if (numParams > 2) params[2] = 0.7f;   // Usually output level
        if (numParams > 3) params[3] = 1.0f;   // Usually mix (full wet)

        engine->updateParameters(params);

        // Check for timeout
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > timeoutSeconds) {
            std::cout << "ERROR: Timeout during initialization\n";
            return false;
        }

        // Analyze harmonics
        std::cout << "[DEBUG] Analyzing harmonics...\n" << std::flush;
        metrics.harmonics = analyzeHarmonics(engine.get(), sampleRate, params);

        elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > timeoutSeconds) {
            std::cout << "ERROR: Timeout during harmonic analysis\n";
            return false;
        }

        // Analyze transfer function
        std::cout << "[DEBUG] Analyzing transfer function...\n" << std::flush;
        metrics.transfer = analyzeTransferFunction(engine.get(), sampleRate, params);

        elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > timeoutSeconds) {
            std::cout << "ERROR: Timeout during transfer function analysis\n";
            return false;
        }

        // Analyze frequency-dependent distortion
        std::cout << "[DEBUG] Analyzing frequency-dependent distortion...\n" << std::flush;
        metrics.freqDependent = analyzeFrequencyDependent(engine.get(), sampleRate, params);

        elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > timeoutSeconds) {
            std::cout << "ERROR: Timeout during frequency analysis\n";
            return false;
        }

        // Rate quality
        int rating = 5;  // Start with perfect score

        // Deduct points for issues
        if (metrics.harmonics.thd > 0.5f) rating--;  // Excessive distortion
        if (metrics.freqDependent.hasAliasing) rating--;  // Aliasing
        if (metrics.freqDependent.imd > 0.1f) rating--;  // High IMD
        if (metrics.transfer.clippingType == "hard (abrupt)") rating--;  // Harsh clipping

        metrics.qualityRating = std::max(1, rating);
        metrics.passed = (rating >= 3);

        return true;

    } catch (const std::exception& e) {
        std::cout << "ERROR: Exception during testing: " << e.what() << "\n";
        return false;
    }
}

void saveCSV(int engineId, const std::string& name, const DistortionMetrics& metrics) {
    // Save harmonics
    {
        std::string filename = "distortion_engine_" + std::to_string(engineId) + "_harmonics.csv";
        std::ofstream file(filename);
        file << "Harmonic,Amplitude,Level_dB\n";
        file << "Fundamental," << metrics.harmonics.fundamentalAmplitude << ",0.0\n";
        for (int i = 0; i < 10; ++i) {
            if (metrics.harmonics.harmonicAmplitudes[i] > 0.001f) {
                float dB = 20.0f * std::log10(metrics.harmonics.harmonicAmplitudes[i] /
                                             metrics.harmonics.fundamentalAmplitude);
                file << "H" << (i+2) << "," << metrics.harmonics.harmonicAmplitudes[i]
                     << "," << dB << "\n";
            }
        }
        file.close();
    }

    // Save transfer function
    {
        std::string filename = "distortion_engine_" + std::to_string(engineId) + "_transfer.csv";
        std::ofstream file(filename);
        file << "Input_dB,Output_dB\n";
        for (size_t i = 0; i < metrics.transfer.inputLevels_dB.size(); ++i) {
            file << metrics.transfer.inputLevels_dB[i] << ","
                 << metrics.transfer.outputLevels_dB[i] << "\n";
        }
        file.close();
    }

    // Save spectrum info
    {
        std::string filename = "distortion_engine_" + std::to_string(engineId) + "_spectrum.csv";
        std::ofstream file(filename);
        file << "Frequency_Hz,THD_percent\n";
        file << "100," << metrics.freqDependent.thd_100Hz * 100.0f << "\n";
        file << "500," << metrics.freqDependent.thd_500Hz * 100.0f << "\n";
        file << "1000," << metrics.freqDependent.thd_1kHz * 100.0f << "\n";
        file << "4000," << metrics.freqDependent.thd_4kHz * 100.0f << "\n";
        file << "10000," << metrics.freqDependent.thd_10kHz * 100.0f << "\n";
        file.close();
    }
}

void printMetrics(int engineId, const std::string& name, const DistortionMetrics& metrics) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": " << std::setw(45) << std::left << name << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    metrics.harmonics.print();
    metrics.transfer.print();
    metrics.freqDependent.print();

    std::cout << "\n  QUALITY RATING: ";
    for (int i = 0; i < metrics.qualityRating; ++i) {
        std::cout << "★";
    }
    for (int i = metrics.qualityRating; i < 5; ++i) {
        std::cout << "☆";
    }
    std::cout << " (" << metrics.qualityRating << "/5)\n";

    if (!metrics.comparison.empty()) {
        std::cout << "  COMPARISON: " << metrics.comparison << "\n";
    }

    std::cout << "\n  OVERALL: " << (metrics.passed ? "✓ PASSED" : "✗ FAILED") << "\n\n";
}

} // namespace DistortionTests

int main(int argc, char* argv[]) {
    std::vector<std::pair<int, std::string>> distortionEngines = {
        {15, "Vintage Tube Preamp Studio"},
        {16, "Wave Folder"},
        {17, "Harmonic Exciter Platinum"},
        {18, "Bit Crusher"},
        {19, "Multiband Saturator"},
        {20, "Muff Fuzz"},
        {21, "Rodent Distortion"},
        {22, "K-Style Overdrive"}
    };

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ChimeraPhoenix Distortion & Saturation Test Suite        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    std::vector<std::pair<int, DistortionTests::DistortionMetrics>> results;
    int passed = 0, failed = 0;

    for (const auto& [id, name] : distortionEngines) {
        DistortionTests::DistortionMetrics metrics;

        bool success = DistortionTests::testEngineWithTimeout(id, name, metrics, 10);

        if (success) {
            DistortionTests::printMetrics(id, name, metrics);
            DistortionTests::saveCSV(id, name, metrics);
            results.push_back({id, metrics});

            if (metrics.passed) {
                passed++;
            } else {
                failed++;
            }
        } else {
            std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
            std::cout << "║  Engine " << std::setw(2) << id << ": " << std::setw(45) << std::left << name << "║\n";
            std::cout << "╚════════════════════════════════════════════════════════════╝\n";
            std::cout << "\n  ✗ FAILED - Engine hung or crashed\n\n";
            failed++;
        }
    }

    // Summary
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST SUMMARY                                              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n  Total engines tested: " << distortionEngines.size() << "\n";
    std::cout << "  Passed: " << passed << "\n";
    std::cout << "  Failed: " << failed << "\n\n";

    return (failed > 0) ? 1 : 0;
}

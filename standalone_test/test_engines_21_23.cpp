#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>

// Test suite for engines 21-23
// NOTE: Based on EngineFactory.cpp:
//   Engine 21: RodentDistortion
//   Engine 22: KStyleOverdrive
//   Engine 23: StereoChorus (Modulation, not distortion)
//
// User requested: TapeSaturation, VinylDistortion, HarmonicExciter_Platinum
// HarmonicExciter_Platinum is actually Engine 17, not in the 21-23 range

struct ImpulseTestResult {
    bool passes_through_impulse;
    float peak_output;
    float rms_output;
    int non_zero_samples;
    bool has_valid_output;
    std::string error_msg;
};

struct DistortionCharacteristics {
    float thd;                  // Total Harmonic Distortion
    float second_harmonic_db;   // 2nd harmonic level
    float third_harmonic_db;    // 3rd harmonic level
    float compression_ratio;    // Dynamic range compression
    std::string distortion_type; // "soft", "hard", "asymmetric", "none"
};

struct OutputLevels {
    float peak_level_db;
    float rms_level_db;
    bool exceeds_0db;
    bool within_range;
};

// Simple FFT for harmonic analysis
std::vector<float> computeSpectrum(const std::vector<float>& signal, int sampleRate) {
    int N = signal.size();
    std::vector<float> magnitude(N/2, 0.0f);

    // Simple DFT for key frequencies
    for (int k = 0; k < N/2; ++k) {
        float real = 0.0f, imag = 0.0f;
        for (int n = 0; n < N; ++n) {
            float angle = -2.0f * M_PI * k * n / N;
            real += signal[n] * std::cos(angle);
            imag += signal[n] * std::sin(angle);
        }
        magnitude[k] = std::sqrt(real*real + imag*imag) / N;
    }

    return magnitude;
}

ImpulseTestResult testImpulseResponse(EngineBase* engine, float sampleRate, const std::map<int, float>& params) {
    ImpulseTestResult result = {};

    try {
        engine->updateParameters(params);

        const int blockSize = 2048;
        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();

        // Create impulse at sample 100
        buffer.setSample(0, 100, 1.0f);
        buffer.setSample(1, 100, 1.0f);

        engine->process(buffer);

        // Analyze output
        float peak = 0.0f;
        float sumSquares = 0.0f;
        int nonZero = 0;

        for (int i = 0; i < blockSize; ++i) {
            float sample = std::abs(buffer.getSample(0, i));
            if (sample > 0.0001f) {
                nonZero++;
            }
            peak = std::max(peak, sample);
            sumSquares += sample * sample;
        }

        result.peak_output = peak;
        result.rms_output = std::sqrt(sumSquares / blockSize);
        result.non_zero_samples = nonZero;
        result.passes_through_impulse = (nonZero > 0 && peak > 0.01f);
        result.has_valid_output = (peak < 10.0f && !std::isnan(peak) && !std::isinf(peak));

        if (!result.passes_through_impulse) {
            result.error_msg = "No output detected from impulse";
        } else if (!result.has_valid_output) {
            result.error_msg = "Invalid output (NaN/Inf or excessive level)";
        }

        return result;

    } catch (const std::exception& e) {
        result.error_msg = std::string("Exception: ") + e.what();
        return result;
    }
}

DistortionCharacteristics analyzeDistortionCharacteristics(EngineBase* engine, float sampleRate,
                                                          const std::map<int, float>& params) {
    DistortionCharacteristics result = {};

    try {
        engine->updateParameters(params);

        const int blockSize = 8192;
        juce::AudioBuffer<float> buffer(2, blockSize);

        // Generate 1kHz sine at -10dB
        float amplitude = 0.316f;  // -10dB
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            float sample = amplitude * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        engine->process(buffer);

        // Convert to vector
        std::vector<float> signal(blockSize);
        for (int i = 0; i < blockSize; ++i) {
            signal[i] = buffer.getSample(0, i);
        }

        // Compute spectrum
        auto spectrum = computeSpectrum(signal, sampleRate);

        // Find fundamental and harmonics (1kHz, 2kHz, 3kHz)
        int binSize = sampleRate / blockSize;
        int bin1k = 1000 / binSize;
        int bin2k = 2000 / binSize;
        int bin3k = 3000 / binSize;

        float fundamental = spectrum[bin1k];
        float second_harmonic = (bin2k < spectrum.size()) ? spectrum[bin2k] : 0.0f;
        float third_harmonic = (bin3k < spectrum.size()) ? spectrum[bin3k] : 0.0f;

        // Calculate THD
        float harmonic_power = second_harmonic*second_harmonic + third_harmonic*third_harmonic;
        float fundamental_power = fundamental*fundamental;
        result.thd = (fundamental_power > 0) ? std::sqrt(harmonic_power / fundamental_power) : 0.0f;

        // Calculate dB levels
        if (fundamental > 1e-6f) {
            result.second_harmonic_db = 20.0f * std::log10(second_harmonic / fundamental);
            result.third_harmonic_db = 20.0f * std::log10(third_harmonic / fundamental);
        } else {
            result.second_harmonic_db = -120.0f;
            result.third_harmonic_db = -120.0f;
        }

        // Test compression ratio with two different levels
        float lowLevel = 0.1f, highLevel = 0.5f;
        float lowOut = 0.0f, highOut = 0.0f;

        // Low level test
        buffer.clear();
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            buffer.setSample(0, i, lowLevel * std::sin(phase));
            buffer.setSample(1, i, lowLevel * std::sin(phase));
        }
        engine->process(buffer);
        for (int i = 0; i < blockSize; ++i) {
            lowOut += std::abs(buffer.getSample(0, i));
        }
        lowOut /= blockSize;

        // High level test
        buffer.clear();
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            buffer.setSample(0, i, highLevel * std::sin(phase));
            buffer.setSample(1, i, highLevel * std::sin(phase));
        }
        engine->process(buffer);
        for (int i = 0; i < blockSize; ++i) {
            highOut += std::abs(buffer.getSample(0, i));
        }
        highOut /= blockSize;

        float inputRatio = highLevel / lowLevel;
        float outputRatio = (lowOut > 0) ? (highOut / lowOut) : 1.0f;
        result.compression_ratio = inputRatio / outputRatio;

        // Classify distortion type
        if (result.thd < 0.01f) {
            result.distortion_type = "none/clean";
        } else if (result.second_harmonic_db > result.third_harmonic_db) {
            result.distortion_type = "soft (even harmonics dominant)";
        } else if (result.third_harmonic_db > result.second_harmonic_db) {
            result.distortion_type = "hard (odd harmonics dominant)";
        } else {
            result.distortion_type = "balanced";
        }

        return result;

    } catch (const std::exception& e) {
        result.distortion_type = std::string("ERROR: ") + e.what();
        return result;
    }
}

OutputLevels analyzeOutputLevels(EngineBase* engine, float sampleRate, const std::map<int, float>& params) {
    OutputLevels result = {};

    try {
        engine->updateParameters(params);

        const int blockSize = 1024;
        juce::AudioBuffer<float> buffer(2, blockSize);

        // Test with 0dBFS sine wave
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            float sample = 0.9f * std::sin(phase);  // 0.9 to avoid clipping before processing
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        engine->process(buffer);

        float peak = 0.0f;
        float sumSquares = 0.0f;

        for (int i = 0; i < blockSize; ++i) {
            float sample = std::abs(buffer.getSample(0, i));
            peak = std::max(peak, sample);
            sumSquares += sample * sample;
        }

        float rms = std::sqrt(sumSquares / blockSize);

        result.peak_level_db = 20.0f * std::log10(std::max(1e-10f, peak));
        result.rms_level_db = 20.0f * std::log10(std::max(1e-10f, rms));
        result.exceeds_0db = (peak > 1.0f);
        result.within_range = (peak >= 0.01f && peak <= 2.0f);  // Allow some headroom

        return result;

    } catch (const std::exception& e) {
        result.within_range = false;
        return result;
    }
}

void printResult(int engineId, const std::string& name,
                const ImpulseTestResult& impulse,
                const DistortionCharacteristics& distortion,
                const OutputLevels& levels) {

    std::cout << "\n========================================\n";
    std::cout << "ENGINE " << engineId << ": " << name << "\n";
    std::cout << "========================================\n";

    // Impulse Test
    std::cout << "\n[IMPULSE TEST]\n";
    std::cout << "  Passes through: " << (impulse.passes_through_impulse ? "YES" : "NO") << "\n";
    std::cout << "  Peak output: " << std::fixed << std::setprecision(4) << impulse.peak_output << "\n";
    std::cout << "  RMS output: " << impulse.rms_output << "\n";
    std::cout << "  Non-zero samples: " << impulse.non_zero_samples << "\n";
    std::cout << "  Valid output: " << (impulse.has_valid_output ? "YES" : "NO") << "\n";
    if (!impulse.error_msg.empty()) {
        std::cout << "  ERROR: " << impulse.error_msg << "\n";
    }

    // Distortion Characteristics
    std::cout << "\n[DISTORTION CHARACTERISTICS]\n";
    std::cout << "  THD: " << std::fixed << std::setprecision(2) << (distortion.thd * 100.0f) << "%\n";
    std::cout << "  2nd harmonic: " << std::fixed << std::setprecision(1)
              << distortion.second_harmonic_db << " dB\n";
    std::cout << "  3rd harmonic: " << distortion.third_harmonic_db << " dB\n";
    std::cout << "  Compression ratio: " << std::fixed << std::setprecision(2)
              << distortion.compression_ratio << ":1\n";
    std::cout << "  Type: " << distortion.distortion_type << "\n";

    // Output Levels
    std::cout << "\n[OUTPUT LEVELS]\n";
    std::cout << "  Peak level: " << std::fixed << std::setprecision(1) << levels.peak_level_db << " dB\n";
    std::cout << "  RMS level: " << levels.rms_level_db << " dB\n";
    std::cout << "  Exceeds 0dB: " << (levels.exceeds_0db ? "YES (WARNING)" : "NO") << "\n";
    std::cout << "  Within range: " << (levels.within_range ? "YES" : "NO") << "\n";

    // Overall Pass/Fail
    bool pass = impulse.passes_through_impulse &&
                impulse.has_valid_output &&
                levels.within_range &&
                !levels.exceeds_0db;

    std::cout << "\n[RESULT]: " << (pass ? "PASS" : "FAIL") << "\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ChimeraPhoenix Engine Test: 21-23                      ║\n";
    std::cout << "║  Testing: RodentDistortion, KStyleOverdrive, Chorus     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    std::cout << "\nNOTE: User requested TapeSaturation, VinylDistortion, HarmonicExciter_Platinum\n";
    std::cout << "      but these are not engines 21-23. HarmonicExciter_Platinum is Engine 17.\n";
    std::cout << "      Testing actual engines 21-23 from codebase...\n";

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    std::vector<std::pair<int, std::string>> engines = {
        {21, "RodentDistortion"},
        {22, "KStyleOverdrive"},
        {23, "StereoChorus"}
    };

    int totalTests = 0;
    int passedTests = 0;

    for (const auto& [id, name] : engines) {
        totalTests++;

        try {
            auto engine = EngineFactory::createEngine(id);
            if (!engine) {
                std::cout << "\n[ERROR] Failed to create engine " << id << ": " << name << "\n";
                continue;
            }

            engine->prepareToPlay(sampleRate, blockSize);

            // Set moderate parameters
            std::map<int, float> params;
            int numParams = engine->getNumParameters();
            if (numParams > 0) params[0] = 0.5f;  // Drive/Gain
            if (numParams > 1) params[1] = 0.5f;  // Tone
            if (numParams > 2) params[2] = 0.7f;  // Level
            if (numParams > 3) params[3] = 1.0f;  // Mix (full wet)

            // Run tests
            auto impulse = testImpulseResponse(engine.get(), sampleRate, params);
            auto distortion = analyzeDistortionCharacteristics(engine.get(), sampleRate, params);
            auto levels = analyzeOutputLevels(engine.get(), sampleRate, params);

            // Print results
            printResult(id, name, impulse, distortion, levels);

            // Check pass/fail
            bool pass = impulse.passes_through_impulse &&
                       impulse.has_valid_output &&
                       levels.within_range &&
                       !levels.exceeds_0db;

            if (pass) {
                passedTests++;
            }

        } catch (const std::exception& e) {
            std::cout << "\n[EXCEPTION] Engine " << id << " (" << name << "): " << e.what() << "\n";
        }
    }

    // Summary
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST SUMMARY                                            ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\nTotal tests: " << totalTests << "\n";
    std::cout << "Passed: " << passedTests << "\n";
    std::cout << "Failed: " << (totalTests - passedTests) << "\n";
    std::cout << "\nSuccess rate: " << std::fixed << std::setprecision(1)
              << (100.0 * passedTests / totalTests) << "%\n\n";

    return (passedTests == totalTests) ? 0 : 1;
}

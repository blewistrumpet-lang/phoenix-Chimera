// Comprehensive test for Engine 6 (DynamicEQ)
// Tests: THD <1% across all modes, compression accuracy, parameter response, audio quality

#include "../src/AudioEngine.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;

double calculateTHD(const std::vector<float>& signal, double fundamental_freq, size_t skip) {
    int N = signal.size() - skip;

    // Calculate RMS of fundamental
    double fundamental_rms = 0.0;
    for (size_t i = skip; i < signal.size(); ++i) {
        double t = (i - skip) / SAMPLE_RATE;
        double expected = std::sin(2.0 * M_PI * fundamental_freq * t);
        fundamental_rms += signal[i] * expected;
    }
    fundamental_rms = std::abs(fundamental_rms) / N;

    // Calculate total RMS
    double total_rms = 0.0;
    for (size_t i = skip; i < signal.size(); ++i) {
        total_rms += signal[i] * signal[i];
    }
    total_rms = std::sqrt(total_rms / N);

    // THD = sqrt(total^2 - fundamental^2) / fundamental
    double harmonic_rms = std::sqrt(std::max(0.0, total_rms * total_rms - fundamental_rms * fundamental_rms));
    double thd = (fundamental_rms > 0.0001) ? (harmonic_rms / fundamental_rms) * 100.0 : 0.0;

    return thd;
}

bool testDynamicEQ() {
    std::cout << "\n=== Engine 6 (DynamicEQ) Comprehensive Test ===" << std::endl;

    AudioEngine engine;
    engine.initialize(SAMPLE_RATE, BUFFER_SIZE);
    engine.setCurrentEngine(6); // DynamicEQ

    bool all_passed = true;

    // Test 1: THD <1% across all modes
    std::cout << "\n[Test 1] THD Analysis (<1% target)" << std::endl;
    {
        std::vector<std::string> mode_names = {"Mode 1", "Mode 2", "Mode 3", "Mode 4"};

        for (int mode = 0; mode < 4; ++mode) {
            // Set mode via parameter (if applicable)
            engine.setParameter(0, mode / 3.0f); // Normalize to 0-1
            engine.setParameter(1, 0.5f); // Moderate settings
            engine.setParameter(2, 0.5f);

            std::vector<float> inputL(BUFFER_SIZE * 30);
            std::vector<float> inputR(BUFFER_SIZE * 30);
            std::vector<float> outputL(BUFFER_SIZE * 30);
            std::vector<float> outputR(BUFFER_SIZE * 30);

            // Generate 1kHz sine
            for (size_t i = 0; i < inputL.size(); ++i) {
                double t = i / SAMPLE_RATE;
                inputL[i] = 0.5f * std::sin(2.0 * M_PI * 1000.0 * t);
                inputR[i] = inputL[i];
            }

            for (int chunk = 0; chunk < 30; ++chunk) {
                engine.processBlock(
                    inputL.data() + chunk * BUFFER_SIZE,
                    inputR.data() + chunk * BUFFER_SIZE,
                    outputL.data() + chunk * BUFFER_SIZE,
                    outputR.data() + chunk * BUFFER_SIZE,
                    BUFFER_SIZE
                );
            }

            double thd = calculateTHD(outputL, 1000.0, BUFFER_SIZE * 5);
            bool thd_pass = (thd < 1.0);

            std::cout << "  " << mode_names[mode] << ": THD = " << thd << "% - "
                     << (thd_pass ? "PASS" : "FAIL") << std::endl;
            all_passed &= thd_pass;
        }
    }

    // Test 2: Compression accuracy
    std::cout << "\n[Test 2] Compression Accuracy" << std::endl;
    {
        engine.setParameter(0, 0.5f);
        engine.setParameter(1, 0.7f); // Higher threshold/ratio
        engine.setParameter(2, 0.5f);

        std::vector<float> inputL(BUFFER_SIZE * 40);
        std::vector<float> inputR(BUFFER_SIZE * 40);
        std::vector<float> outputL(BUFFER_SIZE * 40);
        std::vector<float> outputR(BUFFER_SIZE * 40);

        // Generate signal with varying amplitude
        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            double envelope = 0.2f + 0.6f * std::sin(2.0 * M_PI * 2.0 * t); // Slow amplitude variation
            inputL[i] = envelope * std::sin(2.0 * M_PI * 1000.0 * t);
            inputR[i] = inputL[i];
        }

        for (int chunk = 0; chunk < 40; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );
        }

        // Measure dynamic range reduction
        float input_peak = 0.0f;
        float output_peak = 0.0f;
        size_t skip = BUFFER_SIZE * 10;

        for (size_t i = skip; i < inputL.size(); ++i) {
            input_peak = std::max(input_peak, std::abs(inputL[i]));
            output_peak = std::max(output_peak, std::abs(outputL[i]));
        }

        double compression_ratio = input_peak / (output_peak + 0.0001);
        std::cout << "  Input Peak: " << input_peak << std::endl;
        std::cout << "  Output Peak: " << output_peak << std::endl;
        std::cout << "  Ratio: " << compression_ratio << ":1" << std::endl;

        bool compression_pass = (output_peak > 0.01f && output_peak < input_peak * 1.2);
        std::cout << "  Status: " << (compression_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= compression_pass;
    }

    // Test 3: Parameter response
    std::cout << "\n[Test 3] Parameter Response Test" << std::endl;
    {
        std::vector<float> param_values = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

        for (float param_val : param_values) {
            engine.setParameter(0, param_val);
            engine.setParameter(1, 0.5f);
            engine.setParameter(2, 0.5f);

            std::vector<float> inputL(BUFFER_SIZE * 20);
            std::vector<float> inputR(BUFFER_SIZE * 20);
            std::vector<float> outputL(BUFFER_SIZE * 20);
            std::vector<float> outputR(BUFFER_SIZE * 20);

            for (size_t i = 0; i < inputL.size(); ++i) {
                double t = i / SAMPLE_RATE;
                inputL[i] = 0.5f * std::sin(2.0 * M_PI * 1000.0 * t);
                inputR[i] = inputL[i];
            }

            for (int chunk = 0; chunk < 20; ++chunk) {
                engine.processBlock(
                    inputL.data() + chunk * BUFFER_SIZE,
                    inputR.data() + chunk * BUFFER_SIZE,
                    outputL.data() + chunk * BUFFER_SIZE,
                    outputR.data() + chunk * BUFFER_SIZE,
                    BUFFER_SIZE
                );
            }

            double rms = 0.0;
            size_t skip = BUFFER_SIZE * 5;
            for (size_t i = skip; i < outputL.size(); ++i) {
                rms += outputL[i] * outputL[i];
            }
            rms = std::sqrt(rms / (outputL.size() - skip));

            bool param_ok = (rms > 0.01);
            std::cout << "  Param=" << param_val << ": RMS=" << rms
                     << " - " << (param_ok ? "PASS" : "FAIL") << std::endl;
            all_passed &= param_ok;
        }
    }

    // Test 4: Audio quality across frequency range
    std::cout << "\n[Test 4] Audio Quality - Frequency Range" << std::endl;
    {
        engine.setParameter(0, 0.5f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.5f);

        std::vector<double> test_freqs = {100.0, 440.0, 1000.0, 4000.0, 8000.0};

        for (double freq : test_freqs) {
            std::vector<float> inputL(BUFFER_SIZE * 20);
            std::vector<float> inputR(BUFFER_SIZE * 20);
            std::vector<float> outputL(BUFFER_SIZE * 20);
            std::vector<float> outputR(BUFFER_SIZE * 20);

            for (size_t i = 0; i < inputL.size(); ++i) {
                double t = i / SAMPLE_RATE;
                inputL[i] = 0.5f * std::sin(2.0 * M_PI * freq * t);
                inputR[i] = inputL[i];
            }

            for (int chunk = 0; chunk < 20; ++chunk) {
                engine.processBlock(
                    inputL.data() + chunk * BUFFER_SIZE,
                    inputR.data() + chunk * BUFFER_SIZE,
                    outputL.data() + chunk * BUFFER_SIZE,
                    outputR.data() + chunk * BUFFER_SIZE,
                    BUFFER_SIZE
                );
            }

            double rms = 0.0;
            int nan_count = 0;
            size_t skip = BUFFER_SIZE * 5;

            for (size_t i = skip; i < outputL.size(); ++i) {
                rms += outputL[i] * outputL[i];
                if (std::isnan(outputL[i])) nan_count++;
            }
            rms = std::sqrt(rms / (outputL.size() - skip));

            bool freq_ok = (rms > 0.01 && nan_count == 0);
            std::cout << "  " << freq << "Hz: RMS=" << rms
                     << " - " << (freq_ok ? "PASS" : "FAIL") << std::endl;
            all_passed &= freq_ok;
        }
    }

    // Test 5: THD with different input levels
    std::cout << "\n[Test 5] THD at Different Input Levels" << std::endl;
    {
        engine.setParameter(0, 0.5f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.5f);

        std::vector<float> input_levels = {0.1f, 0.3f, 0.5f, 0.7f};

        for (float level : input_levels) {
            std::vector<float> inputL(BUFFER_SIZE * 25);
            std::vector<float> inputR(BUFFER_SIZE * 25);
            std::vector<float> outputL(BUFFER_SIZE * 25);
            std::vector<float> outputR(BUFFER_SIZE * 25);

            for (size_t i = 0; i < inputL.size(); ++i) {
                double t = i / SAMPLE_RATE;
                inputL[i] = level * std::sin(2.0 * M_PI * 1000.0 * t);
                inputR[i] = inputL[i];
            }

            for (int chunk = 0; chunk < 25; ++chunk) {
                engine.processBlock(
                    inputL.data() + chunk * BUFFER_SIZE,
                    inputR.data() + chunk * BUFFER_SIZE,
                    outputL.data() + chunk * BUFFER_SIZE,
                    outputR.data() + chunk * BUFFER_SIZE,
                    BUFFER_SIZE
                );
            }

            double thd = calculateTHD(outputL, 1000.0, BUFFER_SIZE * 5);
            bool thd_pass = (thd < 1.0);

            std::cout << "  Level " << level << ": THD=" << thd << "% - "
                     << (thd_pass ? "PASS" : "FAIL") << std::endl;
            all_passed &= thd_pass;
        }
    }

    // Test 6: Stability test
    std::cout << "\n[Test 6] Stability Test" << std::endl;
    {
        engine.setParameter(0, 0.5f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.5f);

        std::vector<float> inputL(BUFFER_SIZE);
        std::vector<float> inputR(BUFFER_SIZE);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        int nan_count = 0;
        int inf_count = 0;

        for (int i = 0; i < 1000; ++i) {
            for (int j = 0; j < BUFFER_SIZE; ++j) {
                double t = (i * BUFFER_SIZE + j) / SAMPLE_RATE;
                inputL[j] = 0.5f * std::sin(2.0 * M_PI * 1000.0 * t);
                inputR[j] = inputL[j];
            }

            engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);

            for (int j = 0; j < BUFFER_SIZE; ++j) {
                if (std::isnan(outputL[j]) || std::isnan(outputR[j])) nan_count++;
                if (std::isinf(outputL[j]) || std::isinf(outputR[j])) inf_count++;
            }
        }

        std::cout << "  Processed 1000 blocks" << std::endl;
        std::cout << "  NaN count: " << nan_count << std::endl;
        std::cout << "  Inf count: " << inf_count << std::endl;

        bool stability_pass = (nan_count == 0 && inf_count == 0);
        std::cout << "  Status: " << (stability_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= stability_pass;
    }

    return all_passed;
}

int main() {
    std::cout << "Engine 6 (DynamicEQ) - Comprehensive Verification Test" << std::endl;
    std::cout << "=====================================================" << std::endl;

    bool success = testDynamicEQ();

    std::cout << "\n=====================================================" << std::endl;
    std::cout << "Engine 6 Overall Result: " << (success ? "PASS" : "FAIL") << std::endl;
    std::cout << "=====================================================" << std::endl;

    return success ? 0 : 1;
}

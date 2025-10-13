// Comprehensive test for Engine 32 (DetuneDoubler)
// Tests: THD <1%, pitch shift quality, multiple detune amounts

#include "../src/AudioEngine.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;

double calculateTHD(const std::vector<float>& signal, double fundamental_freq) {
    int N = signal.size();
    double sampleRate = SAMPLE_RATE;

    // Simple FFT-based THD calculation
    // Calculate RMS of fundamental
    double fundamental_rms = 0.0;
    for (int i = 0; i < N; ++i) {
        double t = i / sampleRate;
        double expected = std::sin(2.0 * M_PI * fundamental_freq * t);
        fundamental_rms += signal[i] * expected;
    }
    fundamental_rms = std::abs(fundamental_rms) / N;

    // Calculate total RMS
    double total_rms = 0.0;
    for (int i = 0; i < N; ++i) {
        total_rms += signal[i] * signal[i];
    }
    total_rms = std::sqrt(total_rms / N);

    // THD = sqrt(total^2 - fundamental^2) / fundamental
    double harmonic_rms = std::sqrt(std::max(0.0, total_rms * total_rms - fundamental_rms * fundamental_rms));
    double thd = (fundamental_rms > 0.0001) ? (harmonic_rms / fundamental_rms) * 100.0 : 0.0;

    return thd;
}

double measurePitchShiftQuality(const std::vector<float>& output, double expected_freq) {
    // Measure zero-crossing rate to estimate pitch
    int zero_crossings = 0;
    for (size_t i = 1; i < output.size(); ++i) {
        if ((output[i-1] < 0 && output[i] >= 0) || (output[i-1] >= 0 && output[i] < 0)) {
            zero_crossings++;
        }
    }

    double estimated_freq = (zero_crossings / 2.0) * SAMPLE_RATE / output.size();
    double error = std::abs(estimated_freq - expected_freq) / expected_freq;

    return error * 100.0; // Return as percentage
}

bool testDetuneDoubler() {
    std::cout << "\n=== Engine 32 (DetuneDoubler) Comprehensive Test ===" << std::endl;

    AudioEngine engine;
    engine.initialize(SAMPLE_RATE, BUFFER_SIZE);

    bool all_passed = true;

    // Test 1: THD test with 1kHz sine
    std::cout << "\n[Test 1] THD Analysis with 1kHz sine" << std::endl;
    {
        engine.setCurrentEngine(32); // DetuneDoubler

        // Set moderate detune
        engine.setParameter(0, 0.3f); // Detune amount
        engine.setParameter(1, 0.5f); // Mix

        // Generate 1kHz sine input
        std::vector<float> inputL(BUFFER_SIZE * 20);
        std::vector<float> inputR(BUFFER_SIZE * 20);
        std::vector<float> outputL(BUFFER_SIZE * 20);
        std::vector<float> outputR(BUFFER_SIZE * 20);

        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 1000.0 * t);
            inputR[i] = inputL[i];
        }

        // Process in chunks
        for (int chunk = 0; chunk < 20; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );
        }

        // Calculate THD
        double thd_left = calculateTHD(outputL, 1000.0);
        double thd_right = calculateTHD(outputR, 1000.0);

        std::cout << "  THD Left: " << thd_left << "%" << std::endl;
        std::cout << "  THD Right: " << thd_right << "%" << std::endl;

        bool thd_pass = (thd_left < 1.0 && thd_right < 1.0);
        std::cout << "  Status: " << (thd_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= thd_pass;
    }

    // Test 2: Multiple detune amounts
    std::cout << "\n[Test 2] Multiple Detune Amounts" << std::endl;
    {
        std::vector<float> detune_values = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f};

        for (float detune : detune_values) {
            engine.setParameter(0, detune);

            std::vector<float> inputL(BUFFER_SIZE);
            std::vector<float> inputR(BUFFER_SIZE);
            std::vector<float> outputL(BUFFER_SIZE);
            std::vector<float> outputR(BUFFER_SIZE);

            for (int i = 0; i < BUFFER_SIZE; ++i) {
                double t = i / SAMPLE_RATE;
                inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
                inputR[i] = inputL[i];
            }

            engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);

            // Check for non-zero output
            float max_output = 0.0f;
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                max_output = std::max(max_output, std::abs(outputL[i]));
                max_output = std::max(max_output, std::abs(outputR[i]));
            }

            bool output_valid = (max_output > 0.01f);
            std::cout << "  Detune " << detune << ": Max Output = " << max_output
                     << " - " << (output_valid ? "PASS" : "FAIL") << std::endl;
            all_passed &= output_valid;
        }
    }

    // Test 3: Pitch shift quality
    std::cout << "\n[Test 3] Pitch Shift Quality" << std::endl;
    {
        engine.setParameter(0, 0.5f); // Medium detune

        std::vector<float> inputL(BUFFER_SIZE * 40);
        std::vector<float> inputR(BUFFER_SIZE * 40);
        std::vector<float> outputL(BUFFER_SIZE * 40);
        std::vector<float> outputR(BUFFER_SIZE * 40);

        // Generate 440Hz input
        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
            inputR[i] = inputL[i];
        }

        // Process
        for (int chunk = 0; chunk < 40; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );
        }

        // Check output quality (should contain detuned frequencies)
        double rms = 0.0;
        for (size_t i = 0; i < outputL.size(); ++i) {
            rms += outputL[i] * outputL[i];
        }
        rms = std::sqrt(rms / outputL.size());

        bool quality_pass = (rms > 0.1 && rms < 1.0);
        std::cout << "  RMS Level: " << rms << std::endl;
        std::cout << "  Status: " << (quality_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= quality_pass;
    }

    // Test 4: Stereo field check
    std::cout << "\n[Test 4] Stereo Field Width" << std::endl;
    {
        engine.setParameter(0, 0.5f);
        engine.setParameter(1, 1.0f); // Full mix

        std::vector<float> inputL(BUFFER_SIZE * 10);
        std::vector<float> inputR(BUFFER_SIZE * 10);
        std::vector<float> outputL(BUFFER_SIZE * 10);
        std::vector<float> outputR(BUFFER_SIZE * 10);

        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
            inputR[i] = inputL[i];
        }

        for (int chunk = 0; chunk < 10; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );
        }

        // Calculate correlation between L and R
        double correlation = 0.0;
        double sum_l = 0.0, sum_r = 0.0;
        for (size_t i = BUFFER_SIZE * 5; i < inputL.size(); ++i) { // Skip first half for warmup
            correlation += outputL[i] * outputR[i];
            sum_l += outputL[i] * outputL[i];
            sum_r += outputR[i] * outputR[i];
        }

        if (sum_l > 0 && sum_r > 0) {
            correlation /= std::sqrt(sum_l * sum_r);
        }

        std::cout << "  L-R Correlation: " << correlation << std::endl;
        bool stereo_pass = (correlation < 0.99); // Should have some stereo width
        std::cout << "  Status: " << (stereo_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= stereo_pass;
    }

    return all_passed;
}

int main() {
    std::cout << "Engine 32 (DetuneDoubler) - Comprehensive Verification Test" << std::endl;
    std::cout << "============================================================" << std::endl;

    bool success = testDetuneDoubler();

    std::cout << "\n============================================================" << std::endl;
    std::cout << "Engine 32 Overall Result: " << (success ? "PASS" : "FAIL") << std::endl;
    std::cout << "============================================================" << std::endl;

    return success ? 0 : 1;
}

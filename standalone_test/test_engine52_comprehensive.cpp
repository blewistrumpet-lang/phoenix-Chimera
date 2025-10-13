// Comprehensive test for Engine 52 (SpectralGate)
// Tests: 1000 cycle stress test, zero crashes, extreme parameters, output quality

#include "../src/AudioEngine.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;

bool testSpectralGate() {
    std::cout << "\n=== Engine 52 (SpectralGate) Comprehensive Test ===" << std::endl;

    AudioEngine engine;
    engine.initialize(SAMPLE_RATE, BUFFER_SIZE);
    engine.setCurrentEngine(52); // SpectralGate

    bool all_passed = true;

    // Test 1: 1000 cycle stress test
    std::cout << "\n[Test 1] 1000 Cycle Stress Test" << std::endl;
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<float> inputL(BUFFER_SIZE);
        std::vector<float> inputR(BUFFER_SIZE);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        int crash_count = 0;
        int nan_count = 0;
        int inf_count = 0;

        for (int cycle = 0; cycle < 1000; ++cycle) {
            // Vary input signal
            for (int i = 0; i < BUFFER_SIZE; ++i) {
                double t = (cycle * BUFFER_SIZE + i) / SAMPLE_RATE;
                inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t) +
                           0.3f * std::sin(2.0 * M_PI * 880.0 * t);
                inputR[i] = inputL[i];
            }

            // Vary parameters during processing
            if (cycle % 100 == 0) {
                engine.setParameter(0, cycle / 1000.0f); // Threshold
                engine.setParameter(1, (cycle % 200) / 200.0f); // Attack
                engine.setParameter(2, (cycle % 300) / 300.0f); // Release
            }

            try {
                engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);

                // Check for NaN/Inf
                for (int i = 0; i < BUFFER_SIZE; ++i) {
                    if (std::isnan(outputL[i]) || std::isnan(outputR[i])) {
                        nan_count++;
                        break;
                    }
                    if (std::isinf(outputL[i]) || std::isinf(outputR[i])) {
                        inf_count++;
                        break;
                    }
                }
            } catch (...) {
                crash_count++;
            }

            if (cycle % 100 == 0) {
                std::cout << "  Progress: " << cycle << "/1000 cycles..." << std::endl;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "  Completed 1000 cycles in " << duration << " ms" << std::endl;
        std::cout << "  Crashes: " << crash_count << std::endl;
        std::cout << "  NaN outputs: " << nan_count << std::endl;
        std::cout << "  Inf outputs: " << inf_count << std::endl;

        bool stress_pass = (crash_count == 0 && nan_count == 0 && inf_count == 0);
        std::cout << "  Status: " << (stress_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= stress_pass;
    }

    // Test 2: Extreme parameters
    std::cout << "\n[Test 2] Extreme Parameter Test" << std::endl;
    {
        std::vector<std::pair<float, float>> extreme_params = {
            {0.0f, 0.0f},   // All minimum
            {1.0f, 1.0f},   // All maximum
            {0.0f, 1.0f},   // Mixed
            {1.0f, 0.0f},   // Mixed
            {0.5f, 0.5f}    // Middle
        };

        std::vector<float> inputL(BUFFER_SIZE);
        std::vector<float> inputR(BUFFER_SIZE);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        for (int i = 0; i < BUFFER_SIZE; ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.7f * std::sin(2.0 * M_PI * 1000.0 * t);
            inputR[i] = inputL[i];
        }

        int test_num = 0;
        for (const auto& params : extreme_params) {
            engine.setParameter(0, params.first);  // Threshold
            engine.setParameter(1, params.second); // Attack/Release

            bool crashed = false;
            bool has_nan = false;

            try {
                // Process multiple blocks to stabilize
                for (int block = 0; block < 10; ++block) {
                    engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);

                    for (int i = 0; i < BUFFER_SIZE; ++i) {
                        if (std::isnan(outputL[i]) || std::isnan(outputR[i]) ||
                            std::isinf(outputL[i]) || std::isinf(outputR[i])) {
                            has_nan = true;
                            break;
                        }
                    }
                }
            } catch (...) {
                crashed = true;
            }

            bool param_pass = (!crashed && !has_nan);
            std::cout << "  Test " << ++test_num << " (Threshold=" << params.first
                     << ", Attack=" << params.second << "): "
                     << (param_pass ? "PASS" : "FAIL") << std::endl;
            all_passed &= param_pass;
        }
    }

    // Test 3: Output quality verification
    std::cout << "\n[Test 3] Output Quality Verification" << std::endl;
    {
        engine.setParameter(0, 0.3f); // Moderate threshold
        engine.setParameter(1, 0.5f); // Moderate attack
        engine.setParameter(2, 0.5f); // Moderate release

        std::vector<float> inputL(BUFFER_SIZE * 20);
        std::vector<float> inputR(BUFFER_SIZE * 20);
        std::vector<float> outputL(BUFFER_SIZE * 20);
        std::vector<float> outputR(BUFFER_SIZE * 20);

        // Generate signal with noise
        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.6f * std::sin(2.0 * M_PI * 440.0 * t) +
                       0.05f * (2.0f * (rand() / (float)RAND_MAX) - 1.0f); // Add noise
            inputR[i] = inputL[i];
        }

        // Process
        for (int chunk = 0; chunk < 20; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );
        }

        // Check output is reasonable (not silent, not clipping excessively)
        float max_output = 0.0f;
        double rms = 0.0;
        int clip_count = 0;

        for (size_t i = BUFFER_SIZE * 5; i < outputL.size(); ++i) { // Skip warmup
            max_output = std::max(max_output, std::abs(outputL[i]));
            rms += outputL[i] * outputL[i];
            if (std::abs(outputL[i]) > 0.99f) clip_count++;
        }

        rms = std::sqrt(rms / (outputL.size() - BUFFER_SIZE * 5));

        std::cout << "  Max Output: " << max_output << std::endl;
        std::cout << "  RMS Level: " << rms << std::endl;
        std::cout << "  Clips: " << clip_count << std::endl;

        bool quality_pass = (max_output > 0.01f && max_output < 1.5f && rms > 0.01);
        std::cout << "  Status: " << (quality_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= quality_pass;
    }

    // Test 4: Silence handling
    std::cout << "\n[Test 4] Silence Handling" << std::endl;
    {
        std::vector<float> inputL(BUFFER_SIZE, 0.0f);
        std::vector<float> inputR(BUFFER_SIZE, 0.0f);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        bool crashed = false;
        bool has_nan = false;

        try {
            for (int i = 0; i < 100; ++i) {
                engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);

                for (int j = 0; j < BUFFER_SIZE; ++j) {
                    if (std::isnan(outputL[j]) || std::isnan(outputR[j])) {
                        has_nan = true;
                        break;
                    }
                }
            }
        } catch (...) {
            crashed = true;
        }

        bool silence_pass = (!crashed && !has_nan);
        std::cout << "  Status: " << (silence_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= silence_pass;
    }

    return all_passed;
}

int main() {
    std::cout << "Engine 52 (SpectralGate) - Comprehensive Verification Test" << std::endl;
    std::cout << "==========================================================" << std::endl;

    bool success = testSpectralGate();

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "Engine 52 Overall Result: " << (success ? "PASS" : "FAIL") << std::endl;
    std::cout << "==========================================================" << std::endl;

    return success ? 0 : 1;
}

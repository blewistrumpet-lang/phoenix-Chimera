// Comprehensive test for Engine 21 (RodentDistortion)
// Tests: Zero denormals verification, CPU performance, audio quality maintained

#include "../src/AudioEngine.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>
#include <cfenv>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;

// Check for denormal numbers
bool hasDenormals(const float* buffer, int size) {
    for (int i = 0; i < size; ++i) {
        if (std::fpclassify(buffer[i]) == FP_SUBNORMAL) {
            return true;
        }
    }
    return false;
}

bool testRodentDistortion() {
    std::cout << "\n=== Engine 21 (RodentDistortion) Comprehensive Test ===" << std::endl;

    AudioEngine engine;
    engine.initialize(SAMPLE_RATE, BUFFER_SIZE);
    engine.setCurrentEngine(21); // RodentDistortion

    bool all_passed = true;

    // Test 1: Zero denormals verification
    std::cout << "\n[Test 1] Zero Denormals Verification" << std::endl;
    {
        engine.setParameter(0, 0.7f); // Distortion
        engine.setParameter(1, 0.5f); // Filter
        engine.setParameter(2, 0.6f); // Level

        std::vector<float> inputL(BUFFER_SIZE * 100);
        std::vector<float> inputR(BUFFER_SIZE * 100);
        std::vector<float> outputL(BUFFER_SIZE * 100);
        std::vector<float> outputR(BUFFER_SIZE * 100);

        // Test with various signal types that might produce denormals

        // 1. Very low amplitude sine wave
        for (size_t i = 0; i < inputL.size() / 4; ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 1e-20f * std::sin(2.0 * M_PI * 440.0 * t);
            inputR[i] = inputL[i];
        }

        // 2. Decaying signal
        for (size_t i = inputL.size() / 4; i < inputL.size() / 2; ++i) {
            double t = (i - inputL.size() / 4) / SAMPLE_RATE;
            float decay = std::exp(-t * 10.0);
            inputL[i] = 0.5f * decay * std::sin(2.0 * M_PI * 440.0 * t);
            inputR[i] = inputL[i];
        }

        // 3. Near-silence
        for (size_t i = inputL.size() / 2; i < 3 * inputL.size() / 4; ++i) {
            inputL[i] = 1e-30f;
            inputR[i] = 1e-30f;
        }

        // 4. Normal signal
        for (size_t i = 3 * inputL.size() / 4; i < inputL.size(); ++i) {
            double t = (i - 3 * inputL.size() / 4) / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
            inputR[i] = inputL[i];
        }

        int denormal_count = 0;
        int blocks_with_denormals = 0;

        for (int chunk = 0; chunk < 100; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );

            // Check for denormals in output
            if (hasDenormals(outputL.data() + chunk * BUFFER_SIZE, BUFFER_SIZE) ||
                hasDenormals(outputR.data() + chunk * BUFFER_SIZE, BUFFER_SIZE)) {
                blocks_with_denormals++;

                // Count individual denormals
                for (int i = 0; i < BUFFER_SIZE; ++i) {
                    if (std::fpclassify(outputL[chunk * BUFFER_SIZE + i]) == FP_SUBNORMAL) denormal_count++;
                    if (std::fpclassify(outputR[chunk * BUFFER_SIZE + i]) == FP_SUBNORMAL) denormal_count++;
                }
            }
        }

        std::cout << "  Denormal samples: " << denormal_count << std::endl;
        std::cout << "  Blocks with denormals: " << blocks_with_denormals << std::endl;

        bool denormal_pass = (denormal_count == 0);
        std::cout << "  Status: " << (denormal_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= denormal_pass;
    }

    // Test 2: CPU performance check
    std::cout << "\n[Test 2] CPU Performance Check" << std::endl;
    {
        engine.setParameter(0, 0.7f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.6f);

        const int NUM_BLOCKS = 10000;
        std::vector<float> inputL(BUFFER_SIZE);
        std::vector<float> inputR(BUFFER_SIZE);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        for (int i = 0; i < BUFFER_SIZE; ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
            inputR[i] = inputL[i];
        }

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < NUM_BLOCKS; ++i) {
            engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        double total_time_us = duration_us;
        double audio_time_us = (NUM_BLOCKS * BUFFER_SIZE * 1000000.0) / SAMPLE_RATE;
        double cpu_percent = (total_time_us / audio_time_us) * 100.0;

        std::cout << "  Processing time: " << total_time_us << " us" << std::endl;
        std::cout << "  Audio time: " << audio_time_us << " us" << std::endl;
        std::cout << "  CPU Usage: " << cpu_percent << "%" << std::endl;

        bool cpu_pass = (cpu_percent < 1.0); // Reasonable threshold
        std::cout << "  Status: " << (cpu_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= cpu_pass;
    }

    // Test 3: Audio quality maintained
    std::cout << "\n[Test 3] Audio Quality Verification" << std::endl;
    {
        engine.setParameter(0, 0.6f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.7f);

        std::vector<float> inputL(BUFFER_SIZE * 30);
        std::vector<float> inputR(BUFFER_SIZE * 30);
        std::vector<float> outputL(BUFFER_SIZE * 30);
        std::vector<float> outputR(BUFFER_SIZE * 30);

        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
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

        float max_output = 0.0f;
        double rms = 0.0;
        int nan_count = 0;
        int inf_count = 0;
        size_t skip = BUFFER_SIZE * 5;

        for (size_t i = skip; i < outputL.size(); ++i) {
            max_output = std::max(max_output, std::abs(outputL[i]));
            rms += outputL[i] * outputL[i];
            if (std::isnan(outputL[i])) nan_count++;
            if (std::isinf(outputL[i])) inf_count++;
        }
        rms = std::sqrt(rms / (outputL.size() - skip));

        std::cout << "  Max Output: " << max_output << std::endl;
        std::cout << "  RMS Level: " << rms << std::endl;
        std::cout << "  NaN count: " << nan_count << std::endl;
        std::cout << "  Inf count: " << inf_count << std::endl;

        bool quality_pass = (max_output > 0.1f && max_output < 2.0f && nan_count == 0 && inf_count == 0 && rms > 0.1);
        std::cout << "  Status: " << (quality_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= quality_pass;
    }

    // Test 4: Distortion at various drive levels
    std::cout << "\n[Test 4] Distortion at Various Drive Levels" << std::endl;
    {
        std::vector<float> drive_levels = {0.0f, 0.3f, 0.6f, 1.0f};

        for (float drive : drive_levels) {
            engine.setParameter(0, drive);
            engine.setParameter(1, 0.5f);
            engine.setParameter(2, 0.7f);

            std::vector<float> inputL(BUFFER_SIZE * 20);
            std::vector<float> inputR(BUFFER_SIZE * 20);
            std::vector<float> outputL(BUFFER_SIZE * 20);
            std::vector<float> outputR(BUFFER_SIZE * 20);

            for (size_t i = 0; i < inputL.size(); ++i) {
                double t = i / SAMPLE_RATE;
                inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
                inputR[i] = inputL[i];
            }

            int denormal_count = 0;

            for (int chunk = 0; chunk < 20; ++chunk) {
                engine.processBlock(
                    inputL.data() + chunk * BUFFER_SIZE,
                    inputR.data() + chunk * BUFFER_SIZE,
                    outputL.data() + chunk * BUFFER_SIZE,
                    outputR.data() + chunk * BUFFER_SIZE,
                    BUFFER_SIZE
                );

                if (hasDenormals(outputL.data() + chunk * BUFFER_SIZE, BUFFER_SIZE) ||
                    hasDenormals(outputR.data() + chunk * BUFFER_SIZE, BUFFER_SIZE)) {
                    denormal_count++;
                }
            }

            float max_out = 0.0f;
            size_t skip = BUFFER_SIZE * 5;
            for (size_t i = skip; i < outputL.size(); ++i) {
                max_out = std::max(max_out, std::abs(outputL[i]));
            }

            bool drive_ok = (denormal_count == 0 && max_out > 0.05f);
            std::cout << "  Drive " << drive << ": Max=" << max_out
                     << ", Denormals=" << denormal_count
                     << " - " << (drive_ok ? "PASS" : "FAIL") << std::endl;
            all_passed &= drive_ok;
        }
    }

    // Test 5: Silence handling (critical for denormals)
    std::cout << "\n[Test 5] Silence Handling" << std::endl;
    {
        engine.setParameter(0, 0.8f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.7f);

        std::vector<float> inputL(BUFFER_SIZE, 0.0f);
        std::vector<float> inputR(BUFFER_SIZE, 0.0f);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        int denormal_count = 0;
        int nan_count = 0;

        for (int i = 0; i < 200; ++i) {
            engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);

            if (hasDenormals(outputL.data(), BUFFER_SIZE) || hasDenormals(outputR.data(), BUFFER_SIZE)) {
                denormal_count++;
            }

            for (int j = 0; j < BUFFER_SIZE; ++j) {
                if (std::isnan(outputL[j]) || std::isnan(outputR[j])) nan_count++;
            }
        }

        std::cout << "  Blocks with denormals: " << denormal_count << std::endl;
        std::cout << "  NaN count: " << nan_count << std::endl;

        bool silence_pass = (denormal_count == 0 && nan_count == 0);
        std::cout << "  Status: " << (silence_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= silence_pass;
    }

    // Test 6: Long-term stability
    std::cout << "\n[Test 6] Long-term Stability Test" << std::endl;
    {
        engine.setParameter(0, 0.7f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.6f);

        std::vector<float> inputL(BUFFER_SIZE);
        std::vector<float> inputR(BUFFER_SIZE);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        int total_denormals = 0;
        int nan_count = 0;
        int inf_count = 0;

        for (int i = 0; i < 2000; ++i) {
            // Generate varying input
            for (int j = 0; j < BUFFER_SIZE; ++j) {
                double t = (i * BUFFER_SIZE + j) / SAMPLE_RATE;
                inputL[j] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
                inputR[j] = inputL[j];
            }

            engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);

            if (hasDenormals(outputL.data(), BUFFER_SIZE) || hasDenormals(outputR.data(), BUFFER_SIZE)) {
                total_denormals++;
            }

            for (int j = 0; j < BUFFER_SIZE; ++j) {
                if (std::isnan(outputL[j]) || std::isnan(outputR[j])) nan_count++;
                if (std::isinf(outputL[j]) || std::isinf(outputR[j])) inf_count++;
            }
        }

        std::cout << "  Processed 2000 blocks" << std::endl;
        std::cout << "  Blocks with denormals: " << total_denormals << std::endl;
        std::cout << "  NaN count: " << nan_count << std::endl;
        std::cout << "  Inf count: " << inf_count << std::endl;

        bool stability_pass = (total_denormals == 0 && nan_count == 0 && inf_count == 0);
        std::cout << "  Status: " << (stability_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= stability_pass;
    }

    return all_passed;
}

int main() {
    std::cout << "Engine 21 (RodentDistortion) - Comprehensive Verification Test" << std::endl;
    std::cout << "==============================================================" << std::endl;

    bool success = testRodentDistortion();

    std::cout << "\n==============================================================" << std::endl;
    std::cout << "Engine 21 Overall Result: " << (success ? "PASS" : "FAIL") << std::endl;
    std::cout << "==============================================================" << std::endl;

    return success ? 0 : 1;
}

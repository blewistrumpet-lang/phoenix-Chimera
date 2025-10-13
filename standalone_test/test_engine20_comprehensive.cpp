// Comprehensive test for Engine 20 (MuffFuzz)
// Tests: CPU <0.52%, audio quality maintained, distortion character, performance stability

#include "../src/AudioEngine.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;

bool testMuffFuzz() {
    std::cout << "\n=== Engine 20 (MuffFuzz) Comprehensive Test ===" << std::endl;

    AudioEngine engine;
    engine.initialize(SAMPLE_RATE, BUFFER_SIZE);
    engine.setCurrentEngine(20); // MuffFuzz

    bool all_passed = true;

    // Test 1: CPU <0.52% verification
    std::cout << "\n[Test 1] CPU Performance Test (<0.52% target)" << std::endl;
    {
        engine.setParameter(0, 0.7f); // Drive
        engine.setParameter(1, 0.5f); // Tone
        engine.setParameter(2, 0.5f); // Level

        const int NUM_BLOCKS = 10000;
        std::vector<float> inputL(BUFFER_SIZE);
        std::vector<float> inputR(BUFFER_SIZE);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        // Generate test signal
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
            inputR[i] = inputL[i];
        }

        // Measure processing time
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < NUM_BLOCKS; ++i) {
            engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // Calculate CPU usage
        double total_time_us = duration_us;
        double audio_time_us = (NUM_BLOCKS * BUFFER_SIZE * 1000000.0) / SAMPLE_RATE;
        double cpu_percent = (total_time_us / audio_time_us) * 100.0;

        std::cout << "  Processing time: " << total_time_us << " us" << std::endl;
        std::cout << "  Audio time: " << audio_time_us << " us" << std::endl;
        std::cout << "  CPU Usage: " << cpu_percent << "%" << std::endl;

        bool cpu_pass = (cpu_percent < 0.52);
        std::cout << "  Status: " << (cpu_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= cpu_pass;
    }

    // Test 2: Audio quality maintained
    std::cout << "\n[Test 2] Audio Quality Verification" << std::endl;
    {
        engine.setParameter(0, 0.6f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.7f);

        std::vector<float> inputL(BUFFER_SIZE * 30);
        std::vector<float> inputR(BUFFER_SIZE * 30);
        std::vector<float> outputL(BUFFER_SIZE * 30);
        std::vector<float> outputR(BUFFER_SIZE * 30);

        // Generate test signal
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

        // Check output quality
        float max_output = 0.0f;
        double rms = 0.0;
        int nan_count = 0;
        int clip_count = 0;
        size_t skip = BUFFER_SIZE * 5;

        for (size_t i = skip; i < outputL.size(); ++i) {
            max_output = std::max(max_output, std::abs(outputL[i]));
            rms += outputL[i] * outputL[i];
            if (std::isnan(outputL[i]) || std::isinf(outputL[i])) nan_count++;
            if (std::abs(outputL[i]) > 0.99f) clip_count++;
        }
        rms = std::sqrt(rms / (outputL.size() - skip));

        std::cout << "  Max Output: " << max_output << std::endl;
        std::cout << "  RMS Level: " << rms << std::endl;
        std::cout << "  NaN/Inf count: " << nan_count << std::endl;
        std::cout << "  Clip count: " << clip_count << std::endl;

        bool quality_pass = (max_output > 0.1f && max_output < 1.5f && nan_count == 0 && rms > 0.1);
        std::cout << "  Status: " << (quality_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= quality_pass;
    }

    // Test 3: Distortion character check
    std::cout << "\n[Test 3] Distortion Character Check" << std::endl;
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

            for (int chunk = 0; chunk < 20; ++chunk) {
                engine.processBlock(
                    inputL.data() + chunk * BUFFER_SIZE,
                    inputR.data() + chunk * BUFFER_SIZE,
                    outputL.data() + chunk * BUFFER_SIZE,
                    outputR.data() + chunk * BUFFER_SIZE,
                    BUFFER_SIZE
                );
            }

            // Measure output characteristics
            float max_out = 0.0f;
            double rms = 0.0;
            size_t skip = BUFFER_SIZE * 5;

            for (size_t i = skip; i < outputL.size(); ++i) {
                max_out = std::max(max_out, std::abs(outputL[i]));
                rms += outputL[i] * outputL[i];
            }
            rms = std::sqrt(rms / (outputL.size() - skip));

            bool char_ok = (rms > 0.05 && max_out < 2.0f);
            std::cout << "  Drive " << drive << ": RMS=" << rms << ", Max=" << max_out
                     << " - " << (char_ok ? "PASS" : "FAIL") << std::endl;
            all_passed &= char_ok;
        }
    }

    // Test 4: Performance stability over time
    std::cout << "\n[Test 4] Performance Stability Test" << std::endl;
    {
        engine.setParameter(0, 0.7f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.6f);

        std::vector<float> inputL(BUFFER_SIZE);
        std::vector<float> inputR(BUFFER_SIZE);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        const int TEST_BLOCKS = 5000;
        std::vector<double> block_times;
        block_times.reserve(TEST_BLOCKS);

        for (int i = 0; i < TEST_BLOCKS; ++i) {
            // Generate input
            for (int j = 0; j < BUFFER_SIZE; ++j) {
                double t = (i * BUFFER_SIZE + j) / SAMPLE_RATE;
                inputL[j] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
                inputR[j] = inputL[j];
            }

            auto start = std::chrono::high_resolution_clock::now();
            engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);
            auto end = std::chrono::high_resolution_clock::now();

            double block_time_us = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000.0;
            block_times.push_back(block_time_us);
        }

        // Calculate statistics
        double avg_time = 0.0;
        double max_time = 0.0;
        double min_time = 1e9;

        for (double t : block_times) {
            avg_time += t;
            max_time = std::max(max_time, t);
            min_time = std::min(min_time, t);
        }
        avg_time /= block_times.size();

        // Calculate standard deviation
        double variance = 0.0;
        for (double t : block_times) {
            variance += (t - avg_time) * (t - avg_time);
        }
        double std_dev = std::sqrt(variance / block_times.size());

        std::cout << "  Average time: " << avg_time << " us" << std::endl;
        std::cout << "  Min time: " << min_time << " us" << std::endl;
        std::cout << "  Max time: " << max_time << " us" << std::endl;
        std::cout << "  Std dev: " << std_dev << " us" << std::endl;

        // Check stability (low variance is good)
        bool stability_pass = (std_dev < avg_time * 0.5); // Std dev less than 50% of mean
        std::cout << "  Status: " << (stability_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= stability_pass;
    }

    // Test 5: Different input signals
    std::cout << "\n[Test 5] Various Input Signals" << std::endl;
    {
        engine.setParameter(0, 0.6f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.7f);

        std::vector<std::pair<std::string, std::vector<float>>> test_signals;

        // Generate different signal types
        std::vector<float> sine(BUFFER_SIZE * 20);
        std::vector<float> square(BUFFER_SIZE * 20);
        std::vector<float> noise(BUFFER_SIZE * 20);

        for (size_t i = 0; i < sine.size(); ++i) {
            double t = i / SAMPLE_RATE;
            sine[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
            square[i] = (std::sin(2.0 * M_PI * 440.0 * t) > 0) ? 0.5f : -0.5f;
            noise[i] = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 0.3f;
        }

        test_signals.push_back({"Sine", sine});
        test_signals.push_back({"Square", square});
        test_signals.push_back({"Noise", noise});

        for (const auto& signal : test_signals) {
            std::vector<float> outputL(signal.second.size());
            std::vector<float> outputR(signal.second.size());

            for (size_t chunk = 0; chunk < signal.second.size() / BUFFER_SIZE; ++chunk) {
                engine.processBlock(
                    signal.second.data() + chunk * BUFFER_SIZE,
                    signal.second.data() + chunk * BUFFER_SIZE,
                    outputL.data() + chunk * BUFFER_SIZE,
                    outputR.data() + chunk * BUFFER_SIZE,
                    BUFFER_SIZE
                );
            }

            // Check output
            float max_out = 0.0f;
            int nan_count = 0;
            size_t skip = BUFFER_SIZE * 5;

            for (size_t i = skip; i < outputL.size(); ++i) {
                max_out = std::max(max_out, std::abs(outputL[i]));
                if (std::isnan(outputL[i]) || std::isinf(outputL[i])) nan_count++;
            }

            bool signal_ok = (max_out > 0.05f && nan_count == 0);
            std::cout << "  " << signal.first << ": Max=" << max_out
                     << " - " << (signal_ok ? "PASS" : "FAIL") << std::endl;
            all_passed &= signal_ok;
        }
    }

    return all_passed;
}

int main() {
    std::cout << "Engine 20 (MuffFuzz) - Comprehensive Verification Test" << std::endl;
    std::cout << "======================================================" << std::endl;

    bool success = testMuffFuzz();

    std::cout << "\n======================================================" << std::endl;
    std::cout << "Engine 20 Overall Result: " << (success ? "PASS" : "FAIL") << std::endl;
    std::cout << "======================================================" << std::endl;

    return success ? 0 : 1;
}

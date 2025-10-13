// Comprehensive test for Engine 40 (ShimmerReverb)
// Tests: Non-zero output, stereo width >0.8, reverb tail quality, shimmer effect

#include "../src/AudioEngine.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;

double calculateStereoWidth(const std::vector<float>& left, const std::vector<float>& right, size_t skip) {
    double correlation = 0.0;
    double sum_l = 0.0, sum_r = 0.0;

    for (size_t i = skip; i < left.size() && i < right.size(); ++i) {
        correlation += left[i] * right[i];
        sum_l += left[i] * left[i];
        sum_r += right[i] * right[i];
    }

    if (sum_l > 0 && sum_r > 0) {
        correlation /= std::sqrt(sum_l * sum_r);
    }

    return 1.0 - std::abs(correlation);
}

bool testShimmerReverb() {
    std::cout << "\n=== Engine 40 (ShimmerReverb) Comprehensive Test ===" << std::endl;

    AudioEngine engine;
    engine.initialize(SAMPLE_RATE, BUFFER_SIZE);
    engine.setCurrentEngine(40); // ShimmerReverb

    bool all_passed = true;

    // Test 1: Non-zero output verification
    std::cout << "\n[Test 1] Non-Zero Output Verification" << std::endl;
    {
        engine.setParameter(0, 0.7f); // Room size
        engine.setParameter(1, 0.5f); // Shimmer amount
        engine.setParameter(2, 1.0f); // Wet mix

        std::vector<float> inputL(BUFFER_SIZE * 50);
        std::vector<float> inputR(BUFFER_SIZE * 50);
        std::vector<float> outputL(BUFFER_SIZE * 50);
        std::vector<float> outputR(BUFFER_SIZE * 50);

        // Generate input signal
        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
            inputR[i] = inputL[i];
        }

        // Process
        for (int chunk = 0; chunk < 50; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );
        }

        // Check for non-zero output
        float max_output = 0.0f;
        double rms = 0.0;
        size_t skip = BUFFER_SIZE * 10;

        for (size_t i = skip; i < outputL.size(); ++i) {
            max_output = std::max(max_output, std::abs(outputL[i]));
            rms += outputL[i] * outputL[i];
        }
        rms = std::sqrt(rms / (outputL.size() - skip));

        std::cout << "  Max Output: " << max_output << std::endl;
        std::cout << "  RMS Level: " << rms << std::endl;

        bool output_pass = (max_output > 0.01f && rms > 0.01);
        std::cout << "  Status: " << (output_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= output_pass;
    }

    // Test 2: Stereo width >0.8 verification
    std::cout << "\n[Test 2] Stereo Width Verification (Target >0.8)" << std::endl;
    {
        engine.setParameter(0, 0.7f);
        engine.setParameter(1, 0.6f);
        engine.setParameter(2, 1.0f);

        std::vector<float> inputL(BUFFER_SIZE * 100);
        std::vector<float> inputR(BUFFER_SIZE * 100);
        std::vector<float> outputL(BUFFER_SIZE * 100);
        std::vector<float> outputR(BUFFER_SIZE * 100);

        // Generate noise input (mono)
        for (size_t i = 0; i < inputL.size(); ++i) {
            float noise = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 0.3f;
            inputL[i] = noise;
            inputR[i] = noise;
        }

        for (int chunk = 0; chunk < 100; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );
        }

        double stereo_width = calculateStereoWidth(outputL, outputR, BUFFER_SIZE * 30);
        std::cout << "  Stereo Width: " << stereo_width << std::endl;

        bool width_pass = (stereo_width > 0.8);
        std::cout << "  Status: " << (width_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= width_pass;
    }

    // Test 3: Reverb tail quality
    std::cout << "\n[Test 3] Reverb Tail Quality" << std::endl;
    {
        engine.setParameter(0, 0.8f); // Large room
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 1.0f);

        // Send short burst and measure tail
        std::vector<float> inputL(BUFFER_SIZE * 200, 0.0f);
        std::vector<float> inputR(BUFFER_SIZE * 200, 0.0f);
        std::vector<float> outputL(BUFFER_SIZE * 200);
        std::vector<float> outputR(BUFFER_SIZE * 200);

        // Generate short burst at the beginning
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            inputL[i] = 0.7f * std::sin(2.0 * M_PI * 440.0 * i / SAMPLE_RATE);
            inputR[i] = inputL[i];
        }

        for (int chunk = 0; chunk < 200; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );
        }

        // Measure tail decay
        int tail_samples = 0;
        for (size_t i = BUFFER_SIZE * 2; i < outputL.size(); ++i) {
            if (std::abs(outputL[i]) > 0.001f) {
                tail_samples = i;
            }
        }

        double tail_duration_ms = (tail_samples / SAMPLE_RATE) * 1000.0;
        std::cout << "  Tail Duration: " << tail_duration_ms << " ms" << std::endl;

        bool tail_pass = (tail_duration_ms > 100.0); // Should have significant tail
        std::cout << "  Status: " << (tail_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= tail_pass;
    }

    // Test 4: Shimmer effect validation
    std::cout << "\n[Test 4] Shimmer Effect Validation" << std::endl;
    {
        std::vector<float> shimmer_amounts = {0.0f, 0.3f, 0.6f, 1.0f};

        for (float shimmer : shimmer_amounts) {
            engine.setParameter(0, 0.6f);
            engine.setParameter(1, shimmer);
            engine.setParameter(2, 1.0f);

            std::vector<float> inputL(BUFFER_SIZE * 40);
            std::vector<float> inputR(BUFFER_SIZE * 40);
            std::vector<float> outputL(BUFFER_SIZE * 40);
            std::vector<float> outputR(BUFFER_SIZE * 40);

            for (size_t i = 0; i < inputL.size(); ++i) {
                double t = i / SAMPLE_RATE;
                inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
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

            // Check output characteristics
            float max_output = 0.0f;
            double rms = 0.0;
            size_t skip = BUFFER_SIZE * 10;

            for (size_t i = skip; i < outputL.size(); ++i) {
                max_output = std::max(max_output, std::abs(outputL[i]));
                rms += outputL[i] * outputL[i];
            }
            rms = std::sqrt(rms / (outputL.size() - skip));

            bool shimmer_ok = (max_output > 0.01f && rms > 0.01);
            std::cout << "  Shimmer " << shimmer << ": RMS=" << rms
                     << ", Max=" << max_output
                     << " - " << (shimmer_ok ? "PASS" : "FAIL") << std::endl;
            all_passed &= shimmer_ok;
        }
    }

    // Test 5: Parameter stability
    std::cout << "\n[Test 5] Parameter Stability Test" << std::endl;
    {
        std::vector<float> inputL(BUFFER_SIZE);
        std::vector<float> inputR(BUFFER_SIZE);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        int nan_count = 0;
        int inf_count = 0;

        for (int i = 0; i < 500; ++i) {
            // Vary parameters dynamically
            if (i % 50 == 0) {
                engine.setParameter(0, (i % 100) / 100.0f);
                engine.setParameter(1, (i % 75) / 75.0f);
                engine.setParameter(2, 0.8f);
            }

            for (int j = 0; j < BUFFER_SIZE; ++j) {
                double t = (i * BUFFER_SIZE + j) / SAMPLE_RATE;
                inputL[j] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
                inputR[j] = inputL[j];
            }

            engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);

            for (int j = 0; j < BUFFER_SIZE; ++j) {
                if (std::isnan(outputL[j]) || std::isnan(outputR[j])) nan_count++;
                if (std::isinf(outputL[j]) || std::isinf(outputR[j])) inf_count++;
            }
        }

        std::cout << "  Processed 500 blocks with varying parameters" << std::endl;
        std::cout << "  NaN count: " << nan_count << std::endl;
        std::cout << "  Inf count: " << inf_count << std::endl;

        bool stability_pass = (nan_count == 0 && inf_count == 0);
        std::cout << "  Status: " << (stability_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= stability_pass;
    }

    // Test 6: Stereo width measurement at various settings
    std::cout << "\n[Test 6] Stereo Width at Various Settings" << std::endl;
    {
        std::vector<std::pair<float, float>> settings = {
            {0.5f, 0.3f},
            {0.7f, 0.5f},
            {0.9f, 0.7f}
        };

        for (const auto& setting : settings) {
            engine.setParameter(0, setting.first);
            engine.setParameter(1, setting.second);
            engine.setParameter(2, 1.0f);

            std::vector<float> inputL(BUFFER_SIZE * 60);
            std::vector<float> inputR(BUFFER_SIZE * 60);
            std::vector<float> outputL(BUFFER_SIZE * 60);
            std::vector<float> outputR(BUFFER_SIZE * 60);

            for (size_t i = 0; i < inputL.size(); ++i) {
                float noise = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 0.3f;
                inputL[i] = noise;
                inputR[i] = noise;
            }

            for (int chunk = 0; chunk < 60; ++chunk) {
                engine.processBlock(
                    inputL.data() + chunk * BUFFER_SIZE,
                    inputR.data() + chunk * BUFFER_SIZE,
                    outputL.data() + chunk * BUFFER_SIZE,
                    outputR.data() + chunk * BUFFER_SIZE,
                    BUFFER_SIZE
                );
            }

            double width = calculateStereoWidth(outputL, outputR, BUFFER_SIZE * 20);
            bool width_ok = (width > 0.8);
            std::cout << "  Room=" << setting.first << ", Shimmer=" << setting.second
                     << ": Width=" << width << " - " << (width_ok ? "PASS" : "FAIL") << std::endl;
        }
    }

    return all_passed;
}

int main() {
    std::cout << "Engine 40 (ShimmerReverb) - Comprehensive Verification Test" << std::endl;
    std::cout << "==========================================================" << std::endl;

    bool success = testShimmerReverb();

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "Engine 40 Overall Result: " << (success ? "PASS" : "FAIL") << std::endl;
    std::cout << "==========================================================" << std::endl;

    return success ? 0 : 1;
}

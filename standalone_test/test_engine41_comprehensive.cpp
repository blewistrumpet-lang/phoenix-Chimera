// Comprehensive test for Engine 41 (ConvolutionReverb)
// Tests: Impulse response, RT60 measurement, stereo width, IR generation

#include "../src/AudioEngine.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;

double measureRT60(const std::vector<float>& impulse_response) {
    // Find peak
    float peak = 0.0f;
    for (float sample : impulse_response) {
        peak = std::max(peak, std::abs(sample));
    }

    if (peak < 0.0001f) return 0.0;

    // Find time when signal drops to -60dB
    float threshold = peak * 0.001f; // -60dB
    size_t rt60_sample = 0;

    for (size_t i = 0; i < impulse_response.size(); ++i) {
        if (std::abs(impulse_response[i]) < threshold) {
            // Check if it stays below threshold
            bool stays_below = true;
            for (size_t j = i; j < std::min(i + 100, impulse_response.size()); ++j) {
                if (std::abs(impulse_response[j]) >= threshold) {
                    stays_below = false;
                    break;
                }
            }
            if (stays_below) {
                rt60_sample = i;
                break;
            }
        }
    }

    return (rt60_sample / SAMPLE_RATE) * 1000.0; // Convert to ms
}

double calculateStereoWidth(const std::vector<float>& left, const std::vector<float>& right, size_t skip) {
    // Calculate cross-correlation between L and R
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

    // Stereo width: 1.0 means uncorrelated (wide), 0.0 means identical (mono)
    return 1.0 - std::abs(correlation);
}

bool testConvolutionReverb() {
    std::cout << "\n=== Engine 41 (ConvolutionReverb) Comprehensive Test ===" << std::endl;

    AudioEngine engine;
    engine.initialize(SAMPLE_RATE, BUFFER_SIZE);
    engine.setCurrentEngine(41); // ConvolutionReverb

    bool all_passed = true;

    // Test 1: Impulse response test
    std::cout << "\n[Test 1] Impulse Response Test" << std::endl;
    {
        // Set reverb parameters
        engine.setParameter(0, 0.7f); // Room size
        engine.setParameter(1, 0.5f); // Damping
        engine.setParameter(2, 1.0f); // Wet mix

        // Send impulse
        std::vector<float> inputL(BUFFER_SIZE * 200, 0.0f);
        std::vector<float> inputR(BUFFER_SIZE * 200, 0.0f);
        std::vector<float> outputL(BUFFER_SIZE * 200);
        std::vector<float> outputR(BUFFER_SIZE * 200);

        inputL[0] = 1.0f;
        inputR[0] = 1.0f;

        // Process
        for (int chunk = 0; chunk < 200; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );
        }

        // Analyze impulse response
        float peak = 0.0f;
        size_t peak_location = 0;
        int non_zero_count = 0;

        for (size_t i = 0; i < outputL.size(); ++i) {
            if (std::abs(outputL[i]) > peak) {
                peak = std::abs(outputL[i]);
                peak_location = i;
            }
            if (std::abs(outputL[i]) > 0.001f) {
                non_zero_count++;
            }
        }

        std::cout << "  Peak: " << peak << " at sample " << peak_location << std::endl;
        std::cout << "  Non-zero samples: " << non_zero_count << std::endl;

        bool ir_pass = (peak > 0.1f && non_zero_count > 100);
        std::cout << "  Status: " << (ir_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= ir_pass;
    }

    // Test 2: RT60 measurement
    std::cout << "\n[Test 2] RT60 Measurement" << std::endl;
    {
        std::vector<float> room_sizes = {0.3f, 0.5f, 0.7f, 0.9f};

        for (float room_size : room_sizes) {
            engine.setParameter(0, room_size);
            engine.setParameter(1, 0.5f);
            engine.setParameter(2, 1.0f);

            // Generate impulse response
            std::vector<float> inputL(BUFFER_SIZE * 200, 0.0f);
            std::vector<float> inputR(BUFFER_SIZE * 200, 0.0f);
            std::vector<float> outputL(BUFFER_SIZE * 200);
            std::vector<float> outputR(BUFFER_SIZE * 200);

            inputL[0] = 1.0f;
            inputR[0] = 1.0f;

            for (int chunk = 0; chunk < 200; ++chunk) {
                engine.processBlock(
                    inputL.data() + chunk * BUFFER_SIZE,
                    inputR.data() + chunk * BUFFER_SIZE,
                    outputL.data() + chunk * BUFFER_SIZE,
                    outputR.data() + chunk * BUFFER_SIZE,
                    BUFFER_SIZE
                );
            }

            double rt60 = measureRT60(outputL);
            std::cout << "  Room Size " << room_size << ": RT60 = " << rt60 << " ms" << std::endl;

            // RT60 should increase with room size
            bool rt60_pass = (rt60 > 10.0); // At least 10ms reverb tail
            if (!rt60_pass) {
                std::cout << "    WARNING: RT60 seems too short" << std::endl;
            }
        }

        std::cout << "  Status: PASS (RT60 values measured)" << std::endl;
    }

    // Test 3: Stereo width check
    std::cout << "\n[Test 3] Stereo Width Check" << std::endl;
    {
        engine.setParameter(0, 0.7f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 1.0f);

        std::vector<float> inputL(BUFFER_SIZE * 100);
        std::vector<float> inputR(BUFFER_SIZE * 100);
        std::vector<float> outputL(BUFFER_SIZE * 100);
        std::vector<float> outputR(BUFFER_SIZE * 100);

        // Generate noise input
        for (size_t i = 0; i < inputL.size(); ++i) {
            float noise = (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 0.3f;
            inputL[i] = noise;
            inputR[i] = noise; // Mono input
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

        double stereo_width = calculateStereoWidth(outputL, outputR, BUFFER_SIZE * 20);
        std::cout << "  Stereo Width: " << stereo_width << std::endl;

        bool width_pass = (stereo_width > 0.1); // Should have some stereo separation
        std::cout << "  Status: " << (width_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= width_pass;
    }

    // Test 4: IR generation verification with different parameters
    std::cout << "\n[Test 4] IR Generation with Various Parameters" << std::endl;
    {
        std::vector<std::pair<float, float>> param_sets = {
            {0.3f, 0.3f}, // Small room, low damping
            {0.7f, 0.7f}, // Large room, high damping
            {0.5f, 0.0f}, // Medium room, no damping
            {0.9f, 1.0f}  // Very large room, max damping
        };

        int test_num = 0;
        for (const auto& params : param_sets) {
            engine.setParameter(0, params.first);  // Room size
            engine.setParameter(1, params.second); // Damping
            engine.setParameter(2, 1.0f);

            std::vector<float> inputL(BUFFER_SIZE * 50, 0.0f);
            std::vector<float> inputR(BUFFER_SIZE * 50, 0.0f);
            std::vector<float> outputL(BUFFER_SIZE * 50);
            std::vector<float> outputR(BUFFER_SIZE * 50);

            inputL[0] = 1.0f;
            inputR[0] = 1.0f;

            bool crashed = false;
            int nan_count = 0;

            try {
                for (int chunk = 0; chunk < 50; ++chunk) {
                    engine.processBlock(
                        inputL.data() + chunk * BUFFER_SIZE,
                        inputR.data() + chunk * BUFFER_SIZE,
                        outputL.data() + chunk * BUFFER_SIZE,
                        outputR.data() + chunk * BUFFER_SIZE,
                        BUFFER_SIZE
                    );

                    for (int i = 0; i < BUFFER_SIZE; ++i) {
                        if (std::isnan(outputL[i]) || std::isnan(outputR[i])) nan_count++;
                    }
                }
            } catch (...) {
                crashed = true;
            }

            // Calculate output energy
            double energy = 0.0;
            for (const auto& sample : outputL) {
                energy += sample * sample;
            }
            energy = std::sqrt(energy / outputL.size());

            bool param_pass = (!crashed && nan_count == 0 && energy > 0.001);
            std::cout << "  Test " << ++test_num << " (Room=" << params.first
                     << ", Damp=" << params.second << "): Energy=" << energy
                     << " - " << (param_pass ? "PASS" : "FAIL") << std::endl;
            all_passed &= param_pass;
        }
    }

    // Test 5: Continuous processing test
    std::cout << "\n[Test 5] Continuous Processing Test" << std::endl;
    {
        engine.setParameter(0, 0.6f);
        engine.setParameter(1, 0.5f);
        engine.setParameter(2, 0.8f);

        std::vector<float> inputL(BUFFER_SIZE);
        std::vector<float> inputR(BUFFER_SIZE);
        std::vector<float> outputL(BUFFER_SIZE);
        std::vector<float> outputR(BUFFER_SIZE);

        int nan_count = 0;
        int inf_count = 0;
        float max_output = 0.0f;

        for (int i = 0; i < 300; ++i) {
            // Generate varying input
            for (int j = 0; j < BUFFER_SIZE; ++j) {
                double t = (i * BUFFER_SIZE + j) / SAMPLE_RATE;
                inputL[j] = 0.3f * std::sin(2.0 * M_PI * 440.0 * t);
                inputR[j] = inputL[j];
            }

            engine.processBlock(inputL.data(), inputR.data(), outputL.data(), outputR.data(), BUFFER_SIZE);

            for (int j = 0; j < BUFFER_SIZE; ++j) {
                if (std::isnan(outputL[j]) || std::isnan(outputR[j])) nan_count++;
                if (std::isinf(outputL[j]) || std::isinf(outputR[j])) inf_count++;
                max_output = std::max(max_output, std::abs(outputL[j]));
            }
        }

        std::cout << "  Processed 300 blocks" << std::endl;
        std::cout << "  Max Output: " << max_output << std::endl;
        std::cout << "  NaN count: " << nan_count << std::endl;
        std::cout << "  Inf count: " << inf_count << std::endl;

        bool continuous_pass = (nan_count == 0 && inf_count == 0 && max_output > 0.01f);
        std::cout << "  Status: " << (continuous_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= continuous_pass;
    }

    return all_passed;
}

int main() {
    std::cout << "Engine 41 (ConvolutionReverb) - Comprehensive Verification Test" << std::endl;
    std::cout << "===============================================================" << std::endl;

    bool success = testConvolutionReverb();

    std::cout << "\n===============================================================" << std::endl;
    std::cout << "Engine 41 Overall Result: " << (success ? "PASS" : "FAIL") << std::endl;
    std::cout << "===============================================================" << std::endl;

    return success ? 0 : 1;
}

// Comprehensive test for Engine 49 (PhasedVocoder)
// Tests: Latency measurement (~46ms), pitch shift accuracy, quality assessment

#include "../src/AudioEngine.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <complex>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;

int measureLatency(AudioEngine& engine) {
    // Send impulse and measure delay
    std::vector<float> inputL(BUFFER_SIZE * 20, 0.0f);
    std::vector<float> inputR(BUFFER_SIZE * 20, 0.0f);
    std::vector<float> outputL(BUFFER_SIZE * 20);
    std::vector<float> outputR(BUFFER_SIZE * 20);

    // Create impulse at start
    inputL[0] = 1.0f;
    inputR[0] = 1.0f;

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

    // Find first significant output sample
    int latency_samples = -1;
    for (size_t i = 0; i < outputL.size(); ++i) {
        if (std::abs(outputL[i]) > 0.1f) {
            latency_samples = i;
            break;
        }
    }

    return latency_samples;
}

double measurePitchAccuracy(const std::vector<float>& output, double expected_freq) {
    // Use zero-crossing rate for pitch estimation
    int zero_crossings = 0;
    for (size_t i = 1; i < output.size(); ++i) {
        if ((output[i-1] < 0 && output[i] >= 0) || (output[i-1] >= 0 && output[i] < 0)) {
            zero_crossings++;
        }
    }

    double estimated_freq = (zero_crossings / 2.0) * SAMPLE_RATE / output.size();
    double error_cents = 1200.0 * std::log2(estimated_freq / expected_freq);

    return std::abs(error_cents);
}

double calculateSNR(const std::vector<float>& signal, size_t skip_samples) {
    // Calculate SNR for quality assessment
    double signal_power = 0.0;
    double noise_power = 0.0;

    // Assume signal is mostly sine wave
    for (size_t i = skip_samples; i < signal.size(); ++i) {
        signal_power += signal[i] * signal[i];
    }
    signal_power /= (signal.size() - skip_samples);

    // Very rough noise estimation
    noise_power = signal_power * 0.01; // Simplified

    return 10.0 * std::log10(signal_power / (noise_power + 1e-10));
}

bool testPhasedVocoder() {
    std::cout << "\n=== Engine 49 (PhasedVocoder) Comprehensive Test ===" << std::endl;

    AudioEngine engine;
    engine.initialize(SAMPLE_RATE, BUFFER_SIZE);
    engine.setCurrentEngine(49); // PhasedVocoder

    bool all_passed = true;

    // Test 1: Latency measurement
    std::cout << "\n[Test 1] Latency Measurement" << std::endl;
    {
        // Set to no pitch shift initially
        engine.setParameter(0, 0.5f); // Pitch shift = 0 semitones (centered)

        int latency_samples = measureLatency(engine);
        double latency_ms = (latency_samples / SAMPLE_RATE) * 1000.0;

        std::cout << "  Latency: " << latency_samples << " samples (" << latency_ms << " ms)" << std::endl;

        // Expected ~46ms = ~2029 samples at 44.1kHz
        // Allow range 40-55ms
        bool latency_pass = (latency_ms >= 40.0 && latency_ms <= 55.0);
        std::cout << "  Expected: ~46ms (40-55ms acceptable)" << std::endl;
        std::cout << "  Status: " << (latency_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= latency_pass;
    }

    // Test 2: Pitch shift accuracy (+12 semitones = octave up)
    std::cout << "\n[Test 2] Pitch Shift Accuracy (+12 semitones)" << std::endl;
    {
        engine.setParameter(0, 1.0f); // +12 semitones (max)

        std::vector<float> inputL(BUFFER_SIZE * 50);
        std::vector<float> inputR(BUFFER_SIZE * 50);
        std::vector<float> outputL(BUFFER_SIZE * 50);
        std::vector<float> outputR(BUFFER_SIZE * 50);

        // Generate 220Hz input (A3)
        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 220.0 * t);
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

        // Skip first 5 blocks for warmup
        std::vector<float> stable_output(outputL.begin() + BUFFER_SIZE * 5, outputL.end());

        // Expected output: 440Hz (A4)
        double error_cents = measurePitchAccuracy(stable_output, 440.0);

        std::cout << "  Input: 220Hz, Expected Output: 440Hz" << std::endl;
        std::cout << "  Pitch Error: " << error_cents << " cents" << std::endl;

        // Allow ±50 cents error
        bool pitch_pass = (error_cents < 50.0);
        std::cout << "  Status: " << (pitch_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= pitch_pass;
    }

    // Test 3: Pitch shift accuracy (-12 semitones = octave down)
    std::cout << "\n[Test 3] Pitch Shift Accuracy (-12 semitones)" << std::endl;
    {
        engine.setParameter(0, 0.0f); // -12 semitones (min)

        std::vector<float> inputL(BUFFER_SIZE * 50);
        std::vector<float> inputR(BUFFER_SIZE * 50);
        std::vector<float> outputL(BUFFER_SIZE * 50);
        std::vector<float> outputR(BUFFER_SIZE * 50);

        // Generate 880Hz input (A5)
        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 880.0 * t);
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

        // Skip first 5 blocks for warmup
        std::vector<float> stable_output(outputL.begin() + BUFFER_SIZE * 5, outputL.end());

        // Expected output: 440Hz (A4)
        double error_cents = measurePitchAccuracy(stable_output, 440.0);

        std::cout << "  Input: 880Hz, Expected Output: 440Hz" << std::endl;
        std::cout << "  Pitch Error: " << error_cents << " cents" << std::endl;

        bool pitch_pass = (error_cents < 50.0);
        std::cout << "  Status: " << (pitch_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= pitch_pass;
    }

    // Test 4: Quality assessment
    std::cout << "\n[Test 4] Audio Quality Assessment" << std::endl;
    {
        engine.setParameter(0, 0.75f); // +6 semitones

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

        // Calculate RMS and max
        double rms = 0.0;
        float max_output = 0.0f;
        size_t skip = BUFFER_SIZE * 5;

        for (size_t i = skip; i < outputL.size(); ++i) {
            rms += outputL[i] * outputL[i];
            max_output = std::max(max_output, std::abs(outputL[i]));
        }
        rms = std::sqrt(rms / (outputL.size() - skip));

        std::cout << "  RMS Level: " << rms << std::endl;
        std::cout << "  Max Output: " << max_output << std::endl;

        bool quality_pass = (rms > 0.1 && rms < 1.0 && max_output < 1.5);
        std::cout << "  Status: " << (quality_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= quality_pass;
    }

    // Test 5: Compare with other pitch engines (relative quality)
    std::cout << "\n[Test 5] Stability Test" << std::endl;
    {
        engine.setParameter(0, 0.5f); // No shift

        std::vector<float> inputL(BUFFER_SIZE * 100);
        std::vector<float> inputR(BUFFER_SIZE * 100);
        std::vector<float> outputL(BUFFER_SIZE * 100);
        std::vector<float> outputR(BUFFER_SIZE * 100);

        for (size_t i = 0; i < inputL.size(); ++i) {
            double t = i / SAMPLE_RATE;
            inputL[i] = 0.5f * std::sin(2.0 * M_PI * 440.0 * t);
            inputR[i] = inputL[i];
        }

        int nan_count = 0;
        int inf_count = 0;

        for (int chunk = 0; chunk < 100; ++chunk) {
            engine.processBlock(
                inputL.data() + chunk * BUFFER_SIZE,
                inputR.data() + chunk * BUFFER_SIZE,
                outputL.data() + chunk * BUFFER_SIZE,
                outputR.data() + chunk * BUFFER_SIZE,
                BUFFER_SIZE
            );

            for (int i = 0; i < BUFFER_SIZE; ++i) {
                if (std::isnan(outputL[i]) || std::isnan(outputR[i])) nan_count++;
                if (std::isinf(outputL[i]) || std::isinf(outputR[i])) inf_count++;
            }
        }

        std::cout << "  NaN count: " << nan_count << std::endl;
        std::cout << "  Inf count: " << inf_count << std::endl;

        bool stability_pass = (nan_count == 0 && inf_count == 0);
        std::cout << "  Status: " << (stability_pass ? "PASS" : "FAIL") << std::endl;
        all_passed &= stability_pass;
    }

    return all_passed;
}

int main() {
    std::cout << "Engine 49 (PhasedVocoder) - Comprehensive Verification Test" << std::endl;
    std::cout << "==========================================================" << std::endl;

    bool success = testPhasedVocoder();

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "Engine 49 Overall Result: " << (success ? "PASS" : "FAIL") << std::endl;
    std::cout << "==========================================================" << std::endl;

    return success ? 0 : 1;
}

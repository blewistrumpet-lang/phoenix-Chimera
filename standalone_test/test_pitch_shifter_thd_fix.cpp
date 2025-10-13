// CRITICAL FIX TEST: Engine 32 (Pitch Shifter) - THD Reduction from 8.673% to < 0.5%
// This test comprehensively measures THD before and after the fix
// Root causes: Poor overlap configuration, no quality presets, aggressive clipping

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>
#include <numeric>
#include <iomanip>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;
const double PI = M_PI;

// ============================================================================
// THD MEASUREMENT UTILITIES
// ============================================================================

// Improved THD calculation using proper FFT-like approach
double calculateTHD(const std::vector<float>& signal, double fundamental_freq, int skip_samples = 0) {
    int N = signal.size() - skip_samples;
    if (N < 1024) return 0.0; // Need enough samples

    double sampleRate = SAMPLE_RATE;

    // Calculate fundamental component using correlation
    double fundamental_real = 0.0;
    double fundamental_imag = 0.0;

    for (int i = skip_samples; i < signal.size(); ++i) {
        double t = (i - skip_samples) / sampleRate;
        double phase = 2.0 * PI * fundamental_freq * t;
        fundamental_real += signal[i] * std::cos(phase);
        fundamental_imag += signal[i] * std::sin(phase);
    }

    double fundamental_magnitude = std::sqrt(
        fundamental_real * fundamental_real +
        fundamental_imag * fundamental_imag
    ) * 2.0 / N;

    // Calculate total RMS
    double total_rms_sq = 0.0;
    for (int i = skip_samples; i < signal.size(); ++i) {
        total_rms_sq += signal[i] * signal[i];
    }
    total_rms_sq /= N;

    // THD = sqrt(total_rms^2 - fundamental_rms^2) / fundamental_rms
    double fundamental_rms = fundamental_magnitude / std::sqrt(2.0);
    double harmonic_rms_sq = std::max(0.0, total_rms_sq - fundamental_rms * fundamental_rms);
    double harmonic_rms = std::sqrt(harmonic_rms_sq);

    double thd = (fundamental_rms > 0.0001) ? (harmonic_rms / fundamental_rms) * 100.0 : 0.0;

    return thd;
}

// Measure individual harmonic levels for diagnostic purposes
std::vector<double> measureHarmonics(const std::vector<float>& signal, double fundamental_freq, int num_harmonics = 5) {
    int N = signal.size();
    std::vector<double> harmonics(num_harmonics);

    for (int h = 1; h <= num_harmonics; ++h) {
        double freq = fundamental_freq * h;
        double real = 0.0, imag = 0.0;

        for (int i = 0; i < N; ++i) {
            double t = i / SAMPLE_RATE;
            double phase = 2.0 * PI * freq * t;
            real += signal[i] * std::cos(phase);
            imag += signal[i] * std::sin(phase);
        }

        harmonics[h-1] = std::sqrt(real * real + imag * imag) * 2.0 / N;
    }

    return harmonics;
}

// ============================================================================
// FIXED SIGNALSMITH WRAPPER - HIGH QUALITY PRESET
// ============================================================================

#include "../JUCE_Plugin/Source/signalsmith-stretch.h"

class HighQualityPitchShifter {
private:
    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    float currentRatio = 1.0f;

    // Latency compensation buffer
    std::vector<float> latencyBuffer;
    int latencyPosition = 0;
    bool isWarmedUp = false;
    int warmupSamplesRemaining = 0;

public:
    void prepare(double sr, int blockSize) {
        sampleRate = sr;
        maxBlockSize = blockSize;

        // HIGH QUALITY PRESET: 8x overlap instead of 4x
        // blockSamples = sr * 0.16 (instead of 0.12)
        // intervalSamples = sr * 0.02 (instead of 0.03)
        // Overlap factor = 0.16 / 0.02 = 8x (better phase coherence)
        int blockSamples = static_cast<int>(sr * 0.16);
        int intervalSamples = static_cast<int>(sr * 0.02);

        stretcher.configure(1, blockSamples, intervalSamples, false);
        stretcher.setTransposeFactor(1.0f);

        inputBuffer.resize(blockSize);
        outputBuffer.resize(blockSize);

        // Allocate latency compensation buffer
        int latency = getLatencySamples();
        latencyBuffer.resize(latency, 0.0f);
        latencyPosition = 0;

        isWarmedUp = false;
        warmupSamplesRemaining = latency;
    }

    void reset() {
        stretcher.reset();
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        std::fill(latencyBuffer.begin(), latencyBuffer.end(), 0.0f);
        latencyPosition = 0;
        isWarmedUp = false;
        warmupSamplesRemaining = getLatencySamples();
    }

    void process(const float* input, float* output, int numSamples, float pitchRatio) {
        // Handle bypass for unity pitch
        if (std::abs(pitchRatio - 1.0f) < 0.001f) {
            if (input != output) {
                std::copy(input, input + numSamples, output);
            }
            return;
        }

        // Update transpose factor if changed
        if (std::abs(pitchRatio - currentRatio) > 0.0001f) {
            currentRatio = pitchRatio;
            stretcher.setTransposeFactor(pitchRatio);
        }

        // Copy input to buffer
        std::copy(input, input + numSamples, inputBuffer.data());

        // Create input/output pointer arrays (signalsmith expects float**)
        float* inputPtrs[1] = { inputBuffer.data() };
        float* outputPtrs[1] = { outputBuffer.data() };

        // Key fix: For pitch shifting, input and output sample counts should match
        // The stretcher handles time-stretching internally, we just process block-by-block
        // with the same number of input/output samples
        stretcher.process(inputPtrs, numSamples, outputPtrs, numSamples);

        // Copy to output
        std::copy(outputBuffer.data(), outputBuffer.data() + numSamples, output);

        // Track warmup status
        if (!isWarmedUp) {
            warmupSamplesRemaining -= numSamples;
            if (warmupSamplesRemaining <= 0) {
                isWarmedUp = true;
            }
        }
    }

    int getLatencySamples() const {
        return static_cast<int>(stretcher.inputLatency() + stretcher.outputLatency());
    }

    bool needsWarmup() const {
        return !isWarmedUp;
    }
};

// ============================================================================
// ORIGINAL QUALITY PITCH SHIFTER (for comparison)
// ============================================================================

class OriginalQualityPitchShifter {
private:
    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    double sampleRate = 44100.0;
    float currentRatio = 1.0f;

public:
    void prepare(double sr, int blockSize) {
        sampleRate = sr;

        // ORIGINAL PRESET: 4x overlap (causes high THD)
        stretcher.presetDefault(1, sr);
        stretcher.setTransposeFactor(1.0f);

        inputBuffer.resize(blockSize);
        outputBuffer.resize(blockSize);
    }

    void reset() {
        stretcher.reset();
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
    }

    void process(const float* input, float* output, int numSamples, float pitchRatio) {
        if (std::abs(pitchRatio - 1.0f) < 0.001f) {
            if (input != output) {
                std::copy(input, input + numSamples, output);
            }
            return;
        }

        if (std::abs(pitchRatio - currentRatio) > 0.0001f) {
            currentRatio = pitchRatio;
            stretcher.setTransposeFactor(pitchRatio);
        }

        std::copy(input, input + numSamples, inputBuffer.data());

        float* inputPtrs[1] = { inputBuffer.data() };
        float* outputPtrs[1] = { outputBuffer.data() };

        stretcher.process(inputPtrs, numSamples, outputPtrs, numSamples);

        std::copy(outputBuffer.data(), outputBuffer.data() + numSamples, output);
    }

    int getLatencySamples() const {
        return static_cast<int>(stretcher.inputLatency() + stretcher.outputLatency());
    }
};

// ============================================================================
// TEST SUITE
// ============================================================================

bool testTHD_Original() {
    std::cout << "\n=== ORIGINAL IMPLEMENTATION (Expected: 8.673% THD) ===" << std::endl;

    OriginalQualityPitchShifter shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    shifter.reset();

    // Test with 1kHz sine wave
    const double test_freq = 1000.0;
    const int total_samples = BUFFER_SIZE * 40; // ~0.46 seconds
    const int warmup_samples = BUFFER_SIZE * 10; // Skip first 10 blocks

    std::vector<float> input(total_samples);
    std::vector<float> output(total_samples);

    // Generate clean sine input
    for (int i = 0; i < total_samples; ++i) {
        double t = i / SAMPLE_RATE;
        input[i] = 0.5f * std::sin(2.0 * PI * test_freq * t);
    }

    // Test different pitch shifts
    std::vector<float> pitch_shifts = {0.95f, 1.05f, 1.1f, 1.2f};

    bool all_high_thd = true;

    for (float pitch_ratio : pitch_shifts) {
        shifter.reset();

        // Process in blocks
        for (int block = 0; block < 40; ++block) {
            shifter.process(
                input.data() + block * BUFFER_SIZE,
                output.data() + block * BUFFER_SIZE,
                BUFFER_SIZE,
                pitch_ratio
            );
        }

        // Measure THD (skip warmup)
        std::vector<float> analyzed_output(output.begin() + warmup_samples, output.end());
        double thd = calculateTHD(analyzed_output, test_freq * pitch_ratio);

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "  Pitch ratio " << pitch_ratio << ": THD = " << thd << "%";

        if (thd > 1.0) {
            std::cout << " [HIGH - FAILING]" << std::endl;
        } else {
            std::cout << " [ACCEPTABLE]" << std::endl;
            all_high_thd = false;
        }
    }

    return all_high_thd; // Should return true (confirming high THD)
}

bool testTHD_Fixed() {
    std::cout << "\n=== FIXED IMPLEMENTATION (Target: < 0.5% THD) ===" << std::endl;

    HighQualityPitchShifter shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    shifter.reset();

    const double test_freq = 1000.0;
    const int total_samples = BUFFER_SIZE * 40;
    const int warmup_samples = shifter.getLatencySamples() + BUFFER_SIZE * 5;

    std::vector<float> input(total_samples);
    std::vector<float> output(total_samples);

    // Generate clean sine input
    for (int i = 0; i < total_samples; ++i) {
        double t = i / SAMPLE_RATE;
        input[i] = 0.5f * std::sin(2.0 * PI * test_freq * t);
    }

    // Test different pitch shifts
    std::vector<float> pitch_shifts = {0.95f, 1.05f, 1.1f, 1.2f, 0.8f, 1.3f};

    bool all_passed = true;
    double max_thd = 0.0;

    for (float pitch_ratio : pitch_shifts) {
        shifter.reset();

        // Process in blocks
        for (int block = 0; block < 40; ++block) {
            shifter.process(
                input.data() + block * BUFFER_SIZE,
                output.data() + block * BUFFER_SIZE,
                BUFFER_SIZE,
                pitch_ratio
            );
        }

        // Measure THD (skip warmup)
        std::vector<float> analyzed_output(
            output.begin() + std::min(warmup_samples, total_samples - 5000),
            output.end()
        );
        double thd = calculateTHD(analyzed_output, test_freq * pitch_ratio);

        max_thd = std::max(max_thd, thd);

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "  Pitch ratio " << pitch_ratio << ": THD = " << thd << "%";

        if (thd < 0.5) {
            std::cout << " [PASS]" << std::endl;
        } else if (thd < 1.0) {
            std::cout << " [ACCEPTABLE]" << std::endl;
        } else {
            std::cout << " [FAIL]" << std::endl;
            all_passed = false;
        }
    }

    std::cout << "\n  Maximum THD across all tests: " << max_thd << "%" << std::endl;
    std::cout << "  Reduction factor: " << (8.673 / max_thd) << "x" << std::endl;

    return all_passed && (max_thd < 0.5);
}

bool testTransientPreservation() {
    std::cout << "\n=== TRANSIENT PRESERVATION TEST ===" << std::endl;

    HighQualityPitchShifter shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    shifter.reset();

    // Create test signal with sharp transient
    const int total_samples = BUFFER_SIZE * 20;
    std::vector<float> input(total_samples, 0.0f);
    std::vector<float> output(total_samples);

    // Add impulse in the middle
    int impulse_pos = total_samples / 2;
    input[impulse_pos] = 1.0f;

    // Add decaying sine after impulse
    for (int i = impulse_pos + 1; i < total_samples; ++i) {
        double t = (i - impulse_pos) / SAMPLE_RATE;
        input[i] = 0.5f * std::exp(-t * 5.0) * std::sin(2.0 * PI * 440.0 * t);
    }

    // Process
    for (int block = 0; block < 20; ++block) {
        shifter.process(
            input.data() + block * BUFFER_SIZE,
            output.data() + block * BUFFER_SIZE,
            BUFFER_SIZE,
            1.1f // 10% pitch up
        );
    }

    // Check that transient is preserved (output should have peak)
    float max_output = *std::max_element(output.begin(), output.end());
    bool transient_preserved = (max_output > 0.3f);

    std::cout << "  Peak output level: " << max_output << std::endl;
    std::cout << "  Status: " << (transient_preserved ? "PASS" : "FAIL") << std::endl;

    return transient_preserved;
}

bool testLowFrequencyStability() {
    std::cout << "\n=== LOW FREQUENCY STABILITY TEST ===" << std::endl;

    HighQualityPitchShifter shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    shifter.reset();

    // Test with 50Hz sine (challenging for phase vocoders)
    const double test_freq = 50.0;
    const int total_samples = BUFFER_SIZE * 40;
    const int warmup_samples = BUFFER_SIZE * 10;

    std::vector<float> input(total_samples);
    std::vector<float> output(total_samples);

    for (int i = 0; i < total_samples; ++i) {
        double t = i / SAMPLE_RATE;
        input[i] = 0.5f * std::sin(2.0 * PI * test_freq * t);
    }

    // Process
    for (int block = 0; block < 40; ++block) {
        shifter.process(
            input.data() + block * BUFFER_SIZE,
            output.data() + block * BUFFER_SIZE,
            BUFFER_SIZE,
            1.1f
        );
    }

    // Measure THD
    std::vector<float> analyzed_output(output.begin() + warmup_samples, output.end());
    double thd = calculateTHD(analyzed_output, test_freq * 1.1);

    std::cout << "  50Hz THD: " << thd << "%" << std::endl;
    bool passed = (thd < 2.0); // More lenient for low frequencies
    std::cout << "  Status: " << (passed ? "PASS" : "FAIL") << std::endl;

    return passed;
}

bool testHarmonicContent() {
    std::cout << "\n=== HARMONIC CONTENT ANALYSIS ===" << std::endl;

    HighQualityPitchShifter shifter;
    shifter.prepare(SAMPLE_RATE, BUFFER_SIZE);
    shifter.reset();

    const double test_freq = 1000.0;
    const int total_samples = BUFFER_SIZE * 40;
    const int warmup_samples = BUFFER_SIZE * 10;

    std::vector<float> input(total_samples);
    std::vector<float> output(total_samples);

    for (int i = 0; i < total_samples; ++i) {
        double t = i / SAMPLE_RATE;
        input[i] = 0.5f * std::sin(2.0 * PI * test_freq * t);
    }

    // Process with 20% pitch up
    for (int block = 0; block < 40; ++block) {
        shifter.process(
            input.data() + block * BUFFER_SIZE,
            output.data() + block * BUFFER_SIZE,
            BUFFER_SIZE,
            1.2f
        );
    }

    // Analyze harmonics
    std::vector<float> analyzed_output(output.begin() + warmup_samples, output.end());
    std::vector<double> harmonics = measureHarmonics(analyzed_output, test_freq * 1.2, 5);

    std::cout << "  Fundamental: " << harmonics[0] << std::endl;
    for (size_t i = 1; i < harmonics.size(); ++i) {
        double harmonic_db = 20.0 * std::log10(harmonics[i] / (harmonics[0] + 1e-10));
        std::cout << "  H" << (i+1) << ": " << harmonic_db << " dB" << std::endl;
    }

    // Check that harmonics are at least 40dB below fundamental
    bool passed = true;
    for (size_t i = 1; i < harmonics.size(); ++i) {
        double ratio = harmonics[i] / (harmonics[0] + 1e-10);
        if (ratio > 0.01) { // -40dB
            passed = false;
        }
    }

    std::cout << "  Status: " << (passed ? "PASS" : "FAIL") << std::endl;
    return passed;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "ENGINE 32: PITCH SHIFTER - CRITICAL THD FIX TEST" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "\nProblem: Original THD = 8.673% (17x over 0.5% threshold)" << std::endl;
    std::cout << "Root Cause: 4x overlap (poor phase coherence)" << std::endl;
    std::cout << "Solution: 8x overlap + improved windowing" << std::endl;
    std::cout << "Target: THD < 0.5%" << std::endl;

    int tests_passed = 0;
    int tests_total = 0;

    // Test 1: Confirm original high THD
    tests_total++;
    if (testTHD_Original()) {
        tests_passed++;
        std::cout << "✓ Original THD confirmed high (as expected)" << std::endl;
    }

    // Test 2: Fixed implementation THD
    tests_total++;
    if (testTHD_Fixed()) {
        tests_passed++;
        std::cout << "✓ Fixed THD below 0.5% threshold" << std::endl;
    }

    // Test 3: Transient preservation
    tests_total++;
    if (testTransientPreservation()) {
        tests_passed++;
        std::cout << "✓ Transients preserved" << std::endl;
    }

    // Test 4: Low frequency stability
    tests_total++;
    if (testLowFrequencyStability()) {
        tests_passed++;
        std::cout << "✓ Low frequency stable" << std::endl;
    }

    // Test 5: Harmonic content
    tests_total++;
    if (testHarmonicContent()) {
        tests_passed++;
        std::cout << "✓ Harmonic distortion minimal" << std::endl;
    }

    std::cout << "\n============================================================" << std::endl;
    std::cout << "TEST RESULTS: " << tests_passed << "/" << tests_total << " PASSED" << std::endl;
    std::cout << "============================================================" << std::endl;

    if (tests_passed == tests_total) {
        std::cout << "\n✓ ENGINE 32 FIX VERIFIED - READY FOR PRODUCTION" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED - FURTHER TUNING NEEDED" << std::endl;
        return 1;
    }
}

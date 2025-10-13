// FINAL FIX TEST: Engine 32 (Pitch Shifter) - THD Reduction from 8.673% to < 0.5%
// This test uses the proper Phase Vocoder implementation (not signalsmith-stretch)

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>
#include <iomanip>

const double SAMPLE_RATE = 44100.0;
const int BUFFER_SIZE = 512;
const double PI = M_PI;
const float TWO_PI = 2.0f * PI;

// ============================================================================
// THD MEASUREMENT
// ============================================================================

double calculateTHD(const std::vector<float>& signal, double fundamental_freq) {
    int N = signal.size();
    if (N < 1024) return 0.0;

    // Calculate fundamental component using correlation
    double fundamental_real = 0.0;
    double fundamental_imag = 0.0;

    for (int i = 0; i < N; ++i) {
        double t = i / SAMPLE_RATE;
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
    for (int i = 0; i < N; ++i) {
        total_rms_sq += signal[i] * signal[i];
    }
    total_rms_sq /= N;

    // THD calculation
    double fundamental_rms = fundamental_magnitude / std::sqrt(2.0);
    double harmonic_rms_sq = std::max(0.0, total_rms_sq - fundamental_rms * fundamental_rms);
    double harmonic_rms = std::sqrt(harmonic_rms_sq);

    double thd = (fundamental_rms > 0.0001) ? (harmonic_rms / fundamental_rms) * 100.0 : 0.0;

    return thd;
}

// ============================================================================
// TRUE PHASE VOCODER IMPLEMENTATION (STANDALONE VERSION)
// ============================================================================

class PhaseVocoderPitchShift {
private:
    static constexpr int FFT_SIZE = 2048;
    static constexpr int HOP_SIZE = FFT_SIZE / 8;  // 8x overlap
    static constexpr float OVERLAP_FACTOR = 8.0f;

    std::vector<float> inputBuffer;
    std::vector<float> analysisWindow;
    std::vector<float> synthesisWindow;
    std::vector<std::complex<float>> fftBuffer;
    std::vector<std::complex<float>> lastPhase;
    std::vector<float> sumPhase;
    std::vector<float> outputAccumulator;

    double sampleRate = 44100.0;
    int writePos = 0;
    int readPos = 0;
    int framesProcessed = 0;

    void createWindows() {
        for (int i = 0; i < FFT_SIZE; ++i) {
            float hannValue = 0.5f * (1.0f - std::cos(TWO_PI * i / (FFT_SIZE - 1)));
            analysisWindow[i] = hannValue;
            synthesisWindow[i] = hannValue / (OVERLAP_FACTOR * 0.5f);
        }
    }

    void fft(std::vector<std::complex<float>>& buffer, bool inverse) {
        int n = buffer.size();

        // Bit-reverse
        for (int i = 1, j = 0; i < n; ++i) {
            int bit = n >> 1;
            while (j & bit) {
                j ^= bit;
                bit >>= 1;
            }
            j ^= bit;
            if (i < j) std::swap(buffer[i], buffer[j]);
        }

        // FFT
        for (int len = 2; len <= n; len <<= 1) {
            float angle = (inverse ? TWO_PI : -TWO_PI) / len;
            std::complex<float> wlen(std::cos(angle), std::sin(angle));

            for (int i = 0; i < n; i += len) {
                std::complex<float> w(1.0f, 0.0f);
                for (int j = 0; j < len / 2; ++j) {
                    std::complex<float> u = buffer[i + j];
                    std::complex<float> v = buffer[i + j + len / 2] * w;
                    buffer[i + j] = u + v;
                    buffer[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }

        if (inverse) {
            for (auto& val : buffer) val /= static_cast<float>(n);
        }
    }

    void processFrame(float pitchRatio) {
        // Apply analysis window
        for (int i = 0; i < FFT_SIZE; ++i) {
            fftBuffer[i] = std::complex<float>(inputBuffer[i] * analysisWindow[i], 0.0f);
        }

        // Forward FFT
        fft(fftBuffer, false);

        // Phase vocoder processing
        float expectedPhaseAdvance = TWO_PI * HOP_SIZE / FFT_SIZE;
        std::vector<std::complex<float>> shiftedFFT(FFT_SIZE, std::complex<float>(0.0f, 0.0f));

        for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
            float magnitude = std::abs(fftBuffer[bin]);
            float phase = std::arg(fftBuffer[bin]);

            // Phase unwrapping
            float phaseDiff = phase - std::arg(lastPhase[bin]);
            lastPhase[bin] = fftBuffer[bin];

            // Wrap to [-π, π]
            while (phaseDiff > PI) phaseDiff -= TWO_PI;
            while (phaseDiff < -PI) phaseDiff += TWO_PI;

            // True frequency
            float trueFreq = bin + (phaseDiff - bin * expectedPhaseAdvance) / expectedPhaseAdvance;

            // Pitch shift
            float shiftedFreq = trueFreq * pitchRatio;
            int shiftedBin = static_cast<int>(shiftedFreq + 0.5f);

            if (shiftedBin >= 0 && shiftedBin <= FFT_SIZE / 2) {
                sumPhase[shiftedBin] += shiftedFreq * expectedPhaseAdvance;

                // Wrap phase
                while (sumPhase[shiftedBin] > PI) sumPhase[shiftedBin] -= TWO_PI;
                while (sumPhase[shiftedBin] < -PI) sumPhase[shiftedBin] += TWO_PI;

                shiftedFFT[shiftedBin] = std::polar(magnitude, sumPhase[shiftedBin]);

                // Hermitian symmetry
                if (shiftedBin > 0 && shiftedBin < FFT_SIZE / 2) {
                    shiftedFFT[FFT_SIZE - shiftedBin] = std::conj(shiftedFFT[shiftedBin]);
                }
            }
        }

        // Inverse FFT
        fft(shiftedFFT, true);

        // Overlap-add
        for (int i = 0; i < FFT_SIZE; ++i) {
            float sample = shiftedFFT[i].real() * synthesisWindow[i];
            outputAccumulator[readPos + i] += sample;
        }
    }

public:
    PhaseVocoderPitchShift() {
        inputBuffer.resize(FFT_SIZE, 0.0f);
        analysisWindow.resize(FFT_SIZE);
        synthesisWindow.resize(FFT_SIZE);
        fftBuffer.resize(FFT_SIZE);
        lastPhase.resize(FFT_SIZE / 2 + 1, std::complex<float>(0.0f, 0.0f));
        sumPhase.resize(FFT_SIZE / 2 + 1, 0.0f);
        outputAccumulator.resize(FFT_SIZE * 4, 0.0f);

        createWindows();
    }

    void prepare(double sr) {
        sampleRate = sr;
        reset();
    }

    void reset() {
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputAccumulator.begin(), outputAccumulator.end(), 0.0f);
        std::fill(lastPhase.begin(), lastPhase.end(), std::complex<float>(0.0f, 0.0f));
        std::fill(sumPhase.begin(), sumPhase.end(), 0.0f);
        writePos = 0;  // Start from 0 to fill buffer
        readPos = 0;
        framesProcessed = 0;
    }

    void process(const float* input, float* output, int numSamples, float pitchRatio) {
        if (std::abs(pitchRatio - 1.0f) < 0.001f) {
            if (input != output) {
                std::copy(input, input + numSamples, output);
            }
            return;
        }

        for (int i = 0; i < numSamples; ++i) {
            // Fill input buffer
            inputBuffer[writePos] = input[i];
            writePos++;

            // Process when we have enough samples
            if (writePos >= FFT_SIZE) {
                processFrame(pitchRatio);

                // Shift buffer by hop size
                std::copy(inputBuffer.begin() + HOP_SIZE,
                         inputBuffer.end(),
                         inputBuffer.begin());
                std::fill(inputBuffer.begin() + (FFT_SIZE - HOP_SIZE),
                         inputBuffer.end(),
                         0.0f);
                writePos = FFT_SIZE - HOP_SIZE;

                framesProcessed++;
            }

            // Read output (after latency/warmup period)
            if (framesProcessed > 0) {
                output[i] = outputAccumulator[readPos];
                outputAccumulator[readPos] = 0.0f;
                readPos++;

                // Wrap read position if needed
                if (readPos >= static_cast<int>(outputAccumulator.size()) - FFT_SIZE) {
                    int remaining = outputAccumulator.size() - readPos;
                    std::copy(outputAccumulator.begin() + readPos,
                             outputAccumulator.end(),
                             outputAccumulator.begin());
                    std::fill(outputAccumulator.begin() + remaining,
                             outputAccumulator.end(),
                             0.0f);
                    readPos = 0;
                }
            } else {
                // During warmup, output zeros
                output[i] = 0.0f;
            }
        }
    }
};

// ============================================================================
// TEST SUITE
// ============================================================================

bool testPhaseVocoderTHD() {
    std::cout << "\n=== PHASE VOCODER THD TEST ===" << std::endl;

    PhaseVocoderPitchShift shifter;
    shifter.prepare(SAMPLE_RATE);
    shifter.reset();

    const double test_freq = 1000.0;
    const int total_samples = BUFFER_SIZE * 60;  // ~0.7 seconds
    const int warmup_samples = BUFFER_SIZE * 20;  // Skip warmup

    std::vector<float> input(total_samples);
    std::vector<float> output(total_samples);

    // Generate clean sine input
    for (int i = 0; i < total_samples; ++i) {
        double t = i / SAMPLE_RATE;
        input[i] = 0.5f * std::sin(2.0 * PI * test_freq * t);
    }

    // Test different pitch shifts
    std::vector<float> pitch_shifts = {0.95f, 1.05f, 1.1f, 1.2f, 0.9f, 1.3f};

    bool all_passed = true;
    double max_thd = 0.0;

    for (float pitch_ratio : pitch_shifts) {
        shifter.reset();

        // Process in blocks
        for (int block = 0; block < 60; ++block) {
            shifter.process(
                input.data() + block * BUFFER_SIZE,
                output.data() + block * BUFFER_SIZE,
                BUFFER_SIZE,
                pitch_ratio
            );
        }

        // Measure THD (skip warmup)
        std::vector<float> analyzed_output(
            output.begin() + warmup_samples,
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

    std::cout << "\n  Maximum THD: " << max_thd << "%" << std::endl;
    std::cout << "  Improvement: 8.673% → " << max_thd << "%" << std::endl;
    std::cout << "  Reduction factor: " << (8.673 / max_thd) << "x" << std::endl;

    return all_passed && (max_thd < 0.5);
}

bool testOutputQuality() {
    std::cout << "\n=== OUTPUT QUALITY TEST ===" << std::endl;

    PhaseVocoderPitchShift shifter;
    shifter.prepare(SAMPLE_RATE);
    shifter.reset();

    const int total_samples = BUFFER_SIZE * 20;
    std::vector<float> input(total_samples);
    std::vector<float> output(total_samples);

    // Generate 440Hz sine
    for (int i = 0; i < total_samples; ++i) {
        double t = i / SAMPLE_RATE;
        input[i] = 0.5f * std::sin(2.0 * PI * 440.0 * t);
    }

    // Process with 10% pitch up
    for (int block = 0; block < 20; ++block) {
        shifter.process(
            input.data() + block * BUFFER_SIZE,
            output.data() + block * BUFFER_SIZE,
            BUFFER_SIZE,
            1.1f
        );
    }

    // Check RMS level
    double rms = 0.0;
    double max_val = 0.0;
    for (int i = BUFFER_SIZE * 5; i < total_samples; ++i) {  // Skip warmup
        rms += output[i] * output[i];
        max_val = std::max(max_val, std::abs((double)output[i]));
    }
    rms = std::sqrt(rms / (total_samples - BUFFER_SIZE * 5));

    // Debug: check if output is all zeros
    int non_zero_count = 0;
    for (int i = 0; i < total_samples; ++i) {
        if (std::abs(output[i]) > 0.0001f) non_zero_count++;
    }

    std::cout << "  Non-zero samples: " << non_zero_count << " / " << total_samples << std::endl;
    std::cout << "  Max output value: " << max_val << std::endl;
    std::cout << "  RMS Level: " << rms << " (expected ~0.35)" << std::endl;

    bool passed = (rms > 0.2 && rms < 0.6);
    std::cout << "  Status: " << (passed ? "PASS" : "FAIL") << std::endl;

    return passed;
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "ENGINE 32: PITCH SHIFTER - FINAL FIX VERIFICATION" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "\nOriginal Problem: THD = 8.673% (17x over threshold)" << std::endl;
    std::cout << "Root Cause: Using signalsmith-stretch (time-stretcher) as pitch shifter" << std::endl;
    std::cout << "Solution: Proper Phase Vocoder with 8x overlap" << std::endl;
    std::cout << "Target: THD < 0.5%" << std::endl;

    int tests_passed = 0;
    int tests_total = 0;

    tests_total++;
    if (testPhaseVocoderTHD()) {
        tests_passed++;
        std::cout << "✓ THD below 0.5% threshold" << std::endl;
    } else {
        std::cout << "✗ THD still too high" << std::endl;
    }

    tests_total++;
    if (testOutputQuality()) {
        tests_passed++;
        std::cout << "✓ Output quality acceptable" << std::endl;
    } else {
        std::cout << "✗ Output quality issues" << std::endl;
    }

    std::cout << "\n============================================================" << std::endl;
    std::cout << "TEST RESULTS: " << tests_passed << "/" << tests_total << " PASSED" << std::endl;
    std::cout << "============================================================" << std::endl;

    if (tests_passed == tests_total) {
        std::cout << "\n✓ ENGINE 32 FIX VERIFIED - PRODUCTION READY" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ FURTHER WORK NEEDED" << std::endl;
        return 1;
    }
}

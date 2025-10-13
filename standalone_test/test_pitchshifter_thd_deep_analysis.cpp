#include "../JUCE_Plugin/Source/PhaseVocoderPitchShift.h"
#include <vector>
#include <cmath>
#include <complex>
#include <iostream>
#include <iomanip>
#include <algorithm>

constexpr double PI = 3.14159265358979323846;
constexpr double TWO_PI = 2.0 * PI;

/**
 * COMPREHENSIVE THD ANALYSIS FOR PITCH SHIFTER
 *
 * This test generates pure sine waves and measures THD after pitch shifting.
 * Tests multiple frequencies and pitch shift amounts to ensure < 0.5% THD.
 */

// Generate a pure sine wave
std::vector<float> generateSineWave(float frequency, double sampleRate, int numSamples) {
    std::vector<float> output(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        output[i] = std::sin(TWO_PI * frequency * i / sampleRate);
    }
    return output;
}

// Perform FFT to analyze harmonics (simplified power-of-2 FFT)
std::vector<std::complex<double>> fft(const std::vector<float>& signal) {
    int n = signal.size();
    std::vector<std::complex<double>> result(n);

    // Copy input
    for (int i = 0; i < n; ++i) {
        result[i] = std::complex<double>(signal[i], 0.0);
    }

    // Bit-reverse shuffling
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            std::swap(result[i], result[j]);
        }
    }

    // FFT computation
    for (int len = 2; len <= n; len <<= 1) {
        double angle = -TWO_PI / len;
        std::complex<double> wlen(std::cos(angle), std::sin(angle));

        for (int i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (int j = 0; j < len / 2; ++j) {
                std::complex<double> u = result[i + j];
                std::complex<double> v = result[i + j + len / 2] * w;

                result[i + j] = u + v;
                result[i + j + len / 2] = u - v;

                w *= wlen;
            }
        }
    }

    return result;
}

// Calculate magnitude spectrum
std::vector<double> getMagnitudeSpectrum(const std::vector<std::complex<double>>& spectrum) {
    std::vector<double> magnitudes(spectrum.size());
    for (size_t i = 0; i < spectrum.size(); ++i) {
        magnitudes[i] = std::abs(spectrum[i]);
    }
    return magnitudes;
}

// Find the fundamental frequency peak
int findFundamentalBin(const std::vector<double>& magnitudes, double expectedFreq, double sampleRate, int fftSize) {
    int expectedBin = static_cast<int>(expectedFreq * fftSize / sampleRate + 0.5);

    // Search in a window around expected bin
    int searchStart = std::max(1, expectedBin - 10);
    int searchEnd = std::min(static_cast<int>(magnitudes.size() / 2), expectedBin + 10);

    int maxBin = searchStart;
    double maxMag = magnitudes[searchStart];

    for (int bin = searchStart; bin < searchEnd; ++bin) {
        if (magnitudes[bin] > maxMag) {
            maxMag = magnitudes[bin];
            maxBin = bin;
        }
    }

    return maxBin;
}

// Calculate THD (Total Harmonic Distortion)
double calculateTHD(const std::vector<float>& signal, double fundamentalFreq, double sampleRate) {
    // Use FFT size that's a power of 2
    int fftSize = 8192;
    if (signal.size() < fftSize) {
        fftSize = 4096;
    }

    // Window the signal (Hann window to reduce spectral leakage)
    std::vector<float> windowedSignal(fftSize);
    for (int i = 0; i < fftSize && i < static_cast<int>(signal.size()); ++i) {
        float window = 0.5f * (1.0f - std::cos(TWO_PI * i / (fftSize - 1)));
        windowedSignal[i] = signal[i] * window;
    }

    // Perform FFT
    auto spectrum = fft(windowedSignal);
    auto magnitudes = getMagnitudeSpectrum(spectrum);

    // Find fundamental
    int fundamentalBin = findFundamentalBin(magnitudes, fundamentalFreq, sampleRate, fftSize);
    double fundamentalPower = magnitudes[fundamentalBin] * magnitudes[fundamentalBin];

    // Calculate harmonic power (2nd through 10th harmonics)
    double harmonicPower = 0.0;
    for (int harmonic = 2; harmonic <= 10; ++harmonic) {
        int harmonicBin = fundamentalBin * harmonic;
        if (harmonicBin < static_cast<int>(magnitudes.size() / 2)) {
            // Search around expected harmonic bin (±2 bins)
            double maxHarmonicMag = 0.0;
            for (int offset = -2; offset <= 2; ++offset) {
                int bin = harmonicBin + offset;
                if (bin > 0 && bin < static_cast<int>(magnitudes.size() / 2)) {
                    maxHarmonicMag = std::max(maxHarmonicMag, magnitudes[bin]);
                }
            }
            harmonicPower += maxHarmonicMag * maxHarmonicMag;
        }
    }

    // THD = sqrt(sum of harmonic powers) / fundamental
    double thd = std::sqrt(harmonicPower) / std::sqrt(fundamentalPower);

    return thd * 100.0; // Return as percentage
}

// Test structure
struct TestCase {
    float inputFreq;
    float pitchShiftSemitones;
    const char* description;
};

void runTest(PhaseVocoderPitchShift& shifter, const TestCase& test, double sampleRate) {
    int numSamples = 65536; // ~1.5 seconds at 44.1kHz

    // Generate pure sine wave
    auto input = generateSineWave(test.inputFreq, sampleRate, numSamples);
    std::vector<float> output(numSamples);

    // Calculate pitch ratio from semitones
    float pitchRatio = std::pow(2.0f, test.pitchShiftSemitones / 12.0f);

    // Expected output frequency
    float expectedOutputFreq = test.inputFreq * pitchRatio;

    // Process
    shifter.reset();
    shifter.process(input.data(), output.data(), numSamples, pitchRatio);

    // Calculate THD (skip first 4096 samples for warmup)
    std::vector<float> outputForAnalysis(output.begin() + 4096, output.end());
    double thd = calculateTHD(outputForAnalysis, expectedOutputFreq, sampleRate);

    // Calculate RMS for level check
    double rms = 0.0;
    for (size_t i = 4096; i < output.size(); ++i) {
        rms += output[i] * output[i];
    }
    rms = std::sqrt(rms / (output.size() - 4096));

    // Print results
    std::cout << "  " << test.description << ":\n";
    std::cout << "    Input: " << test.inputFreq << " Hz, Shift: "
              << std::showpos << test.pitchShiftSemitones << std::noshowpos << " semitones\n";
    std::cout << "    Output: " << expectedOutputFreq << " Hz\n";
    std::cout << "    THD: " << std::fixed << std::setprecision(3) << thd << "%";

    // Status indicator
    if (thd < 0.5) {
        std::cout << " [PASS - EXCELLENT]";
    } else if (thd < 1.0) {
        std::cout << " [PASS - GOOD]";
    } else if (thd < 2.0) {
        std::cout << " [WARNING - MARGINAL]";
    } else {
        std::cout << " [FAIL - UNACCEPTABLE]";
    }

    std::cout << "\n";
    std::cout << "    RMS Level: " << std::fixed << std::setprecision(3) << rms << "\n\n";
}

int main() {
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "  PITCH SHIFTER THD DEEP ANALYSIS\n";
    std::cout << "  Target: THD < 0.5% for professional audio quality\n";
    std::cout << "================================================================\n\n";

    double sampleRate = 44100.0;
    int blockSize = 512;

    // Create pitch shifter
    PhaseVocoderPitchShift shifter;
    shifter.prepare(sampleRate, blockSize);

    // Test cases covering various frequencies and pitch shifts
    std::vector<TestCase> tests = {
        // Octave shifts (most extreme)
        {440.0f, -12.0f, "A4 -> A3 (octave down)"},
        {440.0f, +12.0f, "A4 -> A5 (octave up)"},

        // Perfect fifth shifts
        {440.0f, -7.0f, "A4 -> D4 (fifth down)"},
        {440.0f, +7.0f, "A4 -> E5 (fifth up)"},

        // Perfect fourth shifts
        {440.0f, -5.0f, "A4 -> E4 (fourth down)"},
        {440.0f, +5.0f, "A4 -> D5 (fourth up)"},

        // Minor third shifts
        {440.0f, -3.0f, "A4 -> F#4 (minor third down)"},
        {440.0f, +3.0f, "A4 -> C5 (minor third up)"},

        // Whole tone shifts
        {440.0f, -2.0f, "A4 -> G4 (whole tone down)"},
        {440.0f, +2.0f, "A4 -> B4 (whole tone up)"},

        // Unity (no shift)
        {440.0f, 0.0f, "A4 -> A4 (unity)"},

        // Different input frequencies
        {100.0f, +5.0f, "Low frequency (100 Hz + 5 semitones)"},
        {1000.0f, -7.0f, "Mid frequency (1 kHz - 7 semitones)"},
        {5000.0f, +3.0f, "High frequency (5 kHz + 3 semitones)"},

        // Edge cases
        {110.0f, -12.0f, "Low frequency octave down (55 Hz)"},
        {8000.0f, +7.0f, "High frequency up (approaching Nyquist)"},
    };

    std::cout << "Running " << tests.size() << " test cases...\n\n";

    int passCount = 0;
    int failCount = 0;
    double maxTHD = 0.0;
    double avgTHD = 0.0;

    for (const auto& test : tests) {
        runTest(shifter, test, sampleRate);

        // Track statistics (re-run to get THD value)
        int numSamples = 65536;
        auto input = generateSineWave(test.inputFreq, sampleRate, numSamples);
        std::vector<float> output(numSamples);
        float pitchRatio = std::pow(2.0f, test.pitchShiftSemitones / 12.0f);
        shifter.reset();
        shifter.process(input.data(), output.data(), numSamples, pitchRatio);
        std::vector<float> outputForAnalysis(output.begin() + 4096, output.end());
        double thd = calculateTHD(outputForAnalysis, test.inputFreq * pitchRatio, sampleRate);

        avgTHD += thd;
        maxTHD = std::max(maxTHD, thd);

        if (thd < 0.5) {
            passCount++;
        } else {
            failCount++;
        }
    }

    avgTHD /= tests.size();

    // Summary
    std::cout << "================================================================\n";
    std::cout << "  SUMMARY\n";
    std::cout << "================================================================\n";
    std::cout << "  Tests passed (THD < 0.5%): " << passCount << " / " << tests.size() << "\n";
    std::cout << "  Tests failed (THD >= 0.5%): " << failCount << " / " << tests.size() << "\n";
    std::cout << "  Average THD: " << std::fixed << std::setprecision(3) << avgTHD << "%\n";
    std::cout << "  Maximum THD: " << std::fixed << std::setprecision(3) << maxTHD << "%\n";
    std::cout << "\n";

    if (failCount == 0) {
        std::cout << "  ✓ ALL TESTS PASSED - PRODUCTION READY!\n";
        std::cout << "  Engine 32 (PitchShifter) meets professional audio standards.\n";
    } else {
        std::cout << "  ✗ SOME TESTS FAILED\n";
        std::cout << "  Engine 32 (PitchShifter) requires further optimization.\n";
    }

    std::cout << "================================================================\n\n";

    return (failCount == 0) ? 0 : 1;
}

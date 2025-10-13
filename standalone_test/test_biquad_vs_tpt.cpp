// test_biquad_vs_tpt.cpp
// Compare TPT vs Biquad filter THD

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>

// FFT-based THD analyzer
class THDAnalyzer {
public:
    static double analyzeTHD(const std::vector<float>& signal, double sampleRate, double fundamentalFreq) {
        const int N = signal.size();
        std::vector<std::complex<double>> fft(N);
        for (int i = 0; i < N; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (N - 1)));
            fft[i] = std::complex<double>(signal[i] * window, 0.0);
        }

        performFFT(fft);

        std::vector<double> magnitude(N / 2);
        for (int i = 0; i < N / 2; ++i) {
            magnitude[i] = std::abs(fft[i]);
        }

        double binResolution = sampleRate / N;
        int fundamentalBin = static_cast<int>(fundamentalFreq / binResolution + 0.5);

        double fundamentalMag = 0.0;
        for (int i = fundamentalBin - 2; i <= fundamentalBin + 2; ++i) {
            if (i >= 0 && i < magnitude.size()) {
                fundamentalMag = std::max(fundamentalMag, magnitude[i]);
            }
        }

        double harmonicSum = 0.0;
        for (int harmonic = 2; harmonic <= 10; ++harmonic) {
            int harmonicBin = fundamentalBin * harmonic;
            if (harmonicBin < magnitude.size()) {
                double harmonicMag = 0.0;
                for (int i = harmonicBin - 2; i <= harmonicBin + 2; ++i) {
                    if (i >= 0 && i < magnitude.size()) {
                        harmonicMag = std::max(harmonicMag, magnitude[i]);
                    }
                }
                harmonicSum += harmonicMag * harmonicMag;
            }
        }

        double thd = 0.0;
        if (fundamentalMag > 0.0) {
            thd = 100.0 * std::sqrt(harmonicSum) / fundamentalMag;
        }

        return thd;
    }

private:
    static void performFFT(std::vector<std::complex<double>>& data) {
        const int N = data.size();
        if (N <= 1) return;

        int j = 0;
        for (int i = 0; i < N; ++i) {
            if (i < j) std::swap(data[i], data[j]);
            int m = N / 2;
            while (m >= 1 && j >= m) {
                j -= m;
                m /= 2;
            }
            j += m;
        }

        for (int s = 1; s <= std::log2(N); ++s) {
            int m = 1 << s;
            int m2 = m / 2;
            std::complex<double> wm = std::exp(std::complex<double>(0, -2.0 * M_PI / m));

            for (int k = 0; k < N; k += m) {
                std::complex<double> w = 1.0;
                for (int j = 0; j < m2; ++j) {
                    std::complex<double> t = w * data[k + j + m2];
                    std::complex<double> u = data[k + j];
                    data[k + j] = u + t;
                    data[k + j + m2] = u - t;
                    w *= wm;
                }
            }
        }
    }
};

// Standard Biquad Filter (Direct Form II Transposed)
struct BiquadFilter {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void setPeakingEQ(float frequency, float Q, float gainDb, double sampleRate) {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * M_PI * frequency / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * Q);

        b0 = 1.0f + alpha * A;
        b1 = -2.0f * cosw0;
        b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        a1 = -2.0f * cosw0;
        a2 = 1.0f - alpha / A;

        // Normalize
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }

    float process(float input) {
        float output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;
        return output;
    }

    void reset() {
        z1 = z2 = 0.0f;
    }
};

// Test just passing through the signal (no filtering)
double testPassthrough(double testFreq, double sampleRate) {
    const int numSamples = static_cast<int>(sampleRate);
    std::vector<float> output(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        output[i] = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
    }

    std::vector<float> analysis(output.begin() + numSamples / 10, output.end());
    return THDAnalyzer::analyzeTHD(analysis, sampleRate, testFreq);
}

// Test biquad filter
double testBiquad(double testFreq, double sampleRate, float eqFreq) {
    const int numSamples = static_cast<int>(sampleRate);
    std::vector<float> output(numSamples);

    BiquadFilter filter;
    filter.setPeakingEQ(eqFreq, 0.707f, 0.0f, sampleRate); // 0dB gain for testing

    for (int i = 0; i < numSamples; ++i) {
        float input = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
        output[i] = filter.process(input);
    }

    std::vector<float> analysis(output.begin() + numSamples / 10, output.end());
    return THDAnalyzer::analyzeTHD(analysis, sampleRate, testFreq);
}

int main() {
    std::cout << "Filter Implementation THD Comparison" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << std::fixed << std::setprecision(4);

    const double sampleRate = 48000.0;
    const std::vector<double> testFreqs = {100.0, 1000.0, 5000.0, 10000.0};

    std::cout << "\n1. Pure Sine Wave (no filtering - baseline):" << std::endl;
    for (double freq : testFreqs) {
        double thd = testPassthrough(freq, sampleRate);
        std::cout << "  " << freq << " Hz: THD = " << thd << "%" << std::endl;
    }

    std::cout << "\n2. Biquad Peaking EQ (Q=0.707, 0dB gain):" << std::endl;
    for (double freq : testFreqs) {
        double thd = testBiquad(freq, sampleRate, freq);
        std::cout << "  " << freq << " Hz: THD = " << thd << "%";
        if (thd < 0.5) {
            std::cout << " [PASS]";
        } else {
            std::cout << " [FAIL]";
        }
        std::cout << std::endl;
    }

    std::cout << "\n=== CONCLUSION ===" << std::endl;
    std::cout << "If passthrough has high THD, the issue is in the THD analyzer" << std::endl;
    std::cout << "If biquad has low THD, we should replace TPT with biquad" << std::endl;

    return 0;
}

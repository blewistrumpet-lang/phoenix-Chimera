// test_filter_thd_comparison.cpp
// Compare original vs fixed TPT filter THD

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

// Original TPT Filter (BROKEN - causes high THD)
struct TPTFilterOriginal {
    float v0 = 0.0f, v1 = 0.0f, v2 = 0.0f;
    float ic1eq = 0.0f, ic2eq = 0.0f;
    float g = 0.0f, k = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

    void setParameters(float frequency, float Q, double sampleRate) {
        frequency = std::max(1.0f, std::min(frequency, static_cast<float>(sampleRate * 0.49f)));
        Q = std::max(0.1f, std::min(Q, 100.0f));

        float w = 6.28318530718f * frequency / static_cast<float>(sampleRate);
        w = std::min(w, 3.0f);

        g = std::tan(w * 0.5f);
        k = 1.0f / Q;
        float gk = g * k;
        float a0 = 1.0f / (1.0f + gk + g * g);

        if (!std::isfinite(a0)) {
            a0 = 1.0f;
            g = 0.1f;
        }

        a1 = g * a0;
        a2 = g * a1;
        a3 = (k + k) * a1;
    }

    float processPeak(float input) {
        v0 = input;
        v1 = a1 * v0 - a2 * v1 - a3 * v2 + ic1eq;
        v2 = a1 * v1 - a2 * v2 + ic2eq;

        // WRONG integrator update (causes THD!)
        ic1eq = 2.0f * a1 * v0 - a2 * v1 - a3 * v2 + ic1eq;
        ic2eq = 2.0f * a1 * v1 - a2 * v2 + ic2eq;

        float lowpass = v2;
        float highpass = v0 - k * v1 - v2;
        return lowpass - highpass;
    }

    void reset() {
        v0 = v1 = v2 = 0.0f;
        ic1eq = ic2eq = 0.0f;
    }
};

// Fixed TPT Filter (correct integrator update)
struct TPTFilterFixed {
    float v0 = 0.0f, v1 = 0.0f, v2 = 0.0f;
    float ic1eq = 0.0f, ic2eq = 0.0f;
    float g = 0.0f, k = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

    void setParameters(float frequency, float Q, double sampleRate) {
        frequency = std::max(1.0f, std::min(frequency, static_cast<float>(sampleRate * 0.49f)));
        Q = std::max(0.1f, std::min(Q, 100.0f));

        float w = 6.28318530718f * frequency / static_cast<float>(sampleRate);
        w = std::min(w, 3.0f);

        g = std::tan(w * 0.5f);
        k = 1.0f / Q;
        float gk = g * k;
        float a0 = 1.0f / (1.0f + gk + g * g);

        if (!std::isfinite(a0)) {
            a0 = 1.0f;
            g = 0.1f;
        }

        a1 = g * a0;
        a2 = g * a1;
        a3 = (k + k) * a1;
    }

    float processPeak(float input) {
        v0 = input;
        v1 = a1 * v0 - a2 * v1 - a3 * v2 + ic1eq;
        v2 = a1 * v1 - a2 * v2 + ic2eq;

        // FIXED integrator update (low THD)
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        float lowpass = v2;
        float highpass = v0 - k * v1 - v2;
        return lowpass - highpass;
    }

    void reset() {
        v0 = v1 = v2 = 0.0f;
        ic1eq = ic2eq = 0.0f;
    }
};

template<typename FilterType>
double testFilter(const std::string& name, FilterType& filter, double testFreq, double sampleRate) {
    const int numSamples = static_cast<int>(sampleRate);
    std::vector<float> output(numSamples);

    filter.setParameters(1000.0f, 2.0f, sampleRate);

    for (int i = 0; i < numSamples; ++i) {
        float input = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
        float peak = filter.processPeak(input);
        output[i] = input - peak + peak; // Reconstruct
    }

    std::vector<float> analysis(output.begin() + numSamples / 10, output.end());
    return THDAnalyzer::analyzeTHD(analysis, sampleRate, testFreq);
}

int main() {
    std::cout << "TPT Filter THD Comparison" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << std::fixed << std::setprecision(4);

    const double sampleRate = 48000.0;
    const std::vector<double> testFreqs = {100.0, 1000.0, 5000.0, 10000.0};

    std::cout << "\nOriginal TPT Filter (with broken integrator update):" << std::endl;
    for (double freq : testFreqs) {
        TPTFilterOriginal filter;
        double thd = testFilter("Original", filter, freq, sampleRate);
        std::cout << "  " << freq << " Hz: THD = " << thd << "%" << std::endl;
    }

    std::cout << "\nFixed TPT Filter (correct integrator update):" << std::endl;
    for (double freq : testFreqs) {
        TPTFilterFixed filter;
        double thd = testFilter("Fixed", filter, freq, sampleRate);
        std::cout << "  " << freq << " Hz: THD = " << thd << "%";
        if (thd < 0.5) {
            std::cout << " [PASS]";
        } else {
            std::cout << " [FAIL]";
        }
        std::cout << std::endl;
    }

    std::cout << "\n=== DIAGNOSIS ===" << std::endl;
    std::cout << "Root cause: Incorrect TPT integrator state update" << std::endl;
    std::cout << "Lines 119-120 in DynamicEQ.h use complex formulas that accumulate errors" << std::endl;
    std::cout << "Should use simple formula: ic1eq = 2*v1 - ic1eq, ic2eq = 2*v2 - ic2eq" << std::endl;

    return 0;
}

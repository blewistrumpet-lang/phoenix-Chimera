// test_thd_analyzer_fixed.cpp
// Fixed THD analyzer with proper FFT size

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>

// Fixed FFT-based THD analyzer
class THDAnalyzerFixed {
public:
    static double analyzeTHD(std::vector<float> signal, double sampleRate, double fundamentalFreq) {
        // Ensure power-of-2 FFT size
        int N = 1;
        while (N < signal.size()) N *= 2;
        if (N != signal.size()) {
            signal.resize(N, 0.0f); // Zero-pad to power of 2
        }

        // Apply Hann window
        std::vector<std::complex<double>> fft(N);
        double windowSum = 0.0;
        for (int i = 0; i < N; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / N));
            fft[i] = std::complex<double>(signal[i] * window, 0.0);
            windowSum += window * window;
        }

        // Perform FFT
        performFFT(fft);

        // Calculate magnitude spectrum (with window compensation)
        std::vector<double> magnitude(N / 2);
        double windowCompensation = std::sqrt(2.0 / windowSum); // RMS compensation
        for (int i = 0; i < N / 2; ++i) {
            magnitude[i] = std::abs(fft[i]) * windowCompensation;
        }

        // Find fundamental peak
        double binResolution = sampleRate / N;
        int fundamentalBin = static_cast<int>(fundamentalFreq / binResolution + 0.5);

        // Get fundamental magnitude (peak of fundamental and 2 neighbors)
        double fundamentalMag = 0.0;
        for (int i = std::max(0, fundamentalBin - 1); i <= std::min((int)magnitude.size() - 1, fundamentalBin + 1); ++i) {
            fundamentalMag = std::max(fundamentalMag, magnitude[i]);
        }

        // Sum harmonic magnitudes (2nd through 10th harmonic)
        double harmonicSum = 0.0;
        for (int harmonic = 2; harmonic <= 10; ++harmonic) {
            int harmonicBin = fundamentalBin * harmonic;
            if (harmonicBin < magnitude.size()) {
                // Peak pick around harmonic bin
                double harmonicMag = 0.0;
                for (int i = std::max(0, harmonicBin - 1); i <= std::min((int)magnitude.size() - 1, harmonicBin + 1); ++i) {
                    harmonicMag = std::max(harmonicMag, magnitude[i]);
                }
                harmonicSum += harmonicMag * harmonicMag;
            }
        }

        // Calculate THD as percentage
        double thd = 0.0;
        if (fundamentalMag > 1e-10) {
            thd = 100.0 * std::sqrt(harmonicSum) / fundamentalMag;
        }

        return thd;
    }

private:
    static void performFFT(std::vector<std::complex<double>>& data) {
        const int N = data.size();
        if (N <= 1) return;

        // Bit-reversal permutation
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

        // FFT computation
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

// Standard Biquad Filter
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

double testPassthrough(double testFreq, double sampleRate) {
    const int numSamples = 65536; // Power of 2
    std::vector<float> output(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        output[i] = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
    }

    return THDAnalyzerFixed::analyzeTHD(output, sampleRate, testFreq);
}

double testBiquad(double testFreq, double sampleRate, float eqFreq, float gainDb) {
    const int numSamples = 65536; // Power of 2
    std::vector<float> output;
    output.reserve(numSamples);

    BiquadFilter filter;
    filter.setPeakingEQ(eqFreq, 0.707f, gainDb, sampleRate);

    // Process with settling time
    for (int i = 0; i < numSamples + 1000; ++i) {
        float input = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
        float result = filter.process(input);
        if (i >= 1000) { // Skip first 1000 samples for settling
            output.push_back(result);
        }
    }

    return THDAnalyzerFixed::analyzeTHD(output, sampleRate, testFreq);
}

int main() {
    std::cout << "Fixed THD Analyzer Test" << std::endl;
    std::cout << "=======================" << std::endl;
    std::cout << std::fixed << std::setprecision(4);

    const double sampleRate = 48000.0;
    const std::vector<double> testFreqs = {100.0, 1000.0, 5000.0, 10000.0};

    std::cout << "\n1. Pure Sine Wave (should be < 0.01% THD):" << std::endl;
    for (double freq : testFreqs) {
        double thd = testPassthrough(freq, sampleRate);
        std::cout << "  " << freq << " Hz: THD = " << thd << "%";
        if (thd < 0.01) {
            std::cout << " [EXCELLENT]";
        } else if (thd < 0.1) {
            std::cout << " [GOOD]";
        } else {
            std::cout << " [ANALYZER BUG]";
        }
        std::cout << std::endl;
    }

    std::cout << "\n2. Biquad Peaking EQ (Q=0.707, 0dB gain - transparent):" << std::endl;
    for (double freq : testFreqs) {
        double thd = testBiquad(freq, sampleRate, freq, 0.0f);
        std::cout << "  " << freq << " Hz: THD = " << thd << "%";
        if (thd < 0.5) {
            std::cout << " [PASS]";
        } else {
            std::cout << " [FAIL]";
        }
        std::cout << std::endl;
    }

    std::cout << "\n3. Biquad with +6dB boost (Q=0.707):" << std::endl;
    for (double freq : testFreqs) {
        double thd = testBiquad(freq, sampleRate, freq, 6.0f);
        std::cout << "  " << freq << " Hz: THD = " << thd << "%";
        if (thd < 0.5) {
            std::cout << " [PASS]";
        } else {
            std::cout << " [FAIL]";
        }
        std::cout << std::endl;
    }

    std::cout << "\n4. Biquad with +12dB boost (Q=0.707 - stress test):" << std::endl;
    for (double freq : testFreqs) {
        double thd = testBiquad(freq, sampleRate, freq, 12.0f);
        std::cout << "  " << freq << " Hz: THD = " << thd << "%";
        if (thd < 0.5) {
            std::cout << " [PASS]";
        } else if (thd < 1.0) {
            std::cout << " [ACCEPTABLE]";
        } else {
            std::cout << " [FAIL]";
        }
        std::cout << std::endl;
    }

    return 0;
}

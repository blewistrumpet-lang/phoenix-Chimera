// test_dynamic_eq_thd_fix.cpp
// Comprehensive THD analysis for Dynamic EQ engine
// Target: Reduce THD from 0.759% to < 0.5%

#include "../JUCE_Plugin/Source/DynamicEQ.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>
#include <numeric>

// FFT-based THD analyzer
class THDAnalyzer {
public:
    static double analyzeTHD(const std::vector<float>& signal, double sampleRate, double fundamentalFreq) {
        const int N = signal.size();

        // Apply Hann window to reduce spectral leakage
        std::vector<std::complex<double>> fft(N);
        for (int i = 0; i < N; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (N - 1)));
            fft[i] = std::complex<double>(signal[i] * window, 0.0);
        }

        // Perform FFT
        performFFT(fft);

        // Calculate magnitude spectrum
        std::vector<double> magnitude(N / 2);
        for (int i = 0; i < N / 2; ++i) {
            magnitude[i] = std::abs(fft[i]);
        }

        // Find fundamental peak
        double binResolution = sampleRate / N;
        int fundamentalBin = static_cast<int>(fundamentalFreq / binResolution + 0.5);

        // Get fundamental magnitude (with neighboring bins for accuracy)
        double fundamentalMag = 0.0;
        for (int i = fundamentalBin - 2; i <= fundamentalBin + 2; ++i) {
            if (i >= 0 && i < magnitude.size()) {
                fundamentalMag = std::max(fundamentalMag, magnitude[i]);
            }
        }

        // Sum harmonic magnitudes (2nd through 10th harmonic)
        double harmonicSum = 0.0;
        for (int harmonic = 2; harmonic <= 10; ++harmonic) {
            int harmonicBin = fundamentalBin * harmonic;
            if (harmonicBin < magnitude.size()) {
                // Peak pick around harmonic bin
                double harmonicMag = 0.0;
                for (int i = harmonicBin - 2; i <= harmonicBin + 2; ++i) {
                    if (i >= 0 && i < magnitude.size()) {
                        harmonicMag = std::max(harmonicMag, magnitude[i]);
                    }
                }
                harmonicSum += harmonicMag * harmonicMag;
            }
        }

        // Calculate THD as percentage
        double thd = 0.0;
        if (fundamentalMag > 0.0) {
            thd = 100.0 * std::sqrt(harmonicSum) / fundamentalMag;
        }

        return thd;
    }

private:
    // Simple Cooley-Tukey FFT implementation
    static void performFFT(std::vector<std::complex<double>>& data) {
        const int N = data.size();
        if (N <= 1) return;

        // Bit-reversal permutation
        int j = 0;
        for (int i = 0; i < N; ++i) {
            if (i < j) {
                std::swap(data[i], data[j]);
            }
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

// Test different parameter configurations
struct TestConfig {
    std::string name;
    float frequency;    // 0-1 parameter
    float threshold;    // 0-1 parameter
    float ratio;        // 0-1 parameter
    float attack;       // 0-1 parameter
    float release;      // 0-1 parameter
    float gain;         // 0-1 parameter
    float mix;          // 0-1 parameter
    int mode;           // 0=compressor, 1=expander, 2=gate
};

double measureTHD(DynamicEQ& eq, const TestConfig& config, double testFreq, double sampleRate) {
    // Set parameters
    std::map<int, float> params;
    params[0] = config.frequency;
    params[1] = config.threshold;
    params[2] = config.ratio;
    params[3] = config.attack;
    params[4] = config.release;
    params[5] = config.gain;
    params[6] = config.mix;
    params[7] = config.mode / 2.99f;
    eq.updateParameters(params);

    // Generate test sine wave (1 second)
    const int bufferSize = 512;
    const int numBuffers = static_cast<int>(sampleRate / bufferSize);
    std::vector<float> recording;
    recording.reserve(numBuffers * bufferSize);

    double phase = 0.0;
    double phaseIncrement = 2.0 * M_PI * testFreq / sampleRate;

    for (int buf = 0; buf < numBuffers; ++buf) {
        juce::AudioBuffer<float> buffer(2, bufferSize);

        // Fill with sine wave at -3dBFS (0.707 amplitude)
        for (int i = 0; i < bufferSize; ++i) {
            float sample = 0.707f * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
            phase += phaseIncrement;
            if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
        }

        // Process
        eq.process(buffer);

        // Record left channel (skip first 10 buffers for settling)
        if (buf >= 10) {
            for (int i = 0; i < bufferSize; ++i) {
                recording.push_back(buffer.getSample(0, i));
            }
        }
    }

    // Analyze THD
    return THDAnalyzer::analyzeTHD(recording, sampleRate, testFreq);
}

void runComprehensiveTHDTest() {
    std::cout << "=== DYNAMIC EQ THD FIX TEST ===" << std::endl;
    std::cout << "Target: Reduce THD from 0.759% to < 0.5%" << std::endl;
    std::cout << std::endl;

    const double sampleRate = 48000.0;
    DynamicEQ eq;
    eq.prepareToPlay(sampleRate, 512);

    // Test configurations
    std::vector<TestConfig> configs = {
        // Baseline: Minimal processing
        {"Bypass (mix=0)", 0.5f, 0.5f, 0.0f, 0.2f, 0.4f, 0.5f, 0.0f, 0},

        // Low Q, gentle processing
        {"Low Q, gentle compression", 0.5f, 0.3f, 0.2f, 0.2f, 0.4f, 0.5f, 1.0f, 0},

        // Moderate settings
        {"Moderate compression", 0.5f, 0.3f, 0.5f, 0.2f, 0.4f, 0.5f, 1.0f, 0},

        // High ratio compression
        {"High ratio compression", 0.5f, 0.3f, 0.8f, 0.2f, 0.4f, 0.5f, 1.0f, 0},

        // Fast attack/release
        {"Fast attack/release", 0.5f, 0.3f, 0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 0},

        // Slow attack/release
        {"Slow attack/release", 0.5f, 0.3f, 0.5f, 1.0f, 1.0f, 0.5f, 1.0f, 0},

        // Low frequency (100 Hz)
        {"Low frequency 100Hz", 0.15f, 0.3f, 0.5f, 0.2f, 0.4f, 0.5f, 1.0f, 0},

        // High frequency (10 kHz)
        {"High frequency 10kHz", 0.85f, 0.3f, 0.5f, 0.2f, 0.4f, 0.5f, 1.0f, 0},

        // With gain boost
        {"With +6dB gain", 0.5f, 0.3f, 0.5f, 0.2f, 0.4f, 0.65f, 1.0f, 0},

        // With gain cut
        {"With -6dB gain", 0.5f, 0.3f, 0.5f, 0.2f, 0.4f, 0.35f, 1.0f, 0},

        // Expander mode
        {"Expander mode", 0.5f, 0.5f, 0.5f, 0.2f, 0.4f, 0.5f, 1.0f, 1},
    };

    // Test frequencies
    std::vector<double> testFreqs = {100.0, 1000.0, 5000.0, 10000.0};

    std::cout << std::fixed << std::setprecision(3);

    double maxTHD = 0.0;
    std::string worstCase;
    double worstFreq = 0.0;

    for (const auto& config : configs) {
        std::cout << "\nTesting: " << config.name << std::endl;
        std::cout << "  Freq param: " << config.frequency
                  << ", Threshold: " << config.threshold
                  << ", Ratio: " << config.ratio << std::endl;

        for (double testFreq : testFreqs) {
            double thd = measureTHD(eq, config, testFreq, sampleRate);

            std::cout << "  " << testFreq << " Hz: THD = " << thd << "%";
            if (thd > 0.5) {
                std::cout << " [FAIL]";
            } else {
                std::cout << " [PASS]";
            }
            std::cout << std::endl;

            if (thd > maxTHD) {
                maxTHD = thd;
                worstCase = config.name;
                worstFreq = testFreq;
            }

            // Reset for next test
            eq.reset();
            eq.prepareToPlay(sampleRate, 512);
        }
    }

    std::cout << "\n=== RESULTS ===" << std::endl;
    std::cout << "Maximum THD: " << maxTHD << "%" << std::endl;
    std::cout << "Worst case: " << worstCase << " at " << worstFreq << " Hz" << std::endl;

    if (maxTHD < 0.5) {
        std::cout << "\n*** SUCCESS: All tests pass THD < 0.5% threshold ***" << std::endl;
    } else {
        std::cout << "\n*** FAIL: Maximum THD " << maxTHD << "% exceeds 0.5% threshold ***" << std::endl;
        std::cout << "Exceeded by: " << ((maxTHD / 0.5 - 1.0) * 100.0) << "%" << std::endl;
    }
}

// Test individual components for THD contribution
void runComponentAnalysis() {
    std::cout << "\n=== COMPONENT ANALYSIS ===" << std::endl;
    std::cout << "Analyzing individual THD sources..." << std::endl;

    const double sampleRate = 48000.0;
    const int bufferSize = 512;

    // Test 1: Pure TPT filter (no dynamics)
    std::cout << "\n1. TPT Filter Only (mix=100%, ratio=0 for no compression):" << std::endl;
    {
        DynamicEQ eq;
        eq.prepareToPlay(sampleRate, bufferSize);

        TestConfig config = {"Filter only", 0.5f, 0.5f, 0.0f, 0.2f, 0.4f, 0.5f, 1.0f, 0};
        double thd1k = measureTHD(eq, config, 1000.0, sampleRate);
        std::cout << "   1kHz THD: " << thd1k << "%" << std::endl;
    }

    // Test 2: Dynamic processing at different threshold points
    std::cout << "\n2. Dynamic Processing Contribution:" << std::endl;
    for (float threshold : {0.2f, 0.5f, 0.8f}) {
        DynamicEQ eq;
        eq.prepareToPlay(sampleRate, bufferSize);

        TestConfig config = {"Dynamic", 0.5f, threshold, 0.5f, 0.2f, 0.4f, 0.5f, 1.0f, 0};
        double thd = measureTHD(eq, config, 1000.0, sampleRate);
        float thresholdDb = -60.0f + threshold * 60.0f;
        std::cout << "   Threshold " << thresholdDb << "dB: THD = " << thd << "%" << std::endl;
    }

    // Test 3: Different ratios
    std::cout << "\n3. Compression Ratio Effect:" << std::endl;
    for (float ratio : {0.2f, 0.5f, 0.8f}) {
        DynamicEQ eq;
        eq.prepareToPlay(sampleRate, bufferSize);

        TestConfig config = {"Ratio test", 0.5f, 0.3f, ratio, 0.2f, 0.4f, 0.5f, 1.0f, 0};
        double thd = measureTHD(eq, config, 1000.0, sampleRate);
        float ratioValue = 0.1f + ratio * 9.9f;
        std::cout << "   Ratio " << ratioValue << ":1: THD = " << thd << "%" << std::endl;
    }

    // Test 4: Attack/Release impact
    std::cout << "\n4. Attack/Release Time Effect:" << std::endl;
    std::vector<std::pair<float, float>> timings = {
        {0.0f, 0.0f},  // Fast
        {0.5f, 0.5f},  // Medium
        {1.0f, 1.0f}   // Slow
    };
    for (const auto& timing : timings) {
        DynamicEQ eq;
        eq.prepareToPlay(sampleRate, bufferSize);

        TestConfig config = {"Timing test", 0.5f, 0.3f, 0.5f, timing.first, timing.second, 0.5f, 1.0f, 0};
        double thd = measureTHD(eq, config, 1000.0, sampleRate);

        float attackMs = 0.1f + timing.first * 99.9f;
        float releaseMs = 10.0f + timing.second * 4990.0f;
        std::cout << "   Attack " << attackMs << "ms, Release " << releaseMs
                  << "ms: THD = " << thd << "%" << std::endl;
    }
}

int main() {
    std::cout << std::setprecision(4) << std::fixed;

    // Run component analysis first to identify primary sources
    runComponentAnalysis();

    // Run comprehensive test
    std::cout << "\n" << std::endl;
    runComprehensiveTHDTest();

    return 0;
}

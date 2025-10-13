// test_dynamic_eq_thd_standalone.cpp
// Standalone THD analysis for Dynamic EQ - No JUCE dependencies
// Tests the core DSP algorithms that contribute to THD

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>
#include <array>
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

// Simplified TPT Filter (from DynamicEQ)
struct TPTFilter {
    float v0 = 0.0f, v1 = 0.0f, v2 = 0.0f;
    float ic1eq = 0.0f, ic2eq = 0.0f;
    float g = 0.0f;
    float k = 0.0f;
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

        ic1eq = 2.0f * a1 * v0 - a2 * v1 - a3 * v2 + ic1eq;
        ic2eq = 2.0f * a1 * v1 - a2 * v2 + ic2eq;

        // Peak = lowpass - highpass
        float lowpass = v2;
        float highpass = v0 - k * v1 - v2;
        return lowpass - highpass;
    }

    void reset() {
        v0 = v1 = v2 = 0.0f;
        ic1eq = ic2eq = 0.0f;
    }
};

// Simplified Dynamic Processor (from DynamicEQ)
struct DynamicProcessor {
    static constexpr int LOOKAHEAD_SAMPLES = 64;
    static constexpr int ENVELOPE_HISTORY = 32;
    static constexpr int GAIN_CURVE_SIZE = 512;

    std::array<float, GAIN_CURVE_SIZE> gainCurve;
    std::array<float, LOOKAHEAD_SAMPLES> delayLine;
    std::array<float, ENVELOPE_HISTORY> gainHistory;
    int delayIndex = 0;
    int historyIndex = 0;
    float envelope = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    void buildGainCurve(float thresholdDb, float ratio, int mode) {
        for (int i = 0; i < GAIN_CURVE_SIZE; ++i) {
            float envLinear = static_cast<float>(i) / (GAIN_CURVE_SIZE - 1);
            float envDb = (envLinear > 0.00001f) ? 20.0f * std::log10(envLinear) : -100.0f;

            float gr = 1.0f;

            if (mode == 0) { // Compressor
                if (envDb > thresholdDb) {
                    float over = envDb - thresholdDb;
                    float compressedOver = over / ratio;
                    gr = std::pow(10.0f, -(over - compressedOver) / 20.0f);
                }
            }

            gainCurve[i] = gr;
        }
    }

    void setTiming(float attackMs, float releaseMs, double sampleRate) {
        attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
        releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
    }

    float process(float input) {
        delayLine[delayIndex] = input;

        int readIndex = (delayIndex + 1) % LOOKAHEAD_SAMPLES;
        float delayedSignal = delayLine[readIndex];

        delayIndex = (delayIndex + 1) % LOOKAHEAD_SAMPLES;

        // Peak detection
        float peak = 0.0f;
        for (int i = 0; i < LOOKAHEAD_SAMPLES; ++i) {
            peak = std::max(peak, std::abs(delayLine[i]));
        }

        // Envelope following
        if (peak > envelope) {
            envelope = peak + (envelope - peak) * attackCoeff;
        } else {
            envelope = peak + (envelope - peak) * releaseCoeff;
        }

        // Lookup gain from curve
        float envClamped = std::max(0.0f, std::min(1.0f, envelope));
        float index = envClamped * (GAIN_CURVE_SIZE - 1);
        int i0 = static_cast<int>(index);
        int i1 = std::min(i0 + 1, GAIN_CURVE_SIZE - 1);
        float frac = index - static_cast<float>(i0);

        float gainReduction = gainCurve[i0] + frac * (gainCurve[i1] - gainCurve[i0]);

        // Smooth gain changes
        gainHistory[historyIndex] = gainReduction;
        historyIndex = (historyIndex + 1) % ENVELOPE_HISTORY;

        float smoothGain = 0.0f;
        for (auto gain : gainHistory) {
            smoothGain += gain;
        }
        smoothGain /= ENVELOPE_HISTORY;

        return delayedSignal * smoothGain;
    }

    void reset() {
        delayLine.fill(0.0f);
        gainHistory.fill(1.0f);
        gainCurve.fill(1.0f);
        delayIndex = historyIndex = 0;
        envelope = 0.0f;
    }
};

// DC Blocker
struct DCBlocker {
    float x1 = 0.0f, y1 = 0.0f;
    static constexpr float R = 0.995f;

    float process(float input) {
        float output = input - x1 + R * y1;
        x1 = input;
        y1 = output;
        return output;
    }

    void reset() { x1 = y1 = 0.0f; }
};

// Test the complete Dynamic EQ signal path
double testDynamicEQPath(double testFreq, double sampleRate,
                         float eqFreq, float thresholdDb, float ratio,
                         float attackMs, float releaseMs) {
    const int numSamples = static_cast<int>(sampleRate); // 1 second
    std::vector<float> input(numSamples);
    std::vector<float> output(numSamples);

    // Generate sine wave at -3dBFS
    for (int i = 0; i < numSamples; ++i) {
        input[i] = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
    }

    // Initialize DSP components
    TPTFilter filter;
    filter.setParameters(eqFreq, 2.0f, sampleRate);

    DynamicProcessor processor;
    processor.setTiming(attackMs, releaseMs, sampleRate);
    processor.buildGainCurve(thresholdDb, ratio, 0);

    DCBlocker dcBlocker;

    // Process signal
    for (int i = 0; i < numSamples; ++i) {
        float sample = input[i];

        // DC blocking
        sample = dcBlocker.process(sample);

        // Filter
        float peakBand = filter.processPeak(sample);

        // Dynamic processing
        float processedPeak = processor.process(peakBand);

        // Reconstruct (THIS IS A KEY THD SOURCE!)
        output[i] = sample - peakBand + processedPeak;
    }

    // Skip first 10% for settling
    std::vector<float> analysis(output.begin() + numSamples / 10, output.end());

    return THDAnalyzer::analyzeTHD(analysis, sampleRate, testFreq);
}

// Test individual components
void testComponentTHD() {
    std::cout << "=== COMPONENT THD ANALYSIS ===" << std::endl;
    const double sampleRate = 48000.0;
    const double testFreq = 1000.0;
    const int numSamples = 48000;

    std::cout << std::fixed << std::setprecision(4);

    // Test 1: Pure TPT Filter
    std::cout << "\n1. TPT Filter Only (no dynamics):" << std::endl;
    {
        std::vector<float> signal(numSamples);
        TPTFilter filter;
        filter.setParameters(1000.0f, 2.0f, sampleRate);

        for (int i = 0; i < numSamples; ++i) {
            float input = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
            float peak = filter.processPeak(input);
            signal[i] = input - peak + peak; // Reconstruct (no gain change)
        }

        std::vector<float> analysis(signal.begin() + 4800, signal.end());
        double thd = THDAnalyzer::analyzeTHD(analysis, sampleRate, testFreq);
        std::cout << "   THD: " << thd << "%" << std::endl;
    }

    // Test 2: Filter reconstruction artifacts
    std::cout << "\n2. Filter Reconstruction (subtract-add method):" << std::endl;
    {
        std::vector<float> signal(numSamples);
        TPTFilter filter;
        filter.setParameters(1000.0f, 2.0f, sampleRate);

        for (int i = 0; i < numSamples; ++i) {
            float input = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
            float peak = filter.processPeak(input);
            // Subtract-and-add reconstruction can introduce floating point errors
            signal[i] = input - peak + peak * 0.5f; // 6dB reduction
        }

        std::vector<float> analysis(signal.begin() + 4800, signal.end());
        double thd = THDAnalyzer::analyzeTHD(analysis, sampleRate, testFreq);
        std::cout << "   THD with 6dB reduction: " << thd << "%" << std::endl;
    }

    // Test 3: Dynamic processor gain smoothing
    std::cout << "\n3. Dynamic Processor (averaging gain changes):" << std::endl;
    {
        std::vector<float> signal(numSamples);
        DynamicProcessor processor;
        processor.setTiming(5.0f, 100.0f, sampleRate);
        processor.buildGainCurve(-30.0f, 4.0f, 0);

        for (int i = 0; i < numSamples; ++i) {
            float input = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
            signal[i] = processor.process(input);
        }

        std::vector<float> analysis(signal.begin() + 4800, signal.end());
        double thd = THDAnalyzer::analyzeTHD(analysis, sampleRate, testFreq);
        std::cout << "   THD: " << thd << "%" << std::endl;
    }

    // Test 4: Gain curve interpolation
    std::cout << "\n4. Gain Curve Interpolation (512 steps):" << std::endl;
    {
        // Test if linear interpolation in gain curve causes stair-stepping
        DynamicProcessor processor;
        processor.setTiming(5.0f, 100.0f, sampleRate);
        processor.buildGainCurve(-20.0f, 8.0f, 0); // Aggressive compression

        std::vector<float> signal(numSamples);
        for (int i = 0; i < numSamples; ++i) {
            float input = 0.707f * std::sin(2.0 * M_PI * testFreq * i / sampleRate);
            signal[i] = processor.process(input);
        }

        std::vector<float> analysis(signal.begin() + 4800, signal.end());
        double thd = THDAnalyzer::analyzeTHD(analysis, sampleRate, testFreq);
        std::cout << "   THD with aggressive compression: " << thd << "%" << std::endl;
    }
}

// Test complete signal path
void testCompletePath() {
    std::cout << "\n=== COMPLETE DYNAMIC EQ PATH ===" << std::endl;
    const double sampleRate = 48000.0;

    std::cout << std::fixed << std::setprecision(4);

    struct TestCase {
        std::string name;
        double testFreq;
        float eqFreq;
        float thresholdDb;
        float ratio;
        float attackMs;
        float releaseMs;
    };

    std::vector<TestCase> tests = {
        {"Gentle 1kHz", 1000.0, 1000.0f, -30.0f, 2.0f, 5.0f, 100.0f},
        {"Moderate 1kHz", 1000.0, 1000.0f, -30.0f, 4.0f, 5.0f, 100.0f},
        {"Aggressive 1kHz", 1000.0, 1000.0f, -20.0f, 8.0f, 5.0f, 100.0f},
        {"Fast timing 1kHz", 1000.0, 1000.0f, -30.0f, 4.0f, 0.1f, 10.0f},
        {"Low freq 100Hz", 100.0, 100.0f, -30.0f, 4.0f, 5.0f, 100.0f},
        {"High freq 10kHz", 10000.0, 10000.0f, -30.0f, 4.0f, 5.0f, 100.0f},
    };

    for (const auto& test : tests) {
        double thd = testDynamicEQPath(test.testFreq, sampleRate, test.eqFreq,
                                       test.thresholdDb, test.ratio,
                                       test.attackMs, test.releaseMs);

        std::cout << test.name << ": THD = " << thd << "%";
        if (thd > 0.5) {
            std::cout << " [FAIL]";
        } else {
            std::cout << " [PASS]";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "Dynamic EQ THD Analysis - Standalone Test" << std::endl;
    std::cout << "==========================================" << std::endl;

    testComponentTHD();
    testCompletePath();

    std::cout << "\n=== KEY FINDINGS ===" << std::endl;
    std::cout << "If THD > 0.5%, primary suspects are:" << std::endl;
    std::cout << "1. Filter state integrator accumulation errors" << std::endl;
    std::cout << "2. Subtract-add signal reconstruction precision" << std::endl;
    std::cout << "3. Gain curve quantization (512 steps)" << std::endl;
    std::cout << "4. Gain smoothing phase distortion (32-sample averaging)" << std::endl;

    return 0;
}

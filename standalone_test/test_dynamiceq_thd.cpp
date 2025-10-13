/*
  ==============================================================================

    test_dynamiceq_thd.cpp
    THD Measurement Test for DynamicEQ (Engine 6) - Bug #9 Investigation

    Purpose: Measure Total Harmonic Distortion in DynamicEQ processing
    Target: THD < 1.0% (user reported 4.234% THD)

    Test Methodology:
    1. Generate 1kHz pure sine wave @ -6dBFS
    2. Process through DynamicEQ with various parameter settings
    3. Perform FFT analysis to extract harmonics
    4. Calculate THD from harmonic content
    5. Identify which parameters/code sections cause highest THD

  ==============================================================================
*/

#include "../JUCE_Plugin/Source/DynamicEQ.h"
#include "../JUCE_Plugin/Source/EngineTypes.h"
#include "../JUCE_Plugin/Source/DspEngineUtilities.h"
#include "JuceHeader.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <complex>
#include <map>
#include <algorithm>

// FFT-based THD measurement
class THDAnalyzer {
public:
    static constexpr int FFT_ORDER = 14; // 16384 samples
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;

    struct HarmonicAnalysis {
        float fundamental_dB = -200.0f;
        float second_harmonic_dB = -200.0f;
        float third_harmonic_dB = -200.0f;
        float fourth_harmonic_dB = -200.0f;
        float fifth_harmonic_dB = -200.0f;
        float thd_percent = 0.0f;
        float thd_plus_noise_percent = 0.0f;
        float snr_dB = 0.0f;

        std::vector<float> harmonic_levels; // All harmonics in dB

        void print() const {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "  Fundamental:       " << fundamental_dB << " dB\n";
            std::cout << "  2nd Harmonic:      " << second_harmonic_dB << " dB ("
                      << (second_harmonic_dB - fundamental_dB) << " dB below)\n";
            std::cout << "  3rd Harmonic:      " << third_harmonic_dB << " dB ("
                      << (third_harmonic_dB - fundamental_dB) << " dB below)\n";
            std::cout << "  4th Harmonic:      " << fourth_harmonic_dB << " dB\n";
            std::cout << "  5th Harmonic:      " << fifth_harmonic_dB << " dB\n";
            std::cout << "  THD:               " << thd_percent << "%\n";
            std::cout << "  THD+N:             " << thd_plus_noise_percent << "%\n";
            std::cout << "  SNR:               " << snr_dB << " dB\n";
        }
    };

    // Measure THD using FFT
    static HarmonicAnalysis measureTHD(const std::vector<float>& signal,
                                      double fundamental_freq,
                                      double sample_rate) {
        HarmonicAnalysis result;

        if (signal.size() < FFT_SIZE) {
            std::cerr << "Signal too short for FFT analysis\n";
            return result;
        }

        // Use middle section of signal (avoid startup transients)
        size_t start_offset = signal.size() / 4;
        std::vector<float> analysis_window(FFT_SIZE);

        // Copy data and apply Blackman-Harris window for better harmonic resolution
        for (int i = 0; i < FFT_SIZE; ++i) {
            float window = applyBlackmanHarrisWindow(i, FFT_SIZE);
            analysis_window[i] = signal[start_offset + i] * window;
        }

        // Perform FFT using JUCE
        juce::dsp::FFT fft(FFT_ORDER);
        std::vector<float> fft_data(FFT_SIZE * 2, 0.0f); // Real + Imaginary

        // Copy windowed signal to FFT buffer (real part)
        for (int i = 0; i < FFT_SIZE; ++i) {
            fft_data[i * 2] = analysis_window[i];
            fft_data[i * 2 + 1] = 0.0f;
        }

        // Perform forward FFT
        fft.performRealOnlyForwardTransform(fft_data.data(), true);

        // Calculate magnitude spectrum
        std::vector<float> magnitude(FFT_SIZE / 2);
        for (int i = 0; i < FFT_SIZE / 2; ++i) {
            float real = fft_data[i * 2];
            float imag = fft_data[i * 2 + 1];
            magnitude[i] = std::sqrt(real * real + imag * imag);
        }

        // Find fundamental and harmonics
        double bin_resolution = sample_rate / FFT_SIZE;
        int fundamental_bin = static_cast<int>(fundamental_freq / bin_resolution + 0.5);

        // Get fundamental magnitude (average over +/- 2 bins for accuracy)
        float fundamental_mag = 0.0f;
        for (int i = -2; i <= 2; ++i) {
            int bin = fundamental_bin + i;
            if (bin >= 0 && bin < FFT_SIZE / 2) {
                fundamental_mag += magnitude[bin];
            }
        }
        fundamental_mag /= 5.0f;

        result.fundamental_dB = 20.0f * std::log10(std::max(1e-10f, fundamental_mag));

        // Measure harmonics
        float harmonic_energy = 0.0f;
        for (int h = 2; h <= 10; ++h) { // Check up to 10th harmonic
            double harmonic_freq = fundamental_freq * h;
            if (harmonic_freq > sample_rate / 2.0) break;

            int harmonic_bin = static_cast<int>(harmonic_freq / bin_resolution + 0.5);

            // Average over +/- 1 bin
            float harmonic_mag = 0.0f;
            for (int i = -1; i <= 1; ++i) {
                int bin = harmonic_bin + i;
                if (bin >= 0 && bin < FFT_SIZE / 2) {
                    harmonic_mag += magnitude[bin];
                }
            }
            harmonic_mag /= 3.0f;

            float harmonic_dB = 20.0f * std::log10(std::max(1e-10f, harmonic_mag));
            result.harmonic_levels.push_back(harmonic_dB);

            // Store specific harmonics
            if (h == 2) result.second_harmonic_dB = harmonic_dB;
            if (h == 3) result.third_harmonic_dB = harmonic_dB;
            if (h == 4) result.fourth_harmonic_dB = harmonic_dB;
            if (h == 5) result.fifth_harmonic_dB = harmonic_dB;

            // Accumulate harmonic energy for THD calculation
            harmonic_energy += harmonic_mag * harmonic_mag;
        }

        // Calculate THD = sqrt(sum of harmonic powers) / fundamental
        float thd_ratio = std::sqrt(harmonic_energy) / (fundamental_mag + 1e-10f);
        result.thd_percent = thd_ratio * 100.0f;

        // Calculate total noise floor (excluding fundamental and harmonics)
        float noise_energy = 0.0f;
        int noise_bins = 0;
        for (int i = 10; i < FFT_SIZE / 2; ++i) { // Start after DC and low frequencies
            double freq = i * bin_resolution;

            // Skip fundamental and harmonics (+/- 3 bins)
            bool is_harmonic = false;
            for (int h = 1; h <= 10; ++h) {
                double harmonic_freq = fundamental_freq * h;
                if (std::abs(freq - harmonic_freq) < 3 * bin_resolution) {
                    is_harmonic = true;
                    break;
                }
            }

            if (!is_harmonic) {
                noise_energy += magnitude[i] * magnitude[i];
                noise_bins++;
            }
        }

        float noise_rms = std::sqrt(noise_energy / std::max(1, noise_bins));
        result.thd_plus_noise_percent = std::sqrt(harmonic_energy + noise_energy * noise_bins)
                                       / (fundamental_mag + 1e-10f) * 100.0f;
        result.snr_dB = 20.0f * std::log10((fundamental_mag + 1e-10f) / (noise_rms + 1e-10f));

        return result;
    }

private:
    // 4-term Blackman-Harris window for excellent sidelobe rejection
    static float applyBlackmanHarrisWindow(int n, int N) {
        const float a0 = 0.35875f;
        const float a1 = 0.48829f;
        const float a2 = 0.14128f;
        const float a3 = 0.01168f;

        float phase = 2.0f * M_PI * n / (N - 1);
        return a0
             - a1 * std::cos(phase)
             + a2 * std::cos(2.0f * phase)
             - a3 * std::cos(3.0f * phase);
    }
};

// Test signal generator
class SignalGenerator {
public:
    static std::vector<float> generateSineWave(double frequency, float amplitude_dB,
                                               double duration, double sample_rate) {
        int num_samples = static_cast<int>(duration * sample_rate);
        std::vector<float> signal(num_samples);

        float amplitude = std::pow(10.0f, amplitude_dB / 20.0f);
        double phase = 0.0;
        double phase_increment = 2.0 * M_PI * frequency / sample_rate;

        for (int i = 0; i < num_samples; ++i) {
            signal[i] = amplitude * std::sin(phase);
            phase += phase_increment;

            // Wrap phase to avoid accumulation errors
            if (phase > 2.0 * M_PI) {
                phase -= 2.0 * M_PI;
            }
        }

        return signal;
    }
};

// Main test class
class DynamicEQ_THD_Test {
private:
    std::unique_ptr<DynamicEQ> engine;
    double sample_rate;
    int block_size;
    std::ofstream log_file;

    struct TestResult {
        std::string test_name;
        std::map<int, float> parameters;
        THDAnalyzer::HarmonicAnalysis analysis;
        bool passed;
    };

    std::vector<TestResult> results;

public:
    DynamicEQ_THD_Test(double sr = 48000.0, int bs = 512)
        : sample_rate(sr), block_size(bs) {
        engine = std::make_unique<DynamicEQ>();
        engine->prepareToPlay(sample_rate, block_size);

        log_file.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/dynamiceq_thd_report.txt");
        if (!log_file.is_open()) {
            std::cerr << "Warning: Could not open log file\n";
        }
    }

    ~DynamicEQ_THD_Test() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    void log(const std::string& message) {
        std::cout << message;
        if (log_file.is_open()) {
            log_file << message;
            log_file.flush();
        }
    }

    std::vector<float> processSignal(const std::vector<float>& input,
                                    const std::map<int, float>& params) {
        // Reset engine for clean measurement
        engine->reset();
        engine->updateParameters(params);

        // Allow parameters to settle (process some silence first)
        juce::AudioBuffer<float> warmup(2, block_size * 10);
        warmup.clear();
        for (int i = 0; i < 10; ++i) {
            engine->process(warmup);
        }

        // Process actual signal
        std::vector<float> output;
        output.reserve(input.size());

        for (size_t i = 0; i < input.size(); i += block_size) {
            size_t samples_this_block = std::min(static_cast<size_t>(block_size),
                                                input.size() - i);

            juce::AudioBuffer<float> buffer(2, static_cast<int>(samples_this_block));

            // Fill with mono signal
            for (size_t j = 0; j < samples_this_block; ++j) {
                float sample = (i + j < input.size()) ? input[i + j] : 0.0f;
                buffer.setSample(0, static_cast<int>(j), sample);
                buffer.setSample(1, static_cast<int>(j), sample);
            }

            // Process
            engine->process(buffer);

            // Extract left channel output
            for (size_t j = 0; j < samples_this_block; ++j) {
                output.push_back(buffer.getSample(0, static_cast<int>(j)));
            }
        }

        return output;
    }

    void testConfiguration(const std::string& test_name,
                          const std::map<int, float>& params,
                          float thd_threshold = 1.0f) {
        log("\n----------------------------------------\n");
        log("Test: " + test_name + "\n");
        log("----------------------------------------\n");

        // Print parameters
        log("Parameters:\n");
        for (const auto& [idx, val] : params) {
            std::string param_name = engine->getParameterName(idx).toStdString();
            log("  " + param_name + " (" + std::to_string(idx) + "): " +
                std::to_string(val) + "\n");
        }
        log("\n");

        // Generate 1kHz sine @ -6dBFS, 2 seconds duration
        const double test_frequency = 1000.0;
        const float test_level_dB = -6.0f;
        const double test_duration = 2.0;

        auto input_signal = SignalGenerator::generateSineWave(test_frequency, test_level_dB,
                                                             test_duration, sample_rate);

        // Process through DynamicEQ
        auto output_signal = processSignal(input_signal, params);

        // Analyze THD
        auto analysis = THDAnalyzer::measureTHD(output_signal, test_frequency, sample_rate);

        log("Results:\n");
        analysis.print();

        // Check if passed
        bool passed = analysis.thd_percent < thd_threshold;
        log("\nStatus: " + std::string(passed ? "PASS" : "FAIL") +
            " (THD " + std::string(passed ? "<" : ">=") + " " +
            std::to_string(thd_threshold) + "%)\n");

        // Store result
        TestResult result;
        result.test_name = test_name;
        result.parameters = params;
        result.analysis = analysis;
        result.passed = passed;
        results.push_back(result);
    }

    void runAllTests() {
        log("\n╔═══════════════════════════════════════════════════════════════╗\n");
        log("║  DynamicEQ THD Measurement Test - Bug #9 Investigation        ║\n");
        log("╚═══════════════════════════════════════════════════════════════╝\n");
        log("\nEngine: DynamicEQ (Engine 6)\n");
        log("Sample Rate: " + std::to_string(sample_rate) + " Hz\n");
        log("Block Size: " + std::to_string(block_size) + " samples\n");
        log("Test Signal: 1kHz sine wave @ -6dBFS\n");
        log("Target THD: < 1.0%\n\n");

        // Test 1: Bypass mode (mix = 0)
        {
            std::map<int, float> params;
            params[6] = 0.0f; // Mix = 0% (completely dry)
            testConfiguration("1. Bypass (Mix = 0%)", params, 0.01f); // Should be near perfect
        }

        // Test 2: Neutral settings (no processing)
        {
            std::map<int, float> params;
            params[0] = 0.5f;  // Frequency = 1kHz
            params[1] = 1.0f;  // Threshold = 0dB (no compression)
            params[2] = 0.0f;  // Ratio = 1:1 (no compression)
            params[5] = 0.5f;  // Gain = 0dB
            params[6] = 1.0f;  // Mix = 100%
            testConfiguration("2. Neutral Settings (no compression)", params, 0.1f);
        }

        // Test 3: Static EQ only (no dynamics)
        {
            std::map<int, float> params;
            params[0] = 0.5f;  // Frequency = 1kHz
            params[1] = 1.0f;  // Threshold very high (no dynamics)
            params[2] = 0.0f;  // Ratio = 1:1
            params[5] = 0.7f;  // Gain = +6dB boost
            params[6] = 1.0f;  // Mix = 100%
            testConfiguration("3. Static EQ (+6dB @ 1kHz)", params, 0.5f);
        }

        // Test 4: Light dynamic compression
        {
            std::map<int, float> params;
            params[0] = 0.5f;  // Frequency = 1kHz
            params[1] = 0.3f;  // Threshold = -40dB
            params[2] = 0.5f;  // Ratio = 3:1
            params[3] = 0.2f;  // Attack = fast
            params[4] = 0.3f;  // Release = medium
            params[5] = 0.5f;  // Gain = 0dB
            params[6] = 1.0f;  // Mix = 100%
            params[7] = 0.0f;  // Mode = Compressor
            testConfiguration("4. Light Compression (3:1, -40dB threshold)", params, 1.0f);
        }

        // Test 5: Heavy compression
        {
            std::map<int, float> params;
            params[0] = 0.5f;  // Frequency = 1kHz
            params[1] = 0.5f;  // Threshold = -30dB
            params[2] = 0.8f;  // Ratio = 8:1
            params[3] = 0.1f;  // Attack = very fast
            params[4] = 0.2f;  // Release = fast
            params[5] = 0.5f;  // Gain = 0dB
            params[6] = 1.0f;  // Mix = 100%
            params[7] = 0.0f;  // Mode = Compressor
            testConfiguration("5. Heavy Compression (8:1, -30dB threshold)", params, 1.0f);
        }

        // Test 6: Expander mode
        {
            std::map<int, float> params;
            params[0] = 0.5f;  // Frequency = 1kHz
            params[1] = 0.4f;  // Threshold
            params[2] = 0.6f;  // Ratio
            params[3] = 0.3f;  // Attack
            params[4] = 0.4f;  // Release
            params[5] = 0.5f;  // Gain = 0dB
            params[6] = 1.0f;  // Mix = 100%
            params[7] = 0.5f;  // Mode = Expander
            testConfiguration("6. Expander Mode", params, 1.0f);
        }

        // Test 7: Gate mode
        {
            std::map<int, float> params;
            params[0] = 0.5f;  // Frequency = 1kHz
            params[1] = 0.2f;  // Threshold (low, shouldn't gate our signal)
            params[2] = 0.5f;  // Ratio
            params[3] = 0.2f;  // Attack
            params[4] = 0.3f;  // Release
            params[5] = 0.5f;  // Gain = 0dB
            params[6] = 1.0f;  // Mix = 100%
            params[7] = 1.0f;  // Mode = Gate
            testConfiguration("7. Gate Mode", params, 1.0f);
        }

        // Test 8: High Q (narrow band)
        {
            std::map<int, float> params;
            params[0] = 0.5f;  // Frequency = 1kHz
            params[1] = 0.4f;  // Threshold
            params[2] = 0.5f;  // Ratio
            params[5] = 0.6f;  // Gain = +3dB
            params[6] = 1.0f;  // Mix = 100%
            // Note: Q is fixed at 2.0 in current implementation (line 111 of DynamicEQ.cpp)
            testConfiguration("8. High Q Filter (narrow band)", params, 1.0f);
        }

        // Test 9: Extreme settings
        {
            std::map<int, float> params;
            params[0] = 0.5f;  // Frequency = 1kHz
            params[1] = 0.7f;  // Threshold = -18dB (signal will be compressed)
            params[2] = 1.0f;  // Ratio = 10:1 (maximum)
            params[3] = 0.0f;  // Attack = 0.1ms (fastest)
            params[4] = 0.0f;  // Release = 10ms (fastest)
            params[5] = 0.8f;  // Gain = +12dB
            params[6] = 1.0f;  // Mix = 100%
            params[7] = 0.0f;  // Mode = Compressor
            testConfiguration("9. Extreme Settings (10:1, +12dB gain)", params, 1.0f);
        }

        // Print summary
        printSummary();
    }

    void printSummary() {
        log("\n╔═══════════════════════════════════════════════════════════════╗\n");
        log("║                        TEST SUMMARY                            ║\n");
        log("╚═══════════════════════════════════════════════════════════════╝\n\n");

        int passed = 0;
        int failed = 0;
        float worst_thd = 0.0f;
        std::string worst_test;

        log("Test Results:\n");
        log("-------------\n");
        for (const auto& result : results) {
            log(result.test_name + ": ");
            log(result.passed ? "PASS" : "FAIL");
            log(" (THD = " + std::to_string(result.analysis.thd_percent) + "%)\n");

            if (result.passed) {
                passed++;
            } else {
                failed++;
            }

            if (result.analysis.thd_percent > worst_thd) {
                worst_thd = result.analysis.thd_percent;
                worst_test = result.test_name;
            }
        }

        log("\nStatistics:\n");
        log("-----------\n");
        log("Total Tests:   " + std::to_string(results.size()) + "\n");
        log("Passed:        " + std::to_string(passed) + "\n");
        log("Failed:        " + std::to_string(failed) + "\n");
        log("Success Rate:  " + std::to_string(100.0f * passed / results.size()) + "%\n");
        log("\nWorst THD:     " + std::to_string(worst_thd) + "% (" + worst_test + ")\n");

        // Diagnosis
        log("\n╔═══════════════════════════════════════════════════════════════╗\n");
        log("║                         DIAGNOSIS                              ║\n");
        log("╚═══════════════════════════════════════════════════════════════╝\n\n");

        if (worst_thd > 4.0f) {
            log("CRITICAL: THD exceeds 4% - severe nonlinearity detected!\n\n");
            log("Potential causes:\n");
            log("1. Logarithmic/exponential operations in gain calculation\n");
            log("   - Line 189: std::log10() in envelope detection\n");
            log("   - Line 195, 201: std::pow(10.0f, ...) in gain reduction\n");
            log("   - Line 143, 389-390: dbToLinear/linearToDb conversions\n\n");
            log("2. Filter nonlinearity\n");
            log("   - Line 82: std::tan() in TPT filter coefficient calculation\n");
            log("   - Filter may be unstable at certain Q values\n\n");
            log("3. Commented-out saturation still causing issues\n");
            log("   - Check if analog saturation is actually disabled\n\n");
        } else if (worst_thd > 1.0f) {
            log("WARNING: THD exceeds target of 1%\n\n");
            log("Recommendations:\n");
            log("1. Add oversampling (2x or 4x) to reduce aliasing\n");
            log("2. Use polynomial approximations instead of std::log/exp\n");
            log("3. Implement linear gain smoothing instead of dB domain\n");
        } else {
            log("PASS: All THD measurements within acceptable range (<1%)\n");
        }

        log("\n");
    }
};

int main(int argc, char* argv[]) {
    try {
        std::cout << "\nStarting DynamicEQ THD Investigation (Bug #9)...\n\n";

        DynamicEQ_THD_Test tester(48000.0, 512);
        tester.runAllTests();

        std::cout << "\n\nTest complete! Check dynamiceq_thd_report.txt for detailed results.\n\n";

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Test failed with unknown exception." << std::endl;
        return 1;
    }
}

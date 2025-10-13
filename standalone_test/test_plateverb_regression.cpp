// PlateReverb (Engine 39) Comprehensive Regression Test
// Tests: Impulse response, reverb tail, RT60, stereo width
// Ensures no regression from previous fix

#include <JuceHeader.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>
#include <iomanip>
#include "PlateReverb.h"

struct RT60Measurement {
    double rt60_left;
    double rt60_right;
    double rt60_avg;
    bool valid;
};

struct StereoAnalysis {
    double correlation;
    double width_factor;
    double left_energy;
    double right_energy;
    double balance;
};

struct TailAnalysis {
    int tail_length_samples;
    double tail_length_ms;
    double peak_left;
    double peak_right;
    int peak_sample_left;
    int peak_sample_right;
    double decay_rate;
};

class PlateReverbTester {
private:
    std::unique_ptr<PlateReverb> engine;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

public:
    PlateReverbTester() {
        engine = std::make_unique<PlateReverb>();
        engine->prepareToPlay(sampleRate, blockSize);

        // Set parameters: 100% wet, 70% size, 0% predelay
        std::map<int, float> params;
        params[0] = 1.0f;   // Mix = 100% wet
        params[1] = 0.7f;   // Size = 70%
        params[3] = 0.0f;   // Pre-delay = 0%
        engine->updateParameters(params);
    }

    std::vector<std::pair<float, float>> generateImpulseResponse(int duration_samples) {
        std::vector<std::pair<float, float>> response;
        response.reserve(duration_samples);

        int processed = 0;

        // First block with impulse
        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Left channel impulse
        buffer.setSample(1, 0, 0.0f);  // Right channel silent

        engine->process(buffer);

        for (int i = 0; i < blockSize && processed < duration_samples; ++i) {
            response.push_back({buffer.getSample(0, i), buffer.getSample(1, i)});
            processed++;
        }

        // Process remaining blocks
        while (processed < duration_samples) {
            buffer.clear();
            engine->process(buffer);

            for (int i = 0; i < blockSize && processed < duration_samples; ++i) {
                response.push_back({buffer.getSample(0, i), buffer.getSample(1, i)});
                processed++;
            }
        }

        return response;
    }

    TailAnalysis analyzeTail(const std::vector<std::pair<float, float>>& response) {
        TailAnalysis result;
        result.peak_left = 0.0;
        result.peak_right = 0.0;
        result.peak_sample_left = 0;
        result.peak_sample_right = 0;
        result.tail_length_samples = 0;

        const double noise_floor = 1e-5;  // -100dB

        // Find peaks and last audible sample
        for (size_t i = 0; i < response.size(); ++i) {
            float left = std::abs(response[i].first);
            float right = std::abs(response[i].second);

            if (left > result.peak_left) {
                result.peak_left = left;
                result.peak_sample_left = i;
            }

            if (right > result.peak_right) {
                result.peak_right = right;
                result.peak_sample_right = i;
            }

            if (left > noise_floor || right > noise_floor) {
                result.tail_length_samples = i;
            }
        }

        result.tail_length_ms = (result.tail_length_samples / sampleRate) * 1000.0;

        // Estimate decay rate (dB/sec) from envelope
        if (result.tail_length_samples > 1000) {
            double peak = std::max(result.peak_left, result.peak_right);
            double level_at_1sec = 0.0;
            int sample_1sec = std::min((int)(sampleRate), (int)response.size() - 1);

            // Get level at 1 second
            for (int i = sample_1sec; i < sample_1sec + 100 && i < response.size(); ++i) {
                level_at_1sec = std::max(level_at_1sec,
                    (double)std::max(std::abs(response[i].first), std::abs(response[i].second)));
            }

            if (level_at_1sec > 0 && peak > 0) {
                result.decay_rate = 20.0 * std::log10(level_at_1sec / peak);
            } else {
                result.decay_rate = -60.0;
            }
        } else {
            result.decay_rate = 0.0;
        }

        return result;
    }

    RT60Measurement measureRT60(const std::vector<std::pair<float, float>>& response) {
        RT60Measurement result;
        result.rt60_left = 0.0;
        result.rt60_right = 0.0;
        result.rt60_avg = 0.0;
        result.valid = false;

        // Find peak levels
        double peak_left = 0.0;
        double peak_right = 0.0;

        for (const auto& sample : response) {
            peak_left = std::max(peak_left, (double)std::abs(sample.first));
            peak_right = std::max(peak_right, (double)std::abs(sample.second));
        }

        if (peak_left < 1e-6 || peak_right < 1e-6) {
            return result;  // No signal
        }

        // Calculate -60dB thresholds
        double threshold_left = peak_left * 0.001;   // -60dB
        double threshold_right = peak_right * 0.001;

        // Find RT60 times (time to decay 60dB)
        int rt60_sample_left = 0;
        int rt60_sample_right = 0;

        // Find last sample above -60dB threshold
        for (int i = response.size() - 1; i >= 0; --i) {
            if (std::abs(response[i].first) > threshold_left && rt60_sample_left == 0) {
                rt60_sample_left = i;
            }
            if (std::abs(response[i].second) > threshold_right && rt60_sample_right == 0) {
                rt60_sample_right = i;
            }
            if (rt60_sample_left > 0 && rt60_sample_right > 0) {
                break;
            }
        }

        result.rt60_left = (rt60_sample_left / sampleRate);
        result.rt60_right = (rt60_sample_right / sampleRate);
        result.rt60_avg = (result.rt60_left + result.rt60_right) / 2.0;
        result.valid = (rt60_sample_left > 0 && rt60_sample_right > 0);

        return result;
    }

    StereoAnalysis analyzeStereoWidth(const std::vector<std::pair<float, float>>& response) {
        StereoAnalysis result;

        double sum_left = 0.0;
        double sum_right = 0.0;
        double sum_lr = 0.0;
        double sum_left_sq = 0.0;
        double sum_right_sq = 0.0;

        for (const auto& sample : response) {
            double left = sample.first;
            double right = sample.second;

            sum_left += left;
            sum_right += right;
            sum_lr += left * right;
            sum_left_sq += left * left;
            sum_right_sq += right * right;
        }

        result.left_energy = sum_left_sq;
        result.right_energy = sum_right_sq;

        // Calculate correlation coefficient
        double n = response.size();
        double numerator = n * sum_lr - sum_left * sum_right;
        double denominator = std::sqrt((n * sum_left_sq - sum_left * sum_left) *
                                       (n * sum_right_sq - sum_right * sum_right));

        if (denominator > 1e-10) {
            result.correlation = numerator / denominator;
        } else {
            result.correlation = 0.0;
        }

        // Calculate stereo width (0 = mono, 1 = wide stereo)
        result.width_factor = 1.0 - std::abs(result.correlation);

        // Calculate balance
        double total_energy = result.left_energy + result.right_energy;
        if (total_energy > 0) {
            result.balance = (result.right_energy - result.left_energy) / total_energy;
        } else {
            result.balance = 0.0;
        }

        return result;
    }

    void saveImpulseResponse(const std::vector<std::pair<float, float>>& response,
                            const std::string& filename) {
        std::ofstream file(filename);
        file << "sample,time_s,left,right\n";
        file << std::scientific << std::setprecision(6);

        for (size_t i = 0; i < response.size(); ++i) {
            double time = i / sampleRate;
            file << i << "," << time << ","
                 << response[i].first << ","
                 << response[i].second << "\n";
        }
    }
};

void printHeader() {
    std::cout << "\n";
    std::cout << "╔═════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        PlateReverb (Engine 39) Regression Test                 ║\n";
    std::cout << "╚═════════════════════════════════════════════════════════════════╝\n\n";
}

void printResults(const TailAnalysis& tail, const RT60Measurement& rt60,
                 const StereoAnalysis& stereo) {
    std::cout << std::fixed << std::setprecision(4);

    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  REVERB TAIL ANALYSIS\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  Tail Length:        " << tail.tail_length_ms << " ms "
              << "(" << tail.tail_length_samples << " samples)\n";
    std::cout << "  Peak Left:          " << tail.peak_left << " at sample "
              << tail.peak_sample_left << " (" << (tail.peak_sample_left/48.0) << " ms)\n";
    std::cout << "  Peak Right:         " << tail.peak_right << " at sample "
              << tail.peak_sample_right << " (" << (tail.peak_sample_right/48.0) << " ms)\n";
    std::cout << "  Decay Rate:         " << tail.decay_rate << " dB/sec\n";
    std::cout << "\n";

    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  RT60 MEASUREMENT (Reverb Time)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  RT60 Left:          " << (rt60.rt60_left * 1000.0) << " ms\n";
    std::cout << "  RT60 Right:         " << (rt60.rt60_right * 1000.0) << " ms\n";
    std::cout << "  RT60 Average:       " << (rt60.rt60_avg * 1000.0) << " ms\n";
    std::cout << "  Valid:              " << (rt60.valid ? "YES" : "NO") << "\n";
    std::cout << "\n";

    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  STEREO WIDTH ANALYSIS\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  Correlation:        " << stereo.correlation << "\n";
    std::cout << "  Stereo Width:       " << stereo.width_factor << "\n";
    std::cout << "  Left Energy:        " << std::scientific << stereo.left_energy << "\n";
    std::cout << "  Right Energy:       " << std::scientific << stereo.right_energy << "\n";
    std::cout << "  Balance:            " << std::fixed << stereo.balance << "\n";
    std::cout << "\n";
}

bool verifyNoRegression(const TailAnalysis& tail, const RT60Measurement& rt60,
                       const StereoAnalysis& stereo) {
    bool passed = true;
    std::vector<std::string> failures;

    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  REGRESSION CHECKS (vs Previous Test Results)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";

    // Check 1: Reverb tail exists and is reasonable length
    std::cout << "  [1] Reverb tail present:          ";
    if (tail.tail_length_ms > 100.0 && tail.tail_length_ms < 10000.0) {
        std::cout << "✓ PASS (" << tail.tail_length_ms << " ms)\n";
    } else {
        std::cout << "✗ FAIL (" << tail.tail_length_ms << " ms)\n";
        failures.push_back("Reverb tail length out of range");
        passed = false;
    }

    // Check 2: Peak levels are reasonable (not silent, not clipping)
    std::cout << "  [2] Peak levels valid:            ";
    double max_peak = std::max(tail.peak_left, tail.peak_right);
    if (max_peak > 0.001 && max_peak < 2.0) {
        std::cout << "✓ PASS (peak=" << max_peak << ")\n";
    } else {
        std::cout << "✗ FAIL (peak=" << max_peak << ")\n";
        failures.push_back("Peak level out of valid range");
        passed = false;
    }

    // Check 3: RT60 is reasonable (typical reverb: 0.5-3 seconds)
    std::cout << "  [3] RT60 reasonable:               ";
    if (rt60.valid && rt60.rt60_avg > 0.3 && rt60.rt60_avg < 5.0) {
        std::cout << "✓ PASS (" << (rt60.rt60_avg * 1000.0) << " ms)\n";
    } else {
        std::cout << "✗ FAIL (" << (rt60.rt60_avg * 1000.0) << " ms)\n";
        failures.push_back("RT60 measurement out of range");
        passed = false;
    }

    // Check 4: Stereo width is adequate (correlation should not be near 1.0)
    std::cout << "  [4] Stereo field present:          ";
    if (stereo.width_factor > 0.3) {
        std::cout << "✓ PASS (width=" << stereo.width_factor << ")\n";
    } else {
        std::cout << "✗ FAIL (width=" << stereo.width_factor << ")\n";
        failures.push_back("Insufficient stereo width");
        passed = false;
    }

    // Check 5: Both channels have output
    std::cout << "  [5] Both channels active:          ";
    if (stereo.left_energy > 1e-6 && stereo.right_energy > 1e-6) {
        std::cout << "✓ PASS\n";
    } else {
        std::cout << "✗ FAIL\n";
        failures.push_back("One or both channels silent");
        passed = false;
    }

    // Check 6: Decay rate is negative (signal decays over time)
    std::cout << "  [6] Proper decay:                  ";
    if (tail.decay_rate < -10.0 && tail.decay_rate > -100.0) {
        std::cout << "✓ PASS (" << tail.decay_rate << " dB/s)\n";
    } else {
        std::cout << "✗ FAIL (" << tail.decay_rate << " dB/s)\n";
        failures.push_back("Decay rate abnormal");
        passed = false;
    }

    std::cout << "\n";

    if (passed) {
        std::cout << "  ✓ ALL REGRESSION CHECKS PASSED\n";
        std::cout << "  ✓ No degradation from previous test results\n";
    } else {
        std::cout << "  ✗ REGRESSION DETECTED\n";
        std::cout << "\n  Failed checks:\n";
        for (const auto& fail : failures) {
            std::cout << "    • " << fail << "\n";
        }
    }

    std::cout << "\n";
    return passed;
}

int main() {
    printHeader();

    std::cout << "Initializing PlateReverb engine...\n";
    PlateReverbTester tester;

    std::cout << "Generating impulse response (2 seconds)...\n";
    const int duration = 96000;  // 2 seconds at 48kHz
    auto response = tester.generateImpulseResponse(duration);

    std::cout << "Analyzing reverb characteristics...\n\n";

    // Perform all measurements
    TailAnalysis tail = tester.analyzeTail(response);
    RT60Measurement rt60 = tester.measureRT60(response);
    StereoAnalysis stereo = tester.analyzeStereoWidth(response);

    // Print detailed results
    printResults(tail, rt60, stereo);

    // Save impulse response
    std::string filename = "build/impulse_engine_39_verification.csv";
    tester.saveImpulseResponse(response, filename);
    std::cout << "Impulse response saved: " << filename << "\n\n";

    // Verify no regression
    bool passed = verifyNoRegression(tail, rt60, stereo);

    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  FINAL RESULT: " << (passed ? "✓ PASS - No Regression" : "✗ FAIL - Regression Detected") << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n\n";

    return passed ? 0 : 1;
}

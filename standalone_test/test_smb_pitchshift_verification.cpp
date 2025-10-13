/*
 * SMBPitchShiftFixed Deep Verification Test
 * =========================================
 *
 * Comprehensive scientific verification of Engine 34 (SMBPitchShiftFixed)
 * Tests accuracy, quality, stability, and edge cases with scientific rigor.
 *
 * Test Categories:
 * 1. Frequency Accuracy: Measure output frequency vs expected (±5 cents target)
 * 2. Quality Metrics: THD, harmonic content, artifacts
 * 3. Stability: Long-duration tests, parameter changes, edge cases
 * 4. Latency: Verify latency reporting and actual delay
 *
 * Author: Deep Verification System
 * Date: 2025-10-11
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <complex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include "../JUCE_Plugin/Source/SMBPitchShiftFixed.h"

// Constants
constexpr double SAMPLE_RATE = 44100.0;
constexpr int BLOCK_SIZE = 512;
constexpr double PI = 3.14159265358979323846;
constexpr double CENTS_PER_SEMITONE = 100.0;
constexpr double TARGET_ACCURACY_CENTS = 5.0;
constexpr double TARGET_THD_PERCENT = 5.0;

// Test results structure
struct FrequencyTest {
    double inputHz;
    double shiftSemitones;
    double expectedHz;
    double measuredHz;
    double errorCents;
    double thd;
    bool passed;
};

struct TestSummary {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    double maxErrorCents = 0.0;
    double avgErrorCents = 0.0;
    double maxTHD = 0.0;
    double avgTHD = 0.0;
    bool productionReady = false;
};

// ============================================================================
// SIGNAL GENERATION
// ============================================================================

std::vector<float> generateSineWave(double frequency, double sampleRate, int numSamples, double amplitude = 0.5) {
    std::vector<float> signal(numSamples);
    double phase = 0.0;
    double phaseIncrement = 2.0 * PI * frequency / sampleRate;

    for (int i = 0; i < numSamples; ++i) {
        signal[i] = static_cast<float>(amplitude * std::sin(phase));
        phase += phaseIncrement;
        if (phase >= 2.0 * PI) {
            phase -= 2.0 * PI;
        }
    }
    return signal;
}

std::vector<float> generateChord(const std::vector<double>& frequencies, double sampleRate, int numSamples, double amplitude = 0.3) {
    std::vector<float> signal(numSamples, 0.0f);

    for (double freq : frequencies) {
        auto sine = generateSineWave(freq, sampleRate, numSamples, amplitude);
        for (int i = 0; i < numSamples; ++i) {
            signal[i] += sine[i];
        }
    }

    // Normalize
    float maxVal = *std::max_element(signal.begin(), signal.end(),
        [](float a, float b) { return std::abs(a) < std::abs(b); });
    if (maxVal > 0.0f) {
        float scale = 0.8f / maxVal;
        for (auto& s : signal) {
            s *= scale;
        }
    }

    return signal;
}

// ============================================================================
// FREQUENCY ANALYSIS
// ============================================================================

double autocorrelationPitch(const std::vector<float>& signal, double sampleRate, double minFreq = 50.0, double maxFreq = 2000.0) {
    int n = static_cast<int>(signal.size());
    int minLag = static_cast<int>(sampleRate / maxFreq);
    int maxLag = static_cast<int>(sampleRate / minFreq);

    // Calculate mean
    double mean = std::accumulate(signal.begin(), signal.end(), 0.0) / n;

    // Center the signal
    std::vector<double> centered(n);
    for (int i = 0; i < n; ++i) {
        centered[i] = signal[i] - mean;
    }

    // Find autocorrelation peak
    double bestCorrelation = -1.0;
    int bestLag = minLag;

    for (int lag = minLag; lag <= maxLag && lag < n / 2; ++lag) {
        double correlation = 0.0;
        double energy = 0.0;

        for (int i = 0; i < n - lag; ++i) {
            correlation += centered[i] * centered[i + lag];
            energy += centered[i] * centered[i];
        }

        if (energy > 1e-10) {
            correlation /= energy;

            if (correlation > bestCorrelation) {
                bestCorrelation = correlation;
                bestLag = lag;
            }
        }
    }

    // Return frequency
    if (bestCorrelation > 0.3) {  // Threshold for valid detection
        return sampleRate / bestLag;
    }

    return 0.0;  // Failed to detect
}

// FFT-based frequency detection (more accurate for pure tones)
double fftPitch(const std::vector<float>& signal, double sampleRate) {
    int n = static_cast<int>(signal.size());

    // Simple magnitude spectrum
    std::vector<double> magnitude(n / 2);

    for (int k = 0; k < n / 2; ++k) {
        double real = 0.0;
        double imag = 0.0;

        for (int i = 0; i < n; ++i) {
            double angle = 2.0 * PI * k * i / n;
            real += signal[i] * std::cos(angle);
            imag += signal[i] * std::sin(angle);
        }

        magnitude[k] = std::sqrt(real * real + imag * imag);
    }

    // Find peak (ignore DC bin 0)
    int peakBin = 1;
    double peakMag = magnitude[1];

    for (int k = 2; k < n / 2; ++k) {
        if (magnitude[k] > peakMag) {
            peakMag = magnitude[k];
            peakBin = k;
        }
    }

    // Parabolic interpolation for sub-bin accuracy
    if (peakBin > 0 && peakBin < n / 2 - 1) {
        double alpha = magnitude[peakBin - 1];
        double beta = magnitude[peakBin];
        double gamma = magnitude[peakBin + 1];

        double p = 0.5 * (alpha - gamma) / (alpha - 2.0 * beta + gamma);
        double binFreq = (peakBin + p) * sampleRate / n;

        return binFreq;
    }

    return peakBin * sampleRate / n;
}

// ============================================================================
// QUALITY METRICS
// ============================================================================

double calculateTHD(const std::vector<float>& signal, double fundamentalFreq, double sampleRate) {
    int n = static_cast<int>(signal.size());

    // Measure fundamental and harmonics
    auto measureFrequency = [&](double freq) -> double {
        double real = 0.0;
        double imag = 0.0;

        for (int i = 0; i < n; ++i) {
            double angle = 2.0 * PI * freq * i / sampleRate;
            real += signal[i] * std::cos(angle);
            imag += signal[i] * std::sin(angle);
        }

        return std::sqrt(real * real + imag * imag);
    };

    double fundamental = measureFrequency(fundamentalFreq);

    // Measure harmonics (2nd through 10th)
    double harmonicsSum = 0.0;
    for (int h = 2; h <= 10; ++h) {
        double harmonic = measureFrequency(fundamentalFreq * h);
        harmonicsSum += harmonic * harmonic;
    }

    if (fundamental < 1e-10) {
        return 100.0;  // Invalid
    }

    double thd = std::sqrt(harmonicsSum) / fundamental * 100.0;
    return thd;
}

double calculateSNR(const std::vector<float>& signal, const std::vector<float>& reference) {
    if (signal.size() != reference.size()) {
        return 0.0;
    }

    double signalPower = 0.0;
    double noisePower = 0.0;

    for (size_t i = 0; i < signal.size(); ++i) {
        double s = reference[i];
        double n = signal[i] - reference[i];
        signalPower += s * s;
        noisePower += n * n;
    }

    if (noisePower < 1e-10) {
        return 120.0;  // Very high SNR
    }

    return 10.0 * std::log10(signalPower / noisePower);
}

bool containsNaNOrInf(const std::vector<float>& signal) {
    for (float s : signal) {
        if (std::isnan(s) || std::isinf(s)) {
            return true;
        }
    }
    return false;
}

bool isSilent(const std::vector<float>& signal, float threshold = 1e-6f) {
    for (float s : signal) {
        if (std::abs(s) > threshold) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// FREQUENCY TO CENTS CONVERSION
// ============================================================================

double hzToCents(double hz1, double hz2) {
    if (hz1 <= 0.0 || hz2 <= 0.0) {
        return 0.0;
    }
    return 1200.0 * std::log2(hz2 / hz1);
}

double semitonesToRatio(double semitones) {
    return std::pow(2.0, semitones / 12.0);
}

// ============================================================================
// TEST EXECUTION
// ============================================================================

FrequencyTest runFrequencyTest(double inputHz, double shiftSemitones, double sampleRate) {
    FrequencyTest result;
    result.inputHz = inputHz;
    result.shiftSemitones = shiftSemitones;
    result.expectedHz = inputHz * semitonesToRatio(shiftSemitones);

    // Create engine
    SMBPitchShiftFixed engine;
    engine.prepare(sampleRate, BLOCK_SIZE);
    engine.reset();

    // Generate input signal (0.5 seconds for stable measurement - faster testing)
    int numSamples = static_cast<int>(sampleRate * 0.5);
    auto input = generateSineWave(inputHz, sampleRate, numSamples);

    std::vector<float> output(numSamples);

    // Process in blocks
    float pitchRatio = static_cast<float>(semitonesToRatio(shiftSemitones));

    for (int i = 0; i < numSamples; i += BLOCK_SIZE) {
        int blockSize = std::min(BLOCK_SIZE, numSamples - i);
        engine.process(input.data() + i, output.data() + i, blockSize, pitchRatio);
    }

    // Skip initial samples for latency/stabilization (skip 0.1 seconds)
    int skipSamples = static_cast<int>(sampleRate * 0.1);
    std::vector<float> analysisWindow(output.begin() + skipSamples, output.end());

    // Measure output frequency (use both methods and prefer FFT for pure tones)
    double measuredHz1 = fftPitch(analysisWindow, sampleRate);
    double measuredHz2 = autocorrelationPitch(analysisWindow, sampleRate);

    // Use FFT result if reasonable, otherwise autocorrelation
    result.measuredHz = measuredHz1;
    if (measuredHz1 <= 0.0 || std::abs(measuredHz1 - result.expectedHz) > result.expectedHz * 0.5) {
        result.measuredHz = measuredHz2;
    }

    // Calculate error in cents
    result.errorCents = hzToCents(result.expectedHz, result.measuredHz);

    // Calculate THD
    result.thd = calculateTHD(analysisWindow, result.measuredHz, sampleRate);

    // Check for validity
    bool frequencyValid = (result.measuredHz > 0.0);
    bool accuracyGood = std::abs(result.errorCents) <= TARGET_ACCURACY_CENTS;
    bool thdGood = result.thd <= TARGET_THD_PERCENT;
    bool noArtifacts = !containsNaNOrInf(output);
    bool notSilent = !isSilent(analysisWindow);

    result.passed = frequencyValid && accuracyGood && thdGood && noArtifacts && notSilent;

    return result;
}

// ============================================================================
// STABILITY TESTS
// ============================================================================

bool testLongDuration(double inputHz, double shiftSemitones, double duration = 2.0) {
    SMBPitchShiftFixed engine;
    engine.prepare(SAMPLE_RATE, BLOCK_SIZE);
    engine.reset();

    int totalSamples = static_cast<int>(SAMPLE_RATE * duration);
    float pitchRatio = static_cast<float>(semitonesToRatio(shiftSemitones));

    std::vector<float> input(BLOCK_SIZE);
    std::vector<float> output(BLOCK_SIZE);

    double phase = 0.0;
    double phaseIncrement = 2.0 * PI * inputHz / SAMPLE_RATE;

    int samplesProcessed = 0;

    while (samplesProcessed < totalSamples) {
        // Generate input block
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            input[i] = 0.5f * std::sin(phase);
            phase += phaseIncrement;
            if (phase >= 2.0 * PI) {
                phase -= 2.0 * PI;
            }
        }

        // Process
        engine.process(input.data(), output.data(), BLOCK_SIZE, pitchRatio);

        // Check for problems
        if (containsNaNOrInf(output)) {
            std::cout << "    FAIL: NaN/Inf detected at sample " << samplesProcessed << std::endl;
            return false;
        }

        if (isSilent(output) && samplesProcessed > SAMPLE_RATE) {  // Allow initial latency
            std::cout << "    FAIL: Unexpected silence at sample " << samplesProcessed << std::endl;
            return false;
        }

        samplesProcessed += BLOCK_SIZE;
    }

    return true;
}

bool testParameterChanges() {
    SMBPitchShiftFixed engine;
    engine.prepare(SAMPLE_RATE, BLOCK_SIZE);
    engine.reset();

    std::vector<float> input = generateSineWave(440.0, SAMPLE_RATE, BLOCK_SIZE);
    std::vector<float> output(BLOCK_SIZE);

    // Test rapid parameter changes
    std::vector<double> shifts = {0, 12, -12, 7, -7, 5, -5, 2, -2, 0};

    for (double shift : shifts) {
        float pitchRatio = static_cast<float>(semitonesToRatio(shift));
        engine.process(input.data(), output.data(), BLOCK_SIZE, pitchRatio);

        if (containsNaNOrInf(output)) {
            std::cout << "    FAIL: NaN/Inf with shift " << shift << " semitones" << std::endl;
            return false;
        }
    }

    return true;
}

bool testEdgeCases() {
    SMBPitchShiftFixed engine;
    engine.prepare(SAMPLE_RATE, BLOCK_SIZE);

    std::vector<float> output(BLOCK_SIZE);
    bool allPassed = true;

    // Test 1: DC offset
    std::vector<float> dcSignal(BLOCK_SIZE, 0.5f);
    engine.reset();
    engine.process(dcSignal.data(), output.data(), BLOCK_SIZE, 1.0f);
    if (containsNaNOrInf(output)) {
        std::cout << "    FAIL: DC offset handling" << std::endl;
        allPassed = false;
    }

    // Test 2: Silence
    std::vector<float> silence(BLOCK_SIZE, 0.0f);
    engine.reset();
    engine.process(silence.data(), output.data(), BLOCK_SIZE, 2.0f);
    if (containsNaNOrInf(output)) {
        std::cout << "    FAIL: Silence handling" << std::endl;
        allPassed = false;
    }

    // Test 3: Extreme shifts
    auto signal = generateSineWave(440.0, SAMPLE_RATE, BLOCK_SIZE);

    for (double shift : {-24.0, -18.0, 18.0, 24.0}) {
        engine.reset();
        float pitchRatio = static_cast<float>(semitonesToRatio(shift));
        engine.process(signal.data(), output.data(), BLOCK_SIZE, pitchRatio);

        if (containsNaNOrInf(output)) {
            std::cout << "    FAIL: Extreme shift " << shift << " semitones" << std::endl;
            allPassed = false;
        }
    }

    // Test 4: Very low frequency
    auto lowFreq = generateSineWave(55.0, SAMPLE_RATE, BLOCK_SIZE);
    engine.reset();
    engine.process(lowFreq.data(), output.data(), BLOCK_SIZE, 2.0f);
    if (containsNaNOrInf(output)) {
        std::cout << "    FAIL: Low frequency (55 Hz) handling" << std::endl;
        allPassed = false;
    }

    // Test 5: Very high frequency
    auto highFreq = generateSineWave(8000.0, SAMPLE_RATE, BLOCK_SIZE);
    engine.reset();
    engine.process(highFreq.data(), output.data(), BLOCK_SIZE, 0.5f);
    if (containsNaNOrInf(output)) {
        std::cout << "    FAIL: High frequency (8000 Hz) handling" << std::endl;
        allPassed = false;
    }

    return allPassed;
}

// ============================================================================
// LATENCY TEST
// ============================================================================

bool testLatency() {
    SMBPitchShiftFixed engine;
    engine.prepare(SAMPLE_RATE, BLOCK_SIZE);
    engine.reset();

    int reportedLatency = engine.getLatencySamples();
    std::cout << "  Reported latency: " << reportedLatency << " samples ("
              << (reportedLatency / SAMPLE_RATE * 1000.0) << " ms)" << std::endl;

    // Generate impulse
    std::vector<float> input(BLOCK_SIZE * 20, 0.0f);
    input[BLOCK_SIZE] = 1.0f;  // Impulse after one block

    std::vector<float> output(input.size(), 0.0f);

    // Process
    for (size_t i = 0; i < input.size(); i += BLOCK_SIZE) {
        engine.process(input.data() + i, output.data() + i, BLOCK_SIZE, 1.0f);
    }

    // Find output peak
    int peakIndex = 0;
    float peakValue = 0.0f;
    for (size_t i = 0; i < output.size(); ++i) {
        if (std::abs(output[i]) > peakValue) {
            peakValue = std::abs(output[i]);
            peakIndex = i;
        }
    }

    int measuredLatency = peakIndex - BLOCK_SIZE;
    std::cout << "  Measured latency: " << measuredLatency << " samples ("
              << (measuredLatency / SAMPLE_RATE * 1000.0) << " ms)" << std::endl;

    bool latencyReasonable = (reportedLatency > 0 && reportedLatency < SAMPLE_RATE * 0.2);  // < 200ms

    return latencyReasonable;
}

// ============================================================================
// MAIN TEST SUITE
// ============================================================================

void printHeader(const std::string& title) {
    std::cout << "\n";
    std::cout << "========================================";
    std::cout << "========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================";
    std::cout << "========================================" << std::endl;
}

void printTestResult(const FrequencyTest& result) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  " << std::setw(8) << result.inputHz << " Hz -> "
              << std::setw(8) << result.expectedHz << " Hz ("
              << std::setw(6) << (result.shiftSemitones >= 0 ? "+" : "") << result.shiftSemitones << " st): "
              << "measured=" << std::setw(8) << result.measuredHz << " Hz, "
              << "error=" << std::setw(7) << result.errorCents << " cents, "
              << "THD=" << std::setw(5) << result.thd << "% "
              << (result.passed ? "[PASS]" : "[FAIL]") << std::endl;
}

int main() {
    printHeader("SMBPitchShiftFixed DEEP VERIFICATION");
    std::cout << "Engine: SMBPitchShiftFixed (Engine 34)" << std::endl;
    std::cout << "Algorithm: Signalsmith Stretch (Phase-vocoder based)" << std::endl;
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Block Size: " << BLOCK_SIZE << " samples" << std::endl;
    std::cout << "Target Accuracy: ±" << TARGET_ACCURACY_CENTS << " cents" << std::endl;
    std::cout << "Target THD: < " << TARGET_THD_PERCENT << "%" << std::endl;

    std::vector<FrequencyTest> allResults;
    TestSummary summary;

    // ========================================================================
    // TEST 1: FREQUENCY ACCURACY
    // ========================================================================

    printHeader("TEST 1: FREQUENCY ACCURACY");

    std::vector<double> testFrequencies = {55, 110, 220, 440, 880};  // A1 to A5
    std::vector<double> testShifts = {-12, -7, -5, -2, 0, 2, 5, 7, 12};

    std::cout << "\nTesting all frequency × shift combinations..." << std::endl;

    for (double freq : testFrequencies) {
        std::cout << "\nInput frequency: " << freq << " Hz" << std::endl;

        for (double shift : testShifts) {
            auto result = runFrequencyTest(freq, shift, SAMPLE_RATE);
            printTestResult(result);
            allResults.push_back(result);

            summary.totalTests++;
            if (result.passed) {
                summary.passedTests++;
            } else {
                summary.failedTests++;
            }

            summary.maxErrorCents = std::max(summary.maxErrorCents, std::abs(result.errorCents));
            summary.maxTHD = std::max(summary.maxTHD, result.thd);
        }
    }

    // Calculate averages
    double totalError = 0.0;
    double totalTHD = 0.0;
    for (const auto& result : allResults) {
        totalError += std::abs(result.errorCents);
        totalTHD += result.thd;
    }
    summary.avgErrorCents = totalError / allResults.size();
    summary.avgTHD = totalTHD / allResults.size();

    // ========================================================================
    // TEST 2: STABILITY TESTS
    // ========================================================================

    printHeader("TEST 2: STABILITY TESTS");

    std::cout << "\nTest 2.1: Long duration processing (2 seconds @ A4, +7 semitones)..." << std::endl;
    bool longDurationPass = testLongDuration(440.0, 7.0, 2.0);
    std::cout << "  " << (longDurationPass ? "[PASS]" : "[FAIL]") << std::endl;

    std::cout << "\nTest 2.2: Rapid parameter changes..." << std::endl;
    bool paramChangesPass = testParameterChanges();
    std::cout << "  " << (paramChangesPass ? "[PASS]" : "[FAIL]") << std::endl;

    // ========================================================================
    // TEST 3: EDGE CASES
    // ========================================================================

    printHeader("TEST 3: EDGE CASES");

    std::cout << "\nTesting edge cases..." << std::endl;
    bool edgeCasesPass = testEdgeCases();
    std::cout << "  " << (edgeCasesPass ? "[PASS]" : "[FAIL]") << std::endl;

    // ========================================================================
    // TEST 4: LATENCY
    // ========================================================================

    printHeader("TEST 4: LATENCY VERIFICATION");

    std::cout << "\nMeasuring latency..." << std::endl;
    bool latencyPass = testLatency();
    std::cout << "  " << (latencyPass ? "[PASS]" : "[FAIL]") << std::endl;

    // ========================================================================
    // FINAL SUMMARY
    // ========================================================================

    printHeader("VERIFICATION SUMMARY");

    std::cout << "\nAccuracy Tests:" << std::endl;
    std::cout << "  Total tests: " << summary.totalTests << std::endl;
    std::cout << "  Passed: " << summary.passedTests << std::endl;
    std::cout << "  Failed: " << summary.failedTests << std::endl;
    std::cout << "  Pass rate: " << std::fixed << std::setprecision(1)
              << (100.0 * summary.passedTests / summary.totalTests) << "%" << std::endl;

    std::cout << "\nFrequency Accuracy:" << std::endl;
    std::cout << "  Average error: " << std::fixed << std::setprecision(2)
              << summary.avgErrorCents << " cents" << std::endl;
    std::cout << "  Maximum error: " << std::fixed << std::setprecision(2)
              << summary.maxErrorCents << " cents" << std::endl;
    std::cout << "  Target: ±" << TARGET_ACCURACY_CENTS << " cents" << std::endl;

    bool accuracyTarget = summary.maxErrorCents <= TARGET_ACCURACY_CENTS;
    std::cout << "  Result: " << (accuracyTarget ? "[PASS]" : "[FAIL]") << std::endl;

    std::cout << "\nQuality Metrics:" << std::endl;
    std::cout << "  Average THD: " << std::fixed << std::setprecision(2)
              << summary.avgTHD << "%" << std::endl;
    std::cout << "  Maximum THD: " << std::fixed << std::setprecision(2)
              << summary.maxTHD << "%" << std::endl;
    std::cout << "  Target: < " << TARGET_THD_PERCENT << "%" << std::endl;

    bool thdTarget = summary.maxTHD <= TARGET_THD_PERCENT;
    std::cout << "  Result: " << (thdTarget ? "[PASS]" : "[FAIL]") << std::endl;

    std::cout << "\nStability Tests:" << std::endl;
    std::cout << "  Long duration: " << (longDurationPass ? "[PASS]" : "[FAIL]") << std::endl;
    std::cout << "  Parameter changes: " << (paramChangesPass ? "[PASS]" : "[FAIL]") << std::endl;
    std::cout << "  Edge cases: " << (edgeCasesPass ? "[PASS]" : "[FAIL]") << std::endl;
    std::cout << "  Latency: " << (latencyPass ? "[PASS]" : "[FAIL]") << std::endl;

    bool allStabilityPass = longDurationPass && paramChangesPass && edgeCasesPass && latencyPass;

    // ========================================================================
    // FINAL VERDICT
    // ========================================================================

    printHeader("FINAL VERDICT");

    bool overallPass = (summary.failedTests == 0) && accuracyTarget && thdTarget && allStabilityPass;

    summary.productionReady = overallPass && (summary.avgErrorCents <= TARGET_ACCURACY_CENTS / 2.0);

    std::cout << "\nDoes SMBPitchShiftFixed work correctly? ";
    if (overallPass) {
        std::cout << "YES ✓" << std::endl;
        std::cout << "\nThe engine passes all verification tests with:" << std::endl;
        std::cout << "  - Frequency accuracy within ±" << TARGET_ACCURACY_CENTS << " cents" << std::endl;
        std::cout << "  - THD below " << TARGET_THD_PERCENT << "%" << std::endl;
        std::cout << "  - Stable processing without artifacts" << std::endl;
        std::cout << "  - Proper edge case handling" << std::endl;
    } else {
        std::cout << "NO ✗" << std::endl;
        std::cout << "\nThe engine failed the following:" << std::endl;
        if (!accuracyTarget) {
            std::cout << "  - Frequency accuracy: " << summary.maxErrorCents << " cents exceeds target" << std::endl;
        }
        if (!thdTarget) {
            std::cout << "  - THD: " << summary.maxTHD << "% exceeds target" << std::endl;
        }
        if (summary.failedTests > 0) {
            std::cout << "  - " << summary.failedTests << " frequency tests failed" << std::endl;
        }
        if (!allStabilityPass) {
            std::cout << "  - Stability tests failed" << std::endl;
        }
    }

    std::cout << "\nProduction ready? ";
    if (summary.productionReady) {
        std::cout << "YES ✓" << std::endl;
        std::cout << "The engine is ready for production use." << std::endl;
    } else {
        std::cout << "NO ✗" << std::endl;
        if (overallPass) {
            std::cout << "The engine works correctly but could benefit from further tuning." << std::endl;
        } else {
            std::cout << "The engine requires fixes before production deployment." << std::endl;
        }
    }

    std::cout << "\n" << std::endl;

    // Save detailed results to file
    std::ofstream csvFile("smb_pitchshift_results.csv");
    csvFile << "InputHz,ShiftSemitones,ExpectedHz,MeasuredHz,ErrorCents,THD,Passed\n";
    for (const auto& result : allResults) {
        csvFile << result.inputHz << ","
                << result.shiftSemitones << ","
                << result.expectedHz << ","
                << result.measuredHz << ","
                << result.errorCents << ","
                << result.thd << ","
                << (result.passed ? "PASS" : "FAIL") << "\n";
    }
    csvFile.close();

    std::cout << "Detailed results saved to: smb_pitchshift_results.csv" << std::endl;

    return overallPass ? 0 : 1;
}

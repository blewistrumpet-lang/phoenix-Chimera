/**
 * test_8_engines_regression.cpp
 *
 * Comprehensive regression test suite for 8 modified engines:
 * - Engine 39: Spring Reverb
 * - Engine 40: Shimmer Reverb
 * - Engine 52: Pitch Shifter
 * - Engine 32: Harmonizer
 * - Engine 49: Detune Doubler
 * - Engine 20: Muff Fuzz
 * - Engine 33: Octave Up
 * - Engine 41: Convolution Reverb
 *
 * Tests: Impulse response, audio quality, output level, crashes, THD, CPU
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <chrono>
#include <vector>
#include <map>
#include <string>

#include "JuceHeader.h"
#include "EngineBase.h"
#include "EngineFactory.h"
#include "EngineTypes.h"

//==============================================================================
// Test Result Structure
//==============================================================================
struct EngineTestResult {
    int engineId;
    std::string engineName;

    // Test categories
    bool impulseTest;
    bool qualityTest;
    bool stabilityTest;
    bool performanceTest;

    // Metrics
    float peakOutput;
    float rmsOutput;
    float thd;
    float cpuPercent;
    bool hasNaN;
    bool hasInf;
    bool crashed;

    // Overall
    bool passed;
    std::string failReason;

    EngineTestResult() : engineId(0), impulseTest(false), qualityTest(false),
                         stabilityTest(false), performanceTest(false),
                         peakOutput(0), rmsOutput(0), thd(0), cpuPercent(0),
                         hasNaN(false), hasInf(false), crashed(false), passed(false) {}
};

//==============================================================================
// Signal Generation
//==============================================================================
void generateImpulse(juce::AudioBuffer<float>& buffer) {
    buffer.clear();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.setSample(ch, 0, 1.0f);
    }
}

void generateSineWave(juce::AudioBuffer<float>& buffer, float freq, double sampleRate, float amplitude = 0.5f) {
    const float omega = 2.0f * juce::MathConstants<float>::pi * freq / static_cast<float>(sampleRate);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = amplitude * std::sin(omega * static_cast<float>(i));
        }
    }
}

void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float amplitude = 0.3f) {
    juce::Random random;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = amplitude * (random.nextFloat() * 2.0f - 1.0f);
        }
    }
}

//==============================================================================
// Analysis Functions
//==============================================================================
float calculatePeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            peak = std::max(peak, std::abs(data[i]));
        }
    }
    return peak;
}

float calculateRMS(const juce::AudioBuffer<float>& buffer) {
    double sumSquares = 0.0;
    int totalSamples = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sumSquares += data[i] * data[i];
        }
        totalSamples += buffer.getNumSamples();
    }
    return std::sqrt(static_cast<float>(sumSquares / totalSamples));
}

bool hasNaN(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::isnan(data[i])) return true;
        }
    }
    return false;
}

bool hasInf(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::isinf(data[i])) return true;
        }
    }
    return false;
}

float calculateTHD(const juce::AudioBuffer<float>& buffer, float fundamentalFreq, double sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13); // 2^13 = 8192
    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy and window
    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    int fundamentalBin = static_cast<int>(fundamentalFreq * fftSize / sampleRate);
    float fundamentalMag = fftData[fundamentalBin];

    float harmonicsSumSquared = 0.0f;
    for (int h = 2; h <= 6; ++h) {
        int harmonicBin = fundamentalBin * h;
        if (harmonicBin < fftSize / 2) {
            float harmonicMag = fftData[harmonicBin];
            harmonicsSumSquared += harmonicMag * harmonicMag;
        }
    }

    if (fundamentalMag < 1e-10f) return 0.0f;
    return (std::sqrt(harmonicsSumSquared) / fundamentalMag) * 100.0f;
}

void saveImpulseResponse(const juce::AudioBuffer<float>& buffer, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "Sample,L,R\n";
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        file << i << ","
             << buffer.getSample(0, i) << ","
             << buffer.getSample(1, i) << "\n";
    }
    file.close();
}

//==============================================================================
// Engine Test Runner
//==============================================================================
class EngineRegressionTester {
public:
    EngineRegressionTester() : sampleRate(48000.0), blockSize(512) {}

    EngineTestResult testEngine(int engineId) {
        EngineTestResult result;
        result.engineId = engineId;

        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "Testing Engine " << engineId << "\n";
        std::cout << std::string(70, '=') << "\n";

        // Create engine
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.crashed = true;
            result.failReason = "Failed to create engine";
            return result;
        }

        result.engineName = engine->getName().toStdString();
        std::cout << "Engine: " << result.engineName << "\n";

        try {
            // Prepare
            engine->prepareToPlay(sampleRate, blockSize);
            engine->reset();

            // Test 1: Impulse Response
            std::cout << "\n[1/4] Impulse Response Test... ";
            result.impulseTest = testImpulse(engine.get(), result);
            std::cout << (result.impulseTest ? "PASS" : "FAIL") << "\n";

            // Test 2: Quality Test
            std::cout << "[2/4] Audio Quality Test... ";
            result.qualityTest = testQuality(engine.get(), result);
            std::cout << (result.qualityTest ? "PASS" : "FAIL") << "\n";

            // Test 3: Stability Test
            std::cout << "[3/4] Stability Test... ";
            result.stabilityTest = testStability(engine.get(), result);
            std::cout << (result.stabilityTest ? "PASS" : "FAIL") << "\n";

            // Test 4: Performance Test
            std::cout << "[4/4] Performance Test... ";
            result.performanceTest = testPerformance(engine.get(), result);
            std::cout << (result.performanceTest ? "PASS" : "FAIL") << "\n";

            result.passed = result.impulseTest && result.qualityTest &&
                           result.stabilityTest && result.performanceTest;

        } catch (const std::exception& e) {
            result.crashed = true;
            result.failReason = std::string("Exception: ") + e.what();
        } catch (...) {
            result.crashed = true;
            result.failReason = "Unknown exception";
        }

        return result;
    }

private:
    double sampleRate;
    int blockSize;

    bool testImpulse(EngineBase* engine, EngineTestResult& result) {
        // Process impulse through engine
        juce::AudioBuffer<float> impulseBuffer(2, 48000); // 1 second
        generateImpulse(impulseBuffer);

        // Process in blocks
        int pos = 0;
        juce::AudioBuffer<float> fullOutput(2, 48000);
        fullOutput.clear();

        while (pos < impulseBuffer.getNumSamples()) {
            int samplesToProcess = std::min(blockSize, impulseBuffer.getNumSamples() - pos);
            juce::AudioBuffer<float> block(2, samplesToProcess);

            for (int ch = 0; ch < 2; ++ch) {
                block.copyFrom(ch, 0, impulseBuffer, ch, pos, samplesToProcess);
            }

            engine->process(block);

            for (int ch = 0; ch < 2; ++ch) {
                fullOutput.copyFrom(ch, pos, block, ch, 0, samplesToProcess);
            }

            pos += samplesToProcess;
        }

        // Save impulse response
        std::string filename = "build/impulse_engine_" + std::to_string(result.engineId) + ".csv";
        saveImpulseResponse(fullOutput, filename);
        std::cout << "(saved to " << filename << ") ";

        // Check for NaN/Inf
        if (hasNaN(fullOutput)) {
            result.hasNaN = true;
            result.failReason = "NaN in impulse response";
            return false;
        }

        if (hasInf(fullOutput)) {
            result.hasInf = true;
            result.failReason = "Inf in impulse response";
            return false;
        }

        // Check for reasonable output
        float peak = calculatePeak(fullOutput);
        result.peakOutput = peak;

        if (peak < 1e-6f) {
            result.failReason = "No output (silence)";
            return false;
        }

        if (peak > 10.0f) {
            result.failReason = "Excessive output level: " + std::to_string(peak);
            return false;
        }

        return true;
    }

    bool testQuality(EngineBase* engine, EngineTestResult& result) {
        engine->reset();

        // Generate 1kHz sine wave
        juce::AudioBuffer<float> buffer(2, 8192);
        generateSineWave(buffer, 1000.0f, sampleRate, 0.3f);

        engine->process(buffer);

        // Check for NaN/Inf
        if (hasNaN(buffer) || hasInf(buffer)) {
            result.failReason = "NaN/Inf in quality test";
            return false;
        }

        // Measure output levels
        result.peakOutput = calculatePeak(buffer);
        result.rmsOutput = calculateRMS(buffer);

        // Calculate THD
        result.thd = calculateTHD(buffer, 1000.0f, sampleRate);

        std::cout << "(Peak: " << std::fixed << std::setprecision(3) << result.peakOutput
                  << ", RMS: " << result.rmsOutput
                  << ", THD: " << std::setprecision(2) << result.thd << "%) ";

        // Quality thresholds
        if (result.peakOutput > 5.0f) {
            result.failReason = "Excessive peak level";
            return false;
        }

        return true;
    }

    bool testStability(EngineBase* engine, EngineTestResult& result) {
        engine->reset();

        juce::AudioBuffer<float> buffer(2, blockSize);

        // Test silence
        buffer.clear();
        engine->process(buffer);
        if (hasNaN(buffer) || hasInf(buffer)) {
            result.failReason = "Unstable with silence";
            return false;
        }

        // Test loud signal
        generateSineWave(buffer, 1000.0f, sampleRate, 2.0f);
        engine->process(buffer);
        if (hasNaN(buffer) || hasInf(buffer)) {
            result.failReason = "Unstable with loud signal";
            return false;
        }

        // Test DC offset
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                buffer.setSample(ch, i, 0.5f);
            }
        }
        engine->process(buffer);
        if (hasNaN(buffer) || hasInf(buffer)) {
            result.failReason = "Unstable with DC";
            return false;
        }

        // Test noise
        generateWhiteNoise(buffer, 1.0f);
        engine->process(buffer);
        if (hasNaN(buffer) || hasInf(buffer)) {
            result.failReason = "Unstable with noise";
            return false;
        }

        // Stress test - rapid parameter changes
        int numParams = engine->getNumParameters();
        for (int i = 0; i < 100; ++i) {
            std::map<int, float> params;
            for (int p = 0; p < numParams; ++p) {
                params[p] = (i % 2 == 0) ? 0.0f : 1.0f;
            }
            engine->updateParameters(params);

            generateSineWave(buffer, 1000.0f, sampleRate);
            engine->process(buffer);

            if (hasNaN(buffer) || hasInf(buffer)) {
                result.failReason = "Unstable with parameter changes";
                return false;
            }
        }

        std::cout << "(100 param sweeps) ";
        return true;
    }

    bool testPerformance(EngineBase* engine, EngineTestResult& result) {
        engine->reset();

        juce::AudioBuffer<float> buffer(2, blockSize);
        generateSineWave(buffer, 1000.0f, sampleRate);

        // Warmup
        for (int i = 0; i < 100; ++i) {
            engine->process(buffer);
        }

        // Measure CPU time
        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            engine->process(buffer);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        double timePerBlock = duration.count() / static_cast<double>(iterations);
        double realTimePerBlock = (blockSize * 1000000.0) / sampleRate;
        result.cpuPercent = static_cast<float>((timePerBlock / realTimePerBlock) * 100.0);

        std::cout << "(CPU: " << std::fixed << std::setprecision(2) << result.cpuPercent << "%) ";

        // Performance threshold
        if (result.cpuPercent > 10.0f) {
            result.failReason = "CPU usage too high";
            return false;
        }

        return true;
    }
};

//==============================================================================
// Report Generation
//==============================================================================
void printTestMatrix(const std::vector<EngineTestResult>& results) {
    std::cout << "\n\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        REGRESSION TEST MATRIX                             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left << std::setw(4) << "ID"
              << std::setw(25) << "Engine"
              << std::setw(8) << "Impulse"
              << std::setw(8) << "Quality"
              << std::setw(10) << "Stability"
              << std::setw(8) << "Perf"
              << std::setw(10) << "Result"
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::left << std::setw(4) << r.engineId
                  << std::setw(25) << r.engineName.substr(0, 24)
                  << std::setw(8) << (r.impulseTest ? "PASS" : "FAIL")
                  << std::setw(8) << (r.qualityTest ? "PASS" : "FAIL")
                  << std::setw(10) << (r.stabilityTest ? "PASS" : "FAIL")
                  << std::setw(8) << (r.performanceTest ? "PASS" : "FAIL")
                  << std::setw(10) << (r.passed ? "✓ PASS" : "✗ FAIL")
                  << "\n";

        if (!r.passed && !r.failReason.empty()) {
            std::cout << "     └─ " << r.failReason << "\n";
        }
    }

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                           DETAILED METRICS                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left << std::setw(4) << "ID"
              << std::setw(25) << "Engine"
              << std::setw(10) << "Peak"
              << std::setw(10) << "RMS"
              << std::setw(10) << "THD%"
              << std::setw(10) << "CPU%"
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::left << std::setw(4) << r.engineId
                  << std::setw(25) << r.engineName.substr(0, 24)
                  << std::fixed << std::setprecision(3)
                  << std::setw(10) << r.peakOutput
                  << std::setw(10) << r.rmsOutput
                  << std::setprecision(2)
                  << std::setw(10) << r.thd
                  << std::setw(10) << r.cpuPercent
                  << "\n";
    }

    // Summary
    int totalTests = results.size();
    int passed = 0;
    int failed = 0;
    int crashed = 0;

    for (const auto& r : results) {
        if (r.crashed) crashed++;
        else if (r.passed) passed++;
        else failed++;
    }

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                              SUMMARY                                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "  Total Engines:     " << totalTests << "\n";
    std::cout << "  Passed:            " << passed << " ("
              << std::fixed << std::setprecision(1) << (passed * 100.0 / totalTests) << "%)\n";
    std::cout << "  Failed:            " << failed << "\n";
    std::cout << "  Crashed:           " << crashed << "\n";
    std::cout << "\n";

    if (passed == totalTests) {
        std::cout << "  ✓ ALL TESTS PASSED - NO REGRESSIONS DETECTED\n\n";
    } else {
        std::cout << "  ✗ REGRESSIONS DETECTED\n\n";
    }
}

void saveCSVReport(const std::vector<EngineTestResult>& results) {
    std::ofstream file("build/regression_test_results.csv");
    if (!file.is_open()) return;

    file << "EngineID,EngineName,ImpulseTest,QualityTest,StabilityTest,PerformanceTest,"
         << "PeakOutput,RMSOutput,THD_Percent,CPU_Percent,Passed,FailReason\n";

    for (const auto& r : results) {
        file << r.engineId << ","
             << "\"" << r.engineName << "\","
             << (r.impulseTest ? "PASS" : "FAIL") << ","
             << (r.qualityTest ? "PASS" : "FAIL") << ","
             << (r.stabilityTest ? "PASS" : "FAIL") << ","
             << (r.performanceTest ? "PASS" : "FAIL") << ","
             << std::fixed << std::setprecision(4)
             << r.peakOutput << ","
             << r.rmsOutput << ","
             << r.thd << ","
             << r.cpuPercent << ","
             << (r.passed ? "PASS" : "FAIL") << ","
             << "\"" << r.failReason << "\"\n";
    }

    file.close();
    std::cout << "CSV report saved: build/regression_test_results.csv\n";
}

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           Comprehensive Regression Test - 8 Modified Engines             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";

    // Define the 8 engines to test
    std::vector<int> engineIds = {39, 40, 52, 32, 49, 20, 33, 41};
    std::vector<std::string> engineDescriptions = {
        "Spring Reverb",
        "Shimmer Reverb",
        "Pitch Shifter",
        "Harmonizer",
        "Detune Doubler",
        "Muff Fuzz",
        "Octave Up",
        "Convolution Reverb"
    };

    std::cout << "\nTesting engines:\n";
    for (size_t i = 0; i < engineIds.size(); ++i) {
        std::cout << "  " << engineIds[i] << ": " << engineDescriptions[i] << "\n";
    }

    EngineRegressionTester tester;
    std::vector<EngineTestResult> results;

    for (int engineId : engineIds) {
        auto result = tester.testEngine(engineId);
        results.push_back(result);
    }

    // Print results
    printTestMatrix(results);

    // Save CSV report
    saveCSVReport(results);

    // Determine exit code
    bool allPassed = true;
    for (const auto& r : results) {
        if (!r.passed) {
            allPassed = false;
            break;
        }
    }

    return allPassed ? 0 : 1;
}

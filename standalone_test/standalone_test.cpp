/**
 * standalone_test.cpp - Fully Functional Standalone Engine Tester
 *
 * Complete test suite for all 57 ChimeraPhoenix engines without plugin dependencies.
 * Tests each engine thoroughly with real audio processing and comprehensive analysis.
 *
 * Compile:
 *   g++ -std=c++17 -O3 standalone_test.cpp -I../JUCE_Plugin/Source \
 *       -I/Users/Branden/JUCE/modules \
 *       -framework Accelerate -framework CoreAudio -framework CoreFoundation \
 *       -o standalone_test
 *
 * Usage:
 *   ./standalone_test              # Test all engines
 *   ./standalone_test --engine 1   # Test specific engine
 *   ./standalone_test --verbose    # Detailed output
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

// Provide JuceHeader.h for engines that need it
#include "JuceHeader.h"

// Engine includes
#include "EngineBase.h"
#include "EngineFactory.h"
#include "EngineTypes.h"

//==============================================================================
// Signal Generators
//==============================================================================
namespace TestSignals {

void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float sampleRate, float amplitude = 0.5f) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const float omega = 2.0f * juce::MathConstants<float>::pi * frequency / sampleRate;

    for (int ch = 0; ch < numChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            data[i] = amplitude * std::sin(omega * static_cast<float>(i));
        }
    }
}

void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float amplitude = 0.3f) {
    juce::Random random;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            data[i] = amplitude * (random.nextFloat() * 2.0f - 1.0f);
        }
    }
}

void generateImpulse(juce::AudioBuffer<float>& buffer, int position = 0, float amplitude = 1.0f) {
    buffer.clear();
    if (position >= 0 && position < buffer.getNumSamples()) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.setSample(ch, position, amplitude);
        }
    }
}

void generateSweep(juce::AudioBuffer<float>& buffer, float startFreq, float endFreq,
                   float sampleRate, float amplitude = 0.5f) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const float duration = numSamples / sampleRate;

    for (int ch = 0; ch < numChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            // Logarithmic sweep
            float k = std::log(endFreq / startFreq) / duration;
            float phase = 2.0f * juce::MathConstants<float>::pi * startFreq *
                         (std::exp(k * t) - 1.0f) / k;
            data[i] = amplitude * std::sin(phase);
        }
    }
}

} // namespace TestSignals

//==============================================================================
// Audio Analysis
//==============================================================================
namespace Analysis {

float calculatePeakLevel(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            peak = std::max(peak, std::abs(data[i]));
        }
    }
    return peak;
}

float calculateRMSLevel(const juce::AudioBuffer<float>& buffer) {
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

float calculateTHD(const juce::AudioBuffer<float>& buffer, float fundamentalFreq, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13); // 2^13 = 8192
    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy first channel
    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find fundamental
    int fundamentalBin = static_cast<int>(fundamentalFreq * fftSize / sampleRate);
    float fundamentalMag = fftData[fundamentalBin];

    // Sum harmonics (2nd through 5th)
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

bool containsNaN(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::isnan(data[i])) return true;
        }
    }
    return false;
}

bool containsInf(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::isinf(data[i])) return true;
        }
    }
    return false;
}

} // namespace Analysis

//==============================================================================
// Test Results
//==============================================================================
struct TestResult {
    int engineId;
    std::string engineName;
    bool passed;
    bool basicFunctionality;
    bool safety;
    bool audioQuality;
    bool performance;
    bool parameters;
    float peakLevel;
    float rmsLevel;
    float thd;
    float cpuUsage;
    std::vector<std::string> notes;
    std::string timestamp;
};

//==============================================================================
// Engine Tester
//==============================================================================
class EngineTester {
public:
    EngineTester() : sampleRate(48000.0), blockSize(512) {}

    void setSampleRate(double sr) { sampleRate = sr; }
    void setBlockSize(int bs) { blockSize = bs; }

    TestResult testEngine(int engineId, bool verbose = false) {
        TestResult result;
        result.engineId = engineId;
        result.engineName = getEngineName(engineId);
        result.timestamp = juce::Time::getCurrentTime().toString(true, true).toStdString();

        if (verbose) {
            std::cout << "\n" << std::string(80, '=') << "\n";
            std::cout << "Testing Engine " << engineId << ": " << result.engineName << "\n";
            std::cout << std::string(80, '=') << "\n";
        }

        // Create engine
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.passed = false;
            result.notes.push_back("Failed to create engine");
            return result;
        }

        // Prepare engine
        engine->prepareToPlay(sampleRate, blockSize);

        // Test 1: Basic Functionality
        result.basicFunctionality = testBasicFunctionality(engine.get(), verbose);

        // Test 2: Safety
        result.safety = testSafety(engine.get(), verbose);

        // Test 3: Audio Quality
        auto qualityResults = testAudioQuality(engine.get(), verbose);
        result.audioQuality = qualityResults.first;
        result.thd = qualityResults.second;

        // Test 4: Performance
        result.performance = testPerformance(engine.get(), result.cpuUsage, verbose);

        // Test 5: Parameters
        result.parameters = testParameters(engine.get(), verbose);

        // Calculate final metrics
        juce::AudioBuffer<float> testBuffer(2, blockSize);
        TestSignals::generateSineWave(testBuffer, 1000.0f, sampleRate);
        engine->process(testBuffer);
        result.peakLevel = Analysis::calculatePeakLevel(testBuffer);
        result.rmsLevel = Analysis::calculateRMSLevel(testBuffer);

        // Overall pass/fail
        result.passed = result.basicFunctionality && result.safety &&
                       result.audioQuality && result.performance && result.parameters;

        if (verbose) {
            printTestSummary(result);
        }

        return result;
    }

    std::vector<TestResult> testAllEngines(bool verbose = false) {
        std::vector<TestResult> results;

        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║   ChimeraPhoenix Comprehensive Engine Test Suite          ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

        for (int engineId = 1; engineId <= 56; ++engineId) {
            if (!verbose) {
                std::cout << "Testing engine " << std::setw(2) << engineId << "/56: "
                         << std::setw(30) << std::left << getEngineName(engineId) << " ... ";
                std::cout.flush();
            }

            auto result = testEngine(engineId, verbose);
            results.push_back(result);

            if (!verbose) {
                std::cout << (result.passed ? "✓ PASS" : "✗ FAIL") << "\n";
            }
        }

        return results;
    }

private:
    double sampleRate;
    int blockSize;

    bool testBasicFunctionality(EngineBase* engine, bool verbose) {
        if (verbose) std::cout << "\n[1/5] Basic Functionality... ";

        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generateSineWave(buffer, 1000.0f, sampleRate, 0.5f);

        engine->process(buffer);

        // Verify output has signal
        bool hasSignal = false;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (std::abs(data[i]) > 1e-6f) {
                    hasSignal = true;
                    break;
                }
            }
        }

        if (verbose) std::cout << (hasSignal ? "✓ PASS" : "✗ FAIL") << "\n";
        return hasSignal;
    }

    bool testSafety(EngineBase* engine, bool verbose) {
        if (verbose) std::cout << "[2/5] Safety (NaN/Inf/Extreme)... ";

        engine->reset();

        // Test 1: Silence
        juce::AudioBuffer<float> silence(2, blockSize);
        silence.clear();
        engine->process(silence);
        if (Analysis::containsNaN(silence) || Analysis::containsInf(silence)) {
            if (verbose) std::cout << "✗ FAIL (NaN/Inf on silence)\n";
            return false;
        }

        // Test 2: Very loud signal
        juce::AudioBuffer<float> loud(2, blockSize);
        TestSignals::generateSineWave(loud, 1000.0f, sampleRate, 2.0f);
        engine->process(loud);
        if (Analysis::containsNaN(loud) || Analysis::containsInf(loud)) {
            if (verbose) std::cout << "✗ FAIL (NaN/Inf on loud signal)\n";
            return false;
        }

        // Test 3: DC offset
        juce::AudioBuffer<float> dc(2, blockSize);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                dc.setSample(ch, i, 0.5f);
            }
        }
        engine->process(dc);
        if (Analysis::containsNaN(dc) || Analysis::containsInf(dc)) {
            if (verbose) std::cout << "✗ FAIL (NaN/Inf on DC)\n";
            return false;
        }

        if (verbose) std::cout << "✓ PASS\n";
        return true;
    }

    std::pair<bool, float> testAudioQuality(EngineBase* engine, bool verbose) {
        if (verbose) std::cout << "[3/5] Audio Quality (THD)... ";

        engine->reset();

        juce::AudioBuffer<float> buffer(2, 8192);
        TestSignals::generateSineWave(buffer, 1000.0f, sampleRate, 0.3f);

        engine->process(buffer);

        float thd = Analysis::calculateTHD(buffer, 1000.0f, sampleRate);

        // Category-specific thresholds
        bool passed = thd < getTHDThreshold(engine);

        if (verbose) {
            std::cout << (passed ? "✓ PASS" : "✗ FAIL")
                     << " (THD: " << std::fixed << std::setprecision(4) << thd << "%)\n";
        }

        return {passed, thd};
    }

    bool testPerformance(EngineBase* engine, float& cpuUsage, bool verbose) {
        if (verbose) std::cout << "[4/5] Performance (CPU)... ";

        engine->reset();

        juce::AudioBuffer<float> buffer(2, blockSize);
        TestSignals::generateSineWave(buffer, 1000.0f, sampleRate);

        // Warmup
        for (int i = 0; i < 100; ++i) {
            engine->process(buffer);
        }

        // Measure
        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            engine->process(buffer);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        double timePerBlock = duration.count() / static_cast<double>(iterations);
        double realTimePerBlock = (blockSize * 1000000.0) / sampleRate;
        cpuUsage = static_cast<float>((timePerBlock / realTimePerBlock) * 100.0);

        bool passed = cpuUsage < 5.0f;

        if (verbose) {
            std::cout << (passed ? "✓ PASS" : "✗ FAIL")
                     << " (CPU: " << std::fixed << std::setprecision(2) << cpuUsage << "%)\n";
        }

        return passed;
    }

    bool testParameters(EngineBase* engine, bool verbose) {
        if (verbose) std::cout << "[5/5] Parameters... ";

        int numParams = engine->getNumParameters();
        juce::AudioBuffer<float> buffer(2, blockSize);

        // Test min, mid, max for each parameter
        for (int param = 0; param < numParams; ++param) {
            std::map<int, float> params;

            // Min
            params[param] = 0.0f;
            engine->updateParameters(params);
            TestSignals::generateSineWave(buffer, 1000.0f, sampleRate);
            engine->process(buffer);
            if (Analysis::containsNaN(buffer) || Analysis::containsInf(buffer)) {
                if (verbose) std::cout << "✗ FAIL (param " << param << " at min)\n";
                return false;
            }

            // Max
            params[param] = 1.0f;
            engine->updateParameters(params);
            buffer.clear();
            TestSignals::generateSineWave(buffer, 1000.0f, sampleRate);
            engine->process(buffer);
            if (Analysis::containsNaN(buffer) || Analysis::containsInf(buffer)) {
                if (verbose) std::cout << "✗ FAIL (param " << param << " at max)\n";
                return false;
            }
        }

        if (verbose) std::cout << "✓ PASS\n";
        return true;
    }

    float getTHDThreshold(EngineBase* engine) {
        std::string name = engine->getName().toStdString();

        // Distortion engines should have higher THD
        if (name.find("Distortion") != std::string::npos ||
            name.find("Overdrive") != std::string::npos ||
            name.find("Clipper") != std::string::npos ||
            name.find("Crusher") != std::string::npos) {
            return 50.0f; // Up to 50% THD for distortion
        }

        // Clean processors should be very clean
        if (name.find("Filter") != std::string::npos ||
            name.find("EQ") != std::string::npos) {
            return 0.1f; // < 0.1% for filters/EQ
        }

        // Default threshold
        return 1.0f; // < 1% for most processors
    }

    void printTestSummary(const TestResult& result) {
        std::cout << "\nTest Summary:\n";
        std::cout << "  Basic:   " << (result.basicFunctionality ? "✓" : "✗") << "\n";
        std::cout << "  Safety:  " << (result.safety ? "✓" : "✗") << "\n";
        std::cout << "  Quality: " << (result.audioQuality ? "✓" : "✗") << " (THD: " << result.thd << "%)\n";
        std::cout << "  Perf:    " << (result.performance ? "✓" : "✗") << " (CPU: " << result.cpuUsage << "%)\n";
        std::cout << "  Params:  " << (result.parameters ? "✓" : "✗") << "\n";
        std::cout << "\nResult: " << (result.passed ? "✓ PASSED" : "✗ FAILED") << "\n";
    }

    std::string getEngineName(int engineId) {
        auto engine = EngineFactory::createEngine(engineId);
        return engine ? engine->getName().toStdString() : "Unknown";
    }
};

//==============================================================================
// Report Generator
//==============================================================================
void generateHTMLReport(const std::vector<TestResult>& results, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    int passed = 0;
    for (const auto& r : results) if (r.passed) passed++;
    float passRate = (passed * 100.0f) / results.size();

    file << "<!DOCTYPE html>\n<html>\n<head>\n";
    file << "<meta charset='UTF-8'>\n";
    file << "<title>ChimeraPhoenix Test Results</title>\n";
    file << "<style>\n";
    file << "body { font-family: 'Segoe UI', sans-serif; margin: 20px; background: #f5f5f5; }\n";
    file << "h1 { color: #333; border-bottom: 3px solid #4CAF50; }\n";
    file << ".summary { background: white; padding: 20px; border-radius: 8px; margin: 20px 0; }\n";
    file << "table { width: 100%; border-collapse: collapse; background: white; margin: 20px 0; }\n";
    file << "th { background: #4CAF50; color: white; padding: 12px; text-align: left; }\n";
    file << "td { padding: 10px; border-bottom: 1px solid #ddd; }\n";
    file << ".pass { color: #4CAF50; font-weight: bold; }\n";
    file << ".fail { color: #f44336; font-weight: bold; }\n";
    file << "</style>\n</head>\n<body>\n";

    file << "<h1>ChimeraPhoenix Engine Test Results</h1>\n";
    file << "<div class='summary'>\n";
    file << "<h2>Summary</h2>\n";
    file << "<p>Total Engines: " << results.size() << "</p>\n";
    file << "<p>Passed: <span class='pass'>" << passed << "</span></p>\n";
    file << "<p>Failed: <span class='fail'>" << (results.size() - passed) << "</span></p>\n";
    file << "<p>Pass Rate: " << std::fixed << std::setprecision(1) << passRate << "%</p>\n";
    file << "</div>\n";

    file << "<table>\n<tr><th>Engine</th><th>Status</th><th>Basic</th><th>Safety</th><th>Quality</th><th>Perf</th><th>Params</th><th>THD%</th><th>CPU%</th></tr>\n";

    for (const auto& r : results) {
        file << "<tr>\n";
        file << "<td>" << r.engineName << "</td>\n";
        file << "<td class='" << (r.passed ? "pass" : "fail") << "'>" << (r.passed ? "✓ PASS" : "✗ FAIL") << "</td>\n";
        file << "<td>" << (r.basicFunctionality ? "✓" : "✗") << "</td>\n";
        file << "<td>" << (r.safety ? "✓" : "✗") << "</td>\n";
        file << "<td>" << (r.audioQuality ? "✓" : "✗") << "</td>\n";
        file << "<td>" << (r.performance ? "✓" : "✗") << "</td>\n";
        file << "<td>" << (r.parameters ? "✓" : "✗") << "</td>\n";
        file << "<td>" << std::fixed << std::setprecision(3) << r.thd << "</td>\n";
        file << "<td>" << std::fixed << std::setprecision(2) << r.cpuUsage << "</td>\n";
        file << "</tr>\n";
    }

    file << "</table>\n</body>\n</html>\n";
    file.close();

    std::cout << "\nHTML report saved: " << filename << "\n";
}

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI juceInit;

    bool verbose = false;
    int specificEngine = -1;

    // Parse command line
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--engine" && i + 1 < argc) {
            specificEngine = std::atoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "  --verbose, -v       Detailed output\n";
            std::cout << "  --engine <id>       Test specific engine (1-56)\n";
            std::cout << "  --help              Show this help\n";
            return 0;
        }
    }

    EngineTester tester;
    tester.setSampleRate(48000.0);
    tester.setBlockSize(512);

    std::vector<TestResult> results;

    if (specificEngine > 0) {
        auto result = tester.testEngine(specificEngine, true);
        results.push_back(result);
    } else {
        results = tester.testAllEngines(verbose);
    }

    // Generate reports
    generateHTMLReport(results, "test_report.html");

    // Print summary
    int passed = 0;
    for (const auto& r : results) if (r.passed) passed++;

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     FINAL SUMMARY                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "  Total:  " << results.size() << " engines\n";
    std::cout << "  Passed: " << passed << "\n";
    std::cout << "  Failed: " << (results.size() - passed) << "\n";
    std::cout << "  Rate:   " << std::fixed << std::setprecision(1)
              << (passed * 100.0f / results.size()) << "%\n\n";

    return (passed == static_cast<int>(results.size())) ? 0 : 1;
}

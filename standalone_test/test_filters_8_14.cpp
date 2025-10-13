#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <complex>
#include <algorithm>

/**
 * FOCUSED TEST FOR FILTER ENGINES 8-14
 *
 * Testing:
 * 8.  VintageConsoleEQ_Studio
 * 9.  LadderFilter
 * 10. StateVariableFilter
 * 11. FormantFilter
 * 12. EnvelopeFilter (AutoWah)
 * 13. CombResonator
 * 14. VocalFormantFilter
 */

constexpr float PI = 3.14159265358979323846f;
constexpr int FFT_SIZE = 8192;

//==============================================================================
// SIMPLE TEST FUNCTIONS
//==============================================================================

struct TestResult {
    int engineId;
    std::string engineName;
    bool created;
    bool stable;
    bool respondsToInput;
    float peakOutputLevel;
    std::vector<float> frequencyResponse;
    std::string errorMessage;
};

TestResult testFilterEngine(int engineId, const std::string& name) {
    TestResult result;
    result.engineId = engineId;
    result.engineName = name;
    result.created = false;
    result.stable = false;
    result.respondsToInput = false;
    result.peakOutputLevel = 0.0f;

    std::cout << "\n[ENGINE " << engineId << "] " << name << "\n";
    std::cout << std::string(60, '=') << "\n";

    try {
        // 1. CREATE ENGINE
        std::cout << "  [1/5] Creating engine...";
        auto engine = EngineFactory::createEngine(engineId);

        if (!engine) {
            std::cout << " FAILED (nullptr returned)\n";
            result.errorMessage = "EngineFactory returned nullptr";
            return result;
        }

        std::cout << " OK\n";
        result.created = true;

        // 2. PREPARE TO PLAY
        std::cout << "  [2/5] Preparing to play (48kHz, 512 samples)...";
        float sampleRate = 48000.0f;
        int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);
        std::cout << " OK\n";

        // 3. SET PARAMETERS
        std::cout << "  [3/5] Setting parameters...";
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        std::cout << " (" << numParams << " parameters) ";

        // Generic parameter setup
        if (numParams > 0) params[0] = 1.0f;   // Mix/Wet = 100%
        if (numParams > 1) params[1] = 0.5f;   // Cutoff/Frequency = middle
        if (numParams > 2) params[2] = 0.7f;   // Resonance/Q = moderate
        if (numParams > 3) params[3] = 0.5f;   // Additional parameter
        if (numParams > 4) params[4] = 0.5f;   // Additional parameter

        engine->updateParameters(params);
        std::cout << "OK\n";

        // 4. IMPULSE TEST
        std::cout << "  [4/5] Running impulse test...";
        const int impulseLength = 2048;
        juce::AudioBuffer<float> impulseBuffer(2, impulseLength);
        impulseBuffer.clear();

        // Create impulse
        impulseBuffer.setSample(0, 0, 1.0f);
        impulseBuffer.setSample(1, 0, 1.0f);

        // Process
        for (int start = 0; start < impulseLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, impulseLength - start);
            juce::AudioBuffer<float> block(impulseBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Check stability
        bool stable = true;
        float peakLevel = 0.0f;
        for (int i = 0; i < impulseLength; ++i) {
            float sample = impulseBuffer.getSample(0, i);
            peakLevel = std::max(peakLevel, std::abs(sample));

            if (std::isnan(sample) || std::isinf(sample) || std::abs(sample) > 100.0f) {
                stable = false;
                break;
            }
        }

        result.stable = stable;
        result.peakOutputLevel = peakLevel;

        if (!stable) {
            std::cout << " UNSTABLE (NaN/Inf/Excessive level)\n";
            result.errorMessage = "Impulse response unstable";
            return result;
        }

        std::cout << " OK (peak=" << std::fixed << std::setprecision(3) << peakLevel << ")\n";

        // 5. FREQUENCY SWEEP TEST
        std::cout << "  [5/5] Frequency response sweep...";
        engine->reset();
        engine->updateParameters(params);

        std::vector<float> testFreqs = {100.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f};

        for (float freq : testFreqs) {
            const int sweepLength = 4096;
            juce::AudioBuffer<float> sweepBuffer(2, sweepLength);

            // Generate sine wave
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < sweepLength; ++i) {
                    float phase = 2.0f * PI * freq * i / sampleRate;
                    sweepBuffer.setSample(ch, i, 0.5f * std::sin(phase));
                }
            }

            // Process
            for (int start = 0; start < sweepLength; start += blockSize) {
                int samplesThisBlock = std::min(blockSize, sweepLength - start);
                juce::AudioBuffer<float> block(sweepBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
                engine->process(block);
            }

            // Measure output level
            float outputLevel = 0.0f;
            for (int i = sweepLength / 2; i < sweepLength; ++i) {
                outputLevel = std::max(outputLevel, std::abs(sweepBuffer.getSample(0, i)));
            }

            result.frequencyResponse.push_back(outputLevel);

            if (outputLevel > 0.01f) {
                result.respondsToInput = true;
            }
        }

        std::cout << " OK\n";

        // Display frequency response
        std::cout << "\n  FREQUENCY RESPONSE:\n";
        for (size_t i = 0; i < testFreqs.size(); ++i) {
            float responseDB = 20.0f * std::log10(result.frequencyResponse[i] / 0.5f + 1e-10f);
            std::cout << "    " << std::setw(6) << testFreqs[i] << " Hz: "
                      << std::fixed << std::setprecision(2) << std::setw(7) << responseDB << " dB\n";
        }

        std::cout << "\n  RESULT: ";
        if (result.created && result.stable && result.respondsToInput) {
            std::cout << "✓ PASS\n";
        } else {
            std::cout << "✗ FAIL\n";
        }

    } catch (const std::exception& e) {
        std::cout << " EXCEPTION: " << e.what() << "\n";
        result.errorMessage = std::string("Exception: ") + e.what();
    } catch (...) {
        std::cout << " UNKNOWN EXCEPTION\n";
        result.errorMessage = "Unknown exception";
    }

    return result;
}

//==============================================================================
// MAIN
//==============================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         Filter Engines 8-14 Test Suite                    ║\n";
    std::cout << "║         Impulse Tests & Frequency Response                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    std::vector<std::pair<int, std::string>> engines = {
        {8, "VintageConsoleEQ_Studio"},
        {9, "LadderFilter"},
        {10, "StateVariableFilter"},
        {11, "FormantFilter"},
        {12, "EnvelopeFilter (AutoWah)"},
        {13, "CombResonator"},
        {14, "VocalFormantFilter"}
    };

    std::vector<TestResult> results;

    for (const auto& [id, name] : engines) {
        TestResult result = testFilterEngine(id, name);
        results.push_back(result);
    }

    // SUMMARY
    std::cout << "\n\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    TEST SUMMARY                            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Engine | Name                         | Created | Stable | Responds | Peak\n";
    std::cout << "-------+------------------------------+---------+--------+----------+-------\n";

    int passCount = 0;
    int totalCount = 0;

    for (const auto& r : results) {
        totalCount++;
        bool passed = r.created && r.stable && r.respondsToInput;
        if (passed) passCount++;

        std::cout << std::setw(6) << r.engineId << " | "
                  << std::setw(28) << std::left << r.engineName << " | "
                  << (r.created ? "   YES  " : "   NO   ") << " | "
                  << (r.stable ? "  YES  " : "  NO   ") << " | "
                  << (r.respondsToInput ? "   YES   " : "   NO    ") << " | "
                  << std::fixed << std::setprecision(3) << r.peakOutputLevel << "\n";

        if (!r.errorMessage.empty()) {
            std::cout << "       | Error: " << r.errorMessage << "\n";
        }
    }

    std::cout << "\n";
    std::cout << "PASS RATE: " << passCount << "/" << totalCount
              << " (" << (100 * passCount / totalCount) << "%)\n\n";

    if (passCount == totalCount) {
        std::cout << "✓ ALL TESTS PASSED\n\n";
        return 0;
    } else {
        std::cout << "✗ SOME TESTS FAILED\n\n";
        return 1;
    }
}

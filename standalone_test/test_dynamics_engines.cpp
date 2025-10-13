#include "JuceHeader.h"
#include "MinimalEngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

// Test Dynamics Engines 0-5
// Engine 0: NoneEngine
// Engine 1: VintageOptoCompressor_Platinum
// Engine 2: ClassicCompressor
// Engine 3: TransientShaper_Platinum
// Engine 4: NoiseGate_Platinum
// Engine 5: MasteringLimiter_Platinum

struct TestResult {
    int engineId;
    std::string name;
    bool initialized;
    bool processedWithoutCrash;
    float peakOutputLevel;
    float rmsOutputLevel;
    std::string status;
};

TestResult testEngine(int engineId, const std::string& name) {
    TestResult result;
    result.engineId = engineId;
    result.name = name;
    result.initialized = false;
    result.processedWithoutCrash = false;
    result.peakOutputLevel = 0.0f;
    result.rmsOutputLevel = 0.0f;
    result.status = "FAIL";

    std::cout << "\nTesting Engine " << engineId << ": " << name << "\n";
    std::cout << "----------------------------------------\n";

    try {
        // Create engine
        auto engine = MinimalEngineFactory::createEngine(engineId);
        if (!engine) {
            std::cout << "  ERROR: Failed to create engine\n";
            result.status = "FAIL - Creation failed";
            return result;
        }
        std::cout << "  [OK] Engine created\n";
        result.initialized = true;

        // Prepare engine
        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);
        std::cout << "  [OK] Engine prepared (SR=" << sampleRate << ", BS=" << blockSize << ")\n";

        // Set up basic parameters for each engine type
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        std::cout << "  [INFO] Engine has " << numParams << " parameters\n";

        // Apply engine-specific default parameters
        switch (engineId) {
            case 0: // NoneEngine
                // No parameters needed
                break;
            case 1: // VintageOptoCompressor
                if (numParams > 0) params[0] = 0.5f;  // Gain
                if (numParams > 1) params[1] = 0.6f;  // Peak Reduction
                if (numParams > 2) params[2] = 0.5f;  // Emphasis
                if (numParams > 3) params[3] = 0.7f;  // Output
                if (numParams > 4) params[4] = 1.0f;  // Mix
                break;
            case 2: // ClassicCompressor
                if (numParams > 0) params[0] = 0.5f;  // Threshold
                if (numParams > 1) params[1] = 0.5f;  // Ratio
                if (numParams > 2) params[2] = 0.3f;  // Attack
                if (numParams > 3) params[3] = 0.5f;  // Release
                if (numParams > 4) params[4] = 0.5f;  // Knee
                if (numParams > 5) params[5] = 0.5f;  // Makeup
                if (numParams > 6) params[6] = 1.0f;  // Mix
                break;
            case 3: // TransientShaper
                if (numParams > 0) params[0] = 0.5f;  // Attack
                if (numParams > 1) params[1] = 0.5f;  // Sustain
                if (numParams > 9) params[9] = 1.0f;  // Mix
                break;
            case 4: // NoiseGate
                if (numParams > 0) params[0] = 0.3f;  // Threshold
                if (numParams > 1) params[1] = 0.5f;  // Range
                if (numParams > 2) params[2] = 0.2f;  // Attack
                if (numParams > 3) params[3] = 0.5f;  // Hold
                if (numParams > 4) params[4] = 0.4f;  // Release
                break;
            case 5: // MasteringLimiter
                if (numParams > 0) params[0] = 0.8f;  // Threshold
                if (numParams > 1) params[1] = 0.95f; // Ceiling
                if (numParams > 2) params[2] = 0.5f;  // Release
                if (numParams > 3) params[3] = 0.5f;  // Lookahead
                break;
        }

        if (!params.empty()) {
            engine->updateParameters(params);
            std::cout << "  [OK] Parameters applied (" << params.size() << " parameters)\n";
        }

        // Create impulse test signal
        // Duration: 1 second = 48000 samples
        const int totalSamples = static_cast<int>(sampleRate);
        juce::AudioBuffer<float> buffer(2, totalSamples);
        buffer.clear();

        // Generate impulse: single sample at full scale at sample 1000
        buffer.setSample(0, 1000, 1.0f);
        buffer.setSample(1, 1000, 1.0f);

        // Add a few smaller impulses to test transient response
        buffer.setSample(0, 12000, 0.5f);
        buffer.setSample(1, 12000, 0.5f);
        buffer.setSample(0, 24000, 0.25f);
        buffer.setSample(1, 24000, 0.25f);

        std::cout << "  [OK] Impulse signal generated\n";

        // Process audio in blocks
        for (int start = 0; start < totalSamples; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, totalSamples - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        std::cout << "  [OK] Audio processed without crash\n";
        result.processedWithoutCrash = true;

        // Measure output levels
        float peakLevel = 0.0f;
        float sumSquares = 0.0f;
        int sampleCount = 0;

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < totalSamples; ++i) {
                float sample = buffer.getSample(ch, i);
                float absSample = std::abs(sample);

                // Check for peak
                if (absSample > peakLevel) {
                    peakLevel = absSample;
                }

                // Accumulate for RMS
                sumSquares += sample * sample;
                sampleCount++;
            }
        }

        result.peakOutputLevel = peakLevel;
        result.rmsOutputLevel = std::sqrt(sumSquares / sampleCount);

        std::cout << "  [MEASURE] Peak level: " << std::fixed << std::setprecision(6) << peakLevel;
        if (peakLevel > 0.0f) {
            std::cout << " (" << 20.0f * std::log10(peakLevel) << " dB)";
        }
        std::cout << "\n";

        std::cout << "  [MEASURE] RMS level:  " << std::fixed << std::setprecision(6) << result.rmsOutputLevel;
        if (result.rmsOutputLevel > 0.0f) {
            std::cout << " (" << 20.0f * std::log10(result.rmsOutputLevel) << " dB)";
        }
        std::cout << "\n";

        // Check for issues
        bool hasNaN = false;
        bool hasInf = false;
        bool hasClipping = false;

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < totalSamples; ++i) {
                float sample = buffer.getSample(ch, i);
                if (std::isnan(sample)) hasNaN = true;
                if (std::isinf(sample)) hasInf = true;
                if (std::abs(sample) > 1.1f) hasClipping = true;
            }
        }

        if (hasNaN) {
            std::cout << "  [WARNING] NaN values detected in output!\n";
            result.status = "FAIL - NaN output";
        } else if (hasInf) {
            std::cout << "  [WARNING] Infinite values detected in output!\n";
            result.status = "FAIL - Inf output";
        } else if (hasClipping) {
            std::cout << "  [WARNING] Severe clipping detected (>1.1)!\n";
            result.status = "FAIL - Clipping";
        } else {
            std::cout << "  [OK] No NaN, Inf, or severe clipping detected\n";
            result.status = "PASS";
        }

        // Additional check for silence (might indicate processing issue)
        if (engineId != 4 && peakLevel < 1e-6f) { // NoiseGate might legitimately silence
            std::cout << "  [WARNING] Output is essentially silent\n";
            if (result.status == "PASS") {
                result.status = "PASS - Silent output";
            }
        }

    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception caught: " << e.what() << "\n";
        result.status = "FAIL - Exception: " + std::string(e.what());
    } catch (...) {
        std::cout << "  [ERROR] Unknown exception caught\n";
        result.status = "FAIL - Unknown exception";
    }

    return result;
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  Dynamics Engines Test (0-5)\n";
    std::cout << "  Impulse Response & Stability Test\n";
    std::cout << "========================================\n";

    std::vector<std::pair<int, std::string>> engines = {
        {0, "NoneEngine"},
        {1, "VintageOptoCompressor"},
        {2, "ClassicCompressor"},
        {3, "TransientShaper"},
        {4, "NoiseGate"},
        {5, "MasteringLimiter"}
    };

    // Note: Engine 6 would be DimensionExpander, but user asked for engines 0-5
    // The user mentioned DimensionExpander but it's actually engine 43 (Spatial category)

    std::vector<TestResult> results;

    for (const auto& [id, name] : engines) {
        TestResult result = testEngine(id, name);
        results.push_back(result);
    }

    // Summary report
    std::cout << "\n\n";
    std::cout << "========================================\n";
    std::cout << "  TEST SUMMARY\n";
    std::cout << "========================================\n\n";

    std::cout << std::left;
    std::cout << std::setw(4) << "ID"
              << std::setw(30) << "Engine Name"
              << std::setw(10) << "Init"
              << std::setw(10) << "Process"
              << std::setw(15) << "Peak Level"
              << std::setw(15) << "RMS Level"
              << "Status\n";
    std::cout << std::string(95, '-') << "\n";

    int passCount = 0;
    int failCount = 0;

    for (const auto& result : results) {
        std::cout << std::setw(4) << result.engineId
                  << std::setw(30) << result.name
                  << std::setw(10) << (result.initialized ? "OK" : "FAIL")
                  << std::setw(10) << (result.processedWithoutCrash ? "OK" : "FAIL")
                  << std::setw(15) << std::fixed << std::setprecision(6) << result.peakOutputLevel
                  << std::setw(15) << std::fixed << std::setprecision(6) << result.rmsOutputLevel
                  << result.status << "\n";

        if (result.status.find("PASS") != std::string::npos) {
            passCount++;
        } else {
            failCount++;
        }
    }

    std::cout << std::string(95, '-') << "\n";
    std::cout << "\nPASSED: " << passCount << " / " << results.size() << "\n";
    std::cout << "FAILED: " << failCount << " / " << results.size() << "\n\n";

    if (failCount == 0) {
        std::cout << "ALL TESTS PASSED!\n\n";
        return 0;
    } else {
        std::cout << "SOME TESTS FAILED - See details above\n\n";
        return 1;
    }
}

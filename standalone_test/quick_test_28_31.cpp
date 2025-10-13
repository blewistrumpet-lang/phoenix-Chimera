#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>

/**
 * Quick Test for Engines 28-31
 * Tests: HarmonicTremolo, ClassicTremolo, RotarySpeaker, PitchShifter
 */

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║      Quick Test: Engines 28-31                         ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";

    const float sampleRate = 48000.0f;
    const int blockSize = 512;
    const int testLength = blockSize * 4; // Short test

    struct Result {
        int id;
        std::string name;
        bool passed;
        std::string error;
    };

    std::vector<Result> results;

    // Test each engine
    for (int engineId = 28; engineId <= 31; ++engineId) {
        Result r;
        r.id = engineId;
        r.passed = false;

        std::cout << "Testing Engine " << engineId << "... ";

        try {
            // Create engine
            auto engine = EngineFactory::createEngine(engineId);
            if (!engine) {
                r.error = "Failed to create engine";
                std::cout << "FAIL (no engine)\n";
                results.push_back(r);
                continue;
            }

            // Prepare
            engine->prepareToPlay(sampleRate, blockSize);

            // Set some parameters
            std::map<int, float> params;
            params[0] = 0.5f;
            params[1] = 0.5f;
            params[2] = 0.5f;
            engine->updateParameters(params);

            // Test 1: Impulse response
            {
                juce::AudioBuffer<float> buffer(2, testLength);
                buffer.clear();
                buffer.setSample(0, 0, 1.0f);
                buffer.setSample(1, 0, 1.0f);

                engine->process(buffer);

                // Check for NaN/Inf
                bool hasInvalid = false;
                float maxLevel = 0.0f;
                for (int ch = 0; ch < 2; ++ch) {
                    for (int i = 0; i < testLength; ++i) {
                        float sample = buffer.getSample(ch, i);
                        if (std::isnan(sample) || std::isinf(sample)) {
                            hasInvalid = true;
                            break;
                        }
                        maxLevel = std::max(maxLevel, std::abs(sample));
                    }
                    if (hasInvalid) break;
                }

                if (hasInvalid) {
                    r.error = "Output contains NaN/Inf";
                    std::cout << "FAIL (invalid samples)\n";
                    results.push_back(r);
                    continue;
                }

                if (maxLevel > 100.0f) {
                    r.error = "Output level too high: " + std::to_string(maxLevel);
                    std::cout << "FAIL (level too high)\n";
                    results.push_back(r);
                    continue;
                }
            }

            // Test 2: Sustained signal
            {
                juce::AudioBuffer<float> buffer(2, testLength);
                for (int ch = 0; ch < 2; ++ch) {
                    for (int i = 0; i < testLength; ++i) {
                        float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
                        buffer.setSample(ch, i, 0.5f * std::sin(phase));
                    }
                }

                engine->reset();
                engine->process(buffer);

                // Check for NaN/Inf
                bool hasInvalid = false;
                float maxLevel = 0.0f;
                for (int ch = 0; ch < 2; ++ch) {
                    for (int i = 0; i < testLength; ++i) {
                        float sample = buffer.getSample(ch, i);
                        if (std::isnan(sample) || std::isinf(sample)) {
                            hasInvalid = true;
                            break;
                        }
                        maxLevel = std::max(maxLevel, std::abs(sample));
                    }
                    if (hasInvalid) break;
                }

                if (hasInvalid) {
                    r.error = "Output contains NaN/Inf on sustained signal";
                    std::cout << "FAIL (sustained test)\n";
                    results.push_back(r);
                    continue;
                }

                if (maxLevel > 100.0f) {
                    r.error = "Output level too high on sustained: " + std::to_string(maxLevel);
                    std::cout << "FAIL (sustained level)\n";
                    results.push_back(r);
                    continue;
                }
            }

            // All tests passed
            r.passed = true;
            std::cout << "PASS\n";

        } catch (const std::exception& e) {
            r.error = std::string("Exception: ") + e.what();
            std::cout << "FAIL (exception)\n";
        } catch (...) {
            r.error = "Unknown exception";
            std::cout << "FAIL (unknown exception)\n";
        }

        results.push_back(r);
    }

    // Summary
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     SUMMARY                            ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";

    int passCount = 0;
    for (const auto& r : results) {
        std::cout << "Engine " << r.id << ": " << (r.passed ? "PASS" : "FAIL");
        if (!r.passed && !r.error.empty()) {
            std::cout << " (" << r.error << ")";
        }
        std::cout << "\n";
        if (r.passed) passCount++;
    }

    std::cout << "\nTotal: " << passCount << "/" << results.size() << " passed\n\n";

    return (passCount == results.size()) ? 0 : 1;
}

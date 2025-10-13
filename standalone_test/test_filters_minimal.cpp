/**
 * Minimal test for filter engines 8-14
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <map>

#include "JuceHeader.h"
#include "EngineBase.h"
#include "EngineFactory.h"

int main() {
    std::cout << "\n=== FILTER ENGINES 8-14 TEST ===\n\n";

    std::vector<std::pair<int, std::string>> engines = {
        {8, "VintageConsoleEQ_Studio"},
        {9, "LadderFilter"},
        {10, "StateVariableFilter"},
        {11, "FormantFilter"},
        {12, "EnvelopeFilter"},
        {13, "CombResonator"},
        {14, "VocalFormantFilter"}
    };

    int passCount = 0;
    int failCount = 0;

    for (const auto& [id, name] : engines) {
        std::cout << "Engine " << id << ": " << name << "\n";

        try {
            auto engine = EngineFactory::createEngine(id);

            if (!engine) {
                std::cout << "  ✗ FAIL - Factory returned nullptr\n\n";
                failCount++;
                continue;
            }

            // Prepare
            engine->prepareToPlay(48000.0, 512);

            // Create params
            std::map<int, float> params;
            int numParams = engine->getNumParameters();
            if (numParams > 0) params[0] = 1.0f;
            if (numParams > 1) params[1] = 0.5f;
            if (numParams > 2) params[2] = 0.7f;
            engine->updateParameters(params);

            // Impulse test
            juce::AudioBuffer<float> buffer(2, 2048);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);

            engine->process(buffer);

            // Check output
            bool stable = true;
            float peak = 0.0f;

            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float s = buffer.getSample(0, i);
                if (std::isnan(s) || std::isinf(s) || std::abs(s) > 100.0f) {
                    stable = false;
                    break;
                }
                peak = std::max(peak, std::abs(s));
            }

            std::cout << "  Parameters: " << numParams << "\n";
            std::cout << "  Peak output: " << std::fixed << std::setprecision(4) << peak << "\n";
            std::cout << "  Stable: " << (stable ? "YES" : "NO") << "\n";

            if (stable && peak > 0.0f) {
                std::cout << "  ✓ PASS\n\n";
                passCount++;
            } else {
                std::cout << "  ✗ FAIL\n\n";
                failCount++;
            }

        } catch (const std::exception& e) {
            std::cout << "  ✗ EXCEPTION: " << e.what() << "\n\n";
            failCount++;
        } catch (...) {
            std::cout << "  ✗ UNKNOWN EXCEPTION\n\n";
            failCount++;
        }
    }

    std::cout << "\n=== SUMMARY ===\n";
    std::cout << "Passed: " << passCount << "\n";
    std::cout << "Failed: " << failCount << "\n";
    std::cout << "Total:  " << (passCount + failCount) << "\n\n";

    return (failCount == 0) ? 0 : 1;
}

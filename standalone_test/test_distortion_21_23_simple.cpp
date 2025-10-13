#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/RodentDistortion.h"
#include "../JUCE_Plugin/Source/KStyleOverdrive.h"
#include "../JUCE_Plugin/Source/StereoChorus.h"
#include <iostream>
#include <iomanip>
#include <cmath>

// Simple focused test for engines 21-23
// Engine 21: RodentDistortion
// Engine 22: KStyleOverdrive
// Engine 23: StereoChorus

struct TestResult {
    std::string engine_name;
    int engine_id;
    bool impulse_pass;
    float peak_output;
    float thd;
    float output_level_db;
    bool overall_pass;
    std::string notes;
};

bool testEngine(EngineBase* engine, const std::string& name, int id, TestResult& result) {
    result.engine_name = name;
    result.engine_id = id;
    result.overall_pass = true;
    result.notes = "";

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    try {
        engine->prepareToPlay(sampleRate, blockSize);

        // Set moderate parameters
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        if (numParams > 0) params[0] = 0.5f;  // Drive/Gain
        if (numParams > 1) params[1] = 0.5f;  // Tone/Rate
        if (numParams > 2) params[2] = 0.7f;  // Level/Depth
        if (numParams > 3) params[3] = 1.0f;  // Mix

        engine->updateParameters(params);

        // TEST 1: Impulse Response
        {
            juce::AudioBuffer<float> buffer(2, blockSize);
            buffer.clear();
            buffer.setSample(0, 10, 1.0f);
            buffer.setSample(1, 10, 1.0f);

            engine->process(buffer);

            float peak = 0.0f;
            int nonZero = 0;
            for (int i = 0; i < blockSize; ++i) {
                float s = std::abs(buffer.getSample(0, i));
                if (s > 0.001f) nonZero++;
                peak = std::max(peak, s);
            }

            result.peak_output = peak;
            result.impulse_pass = (nonZero > 0 && peak > 0.01f && peak < 10.0f);

            if (!result.impulse_pass) {
                result.overall_pass = false;
                if (nonZero == 0 || peak < 0.01f) {
                    result.notes += "No output from impulse; ";
                } else if (peak >= 10.0f) {
                    result.notes += "Excessive output level; ";
                }
            }
        }

        // TEST 2: Distortion Characteristics (simple THD estimate)
        {
            const int fftSize = 4096;
            juce::AudioBuffer<float> buffer(2, fftSize);

            // 1kHz sine at -10dB
            float amplitude = 0.316f;
            for (int i = 0; i < fftSize; ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                float sample = amplitude * std::sin(phase);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            engine->process(buffer);

            // Measure RMS and approximate THD by looking at output waveform shape
            float rms = 0.0f;
            float peak = 0.0f;
            for (int i = 0; i < fftSize; ++i) {
                float s = buffer.getSample(0, i);
                rms += s * s;
                peak = std::max(peak, std::abs(s));
            }
            rms = std::sqrt(rms / fftSize);

            // Simple THD estimate: if it's pure sine, crest factor ~= 1.414
            // Distortion increases RMS relative to peak
            float crestFactor = (rms > 0) ? (peak / rms) : 1.414f;
            result.thd = std::max(0.0f, (1.414f - crestFactor) / 1.414f * 2.0f);  // Rough estimate

            result.output_level_db = 20.0f * std::log10(std::max(1e-10f, peak));

            if (peak > 1.5f) {
                result.overall_pass = false;
                result.notes += "Output exceeds safe level; ";
            }
        }

        // TEST 3: Output Levels
        {
            juce::AudioBuffer<float> buffer(2, blockSize);

            // 0dBFS sine
            for (int i = 0; i < blockSize; ++i) {
                float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
                float sample = 0.9f * std::sin(phase);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            engine->process(buffer);

            float peak = 0.0f;
            for (int i = 0; i < blockSize; ++i) {
                peak = std::max(peak, std::abs(buffer.getSample(0, i)));
            }

            if (peak > 2.0f) {
                result.overall_pass = false;
                result.notes += "Extreme output level; ";
            }
            if (peak < 0.001f) {
                result.overall_pass = false;
                result.notes += "No audio output; ";
            }
        }

        if (result.notes.empty()) {
            result.notes = "All tests passed";
        }

        return true;

    } catch (const std::exception& e) {
        result.overall_pass = false;
        result.notes = std::string("Exception: ") + e.what();
        return false;
    }
}

void printResult(const TestResult& r) {
    std::cout << "\n========================================\n";
    std::cout << "ENGINE " << r.engine_id << ": " << r.engine_name << "\n";
    std::cout << "========================================\n";
    std::cout << "\n[IMPULSE TEST]\n";
    std::cout << "  Result: " << (r.impulse_pass ? "PASS" : "FAIL") << "\n";
    std::cout << "  Peak output: " << std::fixed << std::setprecision(4) << r.peak_output << "\n";

    std::cout << "\n[DISTORTION CHARACTERISTICS]\n";
    std::cout << "  THD (est): " << std::fixed << std::setprecision(2) << (r.thd * 100.0f) << "%\n";

    std::cout << "\n[OUTPUT LEVELS]\n";
    std::cout << "  Output level: " << std::fixed << std::setprecision(1) << r.output_level_db << " dB\n";

    std::cout << "\n[RESULT]: " << (r.overall_pass ? "PASS" : "FAIL") << "\n";
    std::cout << "Notes: " << r.notes << "\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ChimeraPhoenix Distortion Test: Engines 21-23          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    std::cout << "\nNOTE: User requested TapeSaturation, VinylDistortion, HarmonicExciter_Platinum\n";
    std::cout << "      These engines don't exist as 21-23 in the codebase.\n";
    std::cout << "      Actual engines 21-23:\n";
    std::cout << "        21 = RodentDistortion\n";
    std::cout << "        22 = KStyleOverdrive\n";
    std::cout << "        23 = StereoChorus (modulation, not distortion)\n";
    std::cout << "      HarmonicExciter_Platinum is Engine 17, not 21-23.\n\n";

    std::vector<TestResult> results;
    int passed = 0;
    int failed = 0;

    // Test Engine 21: RodentDistortion
    {
        auto engine = std::make_unique<RodentDistortion>();
        TestResult result;
        if (testEngine(engine.get(), "RodentDistortion", 21, result)) {
            printResult(result);
            results.push_back(result);
            if (result.overall_pass) passed++; else failed++;
        } else {
            std::cout << "\n[ERROR] Failed to test Engine 21: RodentDistortion\n";
            failed++;
        }
    }

    // Test Engine 22: KStyleOverdrive
    {
        auto engine = std::make_unique<KStyleOverdrive>();
        TestResult result;
        if (testEngine(engine.get(), "KStyleOverdrive", 22, result)) {
            printResult(result);
            results.push_back(result);
            if (result.overall_pass) passed++; else failed++;
        } else {
            std::cout << "\n[ERROR] Failed to test Engine 22: KStyleOverdrive\n";
            failed++;
        }
    }

    // Test Engine 23: StereoChorus
    {
        auto engine = std::make_unique<StereoChorus>();
        TestResult result;
        if (testEngine(engine.get(), "StereoChorus", 23, result)) {
            printResult(result);
            results.push_back(result);
            if (result.overall_pass) passed++; else failed++;
        } else {
            std::cout << "\n[ERROR] Failed to test Engine 23: StereoChorus\n";
            failed++;
        }
    }

    // Summary
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST SUMMARY                                            ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "\nTotal engines tested: 3\n";
    std::cout << "PASSED: " << passed << "\n";
    std::cout << "FAILED: " << failed << "\n";
    std::cout << "\nSuccess rate: " << std::fixed << std::setprecision(0)
              << (100.0 * passed / 3.0) << "%\n\n";

    return (failed > 0) ? 1 : 0;
}

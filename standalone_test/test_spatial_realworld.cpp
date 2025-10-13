/**
 * REAL-WORLD SPATIAL/STEREO ENGINE TESTING
 * Engines: 46 (StereoImager), 53 (MidSideProcessor_Platinum), 56 (PhaseAlignPlatinum)
 */

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 1
#define JUCE_USE_CURL 0
#define DEBUG 1

#include "../JUCE_Plugin/Source/StereoImager.h"
#include "../JUCE_Plugin/Source/MidSideProcessor_Platinum.h"
#include "../JUCE_Plugin/Source/PhaseAlign_Platinum.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory>
#include <iomanip>
#include <algorithm>
#include <map>

// ========================================================================
// AUDIO I/O
// ========================================================================

std::vector<std::pair<std::vector<float>, std::vector<float>>> loadStereoRAW(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open " << filename << std::endl;
        return {};
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    size_t numSamples = fileSize / (2 * sizeof(float));
    std::vector<float> interleaved(numSamples * 2);

    file.read(reinterpret_cast<char*>(interleaved.data()), fileSize);
    file.close();

    std::vector<float> left(numSamples);
    std::vector<float> right(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        left[i] = interleaved[i * 2];
        right[i] = interleaved[i * 2 + 1];
    }

    return {{left, right}};
}

void writeStereoRAW(const std::string& filename, const std::vector<float>& left, const std::vector<float>& right) {
    std::ofstream file(filename, std::ios::binary);

    std::vector<float> interleaved(left.size() * 2);
    for (size_t i = 0; i < left.size(); ++i) {
        interleaved[i * 2] = left[i];
        interleaved[i * 2 + 1] = right[i];
    }

    file.write(reinterpret_cast<const char*>(interleaved.data()), interleaved.size() * sizeof(float));
    file.close();
}

// ========================================================================
// STEREO ANALYSIS
// ========================================================================

struct StereoMetrics {
    float correlation;      // L-R correlation
    float width;           // Stereo width
    float centerEnergy;    // Mid channel energy
    float sideEnergy;      // Side channel energy
    float monoSum;         // RMS of L+R
    float levelLoss;       // dB loss in mono vs stereo
    float combFilter;      // Comb filtering measure
    float phaseCoherence;  // Phase coherence
};

StereoMetrics analyzeStereo(const std::vector<float>& left, const std::vector<float>& right) {
    StereoMetrics metrics = {};

    float sum_ll = 0, sum_rr = 0, sum_lr = 0;
    for (size_t i = 0; i < left.size(); ++i) {
        sum_ll += left[i] * left[i];
        sum_rr += right[i] * right[i];
        sum_lr += left[i] * right[i];
    }

    float sigma_l = std::sqrt(sum_ll / left.size());
    float sigma_r = std::sqrt(sum_rr / right.size());

    if (sigma_l > 1e-6f && sigma_r > 1e-6f) {
        metrics.correlation = sum_lr / (sigma_l * sigma_r * left.size());
    }

    metrics.width = std::max(0.0f, 1.0f - metrics.correlation);

    // Mid/Side analysis
    float midEnergy = 0, sideEnergy = 0;
    for (size_t i = 0; i < left.size(); ++i) {
        float mid = (left[i] + right[i]) * 0.5f;
        float side = (left[i] - right[i]) * 0.5f;
        midEnergy += mid * mid;
        sideEnergy += side * side;
    }
    metrics.centerEnergy = std::sqrt(midEnergy / left.size());
    metrics.sideEnergy = std::sqrt(sideEnergy / left.size());

    // Mono compatibility
    float stereoRMS = std::sqrt((sum_ll + sum_rr) / (2 * left.size()));
    float monoSum = 0;
    for (size_t i = 0; i < left.size(); ++i) {
        float mono = left[i] + right[i];
        monoSum += mono * mono;
    }
    metrics.monoSum = std::sqrt(monoSum / left.size());

    if (stereoRMS > 1e-6f && metrics.monoSum > 1e-6f) {
        metrics.levelLoss = 20.0f * std::log10(metrics.monoSum / (stereoRMS * 2.0f));
    }

    // Comb filtering detection
    std::vector<float> diff(left.size() - 1);
    for (size_t i = 0; i < diff.size(); ++i) {
        float mono = (left[i] + right[i]) * 0.5f;
        float nextMono = (left[i+1] + right[i+1]) * 0.5f;
        diff[i] = std::abs(nextMono - mono);
    }
    float meanDiff = 0;
    for (float d : diff) meanDiff += d;
    meanDiff /= diff.size();
    metrics.combFilter = meanDiff;

    metrics.phaseCoherence = std::abs(metrics.correlation);

    return metrics;
}

// ========================================================================
// ENGINE PROCESSING
// ========================================================================

template<typename EngineType>
std::pair<std::vector<float>, std::vector<float>> processEngine(
    EngineType& engine,
    const std::vector<float>& inputL,
    const std::vector<float>& inputR,
    const std::map<int, float>& params,
    double sampleRate = 48000.0,
    int blockSize = 512
) {
    engine.prepareToPlay(sampleRate, blockSize);
    engine.updateParameters(params);

    std::vector<float> outputL, outputR;
    outputL.reserve(inputL.size());
    outputR.reserve(inputR.size());

    size_t pos = 0;
    while (pos < inputL.size()) {
        int samplesToProcess = std::min(blockSize, (int)(inputL.size() - pos));

        // Create JUCE AudioBuffer
        juce::AudioBuffer<float> buffer(2, samplesToProcess);

        for (int i = 0; i < samplesToProcess; ++i) {
            buffer.setSample(0, i, inputL[pos + i]);
            buffer.setSample(1, i, inputR[pos + i]);
        }

        engine.process(buffer);

        for (int i = 0; i < samplesToProcess; ++i) {
            outputL.push_back(buffer.getSample(0, i));
            outputR.push_back(buffer.getSample(1, i));
        }

        pos += samplesToProcess;
    }

    return {outputL, outputR};
}

// ========================================================================
// TEST RESULTS
// ========================================================================

struct TestResult {
    std::string engineName;
    int engineID;
    char grade;
    bool pass;
    std::string details;
};

std::vector<TestResult> allResults;

// ========================================================================
// ENGINE TESTS
// ========================================================================

void testEngine46_StereoImager() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  ENGINE 46: StereoImager\n";
    std::cout << std::string(70, '=') << "\n\n";

    auto drums = loadStereoRAW("spatial_test_drums_stereo.raw");
    auto guitar = loadStereoRAW("spatial_test_guitar_double.raw");
    auto mix = loadStereoRAW("spatial_test_full_mix.raw");
    auto mono = loadStereoRAW("spatial_test_mono_source.raw");

    if (drums.empty() || guitar.empty() || mix.empty() || mono.empty()) {
        std::cerr << "ERROR: Failed to load test materials!\n";
        return;
    }

    StereoImager engine;

    // Test 1: Width control
    std::cout << "[Test 1] Width Parameter - Drums (Narrow to Wide)\n";
    for (float width : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
        std::map<int, float> params = {{0, width}};  // Param 0 = width

        auto [outL, outR] = processEngine(engine, drums[0].first, drums[0].second, params);
        auto metrics = analyzeStereo(outL, outR);

        std::cout << "  Width=" << std::fixed << std::setprecision(2) << width
                  << " → Corr=" << std::setprecision(3) << metrics.correlation
                  << ", Width=" << metrics.width
                  << ", Mono Loss=" << metrics.levelLoss << " dB\n";

        writeStereoRAW("spatial_46_drums_width_" + std::to_string((int)(width*100)) + ".raw", outL, outR);
    }

    // Test 2: Mono source enhancement
    std::cout << "\n[Test 2] Mono Source Width Enhancement\n";
    auto inputMetrics = analyzeStereo(mono[0].first, mono[0].second);
    std::cout << "  Input: Correlation=" << inputMetrics.correlation << "\n";

    std::map<int, float> params = {{0, 0.8f}};
    auto [outL, outR] = processEngine(engine, mono[0].first, mono[0].second, params);
    auto outputMetrics = analyzeStereo(outL, outR);

    std::cout << "  Output: Correlation=" << outputMetrics.correlation
              << ", Width=" << outputMetrics.width << "\n";

    bool widthCreated = outputMetrics.correlation < 0.9f;
    std::cout << "  Width Created: " << (widthCreated ? "PASS" : "FAIL") << "\n";

    writeStereoRAW("spatial_46_mono_enhanced.raw", outL, outR);

    // Test 3: Mono compatibility
    std::cout << "\n[Test 3] Mono Compatibility (CRITICAL)\n";
    params = {{0, 0.75f}};

    auto [outL2, outR2] = processEngine(engine, mix[0].first, mix[0].second, params);
    auto metrics2 = analyzeStereo(outL2, outR2);

    std::cout << "  Mono Loss: " << metrics2.levelLoss << " dB\n";
    std::cout << "  Phase Coherence: " << metrics2.phaseCoherence << "\n";

    bool monoCompatible = metrics2.levelLoss > -3.0f;
    std::cout << "  Mono Compatible: " << (monoCompatible ? "PASS" : "FAIL") << "\n";

    // Generate mono fold-down
    std::vector<float> monoFold(outL2.size());
    for (size_t i = 0; i < monoFold.size(); ++i) {
        monoFold[i] = (outL2[i] + outR2[i]) * 0.5f;
    }
    writeStereoRAW("spatial_46_mono_folddown.raw", monoFold, monoFold);

    // Grade
    char grade = 'F';
    if (widthCreated && monoCompatible && metrics2.levelLoss > -6.0f) {
        grade = 'A';
    } else if (widthCreated && metrics2.levelLoss > -6.0f) {
        grade = 'B';
    } else if (widthCreated) {
        grade = 'C';
    } else {
        grade = 'D';
    }

    std::cout << "\n  ENGINE 46 GRADE: " << grade << "\n";

    allResults.push_back({"StereoImager", 46, grade, (grade >= 'C'), ""});
}

void testEngine53_MidSideProcessor() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  ENGINE 53: MidSideProcessor_Platinum\n";
    std::cout << std::string(70, '=') << "\n\n";

    auto drums = loadStereoRAW("spatial_test_drums_stereo.raw");
    auto guitar = loadStereoRAW("spatial_test_guitar_double.raw");
    auto mix = loadStereoRAW("spatial_test_full_mix.raw");

    if (drums.empty() || guitar.empty() || mix.empty()) {
        std::cerr << "ERROR: Failed to load test materials!\n";
        return;
    }

    MidSideProcessor_Platinum engine;

    // Test 1: Mid/Side balance
    std::cout << "[Test 1] Mid/Side Balance Control\n";
    auto inputMetrics = analyzeStereo(guitar[0].first, guitar[0].second);
    std::cout << "  Input: Center=" << std::setprecision(3) << inputMetrics.centerEnergy
              << ", Side=" << inputMetrics.sideEnergy << "\n";

    std::map<int, float> params = {{0, 0.5f}, {1, 0.8f}};  // Mid normal, Side boost
    auto [outL1, outR1] = processEngine(engine, guitar[0].first, guitar[0].second, params);
    auto metrics1 = analyzeStereo(outL1, outR1);

    std::cout << "  Side Boost: Center=" << metrics1.centerEnergy
              << ", Side=" << metrics1.sideEnergy << "\n";

    bool sideIncreased = metrics1.sideEnergy > inputMetrics.sideEnergy * 1.1f;
    std::cout << "  Side Increased: " << (sideIncreased ? "PASS" : "FAIL") << "\n";

    writeStereoRAW("spatial_53_guitar_side_boost.raw", outL1, outR1);

    // Test 2: Mid boost
    std::cout << "\n[Test 2] Mid Boost (Narrow Stereo)\n";
    params = {{0, 0.9f}, {1, 0.3f}};  // Mid up, Side down

    auto [outL2, outR2] = processEngine(engine, drums[0].first, drums[0].second, params);
    auto metrics2 = analyzeStereo(outL2, outR2);

    std::cout << "  Mid Boost: Correlation=" << metrics2.correlation << "\n";

    bool narrowed = metrics2.correlation > inputMetrics.correlation;
    std::cout << "  Narrowed: " << (narrowed ? "PASS" : "FAIL") << "\n";

    writeStereoRAW("spatial_53_drums_mid_boost.raw", outL2, outR2);

    // Test 3: Mono compatibility
    std::cout << "\n[Test 3] Mono Compatibility\n";
    params = {{0, 0.6f}, {1, 0.8f}};

    auto [outL3, outR3] = processEngine(engine, mix[0].first, mix[0].second, params);
    auto metrics3 = analyzeStereo(outL3, outR3);

    std::cout << "  Mono Loss: " << metrics3.levelLoss << " dB\n";

    bool monoCompatible = metrics3.levelLoss > -3.0f;
    std::cout << "  Mono Compatible: " << (monoCompatible ? "PASS" : "FAIL") << "\n";

    // Grade
    char grade = 'F';
    if (sideIncreased && narrowed && monoCompatible) {
        grade = 'A';
    } else if (sideIncreased && narrowed) {
        grade = 'B';
    } else if (sideIncreased || narrowed) {
        grade = 'C';
    } else {
        grade = 'D';
    }

    std::cout << "\n  ENGINE 53 GRADE: " << grade << "\n";

    allResults.push_back({"MidSideProcessor_Platinum", 53, grade, (grade >= 'C'), ""});
}

void testEngine56_PhaseAlignPlatinum() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  ENGINE 56: PhaseAlignPlatinum (VERIFY FIX)\n";
    std::cout << std::string(70, '=') << "\n\n";

    auto drums = loadStereoRAW("spatial_test_drums_stereo.raw");
    auto guitar = loadStereoRAW("spatial_test_guitar_double.raw");
    auto mix = loadStereoRAW("spatial_test_full_mix.raw");

    if (drums.empty() || guitar.empty() || mix.empty()) {
        std::cerr << "ERROR: Failed to load test materials!\n";
        return;
    }

    PhaseAlign_Platinum engine;

    // Test 1: Stability check
    std::cout << "[Test 1] Stability Check (100 iterations)\n";
    bool crashed = false;
    int validOutputs = 0;

    try {
        std::map<int, float> params = {{0, 1.0f}};  // Auto mode

        for (int i = 0; i < 100; ++i) {
            auto [outL, outR] = processEngine(engine, drums[0].first, drums[0].second, params);

            bool valid = true;
            for (size_t j = 0; j < outL.size(); ++j) {
                if (std::isnan(outL[j]) || std::isinf(outL[j]) ||
                    std::isnan(outR[j]) || std::isinf(outR[j])) {
                    valid = false;
                    break;
                }
            }

            if (valid) validOutputs++;
        }
    } catch (...) {
        crashed = true;
    }

    std::cout << "  Crashed: " << (crashed ? "YES" : "NO") << "\n";
    std::cout << "  Valid Outputs: " << validOutputs << "/100\n";

    bool stable = !crashed && (validOutputs == 100);
    std::cout << "  Stability: " << (stable ? "PASS" : "FAIL") << "\n";

    // Test 2: Phase alignment
    std::cout << "\n[Test 2] Phase Alignment Control\n";
    auto inputMetrics = analyzeStereo(guitar[0].first, guitar[0].second);
    std::cout << "  Input Phase Coherence: " << inputMetrics.phaseCoherence << "\n";

    std::map<int, float> params = {{0, 1.0f}};  // Auto

    auto [outL1, outR1] = processEngine(engine, guitar[0].first, guitar[0].second, params);
    auto metrics1 = analyzeStereo(outL1, outR1);

    std::cout << "  Output Phase Coherence: " << metrics1.phaseCoherence << "\n";

    bool phaseImproved = metrics1.phaseCoherence >= inputMetrics.phaseCoherence * 0.95f;
    std::cout << "  Phase Maintained/Improved: " << (phaseImproved ? "PASS" : "FAIL") << "\n";

    writeStereoRAW("spatial_56_guitar_aligned.raw", outL1, outR1);

    // Test 3: Mono compatibility
    std::cout << "\n[Test 3] Mono Compatibility\n";
    auto [outL2, outR2] = processEngine(engine, mix[0].first, mix[0].second, params);
    auto metrics2 = analyzeStereo(outL2, outR2);

    std::cout << "  Mono Loss: " << metrics2.levelLoss << " dB\n";
    std::cout << "  Comb Filtering: " << metrics2.combFilter << "\n";

    bool monoCompatible = metrics2.levelLoss > -3.0f;
    bool noCombFiltering = metrics2.combFilter < 0.05f;

    std::cout << "  Mono Compatible: " << (monoCompatible ? "PASS" : "FAIL") << "\n";
    std::cout << "  No Comb Filtering: " << (noCombFiltering ? "PASS" : "FAIL") << "\n";

    writeStereoRAW("spatial_56_mix_aligned.raw", outL2, outR2);

    // Grade
    char grade = 'F';
    if (stable && phaseImproved && monoCompatible && noCombFiltering) {
        grade = 'A';
    } else if (stable && phaseImproved && monoCompatible) {
        grade = 'B';
    } else if (stable && monoCompatible) {
        grade = 'C';
    } else if (stable) {
        grade = 'D';
    }

    std::cout << "\n  ENGINE 56 GRADE: " << grade << "\n";
    std::cout << "  FIX VERIFIED: " << (grade >= 'C' ? "YES" : "NO") << "\n";

    std::string details = stable ? "Stable, fix verified" : "Still unstable";
    allResults.push_back({"PhaseAlignPlatinum", 56, grade, (grade >= 'C'), details});
}

// ========================================================================
// MAIN
// ========================================================================

int main() {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  REAL-WORLD SPATIAL/STEREO ENGINE TESTING\n";
    std::cout << "  Engines 46, 53, 56\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";

    testEngine46_StereoImager();
    testEngine53_MidSideProcessor();
    testEngine56_PhaseAlignPlatinum();

    // Summary
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  FINAL SUMMARY\n";
    std::cout << std::string(70, '=') << "\n\n";

    int aCount = 0, bCount = 0, cCount = 0, dCount = 0, fCount = 0;

    for (const auto& result : allResults) {
        std::cout << "  Engine " << result.engineID << " (" << result.engineName << "): "
                  << "Grade " << result.grade << " - "
                  << (result.pass ? "PASS" : "FAIL");
        if (!result.details.empty()) {
            std::cout << " (" << result.details << ")";
        }
        std::cout << "\n";

        switch (result.grade) {
            case 'A': aCount++; break;
            case 'B': bCount++; break;
            case 'C': cCount++; break;
            case 'D': dCount++; break;
            case 'F': fCount++; break;
        }
    }

    std::cout << "\n";
    std::cout << "  Grade Distribution:\n";
    std::cout << "    A: " << aCount << "  (Excellent)\n";
    std::cout << "    B: " << bCount << "  (Good)\n";
    std::cout << "    C: " << cCount << "  (Acceptable)\n";
    std::cout << "    D: " << dCount << "  (Poor)\n";
    std::cout << "    F: " << fCount << "  (Fail)\n";

    int passingEngines = aCount + bCount + cCount;
    std::cout << "\n  Passing Engines: " << passingEngines << "/3\n";

    bool productionReady = (fCount == 0) && (dCount == 0);
    std::cout << "\n  PRODUCTION READY: " << (productionReady ? "YES" : "NO") << "\n";

    std::cout << "\n═══════════════════════════════════════════════════════════════════\n\n";

    return (productionReady ? 0 : 1);
}

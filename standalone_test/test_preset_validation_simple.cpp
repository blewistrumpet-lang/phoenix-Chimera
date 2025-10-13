#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

/**
 * Simplified Preset Validation System
 *
 * This validates factory presets without requiring the full plugin to be compiled.
 * It performs:
 * 1. JSON parsing and structure validation
 * 2. Engine ID validation (range checking)
 * 3. Parameter value validation (range [0,1])
 * 4. Preset structure validation
 * 5. Comprehensive reporting
 */

// Engine validation - valid range is 0-56
const int MAX_ENGINE_ID = 56;
const int MIN_ENGINE_ID = 0;

// Engine names for validation
const char* ENGINE_NAMES[] = {
    "None", "OptoCompressor", "VCACompressor", "TransientShaper", "NoiseGate",
    "MasteringLimiter", "DynamicEQ", "ParametricEQ", "VintageConsoleEQ", "LadderFilter",
    "StateVariableFilter", "FormantFilter", "EnvelopeFilter", "CombResonator", "VocalFormant",
    "VintageTube", "WaveFolder", "HarmonicExciter", "BitCrusher", "MultibandSaturator",
    "MuffFuzz", "RodentDistortion", "KStyleOverdrive", "DigitalChorus", "ResonantChorus",
    "AnalogPhaser", "RingModulator", "FrequencyShifter", "HarmonicTremolo", "ClassicTremolo",
    "RotarySpeaker", "PitchShifter", "DetuneDoubler", "IntelligentHarmonizer", "TapeEcho",
    "DigitalDelay", "MagneticDrumEcho", "BucketBrigadeDelay", "BufferRepeat", "PlateReverb",
    "SpringReverb", "ConvolutionReverb", "ShimmerReverb", "GatedReverb", "StereoWidener",
    "StereoImager", "DimensionExpander", "SpectralFreeze", "SpectralGate", "PhasedVocoder",
    "GranularCloud", "ChaosGenerator", "FeedbackNetwork", "MidSideProcessor", "GainUtility",
    "MonoMaker", "PhaseAlign"
};

struct ValidationIssue {
    std::string severity;  // "ERROR", "WARNING", "INFO"
    std::string message;
};

struct PresetValidationResult {
    std::string presetId;
    std::string presetName;
    std::string category;
    std::string subcategory;
    bool passed;
    std::vector<ValidationIssue> issues;

    // Validation checks
    bool validStructure;
    bool validEngineIds;
    bool validParameters;
    bool validSlots;

    // Engine usage statistics
    int numEngines;
    std::vector<int> engineIds;

    PresetValidationResult()
        : passed(false), validStructure(true), validEngineIds(true),
          validParameters(true), validSlots(true), numEngines(0) {}
};

class SimplePresetValidator {
public:
    SimplePresetValidator() {
        // No GUI needed for validation
    }

    ~SimplePresetValidator() {
    }

    bool loadPresetsFromFile(const std::string& filePath) {
        std::cout << "\n[LOADING] Reading presets from: " << filePath << std::endl;

        juce::File presetFile(filePath);
        if (!presetFile.existsAsFile()) {
            std::cerr << "[ERROR] Preset file not found: " << filePath << std::endl;
            return false;
        }

        auto jsonText = presetFile.loadFileAsString();
        auto result = juce::JSON::parse(jsonText);

        if (!result.isObject()) {
            std::cerr << "[ERROR] Failed to parse JSON file" << std::endl;
            return false;
        }

        presetsJson = result;

        if (!presetsJson.hasProperty("presets")) {
            std::cerr << "[ERROR] JSON does not contain 'presets' array" << std::endl;
            return false;
        }

        auto presetsArray = presetsJson.getProperty("presets", juce::var());
        if (!presetsArray.isArray()) {
            std::cerr << "[ERROR] 'presets' is not an array" << std::endl;
            return false;
        }

        int presetCount = presetsArray.size();
        std::cout << "[INFO] Loaded " << presetCount << " presets from file" << std::endl;

        return presetCount > 0;
    }

    std::vector<PresetValidationResult> validateAllPresets() {
        std::vector<PresetValidationResult> results;

        auto presetsArray = presetsJson.getProperty("presets", juce::var());
        if (!presetsArray.isArray()) {
            return results;
        }

        int totalPresets = presetsArray.size();
        std::cout << "\n============================================" << std::endl;
        std::cout << "PRESET VALIDATION SUITE" << std::endl;
        std::cout << "============================================" << std::endl;
        std::cout << "Total presets to validate: " << totalPresets << std::endl;
        std::cout << "============================================\n" << std::endl;

        for (int i = 0; i < totalPresets; i++) {
            auto preset = presetsArray[i];

            std::string presetId = preset.getProperty("id", "").toString().toStdString();
            std::string presetName = preset.getProperty("name", "").toString().toStdString();

            std::cout << "[" << (i+1) << "/" << totalPresets << "] Validating: "
                      << presetName << " (" << presetId << ")" << std::endl;

            PresetValidationResult result = validatePreset(preset);
            result.presetId = presetId;
            result.presetName = presetName;
            result.category = preset.getProperty("category", "").toString().toStdString();
            result.subcategory = preset.getProperty("subcategory", "").toString().toStdString();

            results.push_back(result);

            if (result.passed) {
                std::cout << "  [PASS] All checks passed" << std::endl;
            } else {
                std::cout << "  [FAIL] " << result.issues.size() << " issue(s) found" << std::endl;
                for (const auto& issue : result.issues) {
                    std::cout << "    [" << issue.severity << "] " << issue.message << std::endl;
                }
            }
            std::cout << std::endl;
        }

        return results;
    }

private:
    juce::var presetsJson;

    PresetValidationResult validatePreset(const juce::var& preset) {
        PresetValidationResult result;

        // Check required fields
        if (!preset.hasProperty("id")) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = "Preset missing 'id' field";
            result.issues.push_back(issue);
            result.validStructure = false;
        }

        if (!preset.hasProperty("name")) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = "Preset missing 'name' field";
            result.issues.push_back(issue);
            result.validStructure = false;
        }

        if (!preset.hasProperty("engines")) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = "Preset missing 'engines' array";
            result.issues.push_back(issue);
            result.validStructure = false;
            return result;
        }

        auto enginesArray = preset.getProperty("engines", juce::var());
        if (!enginesArray.isArray()) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = "'engines' is not an array";
            result.issues.push_back(issue);
            result.validStructure = false;
            return result;
        }

        result.numEngines = enginesArray.size();

        // Validate each engine
        std::map<int, bool> usedSlots;  // Track slot usage

        for (int i = 0; i < enginesArray.size(); i++) {
            auto engine = enginesArray[i];

            // Validate slot
            if (!engine.hasProperty("slot")) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Engine " + std::to_string(i) + " missing 'slot' field";
                result.issues.push_back(issue);
                result.validStructure = false;
                continue;
            }

            int slot = engine.getProperty("slot", -1);
            if (slot < 0 || slot >= 6) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Engine " + std::to_string(i) + " has invalid slot: " + std::to_string(slot) + " (must be 0-5)";
                result.issues.push_back(issue);
                result.validSlots = false;
            } else {
                if (usedSlots[slot]) {
                    ValidationIssue issue;
                    issue.severity = "ERROR";
                    issue.message = "Slot " + std::to_string(slot) + " is used multiple times";
                    result.issues.push_back(issue);
                    result.validSlots = false;
                }
                usedSlots[slot] = true;
            }

            // Validate engine type
            if (!engine.hasProperty("type")) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Engine in slot " + std::to_string(slot) + " missing 'type' field";
                result.issues.push_back(issue);
                result.validStructure = false;
                continue;
            }

            int engineType = engine.getProperty("type", -1);
            result.engineIds.push_back(engineType);

            if (engineType < MIN_ENGINE_ID || engineType > MAX_ENGINE_ID) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Slot " + std::to_string(slot) + ": Invalid engine ID " + std::to_string(engineType) + " (valid range: 0-56)";
                result.issues.push_back(issue);
                result.validEngineIds = false;
            }

            // Validate mix
            if (!engine.hasProperty("mix")) {
                ValidationIssue issue;
                issue.severity = "WARNING";
                issue.message = "Slot " + std::to_string(slot) + ": Missing 'mix' field";
                result.issues.push_back(issue);
            } else {
                float mix = engine.getProperty("mix", 1.0f);
                if (mix < 0.0f || mix > 1.0f) {
                    ValidationIssue issue;
                    issue.severity = "ERROR";
                    issue.message = "Slot " + std::to_string(slot) + ": Mix value out of range [0,1]: " + std::to_string(mix);
                    result.issues.push_back(issue);
                    result.validParameters = false;
                }
            }

            // Validate parameters
            if (!engine.hasProperty("params")) {
                ValidationIssue issue;
                issue.severity = "WARNING";
                issue.message = "Slot " + std::to_string(slot) + ": Missing 'params' array";
                result.issues.push_back(issue);
                continue;
            }

            auto paramsArray = engine.getProperty("params", juce::var());
            if (!paramsArray.isArray()) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Slot " + std::to_string(slot) + ": 'params' is not an array";
                result.issues.push_back(issue);
                result.validStructure = false;
                continue;
            }

            // Validate each parameter value
            for (int p = 0; p < paramsArray.size(); p++) {
                float value = paramsArray[p];

                if (std::isnan(value) || std::isinf(value)) {
                    ValidationIssue issue;
                    issue.severity = "ERROR";
                    issue.message = "Slot " + std::to_string(slot) + ", Param " + std::to_string(p) + ": Invalid value (NaN or Inf)";
                    result.issues.push_back(issue);
                    result.validParameters = false;
                } else if (value < 0.0f || value > 1.0f) {
                    ValidationIssue issue;
                    issue.severity = "ERROR";
                    issue.message = "Slot " + std::to_string(slot) + ", Param " + std::to_string(p) + ": Value out of range [0,1]: " + std::to_string(value);
                    result.issues.push_back(issue);
                    result.validParameters = false;
                }
            }
        }

        // Overall pass/fail
        result.passed = result.validStructure &&
                       result.validEngineIds &&
                       result.validParameters &&
                       result.validSlots;

        return result;
    }
};

void generateReport(const std::vector<PresetValidationResult>& results, const std::string& outputPath) {
    std::ofstream report(outputPath);

    report << "============================================\n";
    report << "PRESET VALIDATION REPORT\n";
    report << "============================================\n\n";

    // Summary statistics
    int totalPresets = results.size();
    int passedPresets = 0;
    int failedPresets = 0;
    int errorCount = 0;
    int warningCount = 0;

    std::map<std::string, int> categoryCount;
    std::map<int, int> engineUsage;

    for (const auto& result : results) {
        if (result.passed) {
            passedPresets++;
        } else {
            failedPresets++;
        }

        categoryCount[result.category]++;

        for (int engineId : result.engineIds) {
            engineUsage[engineId]++;
        }

        for (const auto& issue : result.issues) {
            if (issue.severity == "ERROR") {
                errorCount++;
            } else if (issue.severity == "WARNING") {
                warningCount++;
            }
        }
    }

    report << "SUMMARY\n";
    report << "-------\n";
    report << "Total Presets Tested: " << totalPresets << "\n";
    report << "Passed: " << passedPresets << " (" << (totalPresets > 0 ? passedPresets * 100 / totalPresets : 0) << "%)\n";
    report << "Failed: " << failedPresets << " (" << (totalPresets > 0 ? failedPresets * 100 / totalPresets : 0) << "%)\n";
    report << "Total Errors: " << errorCount << "\n";
    report << "Total Warnings: " << warningCount << "\n\n";

    // Category breakdown
    report << "PRESETS BY CATEGORY\n";
    report << "-------------------\n";
    for (const auto& [category, count] : categoryCount) {
        report << category << ": " << count << "\n";
    }
    report << "\n";

    // Engine usage statistics
    report << "ENGINE USAGE STATISTICS\n";
    report << "-----------------------\n";
    report << "Top 10 Most Used Engines:\n";

    std::vector<std::pair<int, int>> engineVec(engineUsage.begin(), engineUsage.end());
    std::sort(engineVec.begin(), engineVec.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    int count = 0;
    for (const auto& [engineId, usage] : engineVec) {
        if (count++ >= 10) break;
        if (engineId >= 0 && engineId <= MAX_ENGINE_ID) {
            report << "  " << ENGINE_NAMES[engineId] << " (ID " << engineId << "): " << usage << " times\n";
        }
    }
    report << "\n";

    // Detailed results
    report << "DETAILED RESULTS\n";
    report << "================\n\n";

    for (const auto& result : results) {
        report << "Preset: " << result.presetName << " (" << result.presetId << ")\n";
        report << "Category: " << result.category;
        if (!result.subcategory.empty()) {
            report << " / " << result.subcategory;
        }
        report << "\n";
        report << "Status: " << (result.passed ? "PASS" : "FAIL") << "\n";
        report << "  Valid Structure: " << (result.validStructure ? "YES" : "NO") << "\n";
        report << "  Valid Engine IDs: " << (result.validEngineIds ? "YES" : "NO") << "\n";
        report << "  Valid Parameters: " << (result.validParameters ? "YES" : "NO") << "\n";
        report << "  Valid Slots: " << (result.validSlots ? "YES" : "NO") << "\n";
        report << "  Number of Engines: " << result.numEngines << "\n";

        if (!result.engineIds.empty()) {
            report << "  Engines Used: ";
            for (size_t i = 0; i < result.engineIds.size(); i++) {
                int id = result.engineIds[i];
                if (id >= 0 && id <= MAX_ENGINE_ID) {
                    report << ENGINE_NAMES[id];
                } else {
                    report << "Invalid(" << id << ")";
                }
                if (i < result.engineIds.size() - 1) {
                    report << ", ";
                }
            }
            report << "\n";
        }

        if (!result.issues.empty()) {
            report << "  Issues:\n";
            for (const auto& issue : result.issues) {
                report << "    [" << issue.severity << "] " << issue.message << "\n";
            }
        }

        report << "\n";
    }

    report << "============================================\n";
    report << "END OF REPORT\n";
    report << "============================================\n";

    report.close();

    std::cout << "\n[INFO] Report saved to: " << outputPath << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "\n============================================" << std::endl;
    std::cout << "CHIMERA PRESET VALIDATION SYSTEM" << std::endl;
    std::cout << "============================================\n" << std::endl;

    std::string presetFilePath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/pi_deployment/JUCE_Plugin/GoldenCorpus/all_presets.json";

    if (argc > 1) {
        presetFilePath = argv[1];
    }

    SimplePresetValidator validator;

    if (!validator.loadPresetsFromFile(presetFilePath)) {
        std::cerr << "[ERROR] Failed to load presets from file" << std::endl;
        return 1;
    }

    auto results = validator.validateAllPresets();

    std::string reportPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/preset_validation_report.txt";
    generateReport(results, reportPath);

    int passed = 0;
    int failed = 0;
    for (const auto& result : results) {
        if (result.passed) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "\n============================================" << std::endl;
    std::cout << "VALIDATION COMPLETE" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "Total: " << results.size() << " presets" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    if (results.size() > 0) {
        std::cout << "Success Rate: " << (passed * 100 / results.size()) << "%" << std::endl;
    }
    std::cout << "============================================\n" << std::endl;

    return (failed == 0) ? 0 : 1;
}

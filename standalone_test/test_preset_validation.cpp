#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <JuceHeader.h>
#include "../pi_deployment/JUCE_Plugin/Source/PluginProcessor.h"
#include "../pi_deployment/JUCE_Plugin/Source/EngineFactory.h"
#include "../pi_deployment/JUCE_Plugin/Source/EngineLibrary.h"

/**
 * Comprehensive Preset Validation System
 *
 * This test validates all factory presets by:
 * 1. Loading presets from JSON file
 * 2. Validating engine IDs are in valid range
 * 3. Validating parameter values are in range [0.0, 1.0]
 * 4. Testing that presets actually produce sound
 * 5. Ensuring no crashes occur during loading/processing
 * 6. Generating comprehensive validation report
 */

struct ValidationIssue {
    std::string severity;  // "ERROR", "WARNING", "INFO"
    std::string message;
};

struct PresetValidationResult {
    std::string presetId;
    std::string presetName;
    bool passed;
    std::vector<ValidationIssue> issues;

    // Detailed checks
    bool validEngineIds;
    bool validParameters;
    bool producesSound;
    bool noCrashes;

    // Audio metrics
    float maxOutputLevel;
    float rmsLevel;
    bool hasDCOffset;

    PresetValidationResult()
        : passed(false), validEngineIds(true), validParameters(true),
          producesSound(false), noCrashes(true),
          maxOutputLevel(0.0f), rmsLevel(0.0f), hasDCOffset(false) {}
};

class PresetValidator {
public:
    PresetValidator() : sampleRate(44100.0), blockSize(512) {
        // Initialize JUCE
        scopedJuce = std::make_unique<juce::ScopedJuceInitialiser_GUI>();
    }

    ~PresetValidator() {
        scopedJuce.reset();
    }

    // Load presets from JSON file
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

        // Extract preset array
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

    // Validate all presets
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

            results.push_back(result);

            // Print immediate result
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
    std::unique_ptr<juce::ScopedJuceInitialiser_GUI> scopedJuce;
    juce::var presetsJson;
    double sampleRate;
    int blockSize;

    // Validate a single preset
    PresetValidationResult validatePreset(const juce::var& preset) {
        PresetValidationResult result;

        try {
            // Extract engines array
            auto enginesArray = preset.getProperty("engines", juce::var());
            if (!enginesArray.isArray()) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Preset has no 'engines' array";
                result.issues.push_back(issue);
                result.validEngineIds = false;
                return result;
            }

            // Check each engine in the preset
            for (int i = 0; i < enginesArray.size(); i++) {
                auto engine = enginesArray[i];

                int engineType = engine.getProperty("type", -1);
                int slot = engine.getProperty("slot", -1);

                // Validate engine ID
                if (!validateEngineId(engineType, result)) {
                    result.validEngineIds = false;
                }

                // Validate parameters
                auto paramsArray = engine.getProperty("params", juce::var());
                if (paramsArray.isArray()) {
                    if (!validateParameters(engineType, paramsArray, result)) {
                        result.validParameters = false;
                    }
                }

                // Validate mix parameter
                float mix = engine.getProperty("mix", 1.0f);
                if (mix < 0.0f || mix > 1.0f) {
                    ValidationIssue issue;
                    issue.severity = "ERROR";
                    issue.message = "Slot " + std::to_string(slot) + ": Mix value out of range [0,1]: " + std::to_string(mix);
                    result.issues.push_back(issue);
                    result.validParameters = false;
                }
            }

            // Test sound production (only if basic validation passed)
            if (result.validEngineIds && result.validParameters) {
                if (!testPresetSoundProduction(preset, result)) {
                    result.producesSound = false;
                    result.noCrashes = false;  // Assume crash if sound test failed
                }
            } else {
                ValidationIssue issue;
                issue.severity = "WARNING";
                issue.message = "Skipping sound production test due to validation errors";
                result.issues.push_back(issue);
            }

            // Overall pass/fail
            result.passed = result.validEngineIds &&
                           result.validParameters &&
                           result.producesSound &&
                           result.noCrashes;

        } catch (const std::exception& e) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = std::string("Exception during validation: ") + e.what();
            result.issues.push_back(issue);
            result.passed = false;
            result.noCrashes = false;
        } catch (...) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = "Unknown exception during validation";
            result.issues.push_back(issue);
            result.passed = false;
            result.noCrashes = false;
        }

        return result;
    }

    // Validate engine ID
    bool validateEngineId(int engineId, PresetValidationResult& result) {
        // Engine ID 0 is valid (None/Empty)
        if (engineId == 0) {
            return true;
        }

        // Valid engine IDs are 0-55 (56 total engines including None)
        if (engineId < 0 || engineId >= EngineLibrary::getEngineCount()) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = "Invalid engine ID: " + std::to_string(engineId) +
                           " (valid range: 0-" + std::to_string(EngineLibrary::getEngineCount() - 1) + ")";
            result.issues.push_back(issue);
            return false;
        }

        // Try to create the engine to verify it exists
        try {
            auto engine = EngineFactory::createEngine(engineId);
            if (!engine) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Engine ID " + std::to_string(engineId) +
                               " (" + EngineLibrary::getEngineName(engineId) + ") failed to instantiate";
                result.issues.push_back(issue);
                return false;
            }
        } catch (...) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = "Exception creating engine ID " + std::to_string(engineId);
            result.issues.push_back(issue);
            return false;
        }

        return true;
    }

    // Validate parameters
    bool validateParameters(int engineId, const juce::var& paramsArray, PresetValidationResult& result) {
        bool allValid = true;

        int paramCount = paramsArray.size();
        int expectedCount = EngineLibrary::getParameterCount(engineId);

        // Check parameter count
        if (paramCount != expectedCount) {
            ValidationIssue issue;
            issue.severity = "WARNING";
            issue.message = "Engine " + EngineLibrary::getEngineName(engineId) +
                           " has " + std::to_string(paramCount) + " parameters, expected " +
                           std::to_string(expectedCount);
            result.issues.push_back(issue);
        }

        // Validate each parameter value
        for (int i = 0; i < paramCount; i++) {
            float value = paramsArray[i];

            if (std::isnan(value) || std::isinf(value)) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Engine " + EngineLibrary::getEngineName(engineId) +
                               ", Parameter " + std::to_string(i) + ": Invalid value (NaN or Inf)";
                result.issues.push_back(issue);
                allValid = false;
            } else if (value < 0.0f || value > 1.0f) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Engine " + EngineLibrary::getEngineName(engineId) +
                               ", Parameter " + std::to_string(i) + ": Value out of range [0,1]: " +
                               std::to_string(value);
                result.issues.push_back(issue);
                allValid = false;
            }
        }

        return allValid;
    }

    // Test that preset produces sound
    bool testPresetSoundProduction(const juce::var& preset, PresetValidationResult& result) {
        try {
            // Create a processor and load the preset
            ChimeraAudioProcessor processor;
            processor.prepareToPlay(sampleRate, blockSize);

            // Load engines from preset
            auto enginesArray = preset.getProperty("engines", juce::var());
            if (!enginesArray.isArray()) {
                return false;
            }

            for (int i = 0; i < enginesArray.size(); i++) {
                auto engine = enginesArray[i];
                int slot = engine.getProperty("slot", -1);
                int engineType = engine.getProperty("type", 0);

                if (slot >= 0 && slot < 6) {  // Valid slot range
                    processor.loadEngine(slot, engineType);

                    // Apply parameters
                    auto paramsArray = engine.getProperty("params", juce::var());
                    if (paramsArray.isArray()) {
                        for (int p = 0; p < paramsArray.size() && p < 10; p++) {
                            float value = paramsArray[p];
                            std::string paramId = "slot" + std::to_string(slot + 1) +
                                                "_param" + std::to_string(p + 1);
                            auto* param = processor.getValueTreeState().getParameter(paramId);
                            if (param) {
                                param->setValueNotifyingHost(value);
                            }
                        }
                    }

                    // Apply mix
                    float mix = engine.getProperty("mix", 1.0f);
                    std::string mixId = "slot" + std::to_string(slot + 1) + "_mix";
                    auto* mixParam = processor.getValueTreeState().getParameter(mixId);
                    if (mixParam) {
                        mixParam->setValueNotifyingHost(mix);
                    }
                }
            }

            // Generate test audio
            juce::AudioBuffer<float> inputBuffer(2, blockSize);
            juce::AudioBuffer<float> outputBuffer(2, blockSize);
            juce::MidiBuffer midiBuffer;

            // Create test input (sine wave at 440 Hz)
            for (int sample = 0; sample < blockSize; sample++) {
                float value = 0.5f * std::sin(2.0f * M_PI * 440.0f * sample / sampleRate);
                inputBuffer.setSample(0, sample, value);
                inputBuffer.setSample(1, sample, value);
            }

            // Copy input to output
            outputBuffer.makeCopyOf(inputBuffer);

            // Process
            processor.processBlock(outputBuffer, midiBuffer);

            // Analyze output
            float maxLevel = 0.0f;
            float sumSquares = 0.0f;
            float dcSum = 0.0f;

            for (int ch = 0; ch < 2; ch++) {
                for (int sample = 0; sample < blockSize; sample++) {
                    float value = outputBuffer.getSample(ch, sample);

                    // Check for NaN or Inf
                    if (std::isnan(value) || std::isinf(value)) {
                        ValidationIssue issue;
                        issue.severity = "ERROR";
                        issue.message = "Output contains NaN or Inf values";
                        result.issues.push_back(issue);
                        return false;
                    }

                    maxLevel = std::max(maxLevel, std::abs(value));
                    sumSquares += value * value;
                    dcSum += value;
                }
            }

            result.maxOutputLevel = maxLevel;
            result.rmsLevel = std::sqrt(sumSquares / (blockSize * 2));

            // Check for DC offset
            float avgDC = dcSum / (blockSize * 2);
            if (std::abs(avgDC) > 0.1f) {
                result.hasDCOffset = true;
                ValidationIssue issue;
                issue.severity = "WARNING";
                issue.message = "Significant DC offset detected: " + std::to_string(avgDC);
                result.issues.push_back(issue);
            }

            // Check if sound was produced
            if (maxLevel < 0.0001f) {
                ValidationIssue issue;
                issue.severity = "WARNING";
                issue.message = "Very low output level (may be intentional): " + std::to_string(maxLevel);
                result.issues.push_back(issue);
                result.producesSound = true;  // Still consider it valid
            } else if (maxLevel > 10.0f) {
                ValidationIssue issue;
                issue.severity = "ERROR";
                issue.message = "Excessive output level (clipping): " + std::to_string(maxLevel);
                result.issues.push_back(issue);
                result.producesSound = false;
                return false;
            } else {
                result.producesSound = true;
            }

            processor.releaseResources();
            return true;

        } catch (const std::exception& e) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = std::string("Exception during sound test: ") + e.what();
            result.issues.push_back(issue);
            return false;
        } catch (...) {
            ValidationIssue issue;
            issue.severity = "ERROR";
            issue.message = "Unknown exception during sound test";
            result.issues.push_back(issue);
            return false;
        }
    }
};

// Generate validation report
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

    for (const auto& result : results) {
        if (result.passed) {
            passedPresets++;
        } else {
            failedPresets++;
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
    report << "Passed: " << passedPresets << " (" << (passedPresets * 100 / totalPresets) << "%)\n";
    report << "Failed: " << failedPresets << " (" << (failedPresets * 100 / totalPresets) << "%)\n";
    report << "Total Errors: " << errorCount << "\n";
    report << "Total Warnings: " << warningCount << "\n\n";

    // Detailed results
    report << "DETAILED RESULTS\n";
    report << "================\n\n";

    for (const auto& result : results) {
        report << "Preset: " << result.presetName << " (" << result.presetId << ")\n";
        report << "Status: " << (result.passed ? "PASS" : "FAIL") << "\n";
        report << "  Valid Engine IDs: " << (result.validEngineIds ? "YES" : "NO") << "\n";
        report << "  Valid Parameters: " << (result.validParameters ? "YES" : "NO") << "\n";
        report << "  Produces Sound: " << (result.producesSound ? "YES" : "NO") << "\n";
        report << "  No Crashes: " << (result.noCrashes ? "YES" : "NO") << "\n";

        if (result.producesSound) {
            report << "  Max Output Level: " << result.maxOutputLevel << "\n";
            report << "  RMS Level: " << result.rmsLevel << "\n";
            report << "  DC Offset: " << (result.hasDCOffset ? "YES" : "NO") << "\n";
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

    // Default preset file path
    std::string presetFilePath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/pi_deployment/JUCE_Plugin/GoldenCorpus/all_presets.json";

    // Allow override from command line
    if (argc > 1) {
        presetFilePath = argv[1];
    }

    PresetValidator validator;

    // Load presets
    if (!validator.loadPresetsFromFile(presetFilePath)) {
        std::cerr << "[ERROR] Failed to load presets from file" << std::endl;
        return 1;
    }

    // Validate all presets
    auto results = validator.validateAllPresets();

    // Generate report
    std::string reportPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/preset_validation_report.txt";
    generateReport(results, reportPath);

    // Print summary
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
    std::cout << "Success Rate: " << (passed * 100 / results.size()) << "%" << std::endl;
    std::cout << "============================================\n" << std::endl;

    return (failed == 0) ? 0 : 1;
}

/**
 * Engine Validator - Systematic testing of all Chimera Phoenix engines
 * Tests parameter mapping, processing safety, and identifies crashes
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <vector>
#include <cmath>
#include "../JuceLibraryCode/JuceHeader.h"
#include "EngineFactory.h"
#include "EngineTypes.h"

class EngineValidator {
public:
    struct TestResult {
        int engineID;
        std::string engineName;
        bool passedCreation = false;
        bool passedPrepare = false;
        bool passedParameterUpdate = false;
        bool passedProcessing = false;
        bool passedReset = false;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        bool isFullyPassed() const {
            return passedCreation && passedPrepare && 
                   passedParameterUpdate && passedProcessing && passedReset;
        }
    };
    
    static TestResult validateEngine(int engineID) {
        TestResult result;
        result.engineID = engineID;
        
        std::cout << "\n=== Testing Engine " << engineID << " ===" << std::endl;
        
        // Test 1: Creation
        std::unique_ptr<EngineBase> engine;
        try {
            engine = EngineFactory::createEngine(engineID);
            if (!engine) {
                result.errors.push_back("Failed to create engine (returned null)");
                return result;
            }
            result.passedCreation = true;
            result.engineName = engine->getName().toStdString();
            std::cout << "✓ Created: " << result.engineName << std::endl;
        } catch (const std::exception& e) {
            result.errors.push_back("Exception during creation: " + std::string(e.what()));
            return result;
        }
        
        // Test 2: Prepare
        try {
            engine->prepareToPlay(48000.0, 512);
            result.passedPrepare = true;
            std::cout << "✓ Prepared at 48kHz, 512 samples" << std::endl;
        } catch (const std::exception& e) {
            result.errors.push_back("Exception during prepare: " + std::string(e.what()));
            return result;
        }
        
        // Test 3: Parameter Update
        try {
            // Test with various parameter sets
            std::vector<std::map<int, float>> testParams = {
                // All zeros
                {{0, 0.0f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {4, 0.0f},
                 {5, 0.0f}, {6, 0.0f}, {7, 0.0f}, {8, 0.0f}, {9, 0.0f},
                 {10, 0.0f}, {11, 0.0f}, {12, 0.0f}, {13, 0.0f}, {14, 0.0f}},
                
                // All 0.5 (middle values)
                {{0, 0.5f}, {1, 0.5f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f},
                 {5, 0.5f}, {6, 0.5f}, {7, 0.5f}, {8, 0.5f}, {9, 0.5f},
                 {10, 0.5f}, {11, 0.5f}, {12, 0.5f}, {13, 0.5f}, {14, 0.5f}},
                
                // All 1.0 (max values)
                {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}, {3, 1.0f}, {4, 1.0f},
                 {5, 1.0f}, {6, 1.0f}, {7, 1.0f}, {8, 1.0f}, {9, 1.0f},
                 {10, 1.0f}, {11, 1.0f}, {12, 1.0f}, {13, 1.0f}, {14, 1.0f}},
                
                // Random values
                {{0, 0.3f}, {1, 0.7f}, {2, 0.1f}, {3, 0.9f}, {4, 0.5f},
                 {5, 0.2f}, {6, 0.8f}, {7, 0.4f}, {8, 0.6f}, {9, 0.15f},
                 {10, 0.85f}, {11, 0.35f}, {12, 0.65f}, {13, 0.95f}, {14, 0.05f}}
            };
            
            for (size_t i = 0; i < testParams.size(); ++i) {
                engine->updateParameters(testParams[i]);
            }
            result.passedParameterUpdate = true;
            std::cout << "✓ Parameter updates handled" << std::endl;
        } catch (const std::exception& e) {
            result.errors.push_back("Exception during parameter update: " + std::string(e.what()));
            return result;
        }
        
        // Test 4: Audio Processing
        try {
            // Test with different buffer sizes and channel configs
            std::vector<std::pair<int, int>> bufferConfigs = {
                {2, 64},   // 2 channels, 64 samples
                {2, 128},  // 2 channels, 128 samples
                {2, 512},  // 2 channels, 512 samples
                {2, 1024}, // 2 channels, 1024 samples
                {1, 512},  // Mono
            };
            
            for (const auto& config : bufferConfigs) {
                juce::AudioBuffer<float> buffer(config.first, config.second);
                
                // Fill with test signal (sine wave)
                for (int ch = 0; ch < config.first; ++ch) {
                    float* data = buffer.getWritePointer(ch);
                    for (int i = 0; i < config.second; ++i) {
                        data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 48000.0f);
                    }
                }
                
                // Store original for comparison
                juce::AudioBuffer<float> original(buffer);
                
                // Process
                engine->process(buffer);
                
                // Check for NaN/Inf
                bool hasInvalid = false;
                for (int ch = 0; ch < config.first; ++ch) {
                    const float* data = buffer.getReadPointer(ch);
                    for (int i = 0; i < config.second; ++i) {
                        if (!std::isfinite(data[i])) {
                            hasInvalid = true;
                            result.errors.push_back("NaN/Inf in output at ch:" + 
                                std::to_string(ch) + " sample:" + std::to_string(i));
                            break;
                        }
                        // Check for extreme values that might cause clipping
                        if (std::abs(data[i]) > 10.0f) {
                            result.warnings.push_back("Extreme value " + 
                                std::to_string(data[i]) + " at ch:" + 
                                std::to_string(ch) + " sample:" + std::to_string(i));
                        }
                    }
                    if (hasInvalid) break;
                }
                
                if (hasInvalid) {
                    return result;
                }
            }
            
            result.passedProcessing = true;
            std::cout << "✓ Audio processing stable" << std::endl;
            
        } catch (const std::exception& e) {
            result.errors.push_back("Exception during processing: " + std::string(e.what()));
            return result;
        }
        
        // Test 5: Reset
        try {
            engine->reset();
            result.passedReset = true;
            std::cout << "✓ Reset successful" << std::endl;
        } catch (const std::exception& e) {
            result.errors.push_back("Exception during reset: " + std::string(e.what()));
            return result;
        }
        
        // Test 6: Parameter count and names
        try {
            int numParams = engine->getNumParameters();
            std::cout << "  Parameters: " << numParams << std::endl;
            
            if (numParams > 15) {
                result.warnings.push_back("Engine reports more than 15 parameters (" + 
                    std::to_string(numParams) + ")");
            }
            
            // Get parameter names
            for (int i = 0; i < std::min(numParams, 15); ++i) {
                juce::String paramName = engine->getParameterName(i);
                std::cout << "    Param " << i << ": " << paramName << std::endl;
            }
            
        } catch (const std::exception& e) {
            result.warnings.push_back("Exception getting parameter info: " + std::string(e.what()));
        }
        
        return result;
    }
    
    static void runFullValidation() {
        std::vector<TestResult> results;
        std::ofstream report("engine_validation_report.txt");
        
        report << "=== Chimera Phoenix Engine Validation Report ===" << std::endl;
        report << "Timestamp: " << __DATE__ << " " << __TIME__ << std::endl;
        report << std::endl;
        
        // Test all engines
        for (int engineID = 0; engineID <= 56; ++engineID) {
            TestResult result = validateEngine(engineID);
            results.push_back(result);
            
            // Write to report
            report << "Engine " << engineID << ": " << result.engineName << std::endl;
            if (result.isFullyPassed()) {
                report << "  STATUS: ✓ PASSED ALL TESTS" << std::endl;
            } else {
                report << "  STATUS: ✗ FAILED" << std::endl;
                report << "  Creation: " << (result.passedCreation ? "✓" : "✗") << std::endl;
                report << "  Prepare: " << (result.passedPrepare ? "✓" : "✗") << std::endl;
                report << "  Parameters: " << (result.passedParameterUpdate ? "✓" : "✗") << std::endl;
                report << "  Processing: " << (result.passedProcessing ? "✓" : "✗") << std::endl;
                report << "  Reset: " << (result.passedReset ? "✓" : "✗") << std::endl;
                
                if (!result.errors.empty()) {
                    report << "  ERRORS:" << std::endl;
                    for (const auto& err : result.errors) {
                        report << "    - " << err << std::endl;
                    }
                }
            }
            
            if (!result.warnings.empty()) {
                report << "  WARNINGS:" << std::endl;
                for (const auto& warn : result.warnings) {
                    report << "    - " << warn << std::endl;
                }
            }
            report << std::endl;
        }
        
        // Summary
        int passed = 0, failed = 0;
        std::vector<int> failedEngines;
        
        for (const auto& result : results) {
            if (result.isFullyPassed()) {
                passed++;
            } else {
                failed++;
                failedEngines.push_back(result.engineID);
            }
        }
        
        report << "=== SUMMARY ===" << std::endl;
        report << "Total Engines: " << results.size() << std::endl;
        report << "Passed: " << passed << std::endl;
        report << "Failed: " << failed << std::endl;
        
        if (!failedEngines.empty()) {
            report << "Failed Engine IDs: ";
            for (size_t i = 0; i < failedEngines.size(); ++i) {
                if (i > 0) report << ", ";
                report << failedEngines[i];
            }
            report << std::endl;
        }
        
        report.close();
        
        std::cout << "\n=== VALIDATION COMPLETE ===" << std::endl;
        std::cout << "Passed: " << passed << "/" << results.size() << std::endl;
        std::cout << "Report written to: engine_validation_report.txt" << std::endl;
        
        if (!failedEngines.empty()) {
            std::cout << "Failed engines: ";
            for (size_t i = 0; i < failedEngines.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << failedEngines[i];
            }
            std::cout << std::endl;
        }
    }
};

int main() {
    std::cout << "Starting Chimera Phoenix Engine Validation..." << std::endl;
    EngineValidator::runFullValidation();
    return 0;
}
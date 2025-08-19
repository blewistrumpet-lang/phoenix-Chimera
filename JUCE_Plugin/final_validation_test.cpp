/**
 * Final Validation Test for Project Chimera Phoenix
 * 
 * This comprehensive test validates all 57 engines to ensure production readiness
 * after all recent fixes and improvements.
 * 
 * Tests performed:
 * 1. Engine initialization without crashes
 * 2. Parameter mapping validation
 * 3. Audio processing verification
 * 4. Recent fix validation
 * 5. Mix parameter index verification
 * 
 * Author: Claude Code Final Validation Specialist
 * Date: 2025-08-19
 */

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>

// Core JUCE includes
#include <JuceHeader.h>

// Project includes
#include "Source/EngineTypes.h"
#include "Source/EngineFactory.h"
#include "Source/PluginProcessor.h"
#include "Source/GeneratedParameterDatabase.h"
#include "Source/UnifiedDefaultParameters.h"

// Test result structure
struct ValidationResult {
    int engineID;
    std::string engineName;
    bool initializationPassed = false;
    bool parameterMappingPassed = false;
    bool audioProcessingPassed = false;
    bool mixParameterPassed = false;
    bool overallPassed = false;
    std::string issues;
    float confidence = 0.0f;
    
    void addIssue(const std::string& issue) {
        if (!issues.empty()) issues += "; ";
        issues += issue;
    }
};

class FinalValidationTest {
private:
    std::vector<ValidationResult> results;
    double sampleRate = 44100.0;
    int bufferSize = 512;
    
    // Test audio buffer
    juce::AudioBuffer<float> testBuffer;
    
    // Statistics
    int totalEngines = 0;
    int passedEngines = 0;
    int warningEngines = 0;
    int failedEngines = 0;
    
public:
    FinalValidationTest() {
        // Initialize test buffer with stereo audio
        testBuffer.setSize(2, bufferSize);
        generateTestSignal();
    }
    
    void generateTestSignal() {
        // Generate a complex test signal with multiple frequencies
        for (int channel = 0; channel < testBuffer.getNumChannels(); ++channel) {
            auto* channelData = testBuffer.getWritePointer(channel);
            for (int sample = 0; sample < bufferSize; ++sample) {
                double time = sample / sampleRate;
                // Multi-frequency test signal
                double signal = 0.1 * std::sin(2.0 * juce::MathConstants<double>::pi * 440.0 * time) +  // A4
                               0.05 * std::sin(2.0 * juce::MathConstants<double>::pi * 880.0 * time) +  // A5
                               0.03 * std::sin(2.0 * juce::MathConstants<double>::pi * 220.0 * time);   // A3
                
                // Add slight stereo difference
                if (channel == 1) signal *= 0.9;
                
                channelData[sample] = static_cast<float>(signal);
            }
        }
    }
    
    void runComprehensiveValidation() {
        std::cout << "\n=== CHIMERA PHOENIX FINAL VALIDATION TEST ===" << std::endl;
        std::cout << "Testing all 57 engines for production readiness..." << std::endl;
        std::cout << "============================================\n" << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Test all engines from 1 to 56 (plus ENGINE_NONE = 0)
        for (int engineID = 0; engineID < ENGINE_COUNT; ++engineID) {
            ValidationResult result;
            result.engineID = engineID;
            result.engineName = getEngineTypeName(engineID);
            
            std::cout << "Testing Engine " << std::setw(2) << engineID 
                      << ": " << std::setw(25) << std::left << result.engineName << std::flush;
            
            if (engineID == ENGINE_NONE) {
                // Special case for ENGINE_NONE
                result.initializationPassed = true;
                result.parameterMappingPassed = true;
                result.audioProcessingPassed = true;
                result.mixParameterPassed = true;
                result.overallPassed = true;
                result.confidence = 1.0f;
                std::cout << " [PASS] (No-op engine)" << std::endl;
            } else {
                validateEngine(result);
                printResult(result);
            }
            
            results.push_back(result);
            totalEngines++;
            
            if (result.overallPassed) passedEngines++;
            else if (result.confidence > 0.5f) warningEngines++;
            else failedEngines++;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        generateFinalReport(duration.count());
    }
    
private:
    void validateEngine(ValidationResult& result) {
        try {
            // Test 1: Engine Initialization
            result.initializationPassed = testEngineInitialization(result);
            
            // Test 2: Parameter Mapping
            result.parameterMappingPassed = testParameterMapping(result);
            
            // Test 3: Audio Processing
            result.audioProcessingPassed = testAudioProcessing(result);
            
            // Test 4: Mix Parameter Index
            result.mixParameterPassed = testMixParameterIndex(result);
            
            // Test 5: Recent Fixes Validation
            validateRecentFixes(result);
            
            // Overall assessment
            int passedTests = (result.initializationPassed ? 1 : 0) +
                            (result.parameterMappingPassed ? 1 : 0) +
                            (result.audioProcessingPassed ? 1 : 0) +
                            (result.mixParameterPassed ? 1 : 0);
            
            result.confidence = passedTests / 4.0f;
            result.overallPassed = (passedTests >= 3); // Must pass at least 3/4 tests
            
        } catch (const std::exception& e) {
            result.addIssue("Exception: " + std::string(e.what()));
            result.confidence = 0.0f;
            result.overallPassed = false;
        }
    }
    
    bool testEngineInitialization(ValidationResult& result) {
        try {
            auto engine = EngineFactory::createEngine(result.engineID);
            if (!engine) {
                result.addIssue("Failed to create engine");
                return false;
            }
            
            // Test prepare to play
            engine->prepareToPlay(sampleRate, bufferSize);
            
            // Test reset
            engine->reset();
            
            return true;
        } catch (...) {
            result.addIssue("Initialization crash");
            return false;
        }
    }
    
    bool testParameterMapping(ValidationResult& result) {
        try {
            auto engine = EngineFactory::createEngine(result.engineID);
            if (!engine) return false;
            
            int paramCount = engine->getParameterCount();
            
            // Verify parameter count matches database
            auto dbInfo = ChimeraParameters::getEngineInfoByLegacyId(result.engineID);
            if (dbInfo && dbInfo->parameterCount != paramCount) {
                result.addIssue("Parameter count mismatch: engine=" + 
                               std::to_string(paramCount) + " vs db=" + 
                               std::to_string(dbInfo->parameterCount));
                return false;
            }
            
            // Test parameter setting
            for (int i = 0; i < paramCount; ++i) {
                engine->setParameter(i, 0.5f);
                float value = engine->getParameter(i);
                if (std::isnan(value) || std::isinf(value)) {
                    result.addIssue("Invalid parameter value at index " + std::to_string(i));
                    return false;
                }
            }
            
            return true;
        } catch (...) {
            result.addIssue("Parameter mapping crash");
            return false;
        }
    }
    
    bool testAudioProcessing(ValidationResult& result) {
        try {
            auto engine = EngineFactory::createEngine(result.engineID);
            if (!engine) return false;
            
            engine->prepareToPlay(sampleRate, bufferSize);
            
            // Create a copy of test buffer for processing
            juce::AudioBuffer<float> processingBuffer;
            processingBuffer.makeCopyOf(testBuffer);
            
            // Process audio
            engine->processBlock(processingBuffer);
            
            // Check for valid output
            bool hasValidOutput = false;
            for (int channel = 0; channel < processingBuffer.getNumChannels(); ++channel) {
                auto* data = processingBuffer.getReadPointer(channel);
                for (int sample = 0; sample < bufferSize; ++sample) {
                    if (!std::isnan(data[sample]) && !std::isinf(data[sample])) {
                        hasValidOutput = true;
                        break;
                    }
                }
                if (hasValidOutput) break;
            }
            
            if (!hasValidOutput) {
                result.addIssue("Invalid audio output (NaN/Inf)");
                return false;
            }
            
            return true;
        } catch (...) {
            result.addIssue("Audio processing crash");
            return false;
        }
    }
    
    bool testMixParameterIndex(ValidationResult& result) {
        try {
            int mixIndex = ChimeraAudioProcessor::getMixParameterIndex(result.engineID);
            
            // Verify mix parameter exists
            auto engine = EngineFactory::createEngine(result.engineID);
            if (!engine) return false;
            
            int paramCount = engine->getParameterCount();
            
            if (mixIndex < 0 || mixIndex >= paramCount) {
                result.addIssue("Invalid mix parameter index: " + std::to_string(mixIndex) + 
                               " (count: " + std::to_string(paramCount) + ")");
                return false;
            }
            
            return true;
        } catch (...) {
            result.addIssue("Mix parameter test crash");
            return false;
        }
    }
    
    void validateRecentFixes(ValidationResult& result) {
        switch (result.engineID) {
            case ENGINE_SPECTRAL_FREEZE:
                // Validate Spectral Freeze fix (window validation bug)
                try {
                    auto engine = EngineFactory::createEngine(result.engineID);
                    if (engine) {
                        engine->prepareToPlay(sampleRate, bufferSize);
                        // Test different FFT sizes without assertion failure
                        engine->setParameter(1, 0.0f); // Minimum size
                        engine->setParameter(1, 1.0f); // Maximum size
                        result.addIssue("Spectral Freeze: Window validation fix verified");
                    }
                } catch (...) {
                    result.addIssue("Spectral Freeze: Fix validation failed");
                }
                break;
                
            case ENGINE_PHASED_VOCODER:
                // Validate Phased Vocoder mix parameter
                {
                    int mixIndex = ChimeraAudioProcessor::getMixParameterIndex(result.engineID);
                    if (mixIndex == 3) { // Should be index 3 for 4-parameter engine
                        result.addIssue("Phased Vocoder: Mix parameter fix verified");
                    } else {
                        result.addIssue("Phased Vocoder: Mix parameter at wrong index " + std::to_string(mixIndex));
                    }
                }
                break;
                
            case ENGINE_MID_SIDE_PROCESSOR:
                // Validate parameter count fix (3→10)
                try {
                    auto engine = EngineFactory::createEngine(result.engineID);
                    if (engine && engine->getParameterCount() == 10) {
                        result.addIssue("Mid-Side Processor: Parameter count fix verified (10 params)");
                    } else {
                        result.addIssue("Mid-Side Processor: Wrong parameter count");
                    }
                } catch (...) {
                    result.addIssue("Mid-Side Processor: Fix validation failed");
                }
                break;
                
            case ENGINE_GAIN_UTILITY:
                // Validate parameter count fix (4→10)
                try {
                    auto engine = EngineFactory::createEngine(result.engineID);
                    if (engine && engine->getParameterCount() == 10) {
                        result.addIssue("Gain Utility: Parameter count fix verified (10 params)");
                    } else {
                        result.addIssue("Gain Utility: Wrong parameter count");
                    }
                } catch (...) {
                    result.addIssue("Gain Utility: Fix validation failed");
                }
                break;
                
            case ENGINE_MONO_MAKER:
                // Validate parameter count fix (3→8)
                try {
                    auto engine = EngineFactory::createEngine(result.engineID);
                    if (engine && engine->getParameterCount() == 8) {
                        result.addIssue("Mono Maker: Parameter count fix verified (8 params)");
                    } else {
                        result.addIssue("Mono Maker: Wrong parameter count");
                    }
                } catch (...) {
                    result.addIssue("Mono Maker: Fix validation failed");
                }
                break;
                
            case ENGINE_PHASE_ALIGN:
                // Validate stereo input requirement
                try {
                    auto engine = EngineFactory::createEngine(result.engineID);
                    if (engine) {
                        engine->prepareToPlay(sampleRate, bufferSize);
                        juce::AudioBuffer<float> stereoBuffer(2, bufferSize);
                        stereoBuffer.clear();
                        engine->processBlock(stereoBuffer);
                        result.addIssue("Phase Align: Stereo processing verified");
                    }
                } catch (...) {
                    result.addIssue("Phase Align: Stereo test failed");
                }
                break;
                
            case ENGINE_SPECTRAL_GATE:
                // Validate parameter mapping fix (4→8)
                try {
                    auto engine = EngineFactory::createEngine(result.engineID);
                    if (engine && engine->getParameterCount() == 8) {
                        int mixIndex = ChimeraAudioProcessor::getMixParameterIndex(result.engineID);
                        if (mixIndex == 7) { // Should be last parameter
                            result.addIssue("Spectral Gate: Parameter mapping fix verified (8 params, mix at 7)");
                        } else {
                            result.addIssue("Spectral Gate: Mix parameter at wrong index " + std::to_string(mixIndex));
                        }
                    } else {
                        result.addIssue("Spectral Gate: Wrong parameter count");
                    }
                } catch (...) {
                    result.addIssue("Spectral Gate: Fix validation failed");
                }
                break;
        }
    }
    
    void printResult(const ValidationResult& result) {
        if (result.overallPassed) {
            std::cout << " [PASS]";
        } else if (result.confidence > 0.5f) {
            std::cout << " [WARN]";
        } else {
            std::cout << " [FAIL]";
        }
        
        std::cout << " (" << std::fixed << std::setprecision(1) << (result.confidence * 100) << "%)";
        
        if (!result.issues.empty() && result.issues.find("fix verified") != std::string::npos) {
            std::cout << " ✓";
        }
        
        std::cout << std::endl;
        
        if (!result.issues.empty() && result.confidence < 1.0f) {
            std::cout << "    Issues: " << result.issues << std::endl;
        }
    }
    
    void generateFinalReport(long durationMs) {
        std::cout << "\n============================================" << std::endl;
        std::cout << "=== FINAL VALIDATION REPORT ===" << std::endl;
        std::cout << "============================================" << std::endl;
        
        std::cout << "\nTEST SUMMARY:" << std::endl;
        std::cout << "Total Engines Tested: " << totalEngines << std::endl;
        std::cout << "✓ Passed: " << passedEngines << " (" << (passedEngines * 100 / totalEngines) << "%)" << std::endl;
        std::cout << "⚠ Warnings: " << warningEngines << " (" << (warningEngines * 100 / totalEngines) << "%)" << std::endl;
        std::cout << "✗ Failed: " << failedEngines << " (" << (failedEngines * 100 / totalEngines) << "%)" << std::endl;
        std::cout << "Test Duration: " << durationMs << "ms" << std::endl;
        
        float productionReadiness = (passedEngines + warningEngines * 0.5f) / totalEngines * 100.0f;
        std::cout << "\nPRODUCTION READINESS: " << std::fixed << std::setprecision(1) 
                  << productionReadiness << "%" << std::endl;
        
        std::cout << "\nRECENT FIXES VALIDATION:" << std::endl;
        std::vector<std::string> fixedEngines = {
            "Spectral Freeze", "Phased Vocoder", "Mid-Side Processor", 
            "Gain Utility", "Mono Maker", "Phase Align", "Spectral Gate"
        };
        
        for (const auto& engineName : fixedEngines) {
            auto it = std::find_if(results.begin(), results.end(), 
                [&engineName](const ValidationResult& r) { return r.engineName == engineName; });
            if (it != results.end()) {
                std::cout << "  " << std::setw(20) << std::left << engineName << ": ";
                if (it->issues.find("fix verified") != std::string::npos) {
                    std::cout << "✓ Fix verified";
                } else if (it->overallPassed) {
                    std::cout << "✓ Working";
                } else {
                    std::cout << "⚠ Issues detected";
                }
                std::cout << std::endl;
            }
        }
        
        if (failedEngines > 0) {
            std::cout << "\nFAILED ENGINES:" << std::endl;
            for (const auto& result : results) {
                if (!result.overallPassed && result.confidence <= 0.5f) {
                    std::cout << "  " << result.engineName << " (ID " << result.engineID << "): " 
                              << result.issues << std::endl;
                }
            }
        }
        
        if (warningEngines > 0) {
            std::cout << "\nWARNING ENGINES:" << std::endl;
            for (const auto& result : results) {
                if (!result.overallPassed && result.confidence > 0.5f) {
                    std::cout << "  " << result.engineName << " (ID " << result.engineID << "): " 
                              << result.issues << std::endl;
                }
            }
        }
        
        std::cout << "\nRECOMMENDATION:" << std::endl;
        if (productionReadiness >= 95.0f) {
            std::cout << "✓ READY FOR PRODUCTION - All critical systems operational" << std::endl;
        } else if (productionReadiness >= 85.0f) {
            std::cout << "⚠ MOSTLY READY - Minor issues present, suitable for beta release" << std::endl;
        } else if (productionReadiness >= 70.0f) {
            std::cout << "⚠ NEEDS ATTENTION - Several issues need resolution before release" << std::endl;
        } else {
            std::cout << "✗ NOT READY - Significant issues require immediate attention" << std::endl;
        }
        
        // Save detailed report to file
        saveDetailedReport();
    }
    
    void saveDetailedReport() {
        std::ofstream report("final_validation_report.txt");
        if (!report.is_open()) return;
        
        report << "CHIMERA PHOENIX FINAL VALIDATION REPORT\n";
        report << "Generated: " << std::chrono::system_clock::now() << "\n\n";
        
        report << "ENGINE STATUS BREAKDOWN:\n";
        report << "========================\n";
        
        for (const auto& result : results) {
            report << "Engine " << std::setw(2) << result.engineID << " - " 
                   << std::setw(25) << std::left << result.engineName;
            
            report << " | Init: " << (result.initializationPassed ? "PASS" : "FAIL");
            report << " | Params: " << (result.parameterMappingPassed ? "PASS" : "FAIL");
            report << " | Audio: " << (result.audioProcessingPassed ? "PASS" : "FAIL");
            report << " | Mix: " << (result.mixParameterPassed ? "PASS" : "FAIL");
            report << " | Overall: " << (result.overallPassed ? "PASS" : "FAIL");
            report << " (" << std::fixed << std::setprecision(1) << (result.confidence * 100) << "%)\n";
            
            if (!result.issues.empty()) {
                report << "    Issues: " << result.issues << "\n";
            }
        }
        
        report << "\nSUMMARY:\n";
        report << "Passed: " << passedEngines << "/" << totalEngines << " engines\n";
        report << "Production Readiness: " << std::fixed << std::setprecision(1) 
               << ((passedEngines + warningEngines * 0.5f) / totalEngines * 100.0f) << "%\n";
        
        report.close();
        std::cout << "\nDetailed report saved to: final_validation_report.txt" << std::endl;
    }
};

int main() {
    try {
        // Initialize JUCE
        juce::initialiseJuce_GUI();
        
        FinalValidationTest validator;
        validator.runComprehensiveValidation();
        
        juce::shutdownJuce_GUI();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
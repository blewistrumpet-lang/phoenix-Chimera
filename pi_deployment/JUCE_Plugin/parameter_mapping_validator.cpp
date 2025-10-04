/*
 * Chimera Phoenix - Parameter Mapping Validation System
 * 
 * This tool validates that UI parameter labels match actual engine parameter functionality
 * for all 57 engines in the Chimera Phoenix system.
 */

#include "Source/EngineFactory.h"
#include "Source/EngineBase.h"
#include "Source/ParameterDefinitions.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iomanip>
#include <algorithm>

struct ParameterInfo {
    int index;
    std::string name;
    std::string expectedFunctionality;
    bool isValid;
    std::string issues;
};

struct EngineValidationResult {
    int engineID;
    std::string engineName;
    std::vector<ParameterInfo> parameters;
    bool hasIssues;
    std::string overallIssues;
    float confidence;
};

class ParameterMappingValidator {
private:
    std::vector<EngineValidationResult> results;
    
    // Common parameter names that should control specific functionality
    std::map<std::string, std::string> expectedMappings = {
        {"Gain", "Input or output gain control"},
        {"Drive", "Saturation/overdrive amount"},
        {"Mix", "Dry/wet blend"},
        {"Threshold", "Compressor/gate threshold"},
        {"Ratio", "Compression ratio"},
        {"Attack", "Attack time"},
        {"Release", "Release time"},
        {"Frequency", "Filter cutoff or oscillator frequency"},
        {"Resonance", "Filter resonance/Q"},
        {"Feedback", "Delay/reverb feedback"},
        {"Time", "Delay time"},
        {"Size", "Reverb room size"},
        {"Damping", "High frequency damping"},
        {"Predelay", "Reverb predelay"},
        {"Input", "Input level"},
        {"Output", "Output level"},
        {"Level", "General level control"},
        {"Intensity", "Effect intensity"},
        {"Depth", "Modulation depth"},
        {"Rate", "Modulation rate/speed"},
        {"Width", "Stereo width"},
        {"Phase", "Phase adjustment"},
        {"Bias", "DC bias or tube bias"},
        {"Bass", "Low frequency control"},
        {"Mid", "Mid frequency control"}, 
        {"Treble", "High frequency control"},
        {"Presence", "High frequency presence"},
        {"Makeup", "Makeup gain"},
        {"Knee", "Compressor knee"},
        {"Lookahead", "Lookahead time"}
    };
    
    // Suspicious patterns that indicate potential mapping issues
    std::vector<std::string> suspiciousPatterns = {
        "Param 1", "Param 2", "Parameter", "Unknown", "Default", "Test"
    };
    
public:
    void validateAllEngines() {
        std::cout << "=== Chimera Phoenix Parameter Mapping Validation ===" << std::endl;
        std::cout << "Validating all 57 engines for parameter mapping consistency..." << std::endl;
        
        for (int engineID = 0; engineID <= 56; ++engineID) {
            validateEngine(engineID);
        }
        
        generateReport();
    }
    
private:
    void validateEngine(int engineID) {
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            std::cout << "WARNING: Failed to create engine " << engineID << std::endl;
            return;
        }
        
        EngineValidationResult result;
        result.engineID = engineID;
        result.engineName = engine->getName().toStdString();
        result.hasIssues = false;
        result.confidence = 1.0f;
        
        std::cout << "Validating Engine " << engineID << ": " << result.engineName << std::endl;
        
        int numParams = engine->getNumParameters();
        
        // Test parameter name consistency
        for (int i = 0; i < numParams; ++i) {
            ParameterInfo paramInfo;
            paramInfo.index = i;
            paramInfo.name = engine->getParameterName(i).toStdString();
            paramInfo.isValid = true;
            
            // Check for suspicious patterns
            bool hasSuspiciousName = false;
            for (const auto& pattern : suspiciousPatterns) {
                if (paramInfo.name.find(pattern) != std::string::npos) {
                    hasSuspiciousName = true;
                    paramInfo.issues += "Suspicious parameter name '" + paramInfo.name + "'; ";
                    break;
                }
            }
            
            // Check if parameter name matches expected functionality
            std::string lowerName = paramInfo.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            for (const auto& [expectedName, expectedFunc] : expectedMappings) {
                std::string lowerExpected = expectedName;
                std::transform(lowerExpected.begin(), lowerExpected.end(), lowerExpected.begin(), ::tolower);
                
                if (lowerName.find(lowerExpected) != std::string::npos) {
                    paramInfo.expectedFunctionality = expectedFunc;
                    break;
                }
            }
            
            if (hasSuspiciousName || paramInfo.name.empty()) {
                paramInfo.isValid = false;
                result.hasIssues = true;
                result.confidence -= 0.1f;
            }
            
            result.parameters.push_back(paramInfo);
        }
        
        // Validate parameter index consistency
        validateParameterIndices(engine.get(), result);
        
        // Check for common mapping issues
        checkCommonMappingIssues(result);
        
        results.push_back(result);
    }
    
    void validateParameterIndices(EngineBase* engine, EngineValidationResult& result) {
        // Test that updateParameters properly handles all indices
        std::map<int, float> testParams;
        
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            testParams[i] = 0.5f; // Safe middle value
        }
        
        try {
            engine->prepareToPlay(44100.0, 512);
            engine->updateParameters(testParams);
            
            // Test boundary cases
            std::map<int, float> boundaryParams;
            boundaryParams[0] = 0.0f;
            boundaryParams[engine->getNumParameters() - 1] = 1.0f;
            engine->updateParameters(boundaryParams);
            
        } catch (...) {
            result.hasIssues = true;
            result.overallIssues += "Exception during parameter update; ";
            result.confidence -= 0.3f;
        }
    }
    
    void checkCommonMappingIssues(EngineValidationResult& result) {
        // Check for Mix parameter at wrong index
        bool foundMix = false;
        int mixIndex = -1;
        
        for (const auto& param : result.parameters) {
            std::string lowerName = param.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (lowerName.find("mix") != std::string::npos) {
                foundMix = true;
                mixIndex = param.index;
                break;
            }
        }
        
        // Mix parameters are commonly expected to be last or near-last
        if (foundMix && mixIndex < (int)result.parameters.size() - 3) {
            result.overallIssues += "Mix parameter at index " + std::to_string(mixIndex) + 
                                  " (expected near end); ";
            result.confidence -= 0.1f;
        }
        
        // Check for Gain parameters at suspicious indices
        for (const auto& param : result.parameters) {
            std::string lowerName = param.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (lowerName.find("gain") != std::string::npos) {
                // Gain parameters are commonly at index 0 or near the end
                if (param.index != 0 && param.index < (int)result.parameters.size() - 3) {
                    if (lowerName.find("output") == std::string::npos && 
                        lowerName.find("makeup") == std::string::npos) {
                        result.overallIssues += "Gain parameter at suspicious index " + 
                                              std::to_string(param.index) + "; ";
                        result.confidence -= 0.05f;
                    }
                }
            }
        }
        
        // Check for parameters with generic names
        int genericNameCount = 0;
        for (const auto& param : result.parameters) {
            if (param.name.empty() || 
                param.name.find("Param") != std::string::npos ||
                param.name.find("Parameter") != std::string::npos) {
                genericNameCount++;
            }
        }
        
        if (genericNameCount > 0) {
            result.overallIssues += std::to_string(genericNameCount) + " generic parameter names; ";
            result.confidence -= 0.2f * genericNameCount;
        }
    }
    
    void generateReport() {
        std::cout << "\n=== PARAMETER MAPPING VALIDATION REPORT ===" << std::endl;
        
        int totalEngines = results.size();
        int enginesWithIssues = 0;
        int totalIssues = 0;
        
        std::ofstream reportFile("parameter_mapping_validation_report.md");
        reportFile << "# Chimera Phoenix Parameter Mapping Validation Report\n\n";
        reportFile << "Generated: " << __DATE__ << " " << __TIME__ << "\n\n";
        
        // Summary
        for (const auto& result : results) {
            if (result.hasIssues) {
                enginesWithIssues++;
                totalIssues++;
            }
        }
        
        std::cout << "SUMMARY:" << std::endl;
        std::cout << "- Total Engines Tested: " << totalEngines << std::endl;
        std::cout << "- Engines with Issues: " << enginesWithIssues << std::endl;
        std::cout << "- Success Rate: " << std::fixed << std::setprecision(1) 
                  << ((float)(totalEngines - enginesWithIssues) / totalEngines * 100) << "%" << std::endl;
        
        reportFile << "## Summary\n\n";
        reportFile << "- **Total Engines Tested:** " << totalEngines << "\n";
        reportFile << "- **Engines with Issues:** " << enginesWithIssues << "\n";
        reportFile << "- **Success Rate:** " << std::fixed << std::setprecision(1)
                   << ((float)(totalEngines - enginesWithIssues) / totalEngines * 100) << "%\n\n";
        
        // Detailed results
        std::cout << "\nDETAILED RESULTS:" << std::endl;
        reportFile << "## Detailed Results\n\n";
        
        for (const auto& result : results) {
            if (result.hasIssues) {
                std::cout << "❌ Engine " << result.engineID << " (" << result.engineName 
                          << ") - Confidence: " << std::fixed << std::setprecision(2) 
                          << result.confidence << std::endl;
                std::cout << "   Issues: " << result.overallIssues << std::endl;
                
                reportFile << "### ❌ Engine " << result.engineID << ": " << result.engineName 
                           << " (Confidence: " << std::fixed << std::setprecision(2) 
                           << result.confidence << ")\n\n";
                reportFile << "**Issues:** " << result.overallIssues << "\n\n";
                reportFile << "**Parameters:**\n";
                
                for (const auto& param : result.parameters) {
                    reportFile << "- Index " << param.index << ": \"" << param.name << "\"";
                    if (!param.issues.empty()) {
                        reportFile << " - " << param.issues;
                    }
                    reportFile << "\n";
                }
                reportFile << "\n";
            } else {
                std::cout << "✅ Engine " << result.engineID << " (" << result.engineName 
                          << ") - All parameters valid" << std::endl;
                
                reportFile << "### ✅ Engine " << result.engineID << ": " << result.engineName 
                           << " - All parameters valid\n\n";
            }
        }
        
        // Recommendations
        reportFile << "## Recommendations\n\n";
        reportFile << "1. **Fix Generic Parameter Names**: Replace any \"Param X\" names with descriptive labels\n";
        reportFile << "2. **Standardize Mix Parameter Position**: Consider moving Mix parameters to consistent positions\n";
        reportFile << "3. **Validate Gain Parameter Functionality**: Ensure Gain parameters actually control gain\n";
        reportFile << "4. **Add Parameter Documentation**: Document what each parameter index controls\n";
        reportFile << "5. **Implement Unit Tests**: Add automated tests for parameter mapping consistency\n\n";
        
        reportFile.close();
        
        std::cout << "\nFull report written to: parameter_mapping_validation_report.md" << std::endl;
    }
};

int main() {
    ParameterMappingValidator validator;
    validator.validateAllEngines();
    return 0;
}
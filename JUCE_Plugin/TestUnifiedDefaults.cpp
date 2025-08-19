#include "Source/UnifiedDefaultParameters.h"
#include "Source/EngineTypes.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>

/**
 * Comprehensive test program for the Unified Default Parameter System
 * 
 * This program validates:
 * 1. Complete coverage of all 57 engines
 * 2. Parameter value safety (0.0-1.0 range)
 * 3. Musical utility guidelines compliance
 * 4. Category organization consistency
 * 5. Mix parameter identification accuracy
 */

struct TestResults {
    int totalEngines = 0;
    int enginesWithDefaults = 0;
    int totalParameters = 0;
    int validationErrors = 0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

void printHeader(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void printEngineCategory(UnifiedDefaultParameters::EngineCategory category) {
    switch (category) {
        case UnifiedDefaultParameters::EngineCategory::DISTORTION: std::cout << "Distortion"; break;
        case UnifiedDefaultParameters::EngineCategory::SATURATION: std::cout << "Saturation"; break;
        case UnifiedDefaultParameters::EngineCategory::REVERB: std::cout << "Reverb"; break;
        case UnifiedDefaultParameters::EngineCategory::DELAY: std::cout << "Delay"; break;
        case UnifiedDefaultParameters::EngineCategory::MODULATION: std::cout << "Modulation"; break;
        case UnifiedDefaultParameters::EngineCategory::FILTER: std::cout << "Filter"; break;
        case UnifiedDefaultParameters::EngineCategory::DYNAMICS: std::cout << "Dynamics"; break;
        case UnifiedDefaultParameters::EngineCategory::SPATIAL: std::cout << "Spatial"; break;
        case UnifiedDefaultParameters::EngineCategory::PITCH: std::cout << "Pitch"; break;
        case UnifiedDefaultParameters::EngineCategory::UTILITY: std::cout << "Utility"; break;
        case UnifiedDefaultParameters::EngineCategory::SPECTRAL: std::cout << "Spectral"; break;
        case UnifiedDefaultParameters::EngineCategory::EXPERIMENTAL: std::cout << "Experimental"; break;
        default: std::cout << "Unknown"; break;
    }
}

TestResults testEngineDefaults() {
    printHeader("ENGINE DEFAULTS VALIDATION");
    
    TestResults results;
    
    // Test all engines from 0 to ENGINE_COUNT-1
    for (int engineId = ENGINE_NONE; engineId < ENGINE_COUNT; ++engineId) {
        results.totalEngines++;
        
        auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
        
        if (!defaults.empty() || engineId == ENGINE_NONE) {
            results.enginesWithDefaults++;
            results.totalParameters += defaults.size();
            
            std::cout << std::setw(3) << engineId << " | " 
                      << std::setw(25) << getEngineTypeName(engineId) << " | "
                      << std::setw(2) << defaults.size() << " params";
            
            // Validate parameter values
            bool hasErrors = false;
            for (const auto& [index, value] : defaults) {
                if (value < 0.0f || value > 1.0f) {
                    std::string error = "Engine " + std::to_string(engineId) + " parameter " + 
                                      std::to_string(index) + " out of range: " + std::to_string(value);
                    results.errors.push_back(error);
                    results.validationErrors++;
                    hasErrors = true;
                }
            }
            
            if (hasErrors) {
                std::cout << " | ❌ RANGE ERRORS";
            } else {
                std::cout << " | ✅ Valid";
            }
            
            // Get mix parameter index
            int mixIndex = UnifiedDefaultParameters::getMixParameterIndex(engineId);
            if (mixIndex >= 0) {
                std::cout << " | Mix: param " << (mixIndex + 1);
                // Check if mix parameter has a default
                if (defaults.find(mixIndex) != defaults.end()) {
                    std::cout << " (" << defaults[mixIndex] << ")";
                } else {
                    std::cout << " (MISSING!)";
                    results.warnings.push_back("Engine " + std::to_string(engineId) + 
                                               " has mix parameter but no default");
                }
            } else {
                std::cout << " | No mix";
            }
            
            std::cout << std::endl;
            
        } else {
            std::cout << std::setw(3) << engineId << " | " 
                      << std::setw(25) << getEngineTypeName(engineId) << " | "
                      << "NO DEFAULTS ❌" << std::endl;
            results.warnings.push_back("Engine " + std::to_string(engineId) + " has no defaults");
        }
    }
    
    return results;
}

void testCategoryOrganization() {
    printHeader("CATEGORY ORGANIZATION");
    
    auto categories = UnifiedDefaultParameters::getEnginesByCategory();
    
    for (const auto& [category, engines] : categories) {
        std::cout << "\n";
        printEngineCategory(category);
        std::cout << " (" << engines.size() << " engines):" << std::endl;
        
        for (int engineId : engines) {
            auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
            std::cout << "  " << std::setw(3) << engineId << " | " 
                      << std::setw(25) << getEngineTypeName(engineId) << " | "
                      << defaults.size() << " params" << std::endl;
        }
        
        // Show category guidelines
        std::cout << "  Guidelines: " << UnifiedDefaultParameters::getCategoryGuidelines(category) << std::endl;
    }
}

void testParameterRangeDistribution() {
    printHeader("PARAMETER VALUE DISTRIBUTION ANALYSIS");
    
    std::map<std::string, int> rangeDistribution;
    rangeDistribution["0.0-0.2"] = 0;
    rangeDistribution["0.2-0.4"] = 0;
    rangeDistribution["0.4-0.6"] = 0;
    rangeDistribution["0.6-0.8"] = 0;
    rangeDistribution["0.8-1.0"] = 0;
    
    int totalParams = 0;
    
    for (int engineId = ENGINE_NONE; engineId < ENGINE_COUNT; ++engineId) {
        auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
        
        for (const auto& [index, value] : defaults) {
            totalParams++;
            
            if (value >= 0.0f && value < 0.2f) rangeDistribution["0.0-0.2"]++;
            else if (value >= 0.2f && value < 0.4f) rangeDistribution["0.2-0.4"]++;
            else if (value >= 0.4f && value < 0.6f) rangeDistribution["0.4-0.6"]++;
            else if (value >= 0.6f && value < 0.8f) rangeDistribution["0.6-0.8"]++;
            else if (value >= 0.8f && value <= 1.0f) rangeDistribution["0.8-1.0"]++;
        }
    }
    
    std::cout << "Parameter value distribution across all engines:" << std::endl;
    for (const auto& [range, count] : rangeDistribution) {
        float percentage = (100.0f * count) / totalParams;
        std::cout << range << ": " << std::setw(4) << count << " (" 
                  << std::fixed << std::setprecision(1) << percentage << "%)" << std::endl;
    }
    
    std::cout << "\nTotal parameters analyzed: " << totalParams << std::endl;
    
    // Analyze methodology compliance
    std::cout << "\nMethodology Compliance:" << std::endl;
    float moderateRange = rangeDistribution["0.2-0.4"] + rangeDistribution["0.4-0.6"] + rangeDistribution["0.6-0.8"];
    float moderatePercentage = (100.0f * moderateRange) / totalParams;
    std::cout << "Moderate values (0.2-0.8): " << std::fixed << std::setprecision(1) 
              << moderatePercentage << "% (Target: >60%)" << std::endl;
    
    if (moderatePercentage >= 60.0f) {
        std::cout << "✅ Methodology compliance: PASS" << std::endl;
    } else {
        std::cout << "❌ Methodology compliance: FAIL (too many extreme values)" << std::endl;
    }
}

void testSpecificEngineExamples() {
    printHeader("SPECIFIC ENGINE EXAMPLES");
    
    // Test key representative engines from each category
    std::vector<int> testEngines = {
        ENGINE_K_STYLE,           // Distortion
        ENGINE_VINTAGE_TUBE,      // Saturation  
        ENGINE_PLATE_REVERB,      // Reverb
        ENGINE_TAPE_ECHO,         // Delay
        ENGINE_DIGITAL_CHORUS,    // Modulation
        ENGINE_LADDER_FILTER,     // Filter
        ENGINE_VCA_COMPRESSOR,    // Dynamics
        ENGINE_STEREO_WIDENER,    // Spatial
        ENGINE_PITCH_SHIFTER,     // Pitch
        ENGINE_GAIN_UTILITY,      // Utility
        ENGINE_SPECTRAL_FREEZE,   // Spectral
        ENGINE_CHAOS_GENERATOR    // Experimental
    };
    
    for (int engineId : testEngines) {
        auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
        auto engineInfo = UnifiedDefaultParameters::getEngineDefaults(engineId);
        
        std::cout << "\n" << getEngineTypeName(engineId) << " (Engine " << engineId << "):" << std::endl;
        std::cout << "Category: ";
        printEngineCategory(engineInfo.category);
        std::cout << std::endl;
        
        // Show first few parameters
        int paramCount = 0;
        for (const auto& [index, value] : defaults) {
            if (paramCount >= 5) break; // Show first 5 parameters
            std::cout << "  Param " << (index + 1) << ": " << std::fixed << std::setprecision(3) << value;
            
            // Add parameter name if available
            std::string paramName = UnifiedDefaultParameters::getParameterName(engineId, index);
            if (paramName != "Parameter " + std::to_string(index + 1)) {
                std::cout << " (" << paramName << ")";
            }
            std::cout << std::endl;
            paramCount++;
        }
        
        if (defaults.size() > 5) {
            std::cout << "  ... and " << (defaults.size() - 5) << " more parameters" << std::endl;
        }
    }
}

void testValidationSystem() {
    printHeader("VALIDATION SYSTEM TEST");
    
    int passedValidation = 0;
    int failedValidation = 0;
    
    for (int engineId = ENGINE_NONE; engineId < ENGINE_COUNT; ++engineId) {
        if (UnifiedDefaultParameters::validateEngineDefaults(engineId)) {
            passedValidation++;
        } else {
            failedValidation++;
            std::cout << "❌ Engine " << engineId << " (" << getEngineTypeName(engineId) 
                      << ") failed validation" << std::endl;
        }
    }
    
    std::cout << "Validation Results:" << std::endl;
    std::cout << "✅ Passed: " << passedValidation << std::endl;
    std::cout << "❌ Failed: " << failedValidation << std::endl;
    
    if (failedValidation == 0) {
        std::cout << "🎉 All engines passed validation!" << std::endl;
    }
}

int main() {
    std::cout << "Unified Default Parameter System - Comprehensive Test Suite" << std::endl;
    std::cout << "Version: 1.0" << std::endl;
    std::cout << "Engines to test: " << ENGINE_COUNT << " (including ENGINE_NONE)" << std::endl;
    
    // Run all tests
    TestResults results = testEngineDefaults();
    testCategoryOrganization();
    testParameterRangeDistribution();
    testSpecificEngineExamples();
    testValidationSystem();
    
    // Final summary
    printHeader("FINAL SUMMARY");
    
    std::cout << "Total engines: " << results.totalEngines << std::endl;
    std::cout << "Engines with defaults: " << results.enginesWithDefaults << std::endl;
    float coverage = (100.0f * results.enginesWithDefaults) / results.totalEngines;
    std::cout << "Coverage: " << std::fixed << std::setprecision(1) << coverage << "%" << std::endl;
    std::cout << "Total parameters: " << results.totalParameters << std::endl;
    std::cout << "Validation errors: " << results.validationErrors << std::endl;
    std::cout << "Warnings: " << results.warnings.size() << std::endl;
    
    // Print warnings
    if (!results.warnings.empty()) {
        std::cout << "\nWarnings:" << std::endl;
        for (const auto& warning : results.warnings) {
            std::cout << "⚠️  " << warning << std::endl;
        }
    }
    
    // Print errors
    if (!results.errors.empty()) {
        std::cout << "\nErrors:" << std::endl;
        for (const auto& error : results.errors) {
            std::cout << "❌ " << error << std::endl;
        }
    }
    
    // Overall result
    std::cout << "\n" << std::string(60, '=') << std::endl;
    if (coverage >= 100.0f && results.validationErrors == 0) {
        std::cout << "🎉 TEST SUITE PASSED! Unified Default Parameters system is ready for integration." << std::endl;
    } else {
        std::cout << "❌ TEST SUITE FAILED! Issues must be resolved before integration." << std::endl;
    }
    std::cout << std::string(60, '=') << std::endl;
    
    return (coverage >= 100.0f && results.validationErrors == 0) ? 0 : 1;
}
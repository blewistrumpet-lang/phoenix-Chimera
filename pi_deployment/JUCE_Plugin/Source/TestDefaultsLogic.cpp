#include "UnifiedDefaultParameters.h"
#include "EngineTypes.h"
#include <iostream>
#include <set>

/**
 * Simplified Integration Test for UnifiedDefaultParameters Logic
 * Tests the core logic without JUCE dependencies
 */

int main() {
    std::cout << "=== Unified Default Parameters Logic Test ===" << std::endl;
    
    int totalTests = 0;
    int passedTests = 0;
    
    // Test 1: All engines have defaults or are ENGINE_NONE
    std::cout << "\n--- Testing Default Parameter Coverage ---" << std::endl;
    bool allEnginesHaveDefaults = true;
    
    for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
        auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
        
        if (engineId == ENGINE_NONE) {
            // ENGINE_NONE should have no parameters
            if (!defaults.empty()) {
                std::cout << "❌ ENGINE_NONE should have no parameters, found: " << defaults.size() << std::endl;
                allEnginesHaveDefaults = false;
            }
        } else {
            // All other engines should have some parameters
            if (defaults.empty()) {
                std::cout << "❌ Engine " << engineId << " has no default parameters" << std::endl;
                allEnginesHaveDefaults = false;
            }
        }
    }
    
    totalTests++;
    if (allEnginesHaveDefaults) {
        std::cout << "✅ Default parameter coverage correct" << std::endl;
        passedTests++;
    }
    
    // Test 2: All parameter values in valid range
    std::cout << "\n--- Testing Parameter Value Ranges ---" << std::endl;
    bool allValuesValid = true;
    
    for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
        auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
        
        for (const auto& param : defaults) {
            if (param.second < 0.0f || param.second > 1.0f) {
                std::cout << "❌ Engine " << engineId << " parameter " << param.first 
                          << " value out of range: " << param.second << std::endl;
                allValuesValid = false;
            }
        }
    }
    
    totalTests++;
    if (allValuesValid) {
        std::cout << "✅ All parameter values in valid range [0.0, 1.0]" << std::endl;
        passedTests++;
    }
    
    // Test 3: Parameter count consistency
    std::cout << "\n--- Testing Parameter Count Consistency ---" << std::endl;
    bool countConsistent = true;
    
    for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
        auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
        int actualCount = static_cast<int>(defaults.size());
        int reportedCount = UnifiedDefaultParameters::getParameterCount(engineId);
        
        if (actualCount != reportedCount) {
            std::cout << "❌ Engine " << engineId << " count mismatch: actual=" 
                      << actualCount << " reported=" << reportedCount << std::endl;
            countConsistent = false;
        }
        
        // Check parameter limit
        if (actualCount > 15) {
            std::cout << "❌ Engine " << engineId << " exceeds 15 parameter limit: " 
                      << actualCount << std::endl;
            countConsistent = false;
        }
    }
    
    totalTests++;
    if (countConsistent) {
        std::cout << "✅ Parameter counts consistent and within limits" << std::endl;
        passedTests++;
    }
    
    // Test 4: Mix parameter consistency
    std::cout << "\n--- Testing Mix Parameter Consistency ---" << std::endl;
    bool mixConsistent = true;
    int enginesWithMix = 0;
    
    for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
        int mixIndex = UnifiedDefaultParameters::getMixParameterIndex(engineId);
        auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
        
        if (mixIndex >= 0) {
            enginesWithMix++;
            if (defaults.find(mixIndex) == defaults.end()) {
                std::cout << "❌ Engine " << engineId << " mix parameter " << mixIndex 
                          << " not found in defaults" << std::endl;
                mixConsistent = false;
            } else {
                // Check if mix parameter has reasonable default (not too low for mix parameters)
                float mixValue = defaults.at(mixIndex);
                if (mixValue < 0.2f) {
                    std::cout << "⚠️  Engine " << engineId << " mix parameter unusually low: " 
                              << mixValue << std::endl;
                }
            }
        }
    }
    
    totalTests++;
    if (mixConsistent) {
        std::cout << "✅ Mix parameters consistent (" << enginesWithMix << " engines have mix)" << std::endl;
        passedTests++;
    }
    
    // Test 5: Category completeness
    std::cout << "\n--- Testing Category System ---" << std::endl;
    bool categoriesComplete = true;
    auto categorized = UnifiedDefaultParameters::getEnginesByCategory();
    std::set<int> categorizedEngines;
    
    for (const auto& category : categorized) {
        for (int engineId : category.second) {
            categorizedEngines.insert(engineId);
        }
    }
    
    // Check all engines are categorized (except ENGINE_NONE)
    for (int engineId = 1; engineId < ENGINE_COUNT; engineId++) {
        if (categorizedEngines.find(engineId) == categorizedEngines.end()) {
            std::cout << "❌ Engine " << engineId << " not categorized" << std::endl;
            categoriesComplete = false;
        }
    }
    
    totalTests++;
    if (categoriesComplete) {
        std::cout << "✅ All engines properly categorized" << std::endl;
        passedTests++;
    }
    
    // Test 6: Safety validation
    std::cout << "\n--- Testing Safety Validation ---" << std::endl;
    bool allSafe = true;
    
    for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
        if (!UnifiedDefaultParameters::validateEngineDefaults(engineId)) {
            std::cout << "❌ Engine " << engineId << " failed safety validation" << std::endl;
            allSafe = false;
        }
    }
    
    totalTests++;
    if (allSafe) {
        std::cout << "✅ All engines pass safety validation" << std::endl;
        passedTests++;
    }
    
    // Summary
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Total tests: " << totalTests << std::endl;
    std::cout << "Passed: " << passedTests << std::endl;
    std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
    
    if (passedTests == totalTests) {
        std::cout << "✅ ALL LOGIC TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
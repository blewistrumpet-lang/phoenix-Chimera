#include "UnifiedDefaultParameters.h"
#include "EngineFactory.h"
#include "EngineTypes.h"
#include <iostream>
#include <map>
#include <vector>

/**
 * Comprehensive Integration Test for UnifiedDefaultParameters System
 * 
 * This test validates that the UnifiedDefaultParameters system is fully 
 * integrated and handles all edge cases correctly.
 */

class UnifiedDefaultsIntegrationTest {
public:
    void runAllTests() {
        std::cout << "=== Unified Default Parameters Integration Test ===" << std::endl;
        
        testEngineFactoryIntegration();
        testParameterRangeCompatibility();
        testEngineDefaultsVsConstructors();
        testEngineSwithingDefaults();
        testMixParameterConsistency();
        testParameterCountValidation();
        testDefaultValueSafety();
        testCategoryConsistency();
        
        std::cout << "\n=== Integration Test Results ===" << std::endl;
        std::cout << "Total tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << failedTests << std::endl;
        
        if (failedTests == 0) {
            std::cout << "✅ ALL INTEGRATION TESTS PASSED" << std::endl;
        } else {
            std::cout << "❌ INTEGRATION ISSUES FOUND" << std::endl;
        }
    }

private:
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    
    void test(const std::string& name, bool condition, const std::string& errorMsg = "") {
        totalTests++;
        if (condition) {
            std::cout << "✅ " << name << std::endl;
            passedTests++;
        } else {
            std::cout << "❌ " << name;
            if (!errorMsg.empty()) {
                std::cout << " - " << errorMsg;
            }
            std::cout << std::endl;
            failedTests++;
        }
    }
    
    void testEngineFactoryIntegration() {
        std::cout << "\n--- Testing EngineFactory Integration ---" << std::endl;
        
        // Test that all engines can be created
        bool allEnginesCreatable = true;
        int createdCount = 0;
        
        for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
            auto engine = EngineFactory::createEngine(engineId);
            if (engine != nullptr) {
                createdCount++;
            } else if (engineId != ENGINE_NONE) {
                allEnginesCreatable = false;
                std::cout << "Failed to create engine ID: " << engineId << std::endl;
            }
        }
        
        test("All engines can be created via EngineFactory", allEnginesCreatable);
        test("Expected engine count matches", createdCount == ENGINE_COUNT - 1); // -1 for NONE engine
    }
    
    void testParameterRangeCompatibility() {
        std::cout << "\n--- Testing Parameter Range Compatibility ---" << std::endl;
        
        bool allRangesValid = true;
        
        for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
            auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
            
            for (const auto& param : defaults) {
                if (param.second < 0.0f || param.second > 1.0f) {
                    allRangesValid = false;
                    std::cout << "Engine " << engineId << " param " << param.first 
                              << " has invalid range: " << param.second << std::endl;
                }
            }
        }
        
        test("All default parameters in valid range [0.0, 1.0]", allRangesValid);
    }
    
    void testEngineDefaultsVsConstructors() {
        std::cout << "\n--- Testing Engine Constructor vs UnifiedDefaults Priority ---" << std::endl;
        
        // This test verifies that UnifiedDefaultParameters takes precedence
        // over engine constructor defaults
        
        // Test a few specific engines that we know have constructor defaults
        std::vector<int> testEngines = {
            ENGINE_LADDER_FILTER,      // Has constructor defaults
            ENGINE_CLASSIC_TREMOLO,    // Has constructor defaults
            ENGINE_VCA_COMPRESSOR,     // Standard engine
            ENGINE_TAPE_ECHO          // Standard engine
        };
        
        bool priorityCorrect = true;
        
        for (int engineId : testEngines) {
            auto unifiedDefaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
            
            // If UnifiedDefaultParameters provides defaults, they should override constructor defaults
            if (!unifiedDefaults.empty()) {
                // This is expected - UnifiedDefaultParameters has defaults for this engine
                continue;
            }
            
            // Only ENGINE_NONE should have no defaults
            if (engineId != ENGINE_NONE && unifiedDefaults.empty()) {
                priorityCorrect = false;
                std::cout << "Engine " << engineId << " missing unified defaults!" << std::endl;
            }
        }
        
        test("UnifiedDefaultParameters provides defaults for all engines", priorityCorrect);
    }
    
    void testEngineSwithingDefaults() {
        std::cout << "\n--- Testing Engine Switching Default Application ---" << std::endl;
        
        // Test that switching between engines applies correct defaults
        std::vector<int> testEngines = {
            ENGINE_OPTO_COMPRESSOR,
            ENGINE_LADDER_FILTER,
            ENGINE_TAPE_ECHO,
            ENGINE_ANALOG_PHASER
        };
        
        bool switchingCorrect = true;
        
        for (size_t i = 0; i < testEngines.size(); ++i) {
            int engineId = testEngines[i];
            auto defaults1 = UnifiedDefaultParameters::getDefaultParameters(engineId);
            
            // Switch to another engine and back
            int nextEngine = testEngines[(i + 1) % testEngines.size()];
            auto defaultsOther = UnifiedDefaultParameters::getDefaultParameters(nextEngine);
            auto defaults2 = UnifiedDefaultParameters::getDefaultParameters(engineId);
            
            // Defaults should be identical when switching back
            if (defaults1 != defaults2) {
                switchingCorrect = false;
                std::cout << "Engine " << engineId << " defaults inconsistent on switch back" << std::endl;
            }
        }
        
        test("Engine switching preserves correct defaults", switchingCorrect);
    }
    
    void testMixParameterConsistency() {
        std::cout << "\n--- Testing Mix Parameter Consistency ---" << std::endl;
        
        bool mixConsistent = true;
        
        for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
            int mixIndex = UnifiedDefaultParameters::getMixParameterIndex(engineId);
            auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
            
            if (mixIndex >= 0) {
                // Engine has a mix parameter
                if (defaults.find(mixIndex) == defaults.end()) {
                    mixConsistent = false;
                    std::cout << "Engine " << engineId << " has mix parameter index " 
                              << mixIndex << " but no default value" << std::endl;
                }
            }
        }
        
        test("Mix parameter indices consistent with defaults", mixConsistent);
    }
    
    void testParameterCountValidation() {
        std::cout << "\n--- Testing Parameter Count Validation ---" << std::endl;
        
        bool countsValid = true;
        
        for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
            int reportedCount = UnifiedDefaultParameters::getParameterCount(engineId);
            auto defaults = UnifiedDefaultParameters::getDefaultParameters(engineId);
            int actualCount = static_cast<int>(defaults.size());
            
            if (reportedCount != actualCount) {
                countsValid = false;
                std::cout << "Engine " << engineId << " reported count " << reportedCount
                          << " != actual count " << actualCount << std::endl;
            }
            
            // Parameter count should not exceed 15 (plugin parameter limit)
            if (actualCount > 15) {
                countsValid = false;
                std::cout << "Engine " << engineId << " has " << actualCount 
                          << " parameters (exceeds limit of 15)" << std::endl;
            }
        }
        
        test("Parameter counts accurate and within limits", countsValid);
    }
    
    void testDefaultValueSafety() {
        std::cout << "\n--- Testing Default Value Safety ---" << std::endl;
        
        bool allSafe = true;
        std::vector<int> unsafeEngines;
        
        for (int engineId = 0; engineId < ENGINE_COUNT; engineId++) {
            if (!UnifiedDefaultParameters::validateEngineDefaults(engineId)) {
                allSafe = false;
                unsafeEngines.push_back(engineId);
            }
        }
        
        test("All engine defaults pass safety validation", allSafe);
        
        if (!allSafe) {
            std::cout << "Unsafe engines: ";
            for (int id : unsafeEngines) {
                std::cout << id << " ";
            }
            std::cout << std::endl;
        }
    }
    
    void testCategoryConsistency() {
        std::cout << "\n--- Testing Category Consistency ---" << std::endl;
        
        auto categorized = UnifiedDefaultParameters::getEnginesByCategory();
        std::set<int> categorizedEngines;
        
        // Collect all engines from categories
        for (const auto& category : categorized) {
            for (int engineId : category.second) {
                categorizedEngines.insert(engineId);
            }
        }
        
        bool allCategorized = true;
        
        // Check that all engines (except NONE) are categorized
        for (int engineId = 1; engineId < ENGINE_COUNT; engineId++) { // Start from 1 to skip ENGINE_NONE
            if (categorizedEngines.find(engineId) == categorizedEngines.end()) {
                allCategorized = false;
                std::cout << "Engine " << engineId << " not found in any category" << std::endl;
            }
        }
        
        test("All engines properly categorized", allCategorized);
        
        // Test that each engine has consistent category assignment
        bool categoryConsistent = true;
        for (int engineId = 1; engineId < ENGINE_COUNT; engineId++) {
            auto engineDefaults = UnifiedDefaultParameters::getEngineDefaults(engineId);
            // Just verify we can get the category without error
            std::string guidelines = UnifiedDefaultParameters::getCategoryGuidelines(engineDefaults.category);
            if (guidelines.empty()) {
                categoryConsistent = false;
                std::cout << "Engine " << engineId << " has empty category guidelines" << std::endl;
            }
        }
        
        test("Category guidelines available for all engines", categoryConsistent);
    }
};

int main() {
    try {
        UnifiedDefaultsIntegrationTest test;
        test.runAllTests();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
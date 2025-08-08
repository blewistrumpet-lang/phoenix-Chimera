#include "SimplifiedEngineTestHarness.h"
#include <iostream>

/**
 * RunSimplifiedEngineTests.cpp
 * 
 * Simple runner for the Simplified Engine Test Harness.
 * Compile and run this to test all implemented engines.
 * 
 * Usage:
 * 1. Compile with your JUCE project
 * 2. Run the executable
 * 3. Check the generated report at /tmp/simplified_engine_test_report.txt
 */

int main(int argc, char* argv[]) {
    std::cout << "=== Project Chimera - Simplified Engine Test Harness ===" << std::endl;
    std::cout << "Testing all implemented engines for safety and quality..." << std::endl;
    std::cout << std::endl;
    
    try {
        // Create test harness with default configuration
        SimplifiedEngineTestHarness::TestConfig config;
        
        // Override report path if provided as command line argument
        if (argc > 1) {
            config.reportPath = argv[1];
            std::cout << "Using custom report path: " << config.reportPath << std::endl;
        }
        
        // Create and run the test harness
        SimplifiedEngineTestHarness harness(config);
        harness.runAllTests();
        
        // Get results summary
        const auto& results = harness.getResults();
        
        int criticalIssues = 0;
        int warnings = 0;
        
        // Count critical issues
        for (const auto& result : results) {
            if (!result.creationSuccess || !result.nanInfHandling) {
                criticalIssues++;
            } else if (!result.parameterFunctionality || !result.audioQuality || !result.threadSafety) {
                warnings++;
            }
        }
        
        std::cout << std::endl << "=== FINAL ANALYSIS ===" << std::endl;
        std::cout << "Critical Issues (creation failures, NaN/Inf problems): " << criticalIssues << std::endl;
        std::cout << "Warnings (parameter, audio quality, thread safety issues): " << warnings << std::endl;
        
        if (criticalIssues == 0 && warnings == 0) {
            std::cout << "🎉 All engines passed basic safety and quality tests!" << std::endl;
            return 0;
        } else if (criticalIssues == 0) {
            std::cout << "⚠️  All engines are safe, but some have quality issues." << std::endl;
            return 1;
        } else {
            std::cout << "❌ Some engines have critical safety issues that need immediate attention." << std::endl;
            return 2;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error running tests: " << e.what() << std::endl;
        return 3;
    } catch (...) {
        std::cerr << "Unknown error running tests" << std::endl;
        return 4;
    }
}

// Alternative entry points for integration with existing test systems

namespace SimplifiedEngineTests {
    
    /**
     * Run tests and return simple pass/fail result
     */
    bool runQuickSafetyCheck() {
        try {
            SimplifiedEngineTestHarness::TestConfig config;
            config.threadTestIterations = 10;  // Faster for quick check
            config.testBufferSize = 256;       // Smaller buffers for speed
            config.reportPath = "/tmp/quick_engine_safety_check.txt";
            
            SimplifiedEngineTestHarness harness(config);
            harness.runAllTests();
            
            const auto& results = harness.getResults();
            
            // Only critical safety checks for quick mode
            for (const auto& result : results) {
                if (!result.creationSuccess || !result.nanInfHandling) {
                    return false;
                }
            }
            
            return true;
            
        } catch (...) {
            return false;
        }
    }
    
    /**
     * Test specific engine by ID
     */
    bool testSpecificEngine(int engineID) {
        try {
            SimplifiedEngineTestHarness harness;
            auto result = harness.testEngine(engineID);
            
            std::cout << "Engine " << engineID << " (" << result.engineName << "): ";
            if (result.allTestsPassed()) {
                std::cout << "PASS" << std::endl;
                return true;
            } else {
                std::cout << "FAIL - Issues:" << std::endl;
                for (const auto& issue : result.issues) {
                    std::cout << "  - " << issue << std::endl;
                }
                return false;
            }
            
        } catch (const std::exception& e) {
            std::cout << "Engine " << engineID << ": ERROR - " << e.what() << std::endl;
            return false;
        } catch (...) {
            std::cout << "Engine " << engineID << ": UNKNOWN ERROR" << std::endl;
            return false;
        }
    }
    
    /**
     * Get list of engines that have critical issues
     */
    std::vector<int> getCriticallyFailedEngines() {
        std::vector<int> failedEngines;
        
        try {
            SimplifiedEngineTestHarness harness;
            harness.runAllTests();
            
            const auto& results = harness.getResults();
            
            for (const auto& result : results) {
                if (!result.creationSuccess || !result.nanInfHandling) {
                    failedEngines.push_back(result.engineID);
                }
            }
            
        } catch (...) {
            // Return all engines as potentially failed if we can't test
            failedEngines = SimplifiedEngineTestHarness::IMPLEMENTED_ENGINES;
        }
        
        return failedEngines;
    }
}
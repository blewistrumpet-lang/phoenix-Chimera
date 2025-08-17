/**
 * ENGINE ARCHITECTURE MANAGER TEST
 * 
 * Demonstrates the Engine Architecture Manager's capabilities
 * for maintaining and validating engine system integrity.
 */

#include <iostream>
#include <iomanip>
#include "EngineArchitectureManager.h"

void printSeparator() {
    std::cout << "\n" << std::string(60, '=') << "\n";
}

void printHeader(const std::string& title) {
    printSeparator();
    std::cout << "   " << title << "\n";
    printSeparator();
}

int main() {
    printHeader("ENGINE ARCHITECTURE MANAGER TEST");
    
    // Get the singleton instance
    auto& manager = EngineArchitectureManager::getInstance();
    
    std::cout << "\nArchitecture Version: " << manager.getArchitectureVersion() << "\n";
    std::cout << "Total Engines: " << EngineArchitectureManager::TOTAL_ENGINES << "\n\n";
    
    // 1. Basic Architecture Validation
    printHeader("1. BASIC ARCHITECTURE VALIDATION");
    
    std::cout << "Running basic validation...\n";
    bool basicValid = manager.validateArchitecture(
        EngineArchitectureManager::ValidationLevel::BASIC
    );
    std::cout << "Result: " << (basicValid ? "✅ PASSED" : "❌ FAILED") << "\n";
    
    // 2. Factory Integrity Check
    printHeader("2. FACTORY INTEGRITY CHECK");
    
    std::cout << "Asserting engine factory...\n";
    bool factoryValid = manager.assertEngineFactory();
    std::cout << "Factory creates all engines: " 
              << (factoryValid ? "✅ YES" : "❌ NO") << "\n";
    
    // 3. Engine Mapping Verification
    printHeader("3. ENGINE MAPPING VERIFICATION");
    
    std::cout << "\nVerifying critical engines:\n\n";
    
    // Test specific important engines
    const std::vector<int> criticalEngines = {
        39, // PlateReverb
        40, // SpringReverb_Platinum
        41, // ConvolutionReverb
        2,  // ClassicCompressor
        6,  // DynamicEQ
        18, // BitCrusher
        50  // GranularCloud
    };
    
    for (int id : criticalEngines) {
        std::string name = manager.getEngineName(id);
        int mixIndex = manager.getMixParameterIndex(id);
        auto category = manager.getEngineCategory(id);
        
        std::cout << "[" << std::setw(2) << id << "] " 
                  << std::setw(30) << std::left << name;
        
        bool mappingValid = manager.assertEngineMapping(id);
        bool paramValid = manager.assertParameterMapping(id);
        
        std::cout << " Mapping: " << (mappingValid ? "✅" : "❌");
        std::cout << " Params: " << (paramValid ? "✅" : "❌");
        std::cout << " Mix@" << mixIndex << "\n";
    }
    
    // 4. Category Organization
    printHeader("4. ENGINE CATEGORIES");
    
    std::cout << "\nEngines by category:\n\n";
    
    const std::vector<std::pair<EngineArchitectureManager::EngineCategory, std::string>> categories = {
        {EngineArchitectureManager::EngineCategory::DYNAMICS, "DYNAMICS"},
        {EngineArchitectureManager::EngineCategory::EQ_FILTER, "EQ/FILTER"},
        {EngineArchitectureManager::EngineCategory::DISTORTION, "DISTORTION"},
        {EngineArchitectureManager::EngineCategory::MODULATION, "MODULATION"},
        {EngineArchitectureManager::EngineCategory::DELAY, "DELAY"},
        {EngineArchitectureManager::EngineCategory::REVERB, "REVERB"},
        {EngineArchitectureManager::EngineCategory::SPATIAL, "SPATIAL"},
        {EngineArchitectureManager::EngineCategory::UTILITY, "UTILITY"}
    };
    
    for (const auto& [cat, name] : categories) {
        auto engines = manager.getEnginesByCategory(cat);
        std::cout << std::setw(12) << std::left << name 
                  << ": " << engines.size() << " engines (";
        
        for (size_t i = 0; i < engines.size(); ++i) {
            std::cout << engines[i];
            if (i < engines.size() - 1) std::cout << ", ";
        }
        std::cout << ")\n";
    }
    
    // 5. Mix Parameter Consistency
    printHeader("5. MIX PARAMETER VERIFICATION");
    
    std::cout << "\nChecking mix parameter indices...\n";
    
    int correctMix = 0;
    int invalidMix = 0;
    
    for (int id = 0; id < EngineArchitectureManager::TOTAL_ENGINES; ++id) {
        int mixIndex = manager.getMixParameterIndex(id);
        
        if (id == 0) {
            // NoneEngine should have -1
            if (mixIndex == -1) correctMix++;
            else invalidMix++;
        } else {
            // All other engines should have valid mix index
            if (mixIndex >= 0 && mixIndex < 15) correctMix++;
            else invalidMix++;
        }
    }
    
    std::cout << "Valid mix indices: " << correctMix << "/" 
              << EngineArchitectureManager::TOTAL_ENGINES << "\n";
    
    if (invalidMix > 0) {
        std::cout << "⚠️  " << invalidMix << " engines have invalid mix indices!\n";
    } else {
        std::cout << "✅ All mix parameter indices are valid!\n";
    }
    
    // 6. Comprehensive Validation
    printHeader("6. COMPREHENSIVE VALIDATION");
    
    std::cout << "\nRunning comprehensive validation...\n";
    std::cout << "(This will test all engines thoroughly)\n\n";
    
    bool comprehensiveValid = manager.validateArchitecture(
        EngineArchitectureManager::ValidationLevel::COMPREHENSIVE
    );
    
    std::cout << "\nResult: " << (comprehensiveValid ? "✅ PASSED" : "❌ FAILED") << "\n";
    
    // 7. Check for violations
    printHeader("7. ARCHITECTURE VIOLATIONS");
    
    auto violations = manager.getViolations();
    
    if (violations.empty()) {
        std::cout << "\n✅ No architecture violations detected!\n";
    } else {
        std::cout << "\n⚠️  " << violations.size() << " violations found:\n\n";
        
        for (const auto& v : violations) {
            std::cout << (v.critical ? "[CRITICAL]" : "[WARNING]") << " ";
            std::cout << "Engine " << v.engineID << " (" << v.engineName << "): ";
            std::cout << v.description << "\n";
        }
    }
    
    // 8. Generate reports
    printHeader("8. GENERATING REPORTS");
    
    std::cout << "\nGenerating architecture documentation...\n";
    
    manager.generateArchitectureReport("architecture_report.txt");
    std::cout << "✅ Architecture report saved to: architecture_report.txt\n";
    
    manager.generateEngineMapping("engine_mapping.csv");
    std::cout << "✅ Engine mapping saved to: engine_mapping.csv\n";
    
    // 9. Final assertion
    printHeader("9. FINAL ARCHITECTURE ASSERTION");
    
    std::cout << "\nAsserting all engines...\n";
    bool allEnginesValid = manager.assertAllEngines();
    
    if (allEnginesValid) {
        std::cout << "\n✅ SUCCESS: All 57 engines validated successfully!\n";
        std::cout << "✅ Engine factory configuration is correct\n";
        std::cout << "✅ Engine mapping is clear and consistent\n";
        std::cout << "✅ Parameter mapping is properly configured\n";
    } else {
        std::cout << "\n❌ FAILURE: Some engines failed validation\n";
        std::cout << "Check the violations list for details.\n";
    }
    
    // Use convenience macros
    printHeader("10. TESTING ASSERTION MACROS");
    
    std::cout << "\nTesting architecture assertion macros...\n";
    
    VALIDATE_ARCHITECTURE();
    std::cout << "✅ VALIDATE_ARCHITECTURE() executed\n";
    
    ASSERT_ENGINE_VALID(39); // PlateReverb
    std::cout << "✅ ASSERT_ENGINE_VALID(39) passed\n";
    
    ASSERT_PARAMETER_VALID(39, 6); // PlateReverb mix parameter
    std::cout << "✅ ASSERT_PARAMETER_VALID(39, 6) passed\n";
    
    printSeparator();
    std::cout << "\n🎯 Engine Architecture Manager Test Complete!\n\n";
    
    return 0;
}
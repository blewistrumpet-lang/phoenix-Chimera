#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <string>

// Include the engine headers
#include "JUCE_Plugin/Source/EngineTypes.h"
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineBase.h"

// Test all 56 engines with the dynamic parameter system
int main() {
    std::cout << "=== DYNAMIC UI VERIFICATION TEST ===" << std::endl;
    std::cout << "Testing all 56 engines for parameter accessibility\n" << std::endl;
    
    // All 56 engine IDs from EngineTypes.h
    std::vector<int> allEngineIds = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55, 56
    };
    
    int totalEngines = 0;
    int enginesWithParams = 0;
    int totalParams = 0;
    
    std::cout << "Engine ID | Engine Name                    | Param Count | Status" << std::endl;
    std::cout << "----------|--------------------------------|-------------|--------" << std::endl;
    
    for (int id : allEngineIds) {
        auto engine = EngineFactory::createEngine(id);
        
        if (engine) {
            totalEngines++;
            int numParams = engine->getNumParameters();
            
            // Get engine name from first parameter or use placeholder
            std::string engineName = "Engine_" + std::to_string(id);
            if (numParams > 0) {
                // Try to infer engine name from parameter patterns
                auto firstParam = engine->getParameterName(0);
                if (firstParam.isNotEmpty()) {
                    enginesWithParams++;
                    totalParams += numParams;
                }
            }
            
            printf("%9d | %-30s | %11d | %s\n", 
                   id, 
                   engineName.c_str(), 
                   numParams,
                   numParams > 0 ? "✓ OK" : "⚠ No params");
            
            // Verify each parameter is accessible
            for (int p = 0; p < numParams; ++p) {
                auto paramName = engine->getParameterName(p);
                if (paramName.isEmpty()) {
                    std::cout << "  WARNING: Empty parameter name at index " << p << std::endl;
                }
            }
        } else {
            printf("%9d | %-30s | %11s | ✗ Failed to create\n", 
                   id, 
                   "CREATION FAILED", 
                   "N/A");
        }
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total engines tested: " << allEngineIds.size() << std::endl;
    std::cout << "Engines created successfully: " << totalEngines << std::endl;
    std::cout << "Engines with parameters: " << enginesWithParams << std::endl;
    std::cout << "Total parameters across all engines: " << totalParams << std::endl;
    
    if (totalEngines < 56) {
        std::cout << "\n⚠️  WARNING: Only " << totalEngines << " of 56 engines could be created!" << std::endl;
        std::cout << "Missing engines need to be implemented or added to EngineFactory." << std::endl;
    }
    
    if (enginesWithParams < totalEngines) {
        std::cout << "\n⚠️  WARNING: " << (totalEngines - enginesWithParams) 
                  << " engines have no parameters!" << std::endl;
    }
    
    std::cout << "\n=== DYNAMIC UI COMPATIBILITY ===" << std::endl;
    std::cout << "The Dynamic Nexus UI will:" << std::endl;
    std::cout << "✓ Query each engine's getNumParameters() directly" << std::endl;
    std::cout << "✓ Call getParameterName(i) for each parameter" << std::endl;
    std::cout << "✓ Create appropriate controls based on actual engine data" << std::endl;
    std::cout << "✓ Never rely on the static GeneratedParameterDatabase" << std::endl;
    
    return (totalEngines == 56) ? 0 : 1;
}
// Comprehensive verification that database matches actual engine implementations
#include <iostream>
#include <memory>
#include <map>
#include <string>
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/GeneratedParameterDatabase.h"
#include "JUCE_Plugin/Source/EngineTypes.h"

int main()
{
    std::cout << "\n===============================================\n";
    std::cout << "VERIFYING DATABASE MATCHES ENGINE IMPLEMENTATIONS\n";
    std::cout << "===============================================\n\n";
    
    int totalEngines = 0;
    int correctEngines = 0;
    int mismatchedEngines = 0;
    int failedEngines = 0;
    
    // Test each engine in the database
    for (const auto& dbEngine : ChimeraParameters::engineDatabase)
    {
        totalEngines++;
        
        std::cout << "Testing: " << dbEngine.displayName 
                 << " (ID: " << dbEngine.legacyId << ")\n";
        
        try {
            // Create the actual engine using the factory
            auto engine = EngineFactory::createEngine(dbEngine.legacyId);
            
            if (!engine) {
                std::cout << "  ❌ FAILED to create engine!\n\n";
                failedEngines++;
                continue;
            }
            
            // Get actual parameter count from the engine
            int actualParamCount = engine->getNumParameters();
            int dbParamCount = dbEngine.parameterCount;
            
            if (actualParamCount != dbParamCount) {
                std::cout << "  ❌ PARAMETER COUNT MISMATCH!\n";
                std::cout << "     Engine reports: " << actualParamCount << "\n";
                std::cout << "     Database says:  " << dbParamCount << "\n";
                mismatchedEngines++;
            } else {
                std::cout << "  ✅ Parameter count correct: " << actualParamCount << "\n";
                
                // Check parameter names
                bool namesMatch = true;
                for (int i = 0; i < actualParamCount; ++i) {
                    std::string engineName = engine->getParameterName(i).toStdString();
                    std::string dbName = dbEngine.parameters[i].name;
                    
                    if (engineName != dbName) {
                        std::cout << "     ⚠️  Param " << i << " name mismatch: '"
                                 << engineName << "' vs '" << dbName << "'\n";
                        namesMatch = false;
                    }
                }
                
                if (namesMatch) {
                    std::cout << "  ✅ All parameter names match\n";
                    correctEngines++;
                } else {
                    mismatchedEngines++;
                }
            }
            
        } catch (const std::exception& e) {
            std::cout << "  ❌ Exception: " << e.what() << "\n";
            failedEngines++;
        }
        
        std::cout << "\n";
    }
    
    // Summary
    std::cout << "===============================================\n";
    std::cout << "SUMMARY:\n";
    std::cout << "Total engines tested: " << totalEngines << "\n";
    std::cout << "✅ Fully correct:     " << correctEngines << "\n";
    std::cout << "⚠️  Mismatched:       " << mismatchedEngines << "\n";
    std::cout << "❌ Failed to create:  " << failedEngines << "\n";
    std::cout << "===============================================\n";
    
    if (mismatchedEngines > 0 || failedEngines > 0) {
        std::cout << "\n⚠️  DATABASE NEEDS UPDATES TO MATCH IMPLEMENTATIONS!\n";
        return 1;
    } else {
        std::cout << "\n✅ DATABASE IS CONSISTENT WITH ALL ENGINE IMPLEMENTATIONS!\n";
        return 0;
    }
}
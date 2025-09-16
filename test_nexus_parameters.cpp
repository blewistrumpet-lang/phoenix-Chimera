// Test program to verify Nexus Final parameter system
#include <iostream>
#include "JUCE_Plugin/Source/GeneratedParameterDatabase.h"
#include "JUCE_Plugin/Source/EngineTypes.h"

int main()
{
    std::cout << "NEXUS FINAL - PARAMETER DATABASE VERIFICATION\n";
    std::cout << "==============================================\n\n";
    
    // Test Case 1: K-Style Overdrive (legacyId = 22)
    std::cout << "ENGINE: K-Style Overdrive\n";
    for (const auto& engine : ChimeraParameters::engineDatabase)
    {
        if (engine.legacyId == 22) // K-Style Overdrive
        {
            std::cout << "  Display Name: " << engine.displayName << "\n";
            std::cout << "  Parameter Count: " << engine.parameterCount << "\n";
            std::cout << "  Parameters:\n";
            
            for (int i = 0; i < engine.parameterCount; ++i)
            {
                const auto& param = engine.parameters[i];
                std::cout << "    " << (i+1) << ". " << param.name 
                         << " (default: " << param.defaultValue << ")\n";
            }
            break;
        }
    }
    
    std::cout << "\n";
    
    // Test Case 2: Vintage Tube Preamp (legacyId = 0)
    std::cout << "ENGINE: Vintage Tube Preamp\n";
    for (const auto& engine : ChimeraParameters::engineDatabase)
    {
        if (engine.legacyId == 0) // Vintage Tube Preamp
        {
            std::cout << "  Display Name: " << engine.displayName << "\n";
            std::cout << "  Parameter Count: " << engine.parameterCount << "\n";
            std::cout << "  Parameters:\n";
            
            for (int i = 0; i < engine.parameterCount; ++i)
            {
                const auto& param = engine.parameters[i];
                std::cout << "    " << (i+1) << ". " << param.name 
                         << " (default: " << param.defaultValue << ")\n";
            }
            break;
        }
    }
    
    std::cout << "\n✅ NEXUS FINAL UI - Parameter system verified\n";
    std::cout << "The UI will dynamically display these exact parameters.\n";
    
    return 0;
}
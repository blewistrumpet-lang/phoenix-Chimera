// Comprehensive test to verify ALL engine parameters match actual implementations
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "JUCE_Plugin/Source/GeneratedParameterDatabase.h"
#include "JUCE_Plugin/Source/EngineTypes.h"

struct EngineParameterCheck {
    int engineId;
    std::string engineName;
    std::vector<std::string> actualParameters;
    int actualCount;
};

// Based on actual implementation files checked
std::map<int, EngineParameterCheck> actualEngineParameters = {
    // Reverbs (verified from actual .cpp files)
    {2, {2, "Shimmer Reverb", 
        {"Mix", "Pitch Shift", "Shimmer", "Size", "Damping", 
         "Feedback", "Pre-Delay", "Modulation", "Low Cut", "High Cut"}, 10}},
    
    {3, {3, "Plate Reverb",
        {"Mix", "Size", "Damping", "Pre-Delay", "Width",
         "Freeze", "Low Cut", "High Cut", "Early Reflections", "Diffusion"}, 10}},
    
    {5, {5, "Spring Reverb",
        {"Mix", "Tension", "Damping", "Decay", "Pre-Delay",
         "Drive", "Chirp", "Low Cut", "High Cut"}, 9}},
    
    {43, {43, "Gated Reverb",
        {"Mix", "Threshold", "Hold", "Release", "Attack",
         "Size", "Damping", "Pre-Delay", "Low Cut", "High Cut"}, 10}},
    
    {41, {41, "Convolution Reverb",
        {"Mix", "IR Select", "Size", "Pre-Delay", "Damping",
         "Reverse", "Early/Late", "Low Cut", "High Cut", "Width"}, 10}},
    
    // Dynamics (from NoiseGate.cpp we saw earlier)
    {4, {4, "Noise Gate",
        {"Threshold", "Range", "Attack", "Hold", "Release",
         "Hysteresis", "SC Filter", "Lookahead"}, 8}},
};

int main()
{
    std::cout << "\n========================================\n";
    std::cout << "ENGINE PARAMETER DATABASE VERIFICATION\n";
    std::cout << "========================================\n\n";
    
    int totalEngines = 0;
    int correctEngines = 0;
    int incorrectEngines = 0;
    std::vector<std::string> issues;
    
    // Check each engine in the database
    for (const auto& engine : ChimeraParameters::engineDatabase)
    {
        totalEngines++;
        
        // Check if we have actual parameter data for this engine
        auto it = actualEngineParameters.find(engine.legacyId);
        if (it != actualEngineParameters.end())
        {
            const auto& actual = it->second;
            
            std::cout << "Checking: " << engine.displayName 
                     << " (ID: " << engine.legacyId << ")\n";
            
            // Check parameter count
            if (engine.parameterCount != actual.actualCount)
            {
                std::cout << "  ❌ PARAMETER COUNT MISMATCH!\n";
                std::cout << "     Database says: " << engine.parameterCount << "\n";
                std::cout << "     Actual implementation: " << actual.actualCount << "\n";
                
                issues.push_back(std::string(engine.displayName) + ": count mismatch (" + 
                               std::to_string(engine.parameterCount) + " vs " + 
                               std::to_string(actual.actualCount) + ")");
                incorrectEngines++;
            }
            else
            {
                std::cout << "  ✅ Parameter count correct: " << engine.parameterCount << "\n";
                
                // Check parameter names match
                bool namesMatch = true;
                for (int i = 0; i < engine.parameterCount && i < actual.actualParameters.size(); ++i)
                {
                    if (std::string(engine.parameters[i].name) != actual.actualParameters[i])
                    {
                        std::cout << "     ⚠️  Parameter " << i << " name mismatch: '"
                                 << engine.parameters[i].name << "' vs '"
                                 << actual.actualParameters[i] << "'\n";
                        namesMatch = false;
                    }
                }
                
                if (namesMatch)
                {
                    std::cout << "  ✅ All parameter names match\n";
                    correctEngines++;
                }
                else
                {
                    issues.push_back(std::string(engine.displayName) + ": parameter name mismatches");
                    incorrectEngines++;
                }
            }
            
            std::cout << "\n";
        }
    }
    
    // Summary report
    std::cout << "\n========================================\n";
    std::cout << "SUMMARY REPORT\n";
    std::cout << "========================================\n";
    std::cout << "Total engines in database: " << totalEngines << "\n";
    std::cout << "Engines verified: " << (correctEngines + incorrectEngines) << "\n";
    std::cout << "✅ Correct: " << correctEngines << "\n";
    std::cout << "❌ Issues found: " << incorrectEngines << "\n";
    
    if (!issues.empty())
    {
        std::cout << "\nISSUES TO FIX:\n";
        for (const auto& issue : issues)
        {
            std::cout << "  - " << issue << "\n";
        }
    }
    
    // List all engines for reference
    std::cout << "\n========================================\n";
    std::cout << "ALL ENGINES IN DATABASE:\n";
    std::cout << "========================================\n";
    int count = 0;
    for (const auto& engine : ChimeraParameters::engineDatabase)
    {
        count++;
        std::cout << count << ". " << engine.displayName 
                 << " (ID: " << engine.legacyId 
                 << ", Params: " << engine.parameterCount << ")\n";
    }
    
    return (incorrectEngines > 0) ? 1 : 0;
}
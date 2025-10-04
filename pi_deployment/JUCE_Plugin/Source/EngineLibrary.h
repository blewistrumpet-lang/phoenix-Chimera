#pragma once
#include <string>
#include "EngineTypes.h"
#include "GeneratedParameterDatabase.h"

/**
 * EngineLibrary - Centralized engine information
 */
class EngineLibrary 
{
public:
    static int getEngineCount() { return 56; }
    
    static std::string getEngineName(int engineId)
    {
        // Use the getEngineTypeName function from EngineTypes.h
        return std::string(getEngineTypeName(engineId));
    }
    
    static std::string getParameterName(int engineId, int paramIndex)
    {
        // Search the engine database for this engine
        for (const auto& engine : ChimeraParameters::engineDatabase)
        {
            if (engine.legacyId == engineId)
            {
                if (paramIndex >= 0 && paramIndex < engine.parameterCount)
                {
                    return std::string(engine.parameters[paramIndex].name);
                }
                break;
            }
        }
        
        // Fallback to generic name
        return "Param " + std::to_string(paramIndex + 1);
    }
    
    static int getParameterCount(int engineId)
    {
        // Search the engine database for this engine
        for (const auto& engine : ChimeraParameters::engineDatabase)
        {
            if (engine.legacyId == engineId)
            {
                return engine.parameterCount;
            }
        }
        
        // Default fallback
        return 10;
    }
};
#pragma once

/**
 * EngineDefaults.h - Legacy compatibility wrapper
 * 
 * This file now provides legacy compatibility by delegating to the 
 * UnifiedDefaultParameters system, which is the single source of truth
 * for all default parameter values.
 * 
 * Note: This file is maintained only for backward compatibility.
 * New code should use UnifiedDefaultParameters directly.
 * 
 * Replaced by UnifiedDefaultParameters system on 2025-08-19
 */

#include <map>
#include "EngineTypes.h"
#include "UnifiedDefaultParameters.h"

namespace EngineDefaults {
    
    /**
     * Legacy compatibility function - delegates to UnifiedDefaultParameters
     * @deprecated Use UnifiedDefaultParameters::getDefaultParameters() directly
     * 
     * This function maintains compatibility for any remaining code that
     * might still reference EngineDefaults::getDefaultParameters(). 
     * 
     * All default values now come from the comprehensive, tested, and
     * musically optimized UnifiedDefaultParameters system.
     */
    inline std::map<int, float> getDefaultParameters(int engineID) {
        return UnifiedDefaultParameters::getDefaultParameters(engineID);
    }
    
}
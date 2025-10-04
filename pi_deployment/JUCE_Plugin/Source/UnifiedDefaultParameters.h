#pragma once

#include <map>
#include <vector>
#include <string>
#include "EngineTypes.h"

/**
 * Unified Default Parameter System for Chimera Phoenix
 * 
 * This is the single source of truth for all default parameter values.
 * It replaces the fragmented system of multiple default sources with
 * one comprehensive, tested, and musically optimized system.
 * 
 * Design Principles:
 * 1. Safety First: No harsh, damaging, or unusable sounds
 * 2. Musical Utility: Immediate satisfaction and inspiration  
 * 3. Professional Polish: Values suitable for production use
 * 4. Consistent Categories: Similar engines have similar defaults
 * 5. Educational Value: Defaults teach proper parameter relationships
 */

namespace UnifiedDefaultParameters {

/**
 * Engine category definitions for consistent default patterns
 */
enum class EngineCategory {
    DISTORTION,      // 100% mix, 20-30% drive, full character
    SATURATION,      // 80-100% mix, subtle warmth enhancement
    REVERB,          // 25-35% mix, medium decay, tasteful ambience
    DELAY,           // 25-35% mix, musical timing, 2-3 repeats
    MODULATION,      // 30-50% mix, 2-5Hz rates, subtle movement  
    FILTER,          // Variable mix, midrange cutoff, musical resonance
    DYNAMICS,        // 100% mix, transparent control, musical ratios
    SPATIAL,         // Variable mix, balanced processing, mono compatibility
    PITCH,           // 50% mix, conservative shifting, formant preservation
    UTILITY,         // 100% mix, unity gain, neutral starting points
    SPECTRAL,        // 20-30% mix, conservative processing, safe exploration
    EXPERIMENTAL     // 20-30% mix, minimal initial impact, user exploration
};

/**
 * Parameter metadata for validation and display
 */
struct ParameterInfo {
    std::string name;
    float defaultValue;
    float minValue;
    float maxValue;
    std::string description;
    std::string units;
};

/**
 * Complete engine default configuration
 */
struct EngineDefaults {
    int engineId;
    std::string name;
    EngineCategory category;
    std::map<int, ParameterInfo> parameters;
    std::string notes;  // Special considerations or usage tips
};

/**
 * Get optimized default parameters for any engine
 * 
 * @param engineId The engine ID from EngineTypes.h
 * @return Map of parameter index to default value (0.0-1.0 normalized)
 */
std::map<int, float> getDefaultParameters(int engineId);

/**
 * Get complete engine configuration including metadata
 * 
 * @param engineId The engine ID
 * @return Complete EngineDefaults structure with all information
 */
EngineDefaults getEngineDefaults(int engineId);

/**
 * Get all engines organized by category
 * 
 * @return Map of category to vector of engine IDs
 */
std::map<EngineCategory, std::vector<int>> getEnginesByCategory();

/**
 * Get parameter name for display
 * 
 * @param engineId The engine ID  
 * @param paramIndex The parameter index (0-based)
 * @return Human-readable parameter name
 */
std::string getParameterName(int engineId, int paramIndex);

/**
 * Get parameter count for an engine
 * 
 * @param engineId The engine ID
 * @return Number of parameters for this engine
 */
int getParameterCount(int engineId);

/**
 * Validate engine defaults for safety and consistency
 * 
 * @param engineId The engine ID to validate
 * @return True if defaults pass all safety and musical utility tests
 */
bool validateEngineDefaults(int engineId);

/**
 * Get category-specific default guidelines
 * 
 * @param category The engine category
 * @return Description of default value strategy for this category
 */
std::string getCategoryGuidelines(EngineCategory category);

/**
 * Apply defaults to parameter map (for integration with existing systems)
 * 
 * @param engineId The engine ID
 * @param parameterMap Map to fill with default values
 */
void applyDefaultsToMap(int engineId, std::map<int, float>& parameterMap);

/**
 * Get mix parameter index for an engine (returns -1 if no mix parameter)
 * 
 * @param engineId The engine ID
 * @return Index of mix parameter, or -1 if not applicable
 */
int getMixParameterIndex(int engineId);

} // namespace UnifiedDefaultParameters
#pragma once

#include <map>
#include <vector>
#include <string>

namespace DefaultParameterValues {

/**
 * Structure to hold complete default information for an engine
 */
struct EngineDefaultInfo {
    int engineId;
    std::string name;
    std::string category;
    std::map<int, float> defaults; // parameter index -> default value
};

/**
 * Get default parameter values for a specific engine
 * 
 * @param engineType The engine ID (from ParameterDefinitions.h)
 * @return Map of parameter index to default value (0.0-1.0 normalized)
 */
std::map<int, float> getDefaultParameters(int engineType);

/**
 * Get complete default information for all engines
 * 
 * @return Vector of EngineDefaultInfo structures for all 57 engines (0-56)
 */
std::vector<EngineDefaultInfo> getAllEngineDefaults();

/**
 * Get engines organized by category
 * 
 * @return Map of category name to vector of engine IDs
 */
std::map<std::string, std::vector<int>> getEnginesByCategory();

/**
 * Get a human-readable parameter name for display
 * 
 * @param engineId The engine ID
 * @param paramIndex The parameter index (0-based)
 * @return Parameter name string
 */
std::string getParameterName(int engineId, int paramIndex);

/**
 * Get the total number of parameters for an engine
 * 
 * @param engineId The engine ID
 * @return Number of parameters for this engine
 */
int getParameterCount(int engineId);

/**
 * Categories used for engine organization:
 * 
 * - "distortion": Hard clipping distortions and fuzzes
 * - "saturation": Tube/analog saturation and harmonic enhancement  
 * - "delay": All delay-based effects (tape, digital, BBD, etc.)
 * - "reverb": All reverb types (plate, convolution, spring, etc.)
 * - "modulation": LFO-based effects (tremolo, chorus, phaser, etc.)
 * - "filter": All filtering effects (ladder, formant, envelope, etc.)
 * - "dynamics": Compressors, limiters, gates, transient shapers
 * - "spatial": Stereo imaging and M/S processing
 * - "eq": Equalizers and tone shaping
 * - "spectral": FFT-based spectral processing
 * - "granular": Granular synthesis and processing
 * - "pitch": Pitch shifting and harmonization
 * - "experimental": Creative/experimental effects
 * - "glitch": Glitch and buffer manipulation effects
 */

/**
 * Default Parameter Design Principles:
 * 
 * 1. SAFETY FIRST: All defaults avoid harsh, unusable, or damaging sounds
 * 2. MUSICAL UTILITY: Each engine sounds good immediately upon loading
 * 3. MODERATE VALUES: Most parameters start in the 0.3-0.7 range
 * 4. CONSERVATIVE DRIVE: Drive/gain parameters start low (0.2-0.4)
 * 5. APPROPRIATE MIX: 
 *    - Most effects: 100% wet (1.0) for full effect
 *    - Distortions: 70-80% wet for blend with dry signal
 *    - Dynamics: 100% wet (no dry signal needed)
 * 6. MUSICAL TIMING: Time-based effects use musical note values
 * 7. CONTROLLED FEEDBACK: Conservative feedback (0.2-0.4) to avoid runaway
 * 8. SMOOTH RESONANCE: Moderate Q/resonance (0.3-0.5) for musicality  
 * 9. UNITY GAIN: Output levels maintain consistent volume
 * 10. FIRST IMPRESSION: Users should hear something inspiring immediately
 */

} // namespace DefaultParameterValues
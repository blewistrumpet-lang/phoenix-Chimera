#pragma once

#include <JuceHeader.h>
#include "GoldenPreset.h"

/**
 * PresetVariationGenerator - Creates musical variations of existing presets
 * Uses intelligent parameter morphing to create related but distinct presets
 */
class PresetVariationGenerator {
public:
    // Variation strategies
    enum class VariationType {
        Subtle,          // Small parameter tweaks (±10%)
        Moderate,        // Noticeable changes (±25%)
        Extreme,         // Dramatic alterations (±50%)
        Dark,            // Reduce brightness, increase density
        Bright,          // Increase brightness, reduce density
        Spacious,        // More reverb/delay, wider stereo
        Intimate,        // Less space, more focused
        Aggressive,      // More drive, compression, presence
        Gentle,          // Less drive, softer attack
        Vintage,         // More analog character, noise
        Modern,          // Cleaner, more precise
        Minimal,         // Reduce active engines, simplify
        Maximal,         // Add engines, increase complexity
        Rhythmic,        // Add tempo-synced elements
        Ambient          // Remove rhythm, add space
    };
    
    // Generate variations
    static std::vector<std::unique_ptr<GoldenPreset>> generateVariations(
        const GoldenPreset& parent,
        const std::vector<VariationType>& types
    );
    
    // Generate single variation
    static std::unique_ptr<GoldenPreset> generateVariation(
        const GoldenPreset& parent,
        VariationType type
    );
    
    // Auto-generate complementary variations
    static std::vector<std::unique_ptr<GoldenPreset>> generateComplementarySet(
        const GoldenPreset& parent,
        int count = 3
    );
    
private:
    // Variation application methods
    static void applySubtleVariation(GoldenPreset& preset);
    static void applyModerateVariation(GoldenPreset& preset);
    static void applyExtremeVariation(GoldenPreset& preset);
    static void applyDarkVariation(GoldenPreset& preset);
    static void applyBrightVariation(GoldenPreset& preset);
    static void applySpaciousVariation(GoldenPreset& preset);
    static void applyIntimateVariation(GoldenPreset& preset);
    static void applyAggressiveVariation(GoldenPreset& preset);
    static void applyGentleVariation(GoldenPreset& preset);
    static void applyVintageVariation(GoldenPreset& preset);
    static void applyModernVariation(GoldenPreset& preset);
    static void applyMinimalVariation(GoldenPreset& preset);
    static void applyMaximalVariation(GoldenPreset& preset);
    static void applyRhythmicVariation(GoldenPreset& preset);
    static void applyAmbientVariation(GoldenPreset& preset);
    
    // Helper methods
    static void morphParameter(float& param, float amount, float min = 0.0f, float max = 1.0f);
    static void scaleParameters(std::vector<float>& params, float scale);
    static void shiftParameters(std::vector<float>& params, float offset);
    static void randomizeParameters(std::vector<float>& params, float amount);
    
    // Engine manipulation
    static void swapEngines(GoldenPreset& preset, int slot1, int slot2);
    static void replaceEngine(GoldenPreset& preset, int slot, int newEngine);
    static void disableEngine(GoldenPreset& preset, int slot);
    static void addComplementaryEngine(GoldenPreset& preset);
    
    // Metadata updates
    static void updateMetadataForVariation(GoldenPreset& preset, 
                                          const GoldenPreset& parent,
                                          VariationType type);
    static String generateVariationName(const String& parentName, VariationType type);
    static void adjustSonicProfile(GoldenPreset& preset, VariationType type);
    static void adjustEmotionalProfile(GoldenPreset& preset, VariationType type);
    
    // Musical relationships
    static float getParameterImportance(int engineType, int paramIndex);
    static bool shouldVaryParameter(int engineType, int paramIndex, VariationType type);
    static float getVariationAmount(VariationType type);
    
    // Engine selection for variations
    static int selectReplacementEngine(int originalEngine, VariationType type);
    static std::vector<int> getCompatibleEngines(int engineType);
    static bool enginesSuitableForType(int engine, VariationType type);
};
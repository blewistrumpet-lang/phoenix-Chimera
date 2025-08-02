#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <string>

/**
 * Golden Corpus Preset Structure
 * Core data structure for the 250 curated presets that serve as the DNA
 * for the Trinity AI system
 */

enum class CPUTier {
    LIGHT = 0,      // <3% CPU (120 presets) - For multiple instances
    MEDIUM = 1,     // 3-8% CPU (100 presets) - Standard use
    HEAVY = 2,      // 8-15% CPU (25 presets) - Feature showcase
    EXTREME = 3     // 15-25% CPU (5 presets) - "Hero" presets
};

struct SonicProfile {
    float brightness = 0.5f;      // 0=Dark, 1=Bright
    float density = 0.5f;         // 0=Sparse, 1=Dense
    float movement = 0.5f;        // 0=Static, 1=Animated
    float space = 0.5f;           // 0=Dry, 1=Expansive
    float aggression = 0.5f;      // 0=Gentle, 1=Aggressive
    float vintage = 0.5f;         // 0=Modern, 1=Vintage
};

struct EmotionalProfile {
    float energy = 0.5f;          // 0=Calm, 1=Energetic
    float mood = 0.5f;            // 0=Dark, 1=Uplifting
    float tension = 0.5f;         // 0=Relaxed, 1=Tense
    float organic = 0.5f;         // 0=Digital, 1=Organic
    float nostalgia = 0.5f;       // 0=Contemporary, 1=Nostalgic
};

struct SourceAffinity {
    float vocals = 0.5f;          // How well it works on vocals
    float guitar = 0.5f;          // How well it works on guitar
    float drums = 0.5f;           // How well it works on drums
    float synth = 0.5f;           // How well it works on synths
    float mix = 0.5f;             // How well it works on full mixes
};

struct GoldenPreset {
    // Identification
    String id;                          // "GC_001" - "GC_250"
    String name;                        // "Tears in the Storm"
    String technicalHint;               // "Shimmer Verb + Tremolo"
    String shortCode;                   // "TRS-01"
    int version = 1;
    
    // Hierarchy
    bool isVariation = false;
    String parentId = "";               // "GC_001" if this is a variation
    
    // Engine Configuration (all normalized 0-1)
    std::array<int, 6> engineTypes;     // -1 for empty slot
    std::array<float, 6> engineMix;     // 0.0-1.0 mix level
    std::array<bool, 6> engineActive;   // true if active
    std::array<std::vector<float>, 6> engineParams;  // Normalized parameters
    
    // AI Metadata
    SonicProfile sonicProfile;
    EmotionalProfile emotionalProfile;
    SourceAffinity sourceAffinity;
    
    // Performance
    CPUTier cpuTier = CPUTier::LIGHT;
    float actualCpuPercent = 0.0f;
    float latencySamples = 0.0f;
    bool realtimeSafe = true;
    
    // Musical Context
    float optimalTempo = 0.0f;          // 0 = tempo-independent
    String musicalKey = "";             // "" = key-independent
    std::vector<String> genres;         // ["ambient", "electronic"]
    
    // Quality & Tracking
    String signature;                   // Designer credit
    Time creationDate;
    int popularityScore = 0;
    float qualityScore = 0.0f;          // From validation
    
    // Searchability
    std::vector<String> keywords;
    std::vector<String> antiFeatures;   // Things to avoid
    std::vector<String> userPrompts;    // Example prompts
    
    // Category
    String category;                    // "Studio Essential", "Spatial Design", etc.
    String subcategory;                 // "Vocal Chain", "Natural Space", etc.
    
    // Complexity metrics
    float complexity = 0.5f;            // 0=Simple, 1=Complex
    float experimentalness = 0.5f;      // 0=Traditional, 1=Avant-garde
    float versatility = 0.5f;           // 0=Specialized, 1=Versatile
    
    // Usage hints
    String bestFor;                     // "Vocals in sparse arrangements"
    String avoidFor;                    // "Dense mixes, bass instruments"
    
    // Initialize with safe defaults
    GoldenPreset() {
        engineTypes.fill(-1);
        engineMix.fill(0.0f);
        engineActive.fill(false);
        for (auto& params : engineParams) {
            params.resize(8, 0.5f);  // 8 params per engine, default 0.5
        }
    }
    
    // Helper methods
    int getActiveEngineCount() const {
        int count = 0;
        for (int i = 0; i < 6; ++i) {
            if (engineTypes[i] >= 0 && engineActive[i]) {
                count++;
            }
        }
        return count;
    }
    
    bool hasEngine(int engineType) const {
        for (int type : engineTypes) {
            if (type == engineType) return true;
        }
        return false;
    }
    
    // Validation
    bool isValid() const {
        // Must have at least one active engine
        if (getActiveEngineCount() == 0) return false;
        
        // All active engines must have valid types
        for (int i = 0; i < 6; ++i) {
            if (engineActive[i] && (engineTypes[i] < 0 || engineTypes[i] >= 50)) {
                return false;
            }
        }
        
        // All parameters must be normalized
        for (const auto& engineParamSet : engineParams) {
            for (float param : engineParamSet) {
                if (param < 0.0f || param > 1.0f) return false;
            }
        }
        
        // Mix levels must be normalized
        for (float mix : engineMix) {
            if (mix < 0.0f || mix > 1.0f) return false;
        }
        
        return true;
    }
};

// Preset categories for organization
namespace PresetCategories {
    const String STUDIO_ESSENTIALS = "Studio Essentials";
    const String SPATIAL_DESIGN = "Spatial Design";
    const String CHARACTER_COLOR = "Character & Color";
    const String MOTION_MODULATION = "Motion & Modulation";
    const String EXPERIMENTAL_LAB = "Experimental Laboratory";
}

// Helper for creating variations
struct PresetVariation {
    String suffix;              // "Subtle", "Extreme", "Dark", etc.
    std::function<void(GoldenPreset&)> transform;  // Modification function
};
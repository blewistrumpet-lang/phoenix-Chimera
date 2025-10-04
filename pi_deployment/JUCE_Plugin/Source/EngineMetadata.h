#pragma once
#include <JuceHeader.h>
#include <map>
#include <vector>

// Engine metadata structure for AI interpretation
struct ParameterRange {
    juce::String range;
    juce::String description;
};

struct ParameterMetadata {
    juce::String name;
    float defaultValue;
    float min;
    float max;
    juce::String unit;
    juce::String curve; // "linear", "logarithmic", "exponential"
    std::vector<ParameterRange> rangeDescriptions;
};

struct EngineMetadata {
    // Basic identification
    int engineId;
    juce::String name;
    juce::String category; // "distortion", "dynamics", "reverb", "delay", "modulation", "filter", "utility"
    juce::String description;
    
    // Sonic characteristics
    std::vector<juce::String> sonicTags;
    std::vector<juce::String> emotionalTags;
    juce::String frequencyFocus; // "low", "mid", "high", "full"
    
    // Use cases
    std::vector<juce::String> typicalUseCases;
    std::vector<juce::String> instrumentTags;
    
    // Technical properties
    float latencySamples = 0.0f;
    float cpuComplexity = 0.5f; // 0-1 scale
    bool supportsSidechain = false;
    bool requiresStereo = false;
    
    // Parameters
    std::vector<ParameterMetadata> parameters;
    
    // AI guidance
    std::vector<juce::String> triggerWords;
    std::map<int, float> compatibilityScores; // engineId -> score (0-1)
    std::map<juce::String, float> moodAdjustments; // mood -> parameter adjustment
    
    // Compatibility
    std::vector<juce::String> pairsWellWith;
    std::vector<juce::String> avoidWith;
};

// Preset metadata for Golden Corpus
struct SlotConfiguration {
    int engineId;
    juce::String engineName;
    bool bypass;
    float mix;
    std::map<juce::String, float> parameters;
};

struct PresetMetadata {
    juce::String presetId;
    juce::String name;
    int engineCount;
    std::vector<SlotConfiguration> slots;
    
    // Sonic signature
    juce::String sonicSignature;
    juce::String emotionalCharacter;
    std::vector<juce::String> genreTags;
    std::vector<juce::String> instrumentTags;
    std::vector<juce::String> eraTags;
    
    // Technical info
    int complexity; // 1-6 based on engine count
    juce::String cpuLoad; // "low", "medium", "high"
    std::vector<int> keyEngines; // Most important engines in chain
    
    // Parameter sensitivity
    std::vector<juce::String> highImpactParams;
    std::vector<juce::String> mediumImpactParams;
    std::vector<juce::String> lowImpactParams;
    
    // AI training data
    std::vector<juce::String> commonPrompts;
    std::vector<juce::String> antiPrompts; // What this preset is NOT
    float blendCompatibility = 0.8f; // How well it blends with others
    float morphStability = 0.9f; // How well parameters can be adjusted
};

// Recipe format from Visionary
struct Recipe {
    juce::String userPrompt;
    std::vector<juce::String> sonicGoals;
    std::vector<juce::String> emotionalTargets;
    
    struct TechnicalHints {
        float wetness = 0.5f;
        float spaceSize = 0.5f;
        float brightness = 0.5f;
        float movement = 0.5f;
        float warmth = 0.5f;
        float aggression = 0.5f;
    } technicalHints;
    
    struct SuggestedEngine {
        int id;
        juce::String reason;
    };
    std::vector<SuggestedEngine> suggestedEngines;
    
    std::vector<juce::String> antiFeatures;
};

// Engine metadata registry
class EngineMetadataRegistry {
public:
    static EngineMetadataRegistry& getInstance() {
        static EngineMetadataRegistry instance;
        return instance;
    }
    
    void registerEngine(const EngineMetadata& metadata) {
        engineMetadata[metadata.engineId] = metadata;
    }
    
    EngineMetadata getEngineMetadata(int engineId) const {
        auto it = engineMetadata.find(engineId);
        if (it != engineMetadata.end()) {
            return it->second;
        }
        return EngineMetadata();
    }
    
    std::vector<int> findEnginesByTag(const juce::String& tag) const {
        std::vector<int> results;
        for (const auto& [id, metadata] : engineMetadata) {
            for (const auto& sonicTag : metadata.sonicTags) {
                if (sonicTag.containsIgnoreCase(tag)) {
                    results.push_back(id);
                    break;
                }
            }
        }
        return results;
    }
    
    std::vector<int> findEnginesByCategory(const juce::String& category) const {
        std::vector<int> results;
        for (const auto& [id, metadata] : engineMetadata) {
            if (metadata.category == category) {
                results.push_back(id);
            }
        }
        return results;
    }
    
private:
    EngineMetadataRegistry() = default;
    std::map<int, EngineMetadata> engineMetadata;
};
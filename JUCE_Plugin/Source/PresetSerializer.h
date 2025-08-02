#pragma once

#include <JuceHeader.h>
#include "GoldenPreset.h"

/**
 * Serialization system for Golden Corpus presets
 * Handles JSON (for development/editing) and Binary (for distribution)
 */

class PresetSerializer {
public:
    PresetSerializer() = default;
    
    // JSON Serialization
    static juce::var presetToJSON(const GoldenPreset& preset);
    static GoldenPreset presetFromJSON(const juce::var& json);
    
    // File I/O
    static bool savePresetToFile(const GoldenPreset& preset, const File& file);
    static GoldenPreset loadPresetFromFile(const File& file);
    
    // Binary Serialization (for fast loading in plugin)
    static MemoryBlock presetToBinary(const GoldenPreset& preset);
    static GoldenPreset presetFromBinary(const MemoryBlock& data);
    
    // Batch operations
    static bool saveCorpusToJSON(const std::vector<GoldenPreset>& corpus, const File& file);
    static std::vector<GoldenPreset> loadCorpusFromJSON(const File& file);
    
    // Validation
    static bool validatePresetJSON(const juce::var& json, String& errorMessage);
    
private:
    // Helper methods for JSON conversion
    static juce::var sonicProfileToJSON(const SonicProfile& profile);
    static SonicProfile sonicProfileFromJSON(const juce::var& json);
    
    static juce::var emotionalProfileToJSON(const EmotionalProfile& profile);
    static EmotionalProfile emotionalProfileFromJSON(const juce::var& json);
    
    static juce::var sourceAffinityToJSON(const SourceAffinity& affinity);
    static SourceAffinity sourceAffinityFromJSON(const juce::var& json);
    
    static juce::var engineParamsToJSON(const std::vector<float>& params);
    static std::vector<float> engineParamsFromJSON(const juce::var& json);
    
    // Binary format version
    static constexpr int BINARY_FORMAT_VERSION = 1;
};
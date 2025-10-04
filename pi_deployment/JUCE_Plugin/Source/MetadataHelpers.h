#pragma once
#include "EngineMetadata.h"
#include <functional>

// Helper class for creating consistent, rich metadata
class MetadataBuilder {
public:
    MetadataBuilder& setBasicInfo(int id, const juce::String& name, const juce::String& category, const juce::String& description) {
        meta.engineId = id;
        meta.name = name;
        meta.category = category;
        meta.description = description;
        return *this;
    }
    
    MetadataBuilder& setSonicTags(std::initializer_list<juce::String> tags) {
        meta.sonicTags = std::vector<juce::String>(tags);
        return *this;
    }
    
    MetadataBuilder& setEmotionalTags(std::initializer_list<juce::String> tags) {
        meta.emotionalTags = std::vector<juce::String>(tags);
        return *this;
    }
    
    MetadataBuilder& setFrequencyFocus(const juce::String& focus) {
        meta.frequencyFocus = focus;
        return *this;
    }
    
    MetadataBuilder& setUseCases(std::initializer_list<juce::String> cases) {
        meta.typicalUseCases = std::vector<juce::String>(cases);
        return *this;
    }
    
    MetadataBuilder& setInstrumentTags(std::initializer_list<juce::String> tags) {
        meta.instrumentTags = std::vector<juce::String>(tags);
        return *this;
    }
    
    MetadataBuilder& setTechnicalProps(float cpu, float latency, bool sidechain = false, bool stereo = false) {
        meta.cpuComplexity = cpu;
        meta.latencySamples = latency;
        meta.supportsSidechain = sidechain;
        meta.requiresStereo = stereo;
        return *this;
    }
    
    MetadataBuilder& addParameter(const juce::String& name, float defaultVal, float min, float max,
                                  const juce::String& unit, const juce::String& curve,
                                  std::initializer_list<ParameterRange> ranges) {
        ParameterMetadata param;
        param.name = name;
        param.defaultValue = defaultVal;
        param.min = min;
        param.max = max;
        param.unit = unit;
        param.curve = curve;
        param.rangeDescriptions = std::vector<ParameterRange>(ranges);
        meta.parameters.push_back(param);
        return *this;
    }
    
    MetadataBuilder& setTriggerWords(std::initializer_list<juce::String> words) {
        meta.triggerWords = std::vector<juce::String>(words);
        return *this;
    }
    
    MetadataBuilder& setCompatibility(std::initializer_list<std::pair<int, float>> scores) {
        meta.compatibilityScores = std::map<int, float>(scores);
        return *this;
    }
    
    MetadataBuilder& setPairsWellWith(std::initializer_list<juce::String> items) {
        meta.pairsWellWith = std::vector<juce::String>(items);
        return *this;
    }
    
    MetadataBuilder& setAvoidWith(std::initializer_list<juce::String> items) {
        meta.avoidWith = std::vector<juce::String>(items);
        return *this;
    }
    
    MetadataBuilder& setMoodAdjustments(std::initializer_list<std::pair<juce::String, float>> adjustments) {
        meta.moodAdjustments = std::map<juce::String, float>(adjustments);
        return *this;
    }
    
    EngineMetadata build() {
        return meta;
    }
    
private:
    EngineMetadata meta;
};

// Common parameter range templates
namespace ParamRanges {
    // Drive/Saturation ranges
    const std::vector<ParameterRange> DRIVE_RANGES = {
        {"0-10", "clean, no distortion"},
        {"10-25", "warm, subtle coloration"},
        {"25-40", "moderate saturation"},
        {"40-60", "heavy saturation"},
        {"60-80", "aggressive distortion"},
        {"80-100", "extreme, heavily clipped"}
    };
    
    // Mix ranges
    const std::vector<ParameterRange> MIX_RANGES = {
        {"0-20", "subtle, barely audible"},
        {"20-40", "present but background"},
        {"40-60", "balanced mix"},
        {"60-80", "effect-forward"},
        {"80-100", "wet signal dominates"}
    };
    
    // Time ranges (ms)
    const std::vector<ParameterRange> DELAY_TIME_RANGES = {
        {"0-10", "comb filtering"},
        {"10-30", "doubling effect"},
        {"30-100", "slapback echo"},
        {"100-300", "rhythmic delay"},
        {"300-600", "ambient delay"},
        {"600-1000", "long atmospheric"}
    };
    
    // Frequency ranges
    const std::vector<ParameterRange> FREQ_RANGES = {
        {"0-20", "sub bass (20-60Hz)"},
        {"20-40", "bass (60-200Hz)"},
        {"40-60", "low mids (200-800Hz)"},
        {"60-80", "high mids (800-4kHz)"},
        {"80-100", "treble (4k-20kHz)"}
    };
    
    // Q/Resonance ranges
    const std::vector<ParameterRange> Q_RANGES = {
        {"0-20", "very wide, gentle"},
        {"20-40", "wide, musical"},
        {"40-60", "moderate, focused"},
        {"60-80", "narrow, surgical"},
        {"80-100", "extremely narrow"}
    };
    
    // Modulation depth ranges
    const std::vector<ParameterRange> MOD_DEPTH_RANGES = {
        {"0-20", "subtle movement"},
        {"20-40", "noticeable modulation"},
        {"40-60", "pronounced effect"},
        {"60-80", "heavy modulation"},
        {"80-100", "extreme, seasick"}
    };
    
    // Feedback ranges
    const std::vector<ParameterRange> FEEDBACK_RANGES = {
        {"0-20", "single repeat"},
        {"20-40", "few repeats"},
        {"40-60", "multiple repeats"},
        {"60-75", "many repeats"},
        {"75-90", "near oscillation"},
        {"90-100", "self-oscillation"}
    };
}

// Category-specific sonic tag templates
namespace SonicTagTemplates {
    const std::vector<juce::String> VINTAGE_TAGS = {
        "vintage", "analog", "warm", "nostalgic", "classic", "retro", "old-school"
    };
    
    const std::vector<juce::String> MODERN_TAGS = {
        "modern", "digital", "pristine", "transparent", "clean", "precise", "hi-fi"
    };
    
    const std::vector<juce::String> AGGRESSIVE_TAGS = {
        "aggressive", "harsh", "intense", "brutal", "fierce", "raw", "powerful"
    };
    
    const std::vector<juce::String> AMBIENT_TAGS = {
        "ambient", "spacious", "ethereal", "atmospheric", "dreamy", "floating"
    };
    
    const std::vector<juce::String> EXPERIMENTAL_TAGS = {
        "experimental", "weird", "unusual", "creative", "unique", "avant-garde"
    };
}

// Emotional tag templates
namespace EmotionalTagTemplates {
    const std::vector<juce::String> WARM_EMOTIONS = {
        "cozy", "intimate", "friendly", "inviting", "comfortable", "familiar"
    };
    
    const std::vector<juce::String> AGGRESSIVE_EMOTIONS = {
        "angry", "fierce", "rebellious", "confrontational", "intense", "wild"
    };
    
    const std::vector<juce::String> ETHEREAL_EMOTIONS = {
        "dreamy", "floating", "transcendent", "otherworldly", "mystical", "celestial"
    };
    
    const std::vector<juce::String> DARK_EMOTIONS = {
        "ominous", "mysterious", "haunting", "brooding", "sinister", "foreboding"
    };
}
#pragma once
#include <string>
#include <vector>
#include <map>

// IntelligentHarmonizer Parameter Mappings
// Provides human-readable names for UI while maintaining numerical backend

namespace IntelligentHarmonizerParams {

// Interval mappings (semitones to names)
struct Interval {
    int semitones;
    const char* name;
    float normalizedValue; // 0-1 range where 0.5 = unison
};

static const std::vector<Interval> INTERVALS = {
    {-24, "-2 Oct",     0.000f},
    {-19, "-Oct+5th",   0.104f},
    {-12, "-Octave",    0.250f},
    {-7,  "-5th",       0.354f},
    {-6,  "-Tritone",   0.375f},
    {-5,  "-4th",       0.396f},
    {-4,  "-Maj 3rd",   0.417f},
    {-3,  "-Min 3rd",   0.438f},
    {-2,  "-Maj 2nd",   0.458f},
    {-1,  "-Min 2nd",   0.479f},
    {0,   "Unison",     0.500f},
    {1,   "+Min 2nd",   0.521f},
    {2,   "+Maj 2nd",   0.542f},
    {3,   "+Min 3rd",   0.563f},
    {4,   "+Maj 3rd",   0.583f},
    {5,   "+4th",       0.604f},
    {6,   "+Tritone",   0.625f},
    {7,   "+5th",       0.646f},
    {12,  "+Octave",    0.750f},
    {19,  "+Oct+5th",   0.896f},
    {24,  "+2 Oct",     1.000f}
};

// Scale type mappings with normalized values
struct Scale {
    int index;
    const char* name;
    float normalizedValue; // Discrete values for snapping
};

static const std::vector<Scale> SCALES = {
    {0, "Major",          0.00f},
    {1, "Natural Minor",  0.11f},
    {2, "Harmonic Minor", 0.22f},
    {3, "Melodic Minor",  0.33f},
    {4, "Dorian",         0.44f},
    {5, "Phrygian",       0.56f},
    {6, "Lydian",         0.67f},
    {7, "Mixolydian",     0.78f},
    {8, "Locrian",        0.89f},
    {9, "Chromatic",      1.00f}
};

// For backward compatibility
static const std::vector<const char*> SCALE_NAMES = {
    "Major",           // 0
    "Natural Minor",   // 1
    "Harmonic Minor",  // 2
    "Melodic Minor",   // 3
    "Dorian",          // 4
    "Phrygian",        // 5
    "Lydian",          // 6
    "Mixolydian",      // 7
    "Locrian",         // 8
    "Chromatic"        // 9
};

// Convert normalized value (0-1) to nearest musical interval
inline int normalizedToSemitones(float normalized) {
    // Find closest interval
    int bestSemitones = 0;
    float minDiff = 1.0f;
    
    for (const auto& interval : INTERVALS) {
        float diff = std::abs(normalized - interval.normalizedValue);
        if (diff < minDiff) {
            minDiff = diff;
            bestSemitones = interval.semitones;
        }
    }
    
    return bestSemitones;
}

// Convert semitones to normalized value
inline float semitonesToNormalized(int semitones) {
    // Clamp to range
    semitones = std::max(-24, std::min(24, semitones));
    
    // Find exact match or interpolate
    for (const auto& interval : INTERVALS) {
        if (interval.semitones == semitones) {
            return interval.normalizedValue;
        }
    }
    
    // If not in table, calculate linearly
    return 0.5f + (semitones / 48.0f);
}

// Get display name for interval
inline std::string getIntervalName(float normalized) {
    // Find closest interval
    float minDiff = 1.0f;
    const char* name = "Unison";
    
    for (const auto& interval : INTERVALS) {
        float diff = std::abs(normalized - interval.normalizedValue);
        if (diff < minDiff) {
            minDiff = diff;
            name = interval.name;
        }
    }
    
    return std::string(name);
}

// Get display name for scale
inline std::string getScaleName(float normalized) {
    int index = static_cast<int>(normalized * 9.0f + 0.5f);
    index = std::max(0, std::min(9, index));
    return std::string(SCALE_NAMES[index]);
}

// Parameter indices for IntelligentHarmonizer
enum Parameters {
    kInterval = 0,    // Pitch interval (-24 to +24 semitones)
    kKey = 1,         // Root key (C to B)
    kScale = 2,       // Scale type (0-9)
    kVoices = 3,      // Number of voices (not used currently)
    kSpread = 4,      // Voice spread (not used currently)
    kQuality = 5,     // Quality mode (0 = low latency, 1 = high quality)
    kFormant = 6,     // Formant shift
    kMix = 7          // Dry/Wet mix
};

// Mix presets for quick access
static const std::map<std::string, float> MIX_PRESETS = {
    {"Dry", 0.0f},
    {"25%", 0.25f},
    {"50%", 0.5f},
    {"75%", 0.75f},
    {"Wet", 1.0f}
};

} // namespace IntelligentHarmonizerParams
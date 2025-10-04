#pragma once
#include <vector>
#include <string>
#include <array>

namespace IntelligentHarmonizerChords {

// Chord structure - intervals in semitones from root
struct ChordPreset {
    const char* name;
    std::array<int, 3> intervals;  // Intervals for 3 voices
    const char* description;
};

// Comprehensive chord preset library - using proper voicings without doubling
static const std::vector<ChordPreset> CHORD_PRESETS = {
    // Basic Triads (drop-2 voicings to avoid doubling root)
    {"Major",      {4, 7, 12},   "Major triad"},        // 3rd, 5th, octave
    {"Minor",      {3, 7, 12},   "Minor triad"},        // b3rd, 5th, octave
    {"Sus2",       {2, 7, 12},   "Suspended 2nd"},      // 2nd, 5th, octave
    {"Sus4",       {5, 7, 12},   "Suspended 4th"},      // 4th, 5th, octave
    {"Dim",        {3, 6, 12},   "Diminished"},         // b3rd, b5th, octave
    {"Aug",        {4, 8, 12},   "Augmented"},          // 3rd, #5th, octave
    
    // 7th Chords (drop voicings - 3rd, 7th, octave+3rd for smooth voice leading)
    {"Maj7",       {4, 11, 16},  "Major 7th"},          // 3rd, maj7, octave+3rd
    {"Min7",       {3, 10, 15},  "Minor 7th"},          // b3rd, b7, octave+b3rd
    {"Dom7",       {4, 10, 16},  "Dominant 7th"},       // 3rd, b7, octave+3rd
    {"Min7b5",     {3, 9, 15},   "Half diminished"},    // b3rd, b7-1, octave+b3rd
    {"Dim7",       {3, 9, 18},   "Diminished 7th"},     // b3rd, dim7, octave+b5
    
    // Extended/Jazz
    {"6th",        {4, 9, 12},   "Major 6th"},          // 3rd, 6th, octave
    {"Min6",       {3, 9, 12},   "Minor 6th"},          // b3rd, 6th, octave
    {"Add9",       {4, 7, 14},   "Add 9"},              // 3rd, 5th, 9th
    {"MinAdd9",    {3, 7, 14},   "Minor add 9"},        // b3rd, 5th, 9th
    {"Maj9",       {4, 11, 14},  "Major 9th"},          // 3rd, maj7, 9th
    
    // Power/Rock
    {"5th",        {7, 12, 19},  "Power chord"},        // 5th, octave, octave+5th
    {"4th",        {5, 12, 17},  "4th power"},          // 4th, octave, octave+4th
    {"Oct",        {12, 24, -12},"Octaves"},            // +oct, +2oct, -oct
    {"Unison",     {0, 0, 0},    "Unison/mono"},
    
    // Special Voicings
    {"Wide",       {7, 16, 19},  "Wide voicing"},       // 5th, oct+3rd, oct+5th
    {"Shell",      {3, 10, 12},  "Shell voicing"},      // b3rd, b7, octave
    {"Quartal",    {5, 10, 15},  "Quartal harmony"},    // 4th, 4th+4th, 4th+4th+4th
    {"Quintal",    {7, 14, 21},  "Quintal harmony"},    // 5th, 5th+5th, 5th+5th+5th
    
    // Pop/Modern
    {"Pop",        {4, 12, 16},  "Pop voicing"},        // 3rd, octave, oct+3rd
    {"RnB",        {3, 11, 15},  "R&B voicing"},        // b3rd, maj7, oct+b3rd
    {"Neo",        {2, 11, 14},  "Neo-soul"},           // 2nd, maj7, 9th
    {"Dream",      {5, 9, 16},   "Dreamy"},             // 4th, 6th, oct+3rd
    
    // Custom/Creative
    {"Mystic",     {6, 11, 15},  "Mystical"},           // tritone, maj7, oct+b3rd
    {"Dark",       {1, 6, 13},   "Dark"},               // b2, tritone, b9
    {"Bright",     {4, 16, 19},  "Bright"},             // 3rd, oct+3rd, oct+5th
    {"Ambient",    {7, 15, 19},  "Ambient"}             // 5th, oct+b3rd, oct+5th
};

// Key root notes
static const std::vector<const char*> KEY_NAMES = {
    "C", "C#", "D", "D#", "E", "F", 
    "F#", "G", "G#", "A", "A#", "B"
};

// Scale definitions (for scale quantization)
static const std::vector<std::vector<int>> SCALES = {
    {0, 2, 4, 5, 7, 9, 11},           // Major
    {0, 2, 3, 5, 7, 8, 10},           // Natural Minor
    {0, 2, 3, 5, 7, 8, 11},           // Harmonic Minor
    {0, 2, 3, 5, 7, 9, 11},           // Melodic Minor
    {0, 2, 3, 5, 7, 9, 10},           // Dorian
    {0, 1, 3, 5, 7, 8, 10},           // Phrygian
    {0, 2, 4, 6, 7, 9, 11},           // Lydian
    {0, 2, 4, 5, 7, 9, 10},           // Mixolydian
    {0, 1, 3, 5, 6, 8, 10},           // Locrian
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11} // Chromatic
};

static const std::vector<const char*> SCALE_NAMES = {
    "Major",
    "Natural Minor",
    "Harmonic Minor",
    "Melodic Minor",
    "Dorian",
    "Phrygian",
    "Lydian",
    "Mixolydian",
    "Locrian",
    "Chromatic"
};

// Helper functions
inline int getChordIndex(float normalized) {
    int index = static_cast<int>(normalized * (CHORD_PRESETS.size() - 1) + 0.5f);
    return std::max(0, std::min((int)CHORD_PRESETS.size() - 1, index));
}

inline std::string getChordName(float normalized) {
    return CHORD_PRESETS[getChordIndex(normalized)].name;
}

inline std::array<int, 3> getChordIntervals(float normalized) {
    return CHORD_PRESETS[getChordIndex(normalized)].intervals;
}

inline int getKeyIndex(float normalized) {
    int index = static_cast<int>(normalized * 11.0f + 0.5f);
    return std::max(0, std::min(11, index));
}

inline std::string getKeyName(float normalized) {
    return KEY_NAMES[getKeyIndex(normalized)];
}

inline int getScaleIndex(float normalized) {
    int index = static_cast<int>(normalized * 9.0f + 0.5f);
    return std::max(0, std::min(9, index));
}

inline std::string getScaleName(float normalized) {
    return SCALE_NAMES[getScaleIndex(normalized)];
}

// Quantize to scale
inline int quantizeToScale(int semitones, int scaleIndex, int keyRoot) {
    if (scaleIndex < 0 || scaleIndex >= SCALES.size() || scaleIndex == 9) {
        return semitones; // Chromatic or invalid
    }
    
    const auto& scale = SCALES[scaleIndex];
    int octave = semitones / 12;
    int chroma = ((semitones % 12) + 12) % 12;
    
    // Find closest scale note
    int minDist = 12;
    int closest = chroma;
    for (int note : scale) {
        int adjusted = (note + keyRoot) % 12;
        int dist = std::abs(chroma - adjusted);
        if (dist < minDist) {
            minDist = dist;
            closest = adjusted;
        }
    }
    
    return octave * 12 + closest;
}

// Voice count display
inline std::string getVoiceCountDisplay(float normalized) {
    if (normalized < 0.33f) return "1 Voice";
    else if (normalized < 0.66f) return "2 Voices";
    else return "3 Voices";
}

inline int getVoiceCount(float normalized) {
    if (normalized < 0.33f) return 1;
    else if (normalized < 0.66f) return 2;
    else return 3;
}

// Quality mode display
inline std::string getQualityDisplay(float normalized) {
    return normalized < 0.5f ? "Low Latency" : "High Quality";
}

// Formant display (-100% to +100%)
inline std::string getFormantDisplay(float normalized) {
    int percent = static_cast<int>((normalized - 0.5f) * 200.0f);
    if (percent > 0) return "+" + std::to_string(percent) + "%";
    else if (percent < 0) return std::to_string(percent) + "%";
    else return "0%";
}

// Volume display (0-100%)
inline std::string getVolumeDisplay(float normalized) {
    return std::to_string(static_cast<int>(normalized * 100.0f)) + "%";
}

// Humanize display (0-100%)
inline std::string getHumanizeDisplay(float normalized) {
    return std::to_string(static_cast<int>(normalized * 100.0f)) + "%";
}

// Width display (0-100%)
inline std::string getWidthDisplay(float normalized) {
    return std::to_string(static_cast<int>(normalized * 100.0f)) + "%";
}

// Transpose display (-2 to +2 octaves)
inline std::string getTransposeDisplay(float normalized) {
    int octaves = static_cast<int>((normalized - 0.5f) * 4.0f + 0.5f);
    if (octaves > 0) return "+" + std::to_string(octaves) + " Oct";
    else if (octaves < 0) return std::to_string(octaves) + " Oct";
    else return "0";
}

} // namespace IntelligentHarmonizerChords
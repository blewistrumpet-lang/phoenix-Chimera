// PitchEnginePresets.h
// Musical presets for PitchShifter and IntelligentHarmonizer
// After parameter mapping improvements

#pragma once
#include <vector>
#include <string>
#include <map>

namespace PitchPresets {

// ====================
// PitchShifter Presets (ID: 31)
// ====================
// With new mapping: 0.5 = unison, 0.0 = -24 semitones, 1.0 = +24 semitones

struct PitchShifterPreset {
    std::string name;
    std::string description;
    std::map<int, float> parameters;
};

const std::vector<PitchShifterPreset> pitchShifterPresets = {
    // Basic Intervals
    {
        "Unison",
        "No pitch change - thickening only",
        {{0, 0.5f}, {1, 0.5f}, {2, 0.5f}}  // Pitch=0, Formant=normal, Mix=50%
    },
    {
        "Octave Down",
        "One octave below (-12 semitones)",
        {{0, 0.25f}, {1, 0.5f}, {2, 0.7f}}  // Pitch=-12st, Formant=normal, Mix=70%
    },
    {
        "Octave Up", 
        "One octave above (+12 semitones)",
        {{0, 0.75f}, {1, 0.5f}, {2, 0.7f}}  // Pitch=+12st, Formant=normal, Mix=70%
    },
    {
        "Fifth Down",
        "Perfect fifth below (-7 semitones)",
        {{0, 0.354f}, {1, 0.5f}, {2, 0.6f}}  // Pitch=-7st, Formant=normal, Mix=60%
    },
    {
        "Fifth Up",
        "Perfect fifth above (+7 semitones) - Power chord",
        {{0, 0.646f}, {1, 0.5f}, {2, 0.6f}}  // Pitch=+7st, Formant=normal, Mix=60%
    },
    {
        "Fourth Down",
        "Perfect fourth below (-5 semitones)",
        {{0, 0.396f}, {1, 0.5f}, {2, 0.5f}}  // Pitch=-5st, Formant=normal, Mix=50%
    },
    {
        "Fourth Up",
        "Perfect fourth above (+5 semitones)",
        {{0, 0.604f}, {1, 0.5f}, {2, 0.5f}}  // Pitch=+5st, Formant=normal, Mix=50%
    },
    {
        "Major Third Up",
        "Major third above (+4 semitones)",
        {{0, 0.583f}, {1, 0.5f}, {2, 0.5f}}  // Pitch=+4st, Formant=normal, Mix=50%
    },
    {
        "Minor Third Up",
        "Minor third above (+3 semitones)",
        {{0, 0.563f}, {1, 0.5f}, {2, 0.5f}}  // Pitch=+3st, Formant=normal, Mix=50%
    },
    
    // Creative Presets
    {
        "Subtle Detune",
        "Slight detuning for thickness (+25 cents)",
        {{0, 0.505f}, {1, 0.5f}, {2, 0.3f}}  // Pitch=+0.25st, Mix=30%
    },
    {
        "Wide Detune", 
        "Chorus-like detuning (+50 cents)",
        {{0, 0.510f}, {1, 0.5f}, {2, 0.4f}}  // Pitch=+0.5st, Mix=40%
    },
    {
        "Monster Voice",
        "Deep pitched voice with formant shift",
        {{0, 0.25f}, {1, 0.7f}, {2, 0.8f}}  // Pitch=-12st, Formant=high, Mix=80%
    },
    {
        "Chipmunk Voice",
        "High pitched with formant preservation",
        {{0, 0.75f}, {1, 0.3f}, {2, 0.8f}}  // Pitch=+12st, Formant=low, Mix=80%
    },
    {
        "Gender Change Male",
        "Voice masculinization",
        {{0, 0.458f}, {1, 0.65f}, {2, 1.0f}}  // Pitch=-2st, Formant=high, Mix=100%
    },
    {
        "Gender Change Female",
        "Voice feminization", 
        {{0, 0.542f}, {1, 0.35f}, {2, 1.0f}}  // Pitch=+2st, Formant=low, Mix=100%
    },
    {
        "Two Octaves Down",
        "Sub-bass generation (-24 semitones)",
        {{0, 0.0f}, {1, 0.5f}, {2, 0.5f}}  // Pitch=-24st, Mix=50%
    },
    {
        "Two Octaves Up",
        "Extreme high pitch (+24 semitones)",
        {{0, 1.0f}, {1, 0.5f}, {2, 0.5f}}  // Pitch=+24st, Mix=50%
    }
};

// ====================
// IntelligentHarmonizer Presets (ID: 33)
// ====================
// With new discrete interval mapping

struct HarmonizerPreset {
    std::string name;
    std::string description;
    std::map<int, float> parameters;
};

const std::vector<HarmonizerPreset> harmonizerPresets = {
    // Basic Harmonies
    {
        "Octave Harmony",
        "Adds octave above",
        {{0, 0.87f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {7, 0.6f}}
        // Interval=octave up, Key=C, Scale=Major, Voices=1, Mix=60%
    },
    {
        "Fifth Harmony",
        "Adds perfect fifth (power chord)",
        {{0, 0.79f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {7, 0.6f}}
        // Interval=5th up, Key=C, Scale=Major, Voices=1, Mix=60%
    },
    {
        "Major Third Harmony",
        "Adds major third for major chords",
        {{0, 0.62f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {7, 0.5f}}
        // Interval=maj3rd up, Key=C, Scale=Major, Voices=1, Mix=50%
    },
    {
        "Minor Third Harmony",
        "Adds minor third for minor chords",
        {{0, 0.54f}, {1, 0.0f}, {2, 0.1f}, {3, 0.0f}, {7, 0.5f}}
        // Interval=min3rd up, Key=C, Scale=Minor, Voices=1, Mix=50%
    },
    
    // Multi-Voice Harmonies
    {
        "Major Triad",
        "Generates major chord (root, 3rd, 5th)",
        {{0, 0.62f}, {1, 0.0f}, {2, 0.0f}, {3, 0.5f}, {7, 0.6f}}
        // Interval=maj3rd, Voices=2 (adds 5th too), Mix=60%
    },
    {
        "Minor Triad",
        "Generates minor chord",
        {{0, 0.54f}, {1, 0.0f}, {2, 0.1f}, {3, 0.5f}, {7, 0.6f}}
        // Interval=min3rd, Scale=Minor, Voices=2, Mix=60%
    },
    {
        "Vocal Doubler",
        "Subtle doubling with humanization",
        {{0, 0.46f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {5, 0.7f}, {7, 0.3f}}
        // Interval=unison, Humanize=high, Mix=30%
    },
    {
        "Thick Harmony",
        "Multiple voices with spread",
        {{0, 0.79f}, {1, 0.0f}, {2, 0.0f}, {3, 1.0f}, {4, 0.6f}, {7, 0.7f}}
        // Interval=5th, Voices=4, Spread=60%, Mix=70%
    },
    
    // Scale-Based Harmonies
    {
        "Major Scale Auto",
        "Intelligent harmonization in major key",
        {{0, 0.62f}, {1, 0.0f}, {2, 0.0f}, {3, 0.3f}, {7, 0.5f}}
        // Major scale quantization
    },
    {
        "Minor Scale Auto",
        "Intelligent harmonization in minor key",
        {{0, 0.54f}, {1, 0.0f}, {2, 0.1f}, {3, 0.3f}, {7, 0.5f}}
        // Minor scale quantization
    },
    {
        "Pentatonic Harmony",
        "Harmonize using pentatonic scale",
        {{0, 0.62f}, {1, 0.0f}, {2, 0.6f}, {3, 0.3f}, {7, 0.5f}}
        // Pentatonic scale
    },
    {
        "Blues Harmony",
        "Blues scale harmonization",
        {{0, 0.54f}, {1, 0.0f}, {2, 0.8f}, {3, 0.3f}, {7, 0.5f}}
        // Blues scale
    },
    
    // Creative Presets
    {
        "Octave + Fifth",
        "Power chord plus octave",
        {{0, 0.95f}, {1, 0.0f}, {2, 0.0f}, {3, 0.5f}, {7, 0.6f}}
        // Octave+5th interval
    },
    {
        "Nashville Tuning",
        "Simulates Nashville tuning (octave up on low strings)",
        {{0, 0.87f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {6, 0.3f}, {7, 0.5f}}
        // Octave with formant shift
    },
    {
        "Choir Effect",
        "Multiple harmonized voices",
        {{0, 0.62f}, {1, 0.0f}, {2, 0.0f}, {3, 1.0f}, {4, 0.8f}, {5, 0.6f}, {7, 0.4f}}
        // Many voices, high spread and humanization
    },
    {
        "Bass Harmony",
        "Adds bass note below",
        {{0, 0.04f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {7, 0.5f}}
        // Octave down
    },
    {
        "Shimmer",
        "Ethereal high harmonies",
        {{0, 0.87f}, {1, 0.0f}, {2, 0.0f}, {3, 0.3f}, {4, 0.7f}, {7, 0.3f}}
        // Octave up with spread, low mix
    }
};

// Helper function to apply preset
template<typename EngineType>
void applyPreset(EngineType* engine, const std::map<int, float>& parameters) {
    engine->updateParameters(parameters);
}

// Get preset by name
template<typename PresetType>
const PresetType* getPresetByName(const std::vector<PresetType>& presets, const std::string& name) {
    for (const auto& preset : presets) {
        if (preset.name == name) {
            return &preset;
        }
    }
    return nullptr;
}

} // namespace PitchPresets
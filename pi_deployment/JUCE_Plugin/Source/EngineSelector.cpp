#include "EngineSelector.h"

// Include the actual engine IDs from your project
// These values come from EngineIDs.h in the main codebase
namespace EngineIDs {
    constexpr int ENGINE_NONE = 0;

    // Dynamics (6 engines)
    constexpr int ENGINE_CLASSIC_COMPRESSOR = 1;
    constexpr int ENGINE_VINTAGE_OPTO_COMPRESSOR = 2;
    constexpr int ENGINE_TRANSIENT_SHAPER = 3;
    constexpr int ENGINE_NOISE_GATE = 4;
    constexpr int ENGINE_MASTERING_LIMITER = 5;
    constexpr int ENGINE_DYNAMIC_EQ = 6;

    // Filters (8 engines)
    constexpr int ENGINE_PARAMETRIC_EQ = 7;
    constexpr int ENGINE_VINTAGE_CONSOLE_EQ = 8;
    constexpr int ENGINE_LADDER_FILTER = 9;
    constexpr int ENGINE_STATE_VARIABLE_FILTER = 10;
    constexpr int ENGINE_FORMANT_FILTER = 11;
    constexpr int ENGINE_ENVELOPE_FILTER = 12;
    constexpr int ENGINE_COMB_RESONATOR = 13;
    constexpr int ENGINE_VOCAL_FORMANT = 14;

    // Distortion (8 engines)
    constexpr int ENGINE_VINTAGE_TUBE = 15;
    constexpr int ENGINE_WAVE_FOLDER = 16;
    constexpr int ENGINE_HARMONIC_EXCITER_PLATINUM = 17;
    constexpr int ENGINE_BIT_CRUSHER = 18;
    constexpr int ENGINE_MULTIBAND_SATURATOR = 19;
    constexpr int ENGINE_MUFF_FUZZ = 20;
    constexpr int ENGINE_RODENT_DISTORTION = 21;
    constexpr int ENGINE_K_STYLE_OVERDRIVE = 22;

    // Modulation (11 engines)
    constexpr int ENGINE_DIGITAL_CHORUS = 23;
    constexpr int ENGINE_ANALOG_PHASER = 24;
    constexpr int ENGINE_RING_MODULATOR = 25;
    constexpr int ENGINE_FREQUENCY_SHIFTER = 26;
    constexpr int ENGINE_HARMONIC_TREMOLO = 27;
    constexpr int ENGINE_CLASSIC_TREMOLO = 28;
    constexpr int ENGINE_ROTARY_SPEAKER = 29;
    constexpr int ENGINE_SIMPLE_PITCH_SHIFT = 30;
    constexpr int ENGINE_DETUNE_DOUBLER = 31;
    constexpr int ENGINE_INTELLIGENT_HARMONIZER = 32;
    constexpr int ENGINE_RESONANT_CHORUS = 33;

    // Reverb & Delay (10 engines)
    constexpr int ENGINE_TAPE_ECHO = 34;
    constexpr int ENGINE_DIGITAL_DELAY = 35;
    constexpr int ENGINE_MAGNETIC_DRUM_ECHO = 36;
    constexpr int ENGINE_BUCKET_BRIGADE_DELAY = 37;
    constexpr int ENGINE_BUFFER_REPEAT = 38;
    constexpr int ENGINE_PLATE_REVERB = 39;
    constexpr int ENGINE_SPRING_REVERB = 40;
    constexpr int ENGINE_CONVOLUTION_REVERB = 41;
    constexpr int ENGINE_SHIMMER_REVERB = 42;
    constexpr int ENGINE_GATED_REVERB = 43;

    // Spatial & Special (9 engines)
    constexpr int ENGINE_STEREO_WIDENER = 44;
    constexpr int ENGINE_STEREO_IMAGER = 45;
    constexpr int ENGINE_DIMENSION_EXPANDER = 46;
    constexpr int ENGINE_SPECTRAL_FREEZE = 47;
    constexpr int ENGINE_SPECTRAL_GATE = 48;
    constexpr int ENGINE_PHASED_VOCODER = 49;
    constexpr int ENGINE_GRANULAR_CLOUD = 50;
    constexpr int ENGINE_CHAOS_GENERATOR = 51;
    constexpr int ENGINE_FEEDBACK_NETWORK = 52;

    // Utility (4 engines)
    constexpr int ENGINE_MID_SIDE_PROCESSOR = 53;
    constexpr int ENGINE_GAIN_UTILITY = 54;
    constexpr int ENGINE_MONO_MAKER = 55;
    constexpr int ENGINE_PHASE_ALIGN_PLATINUM = 56;
}

EngineSelector::EngineSelector() {
    initializeEngineDatabase();
    reset();
}

void EngineSelector::initializeEngineDatabase() {
    using namespace EngineIDs;

    // Clear any existing data
    allEngines.clear();
    enginesByCategory.clear();

    // Add NONE category
    enginesByCategory[Category::NONE] = {};

    // Dynamics engines
    allEngines.push_back({ENGINE_CLASSIC_COMPRESSOR, "Classic Compressor", Category::DYNAMICS});
    allEngines.push_back({ENGINE_VINTAGE_OPTO_COMPRESSOR, "Opto Compressor", Category::DYNAMICS});
    allEngines.push_back({ENGINE_TRANSIENT_SHAPER, "Transient Shaper", Category::DYNAMICS});
    allEngines.push_back({ENGINE_NOISE_GATE, "Noise Gate", Category::DYNAMICS});
    allEngines.push_back({ENGINE_MASTERING_LIMITER, "Master Limiter", Category::DYNAMICS});
    allEngines.push_back({ENGINE_DYNAMIC_EQ, "Dynamic EQ", Category::DYNAMICS});

    // Filter engines
    allEngines.push_back({ENGINE_PARAMETRIC_EQ, "Parametric EQ", Category::FILTERS});
    allEngines.push_back({ENGINE_VINTAGE_CONSOLE_EQ, "Console EQ", Category::FILTERS});
    allEngines.push_back({ENGINE_LADDER_FILTER, "Ladder Filter", Category::FILTERS});
    allEngines.push_back({ENGINE_STATE_VARIABLE_FILTER, "State Variable", Category::FILTERS});
    allEngines.push_back({ENGINE_FORMANT_FILTER, "Formant Filter", Category::FILTERS});
    allEngines.push_back({ENGINE_ENVELOPE_FILTER, "Envelope Filter", Category::FILTERS});
    allEngines.push_back({ENGINE_COMB_RESONATOR, "Comb Resonator", Category::FILTERS});
    allEngines.push_back({ENGINE_VOCAL_FORMANT, "Vocal Formant", Category::FILTERS});

    // Distortion engines
    allEngines.push_back({ENGINE_VINTAGE_TUBE, "Vintage Tube", Category::DISTORTION});
    allEngines.push_back({ENGINE_WAVE_FOLDER, "Wave Folder", Category::DISTORTION});
    allEngines.push_back({ENGINE_HARMONIC_EXCITER_PLATINUM, "Harmonic Exciter", Category::DISTORTION});
    allEngines.push_back({ENGINE_BIT_CRUSHER, "Bit Crusher", Category::DISTORTION});
    allEngines.push_back({ENGINE_MULTIBAND_SATURATOR, "Multiband Sat", Category::DISTORTION});
    allEngines.push_back({ENGINE_MUFF_FUZZ, "Muff Fuzz", Category::DISTORTION});
    allEngines.push_back({ENGINE_RODENT_DISTORTION, "Rodent Dist", Category::DISTORTION});
    allEngines.push_back({ENGINE_K_STYLE_OVERDRIVE, "K-Style OD", Category::DISTORTION});

    // Modulation engines
    allEngines.push_back({ENGINE_DIGITAL_CHORUS, "Digital Chorus", Category::MODULATION});
    allEngines.push_back({ENGINE_ANALOG_PHASER, "Analog Phaser", Category::MODULATION});
    allEngines.push_back({ENGINE_RING_MODULATOR, "Ring Modulator", Category::MODULATION});
    allEngines.push_back({ENGINE_FREQUENCY_SHIFTER, "Freq Shifter", Category::MODULATION});
    allEngines.push_back({ENGINE_HARMONIC_TREMOLO, "Harmonic Trem", Category::MODULATION});
    allEngines.push_back({ENGINE_CLASSIC_TREMOLO, "Classic Trem", Category::MODULATION});
    allEngines.push_back({ENGINE_ROTARY_SPEAKER, "Rotary Speaker", Category::MODULATION});
    allEngines.push_back({ENGINE_SIMPLE_PITCH_SHIFT, "Pitch Shift", Category::MODULATION});
    allEngines.push_back({ENGINE_DETUNE_DOUBLER, "Detune Doubler", Category::MODULATION});
    allEngines.push_back({ENGINE_INTELLIGENT_HARMONIZER, "Harmonizer", Category::MODULATION});
    allEngines.push_back({ENGINE_RESONANT_CHORUS, "Resonant Chorus", Category::MODULATION});

    // Reverb & Delay engines
    allEngines.push_back({ENGINE_TAPE_ECHO, "Tape Echo", Category::REVERB});
    allEngines.push_back({ENGINE_DIGITAL_DELAY, "Digital Delay", Category::REVERB});
    allEngines.push_back({ENGINE_MAGNETIC_DRUM_ECHO, "Drum Echo", Category::REVERB});
    allEngines.push_back({ENGINE_BUCKET_BRIGADE_DELAY, "Bucket Brigade", Category::REVERB});
    allEngines.push_back({ENGINE_BUFFER_REPEAT, "Buffer Repeat", Category::REVERB});
    allEngines.push_back({ENGINE_PLATE_REVERB, "Plate Reverb", Category::REVERB});
    allEngines.push_back({ENGINE_SPRING_REVERB, "Spring Reverb", Category::REVERB});
    allEngines.push_back({ENGINE_CONVOLUTION_REVERB, "Convolution", Category::REVERB});
    allEngines.push_back({ENGINE_SHIMMER_REVERB, "Shimmer Reverb", Category::REVERB});
    allEngines.push_back({ENGINE_GATED_REVERB, "Gated Reverb", Category::REVERB});

    // Spatial & Special engines
    allEngines.push_back({ENGINE_STEREO_WIDENER, "Stereo Widener", Category::SPATIAL});
    allEngines.push_back({ENGINE_STEREO_IMAGER, "Stereo Imager", Category::SPATIAL});
    allEngines.push_back({ENGINE_DIMENSION_EXPANDER, "Dimension Exp", Category::SPATIAL});
    allEngines.push_back({ENGINE_SPECTRAL_FREEZE, "Spectral Freeze", Category::SPATIAL});
    allEngines.push_back({ENGINE_SPECTRAL_GATE, "Spectral Gate", Category::SPATIAL});
    allEngines.push_back({ENGINE_PHASED_VOCODER, "Phased Vocoder", Category::SPATIAL});
    allEngines.push_back({ENGINE_GRANULAR_CLOUD, "Granular Cloud", Category::SPATIAL});
    allEngines.push_back({ENGINE_CHAOS_GENERATOR, "Chaos Gen", Category::SPATIAL});
    allEngines.push_back({ENGINE_FEEDBACK_NETWORK, "Feedback Net", Category::SPATIAL});

    // Utility engines
    allEngines.push_back({ENGINE_MID_SIDE_PROCESSOR, "M/S Processor", Category::UTILITY});
    allEngines.push_back({ENGINE_GAIN_UTILITY, "Gain Utility", Category::UTILITY});
    allEngines.push_back({ENGINE_MONO_MAKER, "Mono Maker", Category::UTILITY});
    allEngines.push_back({ENGINE_PHASE_ALIGN_PLATINUM, "Phase Align", Category::UTILITY});

    // Build category index
    for (size_t i = 0; i < allEngines.size(); ++i) {
        enginesByCategory[allEngines[i].category].push_back(static_cast<int>(i));
    }

    DBG("EngineSelector: Initialized with " << allEngines.size() << " engines in "
        << enginesByCategory.size() << " categories");
}

int EngineSelector::getCategoryEngineCount() const {
    auto engines = getEnginesInCategory(currentCategory);
    return static_cast<int>(engines.size());
}

juce::String EngineSelector::getSlotEngineName(int slot) const {
    if (slot < 0 || slot >= 6) return "Invalid";

    int engineID = slotEngines[slot];
    if (engineID == 0) return "None";

    // Find engine in database
    for (const auto& engine : allEngines) {
        if (engine.id == engineID) {
            return engine.name;
        }
    }

    return "Unknown";
}
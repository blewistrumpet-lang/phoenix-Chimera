#pragma once

/**
 * EngineTypes.h - Single source of truth for all engine type definitions
 * 
 * This file defines all DSP engine types used throughout Project Chimera.
 * All components (plugin, presets, AI system) must use these definitions.
 * 
 * Version: 1.0
 * Last Updated: 2025-08-02
 */

// Engine Type Constants - DO NOT MODIFY ORDER (breaks preset compatibility)
// When adding new engines, append to the end and increment ENGINE_COUNT

// Effects - Classic & Vintage
#define ENGINE_VINTAGE_TUBE             0   // Vintage Tube Preamp
#define ENGINE_TAPE_ECHO                1   // Tape Echo/Delay
#define ENGINE_SHIMMER_REVERB           2   // Shimmer Reverb
#define ENGINE_PLATE_REVERB             3   // Plate Reverb
#define ENGINE_CONVOLUTION_REVERB       4   // Convolution Reverb
#define ENGINE_SPRING_REVERB            5   // Spring Reverb
#define ENGINE_OPTO_COMPRESSOR          6   // Vintage Opto Compressor
#define ENGINE_VCA_COMPRESSOR           7   // VCA Compressor
#define ENGINE_MAGNETIC_DRUM_ECHO       8   // Magnetic Drum Echo
#define ENGINE_BUCKET_BRIGADE_DELAY     9   // Bucket Brigade Delay

// Modulation Effects
// #define ENGINE_ANALOG_CHORUS            10  // Analog Chorus - NOT IMPLEMENTED
#define ENGINE_DIGITAL_CHORUS           11  // Digital/Stereo Chorus
#define ENGINE_ANALOG_PHASER            12  // Analog Phaser
// #define ENGINE_DIGITAL_PHASER           13  // Digital Phaser - NOT IMPLEMENTED
#define ENGINE_PITCH_SHIFTER            14  // Pitch Shifter
#define ENGINE_RING_MODULATOR           15  // Ring Modulator
#define ENGINE_GRANULAR_CLOUD           16  // Granular Cloud Processor
#define ENGINE_VOCAL_FORMANT            17  // Vocal Formant Filter
#define ENGINE_DIMENSION_EXPANDER       18  // Dimension Expander
#define ENGINE_FREQUENCY_SHIFTER        19  // Frequency Shifter
#define ENGINE_TRANSIENT_SHAPER         20  // Transient Shaper
#define ENGINE_HARMONIC_TREMOLO         21  // Harmonic Tremolo
#define ENGINE_CLASSIC_TREMOLO          22  // Classic Tremolo

// Filters & EQ
#define ENGINE_COMB_RESONATOR           23  // Comb Resonator
#define ENGINE_ROTARY_SPEAKER           24  // Rotary Speaker Simulator
#define ENGINE_MID_SIDE_PROCESSOR       25  // Mid-Side Processor
#define ENGINE_VINTAGE_CONSOLE_EQ       26  // Vintage Console EQ
#define ENGINE_PARAMETRIC_EQ            27  // Parametric EQ
#define ENGINE_LADDER_FILTER            28  // Ladder Filter
#define ENGINE_STATE_VARIABLE_FILTER    29  // State Variable Filter
#define ENGINE_FORMANT_FILTER           30  // Formant Filter

// Distortion & Saturation
#define ENGINE_WAVE_FOLDER              31  // Wave Folder
#define ENGINE_HARMONIC_EXCITER         32  // Harmonic Exciter
#define ENGINE_BIT_CRUSHER              33  // Bit Crusher
#define ENGINE_MULTIBAND_SATURATOR      34  // Multiband Saturator
#define ENGINE_MUFF_FUZZ                35  // Muff-style Fuzz
#define ENGINE_RODENT_DISTORTION        36  // Rodent-style Distortion
// #define ENGINE_TUBE_SCREAMER            37  // Tube Screamer - NOT IMPLEMENTED
#define ENGINE_K_STYLE                  38  // K-style Overdrive

// Spatial & Time Effects
#define ENGINE_SPECTRAL_FREEZE          39  // Spectral Freeze
#define ENGINE_BUFFER_REPEAT            40  // Buffer Repeat/Glitch
#define ENGINE_CHAOS_GENERATOR          41  // Chaos Generator
#define ENGINE_INTELLIGENT_HARMONIZER   42  // Intelligent Harmonizer
#define ENGINE_GATED_REVERB             43  // Gated Reverb
#define ENGINE_DETUNE_DOUBLER           44  // Detune Doubler
#define ENGINE_PHASED_VOCODER           45  // Phased Vocoder
#define ENGINE_SPECTRAL_GATE            46  // Spectral Gate
#define ENGINE_NOISE_GATE               47  // Noise Gate
#define ENGINE_ENVELOPE_FILTER          48  // Envelope Filter
#define ENGINE_FEEDBACK_NETWORK         49  // Feedback Network
#define ENGINE_MASTERING_LIMITER        50  // Mastering Limiter

// Additional Engines
#define ENGINE_STEREO_WIDENER           51  // Stereo Widener
#define ENGINE_RESONANT_CHORUS          52  // Resonant Chorus
#define ENGINE_DIGITAL_DELAY            53  // Digital Delay
#define ENGINE_DYNAMIC_EQ               54  // Dynamic EQ
#define ENGINE_STEREO_IMAGER            55  // Stereo Imager

// Engine Count - UPDATE when adding new engines
#define ENGINE_COUNT                    53  // 3 engines commented out (10, 13, 37)

// Engine Categories for UI organization
namespace EngineCategory {
    enum Type {
        VINTAGE_EFFECTS = 0,
        MODULATION,
        FILTERS_EQ,
        DISTORTION_SATURATION,
        SPATIAL_TIME,
        DYNAMICS,
        UTILITY
    };
}

// Helper function declarations
inline const char* getEngineTypeName(int engineType);
inline int getEngineCategory(int engineType);
inline bool isValidEngineType(int engineType);

// Implementation of helper functions
inline const char* getEngineTypeName(int engineType) {
    switch (engineType) {
        // Vintage Effects
        case ENGINE_VINTAGE_TUBE:           return "Vintage Tube";
        case ENGINE_TAPE_ECHO:              return "Tape Echo";
        case ENGINE_SHIMMER_REVERB:         return "Shimmer Reverb";
        case ENGINE_PLATE_REVERB:           return "Plate Reverb";
        case ENGINE_CONVOLUTION_REVERB:     return "Convolution Reverb";
        case ENGINE_SPRING_REVERB:          return "Spring Reverb";
        case ENGINE_OPTO_COMPRESSOR:        return "Opto Compressor";
        case ENGINE_VCA_COMPRESSOR:         return "VCA Compressor";
        case ENGINE_MAGNETIC_DRUM_ECHO:     return "Magnetic Drum Echo";
        case ENGINE_BUCKET_BRIGADE_DELAY:   return "Bucket Brigade Delay";
        
        // Modulation
        // case ENGINE_ANALOG_CHORUS:          return "Analog Chorus"; // NOT IMPLEMENTED
        case ENGINE_DIGITAL_CHORUS:         return "Digital Chorus";
        case ENGINE_ANALOG_PHASER:          return "Analog Phaser";
        // case ENGINE_DIGITAL_PHASER:         return "Digital Phaser"; // NOT IMPLEMENTED
        case ENGINE_PITCH_SHIFTER:          return "Pitch Shifter";
        case ENGINE_RING_MODULATOR:         return "Ring Modulator";
        case ENGINE_GRANULAR_CLOUD:         return "Granular Cloud";
        case ENGINE_VOCAL_FORMANT:          return "Vocal Formant Filter";
        case ENGINE_DIMENSION_EXPANDER:     return "Dimension Expander";
        case ENGINE_FREQUENCY_SHIFTER:      return "Frequency Shifter";
        case ENGINE_TRANSIENT_SHAPER:       return "Transient Shaper";
        case ENGINE_HARMONIC_TREMOLO:       return "Harmonic Tremolo";
        case ENGINE_CLASSIC_TREMOLO:        return "Classic Tremolo";
        
        // Filters & EQ
        case ENGINE_COMB_RESONATOR:         return "Comb Resonator";
        case ENGINE_ROTARY_SPEAKER:         return "Rotary Speaker";
        case ENGINE_MID_SIDE_PROCESSOR:     return "Mid-Side Processor";
        case ENGINE_VINTAGE_CONSOLE_EQ:     return "Vintage Console EQ";
        case ENGINE_PARAMETRIC_EQ:          return "Parametric EQ";
        case ENGINE_LADDER_FILTER:          return "Ladder Filter";
        case ENGINE_STATE_VARIABLE_FILTER:  return "State Variable Filter";
        case ENGINE_FORMANT_FILTER:         return "Formant Filter";
        
        // Distortion & Saturation
        case ENGINE_WAVE_FOLDER:            return "Wave Folder";
        case ENGINE_HARMONIC_EXCITER:       return "Harmonic Exciter";
        case ENGINE_BIT_CRUSHER:            return "Bit Crusher";
        case ENGINE_MULTIBAND_SATURATOR:    return "Multiband Saturator";
        case ENGINE_MUFF_FUZZ:              return "Muff Fuzz";
        case ENGINE_RODENT_DISTORTION:      return "Rodent Distortion";
        // case ENGINE_TUBE_SCREAMER:          return "Tube Screamer"; // NOT IMPLEMENTED
        case ENGINE_K_STYLE:                return "K-Style Overdrive";
        
        // Spatial & Time
        case ENGINE_SPECTRAL_FREEZE:        return "Spectral Freeze";
        case ENGINE_BUFFER_REPEAT:          return "Buffer Repeat";
        case ENGINE_CHAOS_GENERATOR:        return "Chaos Generator";
        case ENGINE_INTELLIGENT_HARMONIZER: return "Intelligent Harmonizer";
        case ENGINE_GATED_REVERB:           return "Gated Reverb";
        case ENGINE_DETUNE_DOUBLER:         return "Detune Doubler";
        case ENGINE_PHASED_VOCODER:         return "Phased Vocoder";
        case ENGINE_SPECTRAL_GATE:          return "Spectral Gate";
        case ENGINE_NOISE_GATE:             return "Noise Gate";
        case ENGINE_ENVELOPE_FILTER:        return "Envelope Filter";
        case ENGINE_FEEDBACK_NETWORK:       return "Feedback Network";
        case ENGINE_MASTERING_LIMITER:      return "Mastering Limiter";
        
        // Additional
        case ENGINE_STEREO_WIDENER:         return "Stereo Widener";
        case ENGINE_RESONANT_CHORUS:        return "Resonant Chorus";
        case ENGINE_DIGITAL_DELAY:          return "Digital Delay";
        case ENGINE_DYNAMIC_EQ:             return "Dynamic EQ";
        case ENGINE_STEREO_IMAGER:          return "Stereo Imager";
        
        default:                            return "Unknown Engine";
    }
}

inline int getEngineCategory(int engineType) {
    if (engineType >= ENGINE_VINTAGE_TUBE && engineType <= ENGINE_BUCKET_BRIGADE_DELAY)
        return EngineCategory::VINTAGE_EFFECTS;
    else if (engineType >= ENGINE_ANALOG_CHORUS && engineType <= ENGINE_CLASSIC_TREMOLO)
        return EngineCategory::MODULATION;
    else if (engineType >= ENGINE_COMB_RESONATOR && engineType <= ENGINE_FORMANT_FILTER)
        return EngineCategory::FILTERS_EQ;
    else if (engineType >= ENGINE_WAVE_FOLDER && engineType <= ENGINE_K_STYLE)
        return EngineCategory::DISTORTION_SATURATION;
    else if (engineType >= ENGINE_SPECTRAL_FREEZE && engineType <= ENGINE_MASTERING_LIMITER)
        return EngineCategory::SPATIAL_TIME;
    else if (engineType >= ENGINE_STEREO_WIDENER && engineType <= ENGINE_STEREO_IMAGER)
        return EngineCategory::UTILITY;
    else
        return -1; // Invalid category
}

inline bool isValidEngineType(int engineType) {
    return engineType >= 0 && engineType < ENGINE_COUNT;
}

// Legacy mappings for backward compatibility
// These map old enum names to new defines
#define ENGINE_CLASSIC_COMPRESSOR       ENGINE_VCA_COMPRESSOR
#define ENGINE_STEREO_CHORUS            ENGINE_DIGITAL_CHORUS
#define ENGINE_ANALOG_RING_MODULATOR    ENGINE_RING_MODULATOR
#define ENGINE_VOCAL_FORMANT_FILTER     ENGINE_VOCAL_FORMANT
#define ENGINE_VINTAGE_OPTO_COMPRESSOR  ENGINE_OPTO_COMPRESSOR
#define ENGINE_VINTAGE_TUBE_PREAMP      ENGINE_VINTAGE_TUBE

// Usage notes:
// 1. Always use the ENGINE_* defines, never raw numbers
// 2. When saving presets, store the engine type as an integer
// 3. Use getEngineTypeName() for UI display
// 4. Check validity with isValidEngineType() when loading presets
// 5. DO NOT reorder existing engines - it will break saved presets
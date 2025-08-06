#pragma once

/**
 * EngineTypes.h - Single source of truth for all engine type definitions
 * 
 * This file defines all DSP engine types used throughout Project Chimera.
 * All components (plugin, presets, AI system) must use these definitions.
 * 
 * Version: 1.1
 * Last Updated: 2025-08-06
 */

// Engine Type Constants - DO NOT MODIFY ORDER (breaks preset compatibility)
// When adding new engines, append to the end and increment ENGINE_COUNT

// Special value for "no engine selected"
#define ENGINE_NONE                     0   // No engine (passthrough)

// DYNAMICS & COMPRESSION (IDs 1-6)
#define ENGINE_OPTO_COMPRESSOR          1   // Vintage Opto Compressor
#define ENGINE_VCA_COMPRESSOR           2   // VCA/Classic Compressor  
#define ENGINE_TRANSIENT_SHAPER         3   // Transient Shaper
#define ENGINE_NOISE_GATE               4   // Noise Gate
#define ENGINE_MASTERING_LIMITER        5   // Mastering Limiter
#define ENGINE_DYNAMIC_EQ               6   // Dynamic EQ

// FILTERS & EQ (IDs 7-14)
#define ENGINE_PARAMETRIC_EQ            7   // Parametric EQ
#define ENGINE_VINTAGE_CONSOLE_EQ       8   // Vintage Console EQ
#define ENGINE_LADDER_FILTER            9   // Ladder Filter
#define ENGINE_STATE_VARIABLE_FILTER    10  // State Variable Filter
#define ENGINE_FORMANT_FILTER           11  // Formant Filter
#define ENGINE_ENVELOPE_FILTER          12  // Envelope Filter
#define ENGINE_COMB_RESONATOR           13  // Comb Resonator
#define ENGINE_VOCAL_FORMANT            14  // Vocal Formant Filter

// DISTORTION & SATURATION (IDs 15-22)
#define ENGINE_VINTAGE_TUBE             15  // Vintage Tube Preamp
#define ENGINE_WAVE_FOLDER              16  // Wave Folder
#define ENGINE_HARMONIC_EXCITER         17  // Harmonic Exciter
#define ENGINE_BIT_CRUSHER              18  // Bit Crusher
#define ENGINE_MULTIBAND_SATURATOR      19  // Multiband Saturator
#define ENGINE_MUFF_FUZZ                20  // Muff Fuzz
#define ENGINE_RODENT_DISTORTION        21  // Rodent Distortion
#define ENGINE_K_STYLE                  22  // K-Style Overdrive

// MODULATION EFFECTS (IDs 23-33)
#define ENGINE_DIGITAL_CHORUS           23  // Digital/Stereo Chorus
#define ENGINE_RESONANT_CHORUS          24  // Resonant Chorus
#define ENGINE_ANALOG_PHASER            25  // Analog Phaser
#define ENGINE_RING_MODULATOR           26  // Ring Modulator
#define ENGINE_FREQUENCY_SHIFTER        27  // Frequency Shifter
#define ENGINE_HARMONIC_TREMOLO         28  // Harmonic Tremolo
#define ENGINE_CLASSIC_TREMOLO          29  // Classic Tremolo
#define ENGINE_ROTARY_SPEAKER           30  // Rotary Speaker
#define ENGINE_PITCH_SHIFTER            31  // Pitch Shifter
#define ENGINE_DETUNE_DOUBLER           32  // Detune Doubler
#define ENGINE_INTELLIGENT_HARMONIZER   33  // Intelligent Harmonizer

// REVERB & DELAY (IDs 34-43)
#define ENGINE_TAPE_ECHO                34  // Tape Echo
#define ENGINE_DIGITAL_DELAY            35  // Digital Delay
#define ENGINE_MAGNETIC_DRUM_ECHO       36  // Magnetic Drum Echo
#define ENGINE_BUCKET_BRIGADE_DELAY     37  // Bucket Brigade Delay
#define ENGINE_BUFFER_REPEAT            38  // Buffer Repeat
#define ENGINE_PLATE_REVERB             39  // Plate Reverb
#define ENGINE_SPRING_REVERB            40  // Spring Reverb
#define ENGINE_CONVOLUTION_REVERB       41  // Convolution Reverb
#define ENGINE_SHIMMER_REVERB           42  // Shimmer Reverb
#define ENGINE_GATED_REVERB             43  // Gated Reverb

// SPATIAL & SPECIAL EFFECTS (IDs 44-52)
#define ENGINE_STEREO_WIDENER           44  // Stereo Widener
#define ENGINE_STEREO_IMAGER            45  // Stereo Imager
#define ENGINE_DIMENSION_EXPANDER       46  // Dimension Expander
#define ENGINE_SPECTRAL_FREEZE          47  // Spectral Freeze
#define ENGINE_SPECTRAL_GATE            48  // Spectral Gate
#define ENGINE_PHASED_VOCODER           49  // Phased Vocoder
#define ENGINE_GRANULAR_CLOUD           50  // Granular Cloud
#define ENGINE_CHAOS_GENERATOR          51  // Chaos Generator
#define ENGINE_FEEDBACK_NETWORK         52  // Feedback Network

// UTILITY (IDs 53-56)
#define ENGINE_MID_SIDE_PROCESSOR       53  // Mid-Side Processor
#define ENGINE_GAIN_UTILITY             54  // Gain Utility
#define ENGINE_MONO_MAKER               55  // Mono Maker
#define ENGINE_PHASE_ALIGN              56  // Phase Align

// Engine Count - UPDATE when adding new engines
#define ENGINE_COUNT                    57  // Total engines 1-56, plus ENGINE_NONE (0)

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
        // Special case
        case ENGINE_NONE:                   return "None";
        
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
        
        // Utility
        case ENGINE_GAIN_UTILITY:           return "Gain Utility";
        case ENGINE_MONO_MAKER:             return "Mono Maker";
        case ENGINE_PHASE_ALIGN:            return "Phase Align";
        
        default:                            return "Unknown Engine";
    }
}

inline int getEngineCategory(int engineType) {
    if (engineType == ENGINE_NONE)
        return -1; // No category for "None"
    else if (engineType >= ENGINE_VINTAGE_TUBE && engineType <= ENGINE_BUCKET_BRIGADE_DELAY)
        return EngineCategory::VINTAGE_EFFECTS;
    else if (engineType >= ENGINE_DIGITAL_CHORUS && engineType <= ENGINE_CLASSIC_TREMOLO)
        return EngineCategory::MODULATION;
    else if (engineType >= ENGINE_COMB_RESONATOR && engineType <= ENGINE_FORMANT_FILTER)
        return EngineCategory::FILTERS_EQ;
    else if (engineType >= ENGINE_WAVE_FOLDER && engineType <= ENGINE_K_STYLE)
        return EngineCategory::DISTORTION_SATURATION;
    else if (engineType >= ENGINE_SPECTRAL_FREEZE && engineType <= ENGINE_MASTERING_LIMITER)
        return EngineCategory::SPATIAL_TIME;
    else if ((engineType >= ENGINE_STEREO_WIDENER && engineType <= ENGINE_STEREO_IMAGER) ||
             (engineType >= ENGINE_GAIN_UTILITY && engineType <= ENGINE_PHASE_ALIGN))
        return EngineCategory::UTILITY;
    else
        return -1; // Invalid category
}

inline bool isValidEngineType(int engineType) {
    return engineType == ENGINE_NONE || (engineType >= 0 && engineType < ENGINE_COUNT);
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
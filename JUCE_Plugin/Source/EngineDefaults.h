#pragma once
#include <map>
#include "EngineIDs.h"

// Safe default parameter values for each engine type
// This prevents static noise from uninitialized parameters

namespace EngineDefaults {
    
    inline std::map<int, float> getDefaultParameters(int engineID) {
        std::map<int, float> params;
        
        // Initialize all to safe center values
        for (int i = 0; i < 10; ++i) {
            params[i] = 0.5f;
        }
        
        // Engine-specific safe defaults
        switch (engineID) {
            case ENGINE_BYPASS:
                // No parameters needed
                break;
                
            case ENGINE_K_STYLE:
                params[0] = 0.3f;  // Drive - moderate
                params[1] = 0.5f;  // Tone - neutral
                params[2] = 0.5f;  // Output - unity
                break;
                
            case ENGINE_TAPE_ECHO:
                params[0] = 0.4f;  // Delay time - 200ms
                params[1] = 0.4f;  // Feedback - safe
                params[2] = 0.3f;  // Mix - 30% wet
                params[3] = 0.5f;  // Wow/Flutter - moderate
                params[4] = 0.5f;  // Saturation
                break;
                
            case ENGINE_PLATE_REVERB:
                params[0] = 0.5f;  // Size
                params[1] = 0.6f;  // Damping
                params[2] = 0.5f;  // Width
                params[3] = 0.3f;  // Mix - 30% wet
                break;
                
            case ENGINE_CLASSIC_COMPRESSOR:
                params[0] = 0.7f;  // Threshold -10dB
                params[1] = 0.3f;  // Ratio 3:1
                params[2] = 0.2f;  // Attack - fast
                params[3] = 0.4f;  // Release - medium
                params[4] = 0.0f;  // Knee - hard
                params[5] = 0.5f;  // Makeup gain - 0dB
                params[6] = 0.0f;  // Mix - 100% wet
                break;
                
            case ENGINE_RODENT_DISTORTION:
            case ENGINE_MUFF_FUZZ:
                params[0] = 0.3f;  // Gain - moderate
                params[1] = 0.5f;  // Tone - neutral
                params[2] = 0.5f;  // Volume - unity
                params[3] = 0.0f;  // Mode - classic
                break;
                
            case ENGINE_CLASSIC_TREMOLO:
                params[0] = 0.25f; // Rate - 5Hz
                params[1] = 0.5f;  // Depth
                params[2] = 0.0f;  // Waveform - sine
                params[3] = 0.0f;  // Stereo phase
                params[4] = 1.0f;  // Volume - unity
                break;
                
            case ENGINE_PARAMETRIC_EQ:
                params[0] = 0.5f;  // Freq - 1kHz
                params[1] = 0.5f;  // Gain - 0dB
                params[2] = 0.5f;  // Q - moderate
                params[3] = 0.5f;  // Type - peak
                break;
                
            case ENGINE_STEREO_CHORUS:
                params[0] = 0.2f;  // Rate - slow
                params[1] = 0.3f;  // Depth - subtle
                params[2] = 0.3f;  // Mix - 30%
                params[3] = 0.5f;  // Feedback
                break;
                
            case ENGINE_BIT_CRUSHER:
                params[0] = 0.9f;  // Bit depth - high (less crushing)
                params[1] = 0.9f;  // Sample rate - high (less aliasing)
                params[2] = 0.3f;  // Mix - 30%
                break;
                
            case ENGINE_NOISE_GATE:
                params[0] = 0.2f;  // Threshold - low
                params[1] = 0.1f;  // Attack - fast
                params[2] = 0.3f;  // Hold
                params[3] = 0.4f;  // Release
                params[4] = 0.8f;  // Range
                break;
                
            case ENGINE_MASTERING_LIMITER:
                params[0] = 0.9f;  // Threshold - high (-1dB)
                params[1] = 0.2f;  // Release - fast
                params[2] = 0.0f;  // Knee - hard
                params[3] = 0.0f;  // Lookahead
                break;
                
            // Filters default to neutral/bypassed state
            case ENGINE_LADDER_FILTER:
            case ENGINE_STATE_VARIABLE_FILTER:
            case ENGINE_FORMANT_FILTER:
            case ENGINE_ENVELOPE_FILTER:
                params[0] = 0.5f;  // Cutoff - mid
                params[1] = 0.0f;  // Resonance - none
                params[2] = 0.5f;  // Drive
                params[3] = 0.0f;  // Mix - dry
                break;
                
            // Delays/Reverbs start with safe mix
            case ENGINE_BUCKET_BRIGADE_DELAY:
            case ENGINE_DIGITAL_DELAY:
            case ENGINE_MAGNETIC_DRUM_ECHO:
            case ENGINE_SHIMMER_REVERB:
            case ENGINE_SPRING_REVERB:
            case ENGINE_GATED_REVERB:
            case ENGINE_CONVOLUTION_REVERB:
                params[0] = 0.4f;  // Time
                params[1] = 0.3f;  // Feedback/Decay
                params[2] = 0.3f;  // Mix - 30%
                break;
                
            // Modulators start subtle
            case ENGINE_HARMONIC_TREMOLO:
            case ENGINE_ROTARY_SPEAKER:
            case ENGINE_ANALOG_PHASER:
            case ENGINE_DETUNE_DOUBLER:
            case ENGINE_PITCH_SHIFTER:
            case ENGINE_FREQUENCY_SHIFTER:
                params[0] = 0.2f;  // Rate/Amount - subtle
                params[1] = 0.3f;  // Depth
                params[2] = 0.3f;  // Mix
                break;
                
            // Experimental/Complex engines - very conservative
            case ENGINE_SPECTRAL_FREEZE:
            case ENGINE_GRANULAR_CLOUD:
            case ENGINE_PHASED_VOCODER:
            case ENGINE_CHAOS_GENERATOR:
            case ENGINE_BUFFER_REPEAT:
            case ENGINE_FEEDBACK_NETWORK:
                params[0] = 0.1f;  // Main parameter - minimal
                params[1] = 0.1f;  // Secondary - minimal
                params[2] = 0.2f;  // Mix - 20%
                break;
                
            // Saturators/Exciters - gentle
            case ENGINE_MULTIBAND_SATURATOR:
            case ENGINE_HARMONIC_EXCITER:
            case ENGINE_WAVE_FOLDER:
            case ENGINE_ANALOG_RING_MODULATOR:
            case ENGINE_VINTAGE_TUBE_PREAMP:
                params[0] = 0.2f;  // Drive - gentle
                params[1] = 0.5f;  // Tone
                params[2] = 0.3f;  // Mix
                params[3] = 0.5f;  // Output
                break;
                
            // Utility processors
            case ENGINE_TRANSIENT_SHAPER:
            case ENGINE_DIMENSION_EXPANDER:
            case ENGINE_MID_SIDE_PROCESSOR:
            case ENGINE_STEREO_WIDENER:
            case ENGINE_STEREO_IMAGER:
                params[0] = 0.5f;  // Amount - neutral
                params[1] = 0.5f;  // Balance
                params[2] = 0.0f;  // Mix - 100% for utility
                break;
                
            // EQs start flat
            case ENGINE_VINTAGE_CONSOLE_EQ:
            case ENGINE_DYNAMIC_EQ:
                params[0] = 0.5f;  // All bands at 0dB
                params[1] = 0.5f;
                params[2] = 0.5f;
                params[3] = 0.5f;
                params[4] = 0.5f;
                break;
                
            // Special processors
            case ENGINE_INTELLIGENT_HARMONIZER:
                params[0] = 0.0f;  // Interval - unison
                params[1] = 0.3f;  // Mix
                params[2] = 0.5f;  // Key
                params[3] = 0.5f;  // Scale
                break;
                
            case ENGINE_VOCAL_FORMANT_FILTER:
                params[0] = 0.0f;  // Vowel 1
                params[1] = 0.0f;  // Vowel 2
                params[2] = 0.0f;  // Morph
                params[3] = 0.3f;  // Resonance
                params[4] = 0.3f;  // Mix
                break;
                
            case ENGINE_VINTAGE_OPTO_COMPRESSOR:
                params[0] = 0.7f;  // Threshold
                params[1] = 0.3f;  // Ratio
                params[2] = 0.5f;  // Speed
                params[3] = 0.5f;  // Makeup
                break;
                
            case ENGINE_SPECTRAL_GATE:
                params[0] = 0.3f;  // Threshold
                params[1] = 0.5f;  // Frequency
                params[2] = 0.5f;  // Q
                params[3] = 0.3f;  // Mix
                break;
                
            case ENGINE_RESONANT_CHORUS:
                params[0] = 0.2f;  // Rate
                params[1] = 0.3f;  // Depth
                params[2] = 0.3f;  // Resonance
                params[3] = 0.3f;  // Mix
                break;
                
            default:
                // Unknown engine - use safe center values
                break;
        }
        
        return params;
    }
}
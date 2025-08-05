// Generated Default Parameter Values for ChimeraPhoenix
// Generated from parameter_database.json on 2025-08-04 02:06:38
// DO NOT EDIT MANUALLY - Edit parameter_database.json and regenerate

#include "DefaultParameterValues.h"
#include "EngineTypes.h"
#include <array>

void DefaultParameterValues::getDefaultParameters(int engineType, std::vector<float>& defaults) {
    defaults.clear();
    
    switch (engineType) {
        case ENGINE_VINTAGE_TUBE: // Vintage Tube Preamp
            defaults.push_back(0.5f); // Input Gain
            defaults.push_back(0.3f); // Drive
            defaults.push_back(0.5f); // Bias
            defaults.push_back(0.5f); // Bass
            defaults.push_back(0.5f); // Mid
            defaults.push_back(0.5f); // Treble
            defaults.push_back(0.5f); // Presence
            defaults.push_back(0.5f); // Output Gain
            defaults.push_back(0.0f); // Tube Type
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_TAPE_ECHO: // Tape Echo
            defaults.push_back(0.375f); // Time
            defaults.push_back(0.35f); // Feedback
            defaults.push_back(0.25f); // Wow & Flutter
            defaults.push_back(0.3f); // Saturation
            defaults.push_back(0.35f); // Mix
            break;

        case ENGINE_SHIMMER_REVERB: // Shimmer Reverb
            defaults.push_back(0.5f); // Size
            defaults.push_back(0.3f); // Shimmer
            defaults.push_back(0.5f); // Damping
            defaults.push_back(0.3f); // Mix
            break;

        case ENGINE_PLATE_REVERB: // Plate Reverb
            defaults.push_back(0.5f); // Size
            defaults.push_back(0.5f); // Damping
            defaults.push_back(0.0f); // Predelay
            defaults.push_back(0.3f); // Mix
            break;

        case ENGINE_SPRING_REVERB: // Spring Reverb
            defaults.push_back(0.5f); // Springs
            defaults.push_back(0.5f); // Decay
            defaults.push_back(0.5f); // Tone
            defaults.push_back(0.3f); // Mix
            break;

        case ENGINE_OPTO_COMPRESSOR: // Vintage Opto
            defaults.push_back(0.7f); // Threshold
            defaults.push_back(0.3f); // Ratio
            defaults.push_back(0.5f); // Speed
            defaults.push_back(0.5f); // Makeup
            break;

        case ENGINE_VCA_COMPRESSOR: // Classic Compressor
            defaults.push_back(0.7f); // Threshold
            defaults.push_back(0.3f); // Ratio
            defaults.push_back(0.2f); // Attack
            defaults.push_back(0.4f); // Release
            defaults.push_back(0.0f); // Knee
            defaults.push_back(0.5f); // Makeup
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_DIGITAL_CHORUS: // Stereo Chorus
            defaults.push_back(0.2f); // Rate
            defaults.push_back(0.3f); // Depth
            defaults.push_back(0.3f); // Mix
            defaults.push_back(0.0f); // Feedback
            break;

        case ENGINE_PITCH_SHIFTER: // Pitch Shifter
            defaults.push_back(0.5f); // Pitch
            defaults.push_back(0.5f); // Fine
            defaults.push_back(0.5f); // Mix
            break;

        case ENGINE_GRANULAR_CLOUD: // Granular Cloud
            defaults.push_back(0.5f); // Grains
            defaults.push_back(0.5f); // Size
            defaults.push_back(0.5f); // Position
            defaults.push_back(0.5f); // Pitch
            defaults.push_back(0.2f); // Mix
            break;

        case ENGINE_DIMENSION_EXPANDER: // Dimension Expander
            defaults.push_back(0.5f); // Size
            defaults.push_back(0.5f); // Width
            defaults.push_back(0.5f); // Mix
            break;

        case ENGINE_TRANSIENT_SHAPER: // Transient Shaper
            defaults.push_back(0.5f); // Attack
            defaults.push_back(0.5f); // Sustain
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_HARMONIC_TREMOLO: // Harmonic Tremolo
            defaults.push_back(0.25f); // Rate
            defaults.push_back(0.5f); // Depth
            defaults.push_back(0.4f); // Harmonics
            defaults.push_back(0.25f); // Stereo Phase
            break;

        case ENGINE_CLASSIC_TREMOLO: // Classic Tremolo
            defaults.push_back(0.25f); // Rate
            defaults.push_back(0.5f); // Depth
            defaults.push_back(0.0f); // Shape
            defaults.push_back(0.0f); // Stereo
            defaults.push_back(0.0f); // Type
            defaults.push_back(0.5f); // Symmetry
            defaults.push_back(1.0f); // Volume
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_COMB_RESONATOR: // Comb Resonator
            defaults.push_back(0.5f); // Frequency
            defaults.push_back(0.5f); // Resonance
            defaults.push_back(0.3f); // Mix
            break;

        case ENGINE_MID_SIDE_PROCESSOR: // Mid/Side Processor
            defaults.push_back(0.5f); // Mid Level
            defaults.push_back(0.5f); // Side Level
            defaults.push_back(0.5f); // Width
            break;

        case ENGINE_VINTAGE_CONSOLE_EQ: // Vintage Console EQ
            defaults.push_back(0.5f); // Low
            defaults.push_back(0.5f); // Low Mid
            defaults.push_back(0.5f); // High Mid
            defaults.push_back(0.5f); // High
            defaults.push_back(0.0f); // Drive
            break;

        case ENGINE_PARAMETRIC_EQ: // Parametric EQ
            defaults.push_back(0.2f); // Freq 1
            defaults.push_back(0.5f); // Gain 1
            defaults.push_back(0.5f); // Q 1
            defaults.push_back(0.5f); // Freq 2
            defaults.push_back(0.5f); // Gain 2
            defaults.push_back(0.5f); // Q 2
            defaults.push_back(0.8f); // Freq 3
            defaults.push_back(0.5f); // Gain 3
            defaults.push_back(0.5f); // Q 3
            break;

        case ENGINE_HARMONIC_EXCITER: // Harmonic Exciter
            defaults.push_back(0.2f); // Harmonics
            defaults.push_back(0.7f); // Frequency
            defaults.push_back(0.2f); // Mix
            break;

        case ENGINE_MUFF_FUZZ: // Muff Fuzz
            defaults.push_back(0.3f); // Sustain
            defaults.push_back(0.5f); // Tone
            defaults.push_back(0.5f); // Volume
            defaults.push_back(0.0f); // Gate
            defaults.push_back(0.0f); // Mids
            defaults.push_back(0.0f); // Variant
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_RODENT_DISTORTION: // Rodent Distortion
            defaults.push_back(0.5f); // Gain
            defaults.push_back(0.4f); // Filter
            defaults.push_back(0.5f); // Clipping
            defaults.push_back(0.5f); // Tone
            defaults.push_back(0.5f); // Output
            defaults.push_back(1.0f); // Mix
            defaults.push_back(0.0f); // Mode
            defaults.push_back(0.3f); // Presence
            break;

        case ENGINE_K_STYLE: // K-Style Overdrive
            defaults.push_back(0.3f); // Drive
            defaults.push_back(0.5f); // Tone
            defaults.push_back(0.5f); // Level
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_SPECTRAL_FREEZE: // Spectral Freeze
            defaults.push_back(0.0f); // Freeze
            defaults.push_back(0.5f); // Size
            defaults.push_back(0.2f); // Mix
            break;

        case ENGINE_BUFFER_REPEAT: // Buffer Repeat
            defaults.push_back(0.5f); // Size
            defaults.push_back(0.5f); // Rate
            defaults.push_back(0.3f); // Feedback
            defaults.push_back(0.3f); // Mix
            break;

        case ENGINE_CHAOS_GENERATOR: // Chaos Generator
            defaults.push_back(0.1f); // Rate
            defaults.push_back(0.1f); // Depth
            defaults.push_back(0.0f); // Type
            defaults.push_back(0.5f); // Smoothing
            defaults.push_back(0.0f); // Target
            defaults.push_back(0.0f); // Sync
            defaults.push_back(0.5f); // Seed
            defaults.push_back(0.2f); // Mix
            break;

        case ENGINE_INTELLIGENT_HARMONIZER: // Intelligent Harmonizer
            defaults.push_back(0.5f); // Interval
            defaults.push_back(0.0f); // Key
            defaults.push_back(0.0f); // Scale
            defaults.push_back(0.0f); // Voices
            defaults.push_back(0.3f); // Spread
            defaults.push_back(0.0f); // Humanize
            defaults.push_back(0.0f); // Formant
            defaults.push_back(0.5f); // Mix
            break;

        case ENGINE_GATED_REVERB: // Gated Reverb
            defaults.push_back(0.5f); // Size
            defaults.push_back(0.3f); // Gate Time
            defaults.push_back(0.5f); // Damping
            defaults.push_back(0.3f); // Mix
            break;

        case ENGINE_DETUNE_DOUBLER: // Detune Doubler
            defaults.push_back(0.3f); // Detune Amount
            defaults.push_back(0.15f); // Delay Time
            defaults.push_back(0.7f); // Stereo Width
            defaults.push_back(0.3f); // Thickness
            defaults.push_back(0.5f); // Mix
            break;

        case ENGINE_PHASED_VOCODER: // Phased Vocoder
            defaults.push_back(0.5f); // Bands
            defaults.push_back(0.5f); // Shift
            defaults.push_back(0.5f); // Formant
            defaults.push_back(0.2f); // Mix
            break;

        case ENGINE_SPECTRAL_GATE: // Spectral Gate
            defaults.push_back(0.3f); // Threshold
            defaults.push_back(0.5f); // Frequency
            defaults.push_back(0.5f); // Q
            defaults.push_back(0.3f); // Mix
            break;

        case ENGINE_NOISE_GATE: // Noise Gate
            defaults.push_back(0.2f); // Threshold
            defaults.push_back(0.1f); // Attack
            defaults.push_back(0.3f); // Hold
            defaults.push_back(0.4f); // Release
            defaults.push_back(0.8f); // Range
            break;

        case ENGINE_ENVELOPE_FILTER: // Envelope Filter
            defaults.push_back(0.5f); // Sensitivity
            defaults.push_back(0.1f); // Attack
            defaults.push_back(0.3f); // Release
            defaults.push_back(0.5f); // Range
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_FEEDBACK_NETWORK: // Feedback Network
            defaults.push_back(0.3f); // Feedback
            defaults.push_back(0.3f); // Delay
            defaults.push_back(0.2f); // Modulation
            defaults.push_back(0.2f); // Mix
            break;

        case ENGINE_MASTERING_LIMITER: // Mastering Limiter
            defaults.push_back(0.9f); // Threshold
            defaults.push_back(0.2f); // Release
            defaults.push_back(0.0f); // Knee
            defaults.push_back(0.0f); // Lookahead
            break;

        case ENGINE_STEREO_WIDENER: // Stereo Widener
            defaults.push_back(0.5f); // Width
            defaults.push_back(0.5f); // Bass Mono
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_RESONANT_CHORUS: // Resonant Chorus
            defaults.push_back(0.2f); // Rate
            defaults.push_back(0.3f); // Depth
            defaults.push_back(0.3f); // Resonance
            defaults.push_back(0.3f); // Mix
            break;

        case ENGINE_DIGITAL_DELAY: // Digital Delay
            defaults.push_back(0.4f); // Time
            defaults.push_back(0.3f); // Feedback
            defaults.push_back(0.3f); // Mix
            defaults.push_back(0.8f); // High Cut
            break;

        case ENGINE_DYNAMIC_EQ: // Dynamic EQ
            defaults.push_back(0.5f); // Frequency
            defaults.push_back(0.5f); // Threshold
            defaults.push_back(0.3f); // Ratio
            defaults.push_back(0.2f); // Attack
            defaults.push_back(0.4f); // Release
            defaults.push_back(0.5f); // Gain
            defaults.push_back(1.0f); // Mix
            defaults.push_back(0.0f); // Mode
            break;

        case ENGINE_STEREO_IMAGER: // Stereo Imager
            defaults.push_back(0.5f); // Width
            defaults.push_back(0.5f); // Center
            defaults.push_back(0.5f); // Rotation
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_ROTARY_SPEAKER: // Rotary Speaker
            defaults.push_back(0.5f); // Speed
            defaults.push_back(0.3f); // Acceleration
            defaults.push_back(0.3f); // Drive
            defaults.push_back(0.6f); // Mic Distance
            defaults.push_back(0.8f); // Stereo Width
            defaults.push_back(1.0f); // Mix
            break;

        case ENGINE_LADDER_FILTER: // Ladder Filter Pro
            defaults.push_back(0.5f); // Cutoff
            defaults.push_back(0.3f); // Resonance
            defaults.push_back(0.2f); // Drive
            defaults.push_back(0.0f); // Filter Type
            defaults.push_back(0.0f); // Asymmetry
            defaults.push_back(0.0f); // Vintage Mode
            defaults.push_back(1.0f); // Mix
            break;

        default:
            // Unknown engine type - provide safe defaults
            for (int i = 0; i < 8; ++i) {
                defaults.push_back(0.5f);
            }
            break;
    }
}

int DefaultParameterValues::getParameterCount(int engineType) {
    switch (engineType) {
        case ENGINE_VINTAGE_TUBE: return 10;
        case ENGINE_TAPE_ECHO: return 5;
        case ENGINE_SHIMMER_REVERB: return 4;
        case ENGINE_PLATE_REVERB: return 4;
        case ENGINE_SPRING_REVERB: return 4;
        case ENGINE_OPTO_COMPRESSOR: return 4;
        case ENGINE_VCA_COMPRESSOR: return 7;
        case ENGINE_DIGITAL_CHORUS: return 4;
        case ENGINE_PITCH_SHIFTER: return 3;
        case ENGINE_GRANULAR_CLOUD: return 5;
        case ENGINE_DIMENSION_EXPANDER: return 3;
        case ENGINE_TRANSIENT_SHAPER: return 3;
        case ENGINE_HARMONIC_TREMOLO: return 4;
        case ENGINE_CLASSIC_TREMOLO: return 8;
        case ENGINE_COMB_RESONATOR: return 3;
        case ENGINE_MID_SIDE_PROCESSOR: return 3;
        case ENGINE_VINTAGE_CONSOLE_EQ: return 5;
        case ENGINE_PARAMETRIC_EQ: return 9;
        case ENGINE_HARMONIC_EXCITER: return 3;
        case ENGINE_MUFF_FUZZ: return 7;
        case ENGINE_RODENT_DISTORTION: return 8;
        case ENGINE_K_STYLE: return 4;
        case ENGINE_SPECTRAL_FREEZE: return 3;
        case ENGINE_BUFFER_REPEAT: return 4;
        case ENGINE_CHAOS_GENERATOR: return 8;
        case ENGINE_INTELLIGENT_HARMONIZER: return 8;
        case ENGINE_GATED_REVERB: return 4;
        case ENGINE_DETUNE_DOUBLER: return 5;
        case ENGINE_PHASED_VOCODER: return 4;
        case ENGINE_SPECTRAL_GATE: return 4;
        case ENGINE_NOISE_GATE: return 5;
        case ENGINE_ENVELOPE_FILTER: return 5;
        case ENGINE_FEEDBACK_NETWORK: return 4;
        case ENGINE_MASTERING_LIMITER: return 4;
        case ENGINE_STEREO_WIDENER: return 3;
        case ENGINE_RESONANT_CHORUS: return 4;
        case ENGINE_DIGITAL_DELAY: return 4;
        case ENGINE_DYNAMIC_EQ: return 8;
        case ENGINE_STEREO_IMAGER: return 4;
        case ENGINE_ROTARY_SPEAKER: return 6;
        case ENGINE_LADDER_FILTER: return 7;
        default: return 0;
    }
}

const char* DefaultParameterValues::getParameterName(int engineType, int paramIndex) {
    switch (engineType) {
        case ENGINE_VINTAGE_TUBE:
            switch (paramIndex) {
                case 0: return "Input Gain";
                case 1: return "Drive";
                case 2: return "Bias";
                case 3: return "Bass";
                case 4: return "Mid";
                case 5: return "Treble";
                case 6: return "Presence";
                case 7: return "Output Gain";
                case 8: return "Tube Type";
                case 9: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_TAPE_ECHO:
            switch (paramIndex) {
                case 0: return "Time";
                case 1: return "Feedback";
                case 2: return "Wow & Flutter";
                case 3: return "Saturation";
                case 4: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_SHIMMER_REVERB:
            switch (paramIndex) {
                case 0: return "Size";
                case 1: return "Shimmer";
                case 2: return "Damping";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_PLATE_REVERB:
            switch (paramIndex) {
                case 0: return "Size";
                case 1: return "Damping";
                case 2: return "Predelay";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_SPRING_REVERB:
            switch (paramIndex) {
                case 0: return "Springs";
                case 1: return "Decay";
                case 2: return "Tone";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_OPTO_COMPRESSOR:
            switch (paramIndex) {
                case 0: return "Threshold";
                case 1: return "Ratio";
                case 2: return "Speed";
                case 3: return "Makeup";
                default: return "";
            }
            break;

        case ENGINE_VCA_COMPRESSOR:
            switch (paramIndex) {
                case 0: return "Threshold";
                case 1: return "Ratio";
                case 2: return "Attack";
                case 3: return "Release";
                case 4: return "Knee";
                case 5: return "Makeup";
                case 6: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_DIGITAL_CHORUS:
            switch (paramIndex) {
                case 0: return "Rate";
                case 1: return "Depth";
                case 2: return "Mix";
                case 3: return "Feedback";
                default: return "";
            }
            break;

        case ENGINE_PITCH_SHIFTER:
            switch (paramIndex) {
                case 0: return "Pitch";
                case 1: return "Fine";
                case 2: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_GRANULAR_CLOUD:
            switch (paramIndex) {
                case 0: return "Grains";
                case 1: return "Size";
                case 2: return "Position";
                case 3: return "Pitch";
                case 4: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_DIMENSION_EXPANDER:
            switch (paramIndex) {
                case 0: return "Size";
                case 1: return "Width";
                case 2: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_TRANSIENT_SHAPER:
            switch (paramIndex) {
                case 0: return "Attack";
                case 1: return "Sustain";
                case 2: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_HARMONIC_TREMOLO:
            switch (paramIndex) {
                case 0: return "Rate";
                case 1: return "Depth";
                case 2: return "Harmonics";
                case 3: return "Stereo Phase";
                default: return "";
            }
            break;

        case ENGINE_CLASSIC_TREMOLO:
            switch (paramIndex) {
                case 0: return "Rate";
                case 1: return "Depth";
                case 2: return "Shape";
                case 3: return "Stereo";
                case 4: return "Type";
                case 5: return "Symmetry";
                case 6: return "Volume";
                case 7: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_COMB_RESONATOR:
            switch (paramIndex) {
                case 0: return "Frequency";
                case 1: return "Resonance";
                case 2: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_MID_SIDE_PROCESSOR:
            switch (paramIndex) {
                case 0: return "Mid Level";
                case 1: return "Side Level";
                case 2: return "Width";
                default: return "";
            }
            break;

        case ENGINE_VINTAGE_CONSOLE_EQ:
            switch (paramIndex) {
                case 0: return "Low";
                case 1: return "Low Mid";
                case 2: return "High Mid";
                case 3: return "High";
                case 4: return "Drive";
                default: return "";
            }
            break;

        case ENGINE_PARAMETRIC_EQ:
            switch (paramIndex) {
                case 0: return "Freq 1";
                case 1: return "Gain 1";
                case 2: return "Q 1";
                case 3: return "Freq 2";
                case 4: return "Gain 2";
                case 5: return "Q 2";
                case 6: return "Freq 3";
                case 7: return "Gain 3";
                case 8: return "Q 3";
                default: return "";
            }
            break;

        case ENGINE_HARMONIC_EXCITER:
            switch (paramIndex) {
                case 0: return "Harmonics";
                case 1: return "Frequency";
                case 2: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_MUFF_FUZZ:
            switch (paramIndex) {
                case 0: return "Sustain";
                case 1: return "Tone";
                case 2: return "Volume";
                case 3: return "Gate";
                case 4: return "Mids";
                case 5: return "Variant";
                case 6: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_RODENT_DISTORTION:
            switch (paramIndex) {
                case 0: return "Gain";
                case 1: return "Filter";
                case 2: return "Clipping";
                case 3: return "Tone";
                case 4: return "Output";
                case 5: return "Mix";
                case 6: return "Mode";
                case 7: return "Presence";
                default: return "";
            }
            break;

        case ENGINE_K_STYLE:
            switch (paramIndex) {
                case 0: return "Drive";
                case 1: return "Tone";
                case 2: return "Level";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_SPECTRAL_FREEZE:
            switch (paramIndex) {
                case 0: return "Freeze";
                case 1: return "Size";
                case 2: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_BUFFER_REPEAT:
            switch (paramIndex) {
                case 0: return "Size";
                case 1: return "Rate";
                case 2: return "Feedback";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_CHAOS_GENERATOR:
            switch (paramIndex) {
                case 0: return "Rate";
                case 1: return "Depth";
                case 2: return "Type";
                case 3: return "Smoothing";
                case 4: return "Target";
                case 5: return "Sync";
                case 6: return "Seed";
                case 7: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_INTELLIGENT_HARMONIZER:
            switch (paramIndex) {
                case 0: return "Interval";
                case 1: return "Key";
                case 2: return "Scale";
                case 3: return "Voices";
                case 4: return "Spread";
                case 5: return "Humanize";
                case 6: return "Formant";
                case 7: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_GATED_REVERB:
            switch (paramIndex) {
                case 0: return "Size";
                case 1: return "Gate Time";
                case 2: return "Damping";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_DETUNE_DOUBLER:
            switch (paramIndex) {
                case 0: return "Detune Amount";
                case 1: return "Delay Time";
                case 2: return "Stereo Width";
                case 3: return "Thickness";
                case 4: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_PHASED_VOCODER:
            switch (paramIndex) {
                case 0: return "Bands";
                case 1: return "Shift";
                case 2: return "Formant";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_SPECTRAL_GATE:
            switch (paramIndex) {
                case 0: return "Threshold";
                case 1: return "Frequency";
                case 2: return "Q";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_NOISE_GATE:
            switch (paramIndex) {
                case 0: return "Threshold";
                case 1: return "Attack";
                case 2: return "Hold";
                case 3: return "Release";
                case 4: return "Range";
                default: return "";
            }
            break;

        case ENGINE_ENVELOPE_FILTER:
            switch (paramIndex) {
                case 0: return "Sensitivity";
                case 1: return "Attack";
                case 2: return "Release";
                case 3: return "Range";
                case 4: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_FEEDBACK_NETWORK:
            switch (paramIndex) {
                case 0: return "Feedback";
                case 1: return "Delay";
                case 2: return "Modulation";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_MASTERING_LIMITER:
            switch (paramIndex) {
                case 0: return "Threshold";
                case 1: return "Release";
                case 2: return "Knee";
                case 3: return "Lookahead";
                default: return "";
            }
            break;

        case ENGINE_STEREO_WIDENER:
            switch (paramIndex) {
                case 0: return "Width";
                case 1: return "Bass Mono";
                case 2: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_RESONANT_CHORUS:
            switch (paramIndex) {
                case 0: return "Rate";
                case 1: return "Depth";
                case 2: return "Resonance";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_DIGITAL_DELAY:
            switch (paramIndex) {
                case 0: return "Time";
                case 1: return "Feedback";
                case 2: return "Mix";
                case 3: return "High Cut";
                default: return "";
            }
            break;

        case ENGINE_DYNAMIC_EQ:
            switch (paramIndex) {
                case 0: return "Frequency";
                case 1: return "Threshold";
                case 2: return "Ratio";
                case 3: return "Attack";
                case 4: return "Release";
                case 5: return "Gain";
                case 6: return "Mix";
                case 7: return "Mode";
                default: return "";
            }
            break;

        case ENGINE_STEREO_IMAGER:
            switch (paramIndex) {
                case 0: return "Width";
                case 1: return "Center";
                case 2: return "Rotation";
                case 3: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_ROTARY_SPEAKER:
            switch (paramIndex) {
                case 0: return "Speed";
                case 1: return "Acceleration";
                case 2: return "Drive";
                case 3: return "Mic Distance";
                case 4: return "Stereo Width";
                case 5: return "Mix";
                default: return "";
            }
            break;

        case ENGINE_LADDER_FILTER:
            switch (paramIndex) {
                case 0: return "Cutoff";
                case 1: return "Resonance";
                case 2: return "Drive";
                case 3: return "Filter Type";
                case 4: return "Asymmetry";
                case 5: return "Vintage Mode";
                case 6: return "Mix";
                default: return "";
            }
            break;

        default: return "";
    }
}

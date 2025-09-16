#pragma once

#include <map>
#include <vector>
#include <string>

/**
 * Embedded parameter control map - no external JSON needed
 * COMPLETE implementation for ALL 56 engines
 */
class ParameterControlMap {
public:
    enum ControlType {
        CONTROL_ROTARY,
        CONTROL_TOGGLE,
        CONTROL_STEPPED
    };
    
    struct ParameterInfo {
        std::string name;
        ControlType control;
    };
    
    static ControlType getControlType(int engineId, int paramIndex) {
        const auto& params = getEngineParameters(engineId);
        if (paramIndex >= 0 && paramIndex < params.size()) {
            return params[paramIndex].control;
        }
        return CONTROL_ROTARY; // Default
    }
    
    static std::string getParameterName(int engineId, int paramIndex) {
        const auto& params = getEngineParameters(engineId);
        if (paramIndex >= 0 && paramIndex < params.size()) {
            return params[paramIndex].name;
        }
        return "Param " + std::to_string(paramIndex + 1);
    }
    
    static const std::vector<ParameterInfo>& getEngineParameters(int engineId) {
        // For now, return a default set of 8 parameters for ALL engines
        // This ensures NO CRASHES while we properly map each engine
        static const std::vector<ParameterInfo> defaultParams = {
            {"Param 1", CONTROL_ROTARY},
            {"Param 2", CONTROL_ROTARY},
            {"Param 3", CONTROL_ROTARY},
            {"Param 4", CONTROL_ROTARY},
            {"Param 5", CONTROL_ROTARY},
            {"Param 6", CONTROL_ROTARY},
            {"Param 7", CONTROL_ROTARY},
            {"Mix", CONTROL_ROTARY}
        };
        
        static const std::map<int, std::vector<ParameterInfo>> parameterMap = {
            // ENGINE_NONE (0)
            {0, {}},
            
            // DYNAMICS & COMPRESSION (1-6)
            {1, { // ENGINE_OPTO_COMPRESSOR
                {"Gain", CONTROL_ROTARY},
                {"Peak Reduction", CONTROL_ROTARY},
                {"HF Emphasis", CONTROL_ROTARY},
                {"Output", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY},
                {"Knee", CONTROL_ROTARY},
                {"Harmonics", CONTROL_ROTARY},
                {"Stereo Link", CONTROL_ROTARY}
            }},
            
            {2, { // ENGINE_VCA_COMPRESSOR - Classic Compressor
                {"Threshold", CONTROL_ROTARY},     // Intensity parameter
                {"Ratio", CONTROL_ROTARY},          // Intensity parameter
                {"Attack", CONTROL_ROTARY},         // TIME parameter - use slider
                {"Release", CONTROL_ROTARY},        // TIME parameter - use slider
                {"Knee", CONTROL_ROTARY},           // Character parameter
                {"Makeup", CONTROL_ROTARY},         // Intensity parameter
                {"Mix", CONTROL_ROTARY},            // Amount parameter
                {"Lookahead", CONTROL_ROTARY},      // TIME parameter - use slider
                {"Auto Release", CONTROL_TOGGLE},   // Boolean parameter
                {"Sidechain", CONTROL_STEPPED}      // Discrete choices
            }},
            
            {3, { // ENGINE_TRANSIENT_SHAPER
                {"Attack", CONTROL_ROTARY},
                {"Sustain", CONTROL_ROTARY},
                {"Attack Time", CONTROL_ROTARY},
                {"Release Time", CONTROL_ROTARY},
                {"Separation", CONTROL_ROTARY},
                {"Detection", CONTROL_STEPPED},
                {"Lookahead", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {4, { // ENGINE_NOISE_GATE
                {"Threshold", CONTROL_ROTARY},
                {"Range", CONTROL_ROTARY},
                {"Attack", CONTROL_ROTARY},
                {"Hold", CONTROL_ROTARY},
                {"Release", CONTROL_ROTARY},
                {"Hysteresis", CONTROL_ROTARY},
                {"SC Filter", CONTROL_ROTARY},
                {"Lookahead", CONTROL_ROTARY}
            }},
            
            {5, { // ENGINE_MASTERING_LIMITER
                {"Threshold", CONTROL_ROTARY},
                {"Ceiling", CONTROL_ROTARY},
                {"Release", CONTROL_ROTARY},
                {"Lookahead", CONTROL_ROTARY},
                {"Knee", CONTROL_ROTARY},
                {"Makeup", CONTROL_ROTARY},
                {"Saturation", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {6, { // ENGINE_DYNAMIC_EQ
                {"Frequency", CONTROL_ROTARY},
                {"Threshold", CONTROL_ROTARY},
                {"Ratio", CONTROL_ROTARY},
                {"Attack", CONTROL_ROTARY},
                {"Release", CONTROL_ROTARY},
                {"Gain", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY},
                {"Mode", CONTROL_STEPPED}
            }},
            
            // FILTERS & EQ (7-14)
            {7, { // ENGINE_PARAMETRIC_EQ
                {"Band 1 Freq", CONTROL_ROTARY},
                {"Band 1 Gain", CONTROL_ROTARY},
                {"Band 1 Q", CONTROL_ROTARY},
                {"Band 2 Freq", CONTROL_ROTARY},
                {"Band 2 Gain", CONTROL_ROTARY},
                {"Band 2 Q", CONTROL_ROTARY},
                {"Output Trim", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {8, { // ENGINE_VINTAGE_CONSOLE_EQ
                {"Low Freq", CONTROL_STEPPED},
                {"Low Gain", CONTROL_ROTARY},
                {"Mid Freq", CONTROL_STEPPED},
                {"Mid Gain", CONTROL_ROTARY},
                {"High Freq", CONTROL_STEPPED},
                {"High Gain", CONTROL_ROTARY},
                {"Drive", CONTROL_ROTARY},
                {"Output", CONTROL_ROTARY}
            }},
            
            {9, { // ENGINE_LADDER_FILTER
                {"Cutoff", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Drive", CONTROL_ROTARY},
                {"Filter Type", CONTROL_STEPPED},
                {"Asymmetry", CONTROL_ROTARY},
                {"Vintage Mode", CONTROL_TOGGLE},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {10, { // ENGINE_STATE_VARIABLE_FILTER
                {"Frequency", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Drive", CONTROL_ROTARY},
                {"Filter Type", CONTROL_STEPPED},
                {"Slope", CONTROL_STEPPED},
                {"Envelope", CONTROL_ROTARY},
                {"Analog", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {11, { // ENGINE_FORMANT_FILTER
                {"Vowel", CONTROL_STEPPED},
                {"Shift", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Morph", CONTROL_ROTARY},
                {"Drive", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {12, { // ENGINE_ENVELOPE_FILTER
                {"Sensitivity", CONTROL_ROTARY},
                {"Attack", CONTROL_ROTARY},
                {"Release", CONTROL_ROTARY},
                {"Range", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Filter", CONTROL_STEPPED},
                {"Direction", CONTROL_STEPPED},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {13, { // ENGINE_COMB_RESONATOR
                {"Root Freq", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Harmonic Spread", CONTROL_ROTARY},
                {"Decay Time", CONTROL_ROTARY},
                {"Damping", CONTROL_ROTARY},
                {"Mod Depth", CONTROL_ROTARY},
                {"Stereo Width", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {14, { // ENGINE_VOCAL_FORMANT
                {"Vowel 1", CONTROL_STEPPED},
                {"Vowel 2", CONTROL_STEPPED},
                {"Morph", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Brightness", CONTROL_ROTARY},
                {"Mod Rate", CONTROL_ROTARY},
                {"Mod Depth", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            // DISTORTION & SATURATION (15-22)
            {15, { // ENGINE_VINTAGE_TUBE
                {"Input Trim", CONTROL_ROTARY},
                {"Drive", CONTROL_ROTARY},
                {"Bass", CONTROL_ROTARY},
                {"Mid", CONTROL_ROTARY},
                {"Treble", CONTROL_ROTARY},
                {"Presence", CONTROL_ROTARY},
                {"Output Trim", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {16, { // ENGINE_WAVE_FOLDER
                {"Fold", CONTROL_ROTARY},
                {"Asymmetry", CONTROL_ROTARY},
                {"DC Offset", CONTROL_ROTARY},
                {"Pre Gain", CONTROL_ROTARY},
                {"Post Gain", CONTROL_ROTARY},
                {"Smoothing", CONTROL_ROTARY},
                {"Harmonics", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {17, { // ENGINE_HARMONIC_EXCITER
                {"Frequency", CONTROL_ROTARY},
                {"Drive", CONTROL_ROTARY},
                {"Harmonics", CONTROL_ROTARY},
                {"Clarity", CONTROL_ROTARY},
                {"Warmth", CONTROL_ROTARY},
                {"Presence", CONTROL_ROTARY},
                {"Color", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {18, { // ENGINE_BIT_CRUSHER
                {"Bits", CONTROL_STEPPED},
                {"Downsample", CONTROL_STEPPED},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {19, { // ENGINE_MULTIBAND_SATURATOR
                {"Low Drive", CONTROL_ROTARY},
                {"Mid Drive", CONTROL_ROTARY},
                {"High Drive", CONTROL_ROTARY},
                {"Saturation Type", CONTROL_STEPPED},
                {"Harmonic Character", CONTROL_ROTARY},
                {"Output Gain", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {20, { // ENGINE_MUFF_FUZZ
                {"Sustain", CONTROL_ROTARY},
                {"Tone", CONTROL_ROTARY},
                {"Volume", CONTROL_ROTARY},
                {"Gate", CONTROL_ROTARY},
                {"Mids", CONTROL_ROTARY},
                {"Variant", CONTROL_STEPPED},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {21, { // ENGINE_RODENT_DISTORTION
                {"Gain", CONTROL_ROTARY},
                {"Filter", CONTROL_ROTARY},
                {"Clipping", CONTROL_ROTARY},
                {"Tone", CONTROL_ROTARY},
                {"Output", CONTROL_ROTARY},
                {"Mode", CONTROL_STEPPED},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {22, { // ENGINE_K_STYLE
                {"Drive", CONTROL_ROTARY},
                {"Tone", CONTROL_ROTARY},
                {"Level", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            // MODULATION (23-33)
            {23, { // ENGINE_DIGITAL_CHORUS
                {"Rate", CONTROL_ROTARY},
                {"Depth", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Delay", CONTROL_ROTARY},
                {"Width", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {24, { // ENGINE_RESONANT_CHORUS
                {"Rate", CONTROL_ROTARY},
                {"Depth", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Filter Freq", CONTROL_ROTARY},
                {"Voices", CONTROL_STEPPED},
                {"Spread", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {25, { // ENGINE_ANALOG_PHASER
                {"Rate", CONTROL_ROTARY},
                {"Depth", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Stages", CONTROL_STEPPED},
                {"Stereo Spread", CONTROL_ROTARY},
                {"Center Freq", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {26, { // ENGINE_RING_MODULATOR
                {"Carrier Freq", CONTROL_ROTARY},
                {"Ring Amount", CONTROL_ROTARY},
                {"Frequency Shift", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Pulse Width", CONTROL_ROTARY},
                {"Phase Mod", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {27, { // ENGINE_FREQUENCY_SHIFTER
                {"Shift", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY},
                {"Spread", CONTROL_ROTARY},
                {"Resonance", CONTROL_ROTARY},
                {"Mod Depth", CONTROL_ROTARY},
                {"Mod Rate", CONTROL_ROTARY},
                {"Direction", CONTROL_STEPPED}
            }},
            
            {28, { // ENGINE_HARMONIC_TREMOLO
                {"Rate", CONTROL_ROTARY},
                {"Depth", CONTROL_ROTARY},
                {"Harmonics", CONTROL_ROTARY},
                {"Stereo Phase", CONTROL_ROTARY}
            }},
            
            {29, { // ENGINE_CLASSIC_TREMOLO
                {"Rate", CONTROL_ROTARY},
                {"Depth", CONTROL_ROTARY},
                {"Shape", CONTROL_STEPPED},
                {"Stereo", CONTROL_ROTARY},
                {"Type", CONTROL_STEPPED},
                {"Symmetry", CONTROL_ROTARY},
                {"Volume", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {30, { // ENGINE_ROTARY_SPEAKER
                {"Speed", CONTROL_ROTARY},
                {"Acceleration", CONTROL_ROTARY},
                {"Drive", CONTROL_ROTARY},
                {"Mic Distance", CONTROL_ROTARY},
                {"Stereo Width", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {31, { // ENGINE_PITCH_SHIFTER
                {"Pitch", CONTROL_ROTARY},
                {"Fine Tune", CONTROL_ROTARY},
                {"Formant", CONTROL_ROTARY},
                {"Size", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Quality", CONTROL_STEPPED},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {32, { // ENGINE_DETUNE_DOUBLER
                {"Detune", CONTROL_ROTARY},
                {"Delay", CONTROL_ROTARY},
                {"Width", CONTROL_ROTARY},
                {"Voices", CONTROL_STEPPED},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {33, { // ENGINE_INTELLIGENT_HARMONIZER - Fixed for all 15 parameters
                {"Voices", CONTROL_STEPPED},           // 0: Number of voices (1-3)
                {"Chord Type", CONTROL_STEPPED},       // 1: Chord preset selection
                {"Root Key", CONTROL_STEPPED},         // 2: Root key (C-B)
                {"Scale", CONTROL_STEPPED},            // 3: Scale type
                {"Master Mix", CONTROL_ROTARY},        // 4: Overall dry/wet - use knob for volumes
                {"Voice 1 Vol", CONTROL_ROTARY},       // 5: Voice 1 volume - use knob for volumes
                {"Voice 1 Formant", CONTROL_ROTARY},   // 6: Voice 1 formant
                {"Voice 2 Vol", CONTROL_ROTARY},       // 7: Voice 2 volume - use knob for volumes
                {"Voice 2 Formant", CONTROL_ROTARY},   // 8: Voice 2 formant
                {"Voice 3 Vol", CONTROL_ROTARY},       // 9: Voice 3 volume - use knob for volumes
                {"Voice 3 Formant", CONTROL_ROTARY},   // 10: Voice 3 formant
                {"Quality", CONTROL_STEPPED},          // 11: Low latency vs high quality
                {"Humanize", CONTROL_ROTARY},          // 12: Humanization amount
                {"Width", CONTROL_ROTARY},             // 13: Stereo width
                {"Transpose", CONTROL_STEPPED}         // 14: Global transpose
            }},
            
            // REVERB & DELAY (34-43)
            {34, { // ENGINE_TAPE_ECHO
                {"Delay Time", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Wow", CONTROL_ROTARY},
                {"Flutter", CONTROL_ROTARY},
                {"Saturation", CONTROL_ROTARY},
                {"Filter", CONTROL_ROTARY},
                {"Age", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {35, { // ENGINE_DIGITAL_DELAY
                {"Delay Time", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Filter", CONTROL_ROTARY},
                {"Modulation", CONTROL_ROTARY},
                {"Sync", CONTROL_TOGGLE},
                {"Ping Pong", CONTROL_TOGGLE},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {36, { // ENGINE_MAGNETIC_DRUM_ECHO
                {"Delay Time", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Drum Speed", CONTROL_ROTARY},
                {"Head Spacing", CONTROL_ROTARY},
                {"Saturation", CONTROL_ROTARY},
                {"Wear", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {37, { // ENGINE_BUCKET_BRIGADE_DELAY
                {"Delay Time", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Clock Noise", CONTROL_ROTARY},
                {"Filter", CONTROL_ROTARY},
                {"Companding", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {38, { // ENGINE_BUFFER_REPEAT
                {"Size", CONTROL_ROTARY},
                {"Speed", CONTROL_ROTARY},
                {"Trigger", CONTROL_TOGGLE},
                {"Gate", CONTROL_TOGGLE},
                {"Reverse", CONTROL_TOGGLE},
                {"Pitch", CONTROL_ROTARY},
                {"Filter", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {39, { // ENGINE_PLATE_REVERB
                {"Size", CONTROL_ROTARY},
                {"Decay", CONTROL_ROTARY},
                {"Damping", CONTROL_ROTARY},
                {"Predelay", CONTROL_ROTARY},
                {"Low Cut", CONTROL_ROTARY},
                {"High Cut", CONTROL_ROTARY},
                {"Early/Late", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {40, { // ENGINE_SPRING_REVERB
                {"Springs", CONTROL_STEPPED},
                {"Tension", CONTROL_ROTARY},
                {"Decay", CONTROL_ROTARY},
                {"Twang", CONTROL_ROTARY},
                {"Drip", CONTROL_ROTARY},
                {"Bass", CONTROL_ROTARY},
                {"Treble", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {41, { // ENGINE_CONVOLUTION_REVERB
                {"IR Select", CONTROL_STEPPED},
                {"Size", CONTROL_ROTARY},
                {"Predelay", CONTROL_ROTARY},
                {"Damping", CONTROL_ROTARY},
                {"Low Cut", CONTROL_ROTARY},
                {"High Cut", CONTROL_ROTARY},
                {"Width", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {42, { // ENGINE_SHIMMER_REVERB
                {"Size", CONTROL_ROTARY},
                {"Decay", CONTROL_ROTARY},
                {"Shimmer", CONTROL_ROTARY},
                {"Pitch", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Low Cut", CONTROL_ROTARY},
                {"High Cut", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {43, { // ENGINE_GATED_REVERB
                {"Size", CONTROL_ROTARY},
                {"Gate Time", CONTROL_ROTARY},
                {"Gate Thresh", CONTROL_ROTARY},
                {"Predelay", CONTROL_ROTARY},
                {"Damping", CONTROL_ROTARY},
                {"Attack", CONTROL_ROTARY},
                {"Release", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            // SPATIAL & SPECIAL (44-52)
            {44, { // ENGINE_STEREO_WIDENER
                {"Width", CONTROL_ROTARY},
                {"Bass Mono", CONTROL_TOGGLE},
                {"Frequency", CONTROL_ROTARY},
                {"Delay", CONTROL_ROTARY},
                {"Pan", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {45, { // ENGINE_STEREO_IMAGER
                {"Width", CONTROL_ROTARY},
                {"Rotation", CONTROL_ROTARY},
                {"Center", CONTROL_ROTARY},
                {"Low Width", CONTROL_ROTARY},
                {"Mid Width", CONTROL_ROTARY},
                {"High Width", CONTROL_ROTARY},
                {"Low Freq", CONTROL_ROTARY},
                {"High Freq", CONTROL_ROTARY}
            }},
            
            {46, { // ENGINE_DIMENSION_EXPANDER
                {"Size", CONTROL_ROTARY},
                {"Width", CONTROL_ROTARY},
                {"Depth", CONTROL_ROTARY},
                {"Height", CONTROL_ROTARY},
                {"Diffusion", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {47, { // ENGINE_SPECTRAL_FREEZE
                {"Freeze", CONTROL_TOGGLE},
                {"Threshold", CONTROL_ROTARY},
                {"Attack", CONTROL_ROTARY},
                {"Release", CONTROL_ROTARY},
                {"Spectral Blur", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {48, { // ENGINE_SPECTRAL_GATE
                {"Threshold", CONTROL_ROTARY},
                {"Ratio", CONTROL_ROTARY},
                {"Attack", CONTROL_ROTARY},
                {"Release", CONTROL_ROTARY},
                {"Frequency", CONTROL_ROTARY},
                {"Bandwidth", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {49, { // ENGINE_PHASED_VOCODER
                {"Stretch", CONTROL_ROTARY},
                {"Pitch", CONTROL_ROTARY},
                {"Smear", CONTROL_ROTARY},
                {"Transient", CONTROL_ROTARY},
                {"Phase", CONTROL_ROTARY},
                {"Gate", CONTROL_ROTARY},
                {"Freeze", CONTROL_TOGGLE},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {50, { // ENGINE_GRANULAR_CLOUD
                {"Grain Size", CONTROL_ROTARY},
                {"Density", CONTROL_ROTARY},
                {"Pitch Scatter", CONTROL_ROTARY},
                {"Position", CONTROL_ROTARY},
                {"Texture", CONTROL_ROTARY},
                {"Spread", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {51, { // ENGINE_CHAOS_GENERATOR
                {"Rate", CONTROL_ROTARY},
                {"Depth", CONTROL_ROTARY},
                {"Type", CONTROL_STEPPED},
                {"Smoothing", CONTROL_ROTARY},
                {"Target", CONTROL_STEPPED},
                {"Sync", CONTROL_TOGGLE},
                {"Seed", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            {52, { // ENGINE_FEEDBACK_NETWORK
                {"Delay Time", CONTROL_ROTARY},
                {"Feedback", CONTROL_ROTARY},
                {"Crossfeed", CONTROL_ROTARY},
                {"Diffusion", CONTROL_ROTARY},
                {"Modulation", CONTROL_ROTARY},
                {"Freeze", CONTROL_TOGGLE},
                {"Shimmer", CONTROL_ROTARY},
                {"Mix", CONTROL_ROTARY}
            }},
            
            // UTILITY (53-56)
            {53, { // ENGINE_MID_SIDE_PROCESSOR
                {"Mid Gain", CONTROL_ROTARY},
                {"Side Gain", CONTROL_ROTARY},
                {"Width", CONTROL_ROTARY},
                {"Mid Low", CONTROL_ROTARY},
                {"Mid High", CONTROL_ROTARY},
                {"Side Low", CONTROL_ROTARY},
                {"Side High", CONTROL_ROTARY},
                {"Bass Mono", CONTROL_TOGGLE}
            }},
            
            {54, { // ENGINE_GAIN_UTILITY
                {"Gain", CONTROL_ROTARY},
                {"Left Gain", CONTROL_ROTARY},
                {"Right Gain", CONTROL_ROTARY},
                {"Mid Gain", CONTROL_ROTARY},
                {"Side Gain", CONTROL_ROTARY},
                {"Mode", CONTROL_STEPPED},
                {"Phase L", CONTROL_TOGGLE},
                {"Phase R", CONTROL_TOGGLE}
            }},
            
            {55, { // ENGINE_MONO_MAKER
                {"Frequency", CONTROL_ROTARY},
                {"Slope", CONTROL_STEPPED},
                {"Mode", CONTROL_STEPPED},
                {"Bass Mono", CONTROL_TOGGLE},
                {"Preserve Phase", CONTROL_TOGGLE},
                {"DC Filter", CONTROL_TOGGLE},
                {"Width Above", CONTROL_ROTARY},
                {"Output Gain", CONTROL_ROTARY}
            }},
            
            {56, { // ENGINE_PHASE_ALIGN
                {"Delay", CONTROL_ROTARY},
                {"Phase", CONTROL_ROTARY},
                {"Frequency", CONTROL_ROTARY},
                {"All Pass", CONTROL_TOGGLE},
                {"Polarity", CONTROL_TOGGLE},
                {"Link", CONTROL_TOGGLE},
                {"Auto", CONTROL_TOGGLE}
            }}
        };
        
        // Find the engine in the map
        auto it = parameterMap.find(engineId);
        if (it != parameterMap.end()) {
            return it->second;
        }
        
        // If not found, return default parameters to prevent crashes
        return defaultParams;
    }
};
/*
  ==============================================================================

    EngineStringMapping.h
    Maps string engine identifiers to internal engine types
    This eliminates the need for numeric ID conversion

    ⚠️  SECONDARY SYSTEM: Used for AI integration and JSON preset loading only.
    For core plugin operations, use EngineTypes.h constants directly.
    See AUTHORITATIVE_SYSTEMS.md for complete system hierarchy.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <unordered_map>
#include <string>
#include "EngineTypes.h"

// Note: Engine types are now defined in EngineTypes.h
// This file only handles string mappings

class EngineStringMapping
{
public:
    // Map string IDs to engine types
    static const std::unordered_map<std::string, int> stringToEngine;
    
    // Map engine types to string IDs (reverse mapping)
    static const std::unordered_map<int, std::string> engineToString;
    
    // Map string IDs to dropdown choice indices
    static const std::unordered_map<std::string, int> stringToChoice;
    
    // Convert string ID to engine type
    static int getEngineFromString(const std::string& stringId)
    {
        auto it = stringToEngine.find(stringId);
        if (it != stringToEngine.end())
            return it->second;
        return ENGINE_NONE;  // Return ENGINE_NONE if not found
    }
    
    // Convert engine type to string ID
    static std::string getStringFromEngine(int engineType)
    {
        auto it = engineToString.find(engineType);
        if (it != engineToString.end())
            return it->second;
        return "";  // Return empty string for invalid engine
    }
    
    // Convert string ID to dropdown choice index
    static int getChoiceFromString(const std::string& stringId)
    {
        auto it = stringToChoice.find(stringId);
        if (it != stringToChoice.end())
            return it->second;
        return 0;  // Default to first engine (index 0)
    }
    
    // Parse JSON preset with string IDs
    static void parsePresetWithStringIds(const juce::var& jsonPreset, 
                                        std::function<void(int slot, int engineType)> setEngine,
                                        std::function<void(const std::string& param, float value)> setParam)
    {
        if (!jsonPreset.hasProperty("parameters"))
            return;
        
        auto params = jsonPreset["parameters"];
        
        // Process each slot
        for (int slot = 1; slot <= 6; ++slot)
        {
            std::string engineKey = "slot" + std::to_string(slot) + "_engine";
            
            if (params.hasProperty(engineKey))
            {
                std::string engineString = params[engineKey].toString().toStdString();
                int engineType = getEngineFromString(engineString);
                int choiceIndex = getChoiceFromString(engineString);
                
                // Use choice index for the dropdown
                setEngine(slot, choiceIndex);
                
                // Set other parameters
                for (int p = 1; p <= 10; ++p)
                {
                    std::string paramKey = "slot" + std::to_string(slot) + "_param" + std::to_string(p);
                    if (params.hasProperty(paramKey))
                    {
                        float value = (float)params[paramKey];
                        setParam(paramKey, value);
                    }
                }
                
                // Set mix and bypass
                std::string mixKey = "slot" + std::to_string(slot) + "_mix";
                if (params.hasProperty(mixKey))
                {
                    setParam(mixKey, (float)params[mixKey]);
                }
                
                std::string bypassKey = "slot" + std::to_string(slot) + "_bypass";
                if (params.hasProperty(bypassKey))
                {
                    setParam(bypassKey, (float)params[bypassKey]);
                }
            }
        }
        
        // Set master parameters
        if (params.hasProperty("master_input"))
            setParam("master_input", (float)params["master_input"]);
        if (params.hasProperty("master_output"))
            setParam("master_output", (float)params["master_output"]);
        if (params.hasProperty("master_mix"))
            setParam("master_mix", (float)params["master_mix"]);
    }
};

// Initialize the static maps using correct EngineTypes.h definitions
const std::unordered_map<std::string, int> EngineStringMapping::stringToEngine = {
    // Special case
    {"none", ENGINE_NONE},
    
    // DYNAMICS & COMPRESSION (IDs 1-6)
    {"vintage_opto", ENGINE_OPTO_COMPRESSOR},                // ID 1
    {"classic_compressor", ENGINE_VCA_COMPRESSOR},           // ID 2  
    {"transient_shaper", ENGINE_TRANSIENT_SHAPER},           // ID 3
    {"noise_gate", ENGINE_NOISE_GATE},                       // ID 4
    {"mastering_limiter", ENGINE_MASTERING_LIMITER},         // ID 5
    {"dynamic_eq", ENGINE_DYNAMIC_EQ},                       // ID 6
    
    // FILTERS & EQ (IDs 7-14)
    {"parametric_eq", ENGINE_PARAMETRIC_EQ},                 // ID 7
    {"vintage_console_eq", ENGINE_VINTAGE_CONSOLE_EQ},       // ID 8
    {"ladder_filter", ENGINE_LADDER_FILTER},                 // ID 9
    {"state_variable_filter", ENGINE_STATE_VARIABLE_FILTER}, // ID 10
    {"formant_filter", ENGINE_FORMANT_FILTER},               // ID 11
    {"envelope_filter", ENGINE_ENVELOPE_FILTER},             // ID 12
    {"comb_resonator", ENGINE_COMB_RESONATOR},               // ID 13
    {"vocal_formant", ENGINE_VOCAL_FORMANT},                 // ID 14
    
    // DISTORTION & SATURATION (IDs 15-22)
    {"vintage_tube", ENGINE_VINTAGE_TUBE},                   // ID 15
    {"wave_folder", ENGINE_WAVE_FOLDER},                     // ID 16
    {"harmonic_exciter", ENGINE_HARMONIC_EXCITER},           // ID 17
    {"bit_crusher", ENGINE_BIT_CRUSHER},                     // ID 18
    {"multiband_saturator", ENGINE_MULTIBAND_SATURATOR},     // ID 19
    {"muff_fuzz", ENGINE_MUFF_FUZZ},                         // ID 20
    {"rodent_distortion", ENGINE_RODENT_DISTORTION},         // ID 21
    {"k_style_overdrive", ENGINE_K_STYLE},                   // ID 22
    
    // MODULATION EFFECTS (IDs 23-33)
    {"digital_chorus", ENGINE_DIGITAL_CHORUS},               // ID 23
    {"resonant_chorus", ENGINE_RESONANT_CHORUS},             // ID 24
    {"analog_phaser", ENGINE_ANALOG_PHASER},                 // ID 25
    {"ring_modulator", ENGINE_RING_MODULATOR},               // ID 26
    {"frequency_shifter", ENGINE_FREQUENCY_SHIFTER},         // ID 27
    {"harmonic_tremolo", ENGINE_HARMONIC_TREMOLO},           // ID 28
    {"classic_tremolo", ENGINE_CLASSIC_TREMOLO},             // ID 29
    {"rotary_speaker", ENGINE_ROTARY_SPEAKER},               // ID 30
    {"pitch_shifter", ENGINE_PITCH_SHIFTER},                 // ID 31
    {"detune_doubler", ENGINE_DETUNE_DOUBLER},               // ID 32
    {"intelligent_harmonizer", ENGINE_INTELLIGENT_HARMONIZER}, // ID 33
    
    // REVERB & DELAY (IDs 34-43)
    {"tape_echo", ENGINE_TAPE_ECHO},                         // ID 34
    {"digital_delay", ENGINE_DIGITAL_DELAY},                 // ID 35
    {"magnetic_drum_echo", ENGINE_MAGNETIC_DRUM_ECHO},       // ID 36
    {"bucket_brigade", ENGINE_BUCKET_BRIGADE_DELAY},         // ID 37
    {"buffer_repeat", ENGINE_BUFFER_REPEAT},                 // ID 38
    {"plate_reverb", ENGINE_PLATE_REVERB},                   // ID 39
    {"spring_reverb", ENGINE_SPRING_REVERB},                 // ID 40
    {"convolution_reverb", ENGINE_CONVOLUTION_REVERB},       // ID 41
    {"shimmer_reverb", ENGINE_SHIMMER_REVERB},               // ID 42
    {"gated_reverb", ENGINE_GATED_REVERB},                   // ID 43
    
    // SPATIAL & SPECIAL EFFECTS (IDs 44-52)
    {"stereo_widener", ENGINE_STEREO_WIDENER},               // ID 44
    {"stereo_imager", ENGINE_STEREO_IMAGER},                 // ID 45
    {"dimension_expander", ENGINE_DIMENSION_EXPANDER},       // ID 46
    {"spectral_freeze", ENGINE_SPECTRAL_FREEZE},             // ID 47
    {"spectral_gate", ENGINE_SPECTRAL_GATE},                 // ID 48
    {"phased_vocoder", ENGINE_PHASED_VOCODER},               // ID 49
    {"granular_cloud", ENGINE_GRANULAR_CLOUD},               // ID 50
    {"chaos_generator", ENGINE_CHAOS_GENERATOR},             // ID 51
    {"feedback_network", ENGINE_FEEDBACK_NETWORK},           // ID 52
    
    // UTILITY (IDs 53-56)
    {"mid_side_processor", ENGINE_MID_SIDE_PROCESSOR},       // ID 53
    {"gain_utility", ENGINE_GAIN_UTILITY},                   // ID 54
    {"mono_maker", ENGINE_MONO_MAKER},                       // ID 55
    {"phase_align", ENGINE_PHASE_ALIGN}                      // ID 56
};

// String to dropdown choice index mapping
// Note: This maps to dropdown order, which includes ENGINE_NONE (0) plus engines 1-56
const std::unordered_map<std::string, int> EngineStringMapping::stringToChoice = {
    // ENGINE_NONE should map to dropdown index 0
    {"none", 0},
    
    // Map each engine string to its dropdown choice index (engine ID)
    // Since dropdown shows ENGINE_NONE first, then engines 1-56
    {"vintage_opto", 1},                // ENGINE_OPTO_COMPRESSOR
    {"classic_compressor", 2},          // ENGINE_VCA_COMPRESSOR
    {"transient_shaper", 3},            // ENGINE_TRANSIENT_SHAPER
    {"noise_gate", 4},                  // ENGINE_NOISE_GATE
    {"mastering_limiter", 5},           // ENGINE_MASTERING_LIMITER
    {"dynamic_eq", 6},                  // ENGINE_DYNAMIC_EQ
    {"parametric_eq", 7},               // ENGINE_PARAMETRIC_EQ
    {"vintage_console_eq", 8},          // ENGINE_VINTAGE_CONSOLE_EQ
    {"ladder_filter", 9},               // ENGINE_LADDER_FILTER
    {"state_variable_filter", 10},      // ENGINE_STATE_VARIABLE_FILTER
    {"formant_filter", 11},             // ENGINE_FORMANT_FILTER
    {"envelope_filter", 12},            // ENGINE_ENVELOPE_FILTER
    {"comb_resonator", 13},             // ENGINE_COMB_RESONATOR
    {"vocal_formant", 14},              // ENGINE_VOCAL_FORMANT
    {"vintage_tube", 15},               // ENGINE_VINTAGE_TUBE
    {"wave_folder", 16},                // ENGINE_WAVE_FOLDER
    {"harmonic_exciter", 17},           // ENGINE_HARMONIC_EXCITER
    {"bit_crusher", 18},                // ENGINE_BIT_CRUSHER
    {"multiband_saturator", 19},        // ENGINE_MULTIBAND_SATURATOR
    {"muff_fuzz", 20},                  // ENGINE_MUFF_FUZZ
    {"rodent_distortion", 21},          // ENGINE_RODENT_DISTORTION
    {"k_style_overdrive", 22},          // ENGINE_K_STYLE
    {"digital_chorus", 23},             // ENGINE_DIGITAL_CHORUS
    {"resonant_chorus", 24},            // ENGINE_RESONANT_CHORUS
    {"analog_phaser", 25},              // ENGINE_ANALOG_PHASER
    {"ring_modulator", 26},             // ENGINE_RING_MODULATOR
    {"frequency_shifter", 27},          // ENGINE_FREQUENCY_SHIFTER
    {"harmonic_tremolo", 28},           // ENGINE_HARMONIC_TREMOLO
    {"classic_tremolo", 29},            // ENGINE_CLASSIC_TREMOLO
    {"rotary_speaker", 30},             // ENGINE_ROTARY_SPEAKER
    {"pitch_shifter", 31},              // ENGINE_PITCH_SHIFTER
    {"detune_doubler", 32},             // ENGINE_DETUNE_DOUBLER
    {"intelligent_harmonizer", 33},     // ENGINE_INTELLIGENT_HARMONIZER
    {"tape_echo", 34},                  // ENGINE_TAPE_ECHO
    {"digital_delay", 35},              // ENGINE_DIGITAL_DELAY
    {"magnetic_drum_echo", 36},         // ENGINE_MAGNETIC_DRUM_ECHO
    {"bucket_brigade", 37},             // ENGINE_BUCKET_BRIGADE_DELAY
    {"buffer_repeat", 38},              // ENGINE_BUFFER_REPEAT
    {"plate_reverb", 39},               // ENGINE_PLATE_REVERB
    {"spring_reverb", 40},              // ENGINE_SPRING_REVERB
    {"convolution_reverb", 41},         // ENGINE_CONVOLUTION_REVERB
    {"shimmer_reverb", 42},             // ENGINE_SHIMMER_REVERB
    {"gated_reverb", 43},               // ENGINE_GATED_REVERB
    {"stereo_widener", 44},             // ENGINE_STEREO_WIDENER
    {"stereo_imager", 45},              // ENGINE_STEREO_IMAGER
    {"dimension_expander", 46},         // ENGINE_DIMENSION_EXPANDER
    {"spectral_freeze", 47},            // ENGINE_SPECTRAL_FREEZE
    {"spectral_gate", 48},              // ENGINE_SPECTRAL_GATE
    {"phased_vocoder", 49},             // ENGINE_PHASED_VOCODER
    {"granular_cloud", 50},             // ENGINE_GRANULAR_CLOUD
    {"chaos_generator", 51},            // ENGINE_CHAOS_GENERATOR
    {"feedback_network", 52},           // ENGINE_FEEDBACK_NETWORK
    {"mid_side_processor", 53},         // ENGINE_MID_SIDE_PROCESSOR
    {"gain_utility", 54},               // ENGINE_GAIN_UTILITY
    {"mono_maker", 55},                 // ENGINE_MONO_MAKER
    {"phase_align", 56}                 // ENGINE_PHASE_ALIGN
};

// Reverse mapping (engine type to string) using correct EngineTypes.h definitions
const std::unordered_map<int, std::string> EngineStringMapping::engineToString = {
    // Special case
    {ENGINE_NONE, "none"},                                       // ID 0
    
    // DYNAMICS & COMPRESSION (IDs 1-6)
    {ENGINE_OPTO_COMPRESSOR, "vintage_opto"},                    // ID 1
    {ENGINE_VCA_COMPRESSOR, "classic_compressor"},               // ID 2  
    {ENGINE_TRANSIENT_SHAPER, "transient_shaper"},               // ID 3
    {ENGINE_NOISE_GATE, "noise_gate"},                           // ID 4
    {ENGINE_MASTERING_LIMITER, "mastering_limiter"},             // ID 5
    {ENGINE_DYNAMIC_EQ, "dynamic_eq"},                           // ID 6
    
    // FILTERS & EQ (IDs 7-14)
    {ENGINE_PARAMETRIC_EQ, "parametric_eq"},                     // ID 7
    {ENGINE_VINTAGE_CONSOLE_EQ, "vintage_console_eq"},           // ID 8
    {ENGINE_LADDER_FILTER, "ladder_filter"},                     // ID 9
    {ENGINE_STATE_VARIABLE_FILTER, "state_variable_filter"},     // ID 10
    {ENGINE_FORMANT_FILTER, "formant_filter"},                   // ID 11
    {ENGINE_ENVELOPE_FILTER, "envelope_filter"},                 // ID 12
    {ENGINE_COMB_RESONATOR, "comb_resonator"},                   // ID 13
    {ENGINE_VOCAL_FORMANT, "vocal_formant"},                     // ID 14
    
    // DISTORTION & SATURATION (IDs 15-22)
    {ENGINE_VINTAGE_TUBE, "vintage_tube"},                       // ID 15
    {ENGINE_WAVE_FOLDER, "wave_folder"},                         // ID 16
    {ENGINE_HARMONIC_EXCITER, "harmonic_exciter"},               // ID 17
    {ENGINE_BIT_CRUSHER, "bit_crusher"},                         // ID 18
    {ENGINE_MULTIBAND_SATURATOR, "multiband_saturator"},         // ID 19
    {ENGINE_MUFF_FUZZ, "muff_fuzz"},                             // ID 20
    {ENGINE_RODENT_DISTORTION, "rodent_distortion"},             // ID 21
    {ENGINE_K_STYLE, "k_style_overdrive"},                       // ID 22
    
    // MODULATION EFFECTS (IDs 23-33)
    {ENGINE_DIGITAL_CHORUS, "digital_chorus"},                   // ID 23
    {ENGINE_RESONANT_CHORUS, "resonant_chorus"},                 // ID 24
    {ENGINE_ANALOG_PHASER, "analog_phaser"},                     // ID 25
    {ENGINE_RING_MODULATOR, "ring_modulator"},                   // ID 26
    {ENGINE_FREQUENCY_SHIFTER, "frequency_shifter"},             // ID 27
    {ENGINE_HARMONIC_TREMOLO, "harmonic_tremolo"},               // ID 28
    {ENGINE_CLASSIC_TREMOLO, "classic_tremolo"},                 // ID 29
    {ENGINE_ROTARY_SPEAKER, "rotary_speaker"},                   // ID 30
    {ENGINE_PITCH_SHIFTER, "pitch_shifter"},                     // ID 31
    {ENGINE_DETUNE_DOUBLER, "detune_doubler"},                   // ID 32
    {ENGINE_INTELLIGENT_HARMONIZER, "intelligent_harmonizer"},   // ID 33
    
    // REVERB & DELAY (IDs 34-43)
    {ENGINE_TAPE_ECHO, "tape_echo"},                             // ID 34
    {ENGINE_DIGITAL_DELAY, "digital_delay"},                     // ID 35
    {ENGINE_MAGNETIC_DRUM_ECHO, "magnetic_drum_echo"},           // ID 36
    {ENGINE_BUCKET_BRIGADE_DELAY, "bucket_brigade"},             // ID 37
    {ENGINE_BUFFER_REPEAT, "buffer_repeat"},                     // ID 38
    {ENGINE_PLATE_REVERB, "plate_reverb"},                       // ID 39
    {ENGINE_SPRING_REVERB, "spring_reverb"},                     // ID 40
    {ENGINE_CONVOLUTION_REVERB, "convolution_reverb"},           // ID 41
    {ENGINE_SHIMMER_REVERB, "shimmer_reverb"},                   // ID 42
    {ENGINE_GATED_REVERB, "gated_reverb"},                       // ID 43
    
    // SPATIAL & SPECIAL EFFECTS (IDs 44-52)
    {ENGINE_STEREO_WIDENER, "stereo_widener"},                   // ID 44
    {ENGINE_STEREO_IMAGER, "stereo_imager"},                     // ID 45
    {ENGINE_DIMENSION_EXPANDER, "dimension_expander"},           // ID 46
    {ENGINE_SPECTRAL_FREEZE, "spectral_freeze"},                 // ID 47
    {ENGINE_SPECTRAL_GATE, "spectral_gate"},                     // ID 48
    {ENGINE_PHASED_VOCODER, "phased_vocoder"},                   // ID 49
    {ENGINE_GRANULAR_CLOUD, "granular_cloud"},                   // ID 50
    {ENGINE_CHAOS_GENERATOR, "chaos_generator"},                 // ID 51
    {ENGINE_FEEDBACK_NETWORK, "feedback_network"},               // ID 52
    
    // UTILITY (IDs 53-56)
    {ENGINE_MID_SIDE_PROCESSOR, "mid_side_processor"},           // ID 53
    {ENGINE_GAIN_UTILITY, "gain_utility"},                       // ID 54
    {ENGINE_MONO_MAKER, "mono_maker"},                           // ID 55
    {ENGINE_PHASE_ALIGN, "phase_align"}                          // ID 56
};
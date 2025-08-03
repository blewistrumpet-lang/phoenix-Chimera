/*
  ==============================================================================

    EngineStringMapping.h
    Maps string engine identifiers to internal engine types
    This eliminates the need for numeric ID conversion

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <unordered_map>
#include <string>

// Engine type enum (matches existing EngineType in PluginProcessor.h)
enum EngineTypeEnum {
    ENGINE_BYPASS = -1,
    ENGINE_VINTAGE_TUBE = 0,
    ENGINE_TAPE_ECHO = 1,
    ENGINE_SHIMMER_REVERB = 2,
    ENGINE_PLATE_REVERB = 3,
    ENGINE_CONVOLUTION_REVERB = 4,
    ENGINE_SPRING_REVERB = 5,
    ENGINE_OPTO_COMPRESSOR = 6,
    ENGINE_VCA_COMPRESSOR = 7,
    ENGINE_MAGNETIC_DRUM_ECHO = 8,
    ENGINE_BUCKET_BRIGADE_DELAY = 9,
    // 10 commented out (Stereo Flanger)
    ENGINE_DIGITAL_CHORUS = 11,
    ENGINE_ANALOG_PHASER = 12,
    // 13 commented out (Digital Phaser)
    ENGINE_PITCH_SHIFTER = 14,
    ENGINE_RING_MODULATOR = 15,
    ENGINE_GRANULAR_CLOUD = 16,
    ENGINE_VOCAL_FORMANT = 17,
    ENGINE_DIMENSION_EXPANDER = 18,
    ENGINE_FREQUENCY_SHIFTER = 19,
    ENGINE_TRANSIENT_SHAPER = 20,
    ENGINE_HARMONIC_TREMOLO = 21,
    ENGINE_CLASSIC_TREMOLO = 22,
    ENGINE_COMB_RESONATOR = 23,
    ENGINE_ROTARY_SPEAKER = 24,
    ENGINE_MID_SIDE_PROCESSOR = 25,
    ENGINE_VINTAGE_CONSOLE_EQ = 26,
    ENGINE_PARAMETRIC_EQ = 27,
    ENGINE_LADDER_FILTER = 28,
    ENGINE_STATE_VARIABLE_FILTER = 29,
    ENGINE_FORMANT_FILTER = 30,
    ENGINE_WAVE_FOLDER = 31,
    ENGINE_HARMONIC_EXCITER = 32,
    ENGINE_BIT_CRUSHER = 33,
    ENGINE_MULTIBAND_SATURATOR = 34,
    ENGINE_MUFF_FUZZ = 35,
    ENGINE_RODENT_DISTORTION = 36,
    // 37 commented out (Virtual Bass Amp)
    ENGINE_K_STYLE = 38,
    ENGINE_SPECTRAL_FREEZE = 39,
    ENGINE_BUFFER_REPEAT = 40,
    ENGINE_CHAOS_GENERATOR = 41,
    ENGINE_INTELLIGENT_HARMONIZER = 42,
    ENGINE_GATED_REVERB = 43,
    ENGINE_DETUNE_DOUBLER = 44,
    ENGINE_PHASED_VOCODER = 45,
    ENGINE_SPECTRAL_GATE = 46,
    ENGINE_NOISE_GATE = 47,
    ENGINE_ENVELOPE_FILTER = 48,
    ENGINE_FEEDBACK_NETWORK = 49,
    ENGINE_MASTERING_LIMITER = 50,
    ENGINE_STEREO_WIDENER = 51,
    ENGINE_RESONANT_CHORUS = 52,
    ENGINE_DIGITAL_DELAY = 53,
    ENGINE_DYNAMIC_EQ = 54,
    ENGINE_STEREO_IMAGER = 55
};

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
        return ENGINE_BYPASS;  // Default to bypass if not found
    }
    
    // Convert engine type to string ID
    static std::string getStringFromEngine(int engineType)
    {
        auto it = engineToString.find(engineType);
        if (it != engineToString.end())
            return it->second;
        return "bypass";
    }
    
    // Convert string ID to dropdown choice index
    static int getChoiceFromString(const std::string& stringId)
    {
        auto it = stringToChoice.find(stringId);
        if (it != stringToChoice.end())
            return it->second;
        return 0;  // Default to bypass (index 0)
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

// Initialize the static maps
const std::unordered_map<std::string, int> EngineStringMapping::stringToEngine = {
    {"bypass", ENGINE_BYPASS},
    {"vintage_tube", ENGINE_VINTAGE_TUBE},
    {"tape_echo", ENGINE_TAPE_ECHO},
    {"shimmer_reverb", ENGINE_SHIMMER_REVERB},
    {"plate_reverb", ENGINE_PLATE_REVERB},
    {"convolution_reverb", ENGINE_CONVOLUTION_REVERB},
    {"spring_reverb", ENGINE_SPRING_REVERB},
    {"vintage_opto", ENGINE_OPTO_COMPRESSOR},
    {"classic_compressor", ENGINE_VCA_COMPRESSOR},
    {"magnetic_drum_echo", ENGINE_MAGNETIC_DRUM_ECHO},
    {"bucket_brigade", ENGINE_BUCKET_BRIGADE_DELAY},
    {"digital_chorus", ENGINE_DIGITAL_CHORUS},
    {"analog_phaser", ENGINE_ANALOG_PHASER},
    {"pitch_shifter", ENGINE_PITCH_SHIFTER},
    {"ring_modulator", ENGINE_RING_MODULATOR},
    {"granular_cloud", ENGINE_GRANULAR_CLOUD},
    {"vocal_formant", ENGINE_VOCAL_FORMANT},
    {"dimension_expander", ENGINE_DIMENSION_EXPANDER},
    {"frequency_shifter", ENGINE_FREQUENCY_SHIFTER},
    {"transient_shaper", ENGINE_TRANSIENT_SHAPER},
    {"harmonic_tremolo", ENGINE_HARMONIC_TREMOLO},
    {"classic_tremolo", ENGINE_CLASSIC_TREMOLO},
    {"comb_resonator", ENGINE_COMB_RESONATOR},
    {"rotary_speaker", ENGINE_ROTARY_SPEAKER},
    {"mid_side_processor", ENGINE_MID_SIDE_PROCESSOR},
    {"vintage_console_eq", ENGINE_VINTAGE_CONSOLE_EQ},
    {"parametric_eq", ENGINE_PARAMETRIC_EQ},
    {"ladder_filter", ENGINE_LADDER_FILTER},
    {"state_variable_filter", ENGINE_STATE_VARIABLE_FILTER},
    {"formant_filter", ENGINE_FORMANT_FILTER},
    {"wave_folder", ENGINE_WAVE_FOLDER},
    {"harmonic_exciter", ENGINE_HARMONIC_EXCITER},
    {"bit_crusher", ENGINE_BIT_CRUSHER},
    {"multiband_saturator", ENGINE_MULTIBAND_SATURATOR},
    {"muff_fuzz", ENGINE_MUFF_FUZZ},
    {"rodent_distortion", ENGINE_RODENT_DISTORTION},
    {"k_style_overdrive", ENGINE_K_STYLE},
    {"spectral_freeze", ENGINE_SPECTRAL_FREEZE},
    {"buffer_repeat", ENGINE_BUFFER_REPEAT},
    {"chaos_generator", ENGINE_CHAOS_GENERATOR},
    {"intelligent_harmonizer", ENGINE_INTELLIGENT_HARMONIZER},
    {"gated_reverb", ENGINE_GATED_REVERB},
    {"detune_doubler", ENGINE_DETUNE_DOUBLER},
    {"phased_vocoder", ENGINE_PHASED_VOCODER},
    {"spectral_gate", ENGINE_SPECTRAL_GATE},
    {"noise_gate", ENGINE_NOISE_GATE},
    {"envelope_filter", ENGINE_ENVELOPE_FILTER},
    {"feedback_network", ENGINE_FEEDBACK_NETWORK},
    {"mastering_limiter", ENGINE_MASTERING_LIMITER},
    {"stereo_widener", ENGINE_STEREO_WIDENER},
    {"resonant_chorus", ENGINE_RESONANT_CHORUS},
    {"digital_delay", ENGINE_DIGITAL_DELAY},
    {"dynamic_eq", ENGINE_DYNAMIC_EQ},
    {"stereo_imager", ENGINE_STEREO_IMAGER}
};

// String to dropdown choice index mapping
const std::unordered_map<std::string, int> EngineStringMapping::stringToChoice = {
    {"bypass", 0},
    {"k_style_overdrive", 1},
    {"tape_echo", 2},
    {"plate_reverb", 3},
    {"rodent_distortion", 4},
    {"muff_fuzz", 5},
    {"classic_tremolo", 6},
    {"magnetic_drum_echo", 7},
    {"bucket_brigade", 8},
    {"digital_delay", 9},
    {"harmonic_tremolo", 10},
    {"rotary_speaker", 11},
    {"detune_doubler", 12},
    {"ladder_filter", 13},
    {"formant_filter", 14},
    {"classic_compressor", 15},
    {"state_variable_filter", 16},
    {"digital_chorus", 17},
    {"spectral_freeze", 18},
    {"granular_cloud", 19},
    {"ring_modulator", 20},
    {"multiband_saturator", 21},
    {"comb_resonator", 22},
    {"pitch_shifter", 23},
    {"phased_vocoder", 24},
    {"convolution_reverb", 25},
    {"bit_crusher", 26},
    {"frequency_shifter", 27},
    {"wave_folder", 28},
    {"shimmer_reverb", 29},
    {"vocal_formant", 30},
    {"transient_shaper", 31},
    {"dimension_expander", 32},
    {"analog_phaser", 33},
    {"envelope_filter", 34},
    {"gated_reverb", 35},
    {"harmonic_exciter", 36},
    {"feedback_network", 37},
    {"intelligent_harmonizer", 38},
    {"parametric_eq", 39},
    {"mastering_limiter", 40},
    {"noise_gate", 41},
    {"vintage_opto", 42},
    {"spectral_gate", 43},
    {"chaos_generator", 44},
    {"buffer_repeat", 45},
    {"vintage_console_eq", 46},
    {"mid_side_processor", 47},
    {"vintage_tube", 48},
    {"spring_reverb", 49},
    {"resonant_chorus", 50},
    {"stereo_widener", 51},
    {"dynamic_eq", 52},
    {"stereo_imager", 53}
};

// Reverse mapping (engine type to string)
const std::unordered_map<int, std::string> EngineStringMapping::engineToString = {
    {ENGINE_BYPASS, "bypass"},
    {ENGINE_VINTAGE_TUBE, "vintage_tube"},
    {ENGINE_TAPE_ECHO, "tape_echo"},
    {ENGINE_SHIMMER_REVERB, "shimmer_reverb"},
    {ENGINE_PLATE_REVERB, "plate_reverb"},
    {ENGINE_CONVOLUTION_REVERB, "convolution_reverb"},
    {ENGINE_SPRING_REVERB, "spring_reverb"},
    {ENGINE_OPTO_COMPRESSOR, "vintage_opto"},
    {ENGINE_VCA_COMPRESSOR, "classic_compressor"},
    {ENGINE_MAGNETIC_DRUM_ECHO, "magnetic_drum_echo"},
    {ENGINE_BUCKET_BRIGADE_DELAY, "bucket_brigade"},
    {ENGINE_DIGITAL_CHORUS, "digital_chorus"},
    {ENGINE_ANALOG_PHASER, "analog_phaser"},
    {ENGINE_PITCH_SHIFTER, "pitch_shifter"},
    {ENGINE_RING_MODULATOR, "ring_modulator"},
    {ENGINE_GRANULAR_CLOUD, "granular_cloud"},
    {ENGINE_VOCAL_FORMANT, "vocal_formant"},
    {ENGINE_DIMENSION_EXPANDER, "dimension_expander"},
    {ENGINE_FREQUENCY_SHIFTER, "frequency_shifter"},
    {ENGINE_TRANSIENT_SHAPER, "transient_shaper"},
    {ENGINE_HARMONIC_TREMOLO, "harmonic_tremolo"},
    {ENGINE_CLASSIC_TREMOLO, "classic_tremolo"},
    {ENGINE_COMB_RESONATOR, "comb_resonator"},
    {ENGINE_ROTARY_SPEAKER, "rotary_speaker"},
    {ENGINE_MID_SIDE_PROCESSOR, "mid_side_processor"},
    {ENGINE_VINTAGE_CONSOLE_EQ, "vintage_console_eq"},
    {ENGINE_PARAMETRIC_EQ, "parametric_eq"},
    {ENGINE_LADDER_FILTER, "ladder_filter"},
    {ENGINE_STATE_VARIABLE_FILTER, "state_variable_filter"},
    {ENGINE_FORMANT_FILTER, "formant_filter"},
    {ENGINE_WAVE_FOLDER, "wave_folder"},
    {ENGINE_HARMONIC_EXCITER, "harmonic_exciter"},
    {ENGINE_BIT_CRUSHER, "bit_crusher"},
    {ENGINE_MULTIBAND_SATURATOR, "multiband_saturator"},
    {ENGINE_MUFF_FUZZ, "muff_fuzz"},
    {ENGINE_RODENT_DISTORTION, "rodent_distortion"},
    {ENGINE_K_STYLE, "k_style_overdrive"},
    {ENGINE_SPECTRAL_FREEZE, "spectral_freeze"},
    {ENGINE_BUFFER_REPEAT, "buffer_repeat"},
    {ENGINE_CHAOS_GENERATOR, "chaos_generator"},
    {ENGINE_INTELLIGENT_HARMONIZER, "intelligent_harmonizer"},
    {ENGINE_GATED_REVERB, "gated_reverb"},
    {ENGINE_DETUNE_DOUBLER, "detune_doubler"},
    {ENGINE_PHASED_VOCODER, "phased_vocoder"},
    {ENGINE_SPECTRAL_GATE, "spectral_gate"},
    {ENGINE_NOISE_GATE, "noise_gate"},
    {ENGINE_ENVELOPE_FILTER, "envelope_filter"},
    {ENGINE_FEEDBACK_NETWORK, "feedback_network"},
    {ENGINE_MASTERING_LIMITER, "mastering_limiter"},
    {ENGINE_STEREO_WIDENER, "stereo_widener"},
    {ENGINE_RESONANT_CHORUS, "resonant_chorus"},
    {ENGINE_DIGITAL_DELAY, "digital_delay"},
    {ENGINE_DYNAMIC_EQ, "dynamic_eq"},
    {ENGINE_STEREO_IMAGER, "stereo_imager"}
};
#pragma once

#include <JuceHeader.h>
#include "GoldenPreset.h"
#include "EngineTypes.h"

/**
 * PresetExporter - Flexible system for exporting Golden Corpus presets to JSON
 * Designed to handle future additions of presets and engines
 */
class PresetExporter
{
public:
    /**
     * Export a single preset to JSON string
     * @param preset The preset to export
     * @param prettyPrint If true, format with indentation
     * @return JSON string representation
     */
    static String exportPresetToJson(const GoldenPreset& preset, bool prettyPrint = true);
    
    /**
     * Export multiple presets to individual JSON files
     * @param presets Vector of presets to export
     * @param outputDirectory Directory to save JSON files
     * @return Number of successfully exported presets
     */
    static int exportPresetsToDirectory(const std::vector<GoldenPreset>& presets, 
                                       const File& outputDirectory);
    
    /**
     * Export all presets to a single JSON array file
     * @param presets Vector of presets to export
     * @param outputFile File to save JSON array
     * @return True if successful
     */
    static bool exportPresetsToSingleFile(const std::vector<GoldenPreset>& presets,
                                         const File& outputFile);
    
    /**
     * Create corpus metadata file
     * @param presets Vector of presets to analyze
     * @param outputDirectory Directory to save metadata
     */
    static void createCorpusMetadata(const std::vector<GoldenPreset>& presets,
                                    const File& outputDirectory);
    
    /**
     * Engine type mapping - EXTENSIBLE for future engines
     * Maps engine type enum to string name
     */
    static StringArray getEngineTypeNames();
    
    /**
     * Get engine type from name (reverse mapping)
     * @param engineName String name of engine
     * @return Engine type index, or -1 if not found
     */
    static int getEngineTypeFromName(const String& engineName);
    
private:
    /**
     * Convert engine configuration to JSON object
     */
    static var engineToJson(const GoldenPreset& preset, int slotIndex);
    
    /**
     * Convert profile structures to JSON
     */
    static var sonicProfileToJson(const SonicProfile& profile);
    static var emotionalProfileToJson(const EmotionalProfile& profile);
    static var sourceAffinityToJson(const SourceAffinity& affinity);
    
    /**
     * Helper to convert StringArray to JSON array
     */
    static var stringArrayToJson(const std::vector<String>& strings);
    
    /**
     * Get CPU tier as string
     */
    static String cpuTierToString(CPUTier tier);
};

/**
 * PresetRegistry - Central registry for all preset creation functions
 * Allows dynamic addition of new presets without modifying exporter
 */
class PresetRegistry
{
public:
    using PresetCreator = std::function<GoldenPreset()>;
    
    /**
     * Register a preset creation function
     * @param id Preset ID (e.g., "GC_001")
     * @param creator Function that creates the preset
     */
    static void registerPreset(const String& id, PresetCreator creator);
    
    /**
     * Get all registered preset IDs
     */
    static StringArray getAllPresetIds();
    
    /**
     * Create preset by ID
     * @param id Preset ID
     * @return Created preset, or empty preset if ID not found
     */
    static GoldenPreset createPreset(const String& id);
    
    /**
     * Create all registered presets
     * @return Vector of all presets
     */
    static std::vector<GoldenPreset> createAllPresets();
    
    /**
     * Check if preset ID is registered
     */
    static bool hasPreset(const String& id);
    
    /**
     * Get number of registered presets
     */
    static int getPresetCount();
    
    /**
     * Clear all registrations (useful for testing)
     */
    static void clearRegistry();
    
private:
    static std::map<String, PresetCreator>& getRegistry();
};

/**
 * Macro to automatically register presets
 * Use in implementation file:
 * REGISTER_PRESET("GC_001", createPreset_001_VelvetThunder)
 */
#define REGISTER_PRESET(id, func) \
    static struct PresetRegistrar_##func { \
        PresetRegistrar_##func() { \
            PresetRegistry::registerPreset(id, func); \
        } \
    } presetRegistrar_##func;

// Engine types are now defined in EngineTypes.h
// This namespace is kept for backward compatibility
namespace EngineTypes
{
    // Re-export from unified header for compatibility
    constexpr int VINTAGE_TUBE = ENGINE_VINTAGE_TUBE;
    constexpr int TAPE_ECHO = ENGINE_TAPE_ECHO;
    constexpr int SHIMMER_REVERB = ENGINE_SHIMMER_REVERB;
    constexpr int PLATE_REVERB = ENGINE_PLATE_REVERB;
    constexpr int CONVOLUTION_REVERB = ENGINE_CONVOLUTION_REVERB;
    constexpr int SPRING_REVERB = ENGINE_SPRING_REVERB;
    constexpr int OPTO_COMPRESSOR = ENGINE_OPTO_COMPRESSOR;
    constexpr int VCA_COMPRESSOR = ENGINE_VCA_COMPRESSOR;
    constexpr int MAGNETIC_DRUM_ECHO = ENGINE_MAGNETIC_DRUM_ECHO;
    constexpr int BUCKET_BRIGADE_DELAY = ENGINE_BUCKET_BRIGADE_DELAY;
    constexpr int ANALOG_CHORUS = ENGINE_ANALOG_CHORUS;
    constexpr int DIGITAL_CHORUS = ENGINE_DIGITAL_CHORUS;
    constexpr int ANALOG_PHASER = ENGINE_ANALOG_PHASER;
    constexpr int DIGITAL_PHASER = ENGINE_DIGITAL_PHASER;
    constexpr int PITCH_SHIFTER = ENGINE_PITCH_SHIFTER;
    constexpr int RING_MODULATOR = ENGINE_RING_MODULATOR;
    constexpr int GRANULAR_CLOUD = ENGINE_GRANULAR_CLOUD;
    constexpr int VOCAL_FORMANT = ENGINE_VOCAL_FORMANT;
    constexpr int DIMENSION_EXPANDER = ENGINE_DIMENSION_EXPANDER;
    constexpr int FREQUENCY_SHIFTER = ENGINE_FREQUENCY_SHIFTER;
    constexpr int TRANSIENT_SHAPER = ENGINE_TRANSIENT_SHAPER;
    constexpr int HARMONIC_TREMOLO = ENGINE_HARMONIC_TREMOLO;
    constexpr int CLASSIC_TREMOLO = ENGINE_CLASSIC_TREMOLO;
    constexpr int COMB_RESONATOR = ENGINE_COMB_RESONATOR;
    constexpr int RING_MOD = ENGINE_RING_MODULATOR; // Note: using RING_MODULATOR
    constexpr int MID_SIDE_PROCESSOR = ENGINE_MID_SIDE_PROCESSOR;
    constexpr int VINTAGE_CONSOLE_EQ = ENGINE_VINTAGE_CONSOLE_EQ;
    constexpr int PARAMETRIC_EQ = ENGINE_PARAMETRIC_EQ;
    constexpr int LADDER_FILTER = ENGINE_LADDER_FILTER;
    constexpr int STATE_VARIABLE_FILTER = ENGINE_STATE_VARIABLE_FILTER;
    constexpr int FORMANT_FILTER = ENGINE_FORMANT_FILTER;
    constexpr int WAVE_FOLDER = ENGINE_WAVE_FOLDER;
    constexpr int HARMONIC_EXCITER = ENGINE_HARMONIC_EXCITER;
    constexpr int BIT_CRUSHER = ENGINE_BIT_CRUSHER;
    constexpr int MULTIBAND_SATURATOR = ENGINE_MULTIBAND_SATURATOR;
    constexpr int MUFF_FUZZ = ENGINE_MUFF_FUZZ;
    constexpr int RODENT_DISTORTION = ENGINE_RODENT_DISTORTION;
    constexpr int TUBE_SCREAMER = ENGINE_TUBE_SCREAMER;
    constexpr int SPECTRAL_FREEZE = ENGINE_SPECTRAL_FREEZE;
    constexpr int BUFFER_REPEAT = ENGINE_BUFFER_REPEAT;
    constexpr int CHAOS_GENERATOR = ENGINE_CHAOS_GENERATOR;
    constexpr int INTELLIGENT_HARMONIZER = ENGINE_INTELLIGENT_HARMONIZER;
    constexpr int GATED_REVERB = ENGINE_GATED_REVERB;
    constexpr int DETUNE_DOUBLER = ENGINE_DETUNE_DOUBLER;
    constexpr int PHASED_VOCODER = ENGINE_PHASED_VOCODER;
    constexpr int SPECTRAL_GATE = ENGINE_SPECTRAL_GATE;
    constexpr int NOISE_GATE = ENGINE_NOISE_GATE;
    constexpr int ENVELOPE_FILTER = ENGINE_ENVELOPE_FILTER;
    constexpr int FEEDBACK_NETWORK = ENGINE_FEEDBACK_NETWORK;
    constexpr int MASTERING_LIMITER = ENGINE_MASTERING_LIMITER;
    
    constexpr int MAX_ENGINES = ENGINE_COUNT;
}
#pragma once

#include <JuceHeader.h>
#include "GoldenPreset.h"
#include "ParameterDefinitions.h"
#include "CompleteEngineMetadata.h"

/**
 * Generates Golden Corpus presets with intelligent parameter selection
 * and variation creation
 */
class PresetGenerator {
public:
    PresetGenerator();
    
    // Generate the first 10 reference standard presets
    std::vector<GoldenPreset> generateReferencePresets();
    
    // Generate a specific preset by template
    GoldenPreset generatePreset(const String& name, 
                               const String& category,
                               const std::vector<int>& engineTypes,
                               const String& characterGuide);
    
    // Generate variations of a hero preset
    std::vector<GoldenPreset> generateVariations(const GoldenPreset& heroPreset,
                                                 int numVariations = 3);
    
    // Helper methods for preset creation
    void setEngineConfiguration(GoldenPreset& preset,
                               const std::vector<int>& engineTypes,
                               const std::vector<float>& mixLevels);
    
    void setSonicProfile(GoldenPreset& preset,
                        float brightness, float density, float movement,
                        float space, float aggression, float vintage);
    
    void setEmotionalProfile(GoldenPreset& preset,
                           float energy, float mood, float tension,
                           float organic, float nostalgia);
    
    void setSourceAffinity(GoldenPreset& preset,
                          float vocals, float guitar, float drums,
                          float synth, float mix);
    
    void setParameters(GoldenPreset& preset, int slot, 
                      const std::vector<float>& params);
    
    void setMetadata(GoldenPreset& preset,
                    const StringArray& keywords,
                    const StringArray& userPrompts,
                    float complexity, float experimentalness);
    
private:
    int m_presetCounter = 1;
    CompleteEngineMetadata m_metadata;
    
    // Parameter generation strategies
    std::vector<float> generateSweetSpotParams(int engineType);
    std::vector<float> generateSubtleParams(int engineType);
    std::vector<float> generateExtremeParams(int engineType);
    std::vector<float> generateBalancedParams(int engineType);
    
    // Variation strategies
    GoldenPreset createSubtleVariation(const GoldenPreset& original);
    GoldenPreset createExtremeVariation(const GoldenPreset& original);
    GoldenPreset createAlternativeVariation(const GoldenPreset& original);
    
    // Helper to generate unique IDs
    String generatePresetId();
    String generateShortCode(const String& name);
    
    // Parameter range helpers
    float scaleParameter(float normalized, float min, float max);
    float randomInRange(float min, float max);
    float nudgeParameter(float value, float amount);
};

// Preset templates for the 10 reference standards
namespace PresetTemplates {
    
    // 1. Velvet Thunder - Vintage warmth and character
    struct VelvetThunder {
        static constexpr const char* name = "Velvet Thunder";
        static constexpr const char* category = "Character & Color";
        static constexpr const char* subcategory = "Vintage Warmth";
        static constexpr const char* technicalHint = "Tube Preamp + Tape Echo";
        static constexpr int engines[] = {ENGINE_VINTAGE_TUBE_PREAMP, ENGINE_TAPE_ECHO, -1, -1, -1, -1};
        static constexpr float mixLevels[] = {0.8f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f};
    };
    
    // 2. Crystal Palace - Ethereal space
    struct CrystalPalace {
        static constexpr const char* name = "Crystal Palace";
        static constexpr const char* category = "Spatial Design";
        static constexpr const char* subcategory = "Ethereal Atmospheres";
        static constexpr const char* technicalHint = "Shimmer Reverb + Dimension Expander";
        static constexpr int engines[] = {ENGINE_SHIMMER_REVERB, ENGINE_DIMENSION_EXPANDER, -1, -1, -1, -1};
        static constexpr float mixLevels[] = {0.7f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f};
    };
    
    // 3. Broken Radio - Lo-fi character
    struct BrokenRadio {
        static constexpr const char* name = "Broken Radio";
        static constexpr const char* category = "Character & Color";
        static constexpr const char* subcategory = "Lo-Fi Destruction";
        static constexpr const char* technicalHint = "Bit Crusher + Filter + Spring Verb";
        static constexpr int engines[] = {ENGINE_BIT_CRUSHER, ENGINE_LADDER_FILTER, ENGINE_SPRING_REVERB, -1, -1, -1};
        static constexpr float mixLevels[] = {0.6f, 0.8f, 0.4f, 0.0f, 0.0f, 0.0f};
    };
    
    // 4. Pulse Engine - Rhythmic movement
    struct PulseEngine {
        static constexpr const char* name = "Pulse Engine";
        static constexpr const char* category = "Motion & Modulation";
        static constexpr const char* subcategory = "Rhythmic Processors";
        static constexpr const char* technicalHint = "Harmonic Tremolo + Phaser + Delay";
        static constexpr int engines[] = {ENGINE_HARMONIC_TREMOLO, ENGINE_ANALOG_PHASER, ENGINE_DIGITAL_DELAY, -1, -1, -1};
        static constexpr float mixLevels[] = {0.7f, 0.5f, 0.4f, 0.0f, 0.0f, 0.0f};
    };
    
    // 5. Gravity Well - Experimental sound design
    struct GravityWell {
        static constexpr const char* name = "Gravity Well";
        static constexpr const char* category = "Experimental Laboratory";
        static constexpr const char* subcategory = "Feedback Systems";
        static constexpr const char* technicalHint = "Feedback Network + Spectral Freeze + Ring Mod";
        static constexpr int engines[] = {ENGINE_FEEDBACK_NETWORK, ENGINE_SPECTRAL_FREEZE, ENGINE_ANALOG_RING_MODULATOR, -1, -1, -1};
        static constexpr float mixLevels[] = {0.6f, 0.5f, 0.3f, 0.0f, 0.0f, 0.0f};
    };
    
    // 6. Console 73 - Classic mixing chain
    struct Console73 {
        static constexpr const char* name = "Console 73";
        static constexpr const char* category = "Studio Essentials";
        static constexpr const char* subcategory = "Channel Strips";
        static constexpr const char* technicalHint = "Console EQ + Opto Comp + Tape Sat";
        static constexpr int engines[] = {ENGINE_VINTAGE_CONSOLE_EQ, ENGINE_VINTAGE_OPTO_COMPRESSOR, ENGINE_K_STYLE, -1, -1, -1};
        static constexpr float mixLevels[] = {1.0f, 1.0f, 0.3f, 0.0f, 0.0f, 0.0f};
    };
    
    // 7. Infinite Cathedral - Massive space
    struct InfiniteCathedral {
        static constexpr const char* name = "Infinite Cathedral";
        static constexpr const char* category = "Spatial Design";
        static constexpr const char* subcategory = "Impossible Spaces";
        static constexpr const char* technicalHint = "Convolution Reverb + Pitch Shifter";
        static constexpr int engines[] = {ENGINE_CONVOLUTION_REVERB, ENGINE_PITCH_SHIFTER, -1, -1, -1, -1};
        static constexpr float mixLevels[] = {0.8f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f};
    };
    
    // 8. Analog Sunrise - Enhancement and warmth
    struct AnalogSunrise {
        static constexpr const char* name = "Analog Sunrise";
        static constexpr const char* category = "Character & Color";
        static constexpr const char* subcategory = "Harmonic Enhancement";
        static constexpr const char* technicalHint = "Harmonic Exciter + Chorus + Tube";
        static constexpr int engines[] = {ENGINE_HARMONIC_EXCITER, ENGINE_STEREO_CHORUS, ENGINE_VINTAGE_TUBE_PREAMP, -1, -1, -1};
        static constexpr float mixLevels[] = {0.6f, 0.4f, 0.5f, 0.0f, 0.0f, 0.0f};
    };
    
    // 9. Tidal Flow - Organic movement
    struct TidalFlow {
        static constexpr const char* name = "Tidal Flow";
        static constexpr const char* category = "Motion & Modulation";
        static constexpr const char* subcategory = "Envelope Following";
        static constexpr const char* technicalHint = "Envelope Filter + Rotary + BBD";
        static constexpr int engines[] = {ENGINE_ENVELOPE_FILTER, ENGINE_ROTARY_SPEAKER, ENGINE_BUCKET_BRIGADE_DELAY, -1, -1, -1};
        static constexpr float mixLevels[] = {0.7f, 0.6f, 0.3f, 0.0f, 0.0f, 0.0f};
    };
    
    // 10. Data Storm - Glitch and digital destruction
    struct DataStorm {
        static constexpr const char* name = "Data Storm";
        static constexpr const char* category = "Experimental Laboratory";
        static constexpr const char* subcategory = "Glitch Processing";
        static constexpr const char* technicalHint = "Granular + Freq Shift + Buffer Repeat";
        static constexpr int engines[] = {ENGINE_GRANULAR_CLOUD, ENGINE_FREQUENCY_SHIFTER, ENGINE_BUFFER_REPEAT, -1, -1, -1};
        static constexpr float mixLevels[] = {0.7f, 0.4f, 0.5f, 0.0f, 0.0f, 0.0f};
    };
}
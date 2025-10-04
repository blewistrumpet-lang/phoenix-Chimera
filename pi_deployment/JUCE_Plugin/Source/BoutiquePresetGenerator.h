#pragma once

#include <JuceHeader.h>
#include "GoldenPreset.h"
#include "ParameterDefinitions.h"
#include <map>
#include <functional>

/**
 * BoutiquePresetGenerator - Intelligent preset generation system
 * Creates studio-quality presets using musical knowledge and acoustic principles
 */
class BoutiquePresetGenerator {
public:
    // Preset archetypes that define sonic goals
    enum class PresetArchetype {
        // Studio Essentials
        VocalPolish,           // Transparent enhancement for vocals
        MixGlue,              // Subtle cohesion for mix bus
        AnalogWarmth,         // Vintage console character
        SurgicalCorrection,   // Problem-solving tools
        
        // Spatial Design
        IntimateRoom,         // Close, warm spaces
        ConcertHall,          // Natural, expansive halls
        DreamscapeAmbience,   // Ethereal, impossible spaces
        RhythmicSpace,        // Tempo-synced spatial effects
        
        // Character & Color
        TapeNostalgia,        // Authentic tape sound
        TubeSaturation,       // Harmonic richness
        VintageGrit,          // Controlled distortion
        ModernSheen,          // Contemporary polish
        
        // Motion & Modulation
        SubtleMovement,       // Gentle animation
        RhythmicPulse,        // Tempo-locked motion
        OrganicDrift,         // Natural parameter evolution
        PsychedelicSwirl,     // Deep modulation
        
        // Experimental
        GranularTexture,      // Micro-sound manipulation
        SpectralMorphing,     // Frequency domain magic
        ControlledChaos,      // Musical randomness
        SoundDesignTool       // Creative destruction
    };
    
    // Musical context for preset design
    struct MusicalContext {
        String genre = "Universal";
        float tempo = 120.0f;
        String key = "C";
        String sourceType = "Mix";
        float dynamicRange = 0.5f;  // 0=compressed, 1=dynamic
        float spectralBalance = 0.5f; // 0=dark, 1=bright
    };
    
    // Acoustic modeling parameters
    struct AcousticModel {
        float roomSize = 0.5f;
        float decay = 0.5f;
        float damping = 0.5f;
        float diffusion = 0.7f;
        float earlyReflections = 0.3f;
        float preDelay = 0.02f;
        
        // Material characteristics
        float woodResonance = 0.0f;
        float metalResonance = 0.0f;
        float airAbsorption = 0.3f;
    };
    
    // Harmonic structure for saturation/distortion
    struct HarmonicStructure {
        float evenHarmonics = 0.5f;   // Tube-like warmth
        float oddHarmonics = 0.5f;    // Transistor edge
        float intermodulation = 0.1f;  // Complex harmonics
        float asymmetry = 0.0f;       // Even/odd bias
        float frequency = 1000.0f;    // Center frequency
        float bandwidth = 0.5f;       // Harmonic spread
    };
    
    // Main generation methods
    static std::unique_ptr<GoldenPreset> generatePreset(
        PresetArchetype archetype,
        const MusicalContext& context = MusicalContext()
    );
    
    // Generate complete preset categories
    static std::vector<std::unique_ptr<GoldenPreset>> generateStudioEssentials();
    static std::vector<std::unique_ptr<GoldenPreset>> generateSpatialDesigns();
    static std::vector<std::unique_ptr<GoldenPreset>> generateCharacterColors();
    static std::vector<std::unique_ptr<GoldenPreset>> generateMotionModulation();
    static std::vector<std::unique_ptr<GoldenPreset>> generateExperimental();
    
    // Variation generation
    static std::vector<std::unique_ptr<GoldenPreset>> generateVariations(
        const GoldenPreset& parent,
        int numVariations = 3
    );
    
private:
    // Engine selection strategies
    struct EngineChain {
        std::vector<int> engines;
        std::vector<float> mixLevels;
        std::vector<std::function<void(std::vector<float>&)>> parameterSetters;
    };
    
    // Archetype implementations
    static EngineChain createVocalPolishChain();
    static EngineChain createMixGlueChain();
    static EngineChain createAnalogWarmthChain();
    static EngineChain createDreamscapeChain();
    static EngineChain createTapeNostalgiaChain();
    static EngineChain createRhythmicPulseChain();
    static EngineChain createGranularTextureChain();
    
    // Parameter generation strategies
    static std::vector<float> generateReverbParameters(const AcousticModel& model);
    static std::vector<float> generateCompressorParameters(float ratio, float attack, float release);
    static std::vector<float> generateEQParameters(float freq, float gain, float q);
    static std::vector<float> generateSaturationParameters(const HarmonicStructure& harmonics);
    static std::vector<float> generateDelayParameters(float tempo, bool synced, float feedback);
    static std::vector<float> generateModulationParameters(float rate, float depth, float shape);
    
    // Musical parameter relationships
    static void applyGoldenRatio(std::vector<float>& params);
    static void applyFibonacciSpacing(std::vector<float>& params);
    static void applyHarmonicSeries(std::vector<float>& params);
    static void applyPsychoacousticCurves(std::vector<float>& params);
    
    // Preset metadata generation
    static void generateMetadata(GoldenPreset& preset, PresetArchetype archetype);
    static void generateSonicProfile(GoldenPreset& preset, PresetArchetype archetype);
    static void generateEmotionalProfile(GoldenPreset& preset, PresetArchetype archetype);
    static void generateKeywords(GoldenPreset& preset, PresetArchetype archetype);
    static String generateCreativeName(PresetArchetype archetype, const MusicalContext& context);
    
    // Quality assurance
    static void optimizePreset(GoldenPreset& preset);
    static void ensureMusicalParameters(GoldenPreset& preset);
    static void balanceFrequencySpectrum(GoldenPreset& preset);
    static void preventPhaseIssues(GoldenPreset& preset);
    
    // Helper methods
    static float musicalRandom(float min, float max, float bias = 0.5f);
    static float quantizeToMusicalValue(float value, float steps);
    static int selectComplementaryEngine(int primaryEngine);
    static bool enginesAreCompatible(int engine1, int engine2);
    
    // Parameter sweet spots database
    static std::map<int, std::vector<float>> getSweetSpotParameters();
    
    // Acoustic space templates
    static AcousticModel getStudioAcoustics();
    static AcousticModel getConcertHallAcoustics();
    static AcousticModel getIntimateRoomAcoustics();
    static AcousticModel getCathedralAcoustics();
    
    // Harmonic templates  
    static HarmonicStructure getVintageTubeHarmonics();
    static HarmonicStructure getAnalogTapeHarmonics();
    static HarmonicStructure getTransistorHarmonics();
    static HarmonicStructure getDigitalHarmonics();
};
#pragma once

/**
 * SimplifiedEngineMapping.h
 * 
 * Quick reference mapping of ENGINE_* constants to actual implementation classes.
 * This helps the test harness understand which engines are actually implemented.
 * 
 * Generated from analysis of EngineFactory.cpp and EngineTypes.h
 */

#include "EngineTypes.h"

namespace SimplifiedEngineMapping {
    
    struct EngineInfo {
        int id;
        const char* name;
        const char* className;
        const char* category;
        bool isPlatinum;  // Indicates if this is a "Platinum" version
    };
    
    // Complete mapping of all 57 implemented engines (0-56)
    static const EngineInfo ENGINE_MAPPING[] = {
        // Special
        {0, "None", "NoneEngine", "Special", false},
        
        // DYNAMICS & COMPRESSION (1-6)
        {1, "Opto Compressor", "VintageOptoCompressor_Platinum", "Dynamics", true},
        {2, "VCA Compressor", "ClassicCompressor", "Dynamics", false},
        {3, "Transient Shaper", "TransientShaper_Platinum", "Dynamics", true},
        {4, "Noise Gate", "NoiseGate_Platinum", "Dynamics", true},
        {5, "Mastering Limiter", "MasteringLimiter_Platinum", "Dynamics", true},
        {6, "Dynamic EQ", "DynamicEQ", "Dynamics", false},
        
        // FILTERS & EQ (7-14)
        {7, "Parametric EQ", "ParametricEQ_Platinum", "Filters", true},
        {8, "Vintage Console EQ", "VintageConsoleEQ_Platinum", "Filters", true},
        {9, "Ladder Filter", "LadderFilter", "Filters", false},
        {10, "State Variable Filter", "StateVariableFilter", "Filters", false},
        {11, "Formant Filter", "FormantFilter", "Filters", false},
        {12, "Envelope Filter", "EnvelopeFilter", "Filters", false},
        {13, "Comb Resonator", "CombResonator", "Filters", false},
        {14, "Vocal Formant", "VocalFormantFilter", "Filters", false},
        
        // DISTORTION & SATURATION (15-22)
        {15, "Vintage Tube", "VintageTubePreamp", "Distortion", false},
        {16, "Wave Folder", "WaveFolder", "Distortion", false},
        {17, "Harmonic Exciter", "HarmonicExciter_Platinum", "Distortion", true},
        {18, "Bit Crusher", "BitCrusher", "Distortion", false},
        {19, "Multiband Saturator", "MultibandSaturator", "Distortion", false},
        {20, "Muff Fuzz", "MuffFuzz", "Distortion", false},
        {21, "Rodent Distortion", "RodentDistortion", "Distortion", false},
        {22, "K-Style Overdrive", "KStyleOverdrive", "Distortion", false},
        
        // MODULATION (23-33)
        {23, "Digital Chorus", "StereoChorus", "Modulation", false},
        {24, "Resonant Chorus", "ResonantChorus_Platinum", "Modulation", true},
        {25, "Analog Phaser", "AnalogPhaser", "Modulation", false},
        {26, "Ring Modulator", "PlatinumRingModulator", "Modulation", true},
        {27, "Frequency Shifter", "FrequencyShifter", "Modulation", false},
        {28, "Harmonic Tremolo", "HarmonicTremolo", "Modulation", false},
        {29, "Classic Tremolo", "ClassicTremolo", "Modulation", false},
        {30, "Rotary Speaker", "RotarySpeaker_Platinum", "Modulation", true},
        {31, "Pitch Shifter", "PitchShifter", "Modulation", false},
        {32, "Detune Doubler", "DetuneDoubler", "Modulation", false},
        {33, "Intelligent Harmonizer", "IntelligentHarmonizer", "Modulation", false},
        
        // REVERB & DELAY (34-43)
        {34, "Tape Echo", "TapeEcho", "Time", false},
        {35, "Digital Delay", "DigitalDelay", "Time", false},
        {36, "Magnetic Drum Echo", "MagneticDrumEcho", "Time", false},
        {37, "Bucket Brigade Delay", "BucketBrigadeDelay", "Time", false},
        {38, "Buffer Repeat", "BufferRepeat_Platinum", "Time", true},
        {39, "Plate Reverb", "PlateReverb", "Time", false},
        {40, "Spring Reverb", "SpringReverb_Platinum", "Time", true},
        {41, "Convolution Reverb", "ConvolutionReverb", "Time", false},
        {42, "Shimmer Reverb", "ShimmerReverb", "Time", false},
        {43, "Gated Reverb", "GatedReverb", "Time", false},
        
        // SPATIAL & SPECIAL (44-52)
        {44, "Stereo Widener", "StereoWidener", "Spatial", false},
        {45, "Stereo Imager", "StereoImager", "Spatial", false},
        {46, "Dimension Expander", "DimensionExpander", "Spatial", false},
        {47, "Spectral Freeze", "SpectralFreeze", "Special", false},
        {48, "Spectral Gate", "SpectralGate_Platinum", "Special", true},
        {49, "Phased Vocoder", "PhasedVocoder", "Special", false},
        {50, "Granular Cloud", "GranularCloud", "Special", false},
        {51, "Chaos Generator", "ChaosGenerator_Platinum", "Special", true},
        {52, "Feedback Network", "FeedbackNetwork", "Special", false},
        
        // UTILITY (53-56)
        {53, "Mid-Side Processor", "MidSideProcessor_Platinum", "Utility", true},
        {54, "Gain Utility", "GainUtility_Platinum", "Utility", true},
        {55, "Mono Maker", "MonoMaker_Platinum", "Utility", true},
        {56, "Phase Align", "PhaseAlign_Platinum", "Utility", true}
    };
    
    static constexpr int ENGINE_COUNT = sizeof(ENGINE_MAPPING) / sizeof(EngineInfo);
    
    // Helper functions
    inline const EngineInfo* getEngineInfo(int engineID) {
        for (int i = 0; i < ENGINE_COUNT; ++i) {
            if (ENGINE_MAPPING[i].id == engineID) {
                return &ENGINE_MAPPING[i];
            }
        }
        return nullptr;
    }
    
    inline const char* getEngineClassName(int engineID) {
        const auto* info = getEngineInfo(engineID);
        return info ? info->className : "Unknown";
    }
    
    inline const char* getEngineCategory(int engineID) {
        const auto* info = getEngineInfo(engineID);
        return info ? info->category : "Unknown";
    }
    
    inline bool isEnginePlatinum(int engineID) {
        const auto* info = getEngineInfo(engineID);
        return info ? info->isPlatinum : false;
    }
    
    // Get engines by category
    inline std::vector<int> getEnginesByCategory(const char* category) {
        std::vector<int> engines;
        for (int i = 0; i < ENGINE_COUNT; ++i) {
            if (strcmp(ENGINE_MAPPING[i].category, category) == 0) {
                engines.push_back(ENGINE_MAPPING[i].id);
            }
        }
        return engines;
    }
    
    // Get all Platinum engines
    inline std::vector<int> getPlatinumEngines() {
        std::vector<int> engines;
        for (int i = 0; i < ENGINE_COUNT; ++i) {
            if (ENGINE_MAPPING[i].isPlatinum) {
                engines.push_back(ENGINE_MAPPING[i].id);
            }
        }
        return engines;
    }
    
    // Statistics
    inline void printEngineStatistics() {
        int categoryCount[7] = {0}; // Special, Dynamics, Filters, Distortion, Modulation, Time, Spatial, Utility
        int platinumCount = 0;
        
        for (int i = 0; i < ENGINE_COUNT; ++i) {
            const auto& engine = ENGINE_MAPPING[i];
            
            if (engine.isPlatinum) platinumCount++;
            
            if (strcmp(engine.category, "Special") == 0) categoryCount[0]++;
            else if (strcmp(engine.category, "Dynamics") == 0) categoryCount[1]++;
            else if (strcmp(engine.category, "Filters") == 0) categoryCount[2]++;
            else if (strcmp(engine.category, "Distortion") == 0) categoryCount[3]++;
            else if (strcmp(engine.category, "Modulation") == 0) categoryCount[4]++;
            else if (strcmp(engine.category, "Time") == 0) categoryCount[5]++;
            else if (strcmp(engine.category, "Spatial") == 0) categoryCount[6]++;
            else if (strcmp(engine.category, "Utility") == 0) categoryCount[7]++;
        }
        
        printf("Engine Statistics:\n");
        printf("Total Engines: %d\n", ENGINE_COUNT);
        printf("Platinum Engines: %d (%.1f%%)\n", platinumCount, 100.0f * platinumCount / ENGINE_COUNT);
        printf("\nBy Category:\n");
        printf("  Special: %d\n", categoryCount[0]);
        printf("  Dynamics: %d\n", categoryCount[1]);
        printf("  Filters: %d\n", categoryCount[2]);
        printf("  Distortion: %d\n", categoryCount[3]);
        printf("  Modulation: %d\n", categoryCount[4]);
        printf("  Time: %d\n", categoryCount[5]);
        printf("  Spatial: %d\n", categoryCount[6]);
        printf("  Utility: %d\n", categoryCount[7]);
    }
}

/**
 * Usage Examples:
 * 
 * // Get info about an engine
 * auto* info = SimplifiedEngineMapping::getEngineInfo(ENGINE_BIT_CRUSHER);
 * printf("Engine %d: %s (%s)\n", info->id, info->name, info->className);
 * 
 * // Get all distortion engines
 * auto distortionEngines = SimplifiedEngineMapping::getEnginesByCategory("Distortion");
 * 
 * // Get all Platinum engines
 * auto platinumEngines = SimplifiedEngineMapping::getPlatinumEngines();
 * 
 * // Print statistics
 * SimplifiedEngineMapping::printEngineStatistics();
 */
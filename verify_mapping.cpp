// Comprehensive verification of parameter mapping against actual engines
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "JUCE_Plugin/Source/ParameterControlMap.h"
// EngineDefinitions.h not needed for this test

// Expected parameter counts from the actual engine implementations
std::map<int, std::pair<std::string, int>> EXPECTED_ENGINES = {
    {0, {"ENGINE_NONE", 0}},
    {1, {"VintageOptoCompressor", 8}},  
    {2, {"ClassicCompressor", 8}},
    {3, {"TransientShaper", 8}},
    {4, {"NoiseGate", 8}},
    {5, {"MasteringLimiter", 8}},
    {6, {"TubeSaturator", 8}},
    {7, {"TapeEmulation", 8}},
    {8, {"BitCrusher", 8}},
    {9, {"WaveFolder", 7}},
    {10, {"HarmonicExciter", 8}},
    {11, {"VintageEQ", 6}},
    {12, {"GraphicEQ", 8}},
    {13, {"ParametricEQ", 8}},
    {14, {"VocalEQ", 8}},
    {15, {"DynamicEQ", 8}},
    {16, {"ClassicFilter", 8}},
    {17, {"StateVariableFilter", 8}},
    {18, {"CombFilter", 3}},
    {19, {"AutoWah", 7}},
    {20, {"EnvelopeFilter", 7}},
    {21, {"AnalogChorus", 7}},
    {22, {"DimensionExpander", 4}},
    {23, {"ClassicFlanger", 6}},
    {24, {"AnalogPhaser", 8}},
    {25, {"ClassicTremolo", 8}},
    {26, {"AutoPan", 8}},
    {27, {"RotarySpeaker", 8}},
    {28, {"RingModulator", 4}},
    {29, {"PitchShifter", 8}},
    {30, {"IntelligentHarmonizer", 6}},
    {31, {"FrequencyShifter", 7}},
    {32, {"Vocoder", 5}},
    {33, {"SimpleDelay", 7}},
    {34, {"PingPongDelay", 8}},
    {35, {"TapeDelay", 7}},
    {36, {"BucketBrigadeDelay", 7}},
    {37, {"MultiTapDelay", 6}},
    {38, {"PlateReverb", 8}},
    {39, {"SpringReverb", 8}},
    {40, {"HallReverb", 8}},
    {41, {"RoomReverb", 8}},
    {42, {"ShimmerReverb", 8}},
    {43, {"GatedReverb", 8}},
    {44, {"ConvolutionReverb", 6}},
    {45, {"StereoImager", 8}},
    {46, {"AutoGain", 6}},
    {47, {"MidSideEncoder", 6}},
    {48, {"MonoMaker", 7}},
    {49, {"SimplePitchShift", 8}},
    {50, {"FormantShifter", 8}},
    {51, {"DetuneDoubler", 8}},
    {52, {"OctaveGenerator", 8}},
    {53, {"ChordHarmonizer", 8}},
    {54, {"GranularPitchShifter", 8}},
    {55, {"ChaosGenerator", 8}},
    {56, {"SpectralFreeze", 7}}
};

int main() {
    std::cout << "=== COMPREHENSIVE PARAMETER MAPPING VERIFICATION ===" << std::endl;
    std::cout << "Checking all 57 engines (0-56)..." << std::endl << std::endl;
    
    int errors = 0;
    int warnings = 0;
    
    // Check each engine
    for (const auto& [engineId, engineInfo] : EXPECTED_ENGINES) {
        const auto& [engineName, expectedCount] = engineInfo;
        auto& params = ParameterControlMap::getEngineParameters(engineId);
        
        std::cout << "Engine " << engineId << " (" << engineName << "):" << std::endl;
        
        // Check parameter count
        if (engineId == 0) {
            if (!params.empty()) {
                std::cout << "  ERROR: ENGINE_NONE should have 0 parameters, has " 
                          << params.size() << std::endl;
                errors++;
            } else {
                std::cout << "  ✓ Correctly has 0 parameters" << std::endl;
            }
        } else {
            if (params.size() != expectedCount) {
                std::cout << "  ERROR: Expected " << expectedCount << " parameters, got " 
                          << params.size() << std::endl;
                errors++;
                
                // If using defaults, that's a major problem
                if (params.size() == 8 && params[0].name == "Param 1") {
                    std::cout << "  CRITICAL: Engine is using default fallback parameters!" << std::endl;
                }
            } else {
                std::cout << "  ✓ Correct parameter count: " << params.size() << std::endl;
            }
            
            // Check parameter names and control types
            for (size_t i = 0; i < params.size(); ++i) {
                if (params[i].name.empty()) {
                    std::cout << "    ERROR: Parameter " << i << " has empty name" << std::endl;
                    errors++;
                } else if (params[i].name == "Param " + std::to_string(i + 1)) {
                    std::cout << "    WARNING: Parameter " << i << " has generic name: " 
                              << params[i].name << std::endl;
                    warnings++;
                }
                
                // Verify control type is valid
                if (params[i].control < 0 || params[i].control > 3) {
                    std::cout << "    ERROR: Parameter " << i << " has invalid control type: " 
                              << params[i].control << std::endl;
                    errors++;
                }
            }
        }
    }
    
    // Test that out-of-range engines get defaults
    std::cout << std::endl << "=== TESTING FALLBACK BEHAVIOR ===" << std::endl;
    
    auto& params57 = ParameterControlMap::getEngineParameters(57);
    auto& params100 = ParameterControlMap::getEngineParameters(100);
    
    if (params57.size() != 8 || params57[0].name != "Param 1") {
        std::cout << "ERROR: Engine 57 should return default parameters" << std::endl;
        errors++;
    } else {
        std::cout << "✓ Engine 57 correctly returns default parameters" << std::endl;
    }
    
    if (params100.size() != 8 || params100[0].name != "Param 1") {
        std::cout << "ERROR: Engine 100 should return default parameters" << std::endl;
        errors++;
    } else {
        std::cout << "✓ Engine 100 correctly returns default parameters" << std::endl;
    }
    
    // Summary
    std::cout << std::endl << "=== VERIFICATION SUMMARY ===" << std::endl;
    std::cout << "Total Errors: " << errors << std::endl;
    std::cout << "Total Warnings: " << warnings << std::endl;
    
    if (errors == 0) {
        std::cout << "✓ ALL ENGINES PROPERLY MAPPED!" << std::endl;
    } else {
        std::cout << "✗ MAPPING HAS ERRORS THAT NEED FIXING!" << std::endl;
        return 1;
    }
    
    return 0;
}
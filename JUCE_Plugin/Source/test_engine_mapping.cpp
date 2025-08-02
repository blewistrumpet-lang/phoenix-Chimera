#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <cassert>
#include "EngineTypes.h"
#include "ParameterDefinitions.h"

// Test harness for engine ID mapping

// Copy of the mapping from PluginProcessor.cpp
static const std::map<int, int> engineIDToChoiceMap = {
    {-1, 0},  // ENGINE_BYPASS (-1) -> "Bypass" is at index 0
    {38, 1},  // ENGINE_K_STYLE -> "K-Style Overdrive" at index 1
    {1, 2},   // ENGINE_TAPE_ECHO -> "Tape Echo" at index 2
    {3, 3},   // ENGINE_PLATE_REVERB -> "Plate Reverb" at index 3
    {36, 4},  // ENGINE_RODENT_DISTORTION -> "Rodent Distortion" at index 4
    {35, 5},  // ENGINE_MUFF_FUZZ -> "Muff Fuzz" at index 5
    {22, 6},  // ENGINE_CLASSIC_TREMOLO -> "Classic Tremolo" at index 6
    {8, 7},   // ENGINE_MAGNETIC_DRUM_ECHO -> "Magnetic Drum Echo" at index 7
    {9, 8},   // ENGINE_BUCKET_BRIGADE_DELAY -> "Bucket Brigade Delay" at index 8
    {53, 9},  // ENGINE_DIGITAL_DELAY -> "Digital Delay" at index 9
    {21, 10}, // ENGINE_HARMONIC_TREMOLO -> "Harmonic Tremolo" at index 10
    {24, 11}, // ENGINE_ROTARY_SPEAKER -> "Rotary Speaker" at index 11
    {44, 12}, // ENGINE_DETUNE_DOUBLER -> "Detune Doubler" at index 12
    {28, 13}, // ENGINE_LADDER_FILTER -> "Ladder Filter" at index 13
    {30, 14}, // ENGINE_FORMANT_FILTER -> "Formant Filter" at index 14
    {7, 15},  // ENGINE_VCA_COMPRESSOR -> "Classic Compressor" at index 15
    {29, 16}, // ENGINE_STATE_VARIABLE_FILTER -> "State Variable Filter" at index 16
    {11, 17}, // ENGINE_DIGITAL_CHORUS -> "Stereo Chorus" at index 17
    {39, 18}, // ENGINE_SPECTRAL_FREEZE -> "Spectral Freeze" at index 18
    {16, 19}, // ENGINE_GRANULAR_CLOUD -> "Granular Cloud" at index 19
    {15, 20}, // ENGINE_RING_MODULATOR -> "Analog Ring Modulator" at index 20
    {34, 21}, // ENGINE_MULTIBAND_SATURATOR -> "Multiband Saturator" at index 21
    {23, 22}, // ENGINE_COMB_RESONATOR -> "Comb Resonator" at index 22
    {14, 23}, // ENGINE_PITCH_SHIFTER -> "Pitch Shifter" at index 23
    {45, 24}, // ENGINE_PHASED_VOCODER -> "Phased Vocoder" at index 24
    {4, 25},  // ENGINE_CONVOLUTION_REVERB -> "Convolution Reverb" at index 25
    {33, 26}, // ENGINE_BIT_CRUSHER -> "Bit Crusher" at index 26
    {19, 27}, // ENGINE_FREQUENCY_SHIFTER -> "Frequency Shifter" at index 27
    {31, 28}, // ENGINE_WAVE_FOLDER -> "Wave Folder" at index 28
    {2, 29},  // ENGINE_SHIMMER_REVERB -> "Shimmer Reverb" at index 29
    {17, 30}, // ENGINE_VOCAL_FORMANT -> "Vocal Formant Filter" at index 30
    {20, 31}, // ENGINE_TRANSIENT_SHAPER -> "Transient Shaper" at index 31
    {18, 32}, // ENGINE_DIMENSION_EXPANDER -> "Dimension Expander" at index 32
    {12, 33}, // ENGINE_ANALOG_PHASER -> "Analog Phaser" at index 33
    {48, 34}, // ENGINE_ENVELOPE_FILTER -> "Envelope Filter" at index 34
    {43, 35}, // ENGINE_GATED_REVERB -> "Gated Reverb" at index 35
    {32, 36}, // ENGINE_HARMONIC_EXCITER -> "Harmonic Exciter" at index 36
    {49, 37}, // ENGINE_FEEDBACK_NETWORK -> "Feedback Network" at index 37
    {42, 38}, // ENGINE_INTELLIGENT_HARMONIZER -> "Intelligent Harmonizer" at index 38
    {27, 39}, // ENGINE_PARAMETRIC_EQ -> "Parametric EQ" at index 39
    {50, 40}, // ENGINE_MASTERING_LIMITER -> "Mastering Limiter" at index 40
    {47, 41}, // ENGINE_NOISE_GATE -> "Noise Gate" at index 41
    {6, 42},  // ENGINE_OPTO_COMPRESSOR -> "Vintage Opto" at index 42
    {46, 43}, // ENGINE_SPECTRAL_GATE -> "Spectral Gate" at index 43
    {41, 44}, // ENGINE_CHAOS_GENERATOR -> "Chaos Generator" at index 44
    {40, 45}, // ENGINE_BUFFER_REPEAT -> "Buffer Repeat" at index 45
    {26, 46}, // ENGINE_VINTAGE_CONSOLE_EQ -> "Vintage Console EQ" at index 46
    {25, 47}, // ENGINE_MID_SIDE_PROCESSOR -> "Mid/Side Processor" at index 47
    {0, 48},  // ENGINE_VINTAGE_TUBE -> "Vintage Tube Preamp" at index 48
    {5, 49},  // ENGINE_SPRING_REVERB -> "Spring Reverb" at index 49
    {52, 50}, // ENGINE_RESONANT_CHORUS -> "Resonant Chorus" at index 50
    {51, 51}, // ENGINE_STEREO_WIDENER -> "Stereo Widener" at index 51
    {54, 52}, // ENGINE_DYNAMIC_EQ -> "Dynamic EQ" at index 52
    {55, 53}  // ENGINE_STEREO_IMAGER -> "Stereo Imager" at index 53
};

// Expected engine choices array order
static const std::vector<std::string> expectedChoices = {
    "Bypass", "K-Style Overdrive", "Tape Echo", "Plate Reverb",
    "Rodent Distortion", "Muff Fuzz", "Classic Tremolo",
    "Magnetic Drum Echo", "Bucket Brigade Delay", "Digital Delay",
    "Harmonic Tremolo", "Rotary Speaker", "Detune Doubler",
    "Ladder Filter", "Formant Filter", "Classic Compressor",
    "State Variable Filter", "Stereo Chorus", "Spectral Freeze",
    "Granular Cloud", "Analog Ring Modulator", "Multiband Saturator",
    "Comb Resonator", "Pitch Shifter", "Phased Vocoder",
    "Convolution Reverb", "Bit Crusher", "Frequency Shifter",
    "Wave Folder", "Shimmer Reverb", "Vocal Formant Filter",
    "Transient Shaper", "Dimension Expander", "Analog Phaser",
    "Envelope Filter", "Gated Reverb", "Harmonic Exciter",
    "Feedback Network", "Intelligent Harmonizer", "Parametric EQ",
    "Mastering Limiter", "Noise Gate", "Vintage Opto",
    "Spectral Gate", "Chaos Generator", "Buffer Repeat",
    "Vintage Console EQ", "Mid/Side Processor", "Vintage Tube Preamp",
    "Spring Reverb", "Resonant Chorus", "Stereo Widener",
    "Dynamic EQ", "Stereo Imager"
};

void testEngineMapping() {
    std::cout << "Testing Engine ID to Choice Index Mapping..." << std::endl;
    
    // Test 1: Verify all entries in the map
    std::cout << "\nTest 1: Verifying mapping entries..." << std::endl;
    int totalEngines = 0;
    int missingEngines = 0;
    
    // Check all defined engines (including gaps)
    std::vector<int> allEngineIDs = {
        ENGINE_BYPASS,
        ENGINE_VINTAGE_TUBE, ENGINE_TAPE_ECHO, ENGINE_SHIMMER_REVERB, ENGINE_PLATE_REVERB,
        ENGINE_CONVOLUTION_REVERB, ENGINE_SPRING_REVERB, ENGINE_OPTO_COMPRESSOR, ENGINE_VCA_COMPRESSOR,
        ENGINE_MAGNETIC_DRUM_ECHO, ENGINE_BUCKET_BRIGADE_DELAY,
        // 10 is commented out
        ENGINE_DIGITAL_CHORUS, ENGINE_ANALOG_PHASER,
        // 13 is commented out
        ENGINE_PITCH_SHIFTER, ENGINE_RING_MODULATOR, ENGINE_GRANULAR_CLOUD, ENGINE_VOCAL_FORMANT,
        ENGINE_DIMENSION_EXPANDER, ENGINE_FREQUENCY_SHIFTER, ENGINE_TRANSIENT_SHAPER,
        ENGINE_HARMONIC_TREMOLO, ENGINE_CLASSIC_TREMOLO, ENGINE_COMB_RESONATOR, ENGINE_ROTARY_SPEAKER,
        ENGINE_MID_SIDE_PROCESSOR, ENGINE_VINTAGE_CONSOLE_EQ, ENGINE_PARAMETRIC_EQ,
        ENGINE_LADDER_FILTER, ENGINE_STATE_VARIABLE_FILTER, ENGINE_FORMANT_FILTER,
        ENGINE_WAVE_FOLDER, ENGINE_HARMONIC_EXCITER, ENGINE_BIT_CRUSHER, ENGINE_MULTIBAND_SATURATOR,
        ENGINE_MUFF_FUZZ, ENGINE_RODENT_DISTORTION,
        // 37 is commented out
        ENGINE_K_STYLE, ENGINE_SPECTRAL_FREEZE, ENGINE_BUFFER_REPEAT, ENGINE_CHAOS_GENERATOR,
        ENGINE_INTELLIGENT_HARMONIZER, ENGINE_GATED_REVERB, ENGINE_DETUNE_DOUBLER,
        ENGINE_PHASED_VOCODER, ENGINE_SPECTRAL_GATE, ENGINE_NOISE_GATE, ENGINE_ENVELOPE_FILTER,
        ENGINE_FEEDBACK_NETWORK, ENGINE_MASTERING_LIMITER, ENGINE_STEREO_WIDENER,
        ENGINE_RESONANT_CHORUS, ENGINE_DIGITAL_DELAY, ENGINE_DYNAMIC_EQ, ENGINE_STEREO_IMAGER
    };
    
    for (int engineID : allEngineIDs) {
        totalEngines++;
        auto it = engineIDToChoiceMap.find(engineID);
        if (it == engineIDToChoiceMap.end()) {
            std::cout << "  ERROR: Missing mapping for engine ID " << engineID 
                      << " (" << getEngineTypeName(engineID) << ")" << std::endl;
            missingEngines++;
        }
    }
    
    std::cout << "  Total engines checked: " << totalEngines << std::endl;
    std::cout << "  Missing mappings: " << missingEngines << std::endl;
    
    // Test 2: Verify reverse mapping
    std::cout << "\nTest 2: Verifying reverse mapping (round-trip)..." << std::endl;
    std::map<int, int> reverseMap;
    for (const auto& pair : engineIDToChoiceMap) {
        reverseMap[pair.second] = pair.first;
    }
    
    int roundTripErrors = 0;
    for (const auto& pair : engineIDToChoiceMap) {
        int engineID = pair.first;
        int choiceIndex = pair.second;
        
        auto it = reverseMap.find(choiceIndex);
        if (it == reverseMap.end() || it->second != engineID) {
            std::cout << "  ERROR: Round-trip failed for engine ID " << engineID << std::endl;
            roundTripErrors++;
        }
    }
    std::cout << "  Round-trip errors: " << roundTripErrors << std::endl;
    
    // Test 3: Verify choice array size matches
    std::cout << "\nTest 3: Verifying choice array size..." << std::endl;
    std::cout << "  Expected choices: " << expectedChoices.size() << std::endl;
    std::cout << "  Mapped entries: " << engineIDToChoiceMap.size() << std::endl;
    
    if (expectedChoices.size() != engineIDToChoiceMap.size()) {
        std::cout << "  ERROR: Size mismatch!" << std::endl;
    }
    
    // Test 4: Test specific known problematic engine
    std::cout << "\nTest 4: Testing CHAOS_GENERATOR (ID 41)..." << std::endl;
    auto chaosIt = engineIDToChoiceMap.find(ENGINE_CHAOS_GENERATOR);
    if (chaosIt != engineIDToChoiceMap.end()) {
        int choiceIndex = chaosIt->second;
        std::cout << "  ENGINE_CHAOS_GENERATOR (41) -> choice index " << choiceIndex << std::endl;
        if (choiceIndex < expectedChoices.size()) {
            std::cout << "  Expected: \"Chaos Generator\"" << std::endl;
            std::cout << "  Got: \"" << expectedChoices[choiceIndex] << "\"" << std::endl;
        }
    } else {
        std::cout << "  ERROR: ENGINE_CHAOS_GENERATOR not found in mapping!" << std::endl;
    }
    
    // Test 5: Check for duplicate choice indices
    std::cout << "\nTest 5: Checking for duplicate choice indices..." << std::endl;
    std::map<int, int> choiceCount;
    for (const auto& pair : engineIDToChoiceMap) {
        choiceCount[pair.second]++;
    }
    
    int duplicates = 0;
    for (const auto& pair : choiceCount) {
        if (pair.second > 1) {
            std::cout << "  ERROR: Choice index " << pair.first 
                      << " is used " << pair.second << " times!" << std::endl;
            duplicates++;
        }
    }
    std::cout << "  Duplicate indices: " << duplicates << std::endl;
    
    // Summary
    std::cout << "\n=== TEST SUMMARY ===" << std::endl;
    bool allPassed = (missingEngines == 0 && roundTripErrors == 0 && 
                      expectedChoices.size() == engineIDToChoiceMap.size() && 
                      duplicates == 0);
    
    if (allPassed) {
        std::cout << "All tests PASSED!" << std::endl;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
        std::cout << "Issues found:" << std::endl;
        if (missingEngines > 0) std::cout << "  - Missing engine mappings" << std::endl;
        if (roundTripErrors > 0) std::cout << "  - Round-trip conversion errors" << std::endl;
        if (expectedChoices.size() != engineIDToChoiceMap.size()) 
            std::cout << "  - Size mismatch between choices and mappings" << std::endl;
        if (duplicates > 0) std::cout << "  - Duplicate choice indices" << std::endl;
    }
}

int main() {
    testEngineMapping();
    return 0;
}
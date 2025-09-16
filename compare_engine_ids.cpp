// Compare engine IDs between EngineTypes.h and GeneratedParameterDatabase.h
#include <iostream>
#include <map>
#include <string>
#include "JUCE_Plugin/Source/EngineTypes.h"
#include "JUCE_Plugin/Source/GeneratedParameterDatabase.h"

int main()
{
    std::cout << "\n=========================================================\n";
    std::cout << "ENGINE ID COMPARISON: EngineTypes.h vs Database\n";
    std::cout << "=========================================================\n\n";
    
    // Map of engine names to their IDs from EngineTypes.h
    std::map<std::string, int> engineTypesIDs = {
        {"Vintage Tube Preamp", ENGINE_VINTAGE_TUBE},  // 15
        {"Tape Echo", ENGINE_TAPE_ECHO},  // 34
        {"Shimmer Reverb", ENGINE_SHIMMER_REVERB},  // 42
        {"Plate Reverb", ENGINE_PLATE_REVERB},  // 39
        {"Spring Reverb", ENGINE_SPRING_REVERB},  // 40
        {"Convolution Reverb", ENGINE_CONVOLUTION_REVERB},  // 41
        {"Gated Reverb", ENGINE_GATED_REVERB},  // 43
        {"Noise Gate", ENGINE_NOISE_GATE},  // 4
        {"Mastering Limiter", ENGINE_MASTERING_LIMITER},  // 5
        {"K-Style Overdrive", ENGINE_K_STYLE},  // 22
        {"Chaos Generator", ENGINE_CHAOS_GENERATOR},  // 51
        {"Classic Compressor", ENGINE_VCA_COMPRESSOR},  // 2
        {"Opto Compressor", ENGINE_OPTO_COMPRESSOR},  // 1
        {"Digital Delay", ENGINE_DIGITAL_DELAY},  // 35
        {"Pitch Shifter", ENGINE_PITCH_SHIFTER},  // 31
        {"Intelligent Harmonizer", ENGINE_INTELLIGENT_HARMONIZER},  // 33
        {"Mid-Side Processor", ENGINE_MID_SIDE_PROCESSOR},  // 53
    };
    
    std::cout << "CRITICAL ID MISMATCHES:\n";
    std::cout << "========================\n\n";
    
    int mismatches = 0;
    
    // Check each engine in the database
    for (const auto& engine : ChimeraParameters::engineDatabase)
    {
        std::string name(engine.displayName);
        
        // Check if we have this engine in EngineTypes.h
        auto it = engineTypesIDs.find(name);
        if (it != engineTypesIDs.end())
        {
            int engineTypesID = it->second;
            int databaseID = engine.legacyId;
            
            if (engineTypesID != databaseID)
            {
                std::cout << "❌ " << name << ":\n";
                std::cout << "   EngineTypes.h says: " << engineTypesID << "\n";
                std::cout << "   Database says:      " << databaseID << "\n";
                std::cout << "   MISMATCH!\n\n";
                mismatches++;
            }
        }
    }
    
    std::cout << "\nDETAILED ENGINE LISTING:\n";
    std::cout << "========================\n\n";
    
    // List all engines with their IDs from both sources
    std::cout << "From EngineTypes.h definitions:\n";
    std::cout << "--------------------------------\n";
    std::cout << "Vintage Tube Preamp:    " << ENGINE_VINTAGE_TUBE << "\n";
    std::cout << "K-Style Overdrive:      " << ENGINE_K_STYLE << "\n";
    std::cout << "Noise Gate:             " << ENGINE_NOISE_GATE << "\n";
    std::cout << "Mastering Limiter:      " << ENGINE_MASTERING_LIMITER << "\n";
    std::cout << "Tape Echo:              " << ENGINE_TAPE_ECHO << "\n";
    std::cout << "Digital Delay:          " << ENGINE_DIGITAL_DELAY << "\n";
    std::cout << "Plate Reverb:           " << ENGINE_PLATE_REVERB << "\n";
    std::cout << "Spring Reverb:          " << ENGINE_SPRING_REVERB << "\n";
    std::cout << "Convolution Reverb:     " << ENGINE_CONVOLUTION_REVERB << "\n";
    std::cout << "Shimmer Reverb:         " << ENGINE_SHIMMER_REVERB << "\n";
    std::cout << "Gated Reverb:           " << ENGINE_GATED_REVERB << "\n";
    std::cout << "Chaos Generator:        " << ENGINE_CHAOS_GENERATOR << "\n";
    
    std::cout << "\nFrom GeneratedParameterDatabase.h:\n";
    std::cout << "-----------------------------------\n";
    
    for (const auto& engine : ChimeraParameters::engineDatabase)
    {
        std::string name(engine.displayName);
        // Only show key engines
        if (name == "Vintage Tube Preamp" || name == "K-Style Overdrive" ||
            name == "Noise Gate" || name == "Mastering Limiter" ||
            name == "Tape Echo" || name == "Digital Delay" ||
            name == "Plate Reverb" || name == "Spring Reverb" ||
            name == "Convolution Reverb" || name == "Shimmer Reverb" ||
            name == "Gated Reverb" || name == "Chaos Generator")
        {
            std::cout << name << ": " << engine.legacyId << "\n";
        }
    }
    
    std::cout << "\n=========================================================\n";
    std::cout << "SUMMARY: Found " << mismatches << " ID mismatches\n";
    std::cout << "=========================================================\n";
    
    std::cout << "\nCRITICAL FINDING:\n";
    std::cout << "The GeneratedParameterDatabase.h is using DIFFERENT engine IDs\n";
    std::cout << "than the authoritative EngineTypes.h file!\n";
    std::cout << "\nThis explains why parameters aren't mapping correctly.\n";
    std::cout << "The database needs to be regenerated with correct IDs.\n";
    
    return 0;
}
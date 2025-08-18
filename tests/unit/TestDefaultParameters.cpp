#include "DefaultParameterValues.h"
#include "ParameterDefinitions.h"
#include <iostream>
#include <iomanip>

/**
 * Test program to verify and demonstrate the default parameter system
 */

void printEngineDefaults(int engineId, const std::string& name) {
    auto defaults = DefaultParameterValues::getDefaultParameters(engineId);
    
    std::cout << "\n=== " << name << " (ID: " << engineId << ") ===\n";
    std::cout << "Parameters: " << defaults.size() << "\n";
    
    for (const auto& [paramIndex, value] : defaults) {
        std::cout << "  Param " << std::setw(2) << paramIndex << ": " 
                  << std::fixed << std::setprecision(3) << value << "\n";
    }
}

void printCategorySummary() {
    auto categories = DefaultParameterValues::getEnginesByCategory();
    
    std::cout << "\n=== ENGINE CATEGORIES ===\n";
    for (const auto& [category, engines] : categories) {
        std::cout << category << ": " << engines.size() << " engines\n";
        for (int engineId : engines) {
            auto allDefaults = DefaultParameterValues::getAllEngineDefaults();
            for (const auto& info : allDefaults) {
                if (info.engineId == engineId) {
                    std::cout << "  - " << info.name << " (" << info.defaults.size() << " params)\n";
                    break;
                }
            }
        }
        std::cout << "\n";
    }
}

void validateDefaultParameters() {
    std::cout << "\n=== VALIDATION REPORT ===\n";
    
    auto allDefaults = DefaultParameterValues::getAllEngineDefaults();
    int totalEngines = 0;
    int enginesWithDefaults = 0;
    int totalParameters = 0;
    
    for (const auto& info : allDefaults) {
        totalEngines++;
        if (!info.defaults.empty()) {
            enginesWithDefaults++;
            totalParameters += info.defaults.size();
        }
    }
    
    std::cout << "Total engines: " << totalEngines << "\n";
    std::cout << "Engines with defaults: " << enginesWithDefaults << "\n";
    std::cout << "Total parameters configured: " << totalParameters << "\n";
    std::cout << "Coverage: " << std::fixed << std::setprecision(1) 
              << (100.0f * enginesWithDefaults / totalEngines) << "%\n";
    
    // Check for engines missing defaults
    std::cout << "\nEngines missing defaults:\n";
    for (const auto& info : allDefaults) {
        if (info.defaults.empty()) {
            std::cout << "  - " << info.name << " (ID: " << info.engineId << ")\n";
        }
    }
}

void demonstrateKeyEngines() {
    std::cout << "\n=== KEY ENGINE DEMONSTRATIONS ===\n";
    
    // Show examples from each major category
    printEngineDefaults(ENGINE_K_STYLE, "K-Style Overdrive");
    printEngineDefaults(ENGINE_VINTAGE_OPTO_COMPRESSOR, "Vintage Opto Compressor");
    printEngineDefaults(ENGINE_MASTERING_LIMITER, "Mastering Limiter");
    printEngineDefaults(ENGINE_INTELLIGENT_HARMONIZER, "Intelligent Harmonizer");
    printEngineDefaults(ENGINE_TAPE_ECHO, "Tape Echo");
    printEngineDefaults(ENGINE_PLATE_REVERB, "Plate Reverb");
    printEngineDefaults(ENGINE_STEREO_CHORUS, "Stereo Chorus");
    printEngineDefaults(ENGINE_LADDER_FILTER, "Ladder Filter");
}

int main() {
    std::cout << "Chimera Phoenix Default Parameter System Test\n";
    std::cout << "============================================\n";
    
    // Validate the system
    validateDefaultParameters();
    
    // Show category organization
    printCategorySummary();
    
    // Demonstrate key engines
    demonstrateKeyEngines();
    
    std::cout << "\n=== DESIGN PRINCIPLES SUMMARY ===\n";
    std::cout << "1. Safety First: No harsh or damaging sounds\n";
    std::cout << "2. Musical Utility: Immediate musical results\n";
    std::cout << "3. Moderate Values: Most params in 0.3-0.7 range\n";
    std::cout << "4. Conservative Drive: Drive/gain 0.2-0.4\n";
    std::cout << "5. Appropriate Mix: Effects 100% wet, distortions 70-80%\n";
    std::cout << "6. Musical Timing: Note-based time values\n";
    std::cout << "7. Controlled Feedback: 0.2-0.4 to avoid runaway\n";
    std::cout << "8. Smooth Resonance: 0.3-0.5 for musicality\n";
    std::cout << "9. Unity Gain: Maintain consistent levels\n";
    std::cout << "10. First Impression: Inspiring results immediately\n";
    
    return 0;
}
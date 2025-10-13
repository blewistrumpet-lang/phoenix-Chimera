/**
 * Test that Trinity presets actually load engines into the plugin
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PluginProcessor.h"

using namespace std::chrono_literals;

int main() {
    juce::ScopedJuceInitialiser_GUI juce_init;
    
    std::cout << "\n========== TRINITY PRESET LOADING TEST ==========\n" << std::endl;
    
    // Create processor
    ChimeraAudioProcessor processor;
    
    // Check initial state - should have no engines loaded
    std::cout << "Initial state:" << std::endl;
    for (int i = 0; i < 6; ++i) {
        int engineId = processor.getEngineIDForSlot(i);
        std::cout << "  Slot " << i << ": Engine ID " << engineId << std::endl;
    }
    
    // Create a test preset JSON (simulating what Trinity returns)
    std::string presetJson = R"({
        "success": true,
        "preset": {
            "name": "Test Metal Preset",
            "slots": [
                {"engine_id": 22, "parameters": [0.7, 0.5, 0.8]},
                {"engine_id": 15, "parameters": [0.9, 0.3, 0.6]},
                {"engine_id": 39, "parameters": [0.5, 0.5, 0.5]}
            ]
        }
    })";
    
    auto jsonResponse = juce::JSON::parse(presetJson);
    auto preset = jsonResponse["preset"];
    
    std::cout << "\nApplying preset: " << preset["name"].toString() << std::endl;
    
    // Apply the preset like PluginEditorFull does
    if (preset.hasProperty("slots")) {
        auto slots = preset["slots"];
        if (slots.isArray()) {
            for (int i = 0; i < juce::jmin(6, slots.size()); ++i) {
                auto slot = slots[i];
                if (slot.hasProperty("engine_id")) {
                    int engineId = slot["engine_id"];
                    
                    // This is the CRITICAL LINE that was missing before
                    processor.setSlotEngine(i, engineId);
                    
                    std::cout << "  Setting slot " << i << " to engine " << engineId << std::endl;
                }
            }
        }
    }
    
    // Verify engines were loaded
    std::cout << "\nAfter applying preset:" << std::endl;
    int loadedCount = 0;
    for (int i = 0; i < 6; ++i) {
        int engineId = processor.getEngineIDForSlot(i);
        std::cout << "  Slot " << i << ": Engine ID " << engineId;
        if (engineId > 0) {
            std::cout << " ✓ LOADED";
            loadedCount++;
        }
        std::cout << std::endl;
    }
    
    // Test result
    std::cout << "\n========== TEST RESULT ==========\n" << std::endl;
    if (loadedCount == 3) {
        std::cout << "✅ SUCCESS: All 3 engines loaded correctly!" << std::endl;
        std::cout << "  - Slot 0: K-Style Overdrive (ID 22)" << std::endl;
        std::cout << "  - Slot 1: Vintage Tube (ID 15)" << std::endl;
        std::cout << "  - Slot 2: Plate Reverb (ID 39)" << std::endl;
        return 0;
    } else {
        std::cout << "❌ FAILURE: Only " << loadedCount << " engines loaded (expected 3)" << std::endl;
        return 1;
    }
}
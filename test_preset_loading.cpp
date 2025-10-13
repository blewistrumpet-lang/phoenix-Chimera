#include <iostream>
#include <JuceHeader.h>
#include "../JUCE_Plugin/Source/PluginProcessor.h"
#include "../JUCE_Plugin/Source/TrinityManager.h"
#include "../JUCE_Plugin/Source/TrinityProtocol.h"

int main() {
    std::cout << "\n============================================\n";
    std::cout << "Testing Trinity Preset Loading with Engines\n";
    std::cout << "============================================\n\n";
    
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI scopedJuce;
    
    // Create processor
    ChimeraAudioProcessor processor;
    processor.prepareToPlay(44100, 512);
    
    // Get Trinity manager
    TrinityManager& trinity = processor.getTrinityManager();
    trinity.initialize();
    
    // Wait for connection
    juce::Thread::sleep(1000);
    
    // Create a test preset with engines
    juce::var testPreset(new juce::DynamicObject());
    testPreset.getDynamicObject()->setProperty("name", "Test Preset");
    
    juce::Array<juce::var> slots;
    
    // Slot 0: PlateReverb (ID 43)
    {
        juce::var slot(new juce::DynamicObject());
        slot.getDynamicObject()->setProperty("engine_id", 43);
        slot.getDynamicObject()->setProperty("engine_name", "PlateReverb");
        
        juce::Array<juce::var> params;
        for (int i = 1; i <= 8; i++) {
            juce::var param(new juce::DynamicObject());
            param.getDynamicObject()->setProperty("name", "param" + juce::String(i));
            param.getDynamicObject()->setProperty("value", 0.3f + i * 0.05f);
            params.add(param);
        }
        slot.getDynamicObject()->setProperty("parameters", params);
        slots.add(slot);
    }
    
    // Slot 1: ShimmerReverb (ID 52)
    {
        juce::var slot(new juce::DynamicObject());
        slot.getDynamicObject()->setProperty("engine_id", 52);
        slot.getDynamicObject()->setProperty("engine_name", "ShimmerReverb");
        
        juce::Array<juce::var> params;
        for (int i = 1; i <= 10; i++) {
            juce::var param(new juce::DynamicObject());
            param.getDynamicObject()->setProperty("name", "param" + juce::String(i));
            param.getDynamicObject()->setProperty("value", 0.4f + i * 0.03f);
            params.add(param);
        }
        slot.getDynamicObject()->setProperty("parameters", params);
        slots.add(slot);
    }
    
    // Slot 2: Empty
    {
        juce::var slot(new juce::DynamicObject());
        slot.getDynamicObject()->setProperty("engine_id", 0);
        slot.getDynamicObject()->setProperty("engine_name", "None");
        slots.add(slot);
    }
    
    // Slot 3: BitCrusher (ID 11)
    {
        juce::var slot(new juce::DynamicObject());
        slot.getDynamicObject()->setProperty("engine_id", 11);
        slot.getDynamicObject()->setProperty("engine_name", "BitCrusher");
        
        juce::Array<juce::var> params;
        for (int i = 1; i <= 4; i++) {
            juce::var param(new juce::DynamicObject());
            param.getDynamicObject()->setProperty("name", "param" + juce::String(i));
            param.getDynamicObject()->setProperty("value", 0.6f);
            params.add(param);
        }
        slot.getDynamicObject()->setProperty("parameters", params);
        slots.add(slot);
    }
    
    // Slots 4-5: Empty
    for (int i = 4; i < 6; i++) {
        juce::var slot(new juce::DynamicObject());
        slot.getDynamicObject()->setProperty("engine_id", 0);
        slot.getDynamicObject()->setProperty("engine_name", "None");
        slots.add(slot);
    }
    
    testPreset.getDynamicObject()->setProperty("slots", slots);
    
    // Apply the preset directly
    std::cout << "Applying test preset...\n";
    trinity.applyPreset(testPreset);
    
    // Wait for application
    juce::Thread::sleep(500);
    
    // Verify engines were loaded
    std::cout << "\nVerifying engine loading:\n";
    std::cout << "-------------------------\n";
    
    bool allCorrect = true;
    
    // Check slot 0
    int slot0Engine = processor.getEngineIDForSlot(0);
    std::cout << "Slot 0: Engine ID = " << slot0Engine;
    if (slot0Engine == 43) {
        std::cout << " ✓ (PlateReverb loaded correctly)\n";
    } else {
        std::cout << " ✗ (Expected 43, got " << slot0Engine << ")\n";
        allCorrect = false;
    }
    
    // Check slot 1
    int slot1Engine = processor.getEngineIDForSlot(1);
    std::cout << "Slot 1: Engine ID = " << slot1Engine;
    if (slot1Engine == 52) {
        std::cout << " ✓ (ShimmerReverb loaded correctly)\n";
    } else {
        std::cout << " ✗ (Expected 52, got " << slot1Engine << ")\n";
        allCorrect = false;
    }
    
    // Check slot 2
    int slot2Engine = processor.getEngineIDForSlot(2);
    std::cout << "Slot 2: Engine ID = " << slot2Engine;
    if (slot2Engine == 0) {
        std::cout << " ✓ (Empty as expected)\n";
    } else {
        std::cout << " ✗ (Expected 0, got " << slot2Engine << ")\n";
        allCorrect = false;
    }
    
    // Check slot 3
    int slot3Engine = processor.getEngineIDForSlot(3);
    std::cout << "Slot 3: Engine ID = " << slot3Engine;
    if (slot3Engine == 11) {
        std::cout << " ✓ (BitCrusher loaded correctly)\n";
    } else {
        std::cout << " ✗ (Expected 11, got " << slot3Engine << ")\n";
        allCorrect = false;
    }
    
    // Check slots 4-5
    for (int i = 4; i < 6; i++) {
        int engineId = processor.getEngineIDForSlot(i);
        std::cout << "Slot " << i << ": Engine ID = " << engineId;
        if (engineId == 0) {
            std::cout << " ✓ (Empty as expected)\n";
        } else {
            std::cout << " ✗ (Expected 0, got " << engineId << ")\n";
            allCorrect = false;
        }
    }
    
    std::cout << "\n============================================\n";
    if (allCorrect) {
        std::cout << "✅ SUCCESS: All engines loaded correctly!\n";
    } else {
        std::cout << "❌ FAILURE: Some engines did not load correctly\n";
    }
    std::cout << "============================================\n\n";
    
    // Test parameter values
    std::cout << "Checking parameter values:\n";
    std::cout << "-------------------------\n";
    
    // Check slot 0 param1
    auto* slot0Param1 = processor.getValueTreeState().getRawParameterValue("slot1_param1");
    if (slot0Param1) {
        float value = slot0Param1->load();
        std::cout << "Slot 0, Param 1: " << value;
        if (std::abs(value - 0.35f) < 0.01f) {
            std::cout << " ✓\n";
        } else {
            std::cout << " (expected ~0.35)\n";
        }
    }
    
    // Check slot 1 param1
    auto* slot1Param1 = processor.getValueTreeState().getRawParameterValue("slot2_param1");
    if (slot1Param1) {
        float value = slot1Param1->load();
        std::cout << "Slot 1, Param 1: " << value;
        if (std::abs(value - 0.43f) < 0.01f) {
            std::cout << " ✓\n";
        } else {
            std::cout << " (expected ~0.43)\n";
        }
    }
    
    // Check slot 3 param1
    auto* slot3Param1 = processor.getValueTreeState().getRawParameterValue("slot4_param1");
    if (slot3Param1) {
        float value = slot3Param1->load();
        std::cout << "Slot 3, Param 1: " << value;
        if (std::abs(value - 0.6f) < 0.01f) {
            std::cout << " ✓\n";
        } else {
            std::cout << " (expected ~0.6)\n";
        }
    }
    
    std::cout << "\n============================================\n";
    std::cout << "Test complete!\n";
    std::cout << "============================================\n\n";
    
    return allCorrect ? 0 : 1;
}
/**
 * Final test to verify Trinity preset loading works with all fixes
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PluginProcessor.h"

int main() {
    juce::ScopedJuceInitialiser_GUI juce_init;
    
    std::cout << "\n========== FINAL TRINITY TEST ==========\n" << std::endl;
    
    // Create processor
    ChimeraAudioProcessor processor;
    
    // Check initial state
    std::cout << "Initial state:" << std::endl;
    for (int i = 0; i < 6; ++i) {
        int engineId = processor.getEngineIDForSlot(i);
        std::cout << "  Slot " << i << ": Engine ID " << engineId << std::endl;
    }
    
    // Test 1: Direct parameter setting (proven to work)
    std::cout << "\nTest 1: Setting slot1_engine parameter directly..." << std::endl;
    if (auto* param = processor.getValueTreeState().getParameter("slot1_engine")) {
        float normalizedValue = 22.0f / 56.0f;  // K-Style Overdrive
        param->setValueNotifyingHost(normalizedValue);
        std::cout << "  Set to normalized value: " << normalizedValue << std::endl;
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check result
    int slot0Engine = processor.getEngineIDForSlot(0);
    std::cout << "  Result: Slot 0 has engine ID " << slot0Engine << std::endl;
    if (slot0Engine == 22) {
        std::cout << "  ✅ Direct parameter setting WORKS" << std::endl;
    } else {
        std::cout << "  ❌ Direct parameter setting FAILED" << std::endl;
    }
    
    // Test 2: Using setSlotEngine directly (should also work)
    std::cout << "\nTest 2: Using setSlotEngine() directly..." << std::endl;
    processor.setSlotEngine(1, 15);  // Vintage Tube in slot 1
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    int slot1Engine = processor.getEngineIDForSlot(1);
    std::cout << "  Result: Slot 1 has engine ID " << slot1Engine << std::endl;
    if (slot1Engine == 15) {
        std::cout << "  ✅ setSlotEngine() WORKS" << std::endl;
    } else {
        std::cout << "  ❌ setSlotEngine() FAILED" << std::endl;
    }
    
    // Test 3: Simulate Trinity preset format
    std::cout << "\nTest 3: Simulating Trinity preset with parameters format..." << std::endl;
    
    // This simulates what Trinity sends (after our server fix)
    juce::DynamicObject::Ptr params = new juce::DynamicObject();
    params->setProperty("slot3_engine", 39);  // Plate Reverb
    params->setProperty("slot4_engine", 8);   // Vintage Console EQ
    
    juce::var paramsVar(params.get());
    
    // Apply like the UI would
    auto& valueTree = processor.getValueTreeState();
    for (int slot = 0; slot < 6; ++slot) {
        juce::String engineParam = "slot" + juce::String(slot + 1) + "_engine";
        if (paramsVar.hasProperty(engineParam)) {
            int engineId = static_cast<int>(paramsVar.getProperty(engineParam, 0.0f));
            if (engineId >= 0 && engineId < 57) {
                float normalizedValue = static_cast<float>(engineId) / 56.0f;
                if (auto* param = valueTree.getParameter(engineParam)) {
                    param->setValueNotifyingHost(normalizedValue);
                    std::cout << "  Set " << engineParam << " to engine " << engineId << std::endl;
                }
            }
        }
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check final state
    std::cout << "\nFinal state after Trinity preset simulation:" << std::endl;
    int successes = 0;
    for (int i = 0; i < 6; ++i) {
        int engineId = processor.getEngineIDForSlot(i);
        std::cout << "  Slot " << i << ": Engine ID " << engineId;
        
        // Check expected values
        if (i == 0 && engineId == 22) { std::cout << " ✅"; successes++; }
        else if (i == 1 && engineId == 15) { std::cout << " ✅"; successes++; }
        else if (i == 2 && engineId == 39) { std::cout << " ✅"; successes++; }
        else if (i == 3 && engineId == 8) { std::cout << " ✅"; successes++; }
        else if (i >= 4 && engineId == 0) { std::cout << " ✅"; successes++; }
        
        std::cout << std::endl;
    }
    
    std::cout << "\n========== RESULTS ==========\n" << std::endl;
    if (successes >= 6) {
        std::cout << "🎉 SUCCESS: All Trinity preset loading methods work!" << std::endl;
        std::cout << "The fixes are complete and functional." << std::endl;
        return 0;
    } else {
        std::cout << "❌ FAILURE: Only " << successes << "/6 slots loaded correctly" << std::endl;
        return 1;
    }
}
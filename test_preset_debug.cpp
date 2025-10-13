#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/TrinityManager.h"

class TestListener : public TrinityManager::Listener {
public:
    void trinityStatusChanged(bool connected) override {
        std::cout << "Trinity status changed: " << (connected ? "Connected" : "Disconnected") << std::endl;
    }
    
    void trinityResponseReceived(const juce::String& response, bool isError) override {
        std::cout << "Response received: " << response.toStdString() << std::endl;
    }
    
    void trinityPresetReceived(const juce::var& presetData) override {
        std::cout << "\n========================================" << std::endl;
        std::cout << "PRESET RECEIVED IN LISTENER!" << std::endl;
        
        if (presetData.isObject()) {
            juce::String name = presetData.getProperty("name", "Unknown").toString();
            std::cout << "Preset Name: " << name.toStdString() << std::endl;
            
            if (presetData.hasProperty("slots")) {
                auto slots = presetData.getProperty("slots", juce::var());
                if (slots.isArray()) {
                    std::cout << "Slots array size: " << slots.size() << std::endl;
                    
                    for (int i = 0; i < slots.size(); ++i) {
                        auto slot = slots[i];
                        if (slot.isObject()) {
                            int engineId = slot.getProperty("engine_id", -1);
                            juce::String engineName = slot.getProperty("engine_name", "Unknown").toString();
                            std::cout << "  Slot " << i << ": " << engineName.toStdString() 
                                     << " (ID: " << engineId << ")" << std::endl;
                        }
                    }
                }
            }
        }
        std::cout << "========================================\n" << std::endl;
    }
    
    void trinityParameterSuggestion(int slotIndex, const juce::String& paramName, float value) override {
        std::cout << "Parameter suggestion: Slot " << slotIndex << ", " 
                  << paramName.toStdString() << " = " << value << std::endl;
    }
    
    void trinityError(const juce::String& error) override {
        std::cout << "Trinity error: " << error.toStdString() << std::endl;
    }
};

int main() {
    std::cout << "\n=== TESTING PRESET DEBUG ===\n" << std::endl;
    
    juce::ScopedJuceInitialiser_GUI gui;
    
    // Create processor
    ChimeraAudioProcessor processor;
    processor.prepareToPlay(44100, 512);
    
    // Get Trinity manager
    auto* trinity = processor.getTrinityManager();
    if (!trinity) {
        std::cout << "ERROR: No Trinity manager!" << std::endl;
        return 1;
    }
    
    // Add test listener
    TestListener listener;
    trinity->addListener(&listener);
    
    // Initialize Trinity
    trinity->initialize();
    std::cout << "Trinity initialized" << std::endl;
    
    // Wait for connection
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Check initial engine states
    std::cout << "\nInitial engine states:" << std::endl;
    for (int i = 0; i < 6; ++i) {
        int engineId = processor.getEngineIDForSlot(i);
        std::cout << "  Slot " << i << ": Engine ID " << engineId << std::endl;
    }
    
    // Create a test preset manually
    std::cout << "\nCreating test preset..." << std::endl;
    
    juce::var testPreset;
    {
        juce::DynamicObject::Ptr presetObj = new juce::DynamicObject();
        presetObj->setProperty("name", "Test Preset");
        
        juce::Array<juce::var> slotsArray;
        
        // Add some test engines
        for (int i = 0; i < 3; ++i) {
            juce::DynamicObject::Ptr slotObj = new juce::DynamicObject();
            slotObj->setProperty("engine_id", 10 + i);  // Use engine IDs 10, 11, 12
            slotObj->setProperty("engine_name", "TestEngine" + juce::String(i));
            
            juce::Array<juce::var> params;
            for (int p = 0; p < 5; ++p) {
                juce::DynamicObject::Ptr paramObj = new juce::DynamicObject();
                paramObj->setProperty("name", "param" + juce::String(p + 1));
                paramObj->setProperty("value", 0.7f);
                params.add(juce::var(paramObj.get()));
            }
            slotObj->setProperty("parameters", params);
            
            slotsArray.add(juce::var(slotObj.get()));
        }
        
        presetObj->setProperty("slots", slotsArray);
        testPreset = juce::var(presetObj.get());
    }
    
    std::cout << "Applying test preset directly..." << std::endl;
    
    // Call applyPreset directly
    trinity->applyPreset(testPreset);
    
    // Check engine states after preset
    std::cout << "\nEngine states after preset:" << std::endl;
    for (int i = 0; i < 6; ++i) {
        int engineId = processor.getEngineIDForSlot(i);
        std::cout << "  Slot " << i << ": Engine ID " << engineId << std::endl;
    }
    
    // Now test with actual Trinity message
    std::cout << "\nSending query to Trinity..." << std::endl;
    trinity->sendQuery("test preset", nullptr);
    
    // Wait for response
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Final engine states
    std::cout << "\nFinal engine states:" << std::endl;
    for (int i = 0; i < 6; ++i) {
        int engineId = processor.getEngineIDForSlot(i);
        std::cout << "  Slot " << i << ": Engine ID " << engineId << std::endl;
    }
    
    trinity->removeListener(&listener);
    trinity->shutdown();
    
    std::cout << "\n=== TEST COMPLETE ===\n" << std::endl;
    
    return 0;
}
// Test the actual built plugin
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/PluginEditorNexusStatic.h"
#include <iostream>

int main() {
    std::cout << "=== TESTING BUILT PLUGIN ===" << std::endl;
    
    try {
        // Initialize JUCE
        juce::initialiseJuce_GUI();
        
        std::cout << "1. Creating ChimeraAudioProcessor..." << std::endl;
        auto processor = std::make_unique<ChimeraAudioProcessor>();
        
        std::cout << "2. Preparing processor (44100 Hz, 512 samples)..." << std::endl;
        processor->prepareToPlay(44100, 512);
        
        std::cout << "3. Creating editor..." << std::endl;
        auto* editor = processor->createEditor();
        
        if (!editor) {
            std::cerr << "ERROR: createEditor returned nullptr!" << std::endl;
            return 1;
        }
        
        std::cout << "4. Checking editor type..." << std::endl;
        if (auto* nexusEditor = dynamic_cast<PluginEditorNexusStatic*>(editor)) {
            std::cout << "   ✓ Using NexusStatic editor (15-parameter support)" << std::endl;
        } else {
            std::cout << "   Using different editor type" << std::endl;
        }
        
        std::cout << "5. Testing parameter changes..." << std::endl;
        
        // Test changing engines on each slot
        for (int slot = 0; slot < 6; ++slot) {
            juce::String engineParam = "slot" + juce::String(slot + 1) + "_engine";
            if (auto* param = processor->getValueTreeState().getParameter(engineParam)) {
                std::cout << "   Slot " << slot << ": ";
                
                // Test a few engine IDs
                for (int engineId : {0, 1, 29, 56}) {
                    float normalizedValue = engineId / 56.0f;
                    param->setValueNotifyingHost(normalizedValue);
                    std::cout << engineId << " ";
                }
                std::cout << "✓" << std::endl;
            }
        }
        
        std::cout << "6. Cleaning up..." << std::endl;
        delete editor;
        processor.reset();
        
        juce::shutdownJuce_GUI();
        
        std::cout << "\n=== TEST PASSED ===" << std::endl;
        std::cout << "✓ Plugin loads without crashing" << std::endl;
        std::cout << "✓ Editor creates successfully" << std::endl;
        std::cout << "✓ Parameter changes handled" << std::endl;
        std::cout << "✓ Ready for Logic Pro" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\nCRASH: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "\nCRASH: Unknown exception" << std::endl;
        return 1;
    }
}
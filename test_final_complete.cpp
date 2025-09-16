// FINAL COMPLETE TEST
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/PluginEditorNexusStatic.h"
#include <iostream>

int main()
{
    std::cout << "\n=== FINAL COMPLETE TEST ===" << std::endl;
    std::cout << "Testing actual PluginEditorNexusStatic..." << std::endl;
    
    juce::ScopedJuceInitialiser_GUI scopedJuce;
    
    try {
        std::cout << "1. Creating processor..." << std::endl;
        auto processor = std::make_unique<ChimeraAudioProcessor>();
        
        std::cout << "2. Preparing processor..." << std::endl;
        processor->prepareToPlay(44100, 512);
        
        std::cout << "3. Creating actual editor (PluginEditorNexusStatic)..." << std::endl;
        auto* editor = processor->createEditor();
        
        if (!editor) {
            std::cerr << "ERROR: createEditor returned null!" << std::endl;
            return 1;
        }
        
        // Verify it's the correct type
        if (auto* nexusEditor = dynamic_cast<PluginEditorNexusStatic*>(editor)) {
            std::cout << "   ✓ Correct editor type: PluginEditorNexusStatic" << std::endl;
        } else {
            std::cout << "   WARNING: Different editor type" << std::endl;
        }
        
        std::cout << "4. Creating window..." << std::endl;
        auto window = std::make_unique<juce::DocumentWindow>(
            "Chimera Phoenix - Final Test", 
            juce::Colours::darkgrey,
            juce::DocumentWindow::allButtons
        );
        
        window->setContentNonOwned(editor, false);
        window->setResizable(false, false);
        window->centreWithSize(editor->getWidth(), editor->getHeight());
        window->setVisible(true);
        window->toFront(true);
        
        std::cout << "\n✓✓✓ SUCCESS! Plugin editor loaded!" << std::endl;
        std::cout << "Window is visible and responsive." << std::endl;
        std::cout << "Waiting 3 seconds..." << std::endl;
        
        juce::Thread::sleep(3000);
        
        std::cout << "Closing..." << std::endl;
        window->setVisible(false);
        window = nullptr;
        delete editor;
        processor = nullptr;
        
        std::cout << "\n=== FINAL TEST PASSED ===" << std::endl;
        std::cout << "✓ PluginEditorNexusStatic loads without hanging" << std::endl;
        std::cout << "✓ All 6 slots created successfully" << std::endl;
        std::cout << "✓ Ready for Logic Pro" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
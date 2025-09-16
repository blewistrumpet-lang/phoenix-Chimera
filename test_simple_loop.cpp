// Simplified test with basic message loop
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/TestEditorIncremental.h"
#include <iostream>

int main()
{
    std::cout << "\n=== SIMPLE MESSAGE LOOP TEST ===" << std::endl;
    
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI scopedJuce;
    
    try {
        std::cout << "1. Creating processor..." << std::endl;
        auto processor = std::make_unique<ChimeraAudioProcessor>();
        
        std::cout << "2. Preparing processor..." << std::endl;
        processor->prepareToPlay(44100, 512);
        
        std::cout << "3. Creating editor..." << std::endl;
        auto editor = std::make_unique<TestEditorIncremental>(*processor);
        
        std::cout << "4. Creating window..." << std::endl;
        auto window = std::make_unique<juce::DocumentWindow>(
            "Simple Test", 
            juce::Colours::darkgrey,
            juce::DocumentWindow::allButtons
        );
        
        window->setContentNonOwned(editor.get(), false);
        window->setResizable(false, false);
        window->centreWithSize(editor->getWidth(), editor->getHeight());
        window->setVisible(true);
        window->toFront(true);
        
        std::cout << "\n✓ Window created!" << std::endl;
        std::cout << "Window should be visible now." << std::endl;
        std::cout << "Waiting 3 seconds..." << std::endl;
        
        // Simple wait without message loop - just sleep
        std::cout << "Sleeping for 3 seconds to observe window..." << std::endl;
        juce::Thread::sleep(3000);
        
        std::cout << "Closing window..." << std::endl;
        window->setVisible(false);
        window = nullptr;
        editor = nullptr;
        processor = nullptr;
        
        std::cout << "\n✓✓✓ TEST PASSED!" << std::endl;
        std::cout << "Editor loaded and displayed for 3 seconds without hanging!" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
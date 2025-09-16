// Diagnostic test to find exact hang location
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/PluginEditorNexusStatic.h"
#include <iostream>

class DiagnosticEditor : public juce::AudioProcessorEditor
{
public:
    DiagnosticEditor(ChimeraAudioProcessor& p) : AudioProcessorEditor(&p)
    {
        std::cout << "DiagnosticEditor: Constructor start" << std::endl;
        setSize(1200, 800);
        std::cout << "DiagnosticEditor: Set size done" << std::endl;
        
        // Try creating ONE SlotComponent
        std::cout << "DiagnosticEditor: About to create SlotComponent..." << std::endl;
        auto slot = std::make_unique<SlotComponent>(0);
        std::cout << "DiagnosticEditor: SlotComponent created" << std::endl;
        
        std::cout << "DiagnosticEditor: About to addAndMakeVisible..." << std::endl;
        addAndMakeVisible(slot.get());
        std::cout << "DiagnosticEditor: addAndMakeVisible done" << std::endl;
        
        std::cout << "DiagnosticEditor: Constructor complete" << std::endl;
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
    }
};

int main()
{
    std::cout << "\n=== DIAGNOSTIC HANG TEST ===" << std::endl;
    
    juce::ScopedJuceInitialiser_GUI scopedJuce;
    
    try {
        std::cout << "Creating processor..." << std::endl;
        auto processor = std::make_unique<ChimeraAudioProcessor>();
        
        std::cout << "Creating diagnostic editor..." << std::endl;
        auto* editor = new DiagnosticEditor(*processor);
        
        std::cout << "Editor created successfully!" << std::endl;
        
        std::cout << "Creating window..." << std::endl;
        auto window = std::make_unique<juce::DocumentWindow>(
            "Diagnostic Test", 
            juce::Colours::darkgrey,
            juce::DocumentWindow::allButtons
        );
        
        window->setContentNonOwned(editor, false);
        window->centreWithSize(editor->getWidth(), editor->getHeight());
        window->setVisible(true);
        
        std::cout << "Window visible. Waiting 2 seconds..." << std::endl;
        juce::Thread::sleep(2000);
        
        std::cout << "Cleaning up..." << std::endl;
        window->setVisible(false);
        window = nullptr;
        delete editor;
        
        std::cout << "\n=== TEST PASSED ===" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed: " << e.what() << std::endl;
        return 1;
    }
}
// Test just the processor without UI
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include <iostream>

int main() {
    std::cout << "Testing processor only..." << std::endl;
    
    try {
        juce::ScopedJuceInitialiser_GUI init;
        
        std::cout << "1. Creating processor..." << std::endl;
        ChimeraAudioProcessor processor;
        
        std::cout << "2. Preparing processor..." << std::endl;
        processor.prepareToPlay(44100, 512);
        
        std::cout << "3. Processing empty buffer..." << std::endl;
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
        
        std::cout << "✓ Processor works correctly!" << std::endl;
        
        std::cout << "4. Testing createEditor..." << std::endl;
        auto* editor = processor.createEditor();
        if (editor) {
            std::cout << "✓ Editor created successfully!" << std::endl;
            delete editor;
        } else {
            std::cout << "✗ Editor is null!" << std::endl;
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
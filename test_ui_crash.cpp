#include <iostream>
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/PluginEditorNexusDynamic.h"

int main() {
    std::cout << "Testing Dynamic UI initialization..." << std::endl;
    
    // Create processor
    ChimeraAudioProcessor processor;
    std::cout << "Processor created" << std::endl;
    
    // Try to create editor
    try {
        auto* editor = processor.createEditor();
        std::cout << "Editor created successfully!" << std::endl;
        
        if (editor) {
            std::cout << "Editor type: " << typeid(*editor).name() << std::endl;
            delete editor;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "Unknown exception!" << std::endl;
        return 1;
    }
    
    std::cout << "Test passed!" << std::endl;
    return 0;
}
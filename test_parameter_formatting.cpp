#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/ParameterFormatter.h"
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/EngineFactory.h"

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "Testing Parameter Formatting System\n";
    std::cout << "====================================\n\n";
    
    // Test PitchShifter parameters
    std::cout << "PitchShifter Parameters:\n";
    std::cout << "  Pitch at 0.5: " << ParameterFormatter::formatValue("PitchShifter", "Pitch", 0.5f).toStdString() << "\n";
    std::cout << "  Pitch at 0.0: " << ParameterFormatter::formatValue("PitchShifter", "Pitch", 0.0f).toStdString() << "\n";
    std::cout << "  Pitch at 1.0: " << ParameterFormatter::formatValue("PitchShifter", "Pitch", 1.0f).toStdString() << "\n";
    std::cout << "  Mix at 0.75: " << ParameterFormatter::formatValue("PitchShifter", "Mix", 0.75f).toStdString() << "\n";
    std::cout << "  Grain at 0.3: " << ParameterFormatter::formatValue("PitchShifter", "Grain", 0.3f).toStdString() << "\n\n";
    
    // Test Delay parameters  
    std::cout << "Delay Parameters:\n";
    std::cout << "  Time at 0.125: " << ParameterFormatter::formatValue("Delay", "Time", 0.125f).toStdString() << "\n";
    std::cout << "  Feedback at 0.5: " << ParameterFormatter::formatValue("Delay", "Feedback", 0.5f).toStdString() << "\n";
    std::cout << "  Filter at 0.7: " << ParameterFormatter::formatValue("Delay", "Filter", 0.7f).toStdString() << "\n\n";
    
    // Test Compressor parameters
    std::cout << "Compressor Parameters:\n";
    std::cout << "  Threshold at 0.8: " << ParameterFormatter::formatValue("Compressor", "Threshold", 0.8f).toStdString() << "\n";
    std::cout << "  Ratio at 0.4: " << ParameterFormatter::formatValue("Compressor", "Ratio", 0.4f).toStdString() << "\n";
    std::cout << "  Attack at 0.2: " << ParameterFormatter::formatValue("Compressor", "Attack", 0.2f).toStdString() << "\n\n";
    
    // Test Filter parameters
    std::cout << "Filter Parameters:\n";
    std::cout << "  Frequency at 0.3: " << ParameterFormatter::formatValue("Filter", "Frequency", 0.3f).toStdString() << "\n";
    std::cout << "  Frequency at 0.9: " << ParameterFormatter::formatValue("Filter", "Frequency", 0.9f).toStdString() << "\n";
    std::cout << "  Resonance at 0.7: " << ParameterFormatter::formatValue("Filter", "Resonance", 0.7f).toStdString() << "\n\n";
    
    // Test unknown parameter (should return raw value)
    std::cout << "Unknown Parameters:\n";
    std::cout << "  Unknown at 0.456: " << ParameterFormatter::formatValue("UnknownEngine", "UnknownParam", 0.456f).toStdString() << "\n\n";
    
    std::cout << "Parameter formatting test complete!\n";
    
    return 0;
}
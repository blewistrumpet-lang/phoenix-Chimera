#include <iostream>
#include <memory>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PluginProcessor.h"
#include "JUCE_Plugin/Source/EngineFactory.h"

// Comprehensive test to verify engine and parameter loading

void testDirectEngineLoading() {
    std::cout << "\n===== TEST 1: Direct Engine Loading =====\n";
    
    ChimeraAudioProcessor processor;
    processor.prepareToPlay(44100, 512);
    
    // Test loading specific engines into slots
    const int testEngines[] = {
        1,  // Vintage Opto Compressor
        15, // Vintage Tube Preamp  
        39, // Plate Reverb
        18, // BitCrusher
        31, // PitchShifter
        42  // Shimmer Reverb
    };
    
    for (int slot = 0; slot < 6; slot++) {
        int engineID = testEngines[slot];
        std::cout << "\nSlot " << slot << ": Loading engine ID " << engineID << "\n";
        
        // Method 1: Direct loadEngine call
        processor.loadEngine(slot, engineID);
        
        // Verify engine is loaded
        auto& engine = processor.getEngine(slot);
        if (engine) {
            std::cout << "  ✓ Engine loaded: " << engine->getName().toStdString() << "\n";
        } else {
            std::cout << "  ✗ ERROR: Engine failed to load!\n";
        }
    }
}

void testAPVTSEngineLoading() {
    std::cout << "\n===== TEST 2: APVTS Engine Loading =====\n";
    
    ChimeraAudioProcessor processor;
    processor.prepareToPlay(44100, 512);
    
    auto& apvts = processor.getValueTreeState();
    
    // Test loading engines via parameter system
    const int testEngines[] = {22, 8, 41, 25, 33, 50}; // Different engines
    
    for (int slot = 0; slot < 6; slot++) {
        int engineID = testEngines[slot];
        juce::String paramID = "slot" + juce::String(slot + 1) + "_engine";
        
        std::cout << "\nSlot " << slot << ": Setting " << paramID.toStdString() 
                  << " to engine ID " << engineID << "\n";
        
        // Method 2: Via setSlotEngine (which should trigger APVTS)
        processor.setSlotEngine(slot, engineID);
        
        // Give time for parameter change to propagate
        juce::Thread::sleep(10);
        
        // Verify engine is loaded
        auto& engine = processor.getEngine(slot);
        if (engine) {
            std::cout << "  ✓ Engine loaded via APVTS: " << engine->getName().toStdString() << "\n";
            
            // Verify the parameter value matches
            auto* param = apvts.getRawParameterValue(paramID);
            if (param) {
                float normalizedValue = param->load();
                std::cout << "  Parameter normalized value: " << normalizedValue << "\n";
                
                // Check if we can get the choice back correctly
                auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(
                    apvts.getParameter(paramID));
                if (choiceParam) {
                    int choiceIndex = choiceParam->getIndex();
                    int retrievedEngineID = ChimeraAudioProcessor::choiceIndexToEngineID(choiceIndex);
                    std::cout << "  Retrieved engine ID: " << retrievedEngineID 
                              << (retrievedEngineID == engineID ? " ✓" : " ✗ MISMATCH!") << "\n";
                }
            }
        } else {
            std::cout << "  ✗ ERROR: Engine failed to load via APVTS!\n";
        }
    }
}

void testParameterLoading() {
    std::cout << "\n===== TEST 3: Parameter Loading =====\n";
    
    ChimeraAudioProcessor processor;
    processor.prepareToPlay(44100, 512);
    
    // Load a compressor and test its parameters
    processor.loadEngine(0, 1); // Vintage Opto Compressor in slot 0
    
    auto& apvts = processor.getValueTreeState();
    
    // Set specific parameter values
    const float testValues[] = {0.7f, 0.3f, 0.5f, 0.8f, 0.2f};
    
    for (int param = 1; param <= 5; param++) {
        juce::String paramID = "slot1_param" + juce::String(param);
        auto* parameter = apvts.getParameter(paramID);
        
        if (parameter) {
            float value = testValues[param - 1];
            parameter->setValueNotifyingHost(value);
            
            // Verify the value was set
            auto* rawValue = apvts.getRawParameterValue(paramID);
            if (rawValue) {
                float retrievedValue = rawValue->load();
                std::cout << "  Param " << param << ": Set=" << value 
                          << " Retrieved=" << retrievedValue
                          << (std::abs(retrievedValue - value) < 0.01f ? " ✓" : " ✗ MISMATCH!") << "\n";
            }
        }
    }
}

void testPresetApplication() {
    std::cout << "\n===== TEST 4: Preset Application =====\n";
    
    ChimeraAudioProcessor processor;
    processor.prepareToPlay(44100, 512);
    
    // Simulate a Trinity preset
    juce::var presetData = juce::var::object({
        {"name", "Test Preset"},
        {"slots", juce::var::array({
            juce::var::object({
                {"engine_id", 39},  // Plate Reverb
                {"engine_name", "Plate Reverb"},
                {"parameters", juce::var::array({
                    juce::var::object({{"name", "param1"}, {"value", 0.6f}}),
                    juce::var::object({{"name", "param2"}, {"value", 0.4f}}),
                    juce::var::object({{"name", "param3"}, {"value", 0.7f}})
                })}
            }),
            juce::var::object({
                {"engine_id", 1},  // Vintage Opto Compressor
                {"engine_name", "Vintage Opto Compressor"},
                {"parameters", juce::var::array({
                    juce::var::object({{"name", "param1"}, {"value", 0.5f}}),
                    juce::var::object({{"name", "param2"}, {"value", 0.3f}})
                })}
            })
        })}
    });
    
    // Apply preset through TrinityManager would call applyPreset
    // For this test, we'll manually set engines
    processor.setSlotEngine(0, 39); // Plate Reverb
    processor.setSlotEngine(1, 1);  // Vintage Opto Compressor
    
    // Verify engines loaded
    for (int slot = 0; slot < 2; slot++) {
        auto& engine = processor.getEngine(slot);
        if (engine) {
            std::cout << "  Slot " << slot << ": " << engine->getName().toStdString() << " ✓\n";
        } else {
            std::cout << "  Slot " << slot << ": ERROR - No engine loaded ✗\n";
        }
    }
}

void testEngineIDMapping() {
    std::cout << "\n===== TEST 5: Engine ID Mapping =====\n";
    
    // Test the mapping functions
    const int testIDs[] = {0, 1, 15, 31, 42, 56};
    
    for (int engineID : testIDs) {
        int choiceIndex = ChimeraAudioProcessor::engineIDToChoiceIndex(engineID);
        int backToID = ChimeraAudioProcessor::choiceIndexToEngineID(choiceIndex);
        
        std::cout << "  Engine ID " << engineID << " -> Choice " << choiceIndex 
                  << " -> ID " << backToID;
        
        if (engineID == backToID) {
            std::cout << " ✓\n";
        } else {
            std::cout << " ✗ MAPPING ERROR!\n";
        }
    }
}

int main() {
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juce_init;
    
    std::cout << "========================================\n";
    std::cout << "   CHIMERA ENGINE LOADING TEST SUITE   \n";
    std::cout << "========================================\n";
    
    testDirectEngineLoading();
    testAPVTSEngineLoading();
    testParameterLoading();
    testPresetApplication();
    testEngineIDMapping();
    
    std::cout << "\n========================================\n";
    std::cout << "          TEST SUITE COMPLETE           \n";
    std::cout << "========================================\n\n";
    
    return 0;
}
// Test if parameters are actually reaching reverb engines in plugin
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/PluginProcessor.h"

int main() {
    std::cout << "\n=== Testing Parameter Flow to Reverb Engines ===\n" << std::endl;
    
    // Create the plugin processor
    auto processor = std::make_unique<ChimeraPhoenixAudioProcessor>();
    
    // Prepare processor
    processor->prepareToPlay(44100, 512);
    
    // Test PlateReverb (engine 39)
    std::cout << "Setting engine to PlateReverb (39)..." << std::endl;
    
    // Get parameter tree
    auto& params = processor->getParameters();
    
    // Find and set engine parameter
    for (auto* param : params) {
        auto* rangedParam = dynamic_cast<juce::AudioParameterFloat*>(param);
        if (rangedParam) {
            auto id = rangedParam->paramID;
            if (id == "engine1Type") {
                std::cout << "Found engine1Type parameter, setting to 39..." << std::endl;
                rangedParam->setValueNotifyingHost(39.0f / 56.0f); // Normalize to 0-1
                break;
            }
        }
    }
    
    // Enable engine 1
    for (auto* param : params) {
        auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param);
        if (boolParam && boolParam->paramID == "engine1Enabled") {
            std::cout << "Enabling engine 1..." << std::endl;
            boolParam->setValueNotifyingHost(true);
            break;
        }
    }
    
    // Set reverb parameters to extreme values
    std::cout << "\nSetting reverb parameters to extreme values:" << std::endl;
    
    // Find Mix parameter for engine 1 slot 4 (Mix is param 3, zero-indexed)
    for (auto* param : params) {
        auto* rangedParam = dynamic_cast<juce::AudioParameterFloat*>(param);
        if (rangedParam) {
            auto id = rangedParam->paramID;
            
            // Engine 1 Param 0 (Size)
            if (id == "engine1Param0") {
                std::cout << "Setting Size to 1.0..." << std::endl;
                rangedParam->setValueNotifyingHost(1.0f);
            }
            // Engine 1 Param 1 (Damping)  
            else if (id == "engine1Param1") {
                std::cout << "Setting Damping to 0.0..." << std::endl;
                rangedParam->setValueNotifyingHost(0.0f);
            }
            // Engine 1 Param 3 (Mix)
            else if (id == "engine1Param3") {
                std::cout << "Setting Mix to 1.0 (100% wet)..." << std::endl;
                rangedParam->setValueNotifyingHost(1.0f);
            }
        }
    }
    
    // Process some audio
    std::cout << "\nProcessing test audio..." << std::endl;
    
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midiBuffer;
    
    // Generate impulse
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    std::cout << "Input impulse level: " << buffer.getRMSLevel(0, 0, 512) << std::endl;
    
    // Process multiple blocks to see if reverb tail appears
    for (int block = 0; block < 10; ++block) {
        processor->processBlock(buffer, midiBuffer);
        float rms = buffer.getRMSLevel(0, 0, 512);
        std::cout << "Block " << block << " output RMS: " << rms << std::endl;
        
        if (block == 0) {
            // After first block, clear input to hear only reverb tail
            buffer.clear();
        }
    }
    
    // Now test changing to SpringReverb
    std::cout << "\n=== Switching to SpringReverb (40) ===" << std::endl;
    
    for (auto* param : params) {
        auto* rangedParam = dynamic_cast<juce::AudioParameterFloat*>(param);
        if (rangedParam && rangedParam->paramID == "engine1Type") {
            rangedParam->setValueNotifyingHost(40.0f / 56.0f);
            break;
        }
    }
    
    // Process with SpringReverb
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    for (int block = 0; block < 5; ++block) {
        processor->processBlock(buffer, midiBuffer);
        float rms = buffer.getRMSLevel(0, 0, 512);
        std::cout << "SpringReverb Block " << block << " RMS: " << rms << std::endl;
        if (block == 0) buffer.clear();
    }
    
    // Test GatedReverb
    std::cout << "\n=== Switching to GatedReverb (43) ===" << std::endl;
    
    for (auto* param : params) {
        auto* rangedParam = dynamic_cast<juce::AudioParameterFloat*>(param);
        if (rangedParam && rangedParam->paramID == "engine1Type") {
            rangedParam->setValueNotifyingHost(43.0f / 56.0f);
            break;
        }
    }
    
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    for (int block = 0; block < 5; ++block) {
        processor->processBlock(buffer, midiBuffer);
        float rms = buffer.getRMSLevel(0, 0, 512);
        std::cout << "GatedReverb Block " << block << " RMS: " << rms << std::endl;
        if (block == 0) buffer.clear();
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "If all RMS values after block 0 are near 0, parameters aren't reaching engines." << std::endl;
    std::cout << "If RMS values decay gradually, reverbs are working." << std::endl;
    
    return 0;
}
#include <iostream>
#include <memory>
#include <map>
#include <iomanip>
#include "JUCE_Plugin/JuceLibraryCode/JuceHeader.h"
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/PitchShifter.h"
#include "JUCE_Plugin/Source/ClassicCompressor.h"
#include "JUCE_Plugin/Source/BitCrusher.h"
#include "JUCE_Plugin/Source/ParametricEQ.h"
#include "JUCE_Plugin/Source/DigitalDelay.h"

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

void testEngineParameters(int engineId, const std::string& name) {
    std::cout << "\n=== Testing " << name << " (ID: " << engineId << ") ===" << std::endl;
    
    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << RED << "Failed to create engine!" << RESET << std::endl;
        return;
    }
    
    engine->prepareToPlay(44100, 512);
    
    // Get parameter count
    int numParams = engine->getNumParameters();
    std::cout << "Parameter count: " << numParams << std::endl;
    
    // List all parameters
    for (int i = 0; i < numParams; ++i) {
        std::string paramName = engine->getParameterName(i).toStdString();
        std::cout << "  Param " << i << ": " << paramName << std::endl;
    }
    
    // Test parameter updates
    std::cout << "\nTesting parameter updates:" << std::endl;
    
    // Create test buffer
    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 512; ++i) {
            buffer.setSample(ch, i, 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f));
        }
    }
    
    // Store original
    juce::AudioBuffer<float> original(buffer);
    
    // Test each parameter
    for (int paramIdx = 0; paramIdx < numParams; ++paramIdx) {
        // Reset buffer
        buffer = original;
        
        // Set only this parameter to extreme values
        std::map<int, float> params;
        
        // First test with parameter at 0
        params[paramIdx] = 0.0f;
        engine->updateParameters(params);
        engine->process(buffer);
        
        float changeAt0 = 0;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 512; ++i) {
                changeAt0 += std::abs(buffer.getSample(ch, i) - original.getSample(ch, i));
            }
        }
        
        // Reset and test with parameter at 1
        buffer = original;
        params[paramIdx] = 1.0f;
        engine->updateParameters(params);
        engine->process(buffer);
        
        float changeAt1 = 0;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 512; ++i) {
                changeAt1 += std::abs(buffer.getSample(ch, i) - original.getSample(ch, i));
            }
        }
        
        // Check if parameter has any effect
        std::cout << "  Param " << paramIdx << " (" 
                  << engine->getParameterName(paramIdx).toStdString() << "): ";
        
        if (changeAt0 < 0.01f && changeAt1 < 0.01f) {
            std::cout << RED << "NO EFFECT" << RESET << std::endl;
        } else if (std::abs(changeAt0 - changeAt1) < 0.01f) {
            std::cout << YELLOW << "SAME AT 0 AND 1" << RESET << std::endl;
        } else {
            std::cout << GREEN << "OK (change: " 
                      << std::fixed << std::setprecision(2) 
                      << changeAt0 << " -> " << changeAt1 << ")" << RESET << std::endl;
        }
    }
}

void testPitchShifterSpecifically() {
    std::cout << "\n" << CYAN << "=== PITCH SHIFTER DEEP DIVE ===" << RESET << std::endl;
    
    PitchShifter pitch;
    pitch.prepareToPlay(44100, 512);
    
    // Create test tone
    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 512; ++i) {
            buffer.setSample(ch, i, 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f));
        }
    }
    
    juce::AudioBuffer<float> original(buffer);
    
    // Test pitch parameter at different values
    std::cout << "\nPitch parameter mapping test:" << std::endl;
    float testValues[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    
    for (float value : testValues) {
        buffer = original;
        
        std::map<int, float> params;
        params[0] = value;  // kPitch = 0
        params[2] = 1.0f;   // kMix = 2 (full wet)
        pitch.updateParameters(params);
        pitch.process(buffer);
        
        // Analyze pitch change
        float semitones = (value - 0.5f) * 48.0f;
        float expectedRatio = std::pow(2.0f, semitones / 12.0f);
        
        std::cout << "  Value: " << std::fixed << std::setprecision(2) << value 
                  << " -> " << std::setprecision(1) << semitones << " semitones"
                  << " (ratio: " << std::setprecision(3) << expectedRatio << ")";
        
        // Check if audio changed
        float totalChange = 0;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 512; ++i) {
                totalChange += std::abs(buffer.getSample(ch, i) - original.getSample(ch, i));
            }
        }
        
        if (totalChange < 0.01f && std::abs(value - 0.5f) > 0.01f) {
            std::cout << RED << " - NO CHANGE!" << RESET;
        } else {
            std::cout << GREEN << " - OK" << RESET;
        }
        std::cout << std::endl;
    }
    
    // Test musical intervals
    std::cout << "\nMusical interval test:" << std::endl;
    struct Interval {
        std::string name;
        float value;  // 0-1 parameter value
        int semitones;
    };
    
    Interval intervals[] = {
        {"Octave Down", 0.25f, -12},
        {"Perfect 5th Down", 0.354f, -7},
        {"Minor 3rd Down", 0.438f, -3},
        {"Unison", 0.5f, 0},
        {"Major 3rd Up", 0.583f, 4},
        {"Perfect 5th Up", 0.646f, 7},
        {"Octave Up", 0.75f, 12}
    };
    
    for (const auto& interval : intervals) {
        buffer = original;
        
        std::map<int, float> params;
        params[0] = interval.value;  // kPitch
        params[2] = 1.0f;  // kMix = full wet
        pitch.updateParameters(params);
        pitch.process(buffer);
        
        float totalChange = 0;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 512; ++i) {
                totalChange += std::abs(buffer.getSample(ch, i) - original.getSample(ch, i));
            }
        }
        
        std::cout << "  " << std::setw(20) << std::left << interval.name 
                  << " (" << std::setw(3) << interval.semitones << " st): ";
        
        if (totalChange > 0.01f || interval.semitones == 0) {
            std::cout << GREEN << "WORKING" << RESET;
        } else {
            std::cout << RED << "NOT WORKING" << RESET;
        }
        std::cout << std::endl;
    }
}

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "========================================" << std::endl;
    std::cout << "Parameter Routing Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Test key engines
    testEngineParameters(31, "PitchShifter");        // Correct ID for PitchShifter
    testEngineParameters(2, "ClassicCompressor");
    testEngineParameters(18, "BitCrusher");
    testEngineParameters(11, "ParametricEQ");
    testEngineParameters(35, "DigitalDelay");        // Correct ID for DigitalDelay
    testEngineParameters(33, "IntelligentHarmonizer"); // Test the other pitch engine
    
    // Deep dive on pitch shifter
    testPitchShifterSpecifically();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Complete" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
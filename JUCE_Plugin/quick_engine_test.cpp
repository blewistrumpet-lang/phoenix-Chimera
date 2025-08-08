// Quick engine test - tests one engine at a time
#include <iostream>
#include <JuceHeader.h>
#include "Source/EngineFactory.h"
#include "Source/EngineIDs.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./quick_engine_test <engine_id>\n";
        std::cout << "Example: ./quick_engine_test 1  (for Rodent Distortion)\n";
        return 1;
    }
    
    int engineID = std::atoi(argv[1]);
    juce::ScopedJuceInitialiser_GUI juce_init;
    
    // Create engine
    auto engine = EngineFactory::createEngine(engineID);
    if (!engine) {
        std::cout << "Failed to create engine " << engineID << "\n";
        return 1;
    }
    
    std::cout << "Testing: " << engine->getName().toStdString() << " (ID: " << engineID << ")\n";
    
    // Setup
    engine->prepareToPlay(44100.0, 512);
    
    // Create test buffer with sine wave
    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < 512; ++i) {
            data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        }
    }
    
    // Calculate input RMS
    float inputRMS = 0;
    for (int ch = 0; ch < 2; ++ch) {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            inputRMS += data[i] * data[i];
        }
    }
    inputRMS = std::sqrt(inputRMS / (2 * 512));
    
    // Set parameters (100% mix)
    std::map<int, float> params;
    params[0] = 0.7f;  // Main param
    params[3] = 1.0f;  // Mix at index 3
    params[5] = 1.0f;  // Mix at index 5
    params[6] = 1.0f;  // Mix at index 6
    params[7] = 1.0f;  // Mix at index 7
    
    engine->updateParameters(params);
    
    // Process
    engine->process(buffer);
    
    // Calculate output RMS
    float outputRMS = 0;
    for (int ch = 0; ch < 2; ++ch) {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            outputRMS += data[i] * data[i];
        }
    }
    outputRMS = std::sqrt(outputRMS / (2 * 512));
    
    // Results
    std::cout << "Input RMS:  " << inputRMS << "\n";
    std::cout << "Output RMS: " << outputRMS << "\n";
    std::cout << "Changed:    " << (std::abs(outputRMS - inputRMS) > 0.01f ? "YES ✅" : "NO ❌") << "\n";
    
    return 0;
}
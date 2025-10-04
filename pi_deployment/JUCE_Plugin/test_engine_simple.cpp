// Simple standalone test to verify engines work
#include <iostream>
#include <cmath>
#include "../Source/EngineFactory.h"
#include "../Source/EngineIDs.h"

int main() {
    // Test engine IDs
    int testEngines[] = {1, 2, 6, 11, 21};
    const char* engineNames[] = {"Rodent", "Vintage", "DynamicEQ", "Formant", "Plate"};
    
    for (int i = 0; i < 5; ++i) {
        int engineID = testEngines[i];
        std::cout << "\nTesting " << engineNames[i] << " (ID " << engineID << "):\n";
        
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            std::cout << "  FAILED to create!\n";
            continue;
        }
        
        // Prepare
        engine->prepareToPlay(44100.0, 512);
        
        // Create test buffer
        juce::AudioBuffer<float> buffer(2, 512);
        
        // Fill with sine wave
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int s = 0; s < 512; ++s) {
                data[s] = 0.5f * std::sin(2.0f * M_PI * 440.0f * s / 44100.0f);
            }
        }
        
        // Calculate input RMS
        float inputRMS = 0;
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int s = 0; s < 512; ++s) {
                inputRMS += data[s] * data[s];
            }
        }
        inputRMS = std::sqrt(inputRMS / (2 * 512));
        
        // Set parameters - try all possible mix indices
        std::map<int, float> params;
        params[0] = 0.8f;  // Main param high
        params[3] = 1.0f;  // Mix at 3
        params[5] = 1.0f;  // Mix at 5
        params[6] = 1.0f;  // Mix at 6
        params[7] = 1.0f;  // Mix at 7
        
        engine->updateParameters(params);
        
        // Process
        engine->process(buffer);
        
        // Calculate output RMS
        float outputRMS = 0;
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int s = 0; s < 512; ++s) {
                outputRMS += data[s] * data[s];
            }
        }
        outputRMS = std::sqrt(outputRMS / (2 * 512));
        
        // Check if changed
        float diff = std::abs(outputRMS - inputRMS);
        std::cout << "  Input RMS:  " << inputRMS << "\n";
        std::cout << "  Output RMS: " << outputRMS << "\n";
        std::cout << "  Difference: " << diff << "\n";
        std::cout << "  Status: " << (diff > 0.01f ? "WORKING" : "NOT WORKING") << "\n";
    }
    
    return 0;
}
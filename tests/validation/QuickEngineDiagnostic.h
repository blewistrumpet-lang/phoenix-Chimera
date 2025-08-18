#pragma once
#include <JuceHeader.h>
#include "EngineBase.h"
#include "EngineFactory.h"
#include "EngineTypes.h"
#include <fstream>

class QuickEngineDiagnostic {
public:
    static void runQuickTest() {
        // Write to file instead of console
        std::ofstream outFile("/tmp/chimera_quick_test.txt");
        if (!outFile.is_open()) return;
        
        outFile << "=== QUICK ENGINE TEST ===\n\n";
        
        // Test a few key engines
        int testEngines[] = {0, 1, 2, 6, 18, 21};
        const char* names[] = {"None", "Opto", "VCA", "DynamicEQ", "BitCrusher", "Rodent"};
        
        for (int i = 0; i < 6; ++i) {
            int engineID = testEngines[i];
            outFile << "Engine " << engineID << " (" << names[i] << "):\n";
            
            try {
                auto engine = EngineFactory::createEngine(engineID);
                if (!engine) {
                    outFile << "  FAILED to create\n\n";
                    continue;
                }
                
                // Prepare
                engine->prepareToPlay(44100.0, 512);
                
                // Create small test buffer
                juce::AudioBuffer<float> buffer(2, 64); // Small buffer
                
                // Simple test signal
                for (int ch = 0; ch < 2; ++ch) {
                    auto* data = buffer.getWritePointer(ch);
                    for (int s = 0; s < 64; ++s) {
                        data[s] = 0.3f; // DC signal
                    }
                }
                
                // Calculate input
                float inputSum = 0;
                for (int ch = 0; ch < 2; ++ch) {
                    auto* data = buffer.getReadPointer(ch);
                    for (int s = 0; s < 64; ++s) {
                        inputSum += std::abs(data[s]);
                    }
                }
                
                // Set max parameters
                std::map<int, float> params;
                for (int p = 0; p < 15; ++p) {
                    params[p] = (p >= 3 && p <= 7) ? 1.0f : 0.5f;
                }
                
                engine->updateParameters(params);
                engine->process(buffer);
                
                // Calculate output
                float outputSum = 0;
                for (int ch = 0; ch < 2; ++ch) {
                    auto* data = buffer.getReadPointer(ch);
                    for (int s = 0; s < 64; ++s) {
                        outputSum += std::abs(data[s]);
                    }
                }
                
                float diff = std::abs(outputSum - inputSum);
                outFile << "  Input sum: " << inputSum << "\n";
                outFile << "  Output sum: " << outputSum << "\n";
                outFile << "  Difference: " << diff << "\n";
                outFile << "  Status: " << (diff > 0.1f ? "PROCESSING" : "PASSTHROUGH") << "\n\n";
                
            } catch (...) {
                outFile << "  EXCEPTION during test\n\n";
            }
        }
        
        outFile << "=== TEST COMPLETE ===\n";
        outFile.close();
    }
};
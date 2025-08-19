#include <iostream>
#include <cmath>
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineTypes.h"
#include <JuceHeader.h>

int main() {
    std::cout << "Testing fixed engines for NaN issues..." << std::endl;
    
    // Test the 5 engines we fixed
    int testEngines[] = {1, 22, 40, 46, 56};
    const char* engineNames[] = {"Vintage Opto Platinum", "K-Style Overdrive", 
                                "Spring Reverb Platinum", "Dimension Expander", 
                                "Phase Align Platinum"};
    
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
        
        // Create test buffer with sine wave
        juce::AudioBuffer<float> buffer(2, 512);
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int s = 0; s < 512; ++s) {
                data[s] = 0.5f * std::sin(2.0f * M_PI * 440.0f * s / 44100.0f);
            }
        }
        
        // Set parameters to middle values
        std::map<int, float> params;
        for (int p = 0; p < 15; ++p) {
            params[p] = 0.5f;
        }
        engine->updateParameters(params);
        
        // Process audio
        engine->process(buffer);
        
        // Check for NaN/Inf values
        bool hasNaN = false;
        int nanCount = 0;
        int firstNaNSample = -1;
        
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int s = 0; s < 512; ++s) {
                if (!std::isfinite(data[s]) || std::isnan(data[s])) {
                    if (!hasNaN) {
                        hasNaN = true;
                        firstNaNSample = s;
                    }
                    nanCount++;
                }
            }
        }
        
        if (hasNaN) {
            std::cout << "  ❌ STILL HAS NaN: " << nanCount << " values (first at sample " << firstNaNSample << ")\n";
        } else {
            float rms = buffer.getRMSLevel(0, 0, 512);
            std::cout << "  ✅ FIXED: No NaN values, RMS = " << rms << "\n";
        }
    }
    
    return 0;
}
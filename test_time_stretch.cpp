#include <iostream>
#include <map>
#include <cmath>
#include <iomanip>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PhasedVocoder.h"

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "=== PhasedVocoder Time-Stretch Test ===\n\n";
    
    // Test different time stretch factors
    float stretchFactors[] = {0.5f, 0.75f, 1.0f, 1.5f, 2.0f};
    
    for (float targetStretch : stretchFactors) {
        std::cout << "Testing " << targetStretch << "x time stretch:\n";
        
        PhasedVocoder vocoder;
        double sampleRate = 48000;
        int blockSize = 512;
        
        vocoder.prepareToPlay(sampleRate, blockSize);
        
        // Map stretch factor to parameter: 0.5x to 2.0x maps to 0.0 to 1.0
        float timeParam = (targetStretch - 0.5f) / 1.5f;
        
        std::map<int, float> params;
        params[0] = timeParam;      // Time stretch
        params[1] = 0.333333f;      // No pitch shift (1.0x)
        params[6] = 1.0f;           // 100% wet
        vocoder.updateParameters(params);
        
        // Process blocks and measure timing
        int totalSamples = 0;
        float peakLevel = 0;
        
        for (int block = 0; block < 50; ++block) {
            juce::AudioBuffer<float> buffer(2, blockSize);
            
            // Generate test signal - chirp from 100Hz to 1000Hz
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    float t = (block * blockSize + i) / sampleRate;
                    float freq = 100.0f + 900.0f * t / 2.0f; // Sweep over 2 seconds
                    data[i] = 0.3f * std::sin(2.0f * M_PI * freq * t);
                }
            }
            
            vocoder.process(buffer);
            
            // Track output level
            for (int i = 0; i < blockSize; ++i) {
                float sample = std::abs(buffer.getSample(0, i));
                if (sample > peakLevel) peakLevel = sample;
            }
            totalSamples += blockSize;
        }
        
        std::cout << "  Processed " << totalSamples << " samples\n";
        std::cout << "  Peak output level: " << std::fixed << std::setprecision(3) 
                  << peakLevel << "\n";
        
        if (peakLevel > 0.1f) {
            std::cout << "  ✅ Time stretch working\n";
        } else {
            std::cout << "  ❌ Output too low\n";
        }
        std::cout << "\n";
    }
    
    return 0;
}
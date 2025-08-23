#include <iostream>
#include <map>
#include <cmath>
#include <iomanip>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PhasedVocoder.h"

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "=== PhasedVocoder Identity Test ===\n";
    std::cout << "Testing with 1x time stretch, no pitch shift\n\n";
    
    PhasedVocoder vocoder;
    double sampleRate = 48000;
    int blockSize = 512;
    
    vocoder.prepareToPlay(sampleRate, blockSize);
    
    // Set for true pass-through (1x time, no pitch)
    std::map<int, float> params;
    params[0] = 0.2f;   // Time stretch = 1.0x
    params[1] = 0.333333f;   // Pitch shift = 1.0x (no shift) - (1.0 - 0.5) / 1.5 = 0.333
    params[6] = 1.0f;   // 100% wet
    vocoder.updateParameters(params);
    
    std::cout << "Processing blocks...\n";
    
    // Process many blocks to prime the pipeline (4096 samples = 8 blocks of 512)
    float totalInputRMS = 0;
    float totalOutputRMS = 0;
    int blocksWithOutput = 0;
    
    // Process more blocks to ensure we're past warmup (4096 samples = 8 blocks)
    for (int block = 0; block < 100; ++block) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        
        // Generate 440Hz test signal
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                float t = (block * blockSize + i) / sampleRate;
                data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * t);
            }
        }
        
        // Measure input
        float inputRMS = 0;
        for (int i = 0; i < blockSize; ++i) {
            float s = buffer.getSample(0, i);
            inputRMS += s * s;
        }
        inputRMS = std::sqrt(inputRMS / blockSize);
        
        // Process
        vocoder.process(buffer);
        
        // Measure output
        float outputRMS = 0;
        float maxSample = 0;
        for (int i = 0; i < blockSize; ++i) {
            float s = buffer.getSample(0, i);
            outputRMS += s * s;
            maxSample = std::max(maxSample, std::abs(s));
        }
        outputRMS = std::sqrt(outputRMS / blockSize);
        
        // Report every 10 blocks
        if (block % 10 == 0) {
            std::cout << "Block " << std::setw(2) << block 
                      << ": Input=" << std::fixed << std::setprecision(4) << inputRMS
                      << ", Output=" << outputRMS
                      << ", Max=" << maxSample;
            
            if (outputRMS > 0.01f) {
                float db = 20.0f * std::log10(outputRMS / inputRMS);
                std::cout << " (" << std::setprecision(1) << db << " dB)";
                blocksWithOutput++;
            } else if (outputRMS > 0.0001f) {
                std::cout << " ⚠ Low";
            } else {
                std::cout << " ✗ Silent";
            }
            std::cout << std::endl;
        }
        
        if (block >= 50) {  // After priming (well past 4096 sample warmup)
            totalInputRMS += inputRMS;
            totalOutputRMS += outputRMS;
            if (outputRMS > 0.01f) blocksWithOutput++;
        }
    }
    
    std::cout << "\nAfter priming (blocks 50-99):\n";
    float avgInput = totalInputRMS / 50;
    float avgOutput = totalOutputRMS / 50;
    std::cout << "  Average Input RMS:  " << avgInput << std::endl;
    std::cout << "  Average Output RMS: " << avgOutput << std::endl;
    std::cout << "  Blocks with output: " << blocksWithOutput << "/50" << std::endl;
    
    if (avgOutput > avgInput * 0.8f) {
        std::cout << "✅ Identity pass WORKING!\n";
    } else {
        float db = 20.0f * std::log10(avgOutput / avgInput);
        std::cout << "❌ Identity pass FAILED - output is " << db << " dB below input\n";
    }
    
    return 0;
}
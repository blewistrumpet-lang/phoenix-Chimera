#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/BitCrusher.h"

int main() {
    std::cout << "\n=== BITCRUSHER FUNCTIONALITY TEST ===" << std::endl;
    
    BitCrusher crusher;
    crusher.prepareToPlay(44100, 512);
    
    // Create test buffer with sine wave
    juce::AudioBuffer<float> buffer(2, 512);
    for (int i = 0; i < 512; ++i) {
        float sample = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    
    // Test 1: Default parameters (should pass through clean)
    std::cout << "\nTest 1: Default parameters" << std::endl;
    float originalRMS = buffer.getRMSLevel(0, 0, 512);
    std::cout << "  Original RMS: " << originalRMS << std::endl;
    
    std::map<int, float> params;
    params[0] = 0.1f;  // High bit depth (24 bits)
    params[1] = 0.1f;  // No downsampling
    params[2] = 1.0f;  // Full wet
    crusher.updateParameters(params);
    crusher.process(buffer);
    
    float processedRMS = buffer.getRMSLevel(0, 0, 512);
    std::cout << "  Processed RMS: " << processedRMS << std::endl;
    std::cout << "  Should be similar: " << (std::abs(originalRMS - processedRMS) < 0.01f ? "PASS" : "FAIL") << std::endl;
    
    // Test 2: Heavy crushing
    std::cout << "\nTest 2: Heavy bit crushing" << std::endl;
    buffer.clear();
    for (int i = 0; i < 512; ++i) {
        float sample = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    
    params[0] = 0.9f;  // 1-bit (destroyed)
    params[1] = 0.9f;  // Heavy downsampling (1/16)
    params[2] = 1.0f;  // Full wet
    crusher.updateParameters(params);
    crusher.process(buffer);
    
    processedRMS = buffer.getRMSLevel(0, 0, 512);
    std::cout << "  Processed RMS: " << processedRMS << std::endl;
    
    // Check if signal is crushed (should have discontinuities)
    int zeroCount = 0;
    for (int i = 0; i < 512; ++i) {
        float sample = std::abs(buffer.getSample(0, i));
        if (sample < 0.01f) zeroCount++;
    }
    std::cout << "  Zero samples: " << zeroCount << "/512" << std::endl;
    std::cout << "  Crushing active: " << (processedRMS < originalRMS * 0.8f ? "YES" : "NO") << std::endl;
    
    // Test 3: Parameter mapping
    std::cout << "\nTest 3: Parameter mapping" << std::endl;
    std::cout << "  Param 0 (Bits):" << std::endl;
    std::cout << "    0.1 -> 24 bits (clean)" << std::endl;
    std::cout << "    0.3 -> 12 bits (vintage)" << std::endl;
    std::cout << "    0.5 -> 8 bits (classic)" << std::endl;
    std::cout << "    0.7 -> 4 bits (crunchy)" << std::endl;
    std::cout << "    0.9 -> 1 bit (destroyed)" << std::endl;
    
    std::cout << "\n  Param 1 (Downsample):" << std::endl;
    std::cout << "    0.1 -> 1x (no downsampling)" << std::endl;
    std::cout << "    0.3 -> 2x (half rate)" << std::endl;
    std::cout << "    0.5 -> 4x (quarter rate)" << std::endl;
    std::cout << "    0.7 -> 8x (1/8 rate)" << std::endl;
    std::cout << "    0.9 -> 16x (1/16 rate)" << std::endl;
    
    // Test 4: Mix control
    std::cout << "\nTest 4: Mix control (50% wet)" << std::endl;
    buffer.clear();
    for (int i = 0; i < 512; ++i) {
        float sample = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    
    params[0] = 0.9f;  // Heavy crushing
    params[1] = 0.9f;  // Heavy downsampling
    params[2] = 0.5f;  // 50% mix
    crusher.updateParameters(params);
    crusher.process(buffer);
    
    processedRMS = buffer.getRMSLevel(0, 0, 512);
    std::cout << "  Processed RMS: " << processedRMS << std::endl;
    std::cout << "  Should be between clean and crushed: " << 
              (processedRMS > originalRMS * 0.4f && processedRMS < originalRMS * 0.9f ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    
    return 0;
}
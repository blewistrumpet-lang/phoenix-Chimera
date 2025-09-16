// Final comprehensive test of all reverbs with correct parameter indices
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"

struct TestResult {
    bool dryPass = false;
    bool wetPass = false;
    bool mixPass = false;
    float totalEnergy = 0.0f;
};

TestResult testReverb(EngineBase* reverb, const std::string& name, 
                      int mixIndex, int sizeIndex, int dampIndex) {
    TestResult result;
    
    std::cout << "\n=== " << name << " ===" << std::endl;
    std::cout << "Mix index: " << mixIndex << ", Size index: " << sizeIndex 
              << ", Damp index: " << dampIndex << std::endl;
    
    reverb->prepareToPlay(44100, 512);
    
    // Test 1: Dry signal (Mix = 0)
    {
        reverb->reset();
        std::map<int, float> params;
        params[mixIndex] = 0.0f;  // Mix = 0
        if (sizeIndex >= 0) params[sizeIndex] = 0.5f;
        if (dampIndex >= 0) params[dampIndex] = 0.5f;
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 10);
        for (int i = 0; i < 10; i++) {
            buffer.setSample(0, i, 1.0f);
            buffer.setSample(1, i, 1.0f);
        }
        
        reverb->process(buffer);
        
        float firstSample = buffer.getSample(0, 0);
        result.dryPass = std::abs(firstSample - 1.0f) < 0.01f;
        std::cout << "Dry test: " << firstSample << " (expected 1.0) - " 
                  << (result.dryPass ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
    
    // Test 2: Wet signal (Mix = 1)
    {
        reverb->reset();
        std::map<int, float> params;
        params[mixIndex] = 1.0f;  // Mix = 1
        if (sizeIndex >= 0) params[sizeIndex] = 0.8f;  // Large size
        if (dampIndex >= 0) params[dampIndex] = 0.2f;  // Low damping
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse
        buffer.setSample(1, 0, 1.0f);
        
        for (int block = 0; block < 10; block++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            result.totalEnergy += rms;
            if (block == 0) buffer.clear();
        }
        
        result.wetPass = result.totalEnergy > 0.01f;
        std::cout << "Wet test: Total energy = " << result.totalEnergy 
                  << " - " << (result.wetPass ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
    
    // Test 3: Mix parameter works
    {
        reverb->reset();
        
        // Generate test tone
        juce::AudioBuffer<float> testBuffer(2, 100);
        for (int i = 0; i < 100; i++) {
            float val = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f) * 0.5f;
            testBuffer.setSample(0, i, val);
            testBuffer.setSample(1, i, val);
        }
        
        // Test with Mix = 0
        std::map<int, float> params;
        params[mixIndex] = 0.0f;
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer1(testBuffer);
        reverb->process(buffer1);
        float rms1 = buffer1.getRMSLevel(0, 0, 100);
        
        // Test with Mix = 1
        reverb->reset();
        params[mixIndex] = 1.0f;
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer2(testBuffer);
        reverb->process(buffer2);
        float rms2 = buffer2.getRMSLevel(0, 0, 100);
        
        result.mixPass = std::abs(rms1 - rms2) > 0.01f;
        std::cout << "Mix test: RMS@0=" << rms1 << ", RMS@1=" << rms2 
                  << " - " << (result.mixPass ? "PASS ✓" : "FAIL ✗") << std::endl;
    }
    
    return result;
}

int main() {
    std::cout << "FINAL COMPREHENSIVE REVERB TEST" << std::endl;
    std::cout << "================================\n" << std::endl;
    
    int passCount = 0;
    int totalTests = 0;
    
    // SpringReverb: 0=Tension, 1=Damping, 2=Decay, 3=Mix
    {
        auto reverb = std::make_unique<SpringReverb>();
        auto result = testReverb(reverb.get(), "SpringReverb", 3, 2, 1);
        if (result.dryPass) passCount++;
        if (result.wetPass) passCount++;
        if (result.mixPass) passCount++;
        totalTests += 3;
    }
    
    // PlateReverb: 0=Size, 1=Damping, 2=Predelay, 3=Mix
    {
        auto reverb = std::make_unique<PlateReverb>();
        auto result = testReverb(reverb.get(), "PlateReverb", 3, 0, 1);
        if (result.dryPass) passCount++;
        if (result.wetPass) passCount++;
        if (result.mixPass) passCount++;
        totalTests += 3;
    }
    
    // ConvolutionReverb: 0=Mix, 1=Predelay, 2=Damping, 3=Size
    {
        auto reverb = std::make_unique<ConvolutionReverb>();
        auto result = testReverb(reverb.get(), "ConvolutionReverb", 0, 3, 2);
        if (result.dryPass) passCount++;
        if (result.wetPass) passCount++;
        if (result.mixPass) passCount++;
        totalTests += 3;
    }
    
    // ShimmerReverb: 0=PitchShift, 1=Shimmer, 2=RoomSize, 3=Damping, 4=Mix
    {
        auto reverb = std::make_unique<ShimmerReverb>();
        auto result = testReverb(reverb.get(), "ShimmerReverb", 4, 2, 3);
        if (result.dryPass) passCount++;
        if (result.wetPass) passCount++;
        if (result.mixPass) passCount++;
        totalTests += 3;
    }
    
    // GatedReverb: 0=Threshold, 1=Hold, 2=RoomSize, 3=Damping, 4=Mix
    {
        auto reverb = std::make_unique<GatedReverb>();
        
        std::cout << "\n=== GatedReverb (Special) ===" << std::endl;
        
        reverb->prepareToPlay(44100, 512);
        
        // For gated reverb, we need threshold = 0 to keep gate open
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.0f;  // Threshold = 0 (gate always open)
        params[1] = 0.5f;  // Hold
        params[2] = 0.8f;  // Large room
        params[3] = 0.2f;  // Low damping
        params[4] = 1.0f;  // Mix = 100% wet
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        float totalEnergy = 0.0f;
        for (int block = 0; block < 10; block++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            totalEnergy += rms;
            if (block == 0) buffer.clear();
        }
        
        bool gatePass = totalEnergy > 0.01f;
        std::cout << "Gate open test: Energy = " << totalEnergy 
                  << " - " << (gatePass ? "PASS ✓" : "FAIL ✗") << std::endl;
        
        if (gatePass) passCount++;
        totalTests++;
    }
    
    std::cout << "\n================================" << std::endl;
    std::cout << "FINAL RESULTS: " << passCount << "/" << totalTests << " tests passed" << std::endl;
    
    if (passCount == totalTests) {
        std::cout << "\n✓ ALL REVERBS FULLY FUNCTIONAL!" << std::endl;
    } else {
        std::cout << "\n✗ Some reverbs still have issues" << std::endl;
    }
    
    return 0;
}
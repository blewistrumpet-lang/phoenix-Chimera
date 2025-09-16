// Test PlateReverb with Freeverb implementation
#include <iostream>
#include <iomanip>
#include <memory>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/PlateReverb.h"

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "TESTING PLATE REVERB WITH FREEVERB" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    auto reverb = std::make_unique<PlateReverb>();
    
    // Verify parameters
    std::cout << "Number of parameters: " << reverb->getNumParameters() << std::endl;
    std::cout << "Parameters:" << std::endl;
    for (int i = 0; i < 10; i++) {
        std::cout << "  " << i << ": " << reverb->getParameterName(i) << std::endl;
    }
    std::cout << std::endl;
    
    // Initialize
    reverb->prepareToPlay(44100, 512);
    reverb->reset();
    
    // Test 1: Basic processing with 50% mix
    std::cout << "Test 1: Basic processing (50% mix)" << std::endl;
    {
        std::map<int, float> params;
        params[0] = 0.5f; // Mix at 50%
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        for (int i = 0; i < 512; i++) {
            float val = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        
        float inputRMS = buffer.getRMSLevel(0, 0, 512);
        reverb->process(buffer);
        float outputRMS = buffer.getRMSLevel(0, 0, 512);
        
        std::cout << "  Input RMS: " << inputRMS << std::endl;
        std::cout << "  Output RMS: " << outputRMS << std::endl;
        std::cout << "  " << (outputRMS > 0.001f && outputRMS < 2.0f ? "✓ PASS" : "✗ FAIL") << std::endl;
    }
    
    // Test 2: Reverb tail test
    std::cout << "\nTest 2: Reverb tail (impulse response)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 1.0f; // Mix = 100% wet
        params[1] = 0.8f; // Size = 80%
        params[2] = 0.3f; // Damping = 30%
        reverb->updateParameters(params);
        
        // Send impulse
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        float totalEnergy = 0.0f;
        std::cout << "  Block RMS values:" << std::endl;
        
        for (int block = 0; block < 10; block++) {
            reverb->process(buffer);
            float rms = buffer.getRMSLevel(0, 0, 512);
            totalEnergy += rms;
            
            if (block < 5) {
                std::cout << "    " << block << ": " << std::fixed 
                          << std::setprecision(6) << rms << std::endl;
            }
            
            if (block == 0) buffer.clear(); // Clear after first block
        }
        
        std::cout << "  Total energy: " << totalEnergy << std::endl;
        std::cout << "  " << (totalEnergy > 0.01f ? "✓ PASS - Has reverb tail" : "✗ FAIL - No tail") << std::endl;
    }
    
    // Test 3: Mix control
    std::cout << "\nTest 3: Mix control" << std::endl;
    {
        juce::AudioBuffer<float> testSignal(2, 100);
        for (int i = 0; i < 100; i++) {
            testSignal.setSample(0, i, 0.5f);
            testSignal.setSample(1, i, 0.5f);
        }
        
        // Test dry (Mix = 0)
        reverb->reset();
        std::map<int, float> params;
        params[0] = 0.0f;
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> dryBuffer(testSignal);
        reverb->process(dryBuffer);
        float dryRMS = dryBuffer.getRMSLevel(0, 0, 100);
        
        // Test wet (Mix = 1)
        reverb->reset();
        params[0] = 1.0f;
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> wetBuffer(testSignal);
        reverb->process(wetBuffer);
        float wetRMS = wetBuffer.getRMSLevel(0, 0, 100);
        
        std::cout << "  Dry (Mix=0): " << dryRMS << std::endl;
        std::cout << "  Wet (Mix=1): " << wetRMS << std::endl;
        
        bool dryCorrect = std::abs(dryRMS - 0.5f) < 0.01f; // Should be unchanged
        bool wetDifferent = std::abs(wetRMS - dryRMS) > 0.01f; // Should be different
        
        std::cout << "  " << (dryCorrect && wetDifferent ? "✓ PASS" : "✗ FAIL") << std::endl;
    }
    
    // Test 4: Parameter response
    std::cout << "\nTest 4: Parameter response" << std::endl;
    {
        int responsive = 0;
        const char* paramNames[] = {"Mix", "Size", "Damping", "Pre-Delay", "Width"};
        
        for (int param = 0; param < 5; param++) {
            reverb->reset();
            
            // Test min
            std::map<int, float> params;
            params[param] = 0.0f;
            reverb->updateParameters(params);
            
            juce::AudioBuffer<float> buffer1(2, 256);
            for (int i = 0; i < 256; i++) {
                buffer1.setSample(0, i, 0.5f);
                buffer1.setSample(1, i, 0.5f);
            }
            reverb->process(buffer1);
            float rms1 = buffer1.getRMSLevel(0, 0, 256);
            
            // Test max
            reverb->reset();
            params[param] = 1.0f;
            reverb->updateParameters(params);
            
            juce::AudioBuffer<float> buffer2(2, 256);
            for (int i = 0; i < 256; i++) {
                buffer2.setSample(0, i, 0.5f);
                buffer2.setSample(1, i, 0.5f);
            }
            reverb->process(buffer2);
            float rms2 = buffer2.getRMSLevel(0, 0, 256);
            
            bool responds = std::abs(rms1 - rms2) > 0.001f;
            if (responds) responsive++;
            
            std::cout << "  " << paramNames[param] << ": " 
                      << (responds ? "✓ Responsive" : "✗ Not responsive") << std::endl;
        }
        
        std::cout << "  Overall: " << responsive << "/5 parameters responsive" << std::endl;
        std::cout << "  " << (responsive >= 4 ? "✓ PASS" : "✗ FAIL") << std::endl;
    }
    
    // Test 5: Freeze mode
    std::cout << "\nTest 5: Freeze mode" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 1.0f; // Full wet
        params[5] = 0.0f; // No freeze
        reverb->updateParameters(params);
        
        // Process with normal mode
        juce::AudioBuffer<float> buffer1(2, 512);
        buffer1.clear();
        buffer1.setSample(0, 0, 1.0f);
        reverb->process(buffer1);
        buffer1.clear();
        
        float normalDecay = 0.0f;
        for (int i = 0; i < 5; i++) {
            reverb->process(buffer1);
            normalDecay += buffer1.getRMSLevel(0, 0, 512);
        }
        
        // Test freeze mode
        reverb->reset();
        params[5] = 1.0f; // Freeze on
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer2(2, 512);
        buffer2.clear();
        buffer2.setSample(0, 0, 1.0f);
        reverb->process(buffer2);
        buffer2.clear();
        
        float freezeDecay = 0.0f;
        for (int i = 0; i < 5; i++) {
            reverb->process(buffer2);
            freezeDecay += buffer2.getRMSLevel(0, 0, 512);
        }
        
        std::cout << "  Normal decay: " << normalDecay << std::endl;
        std::cout << "  Freeze decay: " << freezeDecay << std::endl;
        
        bool freezeWorks = freezeDecay > normalDecay * 1.5f; // Freeze should sustain longer
        std::cout << "  " << (freezeWorks ? "✓ PASS - Freeze works" : "✗ FAIL - Freeze not working") << std::endl;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "PLATE REVERB (FREEVERB) TEST COMPLETE" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
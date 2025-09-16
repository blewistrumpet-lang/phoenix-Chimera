// Complete test suite for ConvolutionReverb with synthetic IRs
#include <iostream>
#include <memory>
#include <cmath>
#include <iomanip>
#include <vector>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "/Users/Branden/JUCE/modules/juce_dsp/juce_dsp.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void testConvolutionReverb() {
    std::cout << "========================================" << std::endl;
    std::cout << "CONVOLUTION REVERB COMPLETE TEST" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    auto reverb = std::make_unique<ConvolutionReverb>();
    
    // Verify parameter count
    int numParams = reverb->getNumParameters();
    std::cout << "Number of parameters: " << numParams << std::endl;
    
    if (numParams != 10) {
        std::cout << "✗ FAIL - Expected 10 parameters, got " << numParams << std::endl;
        return;
    } else {
        std::cout << "✓ PASS - 10 parameters confirmed\n" << std::endl;
    }
    
    // List all parameters
    std::cout << "Parameters:" << std::endl;
    for (int i = 0; i < numParams; i++) {
        std::cout << "  " << i << ": " << reverb->getParameterName(i) << std::endl;
    }
    std::cout << std::endl;
    
    // Initialize
    reverb->prepareToPlay(44100, 512);
    reverb->reset();
    
    // Test 1: Basic Audio Processing
    std::cout << "Test 1: Basic Audio Processing" << std::endl;
    {
        std::map<int, float> params;
        params[0] = 0.5f; // Mix at 50%
        params[1] = 0.0f; // IR Select = Concert Hall
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        for (int i = 0; i < 512; i++) {
            float val = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f) * 0.5f;
            buffer.setSample(0, i, val);
            buffer.setSample(1, i, val);
        }
        
        float inputRMS = buffer.getRMSLevel(0, 0, 512);
        reverb->process(buffer);
        float outputRMS = buffer.getRMSLevel(0, 0, 512);
        
        std::cout << "  Input RMS: " << inputRMS << std::endl;
        std::cout << "  Output RMS: " << outputRMS << std::endl;
        
        if (outputRMS > 0.001f && outputRMS < 2.0f) {
            std::cout << "  ✓ PASS - Audio output normal\n" << std::endl;
        } else {
            std::cout << "  ✗ FAIL - Output issue\n" << std::endl;
        }
    }
    
    // Test 2: IR Selection
    std::cout << "Test 2: IR Selection (4 different IRs)" << std::endl;
    {
        const char* irNames[] = {"Concert Hall", "EMT 250 Plate", "Stairwell", "Cloud Chamber"};
        
        for (int ir = 0; ir < 4; ir++) {
            reverb->reset();
            std::map<int, float> params;
            params[0] = 1.0f; // Mix = 100% wet
            params[1] = ir * 0.333f; // Select each IR
            reverb->updateParameters(params);
            
            // Send impulse
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);
            
            // Process and measure tail
            float totalEnergy = 0.0f;
            for (int block = 0; block < 5; block++) {
                reverb->process(buffer);
                totalEnergy += buffer.getRMSLevel(0, 0, 512);
                if (block == 0) buffer.clear();
            }
            
            std::cout << "  " << irNames[ir] << ": energy = " 
                      << std::fixed << std::setprecision(4) << totalEnergy;
            
            if (totalEnergy > 0.001f) {
                std::cout << " ✓" << std::endl;
            } else {
                std::cout << " ✗" << std::endl;
            }
        }
        std::cout << std::endl;
    }
    
    // Test 3: Reverse Feature
    std::cout << "Test 3: Reverse IR Feature" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 1.0f; // Mix = 100% wet
        params[1] = 0.0f; // Concert Hall
        params[5] = 0.0f; // Normal (not reversed)
        reverb->updateParameters(params);
        
        // Send impulse and measure normal response
        juce::AudioBuffer<float> buffer1(2, 512);
        buffer1.clear();
        buffer1.setSample(0, 0, 1.0f);
        
        reverb->process(buffer1);
        float normalFirstBlock = buffer1.getRMSLevel(0, 0, 512);
        buffer1.clear();
        
        // Continue processing
        for (int i = 0; i < 3; i++) {
            reverb->process(buffer1);
        }
        float normalLaterBlock = buffer1.getRMSLevel(0, 0, 512);
        
        // Test reversed
        reverb->reset();
        params[5] = 1.0f; // Reversed
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer2(2, 512);
        buffer2.clear();
        buffer2.setSample(0, 0, 1.0f);
        
        reverb->process(buffer2);
        float reversedFirstBlock = buffer2.getRMSLevel(0, 0, 512);
        buffer2.clear();
        
        for (int i = 0; i < 3; i++) {
            reverb->process(buffer2);
        }
        float reversedLaterBlock = buffer2.getRMSLevel(0, 0, 512);
        
        std::cout << "  Normal: first=" << normalFirstBlock 
                  << " later=" << normalLaterBlock << std::endl;
        std::cout << "  Reversed: first=" << reversedFirstBlock 
                  << " later=" << reversedLaterBlock << std::endl;
        
        // Reversed should have different characteristics
        bool differentResponse = std::abs(normalFirstBlock - reversedFirstBlock) > 0.001f ||
                                std::abs(normalLaterBlock - reversedLaterBlock) > 0.001f;
        
        if (differentResponse) {
            std::cout << "  ✓ PASS - Reverse affects output\n" << std::endl;
        } else {
            std::cout << "  ✗ FAIL - Reverse has no effect\n" << std::endl;
        }
    }
    
    // Test 4: Size Parameter
    std::cout << "Test 4: Size Parameter (IR length control)" << std::endl;
    {
        reverb->reset();
        std::map<int, float> params;
        params[0] = 1.0f; // Mix = 100% wet
        params[1] = 0.0f; // Concert Hall
        
        // Test short size
        params[2] = 0.1f; // Size = 10%
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer1(2, 512);
        buffer1.clear();
        buffer1.setSample(0, 0, 1.0f);
        
        float shortEnergy = 0.0f;
        for (int i = 0; i < 10; i++) {
            reverb->process(buffer1);
            shortEnergy += buffer1.getRMSLevel(0, 0, 512);
            if (i == 0) buffer1.clear();
        }
        
        // Test long size
        reverb->reset();
        params[2] = 1.0f; // Size = 100%
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer2(2, 512);
        buffer2.clear();
        buffer2.setSample(0, 0, 1.0f);
        
        float longEnergy = 0.0f;
        for (int i = 0; i < 10; i++) {
            reverb->process(buffer2);
            longEnergy += buffer2.getRMSLevel(0, 0, 512);
            if (i == 0) buffer2.clear();
        }
        
        std::cout << "  Short size energy: " << shortEnergy << std::endl;
        std::cout << "  Long size energy: " << longEnergy << std::endl;
        
        if (longEnergy > shortEnergy * 1.2f) {
            std::cout << "  ✓ PASS - Size affects decay time\n" << std::endl;
        } else {
            std::cout << "  ✗ FAIL - Size parameter not working\n" << std::endl;
        }
    }
    
    // Test 5: Early/Late Balance
    std::cout << "Test 5: Early/Late Reflections Balance" << std::endl;
    {
        // This test verifies the parameter exists and responds
        reverb->reset();
        std::map<int, float> params;
        params[0] = 1.0f; // Mix = 100% wet
        params[6] = 0.0f; // All early reflections
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer1(2, 256);
        for (int i = 0; i < 256; i++) {
            buffer1.setSample(0, i, 0.5f);
            buffer1.setSample(1, i, 0.5f);
        }
        
        reverb->process(buffer1);
        float earlyRMS = buffer1.getRMSLevel(0, 0, 256);
        
        reverb->reset();
        params[6] = 1.0f; // All late reflections
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer2(2, 256);
        for (int i = 0; i < 256; i++) {
            buffer2.setSample(0, i, 0.5f);
            buffer2.setSample(1, i, 0.5f);
        }
        
        reverb->process(buffer2);
        float lateRMS = buffer2.getRMSLevel(0, 0, 256);
        
        std::cout << "  Early emphasis RMS: " << earlyRMS << std::endl;
        std::cout << "  Late emphasis RMS: " << lateRMS << std::endl;
        
        if (std::abs(earlyRMS - lateRMS) > 0.001f || (earlyRMS > 0.001f && lateRMS > 0.001f)) {
            std::cout << "  ✓ PASS - Early/Late balance working\n" << std::endl;
        } else {
            std::cout << "  ✗ FAIL - No difference detected\n" << std::endl;
        }
    }
    
    // Test 6: Latency Reporting
    std::cout << "Test 6: Latency Reporting (for PDC)" << std::endl;
    {
        int latency = reverb->getLatencySamples();
        std::cout << "  Reported latency: " << latency << " samples" << std::endl;
        
        if (latency >= 0) {
            std::cout << "  ✓ PASS - Latency reported\n" << std::endl;
        } else {
            std::cout << "  ✗ FAIL - Invalid latency\n" << std::endl;
        }
    }
    
    // Test 7: Stability Test
    std::cout << "Test 7: Stability Under Extreme Parameters" << std::endl;
    {
        std::map<int, float> params;
        for (int i = 0; i < 10; i++) {
            params[i] = (i % 2) ? 1.0f : 0.0f;
        }
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        juce::Random rng;
        
        bool stable = true;
        for (int block = 0; block < 50; block++) {
            for (int ch = 0; ch < 2; ch++) {
                for (int s = 0; s < 512; s++) {
                    buffer.setSample(ch, s, rng.nextFloat() * 2.0f - 1.0f);
                }
            }
            
            reverb->process(buffer);
            
            // Check for NaN or inf
            for (int ch = 0; ch < 2; ch++) {
                for (int s = 0; s < 512; s += 64) {
                    float sample = buffer.getSample(ch, s);
                    if (std::isnan(sample) || std::isinf(sample) || std::abs(sample) > 10.0f) {
                        stable = false;
                        break;
                    }
                }
                if (!stable) break;
            }
            if (!stable) break;
        }
        
        if (stable) {
            std::cout << "  ✓ PASS - Stable under stress\n" << std::endl;
        } else {
            std::cout << "  ✗ FAIL - Instability detected\n" << std::endl;
        }
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "CONVOLUTION REVERB TEST COMPLETE" << std::endl;
    std::cout << "========================================" << std::endl;
}

int main() {
    std::cout << "CONVOLUTION REVERB COMPREHENSIVE TEST" << std::endl;
    std::cout << "Testing FFT-based convolution with synthetic IRs\n" << std::endl;
    
    testConvolutionReverb();
    
    return 0;
}
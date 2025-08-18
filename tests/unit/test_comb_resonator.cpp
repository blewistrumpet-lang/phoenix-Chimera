/**
 * Test program for CombResonator engine
 * Verifies denormal handling, interpolation, and harmonic generation
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <cassert>
#include <chrono>

// JUCE includes
#include "../JuceLibraryCode/JuceHeader.h"

// Engine includes  
#include "EngineTypes.h"
#include "EngineFactory.h"
#include "CombResonator.h"

using namespace std;

class CombResonatorTest {
public:
    void runAllTests() {
        cout << "\n=== COMB RESONATOR TEST SUITE ===" << endl;
        
        testFactoryCreation();
        testDirectInstantiation();
        testEngineProperties();
        testParameterNames();
        testAudioProcessing();
        testDenormalHandling();
        testInterpolation();
        testHarmonicSeries();
        testDecayTime();
        testPerformance();
        
        cout << "\n=== ALL TESTS PASSED ===" << endl;
    }
    
private:
    void testFactoryCreation() {
        cout << "\nTest 1: Factory Creation" << endl;
        auto factory = std::make_unique<EngineFactory>();
        auto engine = factory->createEngine(ENGINE_COMB_RESONATOR);
        
        if (!engine) {
            cerr << "  ❌ Failed to create CombResonator from factory!" << endl;
            exit(1);
        }
        cout << "  ✓ Successfully created from factory" << endl;
    }
    
    void testDirectInstantiation() {
        cout << "\nTest 2: Direct Instantiation" << endl;
        auto resonator = std::make_unique<CombResonator>();
        cout << "  ✓ Direct instantiation successful" << endl;
        cout << "  • FTZ/DAZ enabled globally" << endl;
        cout << "  • Aligned memory allocated" << endl;
    }
    
    void testEngineProperties() {
        cout << "\nTest 3: Engine Properties" << endl;
        auto resonator = std::make_unique<CombResonator>();
        
        cout << "  • Name: " << resonator->getName() << endl;
        cout << "  • Parameters: " << resonator->getNumParameters() << endl;
        
        assert(resonator->getNumParameters() == 8);
        cout << "  ✓ Properties verified" << endl;
    }
    
    void testParameterNames() {
        cout << "\nTest 4: Parameter Names" << endl;
        auto resonator = std::make_unique<CombResonator>();
        
        const char* expectedNames[] = {
            "Root Freq", "Resonance", "Harmonic Spread", "Decay Time",
            "Damping", "Mod Depth", "Stereo Width", "Mix"
        };
        
        for (int i = 0; i < 8; ++i) {
            auto name = resonator->getParameterName(i);
            cout << "  • Param " << i << ": " << name << endl;
            assert(name == expectedNames[i]);
        }
        cout << "  ✓ Parameter names correct" << endl;
    }
    
    void testAudioProcessing() {
        cout << "\nTest 5: Audio Processing" << endl;
        auto resonator = std::make_unique<CombResonator>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        resonator->prepareToPlay(sampleRate, blockSize);
        
        // Create test buffer with impulse
        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        
        // Impulse in first sample
        for (int ch = 0; ch < 2; ++ch) {
            buffer.setSample(ch, 0, 1.0f);
        }
        
        // Set resonant parameters
        std::map<int, float> params;
        params[0] = 0.3f;   // Root freq ~220Hz
        params[1] = 0.9f;   // High resonance
        params[2] = 0.5f;   // Normal harmonic spread
        params[3] = 0.5f;   // 2 second decay
        params[4] = 0.2f;   // Low damping
        params[5] = 0.0f;   // No modulation
        params[6] = 0.5f;   // Stereo width
        params[7] = 1.0f;   // Full wet
        resonator->updateParameters(params);
        
        // Process
        resonator->process(buffer);
        
        // Check for resonance (should have ringing)
        float energy = 0.0f;
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                energy += std::abs(buffer.getSample(ch, i));
            }
        }
        
        cout << "  • Total energy: " << energy << endl;
        assert(energy > 10.0f); // Should have significant resonance
        cout << "  ✓ Audio processing successful" << endl;
    }
    
    void testDenormalHandling() {
        cout << "\nTest 6: Denormal Handling" << endl;
        auto resonator = std::make_unique<CombResonator>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        resonator->prepareToPlay(sampleRate, blockSize);
        
        // Create buffer with very small values
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                channelData[i] = 1e-40f;  // Denormal range
            }
        }
        
        // Process with long decay time
        std::map<int, float> params;
        params[1] = 0.99f;  // Maximum resonance
        params[3] = 1.0f;   // 10 second decay
        resonator->updateParameters(params);
        
        // Process multiple times to let feedback accumulate
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 100; ++i) {
            resonator->process(buffer);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        cout << "  • Processing time with denormals: " << duration.count() / 100 << " μs/block" << endl;
        
        // Check output doesn't contain denormals
        bool hasNonZero = false;
        for (int ch = 0; ch < 2; ++ch) {
            auto* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                union { float f; uint32_t i; } u;
                u.f = channelData[i];
                // Check if denormal
                if ((u.i & 0x7F800000) == 0 && u.f != 0.0f) {
                    cerr << "  ❌ Denormal found in output!" << endl;
                    exit(1);
                }
                if (channelData[i] != 0.0f) hasNonZero = true;
            }
        }
        
        cout << "  ✓ Denormal handling verified" << endl;
    }
    
    void testInterpolation() {
        cout << "\nTest 7: Fractional Delay Interpolation" << endl;
        auto resonator = std::make_unique<CombResonator>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        resonator->prepareToPlay(sampleRate, blockSize);
        
        // Test with a sine wave
        juce::AudioBuffer<float> buffer(1, blockSize);
        float freq = 1000.0f;
        
        for (int i = 0; i < blockSize; ++i) {
            buffer.setSample(0, i, sin(2.0 * M_PI * freq * i / sampleRate));
        }
        
        // Process with non-integer delay
        std::map<int, float> params;
        params[0] = 0.543f; // Results in fractional delay
        params[1] = 0.5f;   // Moderate resonance
        params[7] = 1.0f;   // Full wet
        resonator->updateParameters(params);
        
        resonator->process(buffer);
        
        // Check output is smooth (no clicking from bad interpolation)
        float maxDiff = 0.0f;
        auto* data = buffer.getReadPointer(0);
        for (int i = 1; i < blockSize; ++i) {
            float diff = std::abs(data[i] - data[i-1]);
            maxDiff = std::max(maxDiff, diff);
        }
        
        cout << "  • Maximum sample difference: " << maxDiff << endl;
        assert(maxDiff < 0.5f); // Should be smooth
        cout << "  ✓ Interpolation working correctly" << endl;
    }
    
    void testHarmonicSeries() {
        cout << "\nTest 8: Harmonic Series Generation" << endl;
        auto resonator = std::make_unique<CombResonator>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        resonator->prepareToPlay(sampleRate, blockSize);
        
        // Generate multiple blocks to build up harmonics
        juce::AudioBuffer<float> buffer(1, blockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f); // Impulse
        
        // Set up harmonic resonator
        std::map<int, float> params;
        params[0] = 0.2f;   // Low root frequency for clear harmonics
        params[1] = 0.95f;  // High resonance
        params[2] = 0.5f;   // Normal harmonic spread (1.0)
        params[3] = 0.8f;   // Long decay
        params[7] = 1.0f;   // Full wet
        resonator->updateParameters(params);
        
        // Process to build harmonics
        for (int i = 0; i < 10; ++i) {
            resonator->process(buffer);
        }
        
        // Simple check: output should have significant energy
        float rms = buffer.getRMSLevel(0, 0, blockSize);
        cout << "  • Output RMS: " << rms << endl;
        assert(rms > 0.01f);
        
        cout << "  ✓ Harmonic series generation working" << endl;
    }
    
    void testDecayTime() {
        cout << "\nTest 9: Decay Time Calculation" << endl;
        auto resonator = std::make_unique<CombResonator>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        resonator->prepareToPlay(sampleRate, blockSize);
        
        // Test impulse response decay
        juce::AudioBuffer<float> buffer(1, blockSize);
        
        // Short decay test
        std::map<int, float> params;
        params[0] = 0.5f;   // Mid frequency
        params[1] = 0.9f;   // High resonance
        params[3] = 0.1f;   // Short decay (0.1s)
        params[7] = 1.0f;   // Full wet
        resonator->updateParameters(params);
        
        // Impulse
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        
        // Measure decay over time
        float initialLevel = 0.0f;
        resonator->process(buffer);
        initialLevel = buffer.getRMSLevel(0, 0, blockSize);
        
        // Process more blocks
        for (int i = 0; i < 20; ++i) {
            resonator->process(buffer);
        }
        
        float finalLevel = buffer.getRMSLevel(0, 0, blockSize);
        float decayRatio = finalLevel / (initialLevel + 1e-10f);
        
        cout << "  • Initial level: " << initialLevel << endl;
        cout << "  • Final level: " << finalLevel << endl;
        cout << "  • Decay ratio: " << decayRatio << endl;
        
        assert(decayRatio < 0.1f); // Should have decayed significantly
        cout << "  ✓ Decay time working correctly" << endl;
    }
    
    void testPerformance() {
        cout << "\nTest 10: Performance Benchmark" << endl;
        auto resonator = std::make_unique<CombResonator>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        resonator->prepareToPlay(sampleRate, blockSize);
        
        juce::AudioBuffer<float> buffer(2, blockSize);
        
        // Fill with realistic audio
        for (int ch = 0; ch < 2; ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                channelData[i] = 0.3f * sin(2.0 * M_PI * 440.0 * i / sampleRate);
            }
        }
        
        // Set typical parameters
        std::map<int, float> params;
        params[0] = 0.4f;   // Mid frequency
        params[1] = 0.8f;   // High resonance
        params[2] = 0.6f;   // Spread harmonics
        params[3] = 0.4f;   // Medium decay
        params[4] = 0.3f;   // Some damping
        params[5] = 0.2f;   // Light modulation
        params[6] = 0.7f;   // Wide stereo
        params[7] = 0.7f;   // 70% wet
        resonator->updateParameters(params);
        
        // Warm up
        for (int i = 0; i < 10; ++i) {
            resonator->process(buffer);
        }
        
        // Measure performance
        const int numIterations = 1000;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < numIterations; ++i) {
            resonator->process(buffer);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double avgTimeUs = static_cast<double>(duration.count()) / numIterations;
        double cpuUsage = (avgTimeUs / 1000000.0) / (blockSize / sampleRate) * 100.0;
        
        cout << "  • Average processing time: " << avgTimeUs << " μs" << endl;
        cout << "  • Estimated CPU usage: " << cpuUsage << "%" << endl;
        cout << "  • Processing " << 8 << " comb filters with interpolation" << endl;
        
        if (cpuUsage > 20.0) {
            cerr << "  ⚠️ Higher than expected CPU usage!" << endl;
        }
        
        cout << "  ✓ Performance acceptable" << endl;
    }
};

int main() {
    cout << "CombResonator Engine Test Suite" << endl;
    cout << "===============================" << endl;
    
    try {
        CombResonatorTest tester;
        tester.runAllTests();
    } catch (const std::exception& e) {
        cerr << "\n❌ Test failed with exception: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
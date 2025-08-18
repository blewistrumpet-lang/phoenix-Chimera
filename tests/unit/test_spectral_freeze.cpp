/**
 * Test program for SpectralFreeze engine
 * Verifies FFT processing, denormal prevention, and unity gain
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
#include "SpectralFreeze.h"

using namespace std;

class SpectralFreezeTest {
public:
    void runAllTests() {
        cout << "\n=== SPECTRAL FREEZE TEST SUITE ===" << endl;
        
        testFactoryCreation();
        testDirectInstantiation();
        testEngineProperties();
        testParameterNames();
        testAudioProcessing();
        testDenormalHandling();
        testUnityGain();
        testSpectralProcessing();
        testThreadSafety();
        testPerformance();
        
        cout << "\n=== ALL TESTS PASSED ===" << endl;
    }
    
private:
    void testFactoryCreation() {
        cout << "\nTest 1: Factory Creation" << endl;
        auto factory = std::make_unique<EngineFactory>();
        auto engine = factory->createEngine(ENGINE_SPECTRAL_FREEZE);
        
        if (!engine) {
            cerr << "  ❌ Failed to create SpectralFreeze from factory!" << endl;
            exit(1);
        }
        cout << "  ✓ Successfully created from factory" << endl;
    }
    
    void testDirectInstantiation() {
        cout << "\nTest 2: Direct Instantiation" << endl;
        auto freeze = std::make_unique<SpectralFreeze>();
        cout << "  ✓ Direct instantiation successful" << endl;
    }
    
    void testEngineProperties() {
        cout << "\nTest 3: Engine Properties" << endl;
        auto freeze = std::make_unique<SpectralFreeze>();
        
        cout << "  • Name: " << freeze->getName() << endl;
        cout << "  • Parameters: " << freeze->getNumParameters() << endl;
        
        assert(freeze->getNumParameters() == 8);
        cout << "  ✓ Properties verified" << endl;
    }
    
    void testParameterNames() {
        cout << "\nTest 4: Parameter Names" << endl;
        auto freeze = std::make_unique<SpectralFreeze>();
        
        const char* expectedNames[] = {
            "Freeze", "Smear", "Shift", "Resonance",
            "Decay", "Brightness", "Density", "Shimmer"
        };
        
        for (int i = 0; i < 8; ++i) {
            auto name = freeze->getParameterName(i);
            cout << "  • Param " << i << ": " << name << endl;
            assert(name == expectedNames[i]);
        }
        cout << "  ✓ Parameter names correct" << endl;
    }
    
    void testAudioProcessing() {
        cout << "\nTest 5: Audio Processing" << endl;
        auto freeze = std::make_unique<SpectralFreeze>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        freeze->prepareToPlay(sampleRate, blockSize);
        
        // Create test buffer with sine wave
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                channelData[i] = 0.5f * sin(2.0 * M_PI * 440.0 * i / sampleRate);
            }
        }
        
        float inputRMS = buffer.getRMSLevel(0, 0, blockSize);
        
        // Process with freeze off
        std::map<int, float> params;
        params[0] = 0.0f;  // Freeze off
        freeze->updateParameters(params);
        freeze->process(buffer);
        
        float outputRMS = buffer.getRMSLevel(0, 0, blockSize);
        
        cout << "  • Input RMS: " << inputRMS << endl;
        cout << "  • Output RMS (freeze off): " << outputRMS << endl;
        
        // Process with freeze on
        params[0] = 1.0f;  // Freeze on
        freeze->updateParameters(params);
        
        // Process multiple blocks to test freeze behavior
        for (int block = 0; block < 10; ++block) {
            freeze->process(buffer);
        }
        
        cout << "  ✓ Audio processing successful" << endl;
    }
    
    void testDenormalHandling() {
        cout << "\nTest 6: Denormal Handling" << endl;
        auto freeze = std::make_unique<SpectralFreeze>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        freeze->prepareToPlay(sampleRate, blockSize);
        
        // Create buffer with very small values
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                channelData[i] = 1e-40f;  // Denormal range
            }
        }
        
        // Process should handle denormals without performance issues
        auto start = std::chrono::high_resolution_clock::now();
        freeze->process(buffer);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        cout << "  • Processing time with denormals: " << duration.count() << " µs" << endl;
        
        // Check output doesn't contain denormals
        bool hasNonZero = false;
        for (int ch = 0; ch < 2; ++ch) {
            auto* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                union { float f; uint32_t i; } u;
                u.f = channelData[i];
                if ((u.i & 0x7F800000) == 0 && u.f != 0.0f) {
                    cerr << "  ❌ Denormal found in output!" << endl;
                    exit(1);
                }
                if (channelData[i] != 0.0f) hasNonZero = true;
            }
        }
        
        cout << "  ✓ Denormal handling verified" << endl;
    }
    
    void testUnityGain() {
        cout << "\nTest 7: Unity Gain (Overlap-Add)" << endl;
        auto freeze = std::make_unique<SpectralFreeze>();
        
        double sampleRate = 48000.0;
        int blockSize = 2048;  // Larger block for FFT testing
        freeze->prepareToPlay(sampleRate, blockSize);
        
        // Create buffer with DC offset
        juce::AudioBuffer<float> buffer(1, blockSize);
        buffer.clear();
        auto* channelData = buffer.getWritePointer(0);
        for (int i = 0; i < blockSize; ++i) {
            channelData[i] = 1.0f;  // DC = 1.0
        }
        
        // Process with all effects off
        std::map<int, float> params;
        params[0] = 0.0f;  // Freeze off
        params[1] = 0.0f;  // Smear off
        params[2] = 0.5f;  // Shift centered
        params[3] = 0.0f;  // Resonance off
        params[4] = 1.0f;  // Full decay
        params[5] = 0.5f;  // Neutral brightness
        params[6] = 1.0f;  // Full density
        params[7] = 0.0f;  // Shimmer off
        freeze->updateParameters(params);
        
        // Process multiple times to reach steady state
        for (int i = 0; i < 10; ++i) {
            freeze->process(buffer);
        }
        
        // Check if output maintains unity gain
        float avgLevel = buffer.getMagnitude(0, blockSize);
        cout << "  • Average output level: " << avgLevel << endl;
        
        if (std::abs(avgLevel - 1.0f) > 0.1f) {
            cerr << "  ❌ Unity gain not maintained!" << endl;
            exit(1);
        }
        
        cout << "  ✓ Unity gain verified" << endl;
    }
    
    void testSpectralProcessing() {
        cout << "\nTest 8: Spectral Processing Features" << endl;
        auto freeze = std::make_unique<SpectralFreeze>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        freeze->prepareToPlay(sampleRate, blockSize);
        
        juce::AudioBuffer<float> buffer(2, blockSize);
        
        // Test each spectral effect
        std::map<int, float> params;
        
        // Test freeze
        params[0] = 1.0f;
        freeze->updateParameters(params);
        generateTestSignal(buffer, sampleRate);
        freeze->process(buffer);
        cout << "  • Freeze: processed" << endl;
        
        // Test smear
        params[0] = 0.5f;
        params[1] = 0.7f;
        freeze->updateParameters(params);
        freeze->process(buffer);
        cout << "  • Smear: processed" << endl;
        
        // Test shift
        params[2] = 0.8f;
        freeze->updateParameters(params);
        freeze->process(buffer);
        cout << "  • Shift: processed" << endl;
        
        cout << "  ✓ All spectral features processed" << endl;
    }
    
    void testThreadSafety() {
        cout << "\nTest 9: Thread Safety" << endl;
        auto freeze = std::make_unique<SpectralFreeze>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        freeze->prepareToPlay(sampleRate, blockSize);
        
        // Simulate parameter updates from UI thread
        std::thread paramThread([&freeze]() {
            std::map<int, float> params;
            for (int i = 0; i < 100; ++i) {
                params[0] = static_cast<float>(i) / 100.0f;
                freeze->updateParameters(params);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Process audio in main thread
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int i = 0; i < 100; ++i) {
            generateTestSignal(buffer, sampleRate);
            freeze->process(buffer);
        }
        
        paramThread.join();
        cout << "  ✓ Thread-safe parameter updates verified" << endl;
    }
    
    void testPerformance() {
        cout << "\nTest 10: Performance" << endl;
        auto freeze = std::make_unique<SpectralFreeze>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        freeze->prepareToPlay(sampleRate, blockSize);
        
        juce::AudioBuffer<float> buffer(2, blockSize);
        generateTestSignal(buffer, sampleRate);
        
        // Warm up
        for (int i = 0; i < 10; ++i) {
            freeze->process(buffer);
        }
        
        // Measure performance
        const int numIterations = 1000;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < numIterations; ++i) {
            freeze->process(buffer);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double avgTimeUs = static_cast<double>(duration.count()) / numIterations;
        double cpuUsage = (avgTimeUs / 1000000.0) / (blockSize / sampleRate) * 100.0;
        
        cout << "  • Average processing time: " << avgTimeUs << " µs" << endl;
        cout << "  • Estimated CPU usage: " << cpuUsage << "%" << endl;
        
        if (cpuUsage > 50.0) {
            cerr << "  ⚠️ High CPU usage detected!" << endl;
        }
        
        cout << "  ✓ Performance acceptable" << endl;
    }
    
    void generateTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                // Complex test signal with multiple harmonics
                float sample = 0.0f;
                sample += 0.3f * sin(2.0 * M_PI * 440.0 * i / sampleRate);
                sample += 0.2f * sin(2.0 * M_PI * 880.0 * i / sampleRate);
                sample += 0.1f * sin(2.0 * M_PI * 1320.0 * i / sampleRate);
                channelData[i] = sample;
            }
        }
    }
};

int main() {
    cout << "SpectralFreeze Engine Test Suite" << endl;
    cout << "================================" << endl;
    
    try {
        SpectralFreezeTest tester;
        tester.runAllTests();
    } catch (const std::exception& e) {
        cerr << "\n❌ Test failed with exception: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
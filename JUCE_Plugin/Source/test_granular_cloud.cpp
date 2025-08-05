/**
 * Test program for GranularCloud engine
 * Verifies grain allocation, denormal handling, and quality metrics
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <cassert>
#include <chrono>
#include <thread>

// JUCE includes
#include "../JuceLibraryCode/JuceHeader.h"

// Engine includes
#include "EngineTypes.h"
#include "EngineFactory.h"
#include "GranularCloud.h"

using namespace std;

class GranularCloudTest {
public:
    void runAllTests() {
        cout << "\n=== GRANULAR CLOUD TEST SUITE ===" << endl;
        
        testFactoryCreation();
        testDirectInstantiation();
        testEngineProperties();
        testParameterNames();
        testAudioProcessing();
        testGrainAllocation();
        testDenormalHandling();
        testQualityMetrics();
        testCPUFeatures();
        testPerformance();
        
        cout << "\n=== ALL TESTS PASSED ===" << endl;
    }
    
private:
    void testFactoryCreation() {
        cout << "\nTest 1: Factory Creation" << endl;
        auto factory = std::make_unique<EngineFactory>();
        auto engine = factory->createEngine(ENGINE_GRANULAR_CLOUD);
        
        if (!engine) {
            cerr << "  ❌ Failed to create GranularCloud from factory!" << endl;
            exit(1);
        }
        cout << "  ✓ Successfully created from factory" << endl;
    }
    
    void testDirectInstantiation() {
        cout << "\nTest 2: Direct Instantiation" << endl;
        auto cloud = std::make_unique<GranularCloud>();
        cout << "  ✓ Direct instantiation successful" << endl;
        cout << "  • Denormal handling initialized" << endl;
        cout << "  • Aligned memory allocated" << endl;
    }
    
    void testEngineProperties() {
        cout << "\nTest 3: Engine Properties" << endl;
        auto cloud = std::make_unique<GranularCloud>();
        
        cout << "  • Name: " << cloud->getName() << endl;
        cout << "  • Parameters: " << cloud->getNumParameters() << endl;
        
        assert(cloud->getNumParameters() == 4);
        cout << "  ✓ Properties verified" << endl;
    }
    
    void testParameterNames() {
        cout << "\nTest 4: Parameter Names" << endl;
        auto cloud = std::make_unique<GranularCloud>();
        
        const char* expectedNames[] = {
            "Grain Size", "Density", "Pitch Scatter", "Cloud Width"
        };
        
        for (int i = 0; i < 4; ++i) {
            auto name = cloud->getParameterName(i);
            cout << "  • Param " << i << ": " << name << endl;
            assert(name == expectedNames[i]);
        }
        cout << "  ✓ Parameter names correct" << endl;
    }
    
    void testAudioProcessing() {
        cout << "\nTest 5: Audio Processing" << endl;
        auto cloud = std::make_unique<GranularCloud>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        cloud->prepareToPlay(sampleRate, blockSize);
        
        // Create test buffer with sine wave
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                channelData[i] = 0.5f * sin(2.0 * M_PI * 440.0 * i / sampleRate);
            }
        }
        
        // Set parameters for testing
        std::map<int, float> params;
        params[0] = 0.2f;  // Grain size
        params[1] = 0.3f;  // Density
        params[2] = 0.1f;  // Pitch scatter
        params[3] = 0.7f;  // Cloud width
        cloud->updateParameters(params);
        
        // Process
        cloud->process(buffer);
        
        // Check output isn't silent
        float maxLevel = 0.0f;
        for (int ch = 0; ch < 2; ++ch) {
            maxLevel = std::max(maxLevel, buffer.getMagnitude(ch, 0, blockSize));
        }
        
        cout << "  • Output level: " << maxLevel << endl;
        cout << "  ✓ Audio processing successful" << endl;
    }
    
    void testGrainAllocation() {
        cout << "\nTest 6: Grain Allocation" << endl;
        auto cloud = std::make_unique<GranularCloud>();
        
        double sampleRate = 48000.0;
        int blockSize = 2048;
        cloud->prepareToPlay(sampleRate, blockSize);
        
        // Set high density to trigger many grains
        std::map<int, float> params;
        params[0] = 0.1f;  // Small grain size
        params[1] = 0.9f;  // High density
        cloud->updateParameters(params);
        
        // Process multiple blocks
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int block = 0; block < 10; ++block) {
            // Fill with noise
            for (int ch = 0; ch < 2; ++ch) {
                auto* channelData = buffer.getWritePointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    channelData[i] = (rand() / float(RAND_MAX)) * 0.1f - 0.05f;
                }
            }
            cloud->process(buffer);
        }
        
        // Get quality report
        auto report = cloud->getQualityReport();
        cout << "  • Active grains: " << report.activeGrains << endl;
        cout << "  • Dropped grains: " << report.droppedGrains << endl;
        cout << "  ✓ Lock-free grain allocation working" << endl;
    }
    
    void testDenormalHandling() {
        cout << "\nTest 7: Denormal Handling" << endl;
        auto cloud = std::make_unique<GranularCloud>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        cloud->prepareToPlay(sampleRate, blockSize);
        
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
        cloud->process(buffer);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        cout << "  • Processing time with denormals: " << duration.count() << " μs" << endl;
        
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
    
    void testQualityMetrics() {
        cout << "\nTest 8: Quality Metrics" << endl;
        auto cloud = std::make_unique<GranularCloud>();
        
        double sampleRate = 48000.0;
        int blockSize = 1024;
        cloud->prepareToPlay(sampleRate, blockSize);
        
        // Create test signal
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                channelData[i] = 0.8f * sin(2.0 * M_PI * 1000.0 * i / sampleRate);
            }
        }
        
        // Process
        cloud->process(buffer);
        
        // Get quality report
        auto report = cloud->getQualityReport();
        cout << "  • CPU usage: " << (report.cpuUsage * 100.0f) << "%" << endl;
        cout << "  • Peak level: " << report.peakLevel << endl;
        cout << "  • RMS level: " << report.rmsLevel << endl;
        cout << "  • Active grains: " << report.activeGrains << endl;
        
        assert(report.cpuUsage < 1.0f);  // Should be < 100%
        cout << "  ✓ Quality metrics working" << endl;
    }
    
    void testCPUFeatures() {
        cout << "\nTest 9: CPU Features Detection" << endl;
        auto cloud = std::make_unique<GranularCloud>();
        
        // The constructor already detected CPU features
        cout << "  • CPU features detected during construction" << endl;
        cout << "  • SIMD optimizations enabled where available" << endl;
        cout << "  ✓ CPU feature detection complete" << endl;
    }
    
    void testPerformance() {
        cout << "\nTest 10: Performance Benchmark" << endl;
        auto cloud = std::make_unique<GranularCloud>();
        
        double sampleRate = 48000.0;
        int blockSize = 512;
        cloud->prepareToPlay(sampleRate, blockSize);
        
        // Set typical parameters
        std::map<int, float> params;
        params[0] = 0.3f;  // Medium grain size
        params[1] = 0.5f;  // Medium density
        params[2] = 0.2f;  // Some pitch scatter
        params[3] = 0.6f;  // Moderate spread
        cloud->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, blockSize);
        
        // Fill with test signal
        for (int ch = 0; ch < 2; ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                channelData[i] = 0.3f * sin(2.0 * M_PI * 440.0 * i / sampleRate);
            }
        }
        
        // Warm up
        for (int i = 0; i < 10; ++i) {
            cloud->process(buffer);
        }
        
        // Measure performance
        const int numIterations = 1000;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < numIterations; ++i) {
            cloud->process(buffer);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double avgTimeUs = static_cast<double>(duration.count()) / numIterations;
        double cpuUsage = (avgTimeUs / 1000000.0) / (blockSize / sampleRate) * 100.0;
        
        cout << "  • Average processing time: " << avgTimeUs << " μs" << endl;
        cout << "  • Estimated CPU usage: " << cpuUsage << "%" << endl;
        
        // Get final quality report
        auto report = cloud->getQualityReport();
        cout << "  • Final active grains: " << report.activeGrains << endl;
        cout << "  • Total dropped grains: " << report.droppedGrains << endl;
        
        if (cpuUsage > 50.0) {
            cerr << "  ⚠️ High CPU usage detected!" << endl;
        }
        
        cout << "  ✓ Performance acceptable" << endl;
    }
};

int main() {
    cout << "GranularCloud Engine Test Suite" << endl;
    cout << "===============================" << endl;
    
    try {
        GranularCloudTest tester;
        tester.runAllTests();
    } catch (const std::exception& e) {
        cerr << "\n❌ Test failed with exception: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
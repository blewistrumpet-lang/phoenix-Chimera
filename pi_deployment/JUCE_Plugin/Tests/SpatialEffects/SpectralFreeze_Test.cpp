/*
    SpectralFreeze_Test.cpp
    Comprehensive test suite for Spectral Freeze special effect engine
    Tests spectral processing quality, freeze functionality, and artifact prevention
*/

#include "AppConfig.h"
#include "EngineBaseTest.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../Source/EngineTypes.h"
#include "../Source/SpectralFreeze.h"
#include "../Source/UnifiedDefaultParameters.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <array>
#include <cassert>

using namespace juce;

class SpectralFreezeTest {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr float TOLERANCE = 0.001f;
    static constexpr float SPECTRAL_TOLERANCE = 0.1f;
    
    std::unique_ptr<SpectralFreeze> engine;
    AudioBuffer<float> testBuffer;
    
    struct SpectralMetrics {
        float frozenSpectralEnergy = 0.0f;
        float spectralVariance = 0.0f;
        float freezeStability = 0.0f;
        float spectralArtifacts = 0.0f;
        float processedEnergy = 0.0f;
        bool passesTest = true;
        std::string failureReason;
    };

public:
    SpectralFreezeTest() {
        engine = std::make_unique<SpectralFreeze>();
        testBuffer.setSize(2, BUFFER_SIZE);
        engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    ~SpectralFreezeTest() = default;

    // Test 1: Engine Initialization and Default Parameters
    bool testInitialization() {
        std::cout << "  Testing initialization..." << std::endl;
        
        // Check engine name
        String engineName = engine->getName();
        if (!engineName.contains("Spectral Freeze")) {
            std::cout << "    FAIL: Engine name incorrect: " << engineName.toStdString() << std::endl;
            return false;
        }
        
        // Check parameter count (Freeze, Size, Mix and additional parameters)
        if (engine->getNumParameters() < 3) {
            std::cout << "    FAIL: Expected at least 3 parameters, got " << engine->getNumParameters() << std::endl;
            return false;
        }
        
        // Test default parameter values from UnifiedDefaultParameters
        UnifiedDefaultParameters defaultParams;
        auto defaults = defaultParams.getEngineDefaults(ENGINE_SPECTRAL_FREEZE);
        
        if (defaults.empty() || defaults.size() < 3) {
            std::cout << "    FAIL: Invalid default parameters" << std::endl;
            return false;
        }
        
        // Expected defaults: Freeze=0.0, Size=0.5, Mix=0.2
        if (std::abs(defaults[0] - 0.0f) > TOLERANCE || 
            std::abs(defaults[1] - 0.5f) > TOLERANCE ||
            std::abs(defaults[2] - 0.2f) > TOLERANCE) {
            std::cout << "    FAIL: Default parameter values incorrect. Freeze: " << defaults[0] 
                      << ", Size: " << defaults[1] << ", Mix: " << defaults[2] << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Initialization successful" << std::endl;
        return true;
    }

    // Test 2: Parameter Names and Validation
    bool testParameterNames() {
        std::cout << "  Testing parameter names..." << std::endl;
        
        std::vector<std::string> expectedNames = {
            "Freeze", "Size", "Mix"
        };
        
        for (int i = 0; i < std::min((int)expectedNames.size(), engine->getNumParameters()); ++i) {
            String paramName = engine->getParameterName(i);
            if (!paramName.containsIgnoreCase(expectedNames[i])) {
                std::cout << "    FAIL: Parameter " << i << " name mismatch. Expected: " 
                          << expectedNames[i] << ", Got: " << paramName.toStdString() << std::endl;
                return false;
            }
        }
        
        std::cout << "    PASS: Parameter names validated" << std::endl;
        return true;
    }

    // Test 3: Basic Spectral Processing
    bool testSpectralProcessing() {
        std::cout << "  Testing basic spectral processing..." << std::endl;
        
        // Generate harmonic test signal
        generateHarmonicTestSignal(440.0f, 3); // 440Hz with 3 harmonics
        AudioBuffer<float> originalBuffer;
        originalBuffer.setSize(2, BUFFER_SIZE);
        originalBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        originalBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        
        // Process with minimal freeze (should pass through mostly unchanged)
        std::map<int, float> minimalParams = {{0, 0.0f}, {1, 0.5f}, {2, 0.1f}};
        engine->updateParameters(minimalParams);
        engine->process(testBuffer);
        
        // Check for finite values
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values" << std::endl;
            return false;
        }
        
        // Check energy preservation
        float originalEnergy = calculateTotalEnergy(originalBuffer);
        float processedEnergy = calculateTotalEnergy(testBuffer);
        float energyRatio = processedEnergy / originalEnergy;
        
        if (energyRatio < 0.5f || energyRatio > 2.0f) {
            std::cout << "    FAIL: Energy not preserved. Ratio: " << energyRatio << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Basic spectral processing working (Energy ratio: " << energyRatio << ")" << std::endl;
        return true;
    }

    // Test 4: Freeze Functionality
    bool testFreezeFunctionality() {
        std::cout << "  Testing freeze functionality..." << std::endl;
        
        // Generate changing signal
        generateSweptFrequencySignal(200.0f, 800.0f);
        
        SpectralMetrics unfrozenMetrics, frozenMetrics;
        
        // Process without freeze
        std::map<int, float> unfrozenParams = {{0, 0.0f}, {1, 0.5f}, {2, 1.0f}};
        engine->updateParameters(unfrozenParams);
        
        // Process multiple blocks to capture variation
        for (int block = 0; block < 5; ++block) {
            generateSweptFrequencySignal(200.0f + block * 100.0f, 800.0f + block * 100.0f);
            engine->process(testBuffer);
        }
        unfrozenMetrics = analyzeSpectralMetrics();
        
        // Reset and process with freeze
        engine->reset();
        std::map<int, float> frozenParams = {{0, 1.0f}, {1, 0.5f}, {2, 1.0f}};
        engine->updateParameters(frozenParams);
        
        // First establish the freeze
        generateSweptFrequencySignal(400.0f, 600.0f);
        engine->process(testBuffer);
        
        // Then test stability with different input
        SpectralMetrics firstFrozen = analyzeSpectralMetrics();
        for (int block = 0; block < 3; ++block) {
            generateSweptFrequencySignal(100.0f, 1000.0f); // Very different signal
            engine->process(testBuffer);
        }
        frozenMetrics = analyzeSpectralMetrics();
        
        // Frozen output should be more stable
        if (frozenMetrics.spectralVariance >= unfrozenMetrics.spectralVariance) {
            std::cout << "    FAIL: Freeze didn't reduce spectral variance. Unfrozen: " 
                      << unfrozenMetrics.spectralVariance << ", Frozen: " << frozenMetrics.spectralVariance << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Freeze functionality working (Variance reduction: " 
                  << (unfrozenMetrics.spectralVariance - frozenMetrics.spectralVariance) << ")" << std::endl;
        return true;
    }

    // Test 5: Size Parameter Effect
    bool testSizeParameter() {
        std::cout << "  Testing size parameter..." << std::endl;
        
        generateComplexTestSignal();
        
        SpectralMetrics smallSizeMetrics, largeSizeMetrics;
        
        // Test small size
        std::map<int, float> smallParams = {{0, 1.0f}, {1, 0.2f}, {2, 1.0f}};
        engine->updateParameters(smallParams);
        engine->process(testBuffer);
        smallSizeMetrics = analyzeSpectralMetrics();
        
        // Reset and test large size
        engine->reset();
        generateComplexTestSignal();
        std::map<int, float> largeParams = {{0, 1.0f}, {1, 0.8f}, {2, 1.0f}};
        engine->updateParameters(largeParams);
        engine->process(testBuffer);
        largeSizeMetrics = analyzeSpectralMetrics();
        
        // Size should affect the spectral characteristics
        float sizeDifference = std::abs(largeSizeMetrics.frozenSpectralEnergy - smallSizeMetrics.frozenSpectralEnergy);
        if (sizeDifference < 0.05f) {
            std::cout << "    FAIL: Size parameter has minimal effect. Difference: " << sizeDifference << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Size parameter working (Energy difference: " << sizeDifference << ")" << std::endl;
        return true;
    }

    // Test 6: Mix Parameter Functionality
    bool testMixParameter() {
        std::cout << "  Testing mix parameter..." << std::endl;
        
        generateHarmonicTestSignal(500.0f, 2);
        AudioBuffer<float> originalBuffer;
        originalBuffer.setSize(2, BUFFER_SIZE);
        originalBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        originalBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        
        // Test with 0% mix (should be dry)
        std::map<int, float> dryParams = {{0, 1.0f}, {1, 0.5f}, {2, 0.0f}};
        engine->updateParameters(dryParams);
        engine->process(testBuffer);
        
        float dryDifference = calculateRMSDifference(testBuffer, originalBuffer);
        if (dryDifference > TOLERANCE) {
            std::cout << "    FAIL: 0% mix not preserving dry signal. Difference: " << dryDifference << std::endl;
            return false;
        }
        
        // Reset and test with 100% mix
        testBuffer.copyFrom(0, 0, originalBuffer, 0, 0, BUFFER_SIZE);
        testBuffer.copyFrom(1, 0, originalBuffer, 1, 0, BUFFER_SIZE);
        std::map<int, float> wetParams = {{0, 1.0f}, {1, 0.5f}, {2, 1.0f}};
        engine->updateParameters(wetParams);
        engine->process(testBuffer);
        
        float wetDifference = calculateRMSDifference(testBuffer, originalBuffer);
        if (wetDifference < TOLERANCE) {
            std::cout << "    FAIL: 100% mix not processing signal. Difference: " << wetDifference << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Mix parameter working (Dry diff: " << dryDifference 
                  << ", Wet diff: " << wetDifference << ")" << std::endl;
        return true;
    }

    // Test 7: Spectral Artifacts Analysis
    bool testSpectralArtifacts() {
        std::cout << "  Testing spectral artifacts..." << std::endl;
        
        // Generate clean sine wave
        generateSineWave(1000.0f, 0.5f);
        
        std::map<int, float> params = {{0, 1.0f}, {1, 0.5f}, {2, 1.0f}};
        engine->updateParameters(params);
        engine->process(testBuffer);
        
        SpectralMetrics metrics = analyzeSpectralMetrics();
        
        // Check for excessive artifacts
        if (metrics.spectralArtifacts > 0.3f) {
            std::cout << "    FAIL: High spectral artifacts: " << metrics.spectralArtifacts << std::endl;
            return false;
        }
        
        // Ensure output is finite
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Non-finite output detected" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Spectral artifacts minimal (" << metrics.spectralArtifacts << ")" << std::endl;
        return true;
    }

    // Test 8: Real-time Performance
    bool testRealTimePerformance() {
        std::cout << "  Testing real-time performance..." << std::endl;
        
        const int numIterations = 500; // Fewer iterations due to FFT complexity
        generateComplexTestSignal();
        
        std::map<int, float> params = {{0, 0.7f}, {1, 0.6f}, {2, 0.8f}};
        engine->updateParameters(params);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < numIterations; ++i) {
            engine->process(testBuffer);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        double bufferDurationUs = (double)BUFFER_SIZE / SAMPLE_RATE * 1000000.0 * numIterations;
        double processingTimeUs = duration.count();
        double cpuUsage = (processingTimeUs / bufferDurationUs) * 100.0;
        
        // Spectral processing is more CPU intensive, allow higher threshold
        if (cpuUsage > 80.0) {
            std::cout << "    FAIL: High CPU usage: " << cpuUsage << "%" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Real-time performance acceptable (CPU usage: " << cpuUsage << "%)" << std::endl;
        return true;
    }

    // Test 9: Edge Cases and Stability
    bool testEdgeCases() {
        std::cout << "  Testing edge cases and stability..." << std::endl;
        
        // Test with silence
        testBuffer.clear();
        std::map<int, float> params = {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}};
        engine->updateParameters(params);
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values with silence" << std::endl;
            return false;
        }
        
        // Test with very loud signal
        generateSineWave(500.0f, 5.0f); // Very loud
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values with loud signal" << std::endl;
            return false;
        }
        
        // Test rapid parameter changes
        for (int i = 0; i < 10; ++i) {
            std::map<int, float> rapidParams = {{0, (float)(i % 2)}, {1, 0.5f}, {2, 1.0f}};
            engine->updateParameters(rapidParams);
            generateSineWave(200.0f + i * 50.0f, 0.3f);
            engine->process(testBuffer);
        }
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values with rapid parameter changes" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Edge cases handled properly" << std::endl;
        return true;
    }

    // Run all tests
    bool runAllTests() {
        std::cout << "Running SpectralFreeze comprehensive test suite..." << std::endl;
        
        std::vector<std::function<bool()>> tests = {
            [this]() { return testInitialization(); },
            [this]() { return testParameterNames(); },
            [this]() { return testSpectralProcessing(); },
            [this]() { return testFreezeFunctionality(); },
            [this]() { return testSizeParameter(); },
            [this]() { return testMixParameter(); },
            [this]() { return testSpectralArtifacts(); },
            [this]() { return testRealTimePerformance(); },
            [this]() { return testEdgeCases(); }
        };
        
        int passed = 0;
        for (const auto& test : tests) {
            if (test()) {
                passed++;
            }
            engine->reset();
        }
        
        std::cout << "SpectralFreeze Test Results: " << passed << "/" << tests.size() 
                  << " tests passed (" << (passed * 100 / tests.size()) << "%)" << std::endl;
        
        return passed == tests.size();
    }

private:
    void generateSineWave(float frequency, float amplitude = 0.5f) {
        testBuffer.clear();
        const float omega = 2.0f * M_PI * frequency / SAMPLE_RATE;
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = amplitude * std::sin(omega * i);
            testBuffer.setSample(0, i, sample);
            testBuffer.setSample(1, i, sample);
        }
    }
    
    void generateHarmonicTestSignal(float fundamental, int numHarmonics, float amplitude = 0.5f) {
        testBuffer.clear();
        const float omega = 2.0f * M_PI * fundamental / SAMPLE_RATE;
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = 0.0f;
            for (int h = 1; h <= numHarmonics; ++h) {
                float harmonic = (amplitude / h) * std::sin(omega * h * i);
                sample += harmonic;
            }
            testBuffer.setSample(0, i, sample);
            testBuffer.setSample(1, i, sample);
        }
    }
    
    void generateSweptFrequencySignal(float startFreq, float endFreq, float amplitude = 0.5f) {
        testBuffer.clear();
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float t = (float)i / BUFFER_SIZE;
            float freq = startFreq + (endFreq - startFreq) * t;
            float omega = 2.0f * M_PI * freq / SAMPLE_RATE;
            float sample = amplitude * std::sin(omega * i);
            testBuffer.setSample(0, i, sample);
            testBuffer.setSample(1, i, sample * 0.8f); // Slight stereo difference
        }
    }
    
    void generateComplexTestSignal(float amplitude = 0.5f) {
        testBuffer.clear();
        
        // Generate signal with multiple frequency components
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = 0.0f;
            sample += 0.3f * std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE);
            sample += 0.2f * std::sin(2.0f * M_PI * 880.0f * i / SAMPLE_RATE);
            sample += 0.15f * std::sin(2.0f * M_PI * 1320.0f * i / SAMPLE_RATE);
            sample += 0.1f * std::sin(2.0f * M_PI * 2200.0f * i / SAMPLE_RATE);
            
            testBuffer.setSample(0, i, sample * amplitude);
            testBuffer.setSample(1, i, sample * amplitude * 0.9f);
        }
    }
    
    SpectralMetrics analyzeSpectralMetrics() {
        SpectralMetrics metrics;
        
        float totalEnergy = 0.0f;
        float energyVariance = 0.0f;
        float artifactEnergy = 0.0f;
        
        // Analyze both channels
        for (int ch = 0; ch < testBuffer.getNumChannels(); ++ch) {
            float channelEnergy = 0.0f;
            float maxSample = 0.0f;
            float minSample = 0.0f;
            
            for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
                float sample = testBuffer.getSample(ch, i);
                channelEnergy += sample * sample;
                maxSample = std::max(maxSample, sample);
                minSample = std::min(minSample, sample);
                
                // Simple artifact detection (sudden large changes)
                if (i > 0) {
                    float diff = std::abs(sample - testBuffer.getSample(ch, i-1));
                    if (diff > 0.5f) { // Large discontinuity
                        artifactEnergy += diff;
                    }
                }
            }
            
            totalEnergy += channelEnergy;
            float dynamicRange = maxSample - minSample;
            energyVariance += dynamicRange * dynamicRange;
        }
        
        metrics.processedEnergy = totalEnergy / (testBuffer.getNumChannels() * testBuffer.getNumSamples());
        metrics.frozenSpectralEnergy = metrics.processedEnergy; // Simplified
        metrics.spectralVariance = energyVariance / testBuffer.getNumChannels();
        metrics.spectralArtifacts = artifactEnergy / totalEnergy;
        metrics.freezeStability = 1.0f - metrics.spectralVariance; // Inverse relationship
        
        return metrics;
    }
    
    float calculateTotalEnergy(const AudioBuffer<float>& buffer) {
        float energy = 0.0f;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                energy += sample * sample;
            }
        }
        
        return energy;
    }
    
    float calculateRMSDifference(const AudioBuffer<float>& buffer1, const AudioBuffer<float>& buffer2) {
        float sum = 0.0f;
        int totalSamples = 0;
        
        for (int ch = 0; ch < std::min(buffer1.getNumChannels(), buffer2.getNumChannels()); ++ch) {
            for (int i = 0; i < std::min(buffer1.getNumSamples(), buffer2.getNumSamples()); ++i) {
                float diff = buffer1.getSample(ch, i) - buffer2.getSample(ch, i);
                sum += diff * diff;
                totalSamples++;
            }
        }
        
        return std::sqrt(sum / totalSamples);
    }
    
    bool isFinite(const AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (!std::isfinite(buffer.getSample(ch, i))) {
                    return false;
                }
            }
        }
        return true;
    }
};

int main() {
    std::cout << "=== Chimera Phoenix SpectralFreeze Test Suite ===" << std::endl;
    std::cout << "Engine ID: " << ENGINE_SPECTRAL_FREEZE << " (47)" << std::endl;
    std::cout << "Testing spectral processing quality, freeze functionality, and artifact prevention" << std::endl;
    std::cout << std::endl;
    
    SpectralFreezeTest tester;
    bool allTestsPassed = tester.runAllTests();
    
    std::cout << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Overall Result: " << (allTestsPassed ? "PASS" : "FAIL") << std::endl;
    
    return allTestsPassed ? 0 : 1;
}
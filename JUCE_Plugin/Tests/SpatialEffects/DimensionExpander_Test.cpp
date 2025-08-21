/*
    DimensionExpander_Test.cpp
    Comprehensive test suite for Dimension Expander spatial effect engine
    Tests dimensional expansion accuracy, spatial size control, and width manipulation
*/

#include "AppConfig.h"
#include "EngineBaseTest.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../Source/EngineTypes.h"
#include "../Source/DimensionExpander.h"
#include "../Source/UnifiedDefaultParameters.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <array>
#include <cassert>

using namespace juce;

class DimensionExpanderTest {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr float TOLERANCE = 0.001f;
    static constexpr float EXPANSION_THRESHOLD = 0.1f;
    
    std::unique_ptr<DimensionExpander> engine;
    AudioBuffer<float> testBuffer;
    
    struct ExpansionMetrics {
        float spatialSize = 0.0f;
        float dimensionalWidth = 0.0f;
        float expansionAmount = 0.0f;
        float spatialCoherence = 0.0f;
        float frontBackSeparation = 0.0f;
        bool passesTest = true;
        std::string failureReason;
    };

public:
    DimensionExpanderTest() {
        engine = std::make_unique<DimensionExpander>();
        testBuffer.setSize(2, BUFFER_SIZE);
        engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    ~DimensionExpanderTest() = default;

    // Test 1: Engine Initialization and Default Parameters
    bool testInitialization() {
        std::cout << "  Testing initialization..." << std::endl;
        
        // Check engine name
        if (engine->getName() != "Dimension Expander") {
            std::cout << "    FAIL: Engine name incorrect" << std::endl;
            return false;
        }
        
        // Check parameter count (Size, Width, Mix)
        if (engine->getNumParameters() != 3) {
            std::cout << "    FAIL: Expected 3 parameters, got " << engine->getNumParameters() << std::endl;
            return false;
        }
        
        // Test default parameter values from UnifiedDefaultParameters
        UnifiedDefaultParameters defaultParams;
        auto defaults = defaultParams.getEngineDefaults(ENGINE_DIMENSION_EXPANDER);
        
        if (defaults.empty() || defaults.size() < 3) {
            std::cout << "    FAIL: Invalid default parameters" << std::endl;
            return false;
        }
        
        // Expected defaults: Size=0.5, Width=0.5, Mix=0.5
        if (std::abs(defaults[0] - 0.5f) > TOLERANCE || 
            std::abs(defaults[1] - 0.5f) > TOLERANCE ||
            std::abs(defaults[2] - 0.5f) > TOLERANCE) {
            std::cout << "    FAIL: Default parameter values incorrect" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Initialization successful" << std::endl;
        return true;
    }

    // Test 2: Parameter Names and Validation
    bool testParameterNames() {
        std::cout << "  Testing parameter names..." << std::endl;
        
        std::vector<std::string> expectedNames = {
            "Size", "Width", "Mix"
        };
        
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            String paramName = engine->getParameterName(i);
            if (i < expectedNames.size()) {
                if (!paramName.containsIgnoreCase(expectedNames[i])) {
                    std::cout << "    FAIL: Parameter " << i << " name mismatch. Expected: " 
                              << expectedNames[i] << ", Got: " << paramName.toStdString() << std::endl;
                    return false;
                }
            }
        }
        
        std::cout << "    PASS: Parameter names validated" << std::endl;
        return true;
    }

    // Test 3: Size Parameter Control
    bool testSizeControl() {
        std::cout << "  Testing size control..." << std::endl;
        
        // Generate test signal
        generateStereoTestSignal(500.0f, 0.2f);
        
        ExpansionMetrics smallMetrics, largeMetrics;
        
        // Test small size (0.2)
        std::map<int, float> smallParams = {{0, 0.2f}, {1, 0.5f}, {2, 1.0f}};
        engine->updateParameters(smallParams);
        engine->process(testBuffer);
        smallMetrics = analyzeExpansionMetrics();
        
        // Reset and test large size (0.8)
        generateStereoTestSignal(500.0f, 0.2f);
        std::map<int, float> largeParams = {{0, 0.8f}, {1, 0.5f}, {2, 1.0f}};
        engine->updateParameters(largeParams);
        engine->process(testBuffer);
        largeMetrics = analyzeExpansionMetrics();
        
        // Large size should increase spatial size
        if (largeMetrics.spatialSize <= smallMetrics.spatialSize) {
            std::cout << "    FAIL: Large size didn't increase spatial size. Small: " 
                      << smallMetrics.spatialSize << ", Large: " << largeMetrics.spatialSize << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Size control working (Small: " << smallMetrics.spatialSize 
                  << ", Large: " << largeMetrics.spatialSize << ")" << std::endl;
        return true;
    }

    // Test 4: Width Parameter Control
    bool testWidthControl() {
        std::cout << "  Testing width control..." << std::endl;
        
        // Generate test signal
        generateStereoTestSignal(800.0f, M_PI/6);
        
        ExpansionMetrics narrowMetrics, wideMetrics;
        
        // Test narrow width (0.2)
        std::map<int, float> narrowParams = {{0, 0.5f}, {1, 0.2f}, {2, 1.0f}};
        engine->updateParameters(narrowParams);
        engine->process(testBuffer);
        narrowMetrics = analyzeExpansionMetrics();
        
        // Reset and test wide width (0.8)
        generateStereoTestSignal(800.0f, M_PI/6);
        std::map<int, float> wideParams = {{0, 0.5f}, {1, 0.8f}, {2, 1.0f}};
        engine->updateParameters(wideParams);
        engine->process(testBuffer);
        wideMetrics = analyzeExpansionMetrics();
        
        // Wide width should increase dimensional width
        if (wideMetrics.dimensionalWidth <= narrowMetrics.dimensionalWidth) {
            std::cout << "    FAIL: Wide width didn't increase dimensional width. Narrow: " 
                      << narrowMetrics.dimensionalWidth << ", Wide: " << wideMetrics.dimensionalWidth << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Width control working (Narrow: " << narrowMetrics.dimensionalWidth 
                  << ", Wide: " << wideMetrics.dimensionalWidth << ")" << std::endl;
        return true;
    }

    // Test 5: Dimensional Expansion Effect
    bool testDimensionalExpansion() {
        std::cout << "  Testing dimensional expansion effect..." << std::endl;
        
        // Generate mono signal to test expansion
        generateMonoTestSignal(600.0f, 0.5f);
        AudioBuffer<float> originalBuffer;
        originalBuffer.setSize(2, BUFFER_SIZE);
        originalBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        originalBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        
        // Apply maximum expansion
        std::map<int, float> expansionParams = {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}};
        engine->updateParameters(expansionParams);
        engine->process(testBuffer);
        
        ExpansionMetrics metrics = analyzeExpansionMetrics();
        
        // Should create significant expansion from mono source
        if (metrics.expansionAmount < EXPANSION_THRESHOLD) {
            std::cout << "    FAIL: Insufficient dimensional expansion: " << metrics.expansionAmount << std::endl;
            return false;
        }
        
        // Should maintain reasonable coherence
        if (metrics.spatialCoherence < 0.3f) {
            std::cout << "    FAIL: Poor spatial coherence: " << metrics.spatialCoherence << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Dimensional expansion working (Expansion: " << metrics.expansionAmount 
                  << ", Coherence: " << metrics.spatialCoherence << ")" << std::endl;
        return true;
    }

    // Test 6: Mix Parameter Functionality
    bool testMixParameter() {
        std::cout << "  Testing mix parameter..." << std::endl;
        
        generateStereoTestSignal(1000.0f, 0.3f);
        AudioBuffer<float> originalBuffer;
        originalBuffer.setSize(2, BUFFER_SIZE);
        originalBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        originalBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        
        // Test with 0% mix (dry signal)
        std::map<int, float> dryParams = {{0, 0.8f}, {1, 0.8f}, {2, 0.0f}};
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
        std::map<int, float> wetParams = {{0, 0.8f}, {1, 0.8f}, {2, 1.0f}};
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

    // Test 7: Spatial Coherence Preservation
    bool testSpatialCoherence() {
        std::cout << "  Testing spatial coherence preservation..." << std::endl;
        
        // Generate correlated stereo signal
        generateStereoTestSignal(440.0f, 0.15f);
        
        std::map<int, float> params = {{0, 0.6f}, {1, 0.6f}, {2, 0.8f}};
        engine->updateParameters(params);
        engine->process(testBuffer);
        
        ExpansionMetrics metrics = analyzeExpansionMetrics();
        
        // Should maintain reasonable spatial coherence
        if (metrics.spatialCoherence < 0.2f) {
            std::cout << "    FAIL: Poor spatial coherence: " << metrics.spatialCoherence << std::endl;
            return false;
        }
        
        // Should show some expansion
        if (metrics.expansionAmount < 0.05f) {
            std::cout << "    FAIL: Insufficient expansion: " << metrics.expansionAmount << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Spatial coherence preserved (" << metrics.spatialCoherence 
                  << ", Expansion: " << metrics.expansionAmount << ")" << std::endl;
        return true;
    }

    // Test 8: Real-time Performance
    bool testRealTimePerformance() {
        std::cout << "  Testing real-time performance..." << std::endl;
        
        const int numIterations = 1000;
        generateStereoTestSignal(440.0f, 0.2f);
        
        std::map<int, float> params = {{0, 0.7f}, {1, 0.7f}, {2, 0.8f}};
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
        
        if (cpuUsage > 50.0) {
            std::cout << "    FAIL: High CPU usage: " << cpuUsage << "%" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Real-time performance good (CPU usage: " << cpuUsage << "%)" << std::endl;
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
        
        // Test with extreme parameters
        generateStereoTestSignal(50.0f, M_PI);
        std::map<int, float> extremeParams = {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}};
        engine->updateParameters(extremeParams);
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values with extreme parameters" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Edge cases handled properly" << std::endl;
        return true;
    }

    // Run all tests
    bool runAllTests() {
        std::cout << "Running DimensionExpander comprehensive test suite..." << std::endl;
        
        std::vector<std::function<bool()>> tests = {
            [this]() { return testInitialization(); },
            [this]() { return testParameterNames(); },
            [this]() { return testSizeControl(); },
            [this]() { return testWidthControl(); },
            [this]() { return testDimensionalExpansion(); },
            [this]() { return testMixParameter(); },
            [this]() { return testSpatialCoherence(); },
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
        
        std::cout << "DimensionExpander Test Results: " << passed << "/" << tests.size() 
                  << " tests passed (" << (passed * 100 / tests.size()) << "%)" << std::endl;
        
        return passed == tests.size();
    }

private:
    void generateStereoTestSignal(float frequency, float phaseOffset, float amplitude = 0.5f) {
        testBuffer.clear();
        const float omega = 2.0f * M_PI * frequency / SAMPLE_RATE;
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float leftSample = amplitude * std::sin(omega * i);
            float rightSample = amplitude * std::sin(omega * i + phaseOffset);
            testBuffer.setSample(0, i, leftSample);
            testBuffer.setSample(1, i, rightSample);
        }
    }
    
    void generateMonoTestSignal(float frequency, float amplitude = 0.5f) {
        testBuffer.clear();
        const float omega = 2.0f * M_PI * frequency / SAMPLE_RATE;
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = amplitude * std::sin(omega * i);
            testBuffer.setSample(0, i, sample);
            testBuffer.setSample(1, i, sample);
        }
    }
    
    ExpansionMetrics analyzeExpansionMetrics() {
        ExpansionMetrics metrics;
        
        float leftEnergy = 0.0f, rightEnergy = 0.0f;
        float midSum = 0.0f, sideSum = 0.0f;
        float crossCorrelation = 0.0f;
        float totalEnergy = 0.0f;
        
        // Analyze different frequency bands for spatial characteristics
        std::vector<float> bandEnergies(4, 0.0f);  // 4 frequency bands
        const int bandSize = BUFFER_SIZE / 4;
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float left = testBuffer.getSample(0, i);
            float right = testBuffer.getSample(1, i);
            
            float mid = (left + right) * 0.5f;
            float side = (left - right) * 0.5f;
            
            leftEnergy += left * left;
            rightEnergy += right * right;
            midSum += mid * mid;
            sideSum += side * side;
            crossCorrelation += left * right;
            totalEnergy += (left * left + right * right);
            
            // Analyze frequency bands
            int band = i / bandSize;
            if (band < 4) {
                bandEnergies[band] += side * side;
            }
        }
        
        // Calculate spatial size (based on side energy distribution)
        float maxBandEnergy = *std::max_element(bandEnergies.begin(), bandEnergies.end());
        float minBandEnergy = *std::min_element(bandEnergies.begin(), bandEnergies.end());
        if (maxBandEnergy > 0.0f) {
            metrics.spatialSize = 1.0f - (minBandEnergy / maxBandEnergy);
        }
        
        // Calculate dimensional width
        if (midSum + sideSum > 0.0f) {
            metrics.dimensionalWidth = sideSum / (midSum + sideSum);
        }
        
        // Calculate expansion amount (how much the signal has been spatially expanded)
        if (totalEnergy > 0.0f) {
            metrics.expansionAmount = sideSum / totalEnergy;
        }
        
        // Calculate spatial coherence
        float leftRMS = std::sqrt(leftEnergy / BUFFER_SIZE);
        float rightRMS = std::sqrt(rightEnergy / BUFFER_SIZE);
        if (leftRMS > 0.0f && rightRMS > 0.0f) {
            metrics.spatialCoherence = crossCorrelation / (BUFFER_SIZE * leftRMS * rightRMS);
            metrics.spatialCoherence = std::abs(metrics.spatialCoherence); // Use absolute value
        }
        
        return metrics;
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
    std::cout << "=== Chimera Phoenix DimensionExpander Test Suite ===" << std::endl;
    std::cout << "Engine ID: " << ENGINE_DIMENSION_EXPANDER << " (46)" << std::endl;
    std::cout << "Testing dimensional expansion accuracy, spatial size control, and width manipulation" << std::endl;
    std::cout << std::endl;
    
    DimensionExpanderTest tester;
    bool allTestsPassed = tester.runAllTests();
    
    std::cout << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Overall Result: " << (allTestsPassed ? "PASS" : "FAIL") << std::endl;
    
    return allTestsPassed ? 0 : 1;
}
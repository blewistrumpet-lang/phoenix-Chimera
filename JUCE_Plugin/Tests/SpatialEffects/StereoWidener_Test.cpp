/*
    StereoWidener_Test.cpp
    Comprehensive test suite for Stereo Widener spatial effect engine
    Tests stereo field manipulation accuracy, phase correlation, and parameter responsiveness
*/

#include "AppConfig.h"
#include "EngineBaseTest.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../Source/EngineTypes.h"
#include "../Source/StereoWidener.h"
#include "../Source/UnifiedDefaultParameters.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <array>
#include <cassert>

using namespace juce;

class StereoWidenerTest {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr float TOLERANCE = 0.001f;
    static constexpr float PHASE_TOLERANCE = 0.1f;
    
    std::unique_ptr<StereoWidener> engine;
    AudioBuffer<float> testBuffer;
    
    struct TestMetrics {
        float stereoWidth = 0.0f;
        float phaseCorrelation = 0.0f;
        float midEnergy = 0.0f;
        float sideEnergy = 0.0f;
        float bassMonoRatio = 0.0f;
        bool passesTest = true;
        std::string failureReason;
    };

public:
    StereoWidenerTest() {
        engine = std::make_unique<StereoWidener>();
        testBuffer.setSize(2, BUFFER_SIZE);
        engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    ~StereoWidenerTest() = default;

    // Test 1: Engine Initialization and Default Parameters
    bool testInitialization() {
        std::cout << "  Testing initialization..." << std::endl;
        
        // Check engine name
        if (engine->getName() != "Stereo Widener") {
            std::cout << "    FAIL: Engine name incorrect" << std::endl;
            return false;
        }
        
        // Check parameter count
        if (engine->getNumParameters() != 8) {
            std::cout << "    FAIL: Expected 8 parameters, got " << engine->getNumParameters() << std::endl;
            return false;
        }
        
        // Test default parameter values from UnifiedDefaultParameters
        UnifiedDefaultParameters defaultParams;
        auto defaults = defaultParams.getEngineDefaults(ENGINE_STEREO_WIDENER);
        
        if (defaults.empty() || defaults.size() < 3) {
            std::cout << "    FAIL: Invalid default parameters" << std::endl;
            return false;
        }
        
        // Expected defaults: Width=0.5, Bass Mono=0.5, Mix=1.0
        if (std::abs(defaults[0] - 0.5f) > TOLERANCE || 
            std::abs(defaults[1] - 0.5f) > TOLERANCE ||
            std::abs(defaults[2] - 1.0f) > TOLERANCE) {
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
            "Width", "Bass Mono", "High Shelf Freq", "High Shelf Gain",
            "Delay Time", "Delay Gain", "Correlation", "Mix"
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

    // Test 3: Stereo Width Manipulation
    bool testStereoWidthControl() {
        std::cout << "  Testing stereo width control..." << std::endl;
        
        // Generate stereo test signal (correlated sine waves with slight phase difference)
        generateStereoTestSignal(440.0f, 0.1f); // 440Hz with 0.1 radian phase difference
        
        TestMetrics narrowMetrics, wideMetrics;
        
        // Test narrow width (0.2)
        std::map<int, float> narrowParams = {{0, 0.2f}, {7, 1.0f}}; // Width=0.2, Mix=1.0
        engine->updateParameters(narrowParams);
        engine->process(testBuffer);
        narrowMetrics = analyzeStereoDynamics();
        
        // Reset and test wide width (0.8)
        generateStereoTestSignal(440.0f, 0.1f);
        std::map<int, float> wideParams = {{0, 0.8f}, {7, 1.0f}}; // Width=0.8, Mix=1.0
        engine->updateParameters(wideParams);
        engine->process(testBuffer);
        wideMetrics = analyzeStereoDynamics();
        
        // Wide setting should increase stereo width
        if (wideMetrics.stereoWidth <= narrowMetrics.stereoWidth) {
            std::cout << "    FAIL: Wide setting didn't increase stereo width. Narrow: " 
                      << narrowMetrics.stereoWidth << ", Wide: " << wideMetrics.stereoWidth << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Stereo width control working (Narrow: " << narrowMetrics.stereoWidth 
                  << ", Wide: " << wideMetrics.stereoWidth << ")" << std::endl;
        return true;
    }

    // Test 4: Phase Correlation Analysis
    bool testPhaseCorrelation() {
        std::cout << "  Testing phase correlation..." << std::endl;
        
        // Generate mono signal (perfect correlation)
        generateMonoTestSignal(1000.0f);
        
        // Test with normal width
        std::map<int, float> normalParams = {{0, 0.5f}, {7, 1.0f}};
        engine->updateParameters(normalParams);
        engine->process(testBuffer);
        TestMetrics normalMetrics = analyzeStereoDynamics();
        
        // Reset and test with maximum width
        generateMonoTestSignal(1000.0f);
        std::map<int, float> maxParams = {{0, 1.0f}, {7, 1.0f}};
        engine->updateParameters(maxParams);
        engine->process(testBuffer);
        TestMetrics maxMetrics = analyzeStereoDynamics();
        
        // Maximum width should reduce phase correlation
        if (maxMetrics.phaseCorrelation >= normalMetrics.phaseCorrelation) {
            std::cout << "    FAIL: Maximum width didn't reduce phase correlation. Normal: " 
                      << normalMetrics.phaseCorrelation << ", Max: " << maxMetrics.phaseCorrelation << std::endl;
            return false;
        }
        
        // Phase correlation should remain reasonable (not completely decorrelated)
        if (maxMetrics.phaseCorrelation < -0.5f) {
            std::cout << "    FAIL: Phase correlation too negative: " << maxMetrics.phaseCorrelation << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Phase correlation control working (Normal: " << normalMetrics.phaseCorrelation 
                  << ", Max: " << maxMetrics.phaseCorrelation << ")" << std::endl;
        return true;
    }

    // Test 5: Bass Mono Functionality
    bool testBassMonoControl() {
        std::cout << "  Testing bass mono functionality..." << std::endl;
        
        // Generate low-frequency stereo content
        generateStereoTestSignal(80.0f, M_PI/4); // 80Hz with significant phase difference
        
        TestMetrics withoutBassMonoMetrics, withBassMonoMetrics;
        
        // Test without bass mono (bass mono = 0)
        std::map<int, float> noBassMonoParams = {{0, 0.8f}, {1, 0.0f}, {7, 1.0f}};
        engine->updateParameters(noBassMonoParams);
        engine->process(testBuffer);
        withoutBassMonoMetrics = analyzeStereoDynamics();
        
        // Reset and test with bass mono (bass mono = 1.0)
        generateStereoTestSignal(80.0f, M_PI/4);
        std::map<int, float> bassMonoParams = {{0, 0.8f}, {1, 1.0f}, {7, 1.0f}};
        engine->updateParameters(bassMonoParams);
        engine->process(testBuffer);
        withBassMonoMetrics = analyzeStereoDynamics();
        
        // Bass mono should increase bass mono ratio
        if (withBassMonoMetrics.bassMonoRatio <= withoutBassMonoMetrics.bassMonoRatio) {
            std::cout << "    FAIL: Bass mono control not working. Without: " 
                      << withoutBassMonoMetrics.bassMonoRatio << ", With: " 
                      << withBassMonoMetrics.bassMonoRatio << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Bass mono control working (Without: " 
                  << withoutBassMonoMetrics.bassMonoRatio << ", With: " 
                  << withBassMonoMetrics.bassMonoRatio << ")" << std::endl;
        return true;
    }

    // Test 6: Mix Parameter Functionality
    bool testMixParameter() {
        std::cout << "  Testing mix parameter..." << std::endl;
        
        generateStereoTestSignal(1000.0f, 0.2f);
        AudioBuffer<float> originalBuffer;
        originalBuffer.setSize(2, BUFFER_SIZE);
        originalBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        originalBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        
        // Test with 0% mix (dry signal)
        std::map<int, float> dryParams = {{0, 0.8f}, {7, 0.0f}};
        engine->updateParameters(dryParams);
        engine->process(testBuffer);
        
        // Compare with original (should be nearly identical)
        float dryDifference = calculateRMSDifference(testBuffer, originalBuffer);
        if (dryDifference > TOLERANCE) {
            std::cout << "    FAIL: 0% mix not preserving dry signal. Difference: " << dryDifference << std::endl;
            return false;
        }
        
        // Reset and test with 100% mix
        testBuffer.copyFrom(0, 0, originalBuffer, 0, 0, BUFFER_SIZE);
        testBuffer.copyFrom(1, 0, originalBuffer, 1, 0, BUFFER_SIZE);
        std::map<int, float> wetParams = {{0, 0.8f}, {7, 1.0f}};
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

    // Test 7: Real-time Performance
    bool testRealTimePerformance() {
        std::cout << "  Testing real-time performance..." << std::endl;
        
        const int numIterations = 1000;
        generateStereoTestSignal(440.0f, 0.1f);
        
        std::map<int, float> params = {{0, 0.7f}, {7, 1.0f}};
        engine->updateParameters(params);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < numIterations; ++i) {
            engine->process(testBuffer);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        // Calculate if it can run in real-time (buffer duration * iterations)
        double bufferDurationUs = (double)BUFFER_SIZE / SAMPLE_RATE * 1000000.0 * numIterations;
        double processingTimeUs = duration.count();
        double cpuUsage = (processingTimeUs / bufferDurationUs) * 100.0;
        
        if (cpuUsage > 50.0) { // Should use less than 50% CPU for this simple test
            std::cout << "    FAIL: High CPU usage: " << cpuUsage << "%" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Real-time performance good (CPU usage: " << cpuUsage << "%)" << std::endl;
        return true;
    }

    // Test 8: Edge Cases and Stability
    bool testEdgeCases() {
        std::cout << "  Testing edge cases and stability..." << std::endl;
        
        // Test with silence
        testBuffer.clear();
        std::map<int, float> params = {{0, 1.0f}, {7, 1.0f}};
        engine->updateParameters(params);
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values with silence" << std::endl;
            return false;
        }
        
        // Test with very loud signal
        generateStereoTestSignal(1000.0f, 0.1f, 10.0f); // Very loud
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values with loud signal" << std::endl;
            return false;
        }
        
        // Test with DC offset
        for (int ch = 0; ch < testBuffer.getNumChannels(); ++ch) {
            for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
                testBuffer.setSample(ch, i, 0.5f); // DC offset
            }
        }
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values with DC offset" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Edge cases handled properly" << std::endl;
        return true;
    }

    // Run all tests
    bool runAllTests() {
        std::cout << "Running StereoWidener comprehensive test suite..." << std::endl;
        
        std::vector<std::function<bool()>> tests = {
            [this]() { return testInitialization(); },
            [this]() { return testParameterNames(); },
            [this]() { return testStereoWidthControl(); },
            [this]() { return testPhaseCorrelation(); },
            [this]() { return testBassMonoControl(); },
            [this]() { return testMixParameter(); },
            [this]() { return testRealTimePerformance(); },
            [this]() { return testEdgeCases(); }
        };
        
        int passed = 0;
        for (const auto& test : tests) {
            if (test()) {
                passed++;
            }
            engine->reset(); // Reset state between tests
        }
        
        std::cout << "StereoWidener Test Results: " << passed << "/" << tests.size() 
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
    
    TestMetrics analyzeStereoDynamics() {
        TestMetrics metrics;
        
        // Calculate Mid/Side energies
        float midSum = 0.0f, sideSum = 0.0f;
        float crossCorrelation = 0.0f;
        float leftEnergy = 0.0f, rightEnergy = 0.0f;
        float bassEnergyMono = 0.0f, bassEnergyStereo = 0.0f;
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float left = testBuffer.getSample(0, i);
            float right = testBuffer.getSample(1, i);
            
            float mid = (left + right) * 0.5f;
            float side = (left - right) * 0.5f;
            
            midSum += mid * mid;
            sideSum += side * side;
            crossCorrelation += left * right;
            leftEnergy += left * left;
            rightEnergy += right * right;
            
            // Simple bass analysis (assume first 1/4 of buffer represents bass content)
            if (i < BUFFER_SIZE / 4) {
                bassEnergyMono += mid * mid;
                bassEnergyStereo += side * side;
            }
        }
        
        metrics.midEnergy = midSum / BUFFER_SIZE;
        metrics.sideEnergy = sideSum / BUFFER_SIZE;
        metrics.stereoWidth = metrics.sideEnergy > 0.0f ? metrics.sideEnergy / (metrics.midEnergy + metrics.sideEnergy) : 0.0f;
        
        float leftRMS = std::sqrt(leftEnergy / BUFFER_SIZE);
        float rightRMS = std::sqrt(rightEnergy / BUFFER_SIZE);
        if (leftRMS > 0.0f && rightRMS > 0.0f) {
            metrics.phaseCorrelation = crossCorrelation / (BUFFER_SIZE * leftRMS * rightRMS);
        }
        
        if (bassEnergyMono + bassEnergyStereo > 0.0f) {
            metrics.bassMonoRatio = bassEnergyMono / (bassEnergyMono + bassEnergyStereo);
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
    std::cout << "=== Chimera Phoenix StereoWidener Test Suite ===" << std::endl;
    std::cout << "Engine ID: " << ENGINE_STEREO_WIDENER << " (44)" << std::endl;
    std::cout << "Testing stereo field manipulation accuracy, phase correlation, and parameter responsiveness" << std::endl;
    std::cout << std::endl;
    
    StereoWidenerTest tester;
    bool allTestsPassed = tester.runAllTests();
    
    std::cout << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Overall Result: " << (allTestsPassed ? "PASS" : "FAIL") << std::endl;
    
    return allTestsPassed ? 0 : 1;
}
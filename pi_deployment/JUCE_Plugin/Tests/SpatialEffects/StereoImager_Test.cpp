/*
    StereoImager_Test.cpp
    Comprehensive test suite for Stereo Imager spatial effect engine
    Tests stereo imaging accuracy, center positioning, and rotation capabilities
*/

#include "AppConfig.h"
#include "EngineBaseTest.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../Source/EngineTypes.h"
#include "../Source/StereoImager.h"
#include "../Source/UnifiedDefaultParameters.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <array>
#include <cassert>

using namespace juce;

class StereoImagerTest {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr float TOLERANCE = 0.001f;
    static constexpr float PHASE_TOLERANCE = 0.1f;
    
    std::unique_ptr<StereoImager> engine;
    AudioBuffer<float> testBuffer;
    
    struct ImagingMetrics {
        float stereoWidth = 0.0f;
        float centerPosition = 0.0f;
        float rotationAmount = 0.0f;
        float leftRightBalance = 0.0f;
        float phaseCoherence = 0.0f;
        bool passesTest = true;
        std::string failureReason;
    };

public:
    StereoImagerTest() {
        engine = std::make_unique<StereoImager>();
        testBuffer.setSize(2, BUFFER_SIZE);
        engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    ~StereoImagerTest() = default;

    // Test 1: Engine Initialization and Default Parameters
    bool testInitialization() {
        std::cout << "  Testing initialization..." << std::endl;
        
        // Check engine name
        if (engine->getName() != "Stereo Imager") {
            std::cout << "    FAIL: Engine name incorrect" << std::endl;
            return false;
        }
        
        // Check parameter count (Width, Center, Rotation, Mix)
        if (engine->getNumParameters() != 4) {
            std::cout << "    FAIL: Expected 4 parameters, got " << engine->getNumParameters() << std::endl;
            return false;
        }
        
        // Test default parameter values from UnifiedDefaultParameters
        UnifiedDefaultParameters defaultParams;
        auto defaults = defaultParams.getEngineDefaults(ENGINE_STEREO_IMAGER);
        
        if (defaults.empty() || defaults.size() < 4) {
            std::cout << "    FAIL: Invalid default parameters" << std::endl;
            return false;
        }
        
        // Expected defaults: Width=0.5, Center=0.5, Rotation=0.5, Mix=1.0
        if (std::abs(defaults[0] - 0.5f) > TOLERANCE || 
            std::abs(defaults[1] - 0.5f) > TOLERANCE ||
            std::abs(defaults[2] - 0.5f) > TOLERANCE ||
            std::abs(defaults[3] - 1.0f) > TOLERANCE) {
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
            "Width", "Center", "Rotation", "Mix"
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

    // Test 3: Stereo Width Control
    bool testWidthControl() {
        std::cout << "  Testing width control..." << std::endl;
        
        // Generate wide stereo test signal
        generateStereoTestSignal(1000.0f, M_PI/2, 0.5f); // 90-degree phase difference
        
        ImagingMetrics narrowMetrics, wideMetrics;
        
        // Test narrow width (0.2)
        std::map<int, float> narrowParams = {{0, 0.2f}, {1, 0.5f}, {2, 0.5f}, {3, 1.0f}};
        engine->updateParameters(narrowParams);
        engine->process(testBuffer);
        narrowMetrics = analyzeImagingMetrics();
        
        // Reset and test wide width (0.8)
        generateStereoTestSignal(1000.0f, M_PI/2, 0.5f);
        std::map<int, float> wideParams = {{0, 0.8f}, {1, 0.5f}, {2, 0.5f}, {3, 1.0f}};
        engine->updateParameters(wideParams);
        engine->process(testBuffer);
        wideMetrics = analyzeImagingMetrics();
        
        // Wide setting should increase stereo width
        if (wideMetrics.stereoWidth <= narrowMetrics.stereoWidth) {
            std::cout << "    FAIL: Wide setting didn't increase stereo width. Narrow: " 
                      << narrowMetrics.stereoWidth << ", Wide: " << wideMetrics.stereoWidth << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Width control working (Narrow: " << narrowMetrics.stereoWidth 
                  << ", Wide: " << wideMetrics.stereoWidth << ")" << std::endl;
        return true;
    }

    // Test 4: Center Position Control
    bool testCenterControl() {
        std::cout << "  Testing center position control..." << std::endl;
        
        // Generate centered mono signal
        generateMonoTestSignal(800.0f, 0.5f);
        
        ImagingMetrics leftMetrics, rightMetrics, centerMetrics;
        
        // Test left center (0.2)
        std::map<int, float> leftParams = {{0, 0.5f}, {1, 0.2f}, {2, 0.5f}, {3, 1.0f}};
        engine->updateParameters(leftParams);
        engine->process(testBuffer);
        leftMetrics = analyzeImagingMetrics();
        
        // Reset and test right center (0.8)
        generateMonoTestSignal(800.0f, 0.5f);
        std::map<int, float> rightParams = {{0, 0.5f}, {1, 0.8f}, {2, 0.5f}, {3, 1.0f}};
        engine->updateParameters(rightParams);
        engine->process(testBuffer);
        rightMetrics = analyzeImagingMetrics();
        
        // Reset and test center (0.5)
        generateMonoTestSignal(800.0f, 0.5f);
        std::map<int, float> centerParams = {{0, 0.5f}, {1, 0.5f}, {2, 0.5f}, {3, 1.0f}};
        engine->updateParameters(centerParams);
        engine->process(testBuffer);
        centerMetrics = analyzeImagingMetrics();
        
        // Left setting should shift balance left, right should shift right
        if (leftMetrics.leftRightBalance >= centerMetrics.leftRightBalance) {
            std::cout << "    FAIL: Left center didn't shift balance left. Left: " 
                      << leftMetrics.leftRightBalance << ", Center: " << centerMetrics.leftRightBalance << std::endl;
            return false;
        }
        
        if (rightMetrics.leftRightBalance <= centerMetrics.leftRightBalance) {
            std::cout << "    FAIL: Right center didn't shift balance right. Right: " 
                      << rightMetrics.leftRightBalance << ", Center: " << centerMetrics.leftRightBalance << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Center control working (Left: " << leftMetrics.leftRightBalance 
                  << ", Center: " << centerMetrics.leftRightBalance 
                  << ", Right: " << rightMetrics.leftRightBalance << ")" << std::endl;
        return true;
    }

    // Test 5: Rotation Control
    bool testRotationControl() {
        std::cout << "  Testing rotation control..." << std::endl;
        
        // Generate stereo test signal with distinct L/R content
        generateAsymmetricStereoSignal(600.0f, 900.0f);
        
        ImagingMetrics noRotationMetrics, rotatedMetrics;
        
        // Test no rotation (0.5 = center position)
        std::map<int, float> noRotParams = {{0, 0.5f}, {1, 0.5f}, {2, 0.5f}, {3, 1.0f}};
        engine->updateParameters(noRotParams);
        AudioBuffer<float> noRotBuffer;
        noRotBuffer.setSize(2, BUFFER_SIZE);
        noRotBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        noRotBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        engine->process(noRotBuffer);
        
        // Reset and test with rotation (0.8 = significant rotation)
        std::map<int, float> rotParams = {{0, 0.5f}, {1, 0.5f}, {2, 0.8f}, {3, 1.0f}};
        engine->updateParameters(rotParams);
        AudioBuffer<float> rotBuffer;
        rotBuffer.setSize(2, BUFFER_SIZE);
        rotBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        rotBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        engine->process(rotBuffer);
        
        // Calculate cross-correlation to detect rotation
        float noRotCrossCorr = calculateCrossCorrelation(noRotBuffer);
        float rotCrossCorr = calculateCrossCorrelation(rotBuffer);
        
        // Rotation should change the cross-correlation pattern
        if (std::abs(rotCrossCorr - noRotCrossCorr) < 0.1f) {
            std::cout << "    FAIL: Rotation didn't significantly change signal. NoRot: " 
                      << noRotCrossCorr << ", Rot: " << rotCrossCorr << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Rotation control working (Change in correlation: " 
                  << std::abs(rotCrossCorr - noRotCrossCorr) << ")" << std::endl;
        return true;
    }

    // Test 6: Mix Parameter Functionality
    bool testMixParameter() {
        std::cout << "  Testing mix parameter..." << std::endl;
        
        generateStereoTestSignal(1200.0f, 0.3f);
        AudioBuffer<float> originalBuffer;
        originalBuffer.setSize(2, BUFFER_SIZE);
        originalBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        originalBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        
        // Test with 0% mix (dry signal)
        std::map<int, float> dryParams = {{0, 0.7f}, {1, 0.3f}, {2, 0.7f}, {3, 0.0f}};
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
        std::map<int, float> wetParams = {{0, 0.7f}, {1, 0.3f}, {2, 0.7f}, {3, 1.0f}};
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

    // Test 7: Phase Coherence
    bool testPhaseCoherence() {
        std::cout << "  Testing phase coherence preservation..." << std::endl;
        
        // Generate correlated stereo signal
        generateStereoTestSignal(440.0f, 0.1f); // Small phase difference
        
        std::map<int, float> params = {{0, 0.6f}, {1, 0.5f}, {2, 0.5f}, {3, 1.0f}};
        engine->updateParameters(params);
        engine->process(testBuffer);
        
        ImagingMetrics metrics = analyzeImagingMetrics();
        
        // Phase coherence should be maintained reasonably well
        if (metrics.phaseCoherence < 0.3f) {
            std::cout << "    FAIL: Poor phase coherence: " << metrics.phaseCoherence << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Phase coherence preserved (" << metrics.phaseCoherence << ")" << std::endl;
        return true;
    }

    // Test 8: Real-time Performance
    bool testRealTimePerformance() {
        std::cout << "  Testing real-time performance..." << std::endl;
        
        const int numIterations = 1000;
        generateStereoTestSignal(440.0f, 0.2f);
        
        std::map<int, float> params = {{0, 0.7f}, {1, 0.6f}, {2, 0.4f}, {3, 1.0f}};
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
        std::map<int, float> params = {{0, 1.0f}, {1, 0.0f}, {2, 1.0f}, {3, 1.0f}};
        engine->updateParameters(params);
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values with silence" << std::endl;
            return false;
        }
        
        // Test with extreme parameters
        std::map<int, float> extremeParams = {{0, 1.0f}, {1, 0.0f}, {2, 1.0f}, {3, 1.0f}};
        generateStereoTestSignal(1000.0f, M_PI);
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
        std::cout << "Running StereoImager comprehensive test suite..." << std::endl;
        
        std::vector<std::function<bool()>> tests = {
            [this]() { return testInitialization(); },
            [this]() { return testParameterNames(); },
            [this]() { return testWidthControl(); },
            [this]() { return testCenterControl(); },
            [this]() { return testRotationControl(); },
            [this]() { return testMixParameter(); },
            [this]() { return testPhaseCoherence(); },
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
        
        std::cout << "StereoImager Test Results: " << passed << "/" << tests.size() 
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
    
    void generateAsymmetricStereoSignal(float leftFreq, float rightFreq, float amplitude = 0.5f) {
        testBuffer.clear();
        const float omegaL = 2.0f * M_PI * leftFreq / SAMPLE_RATE;
        const float omegaR = 2.0f * M_PI * rightFreq / SAMPLE_RATE;
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float leftSample = amplitude * std::sin(omegaL * i);
            float rightSample = amplitude * std::sin(omegaR * i);
            testBuffer.setSample(0, i, leftSample);
            testBuffer.setSample(1, i, rightSample);
        }
    }
    
    ImagingMetrics analyzeImagingMetrics() {
        ImagingMetrics metrics;
        
        float leftEnergy = 0.0f, rightEnergy = 0.0f;
        float midSum = 0.0f, sideSum = 0.0f;
        float crossCorrelation = 0.0f;
        
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
        }
        
        metrics.stereoWidth = sideSum > 0.0f ? sideSum / (midSum + sideSum) : 0.0f;
        
        float totalEnergy = leftEnergy + rightEnergy;
        if (totalEnergy > 0.0f) {
            metrics.leftRightBalance = (rightEnergy - leftEnergy) / totalEnergy;
        }
        
        float leftRMS = std::sqrt(leftEnergy / BUFFER_SIZE);
        float rightRMS = std::sqrt(rightEnergy / BUFFER_SIZE);
        if (leftRMS > 0.0f && rightRMS > 0.0f) {
            metrics.phaseCoherence = crossCorrelation / (BUFFER_SIZE * leftRMS * rightRMS);
        }
        
        return metrics;
    }
    
    float calculateCrossCorrelation(const AudioBuffer<float>& buffer) {
        float correlation = 0.0f;
        float leftEnergy = 0.0f, rightEnergy = 0.0f;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float left = buffer.getSample(0, i);
            float right = buffer.getSample(1, i);
            correlation += left * right;
            leftEnergy += left * left;
            rightEnergy += right * right;
        }
        
        float leftRMS = std::sqrt(leftEnergy / buffer.getNumSamples());
        float rightRMS = std::sqrt(rightEnergy / buffer.getNumSamples());
        
        if (leftRMS > 0.0f && rightRMS > 0.0f) {
            return correlation / (buffer.getNumSamples() * leftRMS * rightRMS);
        }
        
        return 0.0f;
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
    std::cout << "=== Chimera Phoenix StereoImager Test Suite ===" << std::endl;
    std::cout << "Engine ID: " << ENGINE_STEREO_IMAGER << " (45)" << std::endl;
    std::cout << "Testing stereo imaging accuracy, center positioning, and rotation capabilities" << std::endl;
    std::cout << std::endl;
    
    StereoImagerTest tester;
    bool allTestsPassed = tester.runAllTests();
    
    std::cout << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Overall Result: " << (allTestsPassed ? "PASS" : "FAIL") << std::endl;
    
    return allTestsPassed ? 0 : 1;
}
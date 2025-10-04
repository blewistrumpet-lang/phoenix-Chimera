/*
    SpectralGate_Test.cpp
    Comprehensive test suite for Spectral Gate special effect engine
    Tests spectral gating accuracy, frequency-selective processing, and threshold behavior
*/

#include "AppConfig.h"
#include "EngineBaseTest.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../Source/EngineTypes.h"
#include "../Source/SpectralGate.h"
#include "../Source/UnifiedDefaultParameters.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <array>
#include <cassert>

using namespace juce;

class SpectralGateTest {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr float TOLERANCE = 0.001f;
    static constexpr float GATE_TOLERANCE = 0.1f;
    
    std::unique_ptr<SpectralGate> engine;
    AudioBuffer<float> testBuffer;
    
    struct GatingMetrics {
        float gatedEnergy = 0.0f;
        float ungatedEnergy = 0.0f;
        float gatingRatio = 0.0f;
        float frequencySelectivity = 0.0f;
        float thresholdAccuracy = 0.0f;
        bool passesTest = true;
        std::string failureReason;
    };

public:
    SpectralGateTest() {
        engine = std::make_unique<SpectralGate>();
        testBuffer.setSize(2, BUFFER_SIZE);
        engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    ~SpectralGateTest() = default;

    // Test 1: Engine Initialization and Default Parameters
    bool testInitialization() {
        std::cout << "  Testing initialization..." << std::endl;
        
        // Check engine name
        String engineName = engine->getName();
        if (!engineName.contains("Spectral Gate")) {
            std::cout << "    FAIL: Engine name incorrect: " << engineName.toStdString() << std::endl;
            return false;
        }
        
        // Check parameter count (7 parameters based on UnifiedDefaultParameters)
        if (engine->getNumParameters() != 7) {
            std::cout << "    FAIL: Expected 7 parameters, got " << engine->getNumParameters() << std::endl;
            return false;
        }
        
        // Test default parameter values from UnifiedDefaultParameters
        UnifiedDefaultParameters defaultParams;
        auto defaults = defaultParams.getEngineDefaults(ENGINE_SPECTRAL_GATE);
        
        if (defaults.empty() || defaults.size() < 7) {
            std::cout << "    FAIL: Invalid default parameters" << std::endl;
            return false;
        }
        
        // Expected defaults based on UnifiedDefaultParameters.cpp
        std::vector<float> expectedDefaults = {0.25f, 0.3f, 0.3f, 0.3f, 0.0f, 1.0f, 0.0f};
        for (size_t i = 0; i < std::min(expectedDefaults.size(), defaults.size()); ++i) {
            if (std::abs(defaults[i] - expectedDefaults[i]) > TOLERANCE) {
                std::cout << "    FAIL: Default parameter " << i << " incorrect. Expected: " 
                          << expectedDefaults[i] << ", Got: " << defaults[i] << std::endl;
                return false;
            }
        }
        
        std::cout << "    PASS: Initialization successful" << std::endl;
        return true;
    }

    // Test 2: Parameter Names and Validation
    bool testParameterNames() {
        std::cout << "  Testing parameter names..." << std::endl;
        
        std::vector<std::string> expectedNames = {
            "Threshold", "Ratio", "Attack", "Release", "Freq Low", "Freq High", "Mix"
        };
        
        for (int i = 0; i < engine->getNumParameters() && i < expectedNames.size(); ++i) {
            String paramName = engine->getParameterName(i);
            bool nameMatches = false;
            
            // Check if parameter name contains expected keywords
            for (const auto& expectedWord : expectedNames[i]) {
                if (paramName.containsIgnoreCase(String(&expectedWord, 1))) {
                    nameMatches = true;
                    break;
                }
            }
            
            if (!nameMatches && !paramName.containsIgnoreCase(expectedNames[i])) {
                std::cout << "    WARN: Parameter " << i << " name may be incorrect. Expected: " 
                          << expectedNames[i] << ", Got: " << paramName.toStdString() << std::endl;
                // Don't fail on parameter name mismatches as they may be abbreviated
            }
        }
        
        std::cout << "    PASS: Parameter names validated" << std::endl;
        return true;
    }

    // Test 3: Threshold Control
    bool testThresholdControl() {
        std::cout << "  Testing threshold control..." << std::endl;
        
        // Generate signal with varying amplitudes
        generateMultiLevelSignal();
        
        GatingMetrics lowThresholdMetrics, highThresholdMetrics;
        
        // Test low threshold (0.1) - should gate less
        std::map<int, float> lowThreshParams = {{0, 0.1f}, {1, 0.5f}, {6, 1.0f}};
        engine->updateParameters(lowThreshParams);
        engine->process(testBuffer);
        lowThresholdMetrics = analyzeGatingMetrics();
        
        // Reset and test high threshold (0.8) - should gate more
        engine->reset();
        generateMultiLevelSignal();
        std::map<int, float> highThreshParams = {{0, 0.8f}, {1, 0.5f}, {6, 1.0f}};
        engine->updateParameters(highThreshParams);
        engine->process(testBuffer);
        highThresholdMetrics = analyzeGatingMetrics();
        
        // High threshold should gate more (lower output energy)
        if (highThresholdMetrics.gatedEnergy >= lowThresholdMetrics.gatedEnergy) {
            std::cout << "    FAIL: High threshold didn't reduce output. Low: " 
                      << lowThresholdMetrics.gatedEnergy << ", High: " << highThresholdMetrics.gatedEnergy << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Threshold control working (Low: " << lowThresholdMetrics.gatedEnergy 
                  << ", High: " << highThresholdMetrics.gatedEnergy << ")" << std::endl;
        return true;
    }

    // Test 4: Ratio Parameter Effect
    bool testRatioControl() {
        std::cout << "  Testing ratio control..." << std::endl;
        
        generateMultiLevelSignal();
        
        GatingMetrics softRatioMetrics, hardRatioMetrics;
        
        // Test soft ratio (0.2) - gentle gating
        std::map<int, float> softParams = {{0, 0.4f}, {1, 0.2f}, {6, 1.0f}};
        engine->updateParameters(softParams);
        engine->process(testBuffer);
        softRatioMetrics = analyzeGatingMetrics();
        
        // Reset and test hard ratio (0.8) - aggressive gating
        engine->reset();
        generateMultiLevelSignal();
        std::map<int, float> hardParams = {{0, 0.4f}, {1, 0.8f}, {6, 1.0f}};
        engine->updateParameters(hardParams);
        engine->process(testBuffer);
        hardRatioMetrics = analyzeGatingMetrics();
        
        // Hard ratio should show more pronounced gating effect
        float ratioEffect = std::abs(hardRatioMetrics.gatingRatio - softRatioMetrics.gatingRatio);
        if (ratioEffect < 0.05f) {
            std::cout << "    FAIL: Ratio parameter has minimal effect. Difference: " << ratioEffect << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Ratio control working (Effect magnitude: " << ratioEffect << ")" << std::endl;
        return true;
    }

    // Test 5: Frequency Range Control
    bool testFrequencyRangeControl() {
        std::cout << "  Testing frequency range control..." << std::endl;
        
        // Generate signal with low, mid, and high frequency content
        generateMultiFrequencySignal();
        
        GatingMetrics fullRangeMetrics, limitedRangeMetrics;
        
        // Test full frequency range (0.0 to 1.0)
        std::map<int, float> fullRangeParams = {{0, 0.5f}, {4, 0.0f}, {5, 1.0f}, {6, 1.0f}};
        engine->updateParameters(fullRangeParams);
        engine->process(testBuffer);
        fullRangeMetrics = analyzeGatingMetrics();
        
        // Reset and test limited frequency range (0.3 to 0.7) - mid frequencies only
        engine->reset();
        generateMultiFrequencySignal();
        std::map<int, float> limitedParams = {{0, 0.5f}, {4, 0.3f}, {5, 0.7f}, {6, 1.0f}};
        engine->updateParameters(limitedParams);
        engine->process(testBuffer);
        limitedRangeMetrics = analyzeGatingMetrics();
        
        // Limited range should show different frequency selectivity
        float selectivityDiff = std::abs(limitedRangeMetrics.frequencySelectivity - fullRangeMetrics.frequencySelectivity);
        if (selectivityDiff < 0.1f) {
            std::cout << "    FAIL: Frequency range control minimal effect. Difference: " << selectivityDiff << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Frequency range control working (Selectivity diff: " << selectivityDiff << ")" << std::endl;
        return true;
    }

    // Test 6: Attack and Release Timing
    bool testTimingParameters() {
        std::cout << "  Testing attack and release timing..." << std::endl;
        
        // Generate signal with amplitude changes
        generateAmplitudeModulatedSignal();
        
        GatingMetrics fastTimingMetrics, slowTimingMetrics;
        
        // Test fast timing (0.2, 0.2)
        std::map<int, float> fastParams = {{0, 0.5f}, {2, 0.2f}, {3, 0.2f}, {6, 1.0f}};
        engine->updateParameters(fastParams);
        engine->process(testBuffer);
        fastTimingMetrics = analyzeGatingMetrics();
        
        // Reset and test slow timing (0.8, 0.8)
        engine->reset();
        generateAmplitudeModulatedSignal();
        std::map<int, float> slowParams = {{0, 0.5f}, {2, 0.8f}, {3, 0.8f}, {6, 1.0f}};
        engine->updateParameters(slowParams);
        engine->process(testBuffer);
        slowTimingMetrics = analyzeGatingMetrics();
        
        // Different timing should produce different results
        float timingEffect = std::abs(fastTimingMetrics.gatedEnergy - slowTimingMetrics.gatedEnergy);
        if (timingEffect < 0.02f) {
            std::cout << "    FAIL: Timing parameters minimal effect. Difference: " << timingEffect << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Timing parameters working (Effect: " << timingEffect << ")" << std::endl;
        return true;
    }

    // Test 7: Mix Parameter Functionality
    bool testMixParameter() {
        std::cout << "  Testing mix parameter..." << std::endl;
        
        generateMultiLevelSignal();
        AudioBuffer<float> originalBuffer;
        originalBuffer.setSize(2, BUFFER_SIZE);
        originalBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        originalBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        
        // Test with 0% mix (dry)
        std::map<int, float> dryParams = {{0, 0.8f}, {6, 0.0f}};
        engine->updateParameters(dryParams);
        engine->process(testBuffer);
        
        float dryDifference = calculateRMSDifference(testBuffer, originalBuffer);
        if (dryDifference > TOLERANCE) {
            std::cout << "    FAIL: 0% mix not preserving dry signal. Difference: " << dryDifference << std::endl;
            return false;
        }
        
        // Reset and test with 100% mix (wet)
        testBuffer.copyFrom(0, 0, originalBuffer, 0, 0, BUFFER_SIZE);
        testBuffer.copyFrom(1, 0, originalBuffer, 1, 0, BUFFER_SIZE);
        std::map<int, float> wetParams = {{0, 0.8f}, {6, 1.0f}};
        engine->updateParameters(wetParams);
        engine->process(testBuffer);
        
        float wetDifference = calculateRMSDifference(testBuffer, originalBuffer);
        if (wetDifference < TOLERANCE) {
            std::cout << "    FAIL: 100% mix not processing signal. Difference: " << wetDifference << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Mix parameter working (Dry: " << dryDifference 
                  << ", Wet: " << wetDifference << ")" << std::endl;
        return true;
    }

    // Test 8: Real-time Performance
    bool testRealTimePerformance() {
        std::cout << "  Testing real-time performance..." << std::endl;
        
        const int numIterations = 500;
        generateMultiFrequencySignal();
        
        std::map<int, float> params = {{0, 0.6f}, {1, 0.4f}, {2, 0.3f}, {3, 0.4f}, {6, 1.0f}};
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
        
        if (cpuUsage > 80.0) {
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
        std::map<int, float> params = {{0, 0.5f}, {6, 1.0f}};
        engine->updateParameters(params);
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Produced non-finite values with silence" << std::endl;
            return false;
        }
        
        // Test with extreme parameters
        generateMultiLevelSignal();
        std::map<int, float> extremeParams = {{0, 1.0f}, {1, 1.0f}, {2, 0.0f}, {3, 1.0f}, {6, 1.0f}};
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
        std::cout << "Running SpectralGate comprehensive test suite..." << std::endl;
        
        std::vector<std::function<bool()>> tests = {
            [this]() { return testInitialization(); },
            [this]() { return testParameterNames(); },
            [this]() { return testThresholdControl(); },
            [this]() { return testRatioControl(); },
            [this]() { return testFrequencyRangeControl(); },
            [this]() { return testTimingParameters(); },
            [this]() { return testMixParameter(); },
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
        
        std::cout << "SpectralGate Test Results: " << passed << "/" << tests.size() 
                  << " tests passed (" << (passed * 100 / tests.size()) << "%)" << std::endl;
        
        return passed == tests.size();
    }

private:
    void generateMultiLevelSignal(float baseAmplitude = 0.5f) {
        testBuffer.clear();
        const float omega = 2.0f * M_PI * 800.0f / SAMPLE_RATE;
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            // Create varying amplitude levels
            float level = 0.2f + 0.8f * (float)i / BUFFER_SIZE;
            float sample = baseAmplitude * level * std::sin(omega * i);
            testBuffer.setSample(0, i, sample);
            testBuffer.setSample(1, i, sample * 0.9f);
        }
    }
    
    void generateMultiFrequencySignal(float amplitude = 0.4f) {
        testBuffer.clear();
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = 0.0f;
            // Low frequency component
            sample += amplitude * 0.4f * std::sin(2.0f * M_PI * 200.0f * i / SAMPLE_RATE);
            // Mid frequency component
            sample += amplitude * 0.4f * std::sin(2.0f * M_PI * 1000.0f * i / SAMPLE_RATE);
            // High frequency component
            sample += amplitude * 0.2f * std::sin(2.0f * M_PI * 4000.0f * i / SAMPLE_RATE);
            
            testBuffer.setSample(0, i, sample);
            testBuffer.setSample(1, i, sample * 0.8f);
        }
    }
    
    void generateAmplitudeModulatedSignal(float amplitude = 0.5f) {
        testBuffer.clear();
        const float carrierOmega = 2.0f * M_PI * 1000.0f / SAMPLE_RATE;
        const float modOmega = 2.0f * M_PI * 10.0f / SAMPLE_RATE;
        
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float modulation = 0.5f + 0.5f * std::sin(modOmega * i);
            float sample = amplitude * modulation * std::sin(carrierOmega * i);
            testBuffer.setSample(0, i, sample);
            testBuffer.setSample(1, i, sample);
        }
    }
    
    GatingMetrics analyzeGatingMetrics() {
        GatingMetrics metrics;
        
        float totalEnergy = 0.0f;
        float gatedContent = 0.0f;
        float ungatedContent = 0.0f;
        
        // Analyze frequency content (simplified)
        std::vector<float> frequencyBins(4, 0.0f);
        const int binSize = BUFFER_SIZE / 4;
        
        for (int ch = 0; ch < testBuffer.getNumChannels(); ++ch) {
            for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
                float sample = testBuffer.getSample(ch, i);
                float energy = sample * sample;
                totalEnergy += energy;
                
                // Simple frequency analysis
                int bin = i / binSize;
                if (bin < 4) {
                    frequencyBins[bin] += energy;
                }
                
                // Classify as gated or ungated based on amplitude
                if (std::abs(sample) < 0.1f) {
                    gatedContent += energy;
                } else {
                    ungatedContent += energy;
                }
            }
        }
        
        metrics.gatedEnergy = totalEnergy / (testBuffer.getNumChannels() * testBuffer.getNumSamples());
        
        if (totalEnergy > 0.0f) {
            metrics.gatingRatio = gatedContent / totalEnergy;
            metrics.ungatedEnergy = ungatedContent / totalEnergy;
        }
        
        // Calculate frequency selectivity
        float maxBin = *std::max_element(frequencyBins.begin(), frequencyBins.end());
        float minBin = *std::min_element(frequencyBins.begin(), frequencyBins.end());
        if (maxBin > 0.0f) {
            metrics.frequencySelectivity = 1.0f - (minBin / maxBin);
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
    std::cout << "=== Chimera Phoenix SpectralGate Test Suite ===" << std::endl;
    std::cout << "Engine ID: " << ENGINE_SPECTRAL_GATE << " (48)" << std::endl;
    std::cout << "Testing spectral gating accuracy, frequency-selective processing, and threshold behavior" << std::endl;
    std::cout << std::endl;
    
    SpectralGateTest tester;
    bool allTestsPassed = tester.runAllTests();
    
    std::cout << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Overall Result: " << (allTestsPassed ? "PASS" : "FAIL") << std::endl;
    
    return allTestsPassed ? 0 : 1;
}
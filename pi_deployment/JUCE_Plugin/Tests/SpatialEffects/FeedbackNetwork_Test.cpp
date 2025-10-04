/*
    FeedbackNetwork_Test.cpp
    Comprehensive test suite for Feedback Network special effect engine
    Tests feedback processing, network stability, and modulation effects
*/

#include "AppConfig.h"
#include "EngineBaseTest.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../Source/EngineTypes.h"
#include "../Source/FeedbackNetwork.h"
#include "../Source/UnifiedDefaultParameters.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <array>
#include <cassert>

using namespace juce;

class FeedbackNetworkTest {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr float TOLERANCE = 0.001f;
    
    std::unique_ptr<FeedbackNetwork> engine;
    AudioBuffer<float> testBuffer;

public:
    FeedbackNetworkTest() {
        engine = std::make_unique<FeedbackNetwork>();
        testBuffer.setSize(2, BUFFER_SIZE);
        engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    // Test 1: Engine Initialization
    bool testInitialization() {
        std::cout << "  Testing initialization..." << std::endl;
        
        String engineName = engine->getName();
        if (!engineName.contains("Feedback") && !engineName.contains("Network")) {
            std::cout << "    FAIL: Engine name incorrect: " << engineName.toStdString() << std::endl;
            return false;
        }
        
        if (engine->getNumParameters() < 3) {
            std::cout << "    FAIL: Expected at least 3 parameters" << std::endl;
            return false;
        }
        
        UnifiedDefaultParameters defaultParams;
        auto defaults = defaultParams.getEngineDefaults(ENGINE_FEEDBACK_NETWORK);
        if (defaults.empty()) {
            std::cout << "    FAIL: No default parameters" << std::endl;
            return false;
        }
        
        // Expected defaults: Feedback=0.3, Delay=0.5, Modulation=0.2, Mix=0.2
        if (std::abs(defaults[0] - 0.3f) > TOLERANCE) {
            std::cout << "    FAIL: Default feedback incorrect: " << defaults[0] << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Initialization successful" << std::endl;
        return true;
    }
    
    // Test 2: Feedback Processing
    bool testFeedbackProcessing() {
        std::cout << "  Testing feedback processing..." << std::endl;
        
        generateTestSignal(300.0f);
        
        // Test with minimal feedback
        std::map<int, float> minFeedback = {{0, 0.1f}, {3, 0.2f}};
        engine->updateParameters(minFeedback);
        engine->process(testBuffer);
        float minEnergy = calculateEnergy(testBuffer);
        
        engine->reset();
        generateTestSignal(300.0f);
        
        // Test with more feedback (but safe level)
        std::map<int, float> maxFeedback = {{0, 0.5f}, {3, 0.5f}};
        engine->updateParameters(maxFeedback);
        engine->process(testBuffer);
        float maxEnergy = calculateEnergy(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Non-finite output" << std::endl;
            return false;
        }
        
        // Higher feedback should affect the signal
        if (std::abs(maxEnergy - minEnergy) < 0.01f) {
            std::cout << "    FAIL: Feedback levels show minimal difference" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Feedback processing working" << std::endl;
        return true;
    }
    
    // Test 3: Stability Under Feedback
    bool testStability() {
        std::cout << "  Testing stability under feedback..." << std::endl;
        
        generateTestSignal(500.0f);
        
        // Test with conservative feedback to ensure stability
        std::map<int, float> safeParams = {{0, 0.4f}, {1, 0.5f}, {2, 0.3f}, {3, 0.3f}};
        engine->updateParameters(safeParams);
        
        // Process multiple blocks to test for runaway feedback
        for (int i = 0; i < 20; ++i) {
            engine->process(testBuffer);
            
            if (!isFinite(testBuffer)) {
                std::cout << "    FAIL: Instability at iteration " << i << std::endl;
                return false;
            }
            
            // Check for runaway energy growth
            float energy = calculateEnergy(testBuffer);
            if (energy > 10.0f) { // Reasonable threshold
                std::cout << "    FAIL: Runaway feedback detected. Energy: " << energy << std::endl;
                return false;
            }
        }
        
        std::cout << "    PASS: System remains stable under feedback" << std::endl;
        return true;
    }
    
    // Test 4: Mix Parameter
    bool testMixParameter() {
        std::cout << "  Testing mix parameter..." << std::endl;
        
        generateTestSignal(400.0f);
        AudioBuffer<float> originalBuffer;
        originalBuffer.setSize(2, BUFFER_SIZE);
        originalBuffer.copyFrom(0, 0, testBuffer, 0, 0, BUFFER_SIZE);
        originalBuffer.copyFrom(1, 0, testBuffer, 1, 0, BUFFER_SIZE);
        
        // Test with 0% mix (should be dry)
        std::map<int, float> dryParams = {{0, 0.3f}, {3, 0.0f}};\n        engine->updateParameters(dryParams);\n        engine->process(testBuffer);\n        \n        float dryDifference = calculateRMSDifference(testBuffer, originalBuffer);\n        if (dryDifference > TOLERANCE * 10) { // Allow some tolerance for processing\n            std::cout << \"    FAIL: 0% mix not preserving dry signal\" << std::endl;\n            return false;\n        }\n        \n        std::cout << \"    PASS: Mix parameter working\" << std::endl;\n        return true;\n    }\n    \n    bool runAllTests() {\n        std::cout << \"Running FeedbackNetwork test suite...\" << std::endl;\n        \n        std::vector<std::function<bool()>> tests = {\n            [this]() { return testInitialization(); },\n            [this]() { return testFeedbackProcessing(); },\n            [this]() { return testStability(); },\n            [this]() { return testMixParameter(); }\n        };\n        \n        int passed = 0;\n        for (const auto& test : tests) {\n            if (test()) passed++;\n            engine->reset();\n        }\n        \n        std::cout << \"FeedbackNetwork Results: \" << passed << \"/\" << tests.size() \n                  << \" tests passed (\" << (passed * 100 / tests.size()) << \"%)\" << std::endl;\n        return passed == tests.size();\n    }\n\nprivate:\n    void generateTestSignal(float freq) {\n        const float omega = 2.0f * M_PI * freq / SAMPLE_RATE;\n        for (int i = 0; i < BUFFER_SIZE; ++i) {\n            float sample = 0.5f * std::sin(omega * i);\n            testBuffer.setSample(0, i, sample);\n            testBuffer.setSample(1, i, sample);\n        }\n    }\n    \n    float calculateEnergy(const AudioBuffer<float>& buffer) {\n        float energy = 0.0f;\n        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {\n            for (int i = 0; i < buffer.getNumSamples(); ++i) {\n                float sample = buffer.getSample(ch, i);\n                energy += sample * sample;\n            }\n        }\n        return energy / (buffer.getNumChannels() * buffer.getNumSamples());\n    }\n    \n    float calculateRMSDifference(const AudioBuffer<float>& buffer1, const AudioBuffer<float>& buffer2) {\n        float sum = 0.0f;\n        int totalSamples = 0;\n        \n        for (int ch = 0; ch < std::min(buffer1.getNumChannels(), buffer2.getNumChannels()); ++ch) {\n            for (int i = 0; i < std::min(buffer1.getNumSamples(), buffer2.getNumSamples()); ++i) {\n                float diff = buffer1.getSample(ch, i) - buffer2.getSample(ch, i);\n                sum += diff * diff;\n                totalSamples++;\n            }\n        }\n        \n        return std::sqrt(sum / totalSamples);\n    }\n    \n    bool isFinite(const AudioBuffer<float>& buffer) {\n        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {\n            for (int i = 0; i < buffer.getNumSamples(); ++i) {\n                if (!std::isfinite(buffer.getSample(ch, i))) return false;\n            }\n        }\n        return true;\n    }\n};\n\nint main() {\n    std::cout << \"=== Chimera Phoenix FeedbackNetwork Test Suite ===\" << std::endl;\n    std::cout << \"Engine ID: \" << ENGINE_FEEDBACK_NETWORK << \" (52)\" << std::endl;\n    \n    FeedbackNetworkTest tester;\n    bool allTestsPassed = tester.runAllTests();\n    \n    std::cout << \"Overall Result: \" << (allTestsPassed ? \"PASS\" : \"FAIL\") << std::endl;\n    return allTestsPassed ? 0 : 1;\n}
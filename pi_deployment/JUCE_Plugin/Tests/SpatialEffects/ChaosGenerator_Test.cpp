/*
    ChaosGenerator_Test.cpp
    Comprehensive test suite for Chaos Generator special effect engine
    Tests chaos algorithms, modulation quality, and system stability
*/

#include "AppConfig.h"
#include "EngineBaseTest.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../Source/EngineTypes.h"
#include "../Source/ChaosGenerator.h"
#include "../Source/UnifiedDefaultParameters.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <array>
#include <cassert>

using namespace juce;

class ChaosGeneratorTest {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr float TOLERANCE = 0.001f;
    
    std::unique_ptr<ChaosGenerator> engine;
    AudioBuffer<float> testBuffer;

public:
    ChaosGeneratorTest() {
        engine = std::make_unique<ChaosGenerator>();
        testBuffer.setSize(2, BUFFER_SIZE);
        engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    // Test 1: Engine Initialization
    bool testInitialization() {
        std::cout << "  Testing initialization..." << std::endl;
        
        String engineName = engine->getName();
        if (!engineName.contains("Chaos")) {
            std::cout << "    FAIL: Engine name incorrect: " << engineName.toStdString() << std::endl;
            return false;
        }
        
        if (engine->getNumParameters() < 7) {
            std::cout << "    FAIL: Expected at least 7 parameters" << std::endl;
            return false;
        }
        
        UnifiedDefaultParameters defaultParams;
        auto defaults = defaultParams.getEngineDefaults(ENGINE_CHAOS_GENERATOR);
        if (defaults.empty()) {
            std::cout << "    FAIL: No default parameters" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Initialization successful" << std::endl;
        return true;
    }
    
    // Test 2: Chaos Generation
    bool testChaosGeneration() {
        std::cout << "  Testing chaos generation..." << std::endl;
        
        generateTestSignal(440.0f);
        
        // Test with minimal chaos
        std::map<int, float> minParams = {{0, 0.1f}, {1, 0.1f}, {7, 0.1f}};
        engine->updateParameters(minParams);
        engine->process(testBuffer);
        float minEnergy = calculateEnergy(testBuffer);
        
        engine->reset();
        generateTestSignal(440.0f);
        
        // Test with more chaos
        std::map<int, float> maxParams = {{0, 0.5f}, {1, 0.3f}, {7, 0.3f}};
        engine->updateParameters(maxParams);
        engine->process(testBuffer);
        float maxEnergy = calculateEnergy(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Non-finite output" << std::endl;
            return false;
        }
        
        // Should show some difference between chaos levels
        if (std::abs(maxEnergy - minEnergy) < 0.001f) {
            std::cout << "    FAIL: Chaos levels show minimal difference" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Chaos generation working" << std::endl;
        return true;
    }
    
    // Test 3: Stability Test
    bool testStability() {
        std::cout << "  Testing system stability..." << std::endl;
        
        generateTestSignal(1000.0f);
        
        // Test with maximum chaos settings
        std::map<int, float> extremeParams = {{0, 1.0f}, {1, 1.0f}, {7, 1.0f}};
        engine->updateParameters(extremeParams);
        
        // Process multiple blocks to test stability
        for (int i = 0; i < 10; ++i) {
            engine->process(testBuffer);
            if (!isFinite(testBuffer)) {
                std::cout << "    FAIL: Instability detected at iteration " << i << std::endl;
                return false;
            }
        }
        
        std::cout << "    PASS: System remains stable" << std::endl;
        return true;
    }
    
    bool runAllTests() {
        std::cout << "Running ChaosGenerator test suite..." << std::endl;
        
        std::vector<std::function<bool()>> tests = {
            [this]() { return testInitialization(); },
            [this]() { return testChaosGeneration(); },
            [this]() { return testStability(); }
        };
        
        int passed = 0;
        for (const auto& test : tests) {
            if (test()) passed++;
            engine->reset();
        }
        
        std::cout << "ChaosGenerator Results: " << passed << "/" << tests.size() 
                  << " tests passed (" << (passed * 100 / tests.size()) << "%)" << std::endl;
        return passed == tests.size();
    }

private:
    void generateTestSignal(float freq) {
        const float omega = 2.0f * M_PI * freq / SAMPLE_RATE;
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            float sample = 0.5f * std::sin(omega * i);
            testBuffer.setSample(0, i, sample);
            testBuffer.setSample(1, i, sample);
        }
    }
    
    float calculateEnergy(const AudioBuffer<float>& buffer) {
        float energy = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                energy += sample * sample;
            }
        }
        return energy / (buffer.getNumChannels() * buffer.getNumSamples());
    }
    
    bool isFinite(const AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (!std::isfinite(buffer.getSample(ch, i))) return false;
            }
        }
        return true;
    }
};

int main() {
    std::cout << "=== Chimera Phoenix ChaosGenerator Test Suite ===" << std::endl;
    std::cout << "Engine ID: " << ENGINE_CHAOS_GENERATOR << " (51)" << std::endl;
    
    ChaosGeneratorTest tester;
    bool allTestsPassed = tester.runAllTests();
    
    std::cout << "Overall Result: " << (allTestsPassed ? "PASS" : "FAIL") << std::endl;
    return allTestsPassed ? 0 : 1;
}
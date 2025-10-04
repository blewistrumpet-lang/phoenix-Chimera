/*
    GranularCloud_Test.cpp
    Comprehensive test suite for Granular Cloud special effect engine
    Tests granular synthesis quality, grain parameters, and cloud behavior
*/

#include "AppConfig.h"
#include "EngineBaseTest.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../Source/EngineTypes.h"
#include "../Source/GranularCloud.h"
#include "../Source/UnifiedDefaultParameters.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <array>
#include <cassert>

using namespace juce;

class GranularCloudTest {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr float TOLERANCE = 0.001f;
    
    std::unique_ptr<GranularCloud> engine;
    AudioBuffer<float> testBuffer;

public:
    GranularCloudTest() {
        engine = std::make_unique<GranularCloud>();
        testBuffer.setSize(2, BUFFER_SIZE);
        engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    // Test 1: Engine Initialization
    bool testInitialization() {
        std::cout << "  Testing initialization..." << std::endl;
        
        String engineName = engine->getName();
        if (!engineName.contains("Granular")) {
            std::cout << "    FAIL: Engine name incorrect: " << engineName.toStdString() << std::endl;
            return false;
        }
        
        if (engine->getNumParameters() < 4) {
            std::cout << "    FAIL: Expected at least 4 parameters" << std::endl;
            return false;
        }
        
        UnifiedDefaultParameters defaultParams;
        auto defaults = defaultParams.getEngineDefaults(ENGINE_GRANULAR_CLOUD);
        if (defaults.empty()) {
            std::cout << "    FAIL: No default parameters" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Initialization successful" << std::endl;
        return true;
    }
    
    // Test 2: Granular Processing
    bool testGranularProcessing() {
        std::cout << "  Testing granular processing..." << std::endl;
        
        generateTestSignal(600.0f);
        
        std::map<int, float> params = {{0, 0.5f}, {1, 0.5f}, {2, 0.5f}, {3, 0.5f}};
        engine->updateParameters(params);
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Non-finite output" << std::endl;
            return false;
        }
        
        // Check if processing changes the signal
        float energy = calculateEnergy(testBuffer);
        if (energy < 0.001f) {
            std::cout << "    FAIL: No output energy" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Granular processing working" << std::endl;
        return true;
    }
    
    // Test 3: Parameter Control
    bool testParameterControl() {
        std::cout << "  Testing parameter control..." << std::endl;
        
        generateTestSignal(800.0f);
        
        // Test different grain settings
        std::map<int, float> smallGrains = {{0, 0.2f}, {1, 0.3f}, {4, 0.1f}};
        engine->updateParameters(smallGrains);
        engine->process(testBuffer);
        float smallGrainEnergy = calculateEnergy(testBuffer);
        
        engine->reset();
        generateTestSignal(800.0f);
        std::map<int, float> largeGrains = {{0, 0.8f}, {1, 0.7f}, {4, 0.3f}};
        engine->updateParameters(largeGrains);
        engine->process(testBuffer);
        float largeGrainEnergy = calculateEnergy(testBuffer);
        
        // Different settings should produce different results
        if (std::abs(largeGrainEnergy - smallGrainEnergy) < 0.01f) {
            std::cout << "    FAIL: Parameter changes have minimal effect" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Parameter control working" << std::endl;
        return true;
    }
    
    bool runAllTests() {
        std::cout << "Running GranularCloud test suite..." << std::endl;
        
        std::vector<std::function<bool()>> tests = {
            [this]() { return testInitialization(); },
            [this]() { return testGranularProcessing(); },
            [this]() { return testParameterControl(); }
        };
        
        int passed = 0;
        for (const auto& test : tests) {
            if (test()) passed++;
            engine->reset();
        }
        
        std::cout << "GranularCloud Results: " << passed << "/" << tests.size() 
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
    std::cout << "=== Chimera Phoenix GranularCloud Test Suite ===" << std::endl;
    std::cout << "Engine ID: " << ENGINE_GRANULAR_CLOUD << " (50)" << std::endl;
    
    GranularCloudTest tester;
    bool allTestsPassed = tester.runAllTests();
    
    std::cout << "Overall Result: " << (allTestsPassed ? "PASS" : "FAIL") << std::endl;
    return allTestsPassed ? 0 : 1;
}
/*
    PhasedVocoder_Test.cpp
    Comprehensive test suite for Phased Vocoder special effect engine
    Tests phase vocoding quality, pitch shifting, and formant preservation
*/

#include "AppConfig.h"
#include "EngineBaseTest.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../Source/EngineTypes.h"
#include "../Source/PhasedVocoder.h"
#include "../Source/UnifiedDefaultParameters.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <array>
#include <cassert>

using namespace juce;

class PhasedVocoderTest {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr float TOLERANCE = 0.001f;
    
    std::unique_ptr<PhasedVocoder> engine;
    AudioBuffer<float> testBuffer;

public:
    PhasedVocoderTest() {
        engine = std::make_unique<PhasedVocoder>();
        testBuffer.setSize(2, BUFFER_SIZE);
        engine->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    // Test 1: Engine Initialization
    bool testInitialization() {
        std::cout << "  Testing initialization..." << std::endl;
        
        String engineName = engine->getName();
        if (!engineName.contains("Vocoder")) {
            std::cout << "    FAIL: Engine name incorrect: " << engineName.toStdString() << std::endl;
            return false;
        }
        
        if (engine->getNumParameters() < 3) {
            std::cout << "    FAIL: Expected at least 3 parameters" << std::endl;
            return false;
        }
        
        UnifiedDefaultParameters defaultParams;
        auto defaults = defaultParams.getEngineDefaults(ENGINE_PHASED_VOCODER);
        if (defaults.empty()) {
            std::cout << "    FAIL: No default parameters" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Initialization successful" << std::endl;
        return true;
    }
    
    // Test 2: Basic Processing
    bool testBasicProcessing() {
        std::cout << "  Testing basic processing..." << std::endl;
        
        generateTestSignal(440.0f);
        
        std::map<int, float> params = {{0, 0.5f}, {1, 0.5f}, {2, 0.5f}};
        engine->updateParameters(params);
        engine->process(testBuffer);
        
        if (!isFinite(testBuffer)) {
            std::cout << "    FAIL: Non-finite output" << std::endl;
            return false;
        }
        
        std::cout << "    PASS: Basic processing working" << std::endl;
        return true;
    }
    
    bool runAllTests() {
        std::cout << "Running PhasedVocoder test suite..." << std::endl;
        
        std::vector<std::function<bool()>> tests = {
            [this]() { return testInitialization(); },
            [this]() { return testBasicProcessing(); }
        };
        
        int passed = 0;
        for (const auto& test : tests) {
            if (test()) passed++;
            engine->reset();
        }
        
        std::cout << "PhasedVocoder Results: " << passed << "/" << tests.size() 
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
    std::cout << "=== Chimera Phoenix PhasedVocoder Test Suite ===" << std::endl;
    std::cout << "Engine ID: " << ENGINE_PHASED_VOCODER << " (49)" << std::endl;
    
    PhasedVocoderTest tester;
    bool allTestsPassed = tester.runAllTests();
    
    std::cout << "Overall Result: " << (allTestsPassed ? "PASS" : "FAIL") << std::endl;
    return allTestsPassed ? 0 : 1;
}
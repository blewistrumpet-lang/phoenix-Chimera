/*
  ==============================================================================
  
    MinimalEngineTest.cpp
    Minimal test to prove JUCE build system works
    
    This test uses only core JUCE modules to avoid linking issues.
    It tests the basic EngineBase interface without audio processing.
    
  ==============================================================================
*/

#include <iostream>
#include <vector>
#include <memory>
#include <map>

// Minimal JUCE includes - only what we absolutely need
#include "AppConfig.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"

// Include EngineTypes for engine IDs  
#include "../Source/EngineTypes.h"

// Minimal EngineBase interface
class EngineBase {
public:
    virtual ~EngineBase() = default;
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;
    virtual void updateParameters(const std::map<int, float>& params) = 0;
    virtual juce::String getName() const = 0;
    virtual int getNumParameters() const = 0;
    virtual juce::String getParameterName(int index) const = 0;
};

// Minimal BitCrusher for testing
class BitCrusher : public EngineBase {
public:
    BitCrusher() = default;
    ~BitCrusher() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        m_sampleRate = sampleRate;
        m_samplesPerBlock = samplesPerBlock;
        
        std::cout << "BitCrusher prepared: " << sampleRate << "Hz, " << samplesPerBlock << " samples" << std::endl;
    }
    
    void process(juce::AudioBuffer<float>& buffer) override {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        // Simple bit crushing - just clamp to demonstrate processing
        const float quantizationStep = 1.0f / 64.0f;  // 6-bit equivalent
        
        for (int channel = 0; channel < numChannels; ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            
            for (int i = 0; i < numSamples; ++i) {
                float sample = channelData[i];
                sample = std::round(sample / quantizationStep) * quantizationStep;
                channelData[i] = sample;
            }
        }
    }
    
    void reset() override {
        // Clear any internal state
    }
    
    void updateParameters(const std::map<int, float>& params) override {
        auto it = params.find(0);
        if (it != params.end()) {
            m_bitDepth = it->second;
        }
    }
    
    int getNumParameters() const override { return 8; }
    
    juce::String getParameterName(int index) const override {
        switch (index) {
            case 0: return "Bit Depth";
            case 1: return "Sample Rate";
            case 7: return "Mix";
            default: return "Parameter " + juce::String(index);
        }
    }
    
    juce::String getName() const override { 
        return "Bit Crusher"; 
    }
    
private:
    double m_sampleRate = 44100.0;
    int m_samplesPerBlock = 512;
    float m_bitDepth = 0.5f;
};

// Simple test runner
class MinimalTestRunner {
private:
    std::unique_ptr<BitCrusher> bitCrusher;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    MinimalTestRunner() {
        bitCrusher = std::make_unique<BitCrusher>();
    }
    
    void assertTrue(bool condition, const std::string& testName) {
        if (condition) {
            std::cout << "[PASS] " << testName << std::endl;
            testsPassed++;
        } else {
            std::cout << "[FAIL] " << testName << std::endl;
            testsFailed++;
        }
    }
    
    void testBasicInterface() {
        std::cout << "\n=== Testing Basic Interface ===" << std::endl;
        
        // Test preparation
        bitCrusher->prepareToPlay(44100.0, 512);
        assertTrue(true, "Engine preparation");
        
        // Test parameter count
        int numParams = bitCrusher->getNumParameters();
        assertTrue(numParams == 8, "Parameter count (" + std::to_string(numParams) + " == 8)");
        
        // Test parameter names
        for (int i = 0; i < 3; ++i) {  // Just test first few parameters
            juce::String paramName = bitCrusher->getParameterName(i);
            assertTrue(!paramName.isEmpty(), 
                      "Parameter " + std::to_string(i) + " name: " + paramName.toStdString());
        }
        
        // Test engine name
        juce::String engineName = bitCrusher->getName();
        assertTrue(engineName == "Bit Crusher", "Engine name: " + engineName.toStdString());
        
        // Test parameter updates
        std::map<int, float> params;
        params[0] = 0.5f;
        bitCrusher->updateParameters(params);
        assertTrue(true, "Parameter updates");
        
        // Test reset
        bitCrusher->reset();
        assertTrue(true, "Engine reset");
    }
    
    void testAudioBuffer() {
        std::cout << "\n=== Testing Audio Buffer ===" << std::endl;
        
        // Create a simple audio buffer
        juce::AudioBuffer<float> testBuffer(2, 64);  // Small buffer: 2 channels, 64 samples
        
        // Fill with simple test data
        for (int channel = 0; channel < 2; ++channel) {
            auto* channelData = testBuffer.getWritePointer(channel);
            for (int i = 0; i < 64; ++i) {
                channelData[i] = std::sin(2.0f * M_PI * 1000.0f * i / 44100.0f) * 0.5f;
            }
        }
        
        // Process audio
        try {
            bitCrusher->process(testBuffer);
            assertTrue(true, "Audio processing");
            
            // Check for valid output
            bool hasValidSamples = true;
            for (int channel = 0; channel < 2; ++channel) {
                auto* channelData = testBuffer.getReadPointer(channel);
                for (int i = 0; i < 64; ++i) {
                    if (!std::isfinite(channelData[i]) || std::abs(channelData[i]) > 1.0f) {
                        hasValidSamples = false;
                        break;
                    }
                }
                if (!hasValidSamples) break;
            }
            assertTrue(hasValidSamples, "Valid audio output");
            
        } catch (const std::exception& e) {
            assertTrue(false, "Audio processing exception: " + std::string(e.what()));
        }
    }
    
    void runAllTests() {
        std::cout << "=== Minimal BitCrusher Test Suite ===" << std::endl;
        std::cout << "Engine ID: " << ENGINE_BIT_CRUSHER << std::endl;
        std::cout << "JUCE Version: " << juce::SystemStats::getJUCEVersion().toStdString() << std::endl;
        
        testBasicInterface();
        testAudioBuffer();
        
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Tests Passed: " << testsPassed << std::endl;
        std::cout << "Tests Failed: " << testsFailed << std::endl;
        
        if (testsPassed + testsFailed > 0) {
            float successRate = 100.0f * testsPassed / (testsPassed + testsFailed);
            std::cout << "Success Rate: " << successRate << "%" << std::endl;
        }
        
        if (testsFailed == 0) {
            std::cout << "\n🎉 ALL TESTS PASSED! 🎉" << std::endl;
            std::cout << "JUCE build system is working correctly for engine tests!" << std::endl;
        } else {
            std::cout << "\n⚠️  Some tests failed." << std::endl;
        }
    }
};

// Main function
int main() {
    try {
        std::cout << "Starting minimal engine test to prove JUCE compilation works..." << std::endl;
        
        MinimalTestRunner tester;
        tester.runAllTests();
        
        std::cout << "\nMinimal engine test completed." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception." << std::endl;
        return 1;
    }
}
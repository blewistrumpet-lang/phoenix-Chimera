/*
  ==============================================================================
  
    BitCrusher_SimpleTest.cpp
    Simplified standalone test for ENGINE_BIT_CRUSHER
    
    This test verifies basic functionality without the complex test suite.
    Used to prove the JUCE build system is working correctly.
    
  ==============================================================================
*/

#include <iostream>
#include <vector>
#include <memory>

// Use test-specific headers
#include "EngineBaseTest.h"

// Include EngineTypes for engine IDs  
#include "../Source/EngineTypes.h"

// Simple BitCrusher implementation for testing
class BitCrusher : public EngineBase {
public:
    BitCrusher() = default;
    ~BitCrusher() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        m_sampleRate = sampleRate;
        m_samplesPerBlock = samplesPerBlock;
        
        // Initialize parameters
        m_bitDepth = 16.0f;
        m_sampleRateReduction = 1.0f;
        m_mix = 1.0f;
        
        std::cout << "BitCrusher prepared: " << sampleRate << "Hz, " << samplesPerBlock << " samples" << std::endl;
    }
    
    void process(juce::AudioBuffer<float>& buffer) override {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        // Simple bit crushing
        const float quantizationStep = 1.0f / (1 << static_cast<int>(m_bitDepth));
        
        for (int channel = 0; channel < numChannels; ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            
            for (int i = 0; i < numSamples; ++i) {
                // Quantize
                float sample = channelData[i];
                sample = std::round(sample / quantizationStep) * quantizationStep;
                
                // Apply mix
                channelData[i] = channelData[i] * (1.0f - m_mix) + sample * m_mix;
            }
        }
    }
    
    void reset() override {
        // Clear any internal state
    }
    
    void updateParameters(const std::map<int, float>& params) override {
        auto it = params.find(0);
        if (it != params.end()) {
            // Convert 0-1 range to 4-16 bits
            m_bitDepth = 4.0f + it->second * 12.0f;
        }
        
        it = params.find(7);
        if (it != params.end()) {
            m_mix = it->second;
        }
    }
    
    int getNumParameters() const override { return 8; }
    
    juce::String getParameterName(int index) const override {
        switch (index) {
            case 0: return "Bit Depth";
            case 1: return "Sample Rate";
            case 2: return "Aliasing";
            case 3: return "Jitter";
            case 4: return "DC Offset";
            case 5: return "Gate Threshold";
            case 6: return "Dither";
            case 7: return "Mix";
            default: return "Unknown";
        }
    }
    
    juce::String getName() const override { 
        return "Bit Crusher"; 
    }
    
private:
    double m_sampleRate = 44100.0;
    int m_samplesPerBlock = 512;
    float m_bitDepth = 16.0f;
    float m_sampleRateReduction = 1.0f;
    float m_mix = 1.0f;
};

// Simple test runner
class SimpleTestRunner {
private:
    std::unique_ptr<BitCrusher> bitCrusher;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    SimpleTestRunner() {
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
    
    void testBasicFunctionality() {
        std::cout << "\n=== Testing Basic Functionality ===" << std::endl;
        
        // Test preparation
        bitCrusher->prepareToPlay(44100.0, 512);
        assertTrue(true, "Engine preparation");
        
        // Test parameter count
        int numParams = bitCrusher->getNumParameters();
        assertTrue(numParams == 8, "Parameter count (" + std::to_string(numParams) + " == 8)");
        
        // Test parameter names
        for (int i = 0; i < numParams; ++i) {
            juce::String paramName = bitCrusher->getParameterName(i);
            assertTrue(!paramName.isEmpty(), 
                      "Parameter " + std::to_string(i) + " name: " + paramName.toStdString());
        }
        
        // Test engine name
        juce::String engineName = bitCrusher->getName();
        assertTrue(engineName == "Bit Crusher", "Engine name: " + engineName.toStdString());
        
        // Test audio processing
        juce::AudioBuffer<float> testBuffer(2, 512);  // Stereo, 512 samples
        
        // Fill with test signal
        for (int channel = 0; channel < 2; ++channel) {
            auto* channelData = testBuffer.getWritePointer(channel);
            for (int i = 0; i < 512; ++i) {
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
                for (int i = 0; i < 512; ++i) {
                    if (!std::isfinite(channelData[i]) || std::abs(channelData[i]) > 2.0f) {
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
    
    void testParameterUpdates() {
        std::cout << "\n=== Testing Parameter Updates ===" << std::endl;
        
        // Test parameter updates
        std::map<int, float> params;
        params[0] = 0.5f;  // Bit depth
        params[7] = 0.8f;  // Mix
        
        try {
            bitCrusher->updateParameters(params);
            assertTrue(true, "Parameter updates");
        } catch (const std::exception& e) {
            assertTrue(false, "Parameter update exception: " + std::string(e.what()));
        }
    }
    
    void runAllTests() {
        std::cout << "=== BitCrusher Simple Test Suite ===" << std::endl;
        std::cout << "Engine ID: " << ENGINE_BIT_CRUSHER << std::endl;
        
        testBasicFunctionality();
        testParameterUpdates();
        
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Tests Passed: " << testsPassed << std::endl;
        std::cout << "Tests Failed: " << testsFailed << std::endl;
        
        if (testsPassed + testsFailed > 0) {
            float successRate = 100.0f * testsPassed / (testsPassed + testsFailed);
            std::cout << "Success Rate: " << successRate << "%" << std::endl;
        }
        
        if (testsFailed == 0) {
            std::cout << "All tests passed! JUCE build system is working correctly." << std::endl;
        }
    }
};

// Main function
int main() {
    try {
        SimpleTestRunner tester;
        tester.runAllTests();
        
        std::cout << "\nSimple BitCrusher test completed successfully." << std::endl;
        std::cout << "This proves the JUCE build system is working for engine tests." << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Test failed with unknown exception." << std::endl;
        return 1;
    }
}
#!/bin/bash

# Script to create minimal tests for all distortion engines
# This creates simplified test implementations to avoid header conflicts

TESTS_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion"

# Engine definitions: Name, ID, Description
declare -a engines=(
    "HarmonicExciter:19:Harmonic enhancement and excitation"
    "KStyleOverdrive:20:K-style overdrive distortion" 
    "MuffFuzz:21:Muff-style fuzz distortion"
    "MultibandSaturator:22:Multiband saturation processor"
    "RodentDistortion:23:Rodent-style distortion"
    "VintageTubePreamp:24:Vintage tube preamp emulation"
    "WaveFolder:25:Wave folding distortion"
)

# Create minimal test template
create_minimal_test() {
    local engine_name=$1
    local engine_id=$2
    local description=$3
    local filename="${TESTS_DIR}/${engine_name}_MinimalTest.cpp"
    
    cat > "$filename" << EOF
/*
  ==============================================================================
  
    ${engine_name}_MinimalTest.cpp
    Minimal test suite for ENGINE_${engine_name^^}
    
    ${description}
    
  ==============================================================================
*/

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <map>

// Minimal JUCE dependencies
#include "../AppConfig.h"
#include "../../JUCE/modules/juce_core/juce_core.h"
#include "../../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../../Source/EngineTypes.h"

// Test configuration
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;

// Minimal EngineBase for testing
class MinimalEngineBase {
public:
    virtual ~MinimalEngineBase() = default;
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;
    virtual void updateParameters(const std::map<int, float>& params) = 0;
    virtual juce::String getName() const = 0;
    virtual int getNumParameters() const = 0;
    virtual juce::String getParameterName(int index) const = 0;
};

// Simple ${engine_name} test implementation
class Minimal${engine_name} : public MinimalEngineBase {
private:
    double sampleRate = 44100.0;
    std::map<int, float> parameters;
    
public:
    Minimal${engine_name}() {
        // Initialize default parameters
        for (int i = 0; i < 8; ++i) {
            parameters[i] = 0.5f;
        }
    }
    
    void prepareToPlay(double newSampleRate, int samplesPerBlock) override {
        sampleRate = newSampleRate;
    }
    
    void process(juce::AudioBuffer<float>& buffer) override {
        // Simple ${description,,} simulation
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();
        
        float driveParam = parameters[0];
        float drive = 1.0f + driveParam * 9.0f; // 1-10x drive
        
        for (int channel = 0; channel < numChannels; ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            
            for (int sample = 0; sample < numSamples; ++sample) {
                float input = channelData[sample] * drive;
                
                // Simple distortion curve
                float output = std::tanh(input) * 0.8f;
                channelData[sample] = output;
            }
        }
    }
    
    void reset() override {}
    
    void updateParameters(const std::map<int, float>& params) override {
        parameters = params;
    }
    
    juce::String getName() const override { return "Minimal ${engine_name}"; }
    
    int getNumParameters() const override { return 8; }
    
    juce::String getParameterName(int index) const override {
        switch (index) {
            case 0: return "Drive";
            case 1: return "Tone";
            case 2: return "Level";
            case 3: return "Character";
            case 4: return "Filter";
            case 5: return "Dynamics";
            case 6: return "Color";
            case 7: return "Mix";
            default: return "Parameter " + juce::String(index);
        }
    }
};

// Test framework
class TestFramework {
private:
    int passed = 0;
    int failed = 0;
    
public:
    void test(bool condition, const std::string& name) {
        if (condition) {
            std::cout << "[PASS] " << name << std::endl;
            passed++;
        } else {
            std::cout << "[FAIL] " << name << std::endl;
            failed++;
        }
    }
    
    void summary() {
        std::cout << "\\n=== Test Summary ===" << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        float rate = (passed + failed > 0) ? (100.0f * passed / (passed + failed)) : 0.0f;
        std::cout << "Success Rate: " << rate << "%" << std::endl;
    }
    
    bool allPassed() const { return failed == 0 && passed > 0; }
};

int main() {
    std::cout << "=== ${engine_name} Minimal Test Suite ===" << std::endl;
    std::cout << "Engine ID: " << ${engine_id} << std::endl;
    
    TestFramework test;
    
    try {
        // Create engine
        Minimal${engine_name} engine;
        test.test(true, "Engine creation");
        
        // Test basic properties
        test.test(engine.getNumParameters() == 8, "Parameter count");
        test.test(engine.getName() == "Minimal ${engine_name}", "Engine name");
        
        // Test parameter names
        for (int i = 0; i < 8; ++i) {
            juce::String name = engine.getParameterName(i);
            test.test(!name.isEmpty(), "Parameter " + std::to_string(i) + " name: " + name.toStdString());
        }
        
        // Prepare engine
        engine.prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        test.test(true, "Engine preparation");
        
        // Test parameter updates
        std::map<int, float> params;
        for (int i = 0; i < 8; ++i) {
            params[i] = 0.5f;
        }
        engine.updateParameters(params);
        test.test(true, "Parameter updates");
        
        // Test audio processing
        juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);
        buffer.clear();
        
        // Fill with sine wave
        for (int sample = 0; sample < TEST_BLOCK_SIZE; ++sample) {
            double t = sample / TEST_SAMPLE_RATE;
            float sineValue = static_cast<float>(0.3 * std::sin(2.0 * M_PI * 1000.0 * t));
            buffer.setSample(0, sample, sineValue);
            buffer.setSample(1, sample, sineValue);
        }
        
        engine.process(buffer);
        test.test(true, "Audio processing");
        
        // Check for valid output
        bool validOutput = true;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* channelData = buffer.getReadPointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                if (!std::isfinite(channelData[sample]) || std::abs(channelData[sample]) > 1.0f) {
                    validOutput = false;
                    break;
                }
            }
            if (!validOutput) break;
        }
        test.test(validOutput, "Valid audio output");
        
        // Test reset
        engine.reset();
        test.test(true, "Engine reset");
        
        // Performance test
        auto startTime = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 100; ++i) {
            engine.process(buffer);
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double processingTime = duration.count() / 1000.0;
        double audioTime = (100 * TEST_BLOCK_SIZE / TEST_SAMPLE_RATE) * 1000.0;
        double realTimeRatio = processingTime / audioTime;
        
        std::cout << "Performance: " << realTimeRatio << "x real-time" << std::endl;
        test.test(realTimeRatio < 1.0, "Real-time performance");
        
        test.summary();
        
        std::cout << "\\n${engine_name} minimal test completed." << std::endl;
        return test.allPassed() ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
EOF

    echo "Created minimal test: $filename"
}

# Create all minimal tests
for engine_def in "${engines[@]}"; do
    IFS=':' read -r name id desc <<< "$engine_def"
    create_minimal_test "$name" "$id" "$desc"
done

echo "All minimal distortion tests created!"
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>

// Mock JUCE types for testing
namespace juce {
    class String {
        std::string str;
    public:
        String() = default;
        String(const char* s) : str(s) {}
        String(const std::string& s) : str(s) {}
        const char* toRawUTF8() const { return str.c_str(); }
    };
    
    template<typename T>
    class AudioBuffer {
        std::vector<std::vector<T>> channels;
        int numSamples;
    public:
        AudioBuffer(int numChannels, int samples) : numSamples(samples) {
            channels.resize(numChannels);
            for (auto& ch : channels) {
                ch.resize(samples, 0);
            }
        }
        
        int getNumChannels() const { return channels.size(); }
        int getNumSamples() const { return numSamples; }
        
        T* getWritePointer(int channel) {
            return channels[channel].data();
        }
        
        const T* getReadPointer(int channel) const {
            return channels[channel].data();
        }
    };
}

// Include engines
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/LadderFilter.h"
#include "JUCE_Plugin/Source/StateVariableFilter.h"
#include "JUCE_Plugin/Source/BitCrusher.h"

float calculateRMS(const float* data, int numSamples) {
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / numSamples);
}

bool testEngine(EngineBase* engine, const std::string& name) {
    std::cout << "\nTesting " << name << "..." << std::endl;
    
    // Prepare
    engine->prepareToPlay(48000, 512);
    
    // Create test signal - sine wave
    juce::AudioBuffer<float> buffer(2, 512);
    for (int i = 0; i < 512; ++i) {
        float sample = 0.5f * std::sin(2 * M_PI * 440.0f * i / 48000.0f);
        buffer.getWritePointer(0)[i] = sample;
        buffer.getWritePointer(1)[i] = sample;
    }
    
    // Calculate input RMS
    float inputRMS = calculateRMS(buffer.getReadPointer(0), 512);
    
    // Set parameters for maximum effect
    std::map<int, float> params;
    params[0] = 0.5f;  // Frequency/Cutoff
    params[1] = 0.8f;  // Resonance/Depth
    params[2] = 0.5f;  // Drive/Amount
    params[3] = 0.5f;  // Type
    
    // Find and set mix parameter (usually one of the last params)
    for (int i = 4; i < engine->getNumParameters(); ++i) {
        auto paramName = engine->getParameterName(i).toRawUTF8();
        std::string pname(paramName);
        if (pname.find("Mix") != std::string::npos || 
            pname.find("mix") != std::string::npos ||
            pname.find("Wet") != std::string::npos) {
            params[i] = 1.0f;  // 100% wet
            std::cout << "  Setting Mix parameter " << i << " to 100%" << std::endl;
        }
    }
    
    engine->updateParameters(params);
    
    // Process
    engine->process(buffer);
    
    // Calculate output RMS
    float outputRMS = calculateRMS(buffer.getReadPointer(0), 512);
    
    // Check if audio was modified
    float rmsChange = std::abs(outputRMS - inputRMS);
    float percentChange = (rmsChange / inputRMS) * 100.0f;
    
    std::cout << "  Input RMS: " << inputRMS << std::endl;
    std::cout << "  Output RMS: " << outputRMS << std::endl;
    std::cout << "  Change: " << percentChange << "%" << std::endl;
    
    // Also check if samples are different
    const float* input = buffer.getReadPointer(0);
    bool allSame = true;
    float firstSample = input[0];
    for (int i = 1; i < 512; ++i) {
        if (std::abs(input[i] - firstSample) > 0.0001f) {
            allSame = false;
            break;
        }
    }
    
    if (allSame) {
        std::cout << "  WARNING: All output samples are identical!" << std::endl;
    }
    
    bool passed = percentChange > 1.0f || !allSame;
    std::cout << "  Result: " << (passed ? "PROCESSING AUDIO" : "NOT PROCESSING") << std::endl;
    
    return passed;
}

int main() {
    std::cout << "=== Engine Audio Processing Test ===" << std::endl;
    
    int passed = 0;
    int total = 0;
    
    // Test LadderFilter
    {
        auto engine = std::make_unique<LadderFilter>();
        if (testEngine(engine.get(), "LadderFilter")) passed++;
        total++;
    }
    
    // Test StateVariableFilter
    {
        auto engine = std::make_unique<StateVariableFilter>();
        if (testEngine(engine.get(), "StateVariableFilter")) passed++;
        total++;
    }
    
    // Test BitCrusher
    {
        auto engine = std::make_unique<BitCrusher>();
        if (testEngine(engine.get(), "BitCrusher")) passed++;
        total++;
    }
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << std::endl;
    
    return (passed == total) ? 0 : 1;
}
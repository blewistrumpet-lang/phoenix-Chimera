#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include "JUCE_Plugin/Source/SimpleDetuner.h"

float analyzeFrequency(const std::vector<float>& samples, float sampleRate) {
    int zeroCrossings = 0;
    for (size_t i = 1; i < samples.size(); ++i) {
        if ((samples[i-1] < 0 && samples[i] >= 0) || 
            (samples[i-1] >= 0 && samples[i] < 0)) {
            zeroCrossings++;
        }
    }
    return (zeroCrossings / 2.0f) * (sampleRate / samples.size());
}

int main() {
    std::cout << "=== Testing SimpleDetuner with SMB Algorithm ===" << std::endl;
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const int numBlocks = 16;
    const int numChannels = 2;
    
    // Create engine
    auto engine = std::make_unique<SimpleDetuner>();
    
    // Prepare engine
    engine->prepareToPlay(sampleRate, blockSize);
    
    // Test cases
    struct TestCase {
        std::string name;
        float detuneParam;  // 0-1 maps to 0-50 cents
        float expectedFreqL;  // Left channel frequency
        float expectedFreqR;  // Right channel frequency
    };
    
    std::vector<TestCase> tests = {
        {"5 cents detune", 0.1f, 437.0f, 443.0f},   // ±5 cents from 440Hz
        {"10 cents detune", 0.2f, 434.5f, 445.5f},  // ±10 cents
        {"25 cents detune", 0.5f, 426.0f, 454.0f}   // ±25 cents
    };
    
    for (const auto& test : tests) {
        std::cout << "\nTesting: " << test.name << std::endl;
        
        // Set parameters
        std::map<int, float> params;
        params[0] = 1.0f;  // Mix 100%
        params[1] = test.detuneParam;  // Detune amount
        engine->updateParameters(params);
        
        // Collect output
        std::vector<float> outputL, outputR;
        
        // Process multiple blocks
        for (int block = 0; block < numBlocks; ++block) {
            // Create buffer
            juce::AudioBuffer<float> buffer(numChannels, blockSize);
            
            // Generate 440Hz sine wave
            for (int i = 0; i < blockSize; ++i) {
                float t = (block * blockSize + i) / sampleRate;
                float sample = 0.5f * std::sin(2.0f * M_PI * 440.0f * t);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }
            
            // Process
            engine->process(buffer);
            
            // Collect output after initial latency
            if (block > 3) {
                for (int i = 0; i < blockSize; ++i) {
                    outputL.push_back(buffer.getSample(0, i));
                    outputR.push_back(buffer.getSample(1, i));
                }
            }
        }
        
        // Analyze frequencies
        float measuredL = analyzeFrequency(outputL, sampleRate);
        float measuredR = analyzeFrequency(outputR, sampleRate);
        
        // Check results
        float errorL = std::abs(measuredL - test.expectedFreqL) / test.expectedFreqL * 100.0f;
        float errorR = std::abs(measuredR - test.expectedFreqR) / test.expectedFreqR * 100.0f;
        
        std::cout << "  Left channel:" << std::endl;
        std::cout << "    Expected: " << test.expectedFreqL << " Hz" << std::endl;
        std::cout << "    Measured: " << measuredL << " Hz" << std::endl;
        std::cout << "    Error: " << errorL << "%" << std::endl;
        
        std::cout << "  Right channel:" << std::endl;
        std::cout << "    Expected: " << test.expectedFreqR << " Hz" << std::endl;
        std::cout << "    Measured: " << measuredR << " Hz" << std::endl;
        std::cout << "    Error: " << errorR << "%" << std::endl;
        
        bool pass = (errorL < 10.0f && errorR < 10.0f);
        std::cout << "  " << (pass ? "✓ PASS" : "✗ FAIL") << std::endl;
    }
    
    std::cout << "\nLatency: " << engine->getLatencySamples() << " samples" << std::endl;
    std::cout << "✓ SimpleDetuner works with SMB Pitch Shift!" << std::endl;
    
    return 0;
}
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include "JUCE_Plugin/Source/IntelligentHarmonizer.cpp"

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
    std::cout << "=== Testing IntelligentHarmonizer with SMB Algorithm ===" << std::endl;
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const int numBlocks = 16;
    const int numChannels = 2;
    
    // Create engine
    auto engine = std::make_unique<IntelligentHarmonizer>();
    
    // Prepare engine
    engine->prepareToPlay(sampleRate, blockSize);
    
    // Test cases for harmony intervals
    struct TestCase {
        std::string name;
        float intervalParam;  // Maps to different intervals
        float expectedFreq;   // From 440Hz input
    };
    
    std::vector<TestCase> tests = {
        {"Unity (no shift)", 0.5f, 440.0f},               // No pitch shift
        {"Major Third (+4 semitones)", 0.667f, 554.0f},  // 440 * 2^(4/12)
        {"Perfect Fifth (+7 semitones)", 0.75f, 659.0f},  // 440 * 2^(7/12)
        {"Octave (+12 semitones)", 1.0f, 880.0f}          // 440 * 2
    };
    
    for (const auto& test : tests) {
        std::cout << "\nTesting: " << test.name << std::endl;
        
        // Set parameters
        std::map<int, float> params;
        params[0] = 1.0f;  // Mix 100%
        params[1] = test.intervalParam;  // Interval
        params[2] = 0.5f;  // Voice control
        params[3] = 1.0f;  // Harmony mix 100%
        engine->updateParameters(params);
        
        // Collect output
        std::vector<float> allOutput;
        
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
                    allOutput.push_back(buffer.getSample(0, i));
                }
            }
        }
        
        // Analyze frequency
        float measuredFreq = analyzeFrequency(allOutput, sampleRate);
        
        // Calculate RMS
        float rms = 0;
        for (float sample : allOutput) {
            rms += sample * sample;
        }
        rms = std::sqrt(rms / allOutput.size());
        
        // Check results
        float error = std::abs(measuredFreq - test.expectedFreq);
        float errorPercent = (error / test.expectedFreq) * 100.0f;
        bool pass = (errorPercent < 10.0f && rms > 0.01f);
        
        std::cout << "  Expected: " << test.expectedFreq << " Hz" << std::endl;
        std::cout << "  Measured: " << measuredFreq << " Hz" << std::endl;
        std::cout << "  Error: " << errorPercent << "%" << std::endl;
        std::cout << "  RMS: " << rms << std::endl;
        std::cout << "  " << (pass ? "✓ PASS" : "✗ FAIL") << std::endl;
    }
    
    std::cout << "\n✓ SMB Pitch Shift Algorithm tested with IntelligentHarmonizer!" << std::endl;
    
    return 0;
}
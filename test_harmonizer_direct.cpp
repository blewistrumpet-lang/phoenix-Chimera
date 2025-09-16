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
    std::cout << "=== Direct Test of IntelligentHarmonizer with SMB ===" << std::endl;
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const int numBlocks = 16;
    const int numChannels = 2;
    
    // Create engine
    auto engine = std::make_unique<IntelligentHarmonizer>();
    
    // Prepare engine
    engine->prepareToPlay(sampleRate, blockSize);
    
    // Test direct pitch ratio setting
    struct TestCase {
        std::string name;
        float pitchRatio;
        float expectedFreq;
    };
    
    std::vector<TestCase> tests = {
        {"Unity", 1.0f, 440.0f},
        {"Octave Up", 2.0f, 880.0f},
        {"Fifth Up", 1.5f, 660.0f},
        {"Major Third", 1.26f, 554.0f}
    };
    
    for (const auto& test : tests) {
        std::cout << "\nTesting: " << test.name << std::endl;
        
        // Directly set pitch ratio using internal method
        engine->pimpl->setPitchRatio(test.pitchRatio);
        engine->pimpl->setMix(1.0f); // 100% wet
        
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
            if (block > 4) {
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
        float error = std::abs(measuredFreq - test.expectedFreq) / test.expectedFreq * 100.0f;
        bool pass = (error < 10.0f && rms > 0.01f);
        
        std::cout << "  Pitch Ratio: " << test.pitchRatio << std::endl;
        std::cout << "  Expected: " << test.expectedFreq << " Hz" << std::endl;
        std::cout << "  Measured: " << measuredFreq << " Hz" << std::endl;
        std::cout << "  Error: " << error << "%" << std::endl;
        std::cout << "  RMS: " << rms << std::endl;
        std::cout << "  " << (pass ? "✓ PASS" : "✗ FAIL") << std::endl;
    }
    
    std::cout << "\nLatency: " << engine->getLatencySamples() << " samples" << std::endl;
    std::cout << "✓ Direct test complete!" << std::endl;
    
    return 0;
}
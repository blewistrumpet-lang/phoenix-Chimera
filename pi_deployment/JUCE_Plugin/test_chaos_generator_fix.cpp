#include <iostream>
#include <memory>
#include <map>

// Mock JUCE components for testing
namespace juce {
    template<typename T>
    class AudioBuffer {
    public:
        AudioBuffer(int numChannels, int numSamples) 
            : channels(numChannels), samples(numSamples) {
            for (int ch = 0; ch < numChannels; ++ch) {
                data.emplace_back(numSamples, 0.0f);
            }
        }
        
        int getNumChannels() const { return channels; }
        int getNumSamples() const { return samples; }
        
        T* getWritePointer(int channel) { return data[channel].data(); }
        
    private:
        int channels, samples;
        std::vector<std::vector<T>> data;
    };
    
    class String {
    public:
        String(const char* str) : s(str) {}
        const char* toRawUTF8() const { return s.c_str(); }
    private:
        std::string s;
    };
}

// Include the actual implementation
#include "Source/ChaosGenerator_Platinum.h"

int main() {
    std::cout << "Testing Chaos Generator Platinum after fixes...\n";
    
    // Create the engine
    auto chaosGen = std::make_unique<ChaosGenerator_Platinum>();
    
    // Prepare for processing
    chaosGen->prepareToPlay(44100.0, 512);
    
    // Set parameters to match the new defaults
    std::map<int, float> params;
    params[0] = 0.3f;   // Rate - Moderate chaos rate  
    params[1] = 0.5f;   // Depth - Substantial depth
    params[2] = 0.0f;   // Type - Lorenz attractor
    params[3] = 0.5f;   // Smoothing - Moderate smoothing
    params[4] = 0.8f;   // Target - ModGenerate mode for audio generation
    params[5] = 0.0f;   // Sync - No tempo sync
    params[6] = 0.5f;   // Seed - Random seed
    params[7] = 1.0f;   // Mix - Full effect
    
    chaosGen->updateParameters(params);
    
    // Test processing with silence (should generate audio due to ModGenerate)
    juce::AudioBuffer<float> buffer(2, 512);
    
    // Fill with silence
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = 0.0f;
        }
    }
    
    // Process the buffer
    chaosGen->process(buffer);
    
    // Check if audio was generated
    float maxValue = 0.0f;
    float rmsValue = 0.0f;
    int nonZeroSamples = 0;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(data[i]);
            if (sample > 0.0001f) {
                nonZeroSamples++;
                maxValue = std::max(maxValue, sample);
                rmsValue += sample * sample;
            }
        }
    }
    
    rmsValue = std::sqrt(rmsValue / (buffer.getNumChannels() * buffer.getNumSamples()));
    
    std::cout << "Results:\n";
    std::cout << "- Engine created successfully: " << (chaosGen ? "YES" : "NO") << "\n";
    std::cout << "- Parameter count: " << chaosGen->getNumParameters() << " (expected: 8)\n";
    std::cout << "- Engine name: " << chaosGen->getName().toRawUTF8() << "\n";
    std::cout << "- Non-zero samples: " << nonZeroSamples << " / " << (buffer.getNumChannels() * buffer.getNumSamples()) << "\n";
    std::cout << "- Max amplitude: " << maxValue << "\n";
    std::cout << "- RMS level: " << rmsValue << "\n";
    
    // Verify parameter names
    std::cout << "\nParameter names:\n";
    for (int i = 0; i < chaosGen->getNumParameters(); ++i) {
        std::cout << "  " << i << ": " << chaosGen->getParameterName(i).toRawUTF8() << "\n";
    }
    
    bool success = (nonZeroSamples > 0) && (maxValue > 0.01f) && (chaosGen->getNumParameters() == 8);
    
    std::cout << "\nOverall test result: " << (success ? "SUCCESS" : "FAILURE") << "\n";
    std::cout << "The Chaos Generator should now produce " << (success ? "audible" : "NO") << " effects!\n";
    
    return success ? 0 : 1;
}
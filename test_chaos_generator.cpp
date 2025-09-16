// Test Chaos Generator to see why it's not producing output
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <iomanip>
#include <JuceHeader.h>

#include "JUCE_Plugin/Source/ChaosGenerator.h"

constexpr double SAMPLE_RATE = 48000.0;
constexpr int BLOCK_SIZE = 512;

void analyzeBuffer(const juce::AudioBuffer<float>& buffer, const std::string& label) {
    float rms = 0.0f;
    float peak = 0.0f;
    int nonZeroSamples = 0;
    float sum = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = data[i];
            sum += sample;
            rms += sample * sample;
            peak = std::max(peak, std::abs(sample));
            if (std::abs(sample) > 0.0001f) {
                nonZeroSamples++;
            }
        }
    }
    
    rms = std::sqrt(rms / (buffer.getNumSamples() * buffer.getNumChannels()));
    float avg = sum / (buffer.getNumSamples() * buffer.getNumChannels());
    
    std::cout << label << ":" << std::endl;
    std::cout << "  RMS: " << std::fixed << std::setprecision(6) << rms << std::endl;
    std::cout << "  Peak: " << peak << std::endl;
    std::cout << "  Average: " << avg << std::endl;
    std::cout << "  Non-zero samples: " << nonZeroSamples << " / " 
              << (buffer.getNumSamples() * buffer.getNumChannels()) << std::endl;
}

void testChaosGenerator() {
    std::cout << "=====================================" << std::endl;
    std::cout << "    CHAOS GENERATOR DEBUG TEST      " << std::endl;
    std::cout << "=====================================" << std::endl;
    
    auto chaos = std::make_unique<ChaosGenerator>();
    
    // Prepare the engine
    chaos->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
    chaos->reset();
    
    std::cout << "\n1. Testing with default parameters:" << std::endl;
    std::cout << "   Number of parameters: " << chaos->getNumParameters() << std::endl;
    
    // Process with default parameters (should have mix=0.5 now)
    {
        juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
        
        // Fill with test signal
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                data[i] = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
            }
        }
        
        analyzeBuffer(buffer, "Input signal");
        
        chaos->process(buffer);
        
        analyzeBuffer(buffer, "After processing (default params)");
    }
    
    std::cout << "\n2. Testing with mix = 1.0 (fully wet):" << std::endl;
    {
        std::map<int, float> params;
        params[7] = 1.0f; // Set mix to maximum
        chaos->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
        
        // Fill with test signal
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                data[i] = std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * 0.5f;
            }
        }
        
        analyzeBuffer(buffer, "Input signal");
        
        chaos->process(buffer);
        
        analyzeBuffer(buffer, "After processing (mix=1.0)");
    }
    
    std::cout << "\n3. Testing each parameter at max:" << std::endl;
    for (int paramIdx = 0; paramIdx < chaos->getNumParameters(); ++paramIdx) {
        juce::String paramName = chaos->getParameterName(paramIdx);
        std::cout << "\n  Testing parameter " << paramIdx << " (" << paramName << ") at max:" << std::endl;
        
        // Reset all parameters
        std::map<int, float> params;
        for (int i = 0; i < chaos->getNumParameters(); ++i) {
            params[i] = 0.5f; // Default
        }
        params[7] = 1.0f; // Mix at max
        params[paramIdx] = 1.0f; // Test parameter at max
        
        chaos->updateParameters(params);
        chaos->reset();
        
        juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
        
        // Fill with test signal
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                data[i] = 0.5f; // Constant signal to see modulation clearly
            }
        }
        
        // Process multiple blocks to let chaos develop
        float totalChange = 0.0f;
        for (int block = 0; block < 10; ++block) {
            juce::AudioBuffer<float> copyBuffer(buffer);
            chaos->process(buffer);
            
            // Measure change
            for (int ch = 0; ch < 2; ++ch) {
                const float* original = copyBuffer.getReadPointer(ch);
                const float* processed = buffer.getReadPointer(ch);
                for (int i = 0; i < BLOCK_SIZE; ++i) {
                    totalChange += std::abs(processed[i] - original[i]);
                }
            }
        }
        
        if (totalChange > 0.01f) {
            std::cout << "    ✅ Parameter has effect (change: " << totalChange << ")" << std::endl;
        } else {
            std::cout << "    ❌ Parameter has NO effect (change: " << totalChange << ")" << std::endl;
        }
    }
    
    std::cout << "\n4. Testing with different chaos types:" << std::endl;
    {
        // Test each chaos type (0.0 to 1.0 maps to different types)
        std::vector<std::pair<float, std::string>> chaosTypes = {
            {0.0f, "Lorenz"},
            {0.2f, "Rossler"},
            {0.4f, "Henon"},
            {0.6f, "Logistic"},
            {0.8f, "Ikeda"},
            {1.0f, "Duffing"}
        };
        
        for (auto& [typeValue, typeName] : chaosTypes) {
            std::map<int, float> params;
            params[2] = typeValue; // Set chaos type
            params[1] = 1.0f; // Max depth
            params[7] = 1.0f; // Max mix
            chaos->updateParameters(params);
            chaos->reset();
            
            juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < BLOCK_SIZE; ++i) {
                    buffer.setSample(ch, i, 0.5f);
                }
            }
            
            float beforeRMS = 0.5f;
            chaos->process(buffer);
            
            float afterRMS = buffer.getRMSLevel(0, 0, BLOCK_SIZE);
            std::cout << "  " << typeName << ": RMS change = " 
                      << std::abs(afterRMS - beforeRMS) << std::endl;
        }
    }
    
    std::cout << "\n5. Testing with continuous input (1 second):" << std::endl;
    {
        // Set aggressive parameters
        std::map<int, float> params;
        params[0] = 0.5f; // Rate
        params[1] = 1.0f; // Depth at max
        params[2] = 0.0f; // Lorenz
        params[3] = 0.0f; // Low smoothing for more variation
        params[4] = 0.0f; // Amplitude modulation
        params[7] = 1.0f; // Mix at max
        
        chaos->updateParameters(params);
        chaos->reset();
        
        float totalEnergy = 0.0f;
        float totalVariation = 0.0f;
        
        // Process 1 second of audio
        int numBlocks = SAMPLE_RATE / BLOCK_SIZE;
        for (int blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
            juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
            
            // Generate input signal
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < BLOCK_SIZE; ++i) {
                    data[i] = 0.5f; // Constant to see modulation
                }
            }
            
            juce::AudioBuffer<float> original(buffer);
            chaos->process(buffer);
            
            // Analyze
            for (int ch = 0; ch < 2; ++ch) {
                const float* origData = original.getReadPointer(ch);
                const float* procData = buffer.getReadPointer(ch);
                for (int i = 0; i < BLOCK_SIZE; ++i) {
                    totalEnergy += procData[i] * procData[i];
                    totalVariation += std::abs(procData[i] - origData[i]);
                }
            }
        }
        
        std::cout << "  Total energy: " << totalEnergy << std::endl;
        std::cout << "  Total variation from input: " << totalVariation << std::endl;
        
        if (totalVariation < 0.01f) {
            std::cout << "  ❌ NO MODULATION DETECTED!" << std::endl;
        } else {
            std::cout << "  ✅ Modulation detected" << std::endl;
        }
    }
}

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    testChaosGenerator();
    
    return 0;
}
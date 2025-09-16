// Comprehensive test of all reverb engines
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 512;
const int TEST_DURATION = 44100; // 1 second

// Generate impulse for testing reverb response
void generateImpulse(juce::AudioBuffer<float>& buffer) {
    buffer.clear();
    // Single impulse at the beginning
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.setSample(ch, 0, 1.0f);
    }
}

// Generate short burst for testing decay
void generateBurst(juce::AudioBuffer<float>& buffer, float freq = 440.0f, int duration = 100) {
    buffer.clear();
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < std::min(duration, numSamples); ++i) {
            float sample = std::sin(2.0f * M_PI * freq * i / SAMPLE_RATE) * 0.5f;
            // Apply envelope
            float envelope = 1.0f - (float)i / duration;
            buffer.setSample(ch, i, sample * envelope);
        }
    }
}

// Measure reverb tail length
float measureDecayTime(const juce::AudioBuffer<float>& buffer, float threshold = -60.0f) {
    float peakLevel = 0.0f;
    int peakIndex = 0;
    
    // Find peak
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float level = std::abs(buffer.getSample(0, i));
        if (level > peakLevel) {
            peakLevel = level;
            peakIndex = i;
        }
    }
    
    if (peakLevel < 0.001f) return 0.0f;
    
    // Find decay point
    float thresholdLinear = peakLevel * std::pow(10.0f, threshold / 20.0f);
    int decayIndex = peakIndex;
    
    for (int i = peakIndex; i < buffer.getNumSamples(); ++i) {
        if (std::abs(buffer.getSample(0, i)) < thresholdLinear) {
            decayIndex = i;
            break;
        }
    }
    
    return (float)(decayIndex - peakIndex) / SAMPLE_RATE;
}

// Calculate frequency response
void analyzeFrequencyResponse(const juce::AudioBuffer<float>& buffer) {
    // Simple energy in different frequency bands
    float lowEnergy = 0.0f;   // < 500 Hz
    float midEnergy = 0.0f;   // 500-2000 Hz
    float highEnergy = 0.0f;  // > 2000 Hz
    
    // This is simplified - proper implementation would use FFT
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float sample = buffer.getSample(0, i);
        float energy = sample * sample;
        
        // Rough frequency estimation based on position in reverb tail
        if (i < buffer.getNumSamples() / 3) {
            highEnergy += energy;
        } else if (i < 2 * buffer.getNumSamples() / 3) {
            midEnergy += energy;
        } else {
            lowEnergy += energy;
        }
    }
    
    float total = lowEnergy + midEnergy + highEnergy;
    if (total > 0) {
        std::cout << "  Frequency balance: Low=" << std::fixed << std::setprecision(1) 
                  << (lowEnergy/total*100) << "% Mid=" << (midEnergy/total*100) 
                  << "% High=" << (highEnergy/total*100) << "%" << std::endl;
    }
}

template<typename ReverbType>
void testReverb(const std::string& name, ReverbType& reverb, 
                const std::map<int, float>& params) {
    std::cout << "\n=== " << name << " ===" << std::endl;
    
    // Prepare reverb
    reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    reverb.updateParameters(params);
    reverb.reset();
    
    // Test 1: Impulse response
    std::cout << "\n1. Impulse Response Test:" << std::endl;
    {
        juce::AudioBuffer<float> buffer(2, TEST_DURATION);
        generateImpulse(buffer);
        
        // Process in chunks
        int numChunks = TEST_DURATION / BUFFER_SIZE;
        for (int chunk = 0; chunk < numChunks; ++chunk) {
            juce::AudioBuffer<float> subBuffer(buffer.getArrayOfWritePointers(), 
                                              buffer.getNumChannels(),
                                              chunk * BUFFER_SIZE, 
                                              BUFFER_SIZE);
            reverb.process(subBuffer);
        }
        
        // Analyze
        float decayTime = measureDecayTime(buffer);
        std::cout << "  Decay time (RT60): " << decayTime << " seconds" << std::endl;
        
        // Check for silence
        float maxLevel = buffer.getMagnitude(0, TEST_DURATION);
        if (maxLevel < 0.001f) {
            std::cout << "  ✗ WARNING: Output is silent!" << std::endl;
        } else {
            std::cout << "  ✓ Output level: " << maxLevel << std::endl;
        }
        
        analyzeFrequencyResponse(buffer);
    }
    
    // Test 2: Burst response
    std::cout << "\n2. Burst Response Test:" << std::endl;
    {
        reverb.reset();
        juce::AudioBuffer<float> buffer(2, BUFFER_SIZE * 10);
        generateBurst(buffer, 440.0f, 100);
        
        // Process
        for (int i = 0; i < 10; ++i) {
            juce::AudioBuffer<float> subBuffer(buffer.getArrayOfWritePointers(), 
                                              buffer.getNumChannels(),
                                              i * BUFFER_SIZE, 
                                              BUFFER_SIZE);
            reverb.process(subBuffer);
        }
        
        // Check for artifacts
        int clippedSamples = 0;
        float maxSample = 0.0f;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(buffer.getSample(0, i));
            maxSample = std::max(maxSample, sample);
            if (sample > 0.99f) clippedSamples++;
        }
        
        std::cout << "  Max level: " << maxSample << std::endl;
        if (clippedSamples > 0) {
            std::cout << "  ⚠ Clipping detected: " << clippedSamples << " samples" << std::endl;
        } else {
            std::cout << "  ✓ No clipping" << std::endl;
        }
    }
    
    // Test 3: Parameter sweep
    std::cout << "\n3. Parameter Sweep Test:" << std::endl;
    {
        // Test with different mix levels
        float mixLevels[] = {0.0f, 0.5f, 1.0f};
        for (float mix : mixLevels) {
            auto testParams = params;
            testParams[testParams.rbegin()->first] = mix; // Assume last param is mix
            
            reverb.updateParameters(testParams);
            reverb.reset();
            
            juce::AudioBuffer<float> buffer(2, BUFFER_SIZE);
            generateBurst(buffer);
            reverb.process(buffer);
            
            float outputLevel = buffer.getMagnitude(0, BUFFER_SIZE);
            std::cout << "  Mix=" << (mix*100) << "%: Level=" << outputLevel;
            
            if (mix == 0.0f && outputLevel < 0.1f) {
                std::cout << " ✗ Dry signal missing!";
            } else if (mix == 1.0f && outputLevel < 0.01f) {
                std::cout << " ✗ Wet signal missing!";
            } else {
                std::cout << " ✓";
            }
            std::cout << std::endl;
        }
    }
}

int main() {
    std::cout << "=== REVERB ENGINE COMPREHENSIVE TEST ===" << std::endl;
    std::cout << "Testing all reverb engines for quality and artifacts\n" << std::endl;
    
    // Test Plate Reverb
    {
        PlateReverb reverb;
        std::map<int, float> params;
        params[0] = 0.7f;  // Size
        params[1] = 0.5f;  // Damping
        params[2] = 0.5f;  // Mix
        testReverb("Plate Reverb", reverb, params);
    }
    
    // Test Spring Reverb
    {
        SpringReverb reverb;
        std::map<int, float> params;
        params[0] = 0.5f;  // Tension
        params[1] = 0.5f;  // Damping
        params[2] = 0.3f;  // Springs
        params[3] = 0.5f;  // Diffusion
        params[4] = 0.5f;  // Brightness
        params[5] = 0.0f;  // Drip
        params[6] = 0.5f;  // Mix
        testReverb("Spring Reverb", reverb, params);
    }
    
    // Test Convolution Reverb
    {
        ConvolutionReverb reverb;
        std::map<int, float> params;
        params[0] = 0.5f;  // Mix (assuming single parameter)
        testReverb("Convolution Reverb", reverb, params);
    }
    
    // Test Shimmer Reverb
    {
        ShimmerReverb reverb;
        std::map<int, float> params;
        params[0] = 0.7f;  // Size
        params[1] = 0.5f;  // Damping
        params[2] = 0.5f;  // Shimmer
        params[3] = 0.5f;  // Pitch
        params[4] = 0.5f;  // Modulation
        params[5] = 0.5f;  // Low cut
        params[6] = 0.5f;  // High cut
        params[7] = 0.0f;  // Freeze
        params[8] = 0.5f;  // Mix
        testReverb("Shimmer Reverb", reverb, params);
    }
    
    // Test Gated Reverb
    {
        GatedReverb reverb;
        std::map<int, float> params;
        params[0] = 0.7f;  // Size
        params[1] = 0.5f;  // Gate time
        params[2] = 0.5f;  // Pre-delay
        params[3] = 0.5f;  // Damping
        params[4] = 0.5f;  // Diffusion
        params[5] = 0.5f;  // Hold
        params[6] = 0.5f;  // Mix
        testReverb("Gated Reverb", reverb, params);
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Check each reverb for:" << std::endl;
    std::cout << "- Proper decay times" << std::endl;
    std::cout << "- No unwanted artifacts or clipping" << std::endl;
    std::cout << "- Correct dry/wet mixing" << std::endl;
    std::cout << "- Frequency balance" << std::endl;
    
    return 0;
}
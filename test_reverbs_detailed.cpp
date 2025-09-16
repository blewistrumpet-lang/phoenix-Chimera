// Comprehensive test of all reverb engines with detailed analysis
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
#include <random>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 512;
const int LONG_BUFFER = 88200; // 2 seconds

// Generate test signals
void generateImpulse(juce::AudioBuffer<float>& buffer, int position = 0) {
    buffer.clear();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.setSample(ch, position, 1.0f);
    }
}

void generateSineWave(juce::AudioBuffer<float>& buffer, float freq, float duration) {
    buffer.clear();
    int numSamples = std::min((int)(duration * SAMPLE_RATE), buffer.getNumSamples());
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float sample = std::sin(2.0f * M_PI * freq * i / SAMPLE_RATE) * 0.3f;
            buffer.setSample(ch, i, sample);
        }
    }
}

void generateWhiteNoise(juce::AudioBuffer<float>& buffer, int duration) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-0.3f, 0.3f);
    
    buffer.clear();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < std::min(duration, buffer.getNumSamples()); ++i) {
            buffer.setSample(ch, i, dis(gen));
        }
    }
}

// Analysis functions
struct ReverbAnalysis {
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    float decayTime = 0.0f;
    float earlyReflections = 0.0f;
    float lateDiffusion = 0.0f;
    float stereoWidth = 0.0f;
    bool hasOutput = false;
    bool isClipping = false;
};

ReverbAnalysis analyzeReverb(const juce::AudioBuffer<float>& buffer) {
    ReverbAnalysis analysis;
    
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    if (numSamples == 0 || numChannels == 0) return analysis;
    
    // Find peak and RMS
    float sumSquares = 0.0f;
    int firstNonZero = -1;
    int lastNonZero = -1;
    
    for (int i = 0; i < numSamples; ++i) {
        float sample = buffer.getSample(0, i);
        float absSample = std::abs(sample);
        
        if (absSample > analysis.peakLevel) {
            analysis.peakLevel = absSample;
        }
        
        if (absSample > 0.99f) {
            analysis.isClipping = true;
        }
        
        sumSquares += sample * sample;
        
        if (absSample > 0.001f) {
            if (firstNonZero < 0) firstNonZero = i;
            lastNonZero = i;
        }
    }
    
    analysis.rmsLevel = std::sqrt(sumSquares / numSamples);
    analysis.hasOutput = (analysis.peakLevel > 0.001f);
    
    // Calculate decay time
    if (firstNonZero >= 0 && lastNonZero > firstNonZero) {
        analysis.decayTime = (float)(lastNonZero - firstNonZero) / SAMPLE_RATE;
    }
    
    // Analyze early reflections (first 50ms)
    int earlyEnd = std::min(firstNonZero + (int)(0.05f * SAMPLE_RATE), numSamples);
    if (firstNonZero >= 0 && earlyEnd > firstNonZero) {
        float earlySum = 0.0f;
        for (int i = firstNonZero; i < earlyEnd; ++i) {
            earlySum += std::abs(buffer.getSample(0, i));
        }
        analysis.earlyReflections = earlySum / (earlyEnd - firstNonZero);
    }
    
    // Analyze stereo width (if stereo)
    if (numChannels >= 2) {
        float correlation = 0.0f;
        float leftPower = 0.0f;
        float rightPower = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            float left = buffer.getSample(0, i);
            float right = buffer.getSample(1, i);
            correlation += left * right;
            leftPower += left * left;
            rightPower += right * right;
        }
        
        if (leftPower > 0 && rightPower > 0) {
            float normalizedCorr = correlation / std::sqrt(leftPower * rightPower);
            analysis.stereoWidth = 1.0f - std::abs(normalizedCorr);
        }
    }
    
    return analysis;
}

template<typename ReverbType>
void testReverbEngine(const std::string& name, ReverbType& reverb, 
                      const std::map<int, float>& defaultParams,
                      const std::vector<std::pair<std::string, std::map<int, float>>>& presets) {
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing: " << name << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Initialize
    reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test each preset
    for (const auto& [presetName, params] : presets) {
        std::cout << "\nPreset: " << presetName << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        reverb.updateParameters(params);
        reverb.reset();
        
        // Test 1: Impulse Response
        {
            juce::AudioBuffer<float> buffer(2, LONG_BUFFER);
            generateImpulse(buffer);
            
            // Process in blocks
            int numBlocks = LONG_BUFFER / BUFFER_SIZE;
            for (int block = 0; block < numBlocks; ++block) {
                juce::AudioBuffer<float> subBuffer(
                    buffer.getArrayOfWritePointers(),
                    buffer.getNumChannels(),
                    block * BUFFER_SIZE,
                    BUFFER_SIZE
                );
                reverb.process(subBuffer);
            }
            
            ReverbAnalysis analysis = analyzeReverb(buffer);
            
            std::cout << "  Impulse Response:" << std::endl;
            std::cout << "    Peak Level: " << std::fixed << std::setprecision(3) << analysis.peakLevel;
            if (analysis.isClipping) std::cout << " [CLIPPING!]";
            std::cout << std::endl;
            std::cout << "    RMS Level: " << analysis.rmsLevel << std::endl;
            std::cout << "    Decay Time: " << analysis.decayTime << " sec" << std::endl;
            std::cout << "    Early Reflections: " << analysis.earlyReflections << std::endl;
            std::cout << "    Stereo Width: " << (analysis.stereoWidth * 100) << "%" << std::endl;
            std::cout << "    Status: " << (analysis.hasOutput ? "✓ Working" : "✗ NOT WORKING") << std::endl;
        }
        
        // Test 2: Sine Wave (Musical content)
        {
            reverb.reset();
            juce::AudioBuffer<float> buffer(2, BUFFER_SIZE * 4);
            generateSineWave(buffer, 440.0f, 0.1f); // 100ms burst
            
            // Process
            for (int block = 0; block < 4; ++block) {
                juce::AudioBuffer<float> subBuffer(
                    buffer.getArrayOfWritePointers(),
                    buffer.getNumChannels(),
                    block * BUFFER_SIZE,
                    BUFFER_SIZE
                );
                reverb.process(subBuffer);
            }
            
            float outputEnergy = buffer.getMagnitude(0, buffer.getNumSamples());
            std::cout << "  Sine Wave (440Hz):" << std::endl;
            std::cout << "    Output Energy: " << outputEnergy << std::endl;
        }
        
        // Test 3: White Noise (Dense content)
        {
            reverb.reset();
            juce::AudioBuffer<float> buffer(2, BUFFER_SIZE * 2);
            generateWhiteNoise(buffer, BUFFER_SIZE / 2);
            
            // Process
            for (int block = 0; block < 2; ++block) {
                juce::AudioBuffer<float> subBuffer(
                    buffer.getArrayOfWritePointers(),
                    buffer.getNumChannels(),
                    block * BUFFER_SIZE,
                    BUFFER_SIZE
                );
                reverb.process(subBuffer);
            }
            
            float outputEnergy = buffer.getMagnitude(0, buffer.getNumSamples());
            std::cout << "  White Noise:" << std::endl;
            std::cout << "    Output Energy: " << outputEnergy << std::endl;
        }
    }
}

int main() {
    std::cout << "=== COMPREHENSIVE REVERB ENGINE TEST ===" << std::endl;
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Buffer Size: " << BUFFER_SIZE << " samples" << std::endl;
    
    // Test Plate Reverb
    {
        PlateReverb reverb;
        std::vector<std::pair<std::string, std::map<int, float>>> presets = {
            {"Small Room (50% mix)", {{0, 0.3f}, {1, 0.5f}, {2, 0.5f}}},
            {"Large Hall (70% mix)", {{0, 0.8f}, {1, 0.3f}, {2, 0.7f}}},
            {"100% Wet", {{0, 0.6f}, {1, 0.4f}, {2, 1.0f}}},
            {"Bright Plate", {{0, 0.5f}, {1, 0.2f}, {2, 0.6f}}}
        };
        testReverbEngine("PLATE REVERB", reverb, {{0, 0.5f}, {1, 0.5f}, {2, 0.5f}}, presets);
    }
    
    // Test Spring Reverb
    {
        SpringReverb reverb;
        std::vector<std::pair<std::string, std::map<int, float>>> presets = {
            {"Vintage Spring", {{0, 0.4f}, {1, 0.3f}, {2, 0.3f}, {3, 0.6f}, {4, 0.5f}, {5, 0.1f}, {6, 0.5f}}},
            {"Bright Spring", {{0, 0.5f}, {1, 0.2f}, {2, 0.5f}, {3, 0.7f}, {4, 0.8f}, {5, 0.0f}, {6, 0.6f}}},
            {"Dark Spring", {{0, 0.3f}, {1, 0.6f}, {2, 0.4f}, {3, 0.5f}, {4, 0.2f}, {5, 0.0f}, {6, 0.5f}}},
            {"100% Wet + Drip", {{0, 0.5f}, {1, 0.3f}, {2, 0.5f}, {3, 0.6f}, {4, 0.5f}, {5, 0.5f}, {6, 1.0f}}}
        };
        testReverbEngine("SPRING REVERB", reverb, {{0, 0.5f}, {1, 0.5f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f}, {5, 0.0f}, {6, 0.5f}}, presets);
    }
    
    // Test Convolution Reverb
    {
        ConvolutionReverb reverb;
        std::vector<std::pair<std::string, std::map<int, float>>> presets = {
            {"50% Mix", {{0, 0.5f}}},
            {"100% Wet", {{0, 1.0f}}},
            {"Subtle (25%)", {{0, 0.25f}}}
        };
        testReverbEngine("CONVOLUTION REVERB", reverb, {{0, 0.5f}}, presets);
    }
    
    // Test Shimmer Reverb
    {
        ShimmerReverb reverb;
        std::vector<std::pair<std::string, std::map<int, float>>> presets = {
            {"Subtle Shimmer", {{0, 0.5f}, {1, 0.4f}, {2, 0.3f}, {3, 0.5f}, {4, 0.3f}, {5, 0.2f}, {6, 0.8f}, {7, 0.0f}, {8, 0.5f}}},
            {"Ethereal", {{0, 0.8f}, {1, 0.2f}, {2, 0.7f}, {3, 0.7f}, {4, 0.5f}, {5, 0.1f}, {6, 0.9f}, {7, 0.0f}, {8, 0.7f}}},
            {"Frozen Shimmer", {{0, 0.9f}, {1, 0.1f}, {2, 0.5f}, {3, 0.6f}, {4, 0.4f}, {5, 0.2f}, {6, 0.8f}, {7, 1.0f}, {8, 0.8f}}},
            {"100% Wet Max", {{0, 1.0f}, {1, 0.0f}, {2, 1.0f}, {3, 1.0f}, {4, 0.5f}, {5, 0.0f}, {6, 1.0f}, {7, 0.0f}, {8, 1.0f}}}
        };
        testReverbEngine("SHIMMER REVERB", reverb, {{0, 0.7f}, {1, 0.5f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f}, {5, 0.5f}, {6, 0.5f}, {7, 0.0f}, {8, 0.5f}}, presets);
    }
    
    // Test Gated Reverb
    {
        GatedReverb reverb;
        std::vector<std::pair<std::string, std::map<int, float>>> presets = {
            {"Classic Gate", {{0, 0.6f}, {1, 0.3f}, {2, 0.1f}, {3, 0.5f}, {4, 0.6f}, {5, 0.2f}, {6, 0.6f}}},
            {"Long Gate", {{0, 0.8f}, {1, 0.8f}, {2, 0.2f}, {3, 0.3f}, {4, 0.7f}, {5, 0.5f}, {6, 0.7f}}},
            {"Tight Gate", {{0, 0.4f}, {1, 0.1f}, {2, 0.05f}, {3, 0.6f}, {4, 0.5f}, {5, 0.1f}, {6, 0.5f}}},
            {"100% Wet", {{0, 0.7f}, {1, 0.5f}, {2, 0.1f}, {3, 0.4f}, {4, 0.6f}, {5, 0.3f}, {6, 1.0f}}}
        };
        testReverbEngine("GATED REVERB", reverb, {{0, 0.7f}, {1, 0.5f}, {2, 0.1f}, {3, 0.5f}, {4, 0.5f}, {5, 0.3f}, {6, 0.5f}}, presets);
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST COMPLETE" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nSummary:" << std::endl;
    std::cout << "- Check for reverbs marked as 'NOT WORKING'" << std::endl;
    std::cout << "- Verify decay times are reasonable (0.5-5 seconds typical)" << std::endl;
    std::cout << "- Look for clipping indicators" << std::endl;
    std::cout << "- Ensure stereo width shows decorrelation" << std::endl;
    
    return 0;
}
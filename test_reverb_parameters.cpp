// Detailed parameter test for each reverb engine
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <iomanip>
#include <map>
#include <JuceHeader.h>

// Include reverb engines
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

constexpr double SAMPLE_RATE = 48000.0;
constexpr int BLOCK_SIZE = 512;
constexpr int TEST_DURATION = SAMPLE_RATE * 2; // 2 seconds per test

// Analyze audio characteristics
struct AudioAnalysis {
    float rms = 0.0f;
    float peak = 0.0f;
    float spectralCentroid = 0.0f;
    float zeroCrossings = 0;
    float tailEnergy = 0.0f; // Energy in last 50% of buffer
    
    void print(const std::string& label) const {
        std::cout << "    " << label << ": ";
        std::cout << "RMS=" << std::fixed << std::setprecision(4) << rms;
        std::cout << " Peak=" << peak;
        std::cout << " Centroid=" << spectralCentroid;
        std::cout << " ZeroCross=" << zeroCrossings;
        std::cout << " TailEnergy=" << tailEnergy << std::endl;
    }
    
    float difference(const AudioAnalysis& other) const {
        float diff = 0;
        diff += std::abs(rms - other.rms) * 10; // Weight RMS heavily
        diff += std::abs(peak - other.peak) * 5;
        diff += std::abs(spectralCentroid - other.spectralCentroid) / 1000;
        diff += std::abs(zeroCrossings - other.zeroCrossings) / 10000;
        diff += std::abs(tailEnergy - other.tailEnergy) * 10;
        return diff;
    }
};

AudioAnalysis analyzeBuffer(const juce::AudioBuffer<float>& buffer) {
    AudioAnalysis result;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    if (numSamples == 0 || numChannels == 0) return result;
    
    // Calculate RMS and Peak
    float sumSquares = 0;
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            float sample = data[i];
            sumSquares += sample * sample;
            result.peak = std::max(result.peak, std::abs(sample));
        }
    }
    result.rms = std::sqrt(sumSquares / (numSamples * numChannels));
    
    // Calculate zero crossings
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        float prevSample = 0;
        for (int i = 0; i < numSamples; ++i) {
            if ((prevSample < 0 && data[i] >= 0) || (prevSample >= 0 && data[i] < 0)) {
                result.zeroCrossings++;
            }
            prevSample = data[i];
        }
    }
    
    // Calculate spectral centroid (simplified)
    float weightedSum = 0;
    float magnitudeSum = 0;
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            float magnitude = std::abs(data[i]);
            weightedSum += i * magnitude;
            magnitudeSum += magnitude;
        }
    }
    if (magnitudeSum > 0) {
        result.spectralCentroid = weightedSum / magnitudeSum;
    }
    
    // Calculate tail energy (last 50% of buffer)
    float tailSum = 0;
    int tailStart = numSamples / 2;
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = tailStart; i < numSamples; ++i) {
            tailSum += data[i] * data[i];
        }
    }
    result.tailEnergy = std::sqrt(tailSum / ((numSamples - tailStart) * numChannels));
    
    return result;
}

// Generate test impulse
void generateImpulse(juce::AudioBuffer<float>& buffer) {
    buffer.clear();
    // Strong impulse at the beginning
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.setSample(ch, 0, 1.0f);
    }
}

// Test a single parameter
bool testParameter(EngineBase* reverb, int paramIndex, const std::string& paramName) {
    std::cout << "\n  Testing Parameter " << paramIndex << ": " << paramName << std::endl;
    
    // Test at different parameter values
    std::vector<float> testValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    std::vector<AudioAnalysis> analyses;
    
    for (float value : testValues) {
        // Set all parameters to default (0.5) except the one we're testing
        std::map<int, float> params;
        for (int i = 0; i < reverb->getNumParameters(); ++i) {
            params[i] = 0.5f;
        }
        params[paramIndex] = value;
        
        reverb->updateParameters(params);
        reverb->reset();
        
        // Process test signal
        juce::AudioBuffer<float> testBuffer(2, TEST_DURATION);
        
        for (int pos = 0; pos < TEST_DURATION; pos += BLOCK_SIZE) {
            int samplesToProcess = std::min(BLOCK_SIZE, TEST_DURATION - pos);
            juce::AudioBuffer<float> block(2, samplesToProcess);
            
            // Generate impulse only at the start
            if (pos == 0) {
                generateImpulse(block);
            } else {
                block.clear();
            }
            
            reverb->process(block);
            
            // Copy to test buffer
            for (int ch = 0; ch < 2; ++ch) {
                testBuffer.copyFrom(ch, pos, block, ch, 0, samplesToProcess);
            }
        }
        
        AudioAnalysis analysis = analyzeBuffer(testBuffer);
        analyses.push_back(analysis);
        
        std::cout << "    Value=" << std::fixed << std::setprecision(2) << value;
        analysis.print("");
    }
    
    // Check if parameter actually changes the sound
    bool hasEffect = false;
    float totalDifference = 0;
    
    for (size_t i = 1; i < analyses.size(); ++i) {
        float diff = analyses[i].difference(analyses[0]);
        totalDifference += diff;
        if (diff > 0.01f) { // Threshold for detecting change
            hasEffect = true;
        }
    }
    
    if (hasEffect) {
        std::cout << "    ✅ Parameter has audible effect (total diff: " << totalDifference << ")" << std::endl;
    } else {
        std::cout << "    ⚠️  Parameter has NO effect (total diff: " << totalDifference << ")" << std::endl;
    }
    
    return hasEffect;
}

// Test a complete reverb engine
void testReverbEngine(const std::string& name, EngineBase* reverb) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing: " << name << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Prepare reverb
    reverb->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
    reverb->reset();
    
    std::cout << "Number of parameters: " << reverb->getNumParameters() << std::endl;
    
    // First test: Does it produce any output at all?
    std::cout << "\n1. Basic Output Test:" << std::endl;
    {
        std::map<int, float> params;
        for (int i = 0; i < reverb->getNumParameters(); ++i) {
            params[i] = 0.5f; // All parameters at 50%
        }
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE);
        generateImpulse(testBuffer);
        
        AudioAnalysis beforeAnalysis = analyzeBuffer(testBuffer);
        reverb->process(testBuffer);
        AudioAnalysis afterAnalysis = analyzeBuffer(testBuffer);
        
        std::cout << "  Before processing: RMS=" << beforeAnalysis.rms 
                  << " Peak=" << beforeAnalysis.peak << std::endl;
        std::cout << "  After processing:  RMS=" << afterAnalysis.rms 
                  << " Peak=" << afterAnalysis.peak << std::endl;
        
        if (afterAnalysis.rms > beforeAnalysis.rms * 0.5f) {
            std::cout << "  ✅ Reverb produces output" << std::endl;
        } else {
            std::cout << "  ❌ Reverb produces no significant output!" << std::endl;
        }
    }
    
    // Test each parameter
    std::cout << "\n2. Parameter Tests:" << std::endl;
    int workingParams = 0;
    int totalParams = reverb->getNumParameters();
    
    for (int i = 0; i < totalParams; ++i) {
        juce::String paramName = reverb->getParameterName(i);
        if (testParameter(reverb, i, paramName.toStdString())) {
            workingParams++;
        }
    }
    
    std::cout << "\n3. Summary for " << name << ":" << std::endl;
    std::cout << "  Working parameters: " << workingParams << "/" << totalParams << std::endl;
    
    if (workingParams == totalParams) {
        std::cout << "  ✅ All parameters working correctly!" << std::endl;
    } else if (workingParams > 0) {
        std::cout << "  ⚠️  Some parameters not working" << std::endl;
    } else {
        std::cout << "  ❌ No parameters working!" << std::endl;
    }
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "  REVERB PARAMETER VALIDATION TEST  " << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    // Test each reverb engine
    {
        std::cout << "\n[1/5] PLATE REVERB" << std::endl;
        auto reverb = std::make_unique<PlateReverb>();
        testReverbEngine("PlateReverb", reverb.get());
    }
    
    {
        std::cout << "\n[2/5] SPRING REVERB" << std::endl;
        auto reverb = std::make_unique<SpringReverb>();
        testReverbEngine("SpringReverb", reverb.get());
    }
    
    {
        std::cout << "\n[3/5] GATED REVERB" << std::endl;
        auto reverb = std::make_unique<GatedReverb>();
        testReverbEngine("GatedReverb", reverb.get());
    }
    
    {
        std::cout << "\n[4/5] SHIMMER REVERB" << std::endl;
        auto reverb = std::make_unique<ShimmerReverb>();
        testReverbEngine("ShimmerReverb", reverb.get());
    }
    
    {
        std::cout << "\n[5/5] CONVOLUTION REVERB" << std::endl;
        auto reverb = std::make_unique<ConvolutionReverb>();
        testReverbEngine("ConvolutionReverb", reverb.get());
    }
    
    std::cout << "\n=====================================" << std::endl;
    std::cout << "       PARAMETER TEST COMPLETE       " << std::endl;
    std::cout << "=====================================" << std::endl;
    
    return 0;
}
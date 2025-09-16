// ShimmerReverb - Simplified rebuild focusing on basic functionality first
#include "ShimmerReverb.h"
#include <cmath>
#include <algorithm>
#include <memory>

// Very simple reverb using delay lines
class SimpleReverb {
    std::vector<float> delayBuffer;
    int writePos = 0;
    int delaySize = 0;
    float feedback = 0.7f;
    float damping = 0.3f;
    float dampState = 0.0f;
    
public:
    void init(double sampleRate) {
        // 50ms delay
        delaySize = static_cast<int>(sampleRate * 0.05);
        delayBuffer.resize(delaySize, 0.0f);
        writePos = 0;
        dampState = 0.0f;
    }
    
    void setParameters(float roomSize, float damp) {
        feedback = 0.5f + roomSize * 0.45f;  // 0.5 to 0.95
        damping = damp * 0.5f;
    }
    
    float process(float input) {
        if (delaySize == 0) return input;
        
        // Read delayed sample
        float delayed = delayBuffer[writePos];
        
        // Apply damping (simple lowpass)
        dampState = delayed * (1.0f - damping) + dampState * damping;
        
        // Write input + feedback
        delayBuffer[writePos] = input + dampState * feedback;
        
        // Advance position
        writePos = (writePos + 1) % delaySize;
        
        return delayed;
    }
    
    void reset() {
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
        dampState = 0.0f;
        writePos = 0;
    }
};

class ShimmerReverb::Impl {
public:
    // Simple reverb engines (4 delays in parallel for richer sound)
    static constexpr int NUM_DELAYS = 4;
    SimpleReverb reverbsL[NUM_DELAYS];
    SimpleReverb reverbsR[NUM_DELAYS];
    
    // Shimmer buffer (for octave up effect)
    std::vector<float> shimmerBufferL;
    std::vector<float> shimmerBufferR;
    int shimmerWritePos = 0;
    int shimmerSize = 0;
    
    // Simple pitch shift by sample dropping/repeating
    int octaveCounter = 0;
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;
    
    // Parameters
    float pitchShift = 0.5f;      // 0=down octave, 0.5=+octave, 1=+2 octaves
    float shimmerAmount = 0.3f;    // 0-1 shimmer mix
    float roomSize = 0.7f;         // 0-1 reverb size
    float damping = 0.3f;          // 0-1 damping
    float mix = 0.5f;              // 0-1 wet/dry mix
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize reverbs with different delay times
        float delayTimes[NUM_DELAYS] = {0.029f, 0.037f, 0.043f, 0.053f};
        for (int i = 0; i < NUM_DELAYS; i++) {
            // Custom init for each delay
            int delaySamples = static_cast<int>(sr * delayTimes[i]);
            reverbsL[i].delayBuffer.resize(delaySamples, 0.0f);
            reverbsL[i].delaySize = delaySamples;
            
            // Slightly different for right channel (stereo width)
            delaySamples = static_cast<int>(sr * delayTimes[i] * 1.05f);
            reverbsR[i].delayBuffer.resize(delaySamples, 0.0f);
            reverbsR[i].delaySize = delaySamples;
        }
        
        // Shimmer buffer (100ms)
        shimmerSize = static_cast<int>(sr * 0.1);
        shimmerBufferL.resize(shimmerSize, 0.0f);
        shimmerBufferR.resize(shimmerSize, 0.0f);
        
        reset();
    }
    
    void reset() {
        for (int i = 0; i < NUM_DELAYS; i++) {
            reverbsL[i].reset();
            reverbsR[i].reset();
        }
        
        std::fill(shimmerBufferL.begin(), shimmerBufferL.end(), 0.0f);
        std::fill(shimmerBufferR.begin(), shimmerBufferR.end(), 0.0f);
        
        shimmerWritePos = 0;
        octaveCounter = 0;
        lastSampleL = 0.0f;
        lastSampleR = 0.0f;
    }
    
    void updateParameters() {
        for (int i = 0; i < NUM_DELAYS; i++) {
            reverbsL[i].setParameters(roomSize, damping);
            reverbsR[i].setParameters(roomSize, damping);
        }
    }
    
    // Simple octave up by sample doubling
    float processOctaveUp(float input, float& lastSample, int& counter) {
        // Every other sample, repeat the previous one (crude but works)
        if (counter % 2 == 0) {
            lastSample = input;
            counter++;
            return input;
        } else {
            counter++;
            return lastSample;  // Repeat previous sample
        }
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        updateParameters();
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        for (int sample = 0; sample < numSamples; sample++) {
            // Get input
            float inputL = leftData[sample];
            float inputR = rightData ? rightData[sample] : inputL;
            
            // Store dry signal
            float dryL = inputL;
            float dryR = inputR;
            
            // Read shimmer feedback
            float shimmerL = shimmerBufferL[shimmerWritePos] * shimmerAmount * 0.3f;
            float shimmerR = shimmerBufferR[shimmerWritePos] * shimmerAmount * 0.3f;
            
            // Mix input with shimmer
            float reverbInputL = inputL + shimmerL;
            float reverbInputR = inputR + shimmerR;
            
            // Process through parallel reverbs
            float wetL = 0.0f;
            float wetR = 0.0f;
            
            for (int i = 0; i < NUM_DELAYS; i++) {
                wetL += reverbsL[i].process(reverbInputL) * 0.25f;
                wetR += reverbsR[i].process(reverbInputR) * 0.25f;
            }
            
            // Create shimmer effect (simple octave up)
            if (shimmerAmount > 0.01f && pitchShift > 0.4f) {
                float shimmerOutL = processOctaveUp(wetL, lastSampleL, octaveCounter);
                float shimmerOutR = processOctaveUp(wetR, lastSampleR, octaveCounter);
                
                // Store in shimmer buffer
                shimmerBufferL[shimmerWritePos] = shimmerOutL;
                shimmerBufferR[shimmerWritePos] = shimmerOutR;
            } else {
                shimmerBufferL[shimmerWritePos] = 0.0f;
                shimmerBufferR[shimmerWritePos] = 0.0f;
            }
            
            // Advance shimmer buffer position
            shimmerWritePos = (shimmerWritePos + 1) % shimmerSize;
            
            // Mix wet and dry
            leftData[sample] = dryL * (1.0f - mix) + wetL * mix;
            if (rightData) {
                rightData[sample] = dryR * (1.0f - mix) + wetR * mix;
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: pitchShift = value; break;
            case 1: shimmerAmount = value; break;
            case 2: roomSize = value; break;
            case 3: damping = value; break;
            case 4: mix = value; break;
        }
    }
};

// Public interface
ShimmerReverb::ShimmerReverb() : pImpl(std::make_unique<Impl>()) {}
ShimmerReverb::~ShimmerReverb() = default;

void ShimmerReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->init(sampleRate);
}

void ShimmerReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void ShimmerReverb::reset() {
    pImpl->reset();
}

void ShimmerReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 5) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String ShimmerReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Pitch Shift";
        case 1: return "Shimmer";
        case 2: return "Room Size";
        case 3: return "Damping";
        case 4: return "Mix";
        default: return "";
    }
}

int ShimmerReverb::getNumParameters() const {
    return 5;
}

juce::String ShimmerReverb::getName() const {
    return "Shimmer Reverb";
}
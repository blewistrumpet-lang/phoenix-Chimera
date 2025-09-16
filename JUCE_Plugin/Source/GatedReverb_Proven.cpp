// GatedReverb_Proven.cpp - Integration of proven gated reverb algorithm
// Combines Freeverb with envelope-based gating
// Based on classic 80s gated reverb techniques

#include "GatedReverb.h"
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

namespace {
    // Freeverb constants
    const int numCombs = 8;
    const int numAllpasses = 4;
    const float fixedGain = 0.015f;
    const float scaleDamp = 0.4f;
    const float scaleRoom = 0.28f;
    const float offsetRoom = 0.7f;
    const int stereoSpread = 23;
    
    const int combTuning[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    const int allpassTuning[4] = { 556, 441, 341, 225 };
}

// Comb filter from Freeverb
class Comb {
public:
    void setBuffer(float *buf, int size) {
        buffer = buf;
        bufferSize = size;
        bufferIndex = 0;
    }
    
    void mute() {
        if (buffer) {
            std::fill(buffer, buffer + bufferSize, 0.0f);
        }
        filterStore = 0.0f;
    }
    
    void setDamp(float val) {
        damp1 = val;
        damp2 = 1.0f - val;
    }
    
    void setFeedback(float val) {
        feedback = val;
    }
    
    float process(float input) {
        if (!buffer || bufferSize == 0) return 0.0f;
        
        float output = buffer[bufferIndex];
        filterStore = (output * damp2) + (filterStore * damp1);
        buffer[bufferIndex] = input + (filterStore * feedback);
        
        if (++bufferIndex >= bufferSize) {
            bufferIndex = 0;
        }
        
        return output;
    }
    
private:
    float *buffer = nullptr;
    int bufferSize = 0;
    int bufferIndex = 0;
    float filterStore = 0.0f;
    float damp1 = 0.0f;
    float damp2 = 1.0f;
    float feedback = 0.0f;
};

// Allpass filter from Freeverb
class Allpass {
public:
    void setBuffer(float *buf, int size) {
        buffer = buf;
        bufferSize = size;
        bufferIndex = 0;
    }
    
    void mute() {
        if (buffer) {
            std::fill(buffer, buffer + bufferSize, 0.0f);
        }
    }
    
    void setFeedback(float val) {
        feedback = val;
    }
    
    float process(float input) {
        if (!buffer || bufferSize == 0) return 0.0f;
        
        float bufout = buffer[bufferIndex];
        float output = -input + bufout;
        buffer[bufferIndex] = input + (bufout * feedback);
        
        if (++bufferIndex >= bufferSize) {
            bufferIndex = 0;
        }
        
        return output;
    }
    
private:
    float *buffer = nullptr;
    int bufferSize = 0;
    int bufferIndex = 0;
    float feedback = 0.5f;
};

// Gate envelope follower
class GateEnvelope {
public:
    enum State {
        CLOSED,
        ATTACK,
        HOLD,
        RELEASE
    };
    
    void init(double sr) {
        sampleRate = sr;
        reset();
    }
    
    void reset() {
        state = CLOSED;
        envelope = 0.0f;
        holdCounter = 0;
        inputLevel = 0.0f;
    }
    
    void setThreshold(float thresh) {
        threshold = thresh * 0.5f; // Scale for better response
    }
    
    void setAttack(float ms) {
        float samples = ms * sampleRate / 1000.0f;
        attackRate = 1.0f / std::max(samples, 1.0f);
    }
    
    void setHold(float ms) {
        holdTime = static_cast<int>(ms * sampleRate / 1000.0f);
    }
    
    void setRelease(float ms) {
        float samples = ms * sampleRate / 1000.0f;
        releaseRate = 1.0f / std::max(samples, 1.0f);
    }
    
    float process(float input) {
        // Simple peak detection
        float absInput = std::abs(input);
        inputLevel = absInput > inputLevel ? absInput : inputLevel * 0.9999f;
        
        // State machine for gate
        switch (state) {
            case CLOSED:
                if (inputLevel > threshold) {
                    state = ATTACK;
                }
                break;
                
            case ATTACK:
                envelope += attackRate;
                if (envelope >= 1.0f) {
                    envelope = 1.0f;
                    state = HOLD;
                    holdCounter = 0;
                }
                break;
                
            case HOLD:
                envelope = 1.0f;
                holdCounter++;
                if (holdCounter >= holdTime) {
                    if (inputLevel < threshold * 0.8f) { // Hysteresis
                        state = RELEASE;
                    } else {
                        holdCounter = 0; // Reset hold if still above threshold
                    }
                }
                break;
                
            case RELEASE:
                envelope -= releaseRate;
                if (envelope <= 0.0f) {
                    envelope = 0.0f;
                    state = CLOSED;
                } else if (inputLevel > threshold) {
                    state = ATTACK; // Retrigger
                }
                break;
        }
        
        return envelope;
    }
    
    float getEnvelope() const { return envelope; }
    
private:
    double sampleRate = 44100.0;
    State state = CLOSED;
    float envelope = 0.0f;
    float inputLevel = 0.0f;
    float threshold = 0.1f;
    float attackRate = 0.001f;
    float releaseRate = 0.001f;
    int holdTime = 100;
    int holdCounter = 0;
};

// Main GatedReverb implementation
class GatedReverb::Impl {
public:
    // Freeverb components
    Comb combL[numCombs];
    Comb combR[numCombs];
    Allpass allpassL[numAllpasses];
    Allpass allpassR[numAllpasses];
    
    // Buffers
    std::vector<float> combBufferL[numCombs];
    std::vector<float> combBufferR[numCombs];
    std::vector<float> allpassBufferL[numAllpasses];
    std::vector<float> allpassBufferR[numAllpasses];
    
    // Gate envelope
    GateEnvelope gate;
    
    // Pre-delay
    std::vector<float> predelayBufferL;
    std::vector<float> predelayBufferR;
    int predelayIndex = 0;
    int predelaySize = 0;
    
    // Filters
    float lowCutStateL = 0.0f;
    float lowCutStateR = 0.0f;
    float highCutStateL = 0.0f;
    float highCutStateR = 0.0f;
    float lowCutCoeff = 0.0f;
    float highCutCoeff = 0.0f;
    
    // Parameters
    float mixParam = 0.5f;
    float thresholdParam = 0.3f;
    float holdParam = 0.3f;
    float releaseParam = 0.5f;
    float attackParam = 0.1f;
    float sizeParam = 0.5f;
    float dampingParam = 0.5f;
    float predelayParam = 0.0f;
    float lowCutParam = 0.1f;
    float highCutParam = 0.8f;
    
    // Freeverb parameters
    float roomSize = 0.5f;
    float damping = 0.5f;
    float gain = fixedGain;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize gate
        gate.init(sr);
        
        // Calculate scaling for sample rate
        float srScale = static_cast<float>(sr / 44100.0);
        
        // Initialize comb filters
        for (int i = 0; i < numCombs; i++) {
            int sizeL = static_cast<int>(combTuning[i] * srScale);
            int sizeR = static_cast<int>((combTuning[i] + stereoSpread) * srScale);
            
            combBufferL[i].resize(sizeL);
            combBufferR[i].resize(sizeR);
            
            combL[i].setBuffer(combBufferL[i].data(), sizeL);
            combR[i].setBuffer(combBufferR[i].data(), sizeR);
        }
        
        // Initialize allpass filters
        for (int i = 0; i < numAllpasses; i++) {
            int sizeL = static_cast<int>(allpassTuning[i] * srScale);
            int sizeR = static_cast<int>((allpassTuning[i] + stereoSpread) * srScale);
            
            allpassBufferL[i].resize(sizeL);
            allpassBufferR[i].resize(sizeR);
            
            allpassL[i].setBuffer(allpassBufferL[i].data(), sizeL);
            allpassR[i].setBuffer(allpassBufferR[i].data(), sizeR);
            allpassL[i].setFeedback(0.5f);
            allpassR[i].setFeedback(0.5f);
        }
        
        // Initialize predelay
        int maxPredelay = static_cast<int>(0.2f * sr);
        predelayBufferL.resize(maxPredelay);
        predelayBufferR.resize(maxPredelay);
        
        updateInternalParameters();
        reset();
    }
    
    void reset() {
        // Clear Freeverb
        for (int i = 0; i < numCombs; i++) {
            combL[i].mute();
            combR[i].mute();
        }
        
        for (int i = 0; i < numAllpasses; i++) {
            allpassL[i].mute();
            allpassR[i].mute();
        }
        
        // Reset gate
        gate.reset();
        
        // Clear predelay
        std::fill(predelayBufferL.begin(), predelayBufferL.end(), 0.0f);
        std::fill(predelayBufferR.begin(), predelayBufferR.end(), 0.0f);
        predelayIndex = 0;
        
        // Reset filter states
        lowCutStateL = 0.0f;
        lowCutStateR = 0.0f;
        highCutStateL = 0.0f;
        highCutStateR = 0.0f;
    }
    
    void updateInternalParameters() {
        // Gate parameters
        gate.setThreshold(thresholdParam);
        gate.setAttack(0.1f + attackParam * 99.9f); // 0.1 to 100 ms
        gate.setHold(10.0f + holdParam * 490.0f); // 10 to 500 ms
        gate.setRelease(10.0f + releaseParam * 990.0f); // 10 to 1000 ms
        
        // Reverb size (shorter for gated reverb)
        roomSize = (sizeParam * scaleRoom * 0.7f) + offsetRoom; // Slightly smaller room
        
        // Damping
        damping = dampingParam * scaleDamp;
        
        // Update comb filters
        for (int i = 0; i < numCombs; i++) {
            combL[i].setFeedback(roomSize);
            combR[i].setFeedback(roomSize);
            combL[i].setDamp(damping);
            combR[i].setDamp(damping);
        }
        
        // Pre-delay
        predelaySize = static_cast<int>(predelayParam * 0.1f * sampleRate);
        
        // Filter coefficients
        float lowCutFreq = 20.0f * std::pow(50.0f, lowCutParam);
        lowCutCoeff = 1.0f - std::exp(-2.0f * M_PI * lowCutFreq / sampleRate);
        
        float highCutFreq = 1000.0f * std::pow(20.0f, highCutParam);
        highCutCoeff = std::exp(-2.0f * M_PI * highCutFreq / sampleRate);
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        for (int i = 0; i < numSamples; i++) {
            float inputL = leftData[i];
            float inputR = rightData ? rightData[i] : inputL;
            
            // Update gate envelope based on input
            float gateEnv = gate.process((inputL + inputR) * 0.5f);
            
            // Apply pre-delay
            float delayedL = inputL;
            float delayedR = inputR;
            
            if (predelaySize > 0) {
                delayedL = predelayBufferL[predelayIndex];
                delayedR = predelayBufferR[predelayIndex];
                predelayBufferL[predelayIndex] = inputL;
                predelayBufferR[predelayIndex] = inputR;
                
                if (++predelayIndex >= predelaySize) {
                    predelayIndex = 0;
                }
            }
            
            // Process through Freeverb
            float reverbL = 0.0f;
            float reverbR = 0.0f;
            
            // Accumulate comb filters
            for (int j = 0; j < numCombs; j++) {
                reverbL += combL[j].process(delayedL);
                reverbR += combR[j].process(delayedR);
            }
            
            // Process through allpasses
            for (int j = 0; j < numAllpasses; j++) {
                reverbL = allpassL[j].process(reverbL);
                reverbR = allpassR[j].process(reverbR);
            }
            
            // Apply gain correction
            reverbL *= gain;
            reverbR *= gain;
            
            // Apply filters
            if (lowCutParam > 0.001f) {
                lowCutStateL += (reverbL - lowCutStateL) * lowCutCoeff;
                reverbL = reverbL - lowCutStateL;
                
                lowCutStateR += (reverbR - lowCutStateR) * lowCutCoeff;
                reverbR = reverbR - lowCutStateR;
            }
            
            if (highCutParam < 0.999f) {
                highCutStateL = reverbL * (1.0f - highCutCoeff) + highCutStateL * highCutCoeff;
                reverbL = highCutStateL;
                
                highCutStateR = reverbR * (1.0f - highCutCoeff) + highCutStateR * highCutCoeff;
                reverbR = highCutStateR;
            }
            
            // Apply gate envelope to reverb
            reverbL *= gateEnv;
            reverbR *= gateEnv;
            
            // Mix dry and wet
            float wetGain = mixParam;
            float dryGain = 1.0f - mixParam;
            
            leftData[i] = inputL * dryGain + reverbL * wetGain;
            if (rightData) {
                rightData[i] = inputR * dryGain + reverbR * wetGain;
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: mixParam = value; break;
            case 1: thresholdParam = value; break;
            case 2: holdParam = value; break;
            case 3: releaseParam = value; break;
            case 4: attackParam = value; break;
            case 5: sizeParam = value; break;
            case 6: dampingParam = value; break;
            case 7: predelayParam = value; break;
            case 8: lowCutParam = value; break;
            case 9: highCutParam = value; break;
        }
        
        updateInternalParameters();
    }
};

// Public interface
GatedReverb::GatedReverb() : pImpl(std::make_unique<Impl>()) {}
GatedReverb::~GatedReverb() = default;

void GatedReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->init(sampleRate);
}

void GatedReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void GatedReverb::reset() {
    pImpl->reset();
}

void GatedReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 10) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String GatedReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mix";
        case 1: return "Threshold";
        case 2: return "Hold";
        case 3: return "Release";
        case 4: return "Attack";
        case 5: return "Size";
        case 6: return "Damping";
        case 7: return "Pre-Delay";
        case 8: return "Low Cut";
        case 9: return "High Cut";
        default: return "";
    }
}

int GatedReverb::getNumParameters() const {
    return 10;
}

juce::String GatedReverb::getName() const {
    return "Gated Reverb";
}
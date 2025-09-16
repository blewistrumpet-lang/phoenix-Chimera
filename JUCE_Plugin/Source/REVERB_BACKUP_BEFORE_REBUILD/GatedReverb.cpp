// GatedReverb - Complete working implementation with proper gate control
// Classic 80s effect: Large reverb that cuts off abruptly based on input level
#include "GatedReverb.h"
#include <cmath>
#include <algorithm>
#include <memory>
#include <iostream>

class GatedReverb::Impl {
public:
    // Simple delay-based reverb (similar to ShimmerReverb but no pitch shift)
    class DelayLine {
        std::vector<float> buffer;
        int writePos = 0;
        int size = 0;
        
    public:
        void init(int delaySize) {
            size = delaySize;
            buffer.resize(size, 0.0f);
            writePos = 0;
        }
        
        void write(float sample) {
            if (size > 0) {
                buffer[writePos] = sample;
                writePos = (writePos + 1) % size;
            }
        }
        
        float read(int delaySamples) {
            if (size == 0 || delaySamples <= 0) return 0.0f;
            delaySamples = std::min(delaySamples, size - 1);
            int readPos = (writePos - delaySamples + size) % size;
            return buffer[readPos];
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
    };
    
    // Multiple delay lines for rich reverb
    static constexpr int NUM_DELAYS = 6;  // More delays for bigger sound
    DelayLine delaysL[NUM_DELAYS];
    DelayLine delaysR[NUM_DELAYS];
    
    // Damping filters
    float dampStateL[NUM_DELAYS] = {0};
    float dampStateR[NUM_DELAYS] = {0};
    
    // Gate control
    enum GateState {
        CLOSED,
        OPENING,
        OPEN,
        HOLDING,
        CLOSING
    };
    
    GateState gateState = CLOSED;
    float gateLevel = 0.0f;  // Current gate opening (0-1)
    float inputEnvelope = 0.0f;  // Envelope follower
    int holdCounter = 0;  // Samples to hold gate open
    
    // Parameters
    float threshold = 0.1f;    // Gate threshold (0-1)
    float holdTime = 0.2f;     // Hold time in seconds
    float attackTime = 0.001f; // Gate attack in seconds (1ms - very fast)
    float releaseTime = 0.05f;  // Gate release in seconds (50ms - fairly fast)
    float roomSize = 0.7f;     // Reverb size
    float damping = 0.3f;      // Damping amount
    float mix = 0.5f;          // Wet/dry mix
    
    // Derived parameters
    float feedback = 0.7f;
    float dampCoeff = 0.3f;
    float wetGain = 0.5f;
    float dryGain = 0.5f;
    float attackCoeff = 0.01f;
    float releaseCoeff = 0.001f;
    float envAttackCoeff = 0.001f;
    float envReleaseCoeff = 0.0001f;
    int holdSamples = 8820;  // 200ms at 44.1kHz
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize delay lines with different times for density
        const int delayMs[NUM_DELAYS] = {23, 29, 31, 37, 41, 43};
        
        for (int i = 0; i < NUM_DELAYS; i++) {
            int samplesL = static_cast<int>(delayMs[i] * sr / 1000.0);
            int samplesR = static_cast<int>(delayMs[i] * sr / 1000.0 * 1.05); // Stereo spread
            
            delaysL[i].init(samplesL * 2);
            delaysR[i].init(samplesR * 2);
        }
        
        updateCoefficients();
    }
    
    void reset() {
        for (int i = 0; i < NUM_DELAYS; i++) {
            delaysL[i].reset();
            delaysR[i].reset();
            dampStateL[i] = 0.0f;
            dampStateR[i] = 0.0f;
        }
        
        gateState = CLOSED;
        gateLevel = 0.0f;
        inputEnvelope = 0.0f;
        holdCounter = 0;
    }
    
    void updateCoefficients() {
        // Map parameters to DSP coefficients
        feedback = 0.6f + roomSize * 0.35f; // 0.6 to 0.95
        dampCoeff = damping * 0.5f; // 0 to 0.5
        wetGain = mix;
        dryGain = 1.0f - mix;
        
        // Gate timing coefficients
        holdSamples = static_cast<int>(holdTime * sampleRate);
        attackCoeff = 1.0f / (attackTime * sampleRate + 1.0f);
        releaseCoeff = 1.0f / (releaseTime * sampleRate + 1.0f);
        
        // Envelope follower coefficients - faster response
        envAttackCoeff = 1.0f / (0.0001f * sampleRate + 1.0f);  // 0.1ms attack (faster)
        envReleaseCoeff = 1.0f / (0.005f * sampleRate + 1.0f); // 5ms release (faster)
    }
    
    float applyDamping(float input, float& state) {
        // Simple one-pole lowpass
        state = input * (1.0f - dampCoeff) + state * dampCoeff;
        return state;
    }
    
    void updateGate(float inputLevel) {
        // Update envelope follower
        float rectified = std::abs(inputLevel);
        if (rectified > inputEnvelope) {
            inputEnvelope += (rectified - inputEnvelope) * envAttackCoeff;
        } else {
            inputEnvelope += (rectified - inputEnvelope) * envReleaseCoeff;
        }
        
        // Special case: if threshold is very low, keep gate open
        if (threshold < 0.01f) {
            gateState = OPEN;
            gateLevel = 1.0f;
            return;
        }
        
        // Gate state machine
        switch (gateState) {
            case CLOSED:
                if (inputEnvelope > threshold) {
                    gateState = OPENING;
                }
                break;
                
            case OPENING:
                gateLevel += attackCoeff;
                if (gateLevel >= 1.0f) {
                    gateLevel = 1.0f;
                    gateState = OPEN;
                    holdCounter = holdSamples;
                }
                break;
                
            case OPEN:
                if (inputEnvelope < threshold * 0.9f) { // Hysteresis
                    gateState = HOLDING;
                }
                break;
                
            case HOLDING:
                if (inputEnvelope > threshold) {
                    gateState = OPEN;
                    holdCounter = holdSamples;
                } else if (--holdCounter <= 0) {
                    gateState = CLOSING;
                }
                break;
                
            case CLOSING:
                gateLevel -= releaseCoeff;
                if (gateLevel <= 0.0f) {
                    gateLevel = 0.0f;
                    gateState = CLOSED;
                } else if (inputEnvelope > threshold) {
                    gateState = OPENING;
                }
                break;
        }
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        // Delay times in samples
        const int delayMs[NUM_DELAYS] = {23, 29, 31, 37, 41, 43};
        int delaySamples[NUM_DELAYS];
        for (int i = 0; i < NUM_DELAYS; i++) {
            delaySamples[i] = static_cast<int>(delayMs[i] * sampleRate / 1000.0);
        }
        
        for (int sample = 0; sample < numSamples; sample++) {
            float inputL = leftData[sample];
            float inputR = rightData ? rightData[sample] : inputL;
            
            // Update gate based on input level
            updateGate((inputL + inputR) * 0.5f);
            
            // Store dry
            float dryL = inputL;
            float dryR = inputR;
            
            // Process through parallel delays
            float reverbL = 0.0f;
            float reverbR = 0.0f;
            
            for (int i = 0; i < NUM_DELAYS; i++) {
                // Read delayed signal
                float delayedL = delaysL[i].read(delaySamples[i]);
                float delayedR = delaysR[i].read(delaySamples[i]);
                
                // Apply damping
                float dampedL = applyDamping(delayedL, dampStateL[i]);
                float dampedR = applyDamping(delayedR, dampStateR[i]);
                
                // Write input + feedback
                delaysL[i].write(inputL + dampedL * feedback);
                delaysR[i].write(inputR + dampedR * feedback);
                
                // Accumulate output
                reverbL += delayedL / NUM_DELAYS;
                reverbR += delayedR / NUM_DELAYS;
            }
            
            // Apply gate to reverb (not to dry signal!)
            reverbL *= gateLevel;
            reverbR *= gateLevel;
            
            // Mix wet and dry
            leftData[sample] = dryL * dryGain + reverbL * wetGain;
            if (rightData) {
                rightData[sample] = dryR * dryGain + reverbR * wetGain;
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: threshold = value * 0.5f; break;  // 0 to 0.5 range
            case 1: holdTime = 0.01f + value * 0.49f; break;  // 10ms to 500ms
            case 2: roomSize = value; break;
            case 3: damping = value; break;
            case 4: mix = value; break;
        }
        
        // Update coefficients when parameters change
        updateCoefficients();
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
        if (index < 5) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String GatedReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Threshold";
        case 1: return "Hold Time";
        case 2: return "Room Size";
        case 3: return "Damping";
        case 4: return "Mix";
        default: return "";
    }
}

int GatedReverb::getNumParameters() const {
    return 5;
}

juce::String GatedReverb::getName() const {
    return "Gated Reverb";
}
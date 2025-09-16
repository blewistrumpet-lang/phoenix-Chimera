// ShimmerReverb - Complete working implementation with proper parameter handling
#include "ShimmerReverb.h"
#include <cmath>
#include <algorithm>
#include <memory>

class ShimmerReverb::Impl {
public:
    // Simple delay-based reverb
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
        
        float readAndWrite(float input, int delaySamples) {
            float output = read(delaySamples);
            write(input);
            return output;
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
    };
    
    // Multiple delay lines for richer reverb
    static constexpr int NUM_DELAYS = 4;
    DelayLine delaysL[NUM_DELAYS];
    DelayLine delaysR[NUM_DELAYS];
    
    // Shimmer delay for pitch-shifted feedback
    DelayLine shimmerDelayL;
    DelayLine shimmerDelayR;
    
    // Damping filters
    float dampStateL[NUM_DELAYS] = {0};
    float dampStateR[NUM_DELAYS] = {0};
    
    // Simple pitch shifter state
    float pitchBufferL = 0.0f;
    float pitchBufferR = 0.0f;
    int pitchCounter = 0;
    
    // Parameters - PROPERLY INITIALIZED
    float pitchShift = 0.5f;
    float shimmerAmount = 0.3f;
    float roomSize = 0.7f;
    float damping = 0.3f;
    float mix = 0.5f;
    
    // Derived parameters
    float feedback = 0.7f;
    float dampCoeff = 0.3f;
    float wetGain = 0.5f;
    float dryGain = 0.5f;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize delay lines with prime number delays for density
        const int delayMs[NUM_DELAYS] = {29, 37, 43, 53};
        
        for (int i = 0; i < NUM_DELAYS; i++) {
            int samplesL = static_cast<int>(delayMs[i] * sr / 1000.0);
            int samplesR = static_cast<int>(delayMs[i] * sr / 1000.0 * 1.1); // Stereo spread
            
            delaysL[i].init(samplesL * 2); // Extra room for modulation
            delaysR[i].init(samplesR * 2);
        }
        
        // Shimmer delay (200ms)
        int shimmerSamples = static_cast<int>(sr * 0.2);
        shimmerDelayL.init(shimmerSamples);
        shimmerDelayR.init(shimmerSamples);
        
        updateCoefficients();
    }
    
    void reset() {
        for (int i = 0; i < NUM_DELAYS; i++) {
            delaysL[i].reset();
            delaysR[i].reset();
            dampStateL[i] = 0.0f;
            dampStateR[i] = 0.0f;
        }
        
        shimmerDelayL.reset();
        shimmerDelayR.reset();
        
        pitchBufferL = 0.0f;
        pitchBufferR = 0.0f;
        pitchCounter = 0;
    }
    
    void updateCoefficients() {
        // Map parameters to DSP coefficients
        feedback = 0.5f + roomSize * 0.45f; // 0.5 to 0.95
        dampCoeff = damping * 0.5f; // 0 to 0.5
        wetGain = mix;
        dryGain = 1.0f - mix;
    }
    
    float applyDamping(float input, float& state) {
        // Simple one-pole lowpass
        state = input * (1.0f - dampCoeff) + state * dampCoeff;
        return state;
    }
    
    // Simple octave-up effect
    float processOctaveUp(float input, float& buffer, int& counter) {
        // Double every sample (crude pitch shift up)
        if (counter % 2 == 0) {
            buffer = input;
        }
        counter++;
        return buffer;
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        // Delay times in samples
        const int delayMs[NUM_DELAYS] = {29, 37, 43, 53};
        int delaySamples[NUM_DELAYS];
        for (int i = 0; i < NUM_DELAYS; i++) {
            delaySamples[i] = static_cast<int>(delayMs[i] * sampleRate / 1000.0);
        }
        
        for (int sample = 0; sample < numSamples; sample++) {
            float inputL = leftData[sample];
            float inputR = rightData ? rightData[sample] : inputL;
            
            // Store dry
            float dryL = inputL;
            float dryR = inputR;
            
            // Read shimmer feedback
            float shimmerL = shimmerDelayL.read(static_cast<int>(sampleRate * 0.1)) * shimmerAmount * 0.5f;
            float shimmerR = shimmerDelayR.read(static_cast<int>(sampleRate * 0.1)) * shimmerAmount * 0.5f;
            
            // Mix input with shimmer
            float reverbInputL = inputL + shimmerL;
            float reverbInputR = inputR + shimmerR;
            
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
                delaysL[i].write(reverbInputL + dampedL * feedback);
                delaysR[i].write(reverbInputR + dampedR * feedback);
                
                // Accumulate output
                reverbL += delayedL * 0.25f;
                reverbR += delayedR * 0.25f;
            }
            
            // Apply shimmer effect if enabled
            if (shimmerAmount > 0.01f && pitchShift > 0.3f) {
                float shimmerOutL = processOctaveUp(reverbL, pitchBufferL, pitchCounter);
                float shimmerOutR = processOctaveUp(reverbR, pitchBufferR, pitchCounter);
                
                shimmerDelayL.write(shimmerOutL);
                shimmerDelayR.write(shimmerOutR);
            } else {
                shimmerDelayL.write(0.0f);
                shimmerDelayR.write(0.0f);
            }
            
            // Apply mix
            leftData[sample] = dryL * dryGain + reverbL * wetGain;
            if (rightData) {
                rightData[sample] = dryR * dryGain + reverbR * wetGain;
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
        
        // CRITICAL: Update coefficients when parameters change
        updateCoefficients();
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
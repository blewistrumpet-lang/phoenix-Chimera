// PlateReverb - Fixed implementation with proper parameter control
#include "PlateReverb.h"
#include <cmath>
#include <algorithm>
#include <memory>

class PlateReverb::Impl {
public:
    // Simple delay line
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
    
    // Multiple delay lines for plate simulation
    static constexpr int NUM_DELAYS = 4;
    DelayLine delaysL[NUM_DELAYS];
    DelayLine delaysR[NUM_DELAYS];
    
    // Separate damping state for each delay
    float dampStateL[NUM_DELAYS] = {0};
    float dampStateR[NUM_DELAYS] = {0};
    
    // Parameters
    float sizeParam = 0.5f;
    float dampingParam = 0.5f;
    float predelayParam = 0.0f;
    float mixParam = 0.5f;
    
    // Derived DSP parameters
    float feedback = 0.7f;
    float dampCoeff = 0.3f;
    float wetGain = 0.5f;
    float dryGain = 0.5f;
    int delaySamples[NUM_DELAYS] = {0};
    int predelaySamples = 0;
    
    // Predelay buffers
    DelayLine predelayL;
    DelayLine predelayR;
    
    double sampleRate = 44100.0;
    
    void init(double sr) {
        sampleRate = sr;
        
        // Initialize delay lines with different sizes for density
        // These are the base delay times that get scaled by size parameter
        const float baseDelayMs[NUM_DELAYS] = {37.0f, 41.0f, 43.0f, 47.0f};
        
        for (int i = 0; i < NUM_DELAYS; i++) {
            // Max delay time to allocate (150ms)
            int maxSamples = static_cast<int>(0.15f * sr);
            delaysL[i].init(maxSamples);
            delaysR[i].init(maxSamples);
        }
        
        // Initialize predelay (up to 100ms)
        int maxPredelay = static_cast<int>(0.1f * sr);
        predelayL.init(maxPredelay);
        predelayR.init(maxPredelay);
        
        updateCoefficients();
    }
    
    void reset() {
        for (int i = 0; i < NUM_DELAYS; i++) {
            delaysL[i].reset();
            delaysR[i].reset();
            dampStateL[i] = 0.0f;
            dampStateR[i] = 0.0f;
        }
        predelayL.reset();
        predelayR.reset();
    }
    
    void updateCoefficients() {
        // Map parameters to DSP coefficients
        
        // Size affects delay times and feedback
        const float baseDelayMs[NUM_DELAYS] = {37.0f, 41.0f, 43.0f, 47.0f};
        float sizeScale = 0.5f + sizeParam * 1.0f;  // 0.5x to 1.5x
        
        for (int i = 0; i < NUM_DELAYS; i++) {
            delaySamples[i] = static_cast<int>(baseDelayMs[i] * sizeScale * sampleRate / 1000.0f);
        }
        
        // Feedback increases with size for longer decay
        feedback = 0.6f + sizeParam * 0.35f;  // 0.6 to 0.95
        
        // Damping coefficient
        dampCoeff = dampingParam * 0.5f;  // 0 to 0.5
        
        // Predelay
        predelaySamples = static_cast<int>(predelayParam * 0.1f * sampleRate);  // 0 to 100ms
        
        // Mix
        wetGain = mixParam;
        dryGain = 1.0f - mixParam;
    }
    
    float applyDamping(float input, float& state) {
        // One-pole lowpass filter
        state = input * (1.0f - dampCoeff) + state * dampCoeff;
        return state;
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        for (int sample = 0; sample < numSamples; sample++) {
            float inputL = leftData[sample];
            float inputR = rightData ? rightData[sample] : inputL;
            
            // Store dry signal
            float dryL = inputL;
            float dryR = inputR;
            
            // Apply predelay if set
            if (predelaySamples > 0) {
                float predelayedL = predelayL.read(predelaySamples);
                float predelayedR = predelayR.read(predelaySamples);
                predelayL.write(inputL);
                predelayR.write(inputR);
                inputL = predelayedL;
                inputR = predelayedR;
            }
            
            // Process through parallel delays
            float reverbL = 0.0f;
            float reverbR = 0.0f;
            
            for (int i = 0; i < NUM_DELAYS; i++) {
                // Read delayed signal
                float delayedL = delaysL[i].read(delaySamples[i]);
                float delayedR = delaysR[i].read(delaySamples[i] + i + 1);  // Slight stereo offset
                
                // Apply damping (separate state for each delay)
                float dampedL = applyDamping(delayedL, dampStateL[i]);
                float dampedR = applyDamping(delayedR, dampStateR[i]);
                
                // Write input + feedback
                delaysL[i].write(inputL + dampedL * feedback);
                delaysR[i].write(inputR + dampedR * feedback);
                
                // Accumulate output
                reverbL += delayedL / NUM_DELAYS;
                reverbR += delayedR / NUM_DELAYS;
            }
            
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
            case 0: sizeParam = value; break;
            case 1: dampingParam = value; break;
            case 2: predelayParam = value; break;
            case 3: mixParam = value; break;
        }
        
        // Update DSP coefficients when parameters change
        updateCoefficients();
    }
};

// Public interface
PlateReverb::PlateReverb() : pImpl(std::make_unique<Impl>()) {}
PlateReverb::~PlateReverb() = default;

void PlateReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->init(sampleRate);
}

void PlateReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void PlateReverb::reset() {
    pImpl->reset();
}

void PlateReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index < 4) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String PlateReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Size";
        case 1: return "Damping";
        case 2: return "Predelay";
        case 3: return "Mix";
        default: return "";
    }
}

int PlateReverb::getNumParameters() const {
    return 4;
}

juce::String PlateReverb::getName() const {
    return "Plate Reverb";
}
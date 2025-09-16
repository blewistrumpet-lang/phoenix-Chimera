// SpringReverb - Complete rebuild with proven working implementation
// Uses cascaded allpass filters for dispersion and modulated delays for spring character
#include "SpringReverb.h"
#include <cmath>
#include <algorithm>

// Simple but WORKING allpass filter
class SimpleAllpass {
    std::vector<float> buffer;
    int writePos = 0;
    int size = 0;
    
public:
    void init(int delaySize) {
        size = delaySize;
        buffer.resize(size, 0.0f);
        writePos = 0;
    }
    
    float process(float input, float coefficient) {
        if (size == 0) return input;
        
        // Read delayed sample
        int readPos = (writePos + 1) % size;  // Oldest sample
        float delayed = buffer[readPos];
        
        // Allpass difference equation: y = -g*x + x[-D] + g*y[-D]
        float output = -coefficient * input + delayed;
        
        // Store input + g*output for next time
        buffer[writePos] = input + coefficient * output;
        
        // Advance write position
        writePos = (writePos + 1) % size;
        
        return output;
    }
    
    void reset() {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }
};

// Simple delay line for feedback path
class SimpleDelay {
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

// Main implementation
class SpringReverb::Impl {
public:
    // DSP components
    static constexpr int NUM_ALLPASS = 4;
    SimpleAllpass allpassL[NUM_ALLPASS];
    SimpleAllpass allpassR[NUM_ALLPASS];
    SimpleDelay delayL, delayR;
    
    // Damping filter state
    float dampingStateL = 0.0f;
    float dampingStateR = 0.0f;
    
    // Chirp generator state (separate for L/R)
    float chirpEnvelopeL = 0.0f;
    float chirpEnvelopeR = 0.0f;
    float chirpPhaseL = 0.0f;
    float chirpPhaseR = 0.0f;
    bool chirpActiveL = false;
    bool chirpActiveR = false;
    
    // Parameters (0-1 normalized)
    float tensionParam = 0.5f;
    float dampingParam = 0.5f;
    float decayParam = 0.5f;
    float mixParam = 0.5f;
    
    // DSP coefficients (computed from parameters)
    float allpassCoeffs[NUM_ALLPASS] = {-0.7f, -0.6f, -0.5f, -0.4f};
    int delayTime = 2000;  // in samples
    float dampingCutoff = 0.5f;  // 0-1 normalized frequency
    float feedbackGain = 0.5f;
    float wetGain = 0.5f;
    float dryGain = 0.5f;
    
    double sampleRate = 44100.0;
    
    void setSampleRate(double sr) {
        sampleRate = sr;
        
        // Initialize allpass filters with fixed delay times
        // These create the dispersive character of the spring
        int apDelays[NUM_ALLPASS] = {
            static_cast<int>(sr * 0.002),  // 2ms
            static_cast<int>(sr * 0.005),  // 5ms
            static_cast<int>(sr * 0.008),  // 8ms
            static_cast<int>(sr * 0.013)   // 13ms
        };
        
        for (int i = 0; i < NUM_ALLPASS; i++) {
            allpassL[i].init(apDelays[i]);
            allpassR[i].init(apDelays[i] + i + 1);  // Slight stereo offset
        }
        
        // Initialize main delay lines
        int maxDelay = static_cast<int>(sr * 0.15);  // 150ms max
        delayL.init(maxDelay);
        delayR.init(maxDelay);
        
        // Update DSP coefficients from current parameters
        updateCoefficients();
    }
    
    void reset() {
        for (int i = 0; i < NUM_ALLPASS; i++) {
            allpassL[i].reset();
            allpassR[i].reset();
        }
        delayL.reset();
        delayR.reset();
        dampingStateL = 0.0f;
        dampingStateR = 0.0f;
        chirpEnvelopeL = 0.0f;
        chirpEnvelopeR = 0.0f;
        chirpPhaseL = 0.0f;
        chirpPhaseR = 0.0f;
        chirpActiveL = false;
        chirpActiveR = false;
    }
    
    void updateCoefficients() {
        // Map normalized parameters to DSP coefficients
        
        // Tension affects allpass coefficients (more tension = less dispersion)
        float tensionScale = 0.3f + tensionParam * 0.5f;  // 0.3 to 0.8
        allpassCoeffs[0] = -0.7f * tensionScale;
        allpassCoeffs[1] = -0.6f * tensionScale;
        allpassCoeffs[2] = -0.5f * tensionScale;
        allpassCoeffs[3] = -0.4f * tensionScale;
        
        // Tension also affects delay time (tighter spring = shorter delay)
        delayTime = static_cast<int>(sampleRate * (0.02f + (1.0f - tensionParam) * 0.08f));
        
        // Damping controls the lowpass filter cutoff
        // 0 = no damping (bright), 1 = max damping (dark)
        dampingCutoff = 1.0f - dampingParam * 0.9f;  // 0.1 to 1.0
        
        // Decay controls feedback amount
        // Map 0-1 to 0.3-0.85 for stable feedback range
        feedbackGain = 0.3f + decayParam * 0.55f;
        
        // Mix controls wet/dry balance
        wetGain = mixParam;
        dryGain = 1.0f - mixParam;
    }
    
    float processChirp(float input, float& envelope, float& phase, bool& active) {
        // Simple transient detector for "boing" effect
        float rectified = std::abs(input);
        float attackEnv = rectified - envelope;
        
        // Detect attack transient
        if (attackEnv > 0.1f && !active) {
            active = true;
            phase = 0.0f;
        }
        
        // Update envelope follower
        float envCoeff = rectified > envelope ? 0.01f : 0.0001f;
        envelope += (rectified - envelope) * envCoeff;
        
        // Generate chirp if active
        float chirp = 0.0f;
        if (active) {
            // Frequency sweep from 200Hz to 2kHz
            float freq = 200.0f + phase * 1800.0f;
            chirp = std::sin(2.0f * M_PI * freq * phase) * (1.0f - phase);
            
            phase += 1.0f / (sampleRate * 0.015f);  // 15ms chirp
            if (phase >= 1.0f) {
                active = false;
            }
        }
        
        return input + chirp * 0.2f * tensionParam;  // More tension = more "boing"
    }
    
    float processDamping(float input, float& state) {
        // One-pole lowpass filter with proper damping
        // dampingCutoff: 1.0 = no damping (bright), 0.1 = max damping (dark)
        float output = input * dampingCutoff + state * (1.0f - dampingCutoff);
        state = output;
        return output;
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        float* leftData = buffer.getWritePointer(0);
        float* rightData = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        
        for (int sample = 0; sample < numSamples; sample++) {
            // Get input
            float inputL = leftData[sample];
            float inputR = rightData ? rightData[sample] : inputL;
            
            // Store dry signal
            float dryL = inputL;
            float dryR = inputR;
            
            // Add chirp for spring character (separate states for L/R)
            float chirpedL = processChirp(inputL, chirpEnvelopeL, chirpPhaseL, chirpActiveL);
            float chirpedR = processChirp(inputR, chirpEnvelopeR, chirpPhaseR, chirpActiveR);
            
            // Read from delay lines (feedback)
            float delayedL = delayL.read(delayTime);
            float delayedR = delayR.read(delayTime + 17);  // Slight stereo offset
            
            // Mix input with feedback
            float reverbL = chirpedL + delayedL * feedbackGain;
            float reverbR = chirpedR + delayedR * feedbackGain;
            
            // Process through allpass cascade for dispersion
            for (int i = 0; i < NUM_ALLPASS; i++) {
                reverbL = allpassL[i].process(reverbL, allpassCoeffs[i]);
                reverbR = allpassR[i].process(reverbR, allpassCoeffs[i]);
            }
            
            // Apply damping
            reverbL = processDamping(reverbL, dampingStateL);
            reverbR = processDamping(reverbR, dampingStateR);
            
            // Write to delay lines
            delayL.write(reverbL);
            delayR.write(reverbR);
            
            // Mix wet and dry signals
            leftData[sample] = dryL * dryGain + reverbL * wetGain;
            if (rightData) {
                rightData[sample] = dryR * dryGain + reverbR * wetGain;
            }
        }
    }
    
    void setParameter(int index, float value) {
        value = std::clamp(value, 0.0f, 1.0f);
        
        switch (index) {
            case 0: tensionParam = value; break;
            case 1: dampingParam = value; break;
            case 2: decayParam = value; break;
            case 3: mixParam = value; break;
        }
        
        // CRITICAL: Update DSP coefficients whenever parameters change!
        updateCoefficients();
    }
};

// Public interface
SpringReverb::SpringReverb() : pImpl(std::make_unique<Impl>()) {}
SpringReverb::~SpringReverb() = default;

void SpringReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->setSampleRate(sampleRate);
}

void SpringReverb::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void SpringReverb::reset() {
    pImpl->reset();
}

void SpringReverb::updateParameters(const std::map<int, float>& params) {
    // Process each parameter update
    for (const auto& [index, value] : params) {
        if (index < 4) {
            pImpl->setParameter(index, value);
        }
    }
}

juce::String SpringReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Tension";
        case 1: return "Damping";
        case 2: return "Decay";
        case 3: return "Mix";
        default: return "";
    }
}

int SpringReverb::getNumParameters() const {
    return 4;
}

juce::String SpringReverb::getName() const {
    return "Spring Reverb";
}
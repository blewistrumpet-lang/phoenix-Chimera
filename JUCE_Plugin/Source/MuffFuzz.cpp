#include "MuffFuzz.h"
#include <cmath>

MuffFuzz::MuffFuzz() : m_rng(std::random_device{}()) {
    // Initialize smoothed parameters with boutique defaults
    m_sustain.setImmediate(0.7f);
    m_tone.setImmediate(0.5f);
    m_volume.setImmediate(0.5f);
    m_gate.setImmediate(0.1f);
    m_mids.setImmediate(0.3f);      // Default mid scoop
    m_fuzzType.setImmediate(0.0f);  // Silicon transistor default
    
    // Set smoothing rates for different characteristics
    m_sustain.setSmoothingRate(0.99f);   // Fast for sustain changes
    m_tone.setSmoothingRate(0.995f);
    m_volume.setSmoothingRate(0.99f);
    m_gate.setSmoothingRate(0.98f);      // Slower for gate to avoid clicks
    m_mids.setSmoothingRate(0.995f);
    m_fuzzType.setSmoothingRate(0.97f);  // Slowest for mode changes
}

void MuffFuzz::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Reset all enhanced filter states
    for (auto& state : m_channelStates) {
        state.reset();
        
        // Initialize filter coefficients
        state.inputHighpass.setBandpass(80.0, 0.7, sampleRate);  // Remove DC and subsonic
        state.inputLowShelf.setLowShelf(200.0, -3.0, 0.7, sampleRate);  // Shape low-end
        state.midScoop.setNotch(800.0, 2.0, sampleRate);         // Mid scoop character
        state.toneFilter.setHighShelf(2000.0, 0.0, 0.7, sampleRate);  // Tone control
        state.presenceFilter.setHighShelf(5000.0, 0.0, 0.5, sampleRate);  // Presence
    }

void MuffFuzz::reset() {
    // Reset distortion state
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
}

    
    // Reset DC blockers
    for (auto& blocker : m_inputDCBlockers) {
        blocker.reset();
    }
    for (auto& blocker : m_outputDCBlockers) {
        blocker.reset();
    }
    
    // Prepare oversampler
    m_oversampler.prepare(samplesPerBlock);
    
    // Reset aging and thermal
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void MuffFuzz::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update filter coefficients based on parameters
    for (auto& state : m_channelStates) {
        // Input stage filters (creates the characteristic mid-scoop)
        state.inputStage1.setLowShelf(200.0f, -6.0f + m_sustain * 12.0f, m_sampleRate);
        state.inputStage2.setHighShelf(2000.0f, -3.0f + m_sustain * 6.0f, m_sampleRate);
        
        // Tone control (variable high-frequency response)
        float toneFreq = 500.0f + m_tone * 3500.0f;
        state.toneFilter.setHighShelf(toneFreq, -10.0f + m_tone * 20.0f, m_sampleRate);
    }
    
    for (int channel = 0; channel < numChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        auto& state = m_channelStates[channel % m_channelStates.size()];
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            
            // Gate (noise reduction)
            float gated = processGate(input, state.envelope, m_gate * 0.1f);
            
            // Input filtering stages
            float filtered = state.inputStage1.process(gated);
            filtered = state.inputStage2.process(filtered);
            
            // Sustain (gain) stage
            float gained = filtered * (1.0f + m_sustain * 100.0f);
            
            // Diode clipping simulation
            float clipped = processDiodeClipping(gained, 0.7f);
            
            // Tone control
            float toned = state.toneFilter.process(clipped);
            
            // Output volume
            channelData[sample] = toned * m_volume * 0.5f;
        }
    }
}

void MuffFuzz::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Update smoothed parameters
    m_sustain.target = getParam(0, 0.7f);
    m_tone.target = getParam(1, 0.5f);
    m_volume.target = getParam(2, 0.5f);
    m_gate.target = getParam(3, 0.1f);
    m_mids.target = getParam(4, 0.3f);     // Mid scoop control
    m_fuzzType.target = getParam(5, 0.0f); // Modern fuzz variations
}

juce::String MuffFuzz::getParameterName(int index) const {
    switch (index) {
        case 0: return "Sustain";
        case 1: return "Tone";
        case 2: return "Volume";
        case 3: return "Gate";
        case 4: return "Mids";
        case 5: return "Type";
        default: return "";
    }
}

float MuffFuzz::BiquadFilter::process(float input) {
    float output = b0 * input + b1 * z1 + b2 * z2 - a1 * z1 - a2 * z2;
    z2 = z1;
    z1 = output;
    return output;
}

void MuffFuzz::BiquadFilter::setLowShelf(float freq, float gain, float sampleRate) {
    float A = std::pow(10.0f, gain / 40.0f);
    float w = 2.0f * M_PI * freq / sampleRate;
    float cosw = std::cos(w);
    float sinw = std::sin(w);
    float alpha = sinw / std::sqrt(2.0f);
    
    float ap1 = A + 1.0f;
    float am1 = A - 1.0f;
    float ap1_cosw = ap1 * cosw;
    float am1_cosw = am1 * cosw;
    float sqrt_2A_alpha = std::sqrt(2.0f * A) * alpha;
    
    b0 = A * (ap1 - am1_cosw + sqrt_2A_alpha);
    b1 = 2.0f * A * (am1 - ap1_cosw);
    b2 = A * (ap1 - am1_cosw - sqrt_2A_alpha);
    float a0 = ap1 + am1_cosw + sqrt_2A_alpha;
    a1 = -2.0f * (am1 + ap1_cosw) / a0;
    a2 = (ap1 + am1_cosw - sqrt_2A_alpha) / a0;
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
}

void MuffFuzz::BiquadFilter::setHighShelf(float freq, float gain, float sampleRate) {
    float A = std::pow(10.0f, gain / 40.0f);
    float w = 2.0f * M_PI * freq / sampleRate;
    float cosw = std::cos(w);
    float sinw = std::sin(w);
    float alpha = sinw / std::sqrt(2.0f);
    
    float ap1 = A + 1.0f;
    float am1 = A - 1.0f;
    float ap1_cosw = ap1 * cosw;
    float am1_cosw = am1 * cosw;
    float sqrt_2A_alpha = std::sqrt(2.0f * A) * alpha;
    
    b0 = A * (ap1 + am1_cosw + sqrt_2A_alpha);
    b1 = -2.0f * A * (am1 + ap1_cosw);
    b2 = A * (ap1 + am1_cosw - sqrt_2A_alpha);
    float a0 = ap1 - am1_cosw + sqrt_2A_alpha;
    a1 = 2.0f * (am1 - ap1_cosw) / a0;
    a2 = (ap1 - am1_cosw - sqrt_2A_alpha) / a0;
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
}

float MuffFuzz::processDiodeClipping(float x, float threshold) {
    // Asymmetric diode clipping simulation
    if (x > 0) {
        // Positive side - softer clipping
        if (x > threshold) {
            return threshold + (x - threshold) / (1.0f + std::pow((x - threshold) / threshold, 2.0f));
        }
    } else {
        // Negative side - harder clipping
        float negThreshold = -threshold * 0.9f;
        if (x < negThreshold) {
            return negThreshold + (x - negThreshold) / (1.0f + std::pow((x - negThreshold) / negThreshold, 4.0f));
        }
    }
    return x;
}

float MuffFuzz::processGate(float input, float& envelope, float threshold) {
    // Simple envelope follower for gating
    float inputLevel = std::abs(input);
    float attack = 0.001f;
    float release = 0.01f;
    
    if (inputLevel > envelope) {
        envelope += (inputLevel - envelope) * attack;
    } else {
        envelope += (inputLevel - envelope) * release;
    }
    
    if (envelope < threshold) {
        return input * (envelope / threshold);
    }
    return input;
}
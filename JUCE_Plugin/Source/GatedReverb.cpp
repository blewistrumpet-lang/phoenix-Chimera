#include "GatedReverb.h"
#include <cmath>

GatedReverb::GatedReverb() : m_rng(std::random_device{}()) {
    // Initialize smooth parameters
    m_roomSize.setImmediate(0.5f);
    m_gateTime.setImmediate(0.3f);
    m_threshold.setImmediate(0.3f);
    m_predelay.setImmediate(0.1f);
    m_damping.setImmediate(0.5f);
    m_gateShape.setImmediate(0.5f);
    m_brightness.setImmediate(0.5f);
    m_mix.setImmediate(0.5f);
    
    // Set smoothing rates
    m_roomSize.setSmoothingRate(0.999f);
    m_gateTime.setSmoothingRate(0.995f);
    m_threshold.setSmoothingRate(0.99f);
    m_predelay.setSmoothingRate(0.999f);
    m_damping.setSmoothingRate(0.999f);
    m_gateShape.setSmoothingRate(0.995f);
    m_brightness.setSmoothingRate(0.999f);
    m_mix.setSmoothingRate(0.999f);
}

void GatedReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Comb filter tunings (based on classic reverb algorithms)
    const int combTunings[8] = {1557, 1617, 1491, 1422, 1277, 1356, 1188, 1116};
    
    // All-pass tunings
    const int allpassTunings[4] = {225, 341, 441, 556};
    
    for (auto& channel : m_channelStates) {
        // Initialize comb filters
        for (int i = 0; i < 8; ++i) {
            int size = static_cast<int>(combTunings[i] * sampleRate / 44100.0);
            channel.combFilters[i].setSize(size);
            channel.combFilters[i].feedback = 0.84f;
            channel.combFilters[i].damping = m_damping.current;
        }
        
        // Initialize all-pass filters
        for (int i = 0; i < 4; ++i) {
            int size = static_cast<int>(allpassTunings[i] * sampleRate / 44100.0);
            channel.allpassFilters[i].setSize(size);
            channel.allpassFilters[i].feedback = 0.5f;
        }
        
        // Initialize early reflections
        channel.earlyReflections.prepare(sampleRate);
        
        // Initialize pre-delay
        int predelaySize = static_cast<int>(0.1 * sampleRate); // Max 100ms
        channel.predelayBuffer.resize(predelaySize);
        std::fill(channel.predelayBuffer.begin(), channel.predelayBuffer.end(), 0.0f);
        channel.predelayIndex = 0;
        
        // Initialize gate
        channel.gate.holdTime = static_cast<int>(m_gateTime.current * sampleRate);
        channel.gate.level = 0.0f;
        channel.envelopeFollower = 0.0f;
        
        channel.shelfState = 0.0f;
        channel.componentDrift = 0.0f;
        channel.thermalFactor = 1.0f;
    }
    
    // Initialize DC blockers
    for (auto& blocker : m_inputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    for (auto& blocker : m_outputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void GatedReverb::reset() {
    // Clear all reverb buffers
    for (auto& channel : m_channelStates) {
        // Reset comb filters
        for (auto& comb : channel.combFilters) {
            std::fill(comb.buffer.begin(), comb.buffer.end(), 0.0f);
        }
        // Reset allpass filters
        for (auto& allpass : channel.allpassFilters) {
            std::fill(allpass.buffer.begin(), allpass.buffer.end(), 0.0f);
        }
        // Reset predelay
        std::fill(channel.predelayBuffer.begin(), channel.predelayBuffer.end(), 0.0f);
        channel.predelayIndex = 0;
    }
    // Reset any additional reverb state
}

void GatedReverb::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_roomSize.update();
    m_gateTime.update();
    m_threshold.update();
    m_predelay.update();
    m_damping.update();
    m_gateShape.update();
    m_brightness.update();
    m_mix.update();
    
    // Update thermal model periodically
    m_sampleCount += numSamples;
    if (m_sampleCount >= static_cast<int>(m_sampleRate * 0.1)) { // Every 100ms
        m_thermalModel.update(m_sampleRate);
        m_componentAge += 0.0001f; // Slow aging
        m_sampleCount = 0;
    }
    
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Calculate room size scaling
    float roomScale = 0.4f + m_roomSize.current * 0.6f;
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Apply input DC blocking
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = m_inputDCBlockers[channel].process(channelData[sample]);
        }
        
        // Update component aging for this channel
        state.componentDrift += (m_distribution(m_rng) * 0.00001f) * m_componentAge;
        state.componentDrift = std::max(-0.01f, std::min(0.01f, state.componentDrift));
        state.thermalFactor = thermalFactor * (1.0f + state.componentDrift);
        
        // Update gate hold time with thermal effects
        state.gate.holdTime = static_cast<int>(m_gateTime.current * m_sampleRate * state.thermalFactor);
        
        // Update damping with thermal drift
        float thermalDamping = m_damping.current * 0.4f * state.thermalFactor;
        for (auto& comb : state.combFilters) {
            comb.damping = thermalDamping;
        }
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // Envelope follower for gate
            float envelope = std::abs(input);
            float attackTime = 0.001f;
            float releaseTime = 0.01f;
            
            if (envelope > state.envelopeFollower) {
                state.envelopeFollower += (envelope - state.envelopeFollower) * attackTime;
            } else {
                state.envelopeFollower += (envelope - state.envelopeFollower) * releaseTime;
            }
            
            // Gate decision with thermal drift
            bool gateOpen = state.envelopeFollower > (m_threshold.current * state.thermalFactor);
            float gateLevel = state.gate.process(gateOpen, m_gateShape.current);
            
            // Pre-delay with thermal effects
            int predelayTime = static_cast<int>(m_predelay.current * 0.1f * m_sampleRate * state.thermalFactor);
            int readIndex = (state.predelayIndex - predelayTime + state.predelayBuffer.size()) % 
                          state.predelayBuffer.size();
            float delayedInput = state.predelayBuffer[readIndex];
            state.predelayBuffer[state.predelayIndex] = input;
            state.predelayIndex = (state.predelayIndex + 1) % state.predelayBuffer.size();
            
            // Early reflections
            float early = state.earlyReflections.process(delayedInput);
            
            // Feed into parallel comb filters
            float reverbSum = 0.0f;
            for (int i = 0; i < 8; ++i) {
                // Apply room size by scaling feedback
                state.combFilters[i].feedback = 0.84f * roomScale;
                reverbSum += state.combFilters[i].process(delayedInput + early * 0.3f);
            }
            reverbSum *= 0.125f; // Average
            
            // Series all-pass filters for diffusion
            float diffused = reverbSum;
            for (auto& allpass : state.allpassFilters) {
                diffused = allpass.process(diffused);
            }
            
            // Apply brightness control with aging
            if (m_brightness.current != 0.5f) {
                float shelfFreq = 2000.0f + m_brightness.current * 6000.0f * state.thermalFactor;
                float shelfGain = 0.5f + m_brightness.current;
                diffused = processHighShelfWithAging(diffused, state.shelfState, 
                                          shelfFreq / m_sampleRate, shelfGain, m_componentAge);
            }
            
            // Apply gate envelope
            float gatedReverb = diffused * gateLevel;
            
            // Apply soft clipping for analog warmth
            gatedReverb = softClipWithAging(gatedReverb, m_componentAge);
            
            // Mix with dry signal
            channelData[sample] = drySignal * (1.0f - m_mix.current) + gatedReverb * m_mix.current;
        }
        
        // Apply output DC blocking
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = m_outputDCBlockers[channel].process(channelData[sample]);
        }
    }
}

float GatedReverb::processHighShelf(float input, float& state, float frequency, float gain) {
    float w = 2.0f * std::sin(M_PI * frequency);
    float a = (gain - 1.0f) * 0.5f;
    
    float highpass = input - state;
    state += highpass * w;
    
    return input + highpass * a;
}

void GatedReverb::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_roomSize.target = params.at(0);
    if (params.count(1)) m_gateTime.target = params.at(1); // 0-1 second
    if (params.count(2)) m_threshold.target = params.at(2) * 0.5f; // More sensitive range
    if (params.count(3)) m_predelay.target = params.at(3);
    if (params.count(4)) m_damping.target = params.at(4);
    if (params.count(5)) m_gateShape.target = params.at(5);
    if (params.count(6)) m_brightness.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String GatedReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Room Size";
        case 1: return "Gate Time";
        case 2: return "Threshold";
        case 3: return "Pre-Delay";
        case 4: return "Damping";
        case 5: return "Gate Shape";
        case 6: return "Brightness";
        case 7: return "Mix";
        default: return "";
    }
}

float GatedReverb::processHighShelfWithAging(float input, float& state, float frequency, float gain, float aging) {
    // Apply aging effects - frequency drift and slight gain variation
    float agingFactor = 1.0f + aging * 0.1f;
    float agingGain = gain * agingFactor;
    
    float w = 2.0f * std::sin(M_PI * frequency * agingFactor);
    float a = (agingGain - 1.0f) * 0.5f;
    
    float highpass = input - state;
    state += highpass * w;
    
    // Add slight nonlinearity due to aging
    float output = input + highpass * a;
    if (aging > 0.01f) {
        output += aging * 0.01f * std::tanh(output * 5.0f);
    }
    
    return output;
}

float GatedReverb::softClip(float input) {
    // Soft clipping using tanh for analog warmth
    return std::tanh(input * 0.7f);
}

float GatedReverb::softClipWithAging(float input, float aging) {
    // Apply aging effects - increased saturation and slight asymmetry
    float agingFactor = 1.0f + aging * 0.2f;
    float asymmetry = aging * 0.1f;
    
    // Asymmetric soft clipping
    if (input > 0.0f) {
        return std::tanh(input * 0.7f * agingFactor);
    } else {
        return std::tanh(input * 0.7f * agingFactor * (1.0f + asymmetry));
    }
}

void GatedReverb::updateCombFiltersWithThermal(ChannelState& state, float thermalFactor) {
    // Update comb filter parameters with thermal drift
    for (auto& comb : state.combFilters) {
        comb.feedback *= thermalFactor;
        comb.damping *= thermalFactor;
    }
}

void GatedReverb::updateAllPassFiltersWithThermal(ChannelState& state, float thermalFactor) {
    // Update all-pass filter parameters with thermal drift
    for (auto& allpass : state.allpassFilters) {
        allpass.feedback *= thermalFactor;
    }
}
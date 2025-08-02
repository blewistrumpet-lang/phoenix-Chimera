#include "TransientShaper.h"
#include <cmath>
#include <algorithm>

TransientShaper::TransientShaper() {
    // Initialize smoothed parameters with proper defaults
    m_attack.reset(0.5f);
    m_sustain.reset(0.5f);
    m_sensitivity.reset(0.5f);
    m_speed.reset(0.5f);
    m_clipper.reset(0.0f);
    m_punchMode.reset(0.0f);
    m_stereoLink.reset(1.0f);
    m_mix.reset(1.0f);
}

void TransientShaper::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    float fastSmoothingTime = 50.0f; // 50ms for responsive parameters
    float slowSmoothingTime = 100.0f; // 100ms for slower parameters
    
    m_attack.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_sustain.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_sensitivity.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_speed.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_clipper.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_punchMode.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_stereoLink.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_mix.setSmoothingTime(fastSmoothingTime, sampleRate);
    
    m_channelStates.clear();
    m_channelStates.resize(2); // Stereo
    
    for (auto& state : m_channelStates) {
        // Initialize envelope followers
        state.signalEnvelope.envelope = 0.0f;
        state.signalEnvelope.peakEnvelope = 0.0f;
        state.transientEnvelope.envelope = 0.0f;
        state.transientEnvelope.peakEnvelope = 0.0f;
        
        // Initialize processing state
        state.lastSample = 0.0f;
        state.smoothedDiff = 0.0f;
        state.gain = 1.0f;
        state.lastGain = 1.0f;
        
        // Initialize boutique components
        state.inputDCBlocker.reset();
        state.outputDCBlocker.reset();
        state.thermalModel = ThermalModel();
        state.componentAging = ComponentAging();
        state.oversampler.prepare(sampleRate);
        
        // Initialize ZDF highpass filter (20Hz cutoff)
        state.zdfHighpass.setCutoff(20.0f, sampleRate);
        
        // Initialize lookahead buffer
        state.lookaheadIndex = 0;
        state.lookaheadBuffer.fill(0.0f);
    }

void TransientShaper::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for TransientShaper
}

}

void TransientShaper::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    m_attack.update();
    m_sustain.update();
    m_sensitivity.update();
    m_speed.update();
    m_clipper.update();
    m_punchMode.update();
    m_stereoLink.update();
    m_mix.update();
    
    // Update envelope speeds based on smoothed speed parameter
    for (auto& state : m_channelStates) {
        state.signalEnvelope.setSpeed(m_speed.current);
        state.transientEnvelope.setSpeed(m_speed.current * 2.0f); // Faster for transients
        
        // Update thermal and aging models
        float processingLoad = std::min(1.0f, numSamples / 512.0f); // Normalize load
        state.thermalModel.update(processingLoad);
        state.componentAging.update();
    }
    
    // Process stereo linked if enabled (using smoothed parameter)
    float linkedTransientLevel = 0.0f;
    float linkedSustainLevel = 0.0f;
    
    if (m_stereoLink.current > 0.5f && numChannels == 2) {
        // Pre-scan for linked detection
        for (int ch = 0; ch < numChannels; ++ch) {
            auto& state = m_channelStates[ch];
            const float* channelData = buffer.getReadPointer(ch);
            
            for (int i = 0; i < numSamples; ++i) {
                float input = channelData[i];
                float env = state.signalEnvelope.process(input);
                linkedSustainLevel = std::max(linkedSustainLevel, env);
                
                // High-pass filter for transient detection
                float highpassed = input - state.highpassState;
                state.highpassState += highpassed * 0.95f;
                
                float transientEnv = state.transientEnvelope.process(highpassed);
                linkedTransientLevel = std::max(linkedTransientLevel, transientEnv);
            }
        }
        
        // Reset envelope states for actual processing
        for (auto& state : m_channelStates) {
            state.signalEnvelope.envelope *= 0.95f;
            state.transientEnvelope.envelope *= 0.95f;
            state.highpassState *= 0.95f;
        }
    }
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= m_channelStates.size()) continue;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // Store in lookahead buffer
            float delayedInput = state.lookaheadBuffer[state.lookaheadIndex];
            state.lookaheadBuffer[state.lookaheadIndex] = input;
            state.lookaheadIndex = (state.lookaheadIndex + 1) % ChannelState::LOOKAHEAD_SIZE;
            
            // Envelope following
            float sustainLevel = state.signalEnvelope.process(input);
            
            // High-pass filter for transient isolation (20Hz)
            float cutoff = 20.0f / m_sampleRate;
            float highpassed = input - state.highpassState;
            state.highpassState += highpassed * (1.0f - cutoff);
            
            // Transient detection using derivative
            float diff = std::abs(highpassed) - std::abs(state.lastSample);
            state.lastSample = highpassed;
            
            // Smooth the derivative for stability
            float smoothingFactor = 0.95f - m_sensitivity.current * 0.5f;
            state.smoothedDiff = state.smoothedDiff * smoothingFactor + diff * (1.0f - smoothingFactor);
            
            // Enhanced transient envelope
            float transientLevel = state.transientEnvelope.process(
                std::max(0.0f, state.smoothedDiff * (1.0f + m_sensitivity.current * 4.0f))
            );
            
            // Use linked levels if stereo link is enabled
            if (m_stereoLink.current > 0.5f && numChannels == 2) {
                transientLevel = linkedTransientLevel;
                sustainLevel = linkedSustainLevel;
            }
            
            // Calculate gain adjustment
            float gain = calculateTransientGain(transientLevel, sustainLevel);
            
            // Apply punch mode if enabled
            if (m_punchMode.current > 0.0f) {
                gain = processPunchMode(gain, transientLevel);
            }
            
            // Smooth gain changes to prevent clicks
            float gainSmoothing = 0.995f;
            state.gain = state.gain * gainSmoothing + gain * (1.0f - gainSmoothing);
            
            // Apply gain to delayed signal (compensate for lookahead)
            float output = delayedInput * state.gain;
            
            // Optional soft clipping
            if (m_clipper.current > 0.0f) {
                output = softClip(output, 1.0f - m_clipper.current * 0.5f);
            }
            
            // Mix with dry signal
            channelData[sample] = drySignal * (1.0f - m_mix.current) + output * m_mix.current;
        }
    }
}

float TransientShaper::calculateTransientGain(float transientLevel, float sustainLevel) {
    // Separate attack and sustain processing
    float attackGain = 1.0f;
    float sustainGain = 1.0f;
    
    // Attack shaping (-2 to +2 range) using smoothed parameter
    float attackAmount = (m_attack.current - 0.5f) * 4.0f;
    if (attackAmount > 0.0f) {
        // Boost attacks
        attackGain = 1.0f + transientLevel * attackAmount;
    } else {
        // Reduce attacks
        attackGain = 1.0f + transientLevel * attackAmount * 0.5f;
    }
    
    // Sustain shaping (-2 to +2 range) using smoothed parameter
    float sustainAmount = (m_sustain.current - 0.5f) * 4.0f;
    if (sustainAmount > 0.0f) {
        // Boost sustain
        sustainGain = 1.0f + sustainLevel * sustainAmount * 0.5f;
    } else {
        // Reduce sustain
        sustainGain = 1.0f + sustainLevel * sustainAmount;
    }
    
    // Combine gains with priority on transients
    float combinedGain = attackGain * sustainGain;
    
    // Prevent extreme gain values
    combinedGain = std::max(0.1f, std::min(5.0f, combinedGain));
    
    return combinedGain;
}

float TransientShaper::processPunchMode(float gain, float transientLevel) {
    // Punch mode adds extra snap to transients (using smoothed parameter)
    if (transientLevel > 0.1f) {
        float punchBoost = 1.0f + m_punchMode.current * transientLevel * 2.0f;
        gain *= punchBoost;
        
        // Add slight compression after the punch with analog-style nonlinearity
        float compressionDelay = transientLevel * 0.5f;
        float compressionGain = 1.0f - compressionDelay * m_punchMode.current * 0.3f;
        
        // Add subtle saturation curve for analog punch character
        float saturation = std::tanh(gain * 0.8f) * 1.25f;
        gain = gain * (1.0f - m_punchMode.current * 0.3f) + saturation * m_punchMode.current * 0.3f;
        
        gain *= compressionGain;
    }
    
    return gain;
}

float TransientShaper::softClip(float input, float threshold) {
    float absInput = std::abs(input);
    
    if (absInput <= threshold) {
        return input;
    }
    
    float sign = input > 0.0f ? 1.0f : -1.0f;
    float excess = absInput - threshold;
    float clipped = threshold + std::tanh(excess * 2.0f) * (1.0f - threshold);
    
    return sign * clipped;
}

void TransientShaper::updateParameters(const std::map<int, float>& params) {
    // Update target values for smoothed parameters
    if (params.count(0)) m_attack.target = params.at(0);
    if (params.count(1)) m_sustain.target = params.at(1);
    if (params.count(2)) m_sensitivity.target = params.at(2);
    if (params.count(3)) m_speed.target = params.at(3);
    if (params.count(4)) m_clipper.target = params.at(4);
    if (params.count(5)) m_punchMode.target = params.at(5);
    if (params.count(6)) m_stereoLink.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String TransientShaper::getParameterName(int index) const {
    switch (index) {
        case 0: return "Attack";
        case 1: return "Sustain";
        case 2: return "Sensitivity";
        case 3: return "Speed";
        case 4: return "Clipper";
        case 5: return "Punch";
        case 6: return "Link";
        case 7: return "Mix";
        default: return "";
    }
}
#include "SpringReverb.h"
#include "DspEngineUtilities.h"
#include <cmath>
#include <algorithm>

SpringReverb::SpringReverb() {
    // Initialize smoothed parameters with proper defaults
    m_springCount.reset(0.5f);  // Default to 2-3 springs
    m_tension.reset(0.5f);      // Medium tension
    m_damping.reset(0.5f);      // Moderate damping
    m_preDelay.reset(0.1f);     // Small pre-delay
    m_modulation.reset(0.3f);   // Moderate spring wobble
    m_drip.reset(0.2f);         // Some drip character
    m_tone.reset(0.5f);         // Neutral tone
    m_mix.reset(0.3f);          // 30% wet
}

void SpringReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set smoothing times for parameters
    float smoothingTime = 50.0f; // 50ms for most parameters
    m_springCount.setSmoothingTime(smoothingTime, sampleRate);
    m_tension.setSmoothingTime(smoothingTime, sampleRate);
    m_damping.setSmoothingTime(smoothingTime, sampleRate);
    m_preDelay.setSmoothingTime(smoothingTime, sampleRate);
    m_modulation.setSmoothingTime(smoothingTime, sampleRate);
    m_drip.setSmoothingTime(smoothingTime, sampleRate);
    m_tone.setSmoothingTime(smoothingTime, sampleRate);
    m_mix.setSmoothingTime(smoothingTime, sampleRate);
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Reset thermal model
    m_thermalModel = ThermalModel();
    
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
        
        // Set up springs with their characteristics
        for (int i = 0; i < MAX_SPRINGS; ++i) {
            channel.springs[i].setSpringCharacteristics(SPRING_TYPES[i], sampleRate);
        }
    }
}

void SpringReverb::reset() {
    // Clear all reverb buffers
    for (auto& channel : m_channelStates) {
        for (auto& spring : channel.springs) {
            spring.reset();
        }
        // PreDelay doesn't have reset, just reinitialize
        channel.preDelay.prepare(MAX_DELAY_SIZE);
        channel.dcBlocker.reset();
        channel.springAging = 0.0f;
    }
    // Reset any additional reverb state
}

void SpringReverb::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard; // CRITICAL FIX: Prevent denormal numbers from killing reverb tails
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Process each channel
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 12) { // Every 12 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00004f);
        m_sampleCount = 0;
        
        // Update channel aging
        for (auto& state : m_channelStates) {
            state.updateAging(m_componentAge);
        }
    }
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update all smoothed parameters per sample for best quality
            m_springCount.update();
            m_tension.update();
            m_damping.update();
            m_preDelay.update();
            m_modulation.update();
            m_drip.update();
            m_tone.update();
            m_mix.update();
            
            float input = channelData[sample];
            float drySignal = input;
            
            // Apply DC blocking first
            input = state.dcBlocker.process(input);
            
            // Determine active spring count smoothly
            int activeSpringCount = 1 + static_cast<int>(m_springCount.current * 3.99f);
            activeSpringCount = std::min(activeSpringCount, MAX_SPRINGS);
            
            // Calculate parameters with thermal variation
            float preDelayMs = (m_preDelay.current * 100.0f) * thermalFactor; // 0-100ms
            float decayMult = (0.6f + m_tension.current * 0.35f) * thermalFactor; // 0.6 to 0.95
            
            // Apply pre-delay
            state.preDelay.setDelayTime(preDelayMs, m_sampleRate);
            float delayedInput = state.preDelay.process(input);
            
            // Input diffusion for more natural spring tank behavior
            float diffused = m_inputDiffuser.process(delayedInput);
            
            // Add drip effect (characteristic spring tank sound)
            state.dripGen.setThreshold(m_drip.current);
            float drip = state.dripGen.process(diffused, m_drip.current);
            diffused += drip * 0.3f;
            
            // Update spring aging (very slow evolution) - now handled globally
            // state.springAging is updated through updateAging() method
            
            // Process through active springs with enhanced coupling
            float reverbSum = 0.0f;
            std::array<float, MAX_SPRINGS> springOutputs;
            
            for (int i = 0; i < activeSpringCount; ++i) {
                // Update spring parameters
                state.springs[i].setDamping(m_damping.current);
                
                // Adjust modulation based on global parameter and spring characteristics with thermal effects
                state.springs[i].modulation.wobbleDepth = SPRING_TYPES[i].modDepth * m_modulation.current * 1.5f * thermalFactor;
                state.springs[i].modulation.bounceDepth = SPRING_TYPES[i].modDepth * m_modulation.current * 0.6f * thermalFactor;
                
                // Calculate complex feedback including cross-coupling
                float springFeedback = 0.0f;
                for (int j = 0; j < activeSpringCount; ++j) {
                    float coupling = state.feedbackMatrix[i][j];
                    if (j != i) {
                        // Add previous spring outputs for cross-coupling
                        springFeedback += springOutputs[j] * coupling * 0.1f;
                    }
                }
                springFeedback += decayMult * state.feedbackMatrix[i][i];
                
                // Process through enhanced spring waveguide
                float springOut = state.springs[i].process(diffused, springFeedback, 
                                                         state.springAging, m_sampleRate);
                springOutputs[i] = springOut;
                
                // Weight springs by their characteristics (longer = more prominent)
                float weight = 0.8f + i * 0.15f + SPRING_TYPES[i].decay * 0.2f;
                reverbSum += springOut * weight;
            }
            
            // Normalize by active spring count
            if (activeSpringCount > 0) {
                reverbSum /= activeSpringCount;
            }
            
            // Apply enhanced tone control
            float toned = state.toneControl.process(reverbSum, m_tone.current);
            
            // Spring tank saturation (characteristic nonlinear behavior)
            if (std::abs(toned) > 0.5f) {
                // Asymmetric saturation typical of spring reverb tanks
                float sign = toned < 0 ? -1.0f : 1.0f;
                float magnitude = std::abs(toned);
                float saturated = sign * (0.5f + std::tanh((magnitude - 0.5f) * 3.0f) * 0.4f);
                toned = saturated;
            }
            
            // Add subtle harmonics from spring resonance with aging effects
            float harmonics = toned * toned * toned * 0.05f * (1.0f + m_componentAge * 0.1f);
            toned += harmonics;
            
            // Add thermal noise to spring output
            if (m_componentAge > 0.01f) {
                // CRITICAL FIX: Use thread-safe random instead of rand()
                thread_local juce::Random tlsRandom;
                toned += m_componentAge * 0.001f * (tlsRandom.nextFloat() - 0.5f) * 0.5f;
            }
            
            // Final soft limiting
            toned = std::tanh(toned * 0.9f) * 1.1f;
            
            // Mix with dry signal using smooth parameter (aging affects mix slightly)
            float wetAmount = m_mix.current * (1.0f - m_componentAge * 0.02f);
            channelData[sample] = drySignal * (1.0f - wetAmount) + toned * wetAmount;
        }
    }
    
    // Enhanced stereo processing for realistic spring tank behavior
    if (numChannels >= 2) {
        float* leftData = buffer.getWritePointer(0);
        float* rightData = buffer.getWritePointer(1);
        
        // More sophisticated stereo decorrelation
        for (int sample = 0; sample < numSamples; ++sample) {
            // Subtle phase and amplitude differences
            float mid = (leftData[sample] + rightData[sample]) * 0.5f;
            float side = (leftData[sample] - rightData[sample]) * 0.5f;
            
            // Add slight delay and filtering differences between channels
            static float leftDelay = 0.0f, rightDelay = 0.0f;
            leftDelay += (leftData[sample] - leftDelay) * 0.95f;
            rightDelay += (rightData[sample] - rightDelay) * 0.93f;
            
            leftData[sample] = mid + side * 1.1f + rightDelay * 0.02f;
            rightData[sample] = mid - side * 0.9f + leftDelay * 0.015f;
        }
    }
}

void SpringReverb::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_springCount.target = params.at(0);
    if (params.find(1) != params.end()) m_tension.target = params.at(1);
    if (params.find(2) != params.end()) m_damping.target = params.at(2);
    if (params.find(3) != params.end()) m_preDelay.target = params.at(3);
    if (params.find(4) != params.end()) m_modulation.target = params.at(4);
    if (params.find(5) != params.end()) m_drip.target = params.at(5);
    if (params.find(6) != params.end()) m_tone.target = params.at(6);
    if (params.find(7) != params.end()) m_mix.target = params.at(7);
}

juce::String SpringReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Spring Count";
        case 1: return "Tension";
        case 2: return "Damping";
        case 3: return "Pre-Delay";
        case 4: return "Modulation";
        case 5: return "Drip";
        case 6: return "Tone";
        case 7: return "Mix";
        default: return "";
    }
}
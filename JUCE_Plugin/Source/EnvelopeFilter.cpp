#include "EnvelopeFilter.h"
#include <cmath>
#include <algorithm>
#include <random>

EnvelopeFilter::EnvelopeFilter() {
    // Initialize smoothed parameters
    m_sensitivity.reset(0.5f);
    m_attack.reset(0.3f);
    m_release.reset(0.5f);
    m_range.reset(0.7f);
    m_resonance.reset(0.5f);
    m_filterType.reset(0.0f);
    m_direction.reset(1.0f);
    m_mix.reset(1.0f);
}

void EnvelopeFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    m_sensitivity.setSmoothingTime(50.0f, sampleRate);
    m_attack.setSmoothingTime(100.0f, sampleRate);
    m_release.setSmoothingTime(100.0f, sampleRate);
    m_range.setSmoothingTime(200.0f, sampleRate);
    m_resonance.setSmoothingTime(20.0f, sampleRate);
    m_filterType.setSmoothingTime(100.0f, sampleRate);
    m_direction.setSmoothingTime(500.0f, sampleRate);
    m_mix.setSmoothingTime(50.0f, sampleRate);
    
    // Initialize both channels
    for (auto& channel : m_channelStates) {
        channel.reset();
        
        // Set initial envelope times
        float attackMs = 0.5f + m_attack.current * 50.0f; // 0.5-50ms
        float releaseMs = 10.0f + m_release.current * 500.0f; // 10-510ms
        channel.envelope.setAttackRelease(attackMs, releaseMs, sampleRate);
        
        channel.currentCutoff = 0.1f;
        channel.targetCutoff = 0.1f;
    }
}

void EnvelopeFilter::reset() {
    // Reset filter states
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
}

void EnvelopeFilter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update thermal modeling
    m_thermalModel.update(m_sampleRate);
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smoothed parameters
            m_sensitivity.update();
            m_attack.update();
            m_release.update();
            m_range.update();
            m_resonance.update();
            m_filterType.update();
            m_direction.update();
            m_mix.update();
            
            float input = channelData[sample];
            float drySignal = input;
            
            // Apply DC blocking
            input = m_dcBlockers[channel].process(input);
            
            // Update envelope settings dynamically
            float attackMs = 0.5f + m_attack.current * 50.0f;
            float releaseMs = 10.0f + m_release.current * 500.0f;
            state.envelope.setAttackRelease(attackMs, releaseMs, m_sampleRate);
            
            // Pre-emphasis for better envelope tracking with thermal compensation
            float thermalFactor = m_thermalModel.getThermalFactor();
            float emphasized = input;
            if (m_filterType.current > 0.5f) {
                // High-pass mode benefits from low frequency boost for envelope
                emphasized *= (1.0f + m_sensitivity.current) * thermalFactor;
            }
            
            // Get envelope with enhanced sensitivity scaling and RMS option
            bool useRMS = m_sensitivity.current > 0.7f; // Use RMS for high sensitivity
            float envelope = state.envelope.process(emphasized, m_sensitivity.current, useRMS);
            
            // Calculate filter cutoff from envelope with thermal compensation
            state.targetCutoff = calculateCutoff(envelope, thermalFactor);
            
            // Smooth cutoff changes with adaptive rate
            float cutoffSmoothing = 0.992f + m_attack.current * 0.007f; // Faster for quick attack
            state.currentCutoff = state.currentCutoff * cutoffSmoothing + 
                                state.targetCutoff * (1.0f - cutoffSmoothing);
            
            // Component aging simulation
            state.componentAge += 0.0001f / m_sampleRate;
            float agingFactor = 1.0f - state.componentAge * 0.005f;
            
            // Process through enhanced state variable filter
            float resonanceAmount = (0.1f + m_resonance.current * 0.89f) * agingFactor; // Prevent self-oscillation
            bool useVintageMode = false; // Could be a parameter in future
            float driveAmount = 0.0f;    // Could be a parameter in future
            
            auto filterOut = state.filter.process(input, state.currentCutoff, resonanceAmount, 
                                                 m_sampleRate, driveAmount, useVintageMode);
            
            // Select filter output based on type with enhanced morphing
            float output = getFilterMix(filterOut);
            
            // Enhanced soft saturation with analog character
            if (m_resonance.current > 0.5f) {
                output = softClip(output);
            }
            
            // Apply gentle analog saturation for warmth
            if (std::abs(output) > 0.1f) {
                output = analogSaturation(output, 0.1f);
            }
            
            // Mix with dry signal using smoothed parameter
            channelData[sample] = drySignal * (1.0f - m_mix.current) + output * m_mix.current;
        }
    }
}

// Enhanced SVFilter process implementation
EnvelopeFilter::SVFilter::Output EnvelopeFilter::SVFilter::process(float input, float cutoff, float resonance, double sampleRate, float drive, bool vintageMode) {
    // Update component drift slowly
    componentDrift += (((rand() % 1000) / 1000.0f - 0.5f) * 0.0001f) / sampleRate;
    componentDrift = std::max(-0.01f, std::min(0.01f, componentDrift));
    
    // Apply component drift to parameters
    float adjustedCutoff = cutoff * (1.0f + componentDrift);
    float adjustedResonance = resonance * (1.0f + componentDrift * 0.5f);
    
    // Clamp to reasonable bounds
    adjustedCutoff = std::max(0.001f, std::min(adjustedCutoff, 0.49f));
    adjustedResonance = std::max(0.0f, std::min(adjustedResonance, 0.99f));
    
    float g = std::tan(M_PI * adjustedCutoff);
    float k = 2.0f - 2.0f * adjustedResonance;
    float a1 = 1.0f / (1.0f + g * (g + k));
    float a2 = g * a1;
    float a3 = g * a2;
    
    // Apply drive/saturation if enabled
    float processedInput = input;
    if (drive > 0.01f) {
        if (vintageMode) {
            processedInput = std::tanh(input * (1.0f + drive * 2.0f)) / (1.0f + drive * 0.5f);
        } else {
            processedInput = std::tanh(input * (1.0f + drive)) / (1.0f + drive * 0.3f);
        }
    }
    
    // Enhanced Zavalishin topology with potential saturation
    float v3 = processedInput - ic2eq;
    float v1 = a1 * ic1eq + a2 * v3;
    float v2 = ic2eq + a2 * ic1eq + a3 * v3;
    
    // Add subtle saturation in the integrators for vintage character
    if (vintageMode && drive > 0.1f) {
        ic1eq = 2.0f * std::tanh(v1 * (1.0f + drive * 0.1f)) / (1.0f + drive * 0.05f) - ic1eq;
        ic2eq = 2.0f * std::tanh(v2 * (1.0f + drive * 0.05f)) / (1.0f + drive * 0.02f) - ic2eq;
    } else {
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;
    }
    
    Output out;
    out.lowpass = v2;
    out.bandpass = v1;
    out.highpass = processedInput - k * v1 - v2;
    out.notch = processedInput - k * v1;
    out.allpass = processedInput - 2.0f * k * v1;
    
    return out;
}

// Enhanced EnvelopeFollower process implementation
float EnvelopeFilter::EnvelopeFollower::process(float input, float sensitivity, bool useRMS) {
    float detectionSignal;
    
    if (useRMS) {
        // RMS detection for smoother response
        float inputSquared = input * input;
        rmsSum -= rmsBuffer[rmsIndex];
        rmsBuffer[rmsIndex] = inputSquared;
        rmsSum += inputSquared;
        rmsIndex = (rmsIndex + 1) % 64;
        
        detectionSignal = std::sqrt(rmsSum / 64.0f);
    } else {
        // Peak detection
        detectionSignal = std::abs(input);
        
        // Peak hold for punchier response
        if (detectionSignal > peakHold) {
            peakHold = detectionSignal;
            peakTimer = 441; // ~10ms at 44.1kHz
        } else if (peakTimer > 0) {
            peakTimer--;
        } else {
            peakHold *= 0.95f; // Gradual decay
        }
        
        // Combine peak and current value
        detectionSignal = detectionSignal * 0.7f + peakHold * 0.3f;
    }
    
    // Apply sensitivity scaling
    detectionSignal *= (0.1f + sensitivity * 2.0f);
    
    // Envelope following with attack/release
    if (detectionSignal > envelope) {
        envelope = detectionSignal + (envelope - detectionSignal) * attackCoeff;
    } else {
        envelope = detectionSignal + (envelope - detectionSignal) * releaseCoeff;
    }
    
    // Apply smoothing filter
    smoothingState += (envelope - smoothingState) * 0.1f;
    
    return smoothingState;
}

float EnvelopeFilter::calculateCutoff(float envelope, float thermalFactor) {
    // Base frequency (where the filter starts) with thermal compensation
    float baseFreq = (80.0f / m_sampleRate) * thermalFactor; // 80Hz normalized
    
    // Maximum frequency based on range with thermal drift
    float maxFreq = ((80.0f + m_range.current * 8000.0f) / m_sampleRate) * thermalFactor; // Up to ~8kHz
    
    // Apply direction (up or down sweep) with enhanced curve
    float normalizedEnv = std::pow(envelope, 1.2f + m_sensitivity.current * 0.6f); // Variable response curve
    
    if (m_direction.current > 0.5f) {
        // Up sweep (traditional auto-wah)
        return baseFreq + normalizedEnv * (maxFreq - baseFreq);
    } else {
        // Down sweep (inverted)
        return maxFreq - normalizedEnv * (maxFreq - baseFreq);
    }
}

float EnvelopeFilter::getFilterMix(const SVFilter::Output& filterOut) {
    // Enhanced smooth morphing between filter types with additional modes
    float filterType = m_filterType.current;
    
    if (filterType < 0.2f) {
        // Pure lowpass
        return filterOut.lowpass;
    } else if (filterType < 0.4f) {
        // Lowpass to bandpass transition
        float blend = (filterType - 0.2f) * 5.0f;
        return filterOut.lowpass * (1.0f - blend) + filterOut.bandpass * blend;
    } else if (filterType < 0.6f) {
        // Bandpass to highpass transition
        float blend = (filterType - 0.4f) * 5.0f;
        return filterOut.bandpass * (1.0f - blend) + filterOut.highpass * blend;
    } else if (filterType < 0.8f) {
        // Highpass to notch transition
        float blend = (filterType - 0.6f) * 5.0f;
        return filterOut.highpass * (1.0f - blend) + filterOut.notch * blend;
    } else {
        // Notch to allpass transition
        float blend = (filterType - 0.8f) * 5.0f;
        return filterOut.notch * (1.0f - blend) + filterOut.allpass * blend;
    }
}

float EnvelopeFilter::analogSaturation(float input, float amount) {
    // Subtle analog-style saturation
    float driven = input * (1.0f + amount);
    return std::tanh(driven * 0.9f) / (0.9f * (1.0f + amount * 0.2f));
}

void EnvelopeFilter::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_sensitivity.target = params.at(0);
    if (params.count(1)) m_attack.target = params.at(1);
    if (params.count(2)) m_release.target = params.at(2);
    if (params.count(3)) m_range.target = params.at(3);
    if (params.count(4)) m_resonance.target = params.at(4);
    if (params.count(5)) m_filterType.target = params.at(5);
    if (params.count(6)) m_direction.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String EnvelopeFilter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Sensitivity";
        case 1: return "Attack";
        case 2: return "Release";
        case 3: return "Range";
        case 4: return "Resonance";
        case 5: return "Filter";
        case 6: return "Direction";
        case 7: return "Mix";
        default: return "";
    }
}
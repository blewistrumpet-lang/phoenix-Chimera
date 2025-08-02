#include "StereoWidener.h"
#include <cmath>
#include <algorithm>

StereoWidener::StereoWidener() {
    // Initialize smoothed parameters
    m_width.reset(0.5f);          // 100% width default
    m_bassMonoFreq.reset(0.2f);   // ~120Hz default
    m_highShelfFreq.reset(0.5f);  // ~5kHz default
    m_highShelfGain.reset(0.5f);  // 0dB default
    m_delayTime.reset(0.3f);      // ~10ms default
    m_delayGain.reset(0.2f);      // 20% default
    m_correlation.reset(0.8f);    // 80% correlation default
    m_mix.reset(1.0f);            // 100% wet default
}

void StereoWidener::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    m_width.setSmoothingTime(100.0f, sampleRate);
    m_bassMonoFreq.setSmoothingTime(200.0f, sampleRate);
    m_highShelfFreq.setSmoothingTime(150.0f, sampleRate);
    m_highShelfGain.setSmoothingTime(100.0f, sampleRate);
    m_delayTime.setSmoothingTime(80.0f, sampleRate);
    m_delayGain.setSmoothingTime(50.0f, sampleRate);
    m_correlation.setSmoothingTime(300.0f, sampleRate);
    m_mix.setSmoothingTime(30.0f, sampleRate);
    
    // Initialize DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.reset();
    }

void StereoWidener::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for StereoWidener
}

    
    // Initialize channel states
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
    }
}

void StereoWidener::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels < 2) return; // Need stereo for widening
    
    // Update thermal modeling and component aging
    m_thermalModel.update(m_sampleRate);
    updateComponentAging(m_sampleRate);
    
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    for (int sample = 0; sample < numSamples; ++sample) {
        // Update smoothed parameters
        m_width.update();
        m_bassMonoFreq.update();
        m_highShelfFreq.update();
        m_highShelfGain.update();
        m_delayTime.update();
        m_delayGain.update();
        m_correlation.update();
        m_mix.update();
        
        float left = leftChannel[sample];
        float right = rightChannel[sample];
        float dryLeft = left;
        float dryRight = right;
        
        // Apply DC blocking
        left = m_dcBlockers[0].process(left);
        right = m_dcBlockers[1].process(right);
        
        // Apply vintage warmth
        float thermalFactor = m_thermalModel.getThermalFactor();
        left = applyVintageWarmth(left, thermalFactor);
        right = applyVintageWarmth(right, thermalFactor);
        
        // Convert to Mid/Side for processing
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // Bass mono processing - high-pass filter the side channel
        float bassMonoFreq = 20.0f + m_bassMonoFreq.current * 480.0f; // 20Hz to 500Hz
        float cutoff = juce::MathConstants<float>::twoPi * bassMonoFreq / m_sampleRate;
        cutoff = std::min(cutoff, 0.99f);
        
        side = m_channelStates[0].bassMonoFilter.processHighPass(side, cutoff);
        
        // Apply high-shelf EQ to side channel for brightness
        float shelfFreq = 1000.0f + m_highShelfFreq.current * 19000.0f; // 1kHz to 20kHz
        float shelfGain = -20.0f + m_highShelfGain.current * 40.0f; // -20dB to +20dB
        side = m_channelStates[0].shelfFilter.processHighShelf(side, shelfFreq, shelfGain, m_sampleRate);
        
        // Apply stereo width
        float width = m_width.current * 2.0f; // 0% to 200%
        side *= width;
        
        // Phase decorrelation using all-pass filters
        float decorrelatedSide = side;
        decorrelatedSide = m_channelStates[0].allPass1.process(decorrelatedSide);
        decorrelatedSide = m_channelStates[0].allPass2.process(decorrelatedSide);
        
        // Apply inter-channel correlation
        float corrAmount = m_correlation.current;
        side = side * corrAmount + decorrelatedSide * (1.0f - corrAmount);
        
        // Convert back to L/R
        left = mid + side;
        right = mid - side;
        
        // Haas effect processing
        m_channelStates[0].haasDelay.write(left);
        m_channelStates[1].haasDelay.write(right);
        
        float delayMs = m_delayTime.current * 30.0f; // 0ms to 30ms
        float delayGain = m_delayGain.current;
        
        if (delayMs > 0.1f && delayGain > 0.01f) {
            // Apply Haas delay to create stereo width
            float delayedLeft = m_channelStates[1].haasDelay.read(delayMs, m_sampleRate);
            float delayedRight = m_channelStates[0].haasDelay.read(delayMs, m_sampleRate);
            
            // Mix delayed signal with opposite channel
            left = left * (1.0f - delayGain * 0.5f) + delayedRight * delayGain * 0.5f;
            right = right * (1.0f - delayGain * 0.5f) + delayedLeft * delayGain * 0.5f;
        }
        
        // Apply subtle analog saturation for warmth
        left = analogSaturation(left, 0.05f);
        right = analogSaturation(right, 0.05f);
        
        // Final mix with dry signal
        leftChannel[sample] = dryLeft * (1.0f - m_mix.current) + left * m_mix.current;
        rightChannel[sample] = dryRight * (1.0f - m_mix.current) + right * m_mix.current;
    }
}

void StereoWidener::processMidSide(float& left, float& right, float width) {
    float mid = (left + right) * 0.5f;
    float side = (left - right) * 0.5f * width;
    
    left = mid + side;
    right = mid - side;
}

float StereoWidener::calculateCorrelation(float left, float right, float amount) {
    // Simple correlation calculation for demonstration
    // In practice, this would use a sliding window
    float correlation = left * right;
    return correlation * amount;
}

float StereoWidener::analogSaturation(float input, float amount) {
    // Subtle analog-style saturation with component aging
    float agingFactor = 1.0f + m_componentAge * 0.015f;
    float driven = input * (1.0f + amount) * agingFactor;
    
    // Asymmetric saturation for analog character
    if (input > 0.0f) {
        return std::tanh(driven * 0.9f) / (0.9f * (1.0f + amount * 0.1f));
    } else {
        return std::tanh(driven * 0.95f) / (0.95f * (1.0f + amount * 0.08f));
    }
}

float StereoWidener::applyVintageWarmth(float input, float thermalFactor) {
    // Add thermal noise and subtle nonlinearity
    float noise = m_thermalModel.thermalNoise;
    float warmed = input + noise;
    
    // Very subtle saturation for warmth
    if (std::abs(warmed) > 0.1f) {
        warmed = std::tanh(warmed * 1.05f) / 1.05f;
    }
    
    return warmed * thermalFactor;
}

void StereoWidener::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_width.target = params.at(0);
    if (params.count(1)) m_bassMonoFreq.target = params.at(1);
    if (params.count(2)) m_highShelfFreq.target = params.at(2);
    if (params.count(3)) m_highShelfGain.target = params.at(3);
    if (params.count(4)) m_delayTime.target = params.at(4);
    if (params.count(5)) m_delayGain.target = params.at(5);
    if (params.count(6)) m_correlation.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String StereoWidener::getParameterName(int index) const {
    switch (index) {
        case 0: return "Width";
        case 1: return "Bass Mono";
        case 2: return "HF Shelf Freq";
        case 3: return "HF Shelf Gain";
        case 4: return "Haas Delay";
        case 5: return "Delay Gain";
        case 6: return "Correlation";
        case 7: return "Mix";
        default: return "";
    }
}
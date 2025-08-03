#include "TapeEcho.h"
#include <cmath>
#include <algorithm>

TapeEcho::TapeEcho() {
    // Initialize parameters with musical defaults from metadata
    m_time.reset(0.375f);      // 375ms - dotted eighth at 120 BPM
    m_feedback.reset(0.35f);   // Some repeats but stable  
    m_wowFlutter.reset(0.25f); // Vintage character without nausea
    m_saturation.reset(0.3f);  // Warm but not distorted
    m_mix.reset(0.35f);        // Audible but not overpowering
}

void TapeEcho::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set smoothing times
    float smoothingMs = 10.0f;
    m_time.setSmoothingTime(smoothingMs, sampleRate);
    m_feedback.setSmoothingTime(smoothingMs, sampleRate);
    m_wowFlutter.setSmoothingTime(smoothingMs, sampleRate);
    m_saturation.setSmoothingTime(smoothingMs, sampleRate);
    m_mix.setSmoothingTime(smoothingMs, sampleRate);
    
    // Prepare channel states
    for (auto& state : m_channelStates) {
        state.prepare(sampleRate);
    }
}

void TapeEcho::reset() {
    // Clear all delay buffers
    for (auto& state : m_channelStates) {
        // Clear delay line
        state.delayLine.clear();
        state.delayLine.writePos = 0;
        
        // Reset modulation phases
        state.modulation.wowPhase = 0.0f;
        state.modulation.flutterPhase1 = 0.0f;
        state.modulation.flutterPhase2 = 0.0f;
        state.modulation.driftPhase = 0.0f;
        state.modulation.randomWalk = 0.0f;
        state.modulation.randomTarget = 0.0f;
        
        // Reset saturation state
        state.saturation.prevInput = 0.0f;
        state.saturation.prevOutput = 0.0f;
        state.saturation.hysteresisState = 0.0f;
        
        // Reset filter state
        state.filter.reset();
        
        // Reset compression state
        state.compression.envelope = 0.0f;
        
        // Reset feedback filters
        state.feedbackHighpass = 0.0f;
        state.feedbackLowpass = 0.0f;
    }
}

void TapeEcho::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smoothed parameters
            m_time.update();
            m_feedback.update();
            m_wowFlutter.update();
            m_saturation.update();
            m_mix.update();
            
            channelData[sample] = processSample(channelData[sample], channel);
        }
    }
}

float TapeEcho::processSample(float input, int channel) {
    auto& state = m_channelStates[channel];
    
    // Calculate delay time with modulation
    float baseDelayMs = 10.0f + m_time.current * 1990.0f; // 10ms to 2000ms
    float modulation = state.modulation.process(m_wowFlutter.current, m_sampleRate);
    float modulatedDelayMs = baseDelayMs * (1.0f + modulation);
    float delaySamples = modulatedDelayMs * m_sampleRate * 0.001f;
    
    // Clamp delay to valid range
    delaySamples = std::max(1.0f, std::min(delaySamples, 
        static_cast<float>(state.delayLine.buffer.size() - 2)));
    
    // Read delayed signal
    float delayed = state.delayLine.readInterpolated(delaySamples);
    
    // Apply tape filtering to the delayed signal
    float tapeAge = m_saturation.current; // Use saturation as "tape age" parameter
    delayed = state.filter.processPlayback(delayed, tapeAge, m_sampleRate);
    
    // Apply tape compression
    delayed = state.compression.process(delayed, m_saturation.current * 0.5f);
    
    // Apply tape saturation
    delayed = state.saturation.process(delayed, m_saturation.current);
    
    // Feedback path processing
    float feedback = delayed * m_feedback.current;
    
    // Feedback filtering to prevent buildup
    // Highpass at 100Hz
    float hpCutoff = 100.0f / m_sampleRate;
    float hpAlpha = hpCutoff / (hpCutoff + 1.0f);
    float hpOut = feedback - state.feedbackHighpass;
    state.feedbackHighpass += hpAlpha * (feedback - state.feedbackHighpass);
    feedback = hpOut;
    
    // Lowpass at 6kHz (gets darker with each repeat)
    float lpCutoff = 6000.0f / m_sampleRate;
    float lpAlpha = lpCutoff / (lpCutoff + 1.0f);
    state.feedbackLowpass += lpAlpha * (feedback - state.feedbackLowpass);
    feedback = state.feedbackLowpass;
    
    // Soft limiting on feedback to prevent runaway
    if (m_feedback.current > 0.75f) {
        // Apply soft clipping as feedback approaches self-oscillation
        float limitThreshold = 0.8f;
        if (std::abs(feedback) > limitThreshold) {
            float excess = std::abs(feedback) - limitThreshold;
            float limited = limitThreshold + std::tanh(excess * 3.0f) * 0.2f;
            feedback = limited * (feedback < 0 ? -1.0f : 1.0f);
        }
    }
    
    // Process input through tape input stage
    float processedInput = state.filter.processInput(input, tapeAge);
    
    // Write to delay line
    state.delayLine.write(processedInput + feedback);
    
    // Output mixing
    float output = input * (1.0f - m_mix.current) + delayed * m_mix.current;
    
    // Subtle output limiting to prevent clipping
    if (std::abs(output) > 0.95f) {
        output = 0.95f * std::tanh(output / 0.95f);
    }
    
    return output;
}

void TapeEcho::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) {
        m_time.target = params.at(0);
    }
    if (params.find(1) != params.end()) {
        m_feedback.target = params.at(1);
    }
    if (params.find(2) != params.end()) {
        m_wowFlutter.target = params.at(2);
    }
    if (params.find(3) != params.end()) {
        m_saturation.target = params.at(3);
    }
    if (params.find(4) != params.end()) {
        m_mix.target = params.at(4);
    }
}

juce::String TapeEcho::getParameterName(int index) const {
    switch (index) {
        case 0: return "Time";
        case 1: return "Feedback";
        case 2: return "Wow & Flutter";
        case 3: return "Saturation";
        case 4: return "Mix";
        default: return "";
    }
}
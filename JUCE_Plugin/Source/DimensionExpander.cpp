#include "DimensionExpander.h"
#include <cmath>
#include <algorithm>

DimensionExpander::DimensionExpander() {
    // Initialize smooth parameters
    m_width.setImmediate(0.5f);
    m_depth.setImmediate(0.5f);
    m_crossfeed.setImmediate(0.3f);
    m_bassRetention.setImmediate(0.7f);
    m_ambience.setImmediate(0.3f);
    m_movement.setImmediate(0.0f);
    m_clarity.setImmediate(0.5f);
    m_mix.setImmediate(1.0f);
    
    // Set smoothing rates
    m_width.setSmoothingRate(0.995f);
    m_depth.setSmoothingRate(0.998f);
    m_crossfeed.setSmoothingRate(0.992f);
    m_bassRetention.setSmoothingRate(0.999f);
    m_ambience.setSmoothingRate(0.995f);
    m_movement.setSmoothingRate(0.998f);
    m_clarity.setSmoothingRate(0.995f);
    m_mix.setSmoothingRate(0.995f);
}

void DimensionExpander::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize both channels
    for (int ch = 0; ch < 2; ++ch) {
        auto& state = m_channelStates[ch];
        
        // Clear delay buffer
        std::fill(state.delayBuffer.begin(), state.delayBuffer.end(), 0.0f);
        state.delayIndex = 0;
        
        // Setup all-pass filters with different sizes for rich ambience
        const int baseSizes[4] = { 347, 419, 487, 557 }; // Prime numbers
        for (int i = 0; i < 4; ++i) {
            int size = static_cast<int>(baseSizes[i] * sampleRate / 44100.0);
            state.allpassFilters[i].setSize(size);
            state.allpassFilters[i].feedback = 0.5f + i * 0.05f;
        }
        
        // Initialize pitch shifter
        state.pitchShifter.prepare();
        
        // Set initial LFO phase (different for each channel)
        state.lfoPhase = ch * M_PI;
        
        // Clear filter states
        state.lowpassState = 0.0f;
        state.highpassState = 0.0f;
    }
    
    // Initialize DC blockers
    m_inputDCBlockers.fill({});
    m_outputDCBlockers.fill({});
    
    // Prepare oversampler
    if (m_useOversampling) {
        m_oversampler.prepare(samplesPerBlock);
    }
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Reset thermal model
    m_thermalModel = ThermalModel();
}

void DimensionExpander::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for DimensionExpander
}

void DimensionExpander::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Only process stereo
    if (numChannels < 2) {
        return;
    }
    
    // Update smooth parameters
    m_width.update();
    m_depth.update();
    m_crossfeed.update();
    m_bassRetention.update();
    m_ambience.update();
    m_movement.update();
    m_clarity.update();
    m_mix.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 8) { // Every 8 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00007f);
        m_sampleCount = 0;
        
        // Update channel aging
        for (auto& state : m_channelStates) {
            state.updateAging(m_componentAge);
        }
    }
    
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    for (int sample = 0; sample < numSamples; ++sample) {
        float left = leftChannel[sample];
        float right = rightChannel[sample];
        float dryLeft = left;
        float dryRight = right;
        
        // DC block inputs
        left = m_inputDCBlockers[0].process(left);
        right = m_inputDCBlockers[1].process(right);
        
        // Extract mono (center) and side components for bass retention
        float mono = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // Crossover filtering for bass retention (120Hz)
        float crossoverFreq = 120.0f / m_sampleRate;
        float alpha = 1.0f - std::exp(-2.0f * M_PI * crossoverFreq);
        
        // Extract bass from mono signal
        auto& leftState = m_channelStates[0];
        auto& rightState = m_channelStates[1];
        
        leftState.lowpassState += alpha * (mono - leftState.lowpassState);
        float monoBass = leftState.lowpassState * m_bassRetention.current * (1.0f + leftState.componentDrift);
        float monoMid = mono - leftState.lowpassState;
        
        // Process stereo width on mid/high frequencies with thermal variation
        if (m_width.current != 0.5f) {
            float widthAmount = (m_width.current - 0.5f) * 2.0f * thermalFactor; // -1 to +1
            side *= (1.0f + widthAmount * 1.5f);
        }
        
        // Apply Haas effect for extra width
        if (m_width.current > 0.5f) {
            float haasDelay = (m_width.current - 0.5f) * MAX_HAAS_DELAY * 0.5f * thermalFactor;
            int delaySamples = static_cast<int>(haasDelay);
            
            // Delay right channel slightly
            float delayedRight = rightState.delayBuffer[rightState.delayIndex];
            rightState.delayBuffer[rightState.delayIndex] = right;
            rightState.delayIndex = (rightState.delayIndex + 1) % delaySamples;
            
            right = right * 0.7f + delayedRight * 0.3f;
        }
        
        // Add depth with micro pitch shifting and thermal modeling
        if (m_depth.current > 0.0f) {
            // Calculate LFO modulation for movement with thermal variation
            float modDepth = m_movement.current * 5.0f * thermalFactor; // ±5 cents modulation
            float leftMod = std::sin(leftState.lfoPhase) * modDepth;
            float rightMod = std::sin(rightState.lfoPhase) * modDepth;
            
            // Add thermal drift to LFO rates
            float lfoRate1 = 0.3f * thermalFactor;
            float lfoRate2 = 0.37f * thermalFactor;
            
            leftState.lfoPhase += 2.0f * M_PI * lfoRate1 / m_sampleRate;
            rightState.lfoPhase += 2.0f * M_PI * lfoRate2 / m_sampleRate;
            
            // Apply pitch shift (opposite directions for width) with aging effects
            float leftShift = (-m_depth.current * 10.0f + leftMod) * (1.0f + leftState.componentDrift);
            float rightShift = (m_depth.current * 10.0f + rightMod) * (1.0f + rightState.componentDrift);
            
            float shiftedLeft = leftState.pitchShifter.processWithThermal(left, leftShift, thermalFactor);
            float shiftedRight = rightState.pitchShifter.processWithThermal(right, rightShift, thermalFactor);
            
            left = left * (1.0f - m_depth.current * 0.3f) + shiftedLeft * m_depth.current * 0.3f;
            right = right * (1.0f - m_depth.current * 0.3f) + shiftedRight * m_depth.current * 0.3f;
        }
        
        // Add ambience with aging modeling
        if (m_ambience.current > 0.0f) {
            float ambLeft = processAmbience(side, leftState);
            float ambRight = processAmbience(side, rightState);
            
            // Add thermal noise to ambience
            ambLeft += leftState.thermalNoise * ((rand() % 1000) / 1000.0f - 0.5f) * 0.2f;
            ambRight += rightState.thermalNoise * ((rand() % 1000) / 1000.0f - 0.5f) * 0.2f;
            
            left += ambLeft * m_ambience.current * 0.3f * (1.0f - m_componentAge * 0.1f);
            right += ambRight * m_ambience.current * 0.3f * (1.0f - m_componentAge * 0.1f);
        }
        
        // Apply crossfeed for better phantom center
        if (m_crossfeed.current > 0.0f) {
            processCrossfeed(left, right, m_crossfeed.current * thermalFactor);
        }
        
        // Reconstruct with processed components
        left = monoMid + monoBass + side;
        right = monoMid + monoBass - side;
        
        // Clarity enhancement (subtle high-frequency boost) with aging effects
        if (m_clarity.current > 0.5f) {
            float clarityAmount = (m_clarity.current - 0.5f) * 2.0f * (1.0f - m_componentAge * 0.15f);
            float highFreq = (8000.0f / m_sampleRate) * thermalFactor;
            float highAlpha = 1.0f - std::exp(-2.0f * M_PI * highFreq);
            
            leftState.highpassState += highAlpha * (left - leftState.highpassState);
            rightState.highpassState += highAlpha * (right - rightState.highpassState);
            
            left += leftState.highpassState * clarityAmount * 0.2f;
            right += rightState.highpassState * clarityAmount * 0.2f;
        }
        
        // Add subtle noise from aging components
        if (m_componentAge > 0.01f) {
            left += leftState.noiseLevel * ((rand() % 1000) / 1000.0f - 0.5f) * 2.0f;
            right += rightState.noiseLevel * ((rand() % 1000) / 1000.0f - 0.5f) * 2.0f;
        }
        
        // DC block outputs
        left = m_outputDCBlockers[0].process(left);
        right = m_outputDCBlockers[1].process(right);
        
        // Analog-style saturation with aging
        if (std::abs(left) > 0.8f || std::abs(right) > 0.8f) {
            float saturation = 1.0f + m_componentAge * 0.1f;
            left = std::tanh(left * saturation) / saturation;
            right = std::tanh(right * saturation) / saturation;
        }
        
        // Mix with dry signal (aging affects mix slightly)
        float wetAmount = m_mix.current * (1.0f - m_componentAge * 0.02f);
        leftChannel[sample] = dryLeft * (1.0f - wetAmount) + left * wetAmount;
        rightChannel[sample] = dryRight * (1.0f - wetAmount) + right * wetAmount;
    }
}

float DimensionExpander::processAmbience(float input, ChannelState& state) {
    float signal = input;
    
    // Process through cascade of all-pass filters with aging effects
    for (auto& allpass : state.allpassFilters) {
        signal = allpass.processWithAging(signal, m_componentAge);
    }
    
    return signal;
}

void DimensionExpander::processCrossfeed(float& left, float& right, float amount) {
    // Subtle crossfeed to improve phantom center imaging
    float crossfeedGain = amount * 0.3f;
    float delayedLeft = left;
    float delayedRight = right;
    
    // Simple first-order lowpass for crossfeed (300Hz)
    static float leftLowpass = 0.0f;
    static float rightLowpass = 0.0f;
    float lpCutoff = 300.0f / m_sampleRate;
    float lpAlpha = 1.0f - std::exp(-2.0f * M_PI * lpCutoff);
    
    leftLowpass += lpAlpha * (delayedLeft - leftLowpass);
    rightLowpass += lpAlpha * (delayedRight - rightLowpass);
    
    // Apply crossfeed
    left += rightLowpass * crossfeedGain;
    right += leftLowpass * crossfeedGain;
}

void DimensionExpander::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_width.target = params.at(0);
    if (params.count(1)) m_depth.target = params.at(1);
    if (params.count(2)) m_crossfeed.target = params.at(2);
    if (params.count(3)) m_bassRetention.target = params.at(3);
    if (params.count(4)) m_ambience.target = params.at(4);
    if (params.count(5)) m_movement.target = params.at(5);
    if (params.count(6)) m_clarity.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String DimensionExpander::getParameterName(int index) const {
    switch (index) {
        case 0: return "Width";
        case 1: return "Depth";
        case 2: return "Crossfeed";
        case 3: return "Bass";
        case 4: return "Ambience";
        case 5: return "Movement";
        case 6: return "Clarity";
        case 7: return "Mix";
        default: return "";
    }
}
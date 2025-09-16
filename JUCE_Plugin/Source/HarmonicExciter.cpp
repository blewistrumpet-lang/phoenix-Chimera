#include "HarmonicExciter.h"
#include <cmath>

HarmonicExciter::HarmonicExciter() {
    // Initialize smooth parameters
    m_frequency.setImmediate(0.7f);
    m_drive.setImmediate(0.5f);
    m_harmonics.setImmediate(0.5f);
    m_clarity.setImmediate(0.5f);
    m_warmth.setImmediate(0.3f);
    m_presence.setImmediate(0.5f);
    m_color.setImmediate(0.5f);
    m_mix.setImmediate(0.5f);
    
    // Set smoothing rates
    m_frequency.setSmoothingRate(0.992f);
    m_drive.setSmoothingRate(0.990f);
    m_harmonics.setSmoothingRate(0.995f);
    m_clarity.setSmoothingRate(0.995f);
    m_warmth.setSmoothingRate(0.995f);
    m_presence.setSmoothingRate(0.995f);
    m_color.setSmoothingRate(0.998f); // Slow for character changes
    m_mix.setSmoothingRate(0.995f);
}

void HarmonicExciter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    for (auto& channel : m_channelStates) {
        // Setup crossover frequencies
        // Low band: < 800Hz
        channel.lowBand.filter1.setFrequency(800.0f, sampleRate);
        channel.lowBand.filter2.setFrequency(800.0f, sampleRate);
        
        // Mid band: 800Hz - 5kHz
        channel.midBand.filter1.setFrequency(5000.0f, sampleRate);
        channel.midBand.filter2.setFrequency(5000.0f, sampleRate);
        
        // High band: > 5kHz (using mid band highpass output)
        
        // Reset generators
        channel.lowGen = HarmonicGenerator();
        channel.midGen = HarmonicGenerator();
        channel.highGen = HarmonicGenerator();
        
        // Reset filters
        channel.presenceState = 0.0f;
        channel.warmthState = 0.0f;
        channel.dcBlockerState = 0.0f;
        
        // Clear phase history
        std::fill(std::begin(channel.phaseHistory), std::end(channel.phaseHistory), 0.0f);
        channel.phaseIndex = 0;
        
        // Initialize aging parameters
        channel.updateAging(0.0f);
    }
}

void HarmonicExciter::reset() {
    // Reset all internal state
    for (auto& channel : m_channelStates) {
        channel.warmthState = 0.0f;
        channel.presenceState = 0.0f;
        channel.phaseIndex = 0;
    }
}

void HarmonicExciter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_frequency.update();
    m_drive.update();
    m_harmonics.update();
    m_clarity.update();
    m_warmth.update();
    m_presence.update();
    m_color.update();
    m_mix.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 7) { // Every 7 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00007f);
        m_sampleCount = 0;
        
        // Update channel aging
        for (auto& state : m_channelStates) {
            state.updateAging(m_componentAge);
        }
    }
    
    // Calculate frequency-dependent drive amounts with thermal modulation
    float targetFreq = (1000.0f + m_frequency.current * 9000.0f) * thermalFactor; // 1kHz to 10kHz
    float lowDrive = m_drive.current * (1.0f - m_frequency.current * 0.5f);  // More low drive
    float midDrive = m_drive.current * thermalFactor;
    float highDrive = m_drive.current * (0.7f + m_frequency.current * 0.3f) * thermalFactor;
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // DC block input
            input = m_inputDCBlockers[channel].process(input);
            
            // Three-band split
            float low = state.lowBand.processLowpass(input);
            float midInput = state.lowBand.processHighpass(input);
            float mid = state.midBand.processLowpass(midInput);
            float high = state.midBand.processHighpass(midInput);
            
            // Process each band with harmonic generation - FIXED FOR STUDIO QUALITY
            float processedLow = low;
            float processedMid = mid;
            float processedHigh = high;
            
            // Low band - subtle warmth WITHOUT excessive aging effects
            if (lowDrive > 0.01f) {
                // Soft saturation for low frequencies
                processedLow = std::tanh(low * (1.0f + lowDrive * 2.0f));
                processedLow = processWarmthFilter(processedLow, state.warmthState);
            } else {
                processedLow = low;  // Pass through if no drive
            }
            
            // Mid band - main harmonic generation SIMPLIFIED
            if (midDrive > 0.01f) {
                // Pre-emphasis for better harmonic generation
                float emphasized = mid * (1.0f + m_harmonics.current);
                
                // Simple harmonic generation with soft clipping
                processedMid = state.midGen.process(emphasized, midDrive * 2.0f, m_color.current);
                
                // Phase alignment for clarity
                if (m_clarity.current > 0.5f) {
                    state.phaseHistory[state.phaseIndex] = processedMid;
                    state.phaseIndex = (state.phaseIndex + 1) % 4;
                    
                    // Simple phase linearization
                    float sum = 0.0f;
                    for (int i = 0; i < 4; ++i) {
                        sum += state.phaseHistory[i] * (1.0f - i * 0.25f);
                    }
                    processedMid = sum * 0.4f + processedMid * (1.0f - 0.4f * m_clarity.current);
                }
            } else {
                processedMid = mid;  // Pass through if no drive
            }
            
            // High band - presence and air SIMPLIFIED
            if (highDrive > 0.01f) {
                // Enhance transients in high frequencies
                float transient = high - state.highGen.lastSample;
                state.highGen.lastSample = high;
                
                // Harmonic generation with normal drive
                processedHigh = state.highGen.process(high, highDrive, m_color.current);
                processedHigh += transient * m_presence.current * 0.5f;
                
                // Presence filter
                processedHigh = processPresenceFilter(processedHigh, state.presenceState);
            } else {
                processedHigh = high;  // Pass through if no drive
            }
            
            // Recombine bands with unity gain
            float excited = processedLow + processedMid + processedHigh;
            
            // DC blocker
            excited = processDCBlocker(excited, state.dcBlockerState);
            
            // DC block output
            excited = m_outputDCBlockers[channel].process(excited);
            
            // Soft limiting to prevent clipping
            excited = std::tanh(excited * 0.7f) * 1.43f;
            
            // Mix with dry signal
            channelData[sample] = drySignal * (1.0f - m_mix.current) + excited * m_mix.current;
        }
    }
}

float HarmonicExciter::processPresenceFilter(float input, float& state) {
    // High shelf at ~8kHz for air
    float freq = 8000.0f / std::max(8000.0, m_sampleRate);
    float gain = 1.0f + m_presence.current * 0.5f;
    
    float w = 2.0f * std::sin(M_PI * freq);
    float a = (gain - 1.0f) * 0.5f;
    
    float highpass = input - state;
    state += highpass * w;
    
    return input + highpass * a;
}

float HarmonicExciter::processPresenceFilterWithAging(float input, float& state, float aging, float thermalFactor) {
    float basic = processPresenceFilter(input, state);
    
    // Aging affects filter characteristics
    if (aging > 0.01f) {
        // Frequency response changes with aging
        float freqShift = aging * 0.1f * thermalFactor;
        basic *= (1.0f + freqShift);
        
        // Add component tolerances
        basic += aging * 0.02f * m_thermalModel.dist(m_thermalModel.rng) * basic;
    }
    
    return basic;
}

float HarmonicExciter::processWarmthFilter(float input, float& state) {
    // Low shelf at ~100Hz for warmth
    float freq = 100.0f / std::max(8000.0, m_sampleRate);
    float gain = 1.0f + m_warmth.current * 0.3f;
    
    float w = 2.0f * std::sin(M_PI * freq);
    float a = (gain - 1.0f) * 0.5f;
    
    float lowpass = state;
    state += (input - state) * w;
    
    return input + lowpass * a;
}

float HarmonicExciter::processWarmthFilterWithAging(float input, float& state, float aging, float thermalFactor) {
    float basic = processWarmthFilter(input, state);
    
    // Aging affects filter characteristics
    if (aging > 0.01f) {
        // Frequency response changes with aging
        float freqShift = aging * 0.08f * thermalFactor;
        basic *= (1.0f - freqShift); // Warmth decreases slightly with age
        
        // Add component drift
        basic += aging * 0.015f * m_thermalModel.dist(m_thermalModel.rng) * basic;
    }
    
    return basic;
}

float HarmonicExciter::processDCBlocker(float input, float& state) {
    float output = input - state;
    state = input - output * 0.995f;
    return output;
}

void HarmonicExciter::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_frequency.target = params.at(0);
    if (params.count(1)) m_drive.target = params.at(1);
    if (params.count(2)) m_harmonics.target = params.at(2);
    if (params.count(3)) m_clarity.target = params.at(3);
    if (params.count(4)) m_warmth.target = params.at(4);
    if (params.count(5)) m_presence.target = params.at(5);
    if (params.count(6)) m_color.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String HarmonicExciter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Frequency";
        case 1: return "Drive";
        case 2: return "Harmonics";
        case 3: return "Clarity";
        case 4: return "Warmth";
        case 5: return "Presence";
        case 6: return "Color";
        case 7: return "Mix";
        default: return "";
    }
}
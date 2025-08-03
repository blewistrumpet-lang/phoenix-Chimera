#include "AnalogPhaser.h"
#include <cmath>

AnalogPhaser::AnalogPhaser() {
    // Initialize smooth parameters
    m_rate.setImmediate(0.5f);
    m_depth.setImmediate(0.5f);
    m_feedback.setImmediate(0.3f);
    m_stages.setImmediate(0.5f);
    m_stereoSpread.setImmediate(0.5f);
    m_centerFreq.setImmediate(0.5f);
    m_resonance.setImmediate(0.3f);
    m_mix.setImmediate(0.5f);
    
    // Set smoothing rates
    m_rate.setSmoothingRate(0.995f);
    m_depth.setSmoothingRate(0.990f);
    m_feedback.setSmoothingRate(0.995f);
    m_stages.setSmoothingRate(0.999f); // Very slow for stage changes
    m_stereoSpread.setSmoothingRate(0.995f);
    m_centerFreq.setSmoothingRate(0.992f);
    m_resonance.setSmoothingRate(0.995f);
    m_mix.setSmoothingRate(0.995f);
}

void AnalogPhaser::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize both channels
    for (auto& channel : m_channelStates) {
        // Clear all-pass filters
        for (auto& filter : channel.allpassFilters) {
            filter.state = 0.0f;
            filter.coefficient = 0.0f;
        }
        
        // Initialize LFO phase
        channel.lfoPhase = 0.0f;
        channel.feedbackSample = 0.0f;
        channel.currentDepth = m_depth.current;
        channel.targetDepth = m_depth.current;
        channel.componentDrift = 0.0f;
        channel.noiseLevel = 0.0f;
    }
    
    // Set different initial phases for stereo effect
    m_channelStates[1].lfoPhase = M_PI;
    
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

void AnalogPhaser::reset() {
    // Reset all channel states
    for (auto& channel : m_channelStates) {
        // Clear all-pass filters
        for (auto& filter : channel.allpassFilters) {
            filter.state = 0.0f;
            filter.coefficient = 0.0f;
        }
        
        // Reset LFO phase (keep stereo spread)
        channel.lfoPhase = 0.0f;
        channel.feedbackSample = 0.0f;
        channel.currentDepth = m_depth.current;
        channel.targetDepth = m_depth.current;
        channel.componentDrift = 0.0f;
        channel.noiseLevel = 0.0f;
    }
    
    // Restore stereo phase relationship
    m_channelStates[1].lfoPhase = M_PI;
    
    // Reset DC blockers
    for (auto& blocker : m_inputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    for (auto& blocker : m_outputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    // Reset thermal model
    m_thermalModel.temperature = 25.0f;
    m_thermalModel.thermalNoise = 0.0f;
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Reset oversampling filters
    if (m_useOversampling) {
        m_oversampler.upsampleFilter.x.fill(0.0f);
        m_oversampler.upsampleFilter.y.fill(0.0f);
        m_oversampler.downsampleFilter.x.fill(0.0f);
        m_oversampler.downsampleFilter.y.fill(0.0f);
    }
}

void AnalogPhaser::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_rate.update();
    m_depth.update();
    m_feedback.update();
    m_stages.update();
    m_stereoSpread.update();
    m_centerFreq.update();
    m_resonance.update();
    m_mix.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 8) { // Every 8 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00005f);
        m_sampleCount = 0;
        
        // Update channel aging
        for (auto& state : m_channelStates) {
            state.componentDrift = m_componentAge * 0.01f;
            state.noiseLevel = m_componentAge * 0.0005f;
        }
    }
    
    // Calculate active stages based on parameter
    int activeStages = getActiveStages();
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break; // Process only stereo
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Smooth depth changes
        float depthSmoothing = 0.999f;
        state.targetDepth = m_depth.current;
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // DC block input
            input = m_inputDCBlockers[channel].process(input);
            
            // Smooth depth parameter
            state.currentDepth = state.currentDepth * depthSmoothing + 
                               state.targetDepth * (1.0f - depthSmoothing);
            
            // Generate LFO with thermal drift
            float lfo = generateLFOWithThermal(state.lfoPhase, thermalFactor);
            
            // Apply stereo spread to LFO phase
            float stereoOffset = channel * m_stereoSpread.current * 0.5f;
            state.lfoPhase += 2.0f * M_PI * m_rate.current / m_sampleRate;
            if (state.lfoPhase > 2.0f * M_PI) {
                state.lfoPhase -= 2.0f * M_PI;
            }
            
            // Calculate modulated frequency with thermal and aging effects
            float centerFreqHz = (200.0f + m_centerFreq.current * 1800.0f) * thermalFactor; // 200Hz to 2kHz
            float modDepth = (state.currentDepth * 0.9f) * (1.0f - m_componentAge * 0.1f); // Aging reduces depth
            float modulatedFreq = centerFreqHz * std::pow(2.0f, lfo * modDepth);
            
            // Add feedback with enhanced soft clipping and aging
            float feedbackAmount = m_feedback.current * 0.95f * (1.0f + m_componentAge * 0.1f); // Aging increases feedback
            float resonanceBoost = (1.0f + m_resonance.current * 2.0f) * thermalFactor;
            input += softClipWithAging(state.feedbackSample * feedbackAmount * resonanceBoost, m_componentAge);
            
            // Process through all-pass stages with aging
            float output = input;
            for (int stage = 0; stage < activeStages; ++stage) {
                // Set frequency for this stage with component drift
                float stageFreq = modulatedFreq * std::pow(1.1f, stage) * (1.0f + state.componentDrift);
                state.allpassFilters[stage].setFrequency(stageFreq, m_sampleRate);
                
                // Process through all-pass with aging effects
                output = state.allpassFilters[stage].processWithAging(output, m_componentAge);
                
                // Add subtle noise from aging components
                if (m_componentAge > 0.01f) {
                    output += state.noiseLevel * ((rand() % 1000) / 1000.0f - 0.5f) * 2.0f;
                }
            }
            
            // Store feedback sample
            state.feedbackSample = output;
            
            // Apply analog-style saturation with aging
            output = softClipWithAging(output * 0.95f, m_componentAge) * 1.05f;
            
            // DC block output
            output = m_outputDCBlockers[channel].process(output);
            
            // Mix with dry signal
            channelData[sample] = drySignal * (1.0f - m_mix.current) + output * m_mix.current;
        }
    }
}

float AnalogPhaser::generateLFO(float phase) {
    // Triangle wave LFO for classic phaser sound
    float triangle = 2.0f * std::abs(2.0f * (phase / (2.0f * M_PI) - 0.5f)) - 1.0f;
    
    // Add slight sine shaping for smoother modulation
    float sine = std::sin(phase);
    
    // Blend for more musical modulation
    return triangle * 0.7f + sine * 0.3f;
}

float AnalogPhaser::generateLFOWithThermal(float phase, float thermalFactor) {
    float baseLFO = generateLFO(phase);
    
    // Add thermal instability
    float thermalModulation = (thermalFactor - 1.0f) * 5.0f; // Amplify thermal effect
    baseLFO += thermalModulation * std::sin(phase * 3.7f) * 0.1f; // Subtle thermal wobble
    
    return baseLFO;
}

int AnalogPhaser::getActiveStages() const {
    // 2, 4, 6, or 8 stages based on parameter
    if (m_stages.current < 0.25f) return 2;
    else if (m_stages.current < 0.5f) return 4;
    else if (m_stages.current < 0.75f) return 6;
    else return 8;
}

float AnalogPhaser::softClip(float input) {
    // Soft clipping for analog-style saturation
    float threshold = 0.7f;
    
    if (std::abs(input) < threshold) {
        return input;
    }
    
    float sign = input > 0.0f ? 1.0f : -1.0f;
    float excess = std::abs(input) - threshold;
    
    // Smooth clipping curve
    return sign * (threshold + std::tanh(excess * 2.0f) * (1.0f - threshold));
}

float AnalogPhaser::softClipWithAging(float input, float aging) {
    float clipped = softClip(input);
    
    // Aging adds slight nonlinearity and asymmetry
    if (aging > 0.01f) {
        float asymmetry = aging * 0.1f;
        if (clipped > 0) {
            clipped *= (1.0f + asymmetry);
        } else {
            clipped *= (1.0f - asymmetry * 0.5f);
        }
        
        // Add harmonic distortion from aging
        clipped += aging * 0.05f * clipped * clipped * clipped;
    }
    
    return clipped;
}

void AnalogPhaser::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_rate.target = params.at(0) * 10.0f; // 0-10 Hz
    if (params.count(1)) m_depth.target = params.at(1);
    if (params.count(2)) m_feedback.target = params.at(2) * 0.95f; // Max 95% feedback
    if (params.count(3)) m_stages.target = params.at(3);
    if (params.count(4)) m_stereoSpread.target = params.at(4);
    if (params.count(5)) m_centerFreq.target = params.at(5);
    if (params.count(6)) m_resonance.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String AnalogPhaser::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Feedback";
        case 3: return "Stages";
        case 4: return "Spread";
        case 5: return "Center";
        case 6: return "Resonance";
        case 7: return "Mix";
        default: return "";
    }
}
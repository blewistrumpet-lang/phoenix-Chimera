#include "ClassicTremolo.h"
#include <cmath>

ClassicTremolo::ClassicTremolo() {
    m_oscillators.resize(2);
    m_tubeModels.resize(2);
    
    // Initialize smooth parameters
    m_rate.setImmediate(5.0f);
    m_depth.setImmediate(0.5f);
    m_waveform.setImmediate(0.0f);
    m_stereoPhase.setImmediate(0.0f);
    m_volume.setImmediate(1.0f);
    
    // Set smoothing rates
    m_rate.setSmoothingRate(0.992f);
    m_depth.setSmoothingRate(0.995f);
    m_waveform.setSmoothingRate(0.998f);
    m_stereoPhase.setSmoothingRate(0.999f);
    m_volume.setSmoothingRate(0.995f);
}

void ClassicTremolo::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize oscillator states
    for (size_t i = 0; i < m_oscillators.size(); ++i) {
        m_oscillators[i].phase = 0.0f;
        m_oscillators[i].phaseIncrement = m_rate.current / sampleRate;
    }

void ClassicTremolo::reset() {
    // Reset modulation state
    m_lfoPhase = 0.0f;
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
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
    
    // Initialize tube models
    for (auto& tube : m_tubeModels) {
        tube.tubeState = 0.0f;
        tube.bias = 0.5f;
    }
}

void ClassicTremolo::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_rate.update();
    m_depth.update();
    m_waveform.update();
    m_stereoPhase.update();
    m_volume.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 6) { // Every 6 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00008f);
        m_sampleCount = 0;
    }
    
    // Apply stereo phase offset to second oscillator with thermal variation
    if (m_oscillators.size() > 1) {
        float phaseOffset = (m_stereoPhase.current / 360.0f) * thermalFactor;
        m_oscillators[1].phase = m_oscillators[0].phase + phaseOffset;
        while (m_oscillators[1].phase >= 1.0f) m_oscillators[1].phase -= 1.0f;
    }
    
    for (int channel = 0; channel < numChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            
            // DC block input
            input = m_inputDCBlockers[channel].process(input);
            
            // Process with boutique modeling
            float output = processChannelWithModeling(input, channel, thermalFactor, m_componentAge);
            
            // DC block output
            output = m_outputDCBlockers[channel].process(output);
            
            channelData[sample] = output;
        }
    }
}

void ClassicTremolo::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Map normalized 0-1 values to actual parameter ranges
    m_rate.target = 0.1f + getParam(0, 0.25f) * 19.9f;      // 0.1-20 Hz
    m_depth.target = getParam(1, 0.5f);                      // 0-1
    m_waveform.target = getParam(2, 0.0f);                   // 0-1
    m_stereoPhase.target = getParam(3, 0.0f) * 180.0f;      // 0-180 degrees
    m_volume.target = getParam(4, 1.0f);                     // 0-1
}

juce::String ClassicTremolo::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Waveform";
        case 3: return "Stereo";
        case 4: return "Volume";
        default: return "";
    }
}

float ClassicTremolo::generateWaveform(float phase, float waveformMix) {
    // Generate sine wave
    float sine = 0.5f + 0.5f * std::sin(2.0f * M_PI * phase);
    
    // Generate triangle wave
    float triangle;
    if (phase < 0.5f) {
        triangle = 2.0f * phase;
    } else {
        triangle = 2.0f * (1.0f - phase);
    }
    
    // Generate square wave with smooth transitions
    float square = smoothstep(0.45f, 0.55f, phase);
    
    // Interpolate between waveforms
    float result;
    if (waveformMix < 0.5f) {
        // Sine to triangle
        float mix = waveformMix * 2.0f;
        result = sine * (1.0f - mix) + triangle * mix;
    } else {
        // Triangle to square
        float mix = (waveformMix - 0.5f) * 2.0f;
        result = triangle * (1.0f - mix) + square * mix;
    }
    
    return result;
}

float ClassicTremolo::processChannelWithModeling(float input, int channel, float thermalFactor, float aging) {
    auto& osc = m_oscillators[channel % m_oscillators.size()];
    auto& tube = m_tubeModels[channel % m_tubeModels.size()];
    
    // Update LFO phase with thermal and aging effects
    float currentPhase = osc.tick(m_rate.current, m_sampleRate, thermalFactor, aging);
    
    // Generate modulation waveform with thermal variations
    float modulation = generateWaveform(currentPhase, m_waveform.current);
    
    // Add thermal noise to modulation
    if (aging > 0.01f) {
        modulation += aging * 0.05f * ((rand() % 1000) / 1000.0f - 0.5f);
        modulation = std::max(0.0f, std::min(1.0f, modulation));
    }
    
    // Different tremolo modes
    float output = input;
    
    switch (m_currentMode) {
        case TremoloMode::Classic: {
            // Classic amplitude modulation
            float gain = 1.0f - (m_depth.current * 0.5f * (1.0f - modulation));
            output *= gain;
            break;
        }
        
        case TremoloMode::Vintage: {
            // Vintage tremolo with bias modulation
            float biasModulation = 0.5f + (modulation - 0.5f) * m_depth.current;
            output = output * biasModulation + std::tanh(input * 0.5f) * (1.0f - biasModulation);
            break;
        }
        
        case TremoloMode::Modern: {
            // Modern tremolo with crossover distortion
            float gain = 1.0f - (m_depth.current * 0.6f * (1.0f - modulation));
            if (std::abs(output) < 0.1f) {
                output = std::tanh(output * gain * 3.0f) / 3.0f;
            } else {
                output *= gain;
            }
            break;
        }
        
        case TremoloMode::Tube: {
            // Tube tremolo modeling
            output = tube.process(input, modulation - 0.5f, aging);
            break;
        }
    }
    
    // Apply volume with aging compensation
    output *= m_volume.current * (1.0f - aging * 0.05f);
    
    // Soft limiting with aging
    if (std::abs(output) > 0.9f) {
        float saturation = 1.0f + aging * 0.1f;
        output = std::tanh(output * saturation) / saturation;
    }
    
    return output;
}

float ClassicTremolo::smoothstep(float edge0, float edge1, float x) {
    // Clamp x to [0, 1]
    x = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
    // Smooth interpolation
    return x * x * (3.0f - 2.0f * x);
}
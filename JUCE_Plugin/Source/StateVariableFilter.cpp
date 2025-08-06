// StateVariableFilter.cpp - Simplified Working Implementation
#include "StateVariableFilter.h"
#include <cmath>
#include <algorithm>

StateVariableFilter::StateVariableFilter() {
    // Initialize all parameter smoothers
    m_frequency = std::make_unique<ParameterSmoother>();
    m_resonance = std::make_unique<ParameterSmoother>();
    m_drive = std::make_unique<ParameterSmoother>();
    m_filterType = std::make_unique<ParameterSmoother>();
    m_slope = std::make_unique<ParameterSmoother>();
    m_envelope = std::make_unique<ParameterSmoother>();
    m_envAttack = std::make_unique<ParameterSmoother>();
    m_envRelease = std::make_unique<ParameterSmoother>();
    m_analog = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
    
    // Set defaults
    m_frequency->reset(0.5f);
    m_resonance->reset(0.2f);
    m_drive->reset(0.0f);
    m_filterType->reset(0.0f);
    m_slope->reset(0.25f);
    m_envelope->reset(0.0f);
    m_envAttack->reset(0.1f);
    m_envRelease->reset(0.3f);
    m_analog->reset(0.0f);
    m_mix->reset(1.0f);
}

void StateVariableFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = static_cast<float>(sampleRate);
    
    // Configure smoothers
    m_frequency->setSampleRate(sampleRate, 20.0f);
    m_resonance->setSampleRate(sampleRate, 30.0f);
    m_drive->setSampleRate(sampleRate, 30.0f);
    m_filterType->setSampleRate(sampleRate, 50.0f);
    m_slope->setSampleRate(sampleRate, 50.0f);
    m_envelope->setSampleRate(sampleRate, 30.0f);
    m_envAttack->setSampleRate(sampleRate, 40.0f);
    m_envRelease->setSampleRate(sampleRate, 40.0f);
    m_analog->setSampleRate(sampleRate, 50.0f);
    m_mix->setSampleRate(sampleRate, 20.0f);
    
    reset();
}

void StateVariableFilter::reset() {
    for (auto& filter : m_filters) {
        filter.reset();
    }
    for (auto& env : m_envelopes) {
        env.reset();
    }
}

void StateVariableFilter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    if (numChannels == 1) {
        float* data = buffer.getWritePointer(0);
        processStereo(data, data, numSamples);
    } else {
        float* left = buffer.getWritePointer(0);
        float* right = numChannels > 1 ? buffer.getWritePointer(1) : left;
        processStereo(left, right, numSamples);
    }
}

void StateVariableFilter::processStereo(float* left, float* right, int numSamples) {
    // Update parameters
    float frequency = m_frequency->process();
    float resonance = m_resonance->process();
    float drive = m_drive->process();
    float filterTypeParam = m_filterType->process();
    float slopeParam = m_slope->process();
    float envelopeAmount = m_envelope->process();
    float envAttack = m_envAttack->process();
    float envRelease = m_envRelease->process();
    float analogAmount = m_analog->process();
    float mix = m_mix->process();
    
    // Convert to usable values
    float baseFreq = 20.0f * std::pow(1000.0f, frequency);
    float q = 0.5f + resonance * 19.5f;
    
    // Determine filter type
    MultiModeFilter::Type filterType = getFilterTypeFromParam(filterTypeParam);
    
    // Set stages based on slope
    int numStages = 1;
    if (slopeParam < 0.25f) numStages = 1;
    else if (slopeParam < 0.5f) numStages = 2;
    else if (slopeParam < 0.75f) numStages = 3;
    else numStages = 4;
    
    for (int ch = 0; ch < 2; ++ch) {
        m_filters[ch].setNumStages(numStages);
        m_drives[ch].setDrive(drive);
        m_envelopes[ch].setTimes(
            0.1f + envAttack * 99.9f,
            1.0f + envRelease * 999.0f,
            m_sampleRate
        );
    }
    
    // Process channels
    float* channels[2] = {left, right};
    
    for (int ch = 0; ch < 2; ++ch) {
        float* data = channels[ch];
        
        for (int i = 0; i < numSamples; ++i) {
            float input = data[i];
            float dry = input;
            
            // Apply drive
            if (drive > 0.01f) {
                input = m_drives[ch].process(input);
            }
            
            // Get envelope
            float envelope = 0.0f;
            if (envelopeAmount > 0.01f) {
                envelope = m_envelopes[ch].process(input);
            }
            
            // Calculate modulated frequency
            float modulatedFreq = baseFreq;
            if (envelopeAmount > 0.01f) {
                float envMod = envelope * envelopeAmount * 4.0f;
                modulatedFreq *= std::pow(2.0f, envMod);
                modulatedFreq = std::min(modulatedFreq, m_sampleRate * 0.49f);
            }
            
            // Process through filter
            float filtered = m_filters[ch].process(input, filterType, modulatedFreq, q, m_sampleRate);
            
            // Add analog noise
            if (analogAmount > 0.01f) {
                float noise = m_noiseDist(m_noiseGen) * analogAmount * 0.00001f;
                filtered += noise;
            }
            
            // Mix dry/wet
            data[i] = filtered * mix + dry * (1.0f - mix);
        }
    }
}

StateVariableFilter::MultiModeFilter::Type StateVariableFilter::getFilterTypeFromParam(float param) const {
    if (param < 0.111f) return MultiModeFilter::Type::LOWPASS;
    else if (param < 0.222f) return MultiModeFilter::Type::HIGHPASS;
    else if (param < 0.333f) return MultiModeFilter::Type::BANDPASS;
    else if (param < 0.444f) return MultiModeFilter::Type::NOTCH;
    else if (param < 0.555f) return MultiModeFilter::Type::LOWPASS_2;
    else if (param < 0.666f) return MultiModeFilter::Type::HIGHPASS_2;
    else if (param < 0.777f) return MultiModeFilter::Type::BANDPASS_2;
    else if (param < 0.888f) return MultiModeFilter::Type::NOTCH_2;
    else return MultiModeFilter::Type::LOWPASS_4;
}

void StateVariableFilter::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (index) {
            case 0: m_frequency->setTarget(value); break;
            case 1: m_resonance->setTarget(value); break;
            case 2: m_drive->setTarget(value); break;
            case 3: m_filterType->setTarget(value); break;
            case 4: m_slope->setTarget(value); break;
            case 5: m_envelope->setTarget(value); break;
            case 6: m_envAttack->setTarget(value); break;
            case 7: m_envRelease->setTarget(value); break;
            case 8: m_analog->setTarget(value); break;
            case 9: m_mix->setTarget(value); break;
        }
    }
}

juce::String StateVariableFilter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Frequency";
        case 1: return "Resonance";
        case 2: return "Drive";
        case 3: return "Filter Type";
        case 4: return "Slope";
        case 5: return "Envelope";
        case 6: return "Env Attack";
        case 7: return "Env Release";
        case 8: return "Analog";
        case 9: return "Mix";
        default: return "";
    }
}
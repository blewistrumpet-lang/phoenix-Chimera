#include "VintageTubePreamp.h"
#include <cmath>

// Implementation of tube saturation function
float VintageTubePreamp::AdvancedTubeStage::processTubeSaturation(float input, TubeType type, float bias) {
    // Simple tube saturation model
    float biased = input + bias * 0.1f;
    float output;
    
    switch (type) {
        case TUBE_12AX7:
            output = std::tanh(biased * 3.0f) / 3.0f;
            break;
        case TUBE_12AU7:
            output = std::tanh(biased * 2.0f) / 2.0f;
            break;
        case TUBE_6V6:
            output = std::tanh(biased * 1.5f) / 1.5f;
            break;
        case TUBE_EL34:
        default:
            output = std::tanh(biased * 1.8f) / 1.8f;
            break;
    }
    
    return output - bias * 0.1f;
}

// Implementation of tube transconductance function
float VintageTubePreamp::AdvancedTubeStage::getTubeGm(TubeType type) {
    switch (type) {
        case TUBE_12AX7:
            return 1.6f;  // mA/V typical
        case TUBE_12AU7:
            return 2.2f;
        case TUBE_6V6:
            return 4.1f;
        case TUBE_EL34:
        default:
            return 11.0f;
    }
}

// Implementation of tube amplification factor
float VintageTubePreamp::AdvancedTubeStage::getTubeMu(TubeType type) {
    switch (type) {
        case TUBE_12AX7:
            return 100.0f;  // Typical amplification factor
        case TUBE_12AU7:
            return 20.0f;
        case TUBE_6V6:
            return 12.6f;
        case TUBE_EL34:
        default:
            return 10.5f;
    }
}

// Implementation of tube plate resistance
float VintageTubePreamp::AdvancedTubeStage::getTubeRp(TubeType type) {
    switch (type) {
        case TUBE_12AX7:
            return 62500.0f;  // Ohms typical
        case TUBE_12AU7:
            return 7700.0f;
        case TUBE_6V6:
            return 50000.0f;
        case TUBE_EL34:
        default:
            return 15000.0f;
    }
}

// Implementation of tube harmonics function
float VintageTubePreamp::AdvancedTubeStage::addTubeHarmonics(float input, float drive, TubeType type, float aging) {
    // Simple harmonic generation based on tube type
    float output = input;
    float harmonic2, harmonic3;
    
    switch (type) {
        case TUBE_12AX7:
            harmonic2 = input * input * 0.1f * drive;
            harmonic3 = input * input * input * 0.03f * drive;
            break;
        case TUBE_12AU7:
            harmonic2 = input * input * 0.08f * drive;
            harmonic3 = input * input * input * 0.04f * drive;
            break;
        case TUBE_6V6:
            harmonic2 = input * input * 0.05f * drive;
            harmonic3 = input * input * input * 0.08f * drive;
            break;
        case TUBE_EL34:
        default:
            harmonic2 = input * input * 0.06f * drive;
            harmonic3 = input * input * input * 0.07f * drive;
            break;
    }
    
    // Aging affects harmonic content
    harmonic2 *= (1.0f + aging * 0.2f);
    harmonic3 *= (1.0f + aging * 0.15f);
    
    output += harmonic2 - harmonic3;
    return output;
}

VintageTubePreamp::VintageTubePreamp() {
    // Initialize smoothed parameters with boutique defaults
    m_inputGain.setImmediate(0.5f);
    m_warmth.setImmediate(0.5f);
    m_presence.setImmediate(0.5f);
    m_tubeDrive.setImmediate(0.5f);
    m_bias.setImmediate(0.5f);
    m_tone.setImmediate(0.5f);
    m_outputGain.setImmediate(0.5f);
    m_mix.setImmediate(1.0f);
    m_tubeType.setImmediate(0.0f);     // 12AX7 default
    m_saturation.setImmediate(0.3f);   // Moderate saturation
    
    // Set different smoothing rates
    m_inputGain.setSmoothingRate(0.99f);
    m_warmth.setSmoothingRate(0.995f);
    m_presence.setSmoothingRate(0.995f);
    m_tubeDrive.setSmoothingRate(0.99f);
    m_bias.setSmoothingRate(0.98f);        // Slower for bias changes
    m_tone.setSmoothingRate(0.995f);
    m_outputGain.setSmoothingRate(0.99f);
    m_mix.setSmoothingRate(0.995f);
    m_tubeType.setSmoothingRate(0.95f);    // Slowest for tube type changes
    m_saturation.setSmoothingRate(0.99f);
}

void VintageTubePreamp::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Reset tone stacks
    for (auto& toneStack : m_toneStacks) {
        toneStack.reset();
    }
    
    // Reset DC blockers
    for (auto& blocker : m_inputDCBlockers) {
        blocker.reset();
    }
    for (auto& blocker : m_outputDCBlockers) {
        blocker.reset();
    }
    
    // Prepare oversampler
    m_oversampler.prepare(samplesPerBlock);
    
    // Reset aging and thermal
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void VintageTubePreamp::reset() {
    // Reset all internal state
    for (auto& toneStack : m_toneStacks) {
        toneStack.reset();
    }
    for (auto& blocker : m_inputDCBlockers) {
        blocker.reset();
    }
    for (auto& blocker : m_outputDCBlockers) {
        blocker.reset();
    }
}

void VintageTubePreamp::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    float inputLevel = std::pow(10.0f, (m_inputGain.current - 0.5f) * 2.0f);
    float outputLevel = std::pow(10.0f, (m_outputGain.current - 0.5f) * 2.0f);
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& tubeStage = m_tubeStages[channel];
        auto& toneStack = m_toneStacks[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float dry = input;
            
            input *= inputLevel;
            
            // Warmth (low frequency emphasis)
            if (m_warmth.current > 0.5f) {
                float warmthAmount = (m_warmth.current - 0.5f) * 2.0f;
                input *= (1.0f + warmthAmount * 0.3f);
            }
            
            // Tube processing
            float processed = tubeStage.process(input, m_tubeDrive.current, m_bias.current, 
                                                VintageTubePreamp::AdvancedTubeStage::TUBE_12AX7, 1.0f);
            
            // Tone stack - using m_tone for all three bands
            processed = toneStack.process(processed, m_tone.current, 0.5f, m_presence.current, m_sampleRate);
            
            // Output stage
            processed *= outputLevel;
            
            // Soft clipping
            if (std::abs(processed) > 0.9f) {
                processed = std::tanh(processed * 0.8f) * 1.125f;
            }
            
            channelData[sample] = processed * m_mix.current + dry * (1.0f - m_mix.current);
        }
    }
}

void VintageTubePreamp::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_inputGain.target = params.at(0);
    if (params.count(1)) m_warmth.target = params.at(1);
    if (params.count(2)) m_presence.target = params.at(2);
    if (params.count(3)) m_tubeDrive.target = params.at(3);
    if (params.count(4)) m_bias.target = params.at(4);
    if (params.count(5)) m_tone.target = params.at(5);
    if (params.count(6)) m_outputGain.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
    if (params.count(8)) m_tubeType.target = params.at(8);
    if (params.count(9)) m_saturation.target = params.at(9);
}

juce::String VintageTubePreamp::getParameterName(int index) const {
    switch (index) {
        case 0: return "Input Gain";
        case 1: return "Warmth";
        case 2: return "Presence";
        case 3: return "Tube Drive";
        case 4: return "Bias";
        case 5: return "Tone";
        case 6: return "Output Gain";
        case 7: return "Mix";
        case 8: return "Tube Type";
        case 9: return "Saturation";
        default: return "";
    }
}
#include "VocalFormantFilter.h"
#include <cmath>
#include <algorithm>
#include <random>

// Enhanced formant frequencies for standard vowels with improved accuracy
const VocalFormantFilter::FormantSet VocalFormantFilter::vowelFormants[5] = {
    // F1,   F2,    F3,   Q1,  Q2,  Q3
    { 700,  1220,  2600,  10,  12,  15 },  // A (as in "father")
    { 570,  2090,  2840,  10,  15,  20 },  // E (as in "bed")
    { 300,  2290,  3010,  12,  20,  22 },  // I (as in "beat")
    { 590,  880,   2540,  10,  12,  15 },  // O (as in "boat")
    { 440,  1020,  2240,  10,  12,  18 }   // U (as in "boot")
};

VocalFormantFilter::VocalFormantFilter() {
    // Initialize smoothed parameters
    m_vowel1.reset(0.0f);
    m_vowel2.reset(2.0f);
    m_morphAmount.reset(0.0f);
    m_resonance.reset(0.5f);
    m_brightness.reset(0.5f);
    m_modRate.reset(0.0f);
    m_modDepth.reset(0.0f);
    m_mix.reset(1.0f);
}

void VocalFormantFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    m_vowel1.setSmoothingTime(100.0f, sampleRate);       // Slow vowel transitions
    m_vowel2.setSmoothingTime(100.0f, sampleRate);
    m_morphAmount.setSmoothingTime(50.0f, sampleRate);
    m_resonance.setSmoothingTime(20.0f, sampleRate);
    m_brightness.setSmoothingTime(30.0f, sampleRate);
    m_modRate.setSmoothingTime(200.0f, sampleRate);
    m_modDepth.setSmoothingTime(100.0f, sampleRate);
    m_mix.setSmoothingTime(50.0f, sampleRate);
    
    // Reset channel states
    for (auto& state : m_channelStates) {
        state.reset();
        
        // Set envelope follower times
        state.envelopeAttack = std::exp(-1.0f / (0.001f * sampleRate));  // 1ms attack
        state.envelopeRelease = std::exp(-1.0f / (0.01f * sampleRate));   // 10ms release
    }

void VocalFormantFilter::reset() {
    // Reset filter states
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
}

}

void VocalFormantFilter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update thermal modeling
    m_thermalModel.update(m_sampleRate);
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smoothed parameters
            m_vowel1.update();
            m_vowel2.update();
            m_morphAmount.update();
            m_resonance.update();
            m_brightness.update();
            m_modRate.update();
            m_modDepth.update();
            m_mix.update();
            
            float input = channelData[sample];
            float drySignal = input;
            
            // Apply DC blocking
            input = m_dcBlockers[channel].process(input);
            
            // Enhanced envelope follower with better dynamics
            float inputAbs = std::abs(input);
            float targetEnv = inputAbs > state.envelope ? inputAbs : inputAbs * 0.7f;
            float rate = targetEnv > state.envelope ? state.envelopeAttack : state.envelopeRelease;
            state.envelope = targetEnv + (state.envelope - targetEnv) * rate;
            
            // Enhanced modulation with thermal variations
            float modulation = 0.0f;
            if (m_modRate.current > 0.0f && m_modDepth.current > 0.0f) {
                float thermalFactor = m_thermalModel.getThermalFactor();
                float modFreq = m_modRate.current * 10.0f * thermalFactor; // 0-10 Hz with thermal drift
                modulation = std::sin(state.modulationPhase) * m_modDepth.current;
                state.modulationPhase += 2.0f * M_PI * modFreq / m_sampleRate;
                if (state.modulationPhase > 2.0f * M_PI) {
                    state.modulationPhase -= 2.0f * M_PI;
                }
            }
            
            // Calculate current morph position with modulation and envelope
            float morphPos = m_morphAmount.current + modulation;
            morphPos = std::max(0.0f, std::min(1.0f, morphPos));
            
            // Interpolate between vowels with thermal compensation
            float vowel1Index = m_vowel1.current * 4.0f;
            float vowel2Index = m_vowel2.current * 4.0f;
            float vowelIndex = vowel1Index + (vowel2Index - vowel1Index) * morphPos;
            FormantSet currentFormants = interpolateFormants(vowelIndex);
            
            // Apply resonance scaling with thermal drift
            float thermalFactor = m_thermalModel.getThermalFactor();
            float resonanceScale = (0.5f + m_resonance.current * 2.0f) * thermalFactor;
            currentFormants.q1 *= resonanceScale;
            currentFormants.q2 *= resonanceScale;
            currentFormants.q3 *= resonanceScale;
            
            // Dynamic formant modulation based on input envelope with component aging
            state.componentAge += 0.0001f / m_sampleRate; // Slow aging
            float agingFactor = 1.0f - state.componentAge * 0.01f;
            float dynamicShift = (1.0f + state.envelope * 0.2f) * agingFactor;
            currentFormants.f1 *= dynamicShift;
            currentFormants.f2 *= dynamicShift;
            currentFormants.f3 *= dynamicShift;
            
            // Process through enhanced formant filters with drive and vintage mode
            float output = 0.0f;
            bool useVintageMode = false; // Could be a parameter in future
            float driveAmount = 0.0f;    // Could be a parameter in future
            
            output += state.formantFilters[0].process(input, currentFormants.f1, 
                                                     currentFormants.q1, m_sampleRate, driveAmount, useVintageMode) * 0.5f;
            output += state.formantFilters[1].process(input, currentFormants.f2, 
                                                     currentFormants.q2, m_sampleRate, driveAmount, useVintageMode) * 0.35f;
            output += state.formantFilters[2].process(input, currentFormants.f3, 
                                                     currentFormants.q3, m_sampleRate, driveAmount, useVintageMode) * 0.15f;
            
            // Apply brightness control with thermal-compensated high shelf
            if (m_brightness.current != 0.5f) {
                float shelfFreq = 2000.0f + m_brightness.current * 6000.0f;
                float shelfGain = 0.5f + m_brightness.current;
                output = processHighShelf(output, state.highShelfState, shelfFreq, shelfGain, thermalFactor);
            }
            
            // Enhanced soft saturation with analog character
            if (std::abs(output) > 0.8f) {
                output = softClip(output);
            }
            
            // Mix with dry signal using smoothed parameter
            channelData[sample] = drySignal * (1.0f - m_mix.current) + output * m_mix.current;
        }
    }
}

VocalFormantFilter::FormantSet VocalFormantFilter::interpolateFormants(float vowelIndex) {
    // Clamp index
    vowelIndex = std::max(0.0f, std::min(4.0f, vowelIndex));
    
    // Get integer and fractional parts
    int baseIndex = static_cast<int>(vowelIndex);
    float fraction = vowelIndex - baseIndex;
    
    // Handle edge case
    if (baseIndex >= 4) {
        return vowelFormants[4];
    }
    
    // Linear interpolation between adjacent vowels
    const FormantSet& vowel1 = vowelFormants[baseIndex];
    const FormantSet& vowel2 = vowelFormants[std::min(baseIndex + 1, 4)];
    
    FormantSet result;
    result.f1 = vowel1.f1 + (vowel2.f1 - vowel1.f1) * fraction;
    result.f2 = vowel1.f2 + (vowel2.f2 - vowel1.f2) * fraction;
    result.f3 = vowel1.f3 + (vowel2.f3 - vowel1.f3) * fraction;
    result.q1 = vowel1.q1 + (vowel2.q1 - vowel1.q1) * fraction;
    result.q2 = vowel1.q2 + (vowel2.q2 - vowel1.q2) * fraction;
    result.q3 = vowel1.q3 + (vowel2.q3 - vowel1.q3) * fraction;
    
    return result;
}

// Enhanced FormantFilter process implementation with analog modeling
float VocalFormantFilter::FormantFilter::process(float input, float freq, float q, double sampleRate, float drive, bool vintageMode) {
    // Update component drift slowly
    componentDrift += (((rand() % 1000) / 1000.0f - 0.5f) * 0.0001f) / sampleRate;
    componentDrift = std::max(-0.01f, std::min(0.01f, componentDrift));
    
    // Apply component drift to parameters
    float adjustedFreq = freq * (1.0f + componentDrift);
    float adjustedQ = q * (1.0f + componentDrift * 0.5f);
    
    // Clamp to reasonable bounds
    adjustedFreq = std::max(20.0f, std::min(adjustedFreq, (float)sampleRate * 0.45f));
    adjustedQ = std::max(0.5f, std::min(adjustedQ, 30.0f));
    
    float w = 2.0f * std::sin(M_PI * adjustedFreq / sampleRate);
    float q_inv = 1.0f / adjustedQ;
    
    // Apply drive/saturation if enabled
    float processedInput = input;
    if (drive > 0.01f) {
        if (vintageMode) {
            processedInput = std::tanh(input * (1.0f + drive * 2.0f)) / (1.0f + drive * 0.5f);
        } else {
            processedInput = std::tanh(input * (1.0f + drive)) / (1.0f + drive * 0.3f);
        }
    }
    
    // Enhanced state variable filter with better numerical stability
    float bandpass = state2 * w + state1;
    float highpass = processedInput - bandpass * q_inv - state1;
    
    // Add subtle saturation in the integrators for vintage character
    if (vintageMode && drive > 0.1f) {
        state1 += std::tanh(state2 * w * (1.0f + drive * 0.1f)) / (1.0f + drive * 0.05f);
        state2 = std::tanh(highpass * (1.0f + drive * 0.05f)) / (1.0f + drive * 0.02f);
    } else {
        state1 += state2 * w;
        state2 = highpass;
    }
    
    return bandpass;
}

float VocalFormantFilter::processHighShelf(float input, float& state, float frequency, float gain, float thermalFactor) {
    // Enhanced high shelf filter with thermal compensation
    float adjustedFreq = frequency * thermalFactor;
    adjustedFreq = std::max(100.0f, std::min(adjustedFreq, (float)m_sampleRate * 0.45f));
    
    float w = 2.0f * std::sin(M_PI * adjustedFreq / m_sampleRate);
    float a = (gain - 1.0f) * 0.5f;
    
    float highpass = input - state;
    state += highpass * w;
    
    // Add subtle nonlinearity for analog character
    float output = input + highpass * a;
    if (std::abs(a) > 0.1f) {
        output = std::tanh(output * 0.9f) / 0.9f;
    }
    
    return output;
}

float VocalFormantFilter::analogSaturation(float input, float amount) {
    // Analog-style saturation with even harmonics
    float driven = input * (1.0f + amount * 2.0f);
    return std::tanh(driven * 0.8f) / (0.8f * (1.0f + amount * 0.3f));
}

float VocalFormantFilter::vintageTubeDistortion(float input, float amount) {
    // Vintage tube-like distortion with asymmetry
    float driven = input * (1.0f + amount * 3.0f);
    
    // Asymmetric clipping for vintage character
    if (driven > 0.0f) {
        return std::tanh(driven * 0.7f) / (0.7f * (1.0f + amount * 0.2f));
    } else {
        return std::tanh(driven * 0.9f) / (0.9f * (1.0f + amount * 0.1f));
    }
}

void VocalFormantFilter::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_vowel1.target = params.at(0); // 0-1 (normalized)
    if (params.count(1)) m_vowel2.target = params.at(1);
    if (params.count(2)) m_morphAmount.target = params.at(2);
    if (params.count(3)) m_resonance.target = params.at(3);
    if (params.count(4)) m_brightness.target = params.at(4);
    if (params.count(5)) m_modRate.target = params.at(5); // 0-1 (normalized)
    if (params.count(6)) m_modDepth.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String VocalFormantFilter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Vowel 1";
        case 1: return "Vowel 2";
        case 2: return "Morph";
        case 3: return "Resonance";
        case 4: return "Brightness";
        case 5: return "Mod Rate";
        case 6: return "Mod Depth";
        case 7: return "Mix";
        default: return "";
    }
}
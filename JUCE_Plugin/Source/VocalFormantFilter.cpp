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
    // Initialize parameters
    m_vowel1 = 0.0f;
    m_vowel2 = 2.0f;
    m_morphAmount = 0.0f;
    m_resonance = 0.5f;
    m_brightness = 0.5f;
    m_modRate = 0.0f;
    m_modDepth = 0.0f;
    m_mix = 1.0f;
}

void VocalFormantFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Parameters are already initialized in constructor
    // No smoothing setup needed for plain floats
    
    // Reset channel states
    for (auto& state : m_channelStates) {
        // Reset formant filters
        for (auto& filter : state.formantFilters) {
            filter.reset();
        }
        // Reset other state
        state.modulationPhase = 0.0f;
        state.envelope = 0.0f;
        state.highShelfState = 0.0f;
    }
}

void VocalFormantFilter::reset() {
    // Reset filter states
    for (auto& state : m_channelStates) {
        for (auto& filter : state.formantFilters) {
            filter.reset();
        }
        state.modulationPhase = 0.0f;
        state.envelope = 0.0f;
        state.highShelfState = 0.0f;
    }
}

void VocalFormantFilter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Thermal factor simulation (simple)
    float thermalFactor = 1.0f;
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Parameters are plain floats, no update needed
            
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
            if (m_modRate > 0.0f && m_modDepth > 0.0f) {
                float modFreq = m_modRate * 10.0f * thermalFactor; // 0-10 Hz with thermal drift
                modulation = std::sin(state.modulationPhase) * m_modDepth;
                state.modulationPhase += 2.0f * M_PI * modFreq / m_sampleRate;
                if (state.modulationPhase > 2.0f * M_PI) {
                    state.modulationPhase -= 2.0f * M_PI;
                }
            }
            
            // Calculate current morph position with modulation and envelope
            float morphPos = m_morphAmount + modulation;
            morphPos = std::max(0.0f, std::min(1.0f, morphPos));
            
            // Interpolate between vowels with thermal compensation
            float vowel1Index = m_vowel1 * 4.0f;
            float vowel2Index = m_vowel2 * 4.0f;
            float vowelIndex = vowel1Index + (vowel2Index - vowel1Index) * morphPos;
            FormantSet currentFormants = interpolateFormants(vowelIndex);
            
            // Apply resonance scaling with thermal drift
            float thermalFactor = 1.0f; // Simplified thermal model
            float resonanceScale = (0.5f + m_resonance * 2.0f) * thermalFactor;
            currentFormants.q1 *= resonanceScale;
            currentFormants.q2 *= resonanceScale;
            currentFormants.q3 *= resonanceScale;
            
            // Dynamic formant modulation based on input envelope with component aging
            // Component aging simulation removed for simplicity
            float agingFactor = 1.0f;
            float dynamicShift = (1.0f + state.envelope * 0.2f) * agingFactor;
            currentFormants.f1 *= dynamicShift;
            currentFormants.f2 *= dynamicShift;
            currentFormants.f3 *= dynamicShift;
            
            // Process through enhanced formant filters with drive and vintage mode
            float output = 0.0f;
            bool useVintageMode = false; // Could be a parameter in future
            float driveAmount = 0.0f;    // Could be a parameter in future
            
            output += state.formantFilters[0].process(input, currentFormants.f1, 
                                                     currentFormants.q1, m_sampleRate) * 0.5f;
            output += state.formantFilters[1].process(input, currentFormants.f2, 
                                                     currentFormants.q2, m_sampleRate) * 0.35f;
            output += state.formantFilters[2].process(input, currentFormants.f3, 
                                                     currentFormants.q3, m_sampleRate) * 0.15f;
            
            // Apply brightness control with thermal-compensated high shelf
            if (m_brightness != 0.5f) {
                float shelfFreq = 2000.0f + m_brightness * 6000.0f;
                float shelfGain = 0.5f + m_brightness;
                output = processHighShelf(output, state.highShelfState, shelfFreq, shelfGain, thermalFactor);
            }
            
            // Enhanced soft saturation with analog character
            if (std::abs(output) > 0.8f) {
                output = softClip(output);
            }
            
            // Mix with dry signal using smoothed parameter
            channelData[sample] = drySignal * (1.0f - m_mix) + output * m_mix;
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

float VocalFormantFilter::processHighShelf(float input, float& state, float freq, float gain, float thermalFactor) {
    float adjustedFreq = freq * thermalFactor;
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
    if (params.count(0)) m_vowel1 = params.at(0); // 0-1 (normalized)
    if (params.count(1)) m_vowel2 = params.at(1);
    if (params.count(2)) m_morphAmount = params.at(2);
    if (params.count(3)) m_resonance = params.at(3);
    if (params.count(4)) m_brightness = params.at(4);
    if (params.count(5)) m_modRate = params.at(5); // 0-1 (normalized)
    if (params.count(6)) m_modDepth = params.at(6);
    if (params.count(7)) m_mix = params.at(7);
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
#include "FormantFilter.h"
#include <cmath>
#include <algorithm>
#include <random>

// Enhanced vowel formant data with more accurate frequencies
const FormantFilter::FormantData FormantFilter::VOWEL_A = {730.0f, 1090.0f, 2440.0f, 1.0f, 0.5f, 0.25f};
const FormantFilter::FormantData FormantFilter::VOWEL_E = {270.0f, 2290.0f, 3010.0f, 1.0f, 0.4f, 0.2f};
const FormantFilter::FormantData FormantFilter::VOWEL_I = {390.0f, 1990.0f, 2550.0f, 1.0f, 0.35f, 0.15f};
const FormantFilter::FormantData FormantFilter::VOWEL_O = {570.0f, 840.0f, 2410.0f, 1.0f, 0.45f, 0.2f};
const FormantFilter::FormantData FormantFilter::VOWEL_U = {440.0f, 1020.0f, 2240.0f, 1.0f, 0.3f, 0.15f};

FormantFilter::FormantFilter() {
    // Initialize smoothed parameters
    m_vowelPosition.reset(0.0f);
    m_formantShift.reset(0.5f);
    m_resonance.reset(0.4f);
    m_morph.reset(0.0f);
    m_drive.reset(0.1f);
    m_vintageMode.reset(0.0f);
}

void FormantFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    m_vowelPosition.setSmoothingTime(50.0f, sampleRate);     // Smooth vowel transitions
    m_formantShift.setSmoothingTime(30.0f, sampleRate);
    m_resonance.setSmoothingTime(20.0f, sampleRate);
    m_morph.setSmoothingTime(100.0f, sampleRate);            // Slower for morph
    m_drive.setSmoothingTime(100.0f, sampleRate);
    m_vintageMode.setSmoothingTime(500.0f, sampleRate);
    
    // Initialize 3 formant filters per channel
    for (int ch = 0; ch < 2; ++ch) {
        for (int f = 0; f < 3; ++f) {
            auto& formant = m_formantFilters[ch][f];
            
            // Initialize with default vowel (A) frequencies
            if (f == 0) {
                formant.freq = VOWEL_A.f1;
                formant.gain = VOWEL_A.a1;
            } else if (f == 1) {
                formant.freq = VOWEL_A.f2;
                formant.gain = VOWEL_A.a2;
            } else {
                formant.freq = VOWEL_A.f3;
                formant.gain = VOWEL_A.a3;
            }
            
            // Set Q values based on resonance
            formant.q = 2.0f + m_resonance.current * 6.0f; // Q from 2 to 8
            formant.reset();
            calculateFilterCoefficients(formant, 1.0f);
        }
    }
}

void FormantFilter::reset() {
    // Reset filter states
    for (auto& channel : m_formantFilters) {
        for (auto& filter : channel) {
            filter.reset();
        }
    }
    
    // Reset DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.x1 = blocker.y1 = 0.0f;
    }
}

void FormantFilter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update thermal modeling
    m_thermalModel.update(m_sampleRate);
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smoothed parameters
            m_vowelPosition.update();
            m_formantShift.update();
            m_resonance.update();
            m_morph.update();
            m_drive.update();
            m_vintageMode.update();
            
            channelData[sample] = processSample(channelData[sample], channel);
        }
    }
}

float FormantFilter::processSample(float input, int channel) {
    // Apply DC blocking
    float cleanInput = m_dcBlockers[channel].process(input);
    
    // Get current vowel formant data
    FormantData currentFormant = interpolateVowels(m_vowelPosition.current, m_morph.current);
    
    // Update formant filters with thermal compensation
    float thermalFactor = m_thermalModel.getThermalFactor();
    updateFormantFilters(channel, currentFormant);
    
    // Apply input drive for analog character
    float driveAmount = 1.0f + m_drive.current * 3.0f;
    float drivenInput = cleanInput;
    
    if (m_drive.current > 0.01f) {
        if (m_vintageMode.current > 0.5f) {
            drivenInput = vintageDistortion(cleanInput, m_drive.current);
        } else {
            drivenInput = analogSaturation(cleanInput, m_drive.current);
        }
    }
    
    // Process through each formant filter and sum with proper weighting
    float formantSum = 0.0f;
    bool isVintage = m_vintageMode.current > 0.5f;
    
    for (int i = 0; i < 3; ++i) {
        float formantOutput = processFormantFilter(drivenInput, m_formantFilters[channel][i], 
                                                 m_drive.current, isVintage);
        
        // Apply formant gain with slight interaction between formants
        float gain = m_formantFilters[channel][i].gain;
        if (i > 0 && formantSum != 0.0f) {
            // Subtle interaction between formants (analog characteristic)
            gain *= (1.0f - std::abs(formantSum) * 0.1f);
        }
        
        formantSum += formantOutput * gain;
    }
    
    // Apply resonance-dependent makeup gain
    float makeupGain = 1.0f + m_resonance.current * 0.3f;
    formantSum *= makeupGain;
    
    // Soft clipping to prevent harsh peaks
    if (std::abs(formantSum) > 0.8f) {
        formantSum = softClip(formantSum);
    }
    
    // Mix with dry signal for natural sound
    float dryLevel = 0.2f + m_vintageMode.current * 0.2f;  // More dry in vintage mode
    float wetLevel = 0.8f - m_vintageMode.current * 0.1f;
    
    return input * dryLevel + formantSum * wetLevel;
}

FormantFilter::FormantData FormantFilter::interpolateVowels(float vowelPos, float morph) {
    FormantData result;
    
    // Determine which vowels to interpolate between
    const FormantData* vowel1;
    const FormantData* vowel2;
    float interpFactor;
    
    if (vowelPos < 0.25f) {
        // Between A and E
        vowel1 = &VOWEL_A;
        vowel2 = &VOWEL_E;
        interpFactor = vowelPos * 4.0f;
    } else if (vowelPos < 0.5f) {
        // Between E and I
        vowel1 = &VOWEL_E;
        vowel2 = &VOWEL_I;
        interpFactor = (vowelPos - 0.25f) * 4.0f;
    } else if (vowelPos < 0.75f) {
        // Between I and O
        vowel1 = &VOWEL_I;
        vowel2 = &VOWEL_O;
        interpFactor = (vowelPos - 0.5f) * 4.0f;
    } else {
        // Between O and U
        vowel1 = &VOWEL_O;
        vowel2 = &VOWEL_U;
        interpFactor = (vowelPos - 0.75f) * 4.0f;
    }
    
    // Apply morph parameter to interpolation
    float morphedFactor = interpFactor * (1.0f + morph);
    morphedFactor = std::max(0.0f, std::min(1.0f, morphedFactor));
    
    // Interpolate formant frequencies and amplitudes
    result.f1 = vowel1->f1 + morphedFactor * (vowel2->f1 - vowel1->f1);
    result.f2 = vowel1->f2 + morphedFactor * (vowel2->f2 - vowel1->f2);
    result.f3 = vowel1->f3 + morphedFactor * (vowel2->f3 - vowel1->f3);
    
    result.a1 = vowel1->a1 + morphedFactor * (vowel2->a1 - vowel1->a1);
    result.a2 = vowel1->a2 + morphedFactor * (vowel2->a2 - vowel1->a2);
    result.a3 = vowel1->a3 + morphedFactor * (vowel2->a3 - vowel1->a3);
    
    // Apply formant shift (frequency scaling)
    float shiftFactor = 0.5f + m_formantShift.current * 1.5f; // 0.5x to 2x frequency
    result.f1 *= shiftFactor;
    result.f2 *= shiftFactor;
    result.f3 *= shiftFactor;
    
    // Clamp frequencies to reasonable ranges
    result.f1 = std::max(80.0f, std::min(result.f1, 1000.0f));
    result.f2 = std::max(200.0f, std::min(result.f2, 4000.0f));
    result.f3 = std::max(1000.0f, std::min(result.f3, 8000.0f));
    
    return result;
}

void FormantFilter::updateFormantFilters(int channel, const FormantData& formant) {
    // Apply formant shift with thermal compensation
    float thermalFactor = m_thermalModel.getThermalFactor();
    float shiftFactor = (0.5f + m_formantShift.current * 1.5f) * thermalFactor; // 0.5x to 2x frequency
    
    // Update formant frequencies with drift
    m_formantFilters[channel][0].freq = formant.f1 * shiftFactor;
    m_formantFilters[channel][0].gain = formant.a1;
    m_formantFilters[channel][1].freq = formant.f2 * shiftFactor;
    m_formantFilters[channel][1].gain = formant.a2;
    m_formantFilters[channel][2].freq = formant.f3 * shiftFactor;
    m_formantFilters[channel][2].gain = formant.a3;
    
    // Update Q based on resonance parameter with vintage characteristics
    float baseQ = 2.0f + m_resonance.current * 6.0f; // Q from 2 to 8
    
    // Recalculate filter coefficients with thermal modeling
    for (int i = 0; i < 3; ++i) {
        auto& filter = m_formantFilters[channel][i];
        
        // Vintage mode affects Q differently for each formant
        if (m_vintageMode.current > 0.5f) {
            filter.q = baseQ * (0.8f + i * 0.1f); // F1 has lower Q, F3 has higher Q
        } else {
            filter.q = baseQ;
        }
        
        // Update component drift slowly
        filter.componentDrift += (((rand() % 1000) / 1000.0f - 0.5f) * 0.0001f) / m_sampleRate;
        filter.componentDrift = std::max(-0.01f, std::min(0.01f, filter.componentDrift));
        
        calculateFilterCoefficients(filter, thermalFactor);
    }
}

float FormantFilter::processFormantFilter(float input, FormantBandpass& filter, float drive, bool vintageMode) {
    // Apply nonlinear processing if drive is active
    float processedInput = input;
    if (drive > 0.01f) {
        if (vintageMode) {
            // Vintage tube-like saturation in the filter
            processedInput = std::tanh(input * (1.0f + drive * 2.0f)) / (1.0f + drive * 0.5f);
        } else {
            // Modern analog saturation
            processedInput = analogSaturation(input, drive * 0.5f);
        }
    }
    
    // Enhanced biquad bandpass filter implementation with oversampling for nonlinear parts
    if (drive > 0.3f && !vintageMode) {
        // Use 2x oversampling for high drive settings
        float up1 = processedInput * 2.0f;
        float up2 = 0.0f; // Zero-stuff
        
        // Process both samples
        float out1 = filter.b0 * up1 + filter.b1 * filter.oversampleState1 + filter.b2 * filter.oversampleState2;
        filter.oversampleState2 = filter.oversampleState1 - filter.a2 * out1;
        filter.oversampleState1 = up1 - filter.a1 * out1;
        
        float out2 = filter.b0 * up2 + filter.b1 * filter.oversampleState1 + filter.b2 * filter.oversampleState2;
        filter.oversampleState2 = filter.oversampleState1 - filter.a2 * out2;
        filter.oversampleState1 = up2 - filter.a1 * out2;
        
        return (out1 + out2) * 0.25f; // Downsample
    } else {
        // Standard processing
        float output = filter.b0 * processedInput + filter.b1 * filter.state1 + filter.b2 * filter.state2;
        
        // Update states with potential saturation in vintage mode
        if (vintageMode && drive > 0.1f) {
            // Add subtle saturation in the state variables
            float sat1 = filter.state1 - filter.a1 * output;
            float sat2 = filter.state2 - filter.a2 * output;
            
            filter.state1 = processedInput - std::tanh(sat1 * (1.0f + drive)) / (1.0f + drive * 0.3f);
            filter.state2 = sat2;
        } else {
            // Clean update
            filter.state2 = filter.state1 - filter.a2 * output;
            filter.state1 = processedInput - filter.a1 * output;
        }
        
        return output;
    }
}

void FormantFilter::calculateFilterCoefficients(FormantBandpass& filter, float thermalFactor) {
    // Apply component drift and thermal effects
    float adjustedFreq = filter.freq * (1.0f + filter.componentDrift) * thermalFactor;
    float adjustedQ = filter.q * (1.0f + filter.componentDrift * 0.5f);
    
    // Clamp frequency to reasonable bounds
    adjustedFreq = std::max(20.0f, std::min(adjustedFreq, static_cast<float>(m_sampleRate * 0.45)));
    adjustedQ = std::max(0.5f, std::min(adjustedQ, 30.0f));
    
    // Calculate enhanced biquad bandpass filter coefficients with pre-warping
    float omega = 2.0f * M_PI * adjustedFreq / m_sampleRate;
    
    // Pre-warp for better frequency response at high frequencies
    float prewarpedOmega = 2.0f * std::tan(omega * 0.5f);
    float sinOmega = std::sin(prewarpedOmega * 0.5f);
    float cosOmega = std::cos(prewarpedOmega * 0.5f);
    float alpha = sinOmega / adjustedQ;
    
    // Bandpass coefficients with improved numerical stability
    float a0 = 1.0f + alpha;
    filter.a1 = -2.0f * cosOmega / a0;
    filter.a2 = (1.0f - alpha) / a0;
    
    filter.b0 = alpha / a0;
    filter.b1 = 0.0f;
    filter.b2 = -alpha / a0;
    
    // Enhanced stability checks
    if (std::abs(filter.a1) >= 1.99f) filter.a1 = (filter.a1 > 0) ? 1.98f : -1.98f;
    if (std::abs(filter.a2) >= 0.99f) filter.a2 = (filter.a2 > 0) ? 0.98f : -0.98f;
    
    // Ensure the pole radius is less than 1 for stability
    float poleRadius = std::sqrt(filter.a2);
    if (poleRadius >= 0.99f) {
        float scale = 0.98f / poleRadius;
        filter.a2 *= scale;
    }
}

float FormantFilter::analogSaturation(float input, float amount) {
    // Analog-style saturation with even harmonics
    float driven = input * (1.0f + amount * 2.0f);
    return std::tanh(driven * 0.8f) / (0.8f * (1.0f + amount * 0.3f));
}

float FormantFilter::vintageDistortion(float input, float amount) {
    // Vintage tube-like distortion with asymmetry
    float driven = input * (1.0f + amount * 3.0f);
    
    // Asymmetric clipping for vintage character
    if (driven > 0.0f) {
        return std::tanh(driven * 0.7f) / (0.7f * (1.0f + amount * 0.2f));
    } else {
        return std::tanh(driven * 0.9f) / (0.9f * (1.0f + amount * 0.1f));
    }
}

void FormantFilter::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_vowelPosition.target = params.at(0);
    if (params.find(1) != params.end()) m_formantShift.target = params.at(1);
    if (params.find(2) != params.end()) m_resonance.target = params.at(2);
    if (params.find(3) != params.end()) m_morph.target = params.at(3);
    if (params.find(4) != params.end()) m_drive.target = params.at(4);
    if (params.find(5) != params.end()) m_vintageMode.target = params.at(5);
}

juce::String FormantFilter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Vowel Position";
        case 1: return "Formant Shift";
        case 2: return "Resonance";
        case 3: return "Morph";
        case 4: return "Drive";
        case 5: return "Vintage Mode";
        default: return "";
    }
}
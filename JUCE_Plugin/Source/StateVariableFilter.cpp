#include "StateVariableFilter.h"
#include <algorithm>
#include <random>

StateVariableFilter::StateVariableFilter() {
    // Initialize smoothed parameters
    m_frequency.reset(0.5f);  // Mid-range frequency
    m_resonance.reset(0.3f);
    m_mode.reset(0.0f);       // Start with lowpass
    m_drive.reset(0.1f);
    m_nonlinearity.reset(0.0f);
    m_vintageMode.reset(0.0f);
}

void StateVariableFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    m_frequency.setSmoothingTime(10.0f, sampleRate);     // Fast for frequency
    m_resonance.setSmoothingTime(20.0f, sampleRate);
    m_mode.setSmoothingTime(50.0f, sampleRate);          // Slower for mode morphing
    m_drive.setSmoothingTime(100.0f, sampleRate);
    m_nonlinearity.setSmoothingTime(200.0f, sampleRate);
    m_vintageMode.setSmoothingTime(500.0f, sampleRate);
    
    // Reset all filter states
    for (auto& filter : m_filters) {
        filter.reset();
    }

void StateVariableFilter::reset() {
    // Reset filter states
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
}

    
    // Reset oversamplers
    for (auto& oversampler : m_oversamplers) {
        oversampler.reset();
    }
    
    // Initialize filter coefficients
    m_coeffs.updateCoefficients(1000.0f, 0.3f, false, sampleRate);
}

void StateVariableFilter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update thermal modeling
    m_thermalModel.update(m_sampleRate);
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smoothed parameters
            m_frequency.update();
            m_resonance.update();
            m_mode.update();
            m_drive.update();
            m_nonlinearity.update();
            m_vintageMode.update();
            
            // Update filter coefficients periodically
            static int updateCounter = 0;
            if (++updateCounter >= 8) { // Update every 8 samples
                updateCounter = 0;
                
                // Convert frequency parameter to Hz (exponential scaling)
                float freqHz = 20.0f * std::pow(1000.0f, m_frequency.current);
                
                // Apply thermal drift
                float thermalFactor = m_thermalModel.getThermalFactor();
                freqHz *= thermalFactor;
                
                bool vintageMode = m_vintageMode.current > 0.5f;
                m_coeffs.updateCoefficients(freqHz, m_resonance.current, vintageMode, m_sampleRate);
            }
            
            channelData[sample] = processSample(channelData[sample], channel);
        }
    }
}

float StateVariableFilter::processSample(float input, int channel) {
    // Apply DC blocking
    float cleanInput = m_dcBlockers[channel].process(input);
    
    // Use oversampling for high-quality processing
    return processOversampled(cleanInput, channel);
}

float StateVariableFilter::processOversampled(float input, int channel) {
    auto& filter = m_filters[channel];
    auto& oversampler = m_oversamplers[channel];
    
    // Input drive with nonlinearity
    float driveAmount = 1.0f + m_drive.current * 9.0f; // 1x to 10x
    float drivenInput = input * driveAmount;
    
    // Apply nonlinear processing based on mode
    if (m_nonlinearity.current > 0.01f) {
        if (m_vintageMode.current > 0.5f) {
            drivenInput = vintageSaturation(drivenInput, m_nonlinearity.current);
        } else {
            drivenInput = analogSaturation(drivenInput, m_nonlinearity.current, 0.2f);
        }
    }
    
    // Upsample for processing
    float upsampled1 = oversampler.upsampleFilter.process(drivenInput * 2.0f, 0.4f);
    float upsampled2 = oversampler.upsampleFilter.process(0.0f, 0.4f); // Zero-stuff
    
    // Process both upsampled samples
    float output1, output2;
    
    if (m_vintageMode.current > 0.5f) {
        filter.processVintage(upsampled1, m_coeffs.g, m_coeffs.k, m_nonlinearity.current);
        output1 = filter.getMorphedOutput(m_mode.current, m_vintageMode.current);
        
        filter.processVintage(upsampled2, m_coeffs.g, m_coeffs.k, m_nonlinearity.current);
        output2 = filter.getMorphedOutput(m_mode.current, m_vintageMode.current);
    } else {
        filter.processZavalishin(upsampled1, m_coeffs.g, m_coeffs.k, 
                               m_coeffs.a1, m_coeffs.a2, m_coeffs.a3);
        output1 = filter.getMorphedOutput(m_mode.current, m_vintageMode.current);
        
        filter.processZavalishin(upsampled2, m_coeffs.g, m_coeffs.k,
                               m_coeffs.a1, m_coeffs.a2, m_coeffs.a3);
        output2 = filter.getMorphedOutput(m_mode.current, m_vintageMode.current);
    }
    
    // Downsample
    float averaged = (output1 + output2) * 0.5f;
    float downsampled = oversampler.downsampleFilter.process(averaged, 0.4f);
    
    // Compensate for drive
    return downsampled / driveAmount;
}

float StateVariableFilter::analogSaturation(float input, float amount, float asymmetry) {
    float driven = input * (1.0f + amount * 3.0f);
    
    // Asymmetric saturation
    if (driven > 0.0f) {
        float pos_factor = 0.7f + asymmetry * 0.3f;
        return std::tanh(driven * pos_factor) / pos_factor;
    } else {
        float neg_factor = 0.9f - asymmetry * 0.2f;
        return std::tanh(driven * neg_factor) / neg_factor;
    }
}

float StateVariableFilter::vintageSaturation(float input, float amount) {
    // Vintage-style saturation with even harmonics
    float driven = input * (1.0f + amount * 2.0f);
    
    // Add subtle even harmonic content
    float saturated = std::tanh(driven * 0.8f);
    float evenHarmonics = driven * driven * amount * 0.1f;
    
    return saturated + evenHarmonics;
}

// FilterCoefficients implementation
void StateVariableFilter::FilterCoefficients::updateCoefficients(float frequency, float resonance, 
                                                               bool vintageMode, double sampleRate, 
                                                               float componentDrift) {
    // Apply component drift
    float driftedFreq = frequency * (1.0f + componentDrift);
    
    // Calculate g coefficient with better stability
    float wc = 2.0f * M_PI * driftedFreq / sampleRate;
    g = std::tan(wc * 0.5f);
    
    // Resonance calculation
    if (vintageMode) {
        // Vintage mode - more musical resonance curve
        k = 2.0f - 2.0f * resonance;
        k = std::max(0.01f, std::min(1.99f, k));
    } else {
        // Modern mode - linear damping
        k = 2.0f * (1.0f - resonance);
        k = std::max(0.01f, std::min(1.99f, k));
    }
    
    // Calculate Zavalishin's coefficients for zero-delay feedback
    float denom = 1.0f + g * (g + k);
    a1 = 1.0f / denom;
    a2 = g * a1;
    a3 = g * a2;
    
    // Store thermal drift for component modeling
    thermalDrift = componentDrift;
}

// SVFState implementation
void StateVariableFilter::SVFState::processZavalishin(float input, float g, float k, 
                                                     float a1, float a2, float a3) {
    // Zavalishin's zero-delay feedback topology
    float v0 = input;
    v1 = (ic1eq + g * (v0 - ic2eq)) * a1;
    v2 = ic2eq + g * v1;
    ic1eq = 2.0f * v1 - ic1eq;
    ic2eq = 2.0f * v2 - ic2eq;
    
    // Calculate all filter outputs
    lowpass = v2;
    bandpass = v1;
    highpass = v0 - k * v1 - v2;
    notch = v0 - k * v1;
    allpass = v0 - 2.0f * k * v1;
    peak = v0 - k * v1 + 0.5f * v2; // Peaking response
}

void StateVariableFilter::SVFState::processVintage(float input, float g, float k, float nonlinearity) {
    // Vintage-style processing with component nonlinearities
    
    // Update component drift slowly
    componentDrift += (((rand() % 1000) / 1000.0f - 0.5f) * 0.0001f) / 44100.0f;
    componentDrift = std::max(-0.01f, std::min(0.01f, componentDrift));
    
    // Apply drift to coefficients
    float driftedG = g * (1.0f + componentDrift);
    float driftedK = k * (1.0f + componentDrift * 0.5f);
    
    // Process with nonlinearity in the integrators
    float v0 = input;
    
    // First integrator with saturation
    float int1Input = v0 - driftedK * ic1eq - ic2eq;
    if (nonlinearity > 0.01f) {
        int1Input = std::tanh(int1Input * (1.0f + nonlinearity)) / (1.0f + nonlinearity);
    }
    
    v1 = ic1eq + driftedG * int1Input;
    ic1eq = v1;
    
    // Second integrator with saturation
    float int2Input = v1;
    if (nonlinearity > 0.01f) {
        int2Input = std::tanh(int2Input * (1.0f + nonlinearity * 0.5f)) / (1.0f + nonlinearity * 0.5f);
    }
    
    v2 = ic2eq + driftedG * int2Input;
    ic2eq = v2;
    
    // Calculate outputs with vintage characteristics
    lowpass = v2 * (1.0f - nonlinearity * 0.1f);  // Slight high-freq loss
    bandpass = v1;
    highpass = v0 - driftedK * v1 - v2;
    notch = v0 - driftedK * v1;
    allpass = v0 - 2.0f * driftedK * v1;
    peak = v0 - driftedK * v1 + 0.3f * v2;
}

float StateVariableFilter::SVFState::getMorphedOutput(float mode, float vintageAmount) {
    // Enhanced morphing with vintage characteristics
    float output;
    
    if (mode <= 0.2f) {
        // Pure lowpass
        output = lowpass;
    } else if (mode <= 0.4f) {
        // Lowpass to bandpass
        float t = (mode - 0.2f) * 5.0f;
        output = lowpass * (1.0f - t) + bandpass * t;
    } else if (mode <= 0.6f) {
        // Bandpass to highpass
        float t = (mode - 0.4f) * 5.0f;
        output = bandpass * (1.0f - t) + highpass * t;
    } else if (mode <= 0.8f) {
        // Highpass to notch
        float t = (mode - 0.6f) * 5.0f;
        output = highpass * (1.0f - t) + notch * t;
    } else {
        // Notch to allpass/peak
        float t = (mode - 0.8f) * 5.0f;
        float specialResponse = vintageAmount > 0.5f ? peak : allpass;
        output = notch * (1.0f - t) + specialResponse * t;
    }
    
    return output;
}

void StateVariableFilter::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_frequency.target = params.at(0);
    if (params.find(1) != params.end()) m_resonance.target = params.at(1);
    if (params.find(2) != params.end()) m_mode.target = params.at(2);
    if (params.find(3) != params.end()) m_drive.target = params.at(3);
    if (params.find(4) != params.end()) m_nonlinearity.target = params.at(4);
    if (params.find(5) != params.end()) m_vintageMode.target = params.at(5);
}

juce::String StateVariableFilter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Frequency";
        case 1: return "Resonance";
        case 2: return "Mode";
        case 3: return "Drive";
        case 4: return "Nonlinearity";
        case 5: return "Vintage Mode";
        default: return "";
    }
}
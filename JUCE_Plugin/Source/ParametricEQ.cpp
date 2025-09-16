#include "ParametricEQ.h"
#include "DspEngineUtilities.h"
#include <cmath>

ParametricEQ::ParametricEQ() {
    // Initialize with musical defaults
    m_lowGain.reset(0.5f);      // Flat
    m_lowFreq.reset(0.15f);     // ~100 Hz
    m_midGain.reset(0.5f);      // Flat
    m_midFreq.reset(0.5f);      // ~1 kHz
    m_midQ.reset(0.5f);         // Moderate Q
    m_highGain.reset(0.5f);     // Flat
    m_highFreq.reset(0.8f);     // ~8 kHz
    m_outputGain.reset(0.5f);   // Unity gain
    m_mix.reset(1.0f);          // 100% wet for EQ
}

void ParametricEQ::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set smoothing for all parameters - INSTANT response
    float smoothingMs = 0.1f; // Was 5.0f - now nearly instant
    m_lowGain.setSmoothingTime(smoothingMs, sampleRate);
    m_lowFreq.setSmoothingTime(smoothingMs, sampleRate);
    m_midGain.setSmoothingTime(smoothingMs, sampleRate);
    m_midFreq.setSmoothingTime(smoothingMs, sampleRate);
    m_midQ.setSmoothingTime(smoothingMs, sampleRate);
    m_highGain.setSmoothingTime(smoothingMs, sampleRate);
    m_highFreq.setSmoothingTime(smoothingMs, sampleRate);
    m_outputGain.setSmoothingTime(smoothingMs, sampleRate);
    m_mix.setSmoothingTime(smoothingMs, sampleRate);
    
    // Initialize filters for each channel
    for (int ch = 0; ch < 2; ++ch) {
        m_lowShelf[ch].reset();
        m_midBand[ch].reset();
        m_highShelf[ch].reset();
    }
    
    updateFilterCoefficients();
}

void ParametricEQ::reset() {
    // Reset filter states
    for (int ch = 0; ch < 2; ++ch) {
        m_lowShelf[ch].reset();
        m_midBand[ch].reset();
        m_highShelf[ch].reset();
    }
}

void ParametricEQ::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smoothed parameters
    m_lowGain.update();
    m_lowFreq.update();
    m_midGain.update();
    m_midFreq.update();
    m_midQ.update();
    m_highGain.update();
    m_highFreq.update();
    m_outputGain.update();
    m_mix.update();
    
    // Update filter coefficients if parameters changed significantly
    static float lastLowFreq = 0.0f, lastMidFreq = 0.0f, lastHighFreq = 0.0f;
    static float lastLowGain = 0.0f, lastMidGain = 0.0f, lastHighGain = 0.0f;
    static float lastMidQ = 0.0f;
    
    bool needsUpdate = false;
    if (std::abs(m_lowFreq.current - lastLowFreq) > 0.001f ||
        std::abs(m_midFreq.current - lastMidFreq) > 0.001f ||
        std::abs(m_highFreq.current - lastHighFreq) > 0.001f ||
        std::abs(m_lowGain.current - lastLowGain) > 0.001f ||
        std::abs(m_midGain.current - lastMidGain) > 0.001f ||
        std::abs(m_highGain.current - lastHighGain) > 0.001f ||
        std::abs(m_midQ.current - lastMidQ) > 0.001f) {
        
        updateFilterCoefficients();
        
        lastLowFreq = m_lowFreq.current;
        lastMidFreq = m_midFreq.current;
        lastHighFreq = m_highFreq.current;
        lastLowGain = m_lowGain.current;
        lastMidGain = m_midGain.current;
        lastHighGain = m_highGain.current;
        lastMidQ = m_midQ.current;
        needsUpdate = true;
    }
    
    // Process each channel
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        // Store dry signal
        std::vector<float> drySignal(channelData, channelData + numSamples);
        
        // Process through EQ bands
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            
            // Low shelf
            float processed = m_lowShelf[channel].process(input);
            
            // Mid band (parametric)
            processed = m_midBand[channel].process(processed);
            
            // High shelf
            processed = m_highShelf[channel].process(processed);
            
            // Apply output gain
            float outputGainLinear = 0.25f + m_outputGain.current * 1.5f; // 0.25 to 1.75 (-12dB to +5dB)
            processed *= outputGainLinear;
            
            // Mix dry and wet
            channelData[sample] = drySignal[sample] * (1.0f - m_mix.current) + 
                                 processed * m_mix.current;
            
            // Soft limiting to prevent clipping
            if (std::abs(channelData[sample]) > 0.95f) {
                channelData[sample] = 0.95f * std::tanh(channelData[sample] / 0.95f);
            }
        }
    }
}

void ParametricEQ::updateFilterCoefficients() {
    // Convert normalized parameters to actual frequencies
    float lowFreq = 20.0f + m_lowFreq.current * m_lowFreq.current * 480.0f;  // 20-500 Hz
    float midFreq = 200.0f + m_midFreq.current * m_midFreq.current * 4800.0f; // 200-5000 Hz
    float highFreq = 1000.0f + m_highFreq.current * m_highFreq.current * 14000.0f; // 1k-15k Hz
    
    // Convert normalized gains to dB (-12 to +12 dB)
    float lowGainDb = (m_lowGain.current - 0.5f) * 24.0f;
    float midGainDb = (m_midGain.current - 0.5f) * 24.0f;
    float highGainDb = (m_highGain.current - 0.5f) * 24.0f;
    
    // Q factor for mid band (0.3 to 5.0)
    float midQ = 0.3f + m_midQ.current * 4.7f;
    
    // Update filter coefficients for each channel
    for (int ch = 0; ch < 2; ++ch) {
        m_lowShelf[ch].setCoefficients(lowFreq, lowGainDb, 0.7f, m_sampleRate);
        m_midBand[ch].setCoefficients(midFreq, midGainDb, midQ, m_sampleRate);
        m_highShelf[ch].setCoefficients(highFreq, highGainDb, 0.7f, m_sampleRate);
    }
}

// BiQuadFilter implementation
void ParametricEQ::BiQuadFilter::reset() {
    x1 = x2 = y1 = y2 = 0.0f;
}

float ParametricEQ::BiQuadFilter::process(float input) {
    float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    // Update states
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}

void ParametricEQ::BiQuadFilter::setCoefficients(float freq, float gainDb, float Q, double sampleRate) {
    float A = std::pow(10.0f, gainDb / 40.0f);
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sin_omega = std::sin(omega);
    float cos_omega = std::cos(omega);
    float alpha = sin_omega / (2.0f * Q);
    
    // Calculate coefficients based on filter type
    if (type == FilterType::LOW_SHELF) {
        float S = 1.0f; // Shelf slope
        float beta = std::sqrt(A) / Q;
        
        b0 = A * ((A + 1) - (A - 1) * cos_omega + beta * sin_omega);
        b1 = 2 * A * ((A - 1) - (A + 1) * cos_omega);
        b2 = A * ((A + 1) - (A - 1) * cos_omega - beta * sin_omega);
        float a0 = (A + 1) + (A - 1) * cos_omega + beta * sin_omega;
        a1 = -2 * ((A - 1) + (A + 1) * cos_omega);
        a2 = (A + 1) + (A - 1) * cos_omega - beta * sin_omega;
        
        // Normalize
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }
    else if (type == FilterType::HIGH_SHELF) {
        float S = 1.0f;
        float beta = std::sqrt(A) / Q;
        
        b0 = A * ((A + 1) + (A - 1) * cos_omega + beta * sin_omega);
        b1 = -2 * A * ((A - 1) + (A + 1) * cos_omega);
        b2 = A * ((A + 1) + (A - 1) * cos_omega - beta * sin_omega);
        float a0 = (A + 1) - (A - 1) * cos_omega + beta * sin_omega;
        a1 = 2 * ((A - 1) - (A + 1) * cos_omega);
        a2 = (A + 1) - (A - 1) * cos_omega - beta * sin_omega;
        
        // Normalize
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }
    else { // PEAK
        b0 = 1 + alpha * A;
        b1 = -2 * cos_omega;
        b2 = 1 - alpha * A;
        float a0 = 1 + alpha / A;
        a1 = -2 * cos_omega;
        a2 = 1 - alpha / A;
        
        // Normalize
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }
}

void ParametricEQ::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    m_lowGain.target = getParam(0, 0.5f);
    m_lowFreq.target = getParam(1, 0.15f);
    m_midGain.target = getParam(2, 0.5f);
    m_midFreq.target = getParam(3, 0.5f);
    m_midQ.target = getParam(4, 0.5f);
    m_highGain.target = getParam(5, 0.5f);
    m_highFreq.target = getParam(6, 0.8f);
    m_outputGain.target = getParam(7, 0.5f);
    m_mix.target = getParam(8, 1.0f);
}

juce::String ParametricEQ::getParameterName(int index) const {
    switch (index) {
        case 0: return "Low Gain";
        case 1: return "Low Freq";
        case 2: return "Mid Gain";
        case 3: return "Mid Freq";
        case 4: return "Mid Q";
        case 5: return "High Gain";
        case 6: return "High Freq";
        case 7: return "Output";
        case 8: return "Mix";
        default: return "";
    }
}
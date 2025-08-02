#include "ParametricEQ.h"
#include <cmath>

ParametricEQ::ParametricEQ() {
    // Initialize smoothed parameters with proper defaults
    m_band1Freq.reset(0.2f);
    m_band1Gain.reset(0.5f);
    m_band2Freq.reset(0.4f);
    m_band2Gain.reset(0.5f);
    m_band3Freq.reset(0.6f);
    m_band3Gain.reset(0.5f);
    m_band4Freq.reset(0.8f);
    m_band4Gain.reset(0.5f);
    m_globalQ.reset(0.5f);
}

void ParametricEQ::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    float freqSmoothingTime = 150.0f; // 150ms for frequency changes
    float gainSmoothingTime = 100.0f; // 100ms for gain changes
    float qSmoothingTime = 200.0f; // 200ms for Q changes
    
    m_band1Freq.setSmoothingTime(freqSmoothingTime, sampleRate);
    m_band1Gain.setSmoothingTime(gainSmoothingTime, sampleRate);
    m_band2Freq.setSmoothingTime(freqSmoothingTime, sampleRate);
    m_band2Gain.setSmoothingTime(gainSmoothingTime, sampleRate);
    m_band3Freq.setSmoothingTime(freqSmoothingTime, sampleRate);
    m_band3Gain.setSmoothingTime(gainSmoothingTime, sampleRate);
    m_band4Freq.setSmoothingTime(freqSmoothingTime, sampleRate);
    m_band4Gain.setSmoothingTime(gainSmoothingTime, sampleRate);
    m_globalQ.setSmoothingTime(qSmoothingTime, sampleRate);
    
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
        
        // Initialize bands with default frequencies using smoothed parameters
        channel.bands[0].targetFreq = frequencyFromNormalized(m_band1Freq.current);
        channel.bands[1].targetFreq = frequencyFromNormalized(m_band2Freq.current);
        channel.bands[2].targetFreq = frequencyFromNormalized(m_band3Freq.current);
        channel.bands[3].targetFreq = frequencyFromNormalized(m_band4Freq.current);
        
        // Initialize current values to prevent coefficient jumps
        for (auto& band : channel.bands) {
            band.currentFreq = band.targetFreq;
            band.currentGain = 0.0;
            band.currentQ = 1.0;
        }

void ParametricEQ::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for ParametricEQ
}

    }
}

void ParametricEQ::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    m_band1Freq.update();
    m_band1Gain.update();
    m_band2Freq.update();
    m_band2Gain.update();
    m_band3Freq.update();
    m_band3Gain.update();
    m_band4Freq.update();
    m_band4Gain.update();
    m_globalQ.update();
    
    // Convert smoothed parameters to actual values
    double band1Freq = frequencyFromNormalized(m_band1Freq.current);
    double band1Gain = (m_band1Gain.current - 0.5) * 30.0; // -15 to +15 dB
    double band2Freq = frequencyFromNormalized(m_band2Freq.current);
    double band2Gain = (m_band2Gain.current - 0.5) * 30.0;
    double band3Freq = frequencyFromNormalized(m_band3Freq.current);
    double band3Gain = (m_band3Gain.current - 0.5) * 30.0;
    double band4Freq = frequencyFromNormalized(m_band4Freq.current);
    double band4Gain = (m_band4Gain.current - 0.5) * 30.0;
    
    // Global Q from 0.1 to 10 (logarithmic) using smoothed parameter
    double globalQ = 0.1 * std::pow(100.0, m_globalQ.current);
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Update target parameters for all bands
        state.bands[0].targetFreq = band1Freq;
        state.bands[0].targetGain = band1Gain;
        state.bands[0].targetQ = globalQ * 0.7; // Slightly wider for low shelf
        
        state.bands[1].targetFreq = band2Freq;
        state.bands[1].targetGain = band2Gain;
        state.bands[1].targetQ = globalQ;
        
        state.bands[2].targetFreq = band3Freq;
        state.bands[2].targetGain = band3Gain;
        state.bands[2].targetQ = globalQ;
        
        state.bands[3].targetFreq = band4Freq;
        state.bands[3].targetGain = band4Gain;
        state.bands[3].targetQ = globalQ * 0.7; // Slightly wider for high shelf
        
        // Process audio
        // Update thermal and aging models
        float processingLoad = std::min(1.0f, numSamples / 512.0f);
        state.thermalModel.update(processingLoad);
        state.componentAging.update();
        
        for (int sample = 0; sample < numSamples; ++sample) {
            double input = static_cast<double>(channelData[sample]);
            
            // Apply input DC blocking
            input = state.inputDCBlocker.process(input);
            
            // Update smooth parameters for all bands
            for (auto& band : state.bands) {
                band.updateSmooth(m_sampleRate);
            }
            
            // Process through all bands in series with boutique enhancements
            double output = input;
            
            // Analyze signal level for adaptive processing
            float signalLevel = std::abs(output);
            state.lastPeakLevel = std::max(state.lastPeakLevel * 0.999f, signalLevel);
            
            // Determine if oversampling is beneficial (high gain or high frequency content)
            bool useOversampling = (std::abs(band1Gain) > 6.0 || std::abs(band2Gain) > 6.0 || 
                                  std::abs(band3Gain) > 6.0 || std::abs(band4Gain) > 6.0) && 
                                  state.lastPeakLevel > 0.3f;
            
            // Band 1: Low shelf (20-500Hz typical) with thermal compensation
            if (std::abs(state.bands[0].targetGain) > 0.1) {
                output = state.bands[0].process(output, BiquadFilter::LOW_SHELF, m_sampleRate, useOversampling);
            }
            
            // Band 2: Bell (200Hz-2kHz typical)
            if (std::abs(state.bands[1].targetGain) > 0.1) {
                output = state.bands[1].process(output, BiquadFilter::BELL, m_sampleRate, useOversampling);
            }
            
            // Band 3: Bell (1kHz-10kHz typical)
            if (std::abs(state.bands[2].targetGain) > 0.1) {
                output = state.bands[2].process(output, BiquadFilter::BELL, m_sampleRate, useOversampling);
            }
            
            // Band 4: High shelf (5kHz-20kHz typical)
            if (std::abs(state.bands[3].targetGain) > 0.1) {
                output = state.bands[3].process(output, BiquadFilter::HIGH_SHELF, m_sampleRate, useOversampling);
            }
            
            // Apply thermal and aging compensation to overall output
            float thermalDrift = state.thermalModel.getTemperatureDrift();
            float agingFactor = state.componentAging.getAgingFactor();
            output *= (1.0f + thermalDrift * 0.1f) * agingFactor;
            
            // Advanced saturation modeling based on total gain
            float totalGain = std::abs(band1Gain) + std::abs(band2Gain) + std::abs(band3Gain) + std::abs(band4Gain);
            state.saturationDrive = totalGain / 60.0f; // Normalize to 0-1 range
            
            if (state.saturationDrive > 0.1f) {
                output = state.applySaturation(output, state.saturationDrive);
            }
            
            // Enhanced soft clipping with analog character
            if (std::abs(output) > 0.85) {
                // Multi-stage saturation for natural sound
                float sign = output > 0 ? 1.0f : -1.0f;
                float absOutput = std::abs(output);
                
                if (absOutput > 0.95) {
                    // Hard limiting with gentle curve
                    output = sign * (0.95f + std::tanh((absOutput - 0.95f) * 10.0f) * 0.05f);
                } else {
                    // Soft saturation
                    output = sign * (absOutput * 0.9f + std::tanh(absOutput * 2.0f) * 0.1f);
                }
                
                // Add subtle even harmonics from saturation
                output += output * output * output * 0.01f * state.saturationDrive;
            }
            
            // Apply output DC blocking
            output = state.outputDCBlocker.process(output);
            
            channelData[sample] = static_cast<float>(output);
        }
    }
}

double ParametricEQ::frequencyFromNormalized(double normalized) {
    // Logarithmic frequency scaling from 20Hz to 20kHz
    const double minFreq = 20.0;
    const double maxFreq = 20000.0;
    const double logMin = std::log10(minFreq);
    const double logMax = std::log10(maxFreq);
    
    double logFreq = logMin + normalized * (logMax - logMin);
    return std::pow(10.0, logFreq);
}

void ParametricEQ::updateParameters(const std::map<int, float>& params) {
    // Update target values for smoothed parameters
    // Note: We're using pairs of parameters for freq/gain of each band
    // This gives us 4 bands with 2 controls each = 8 parameters total
    if (params.count(0)) m_band1Freq.target = params.at(0);
    if (params.count(1)) m_band1Gain.target = params.at(1);
    if (params.count(2)) m_band2Freq.target = params.at(2);
    if (params.count(3)) m_band2Gain.target = params.at(3);
    if (params.count(4)) m_band3Freq.target = params.at(4);
    if (params.count(5)) m_band3Gain.target = params.at(5);
    if (params.count(6)) m_band4Freq.target = params.at(6);
    if (params.count(7)) m_band4Gain.target = params.at(7);
    
    // Global Q remains at default for now (could be exposed as parameter 8)
    // m_globalQ.target = 0.5f; // Default to Q of 1.0
}

juce::String ParametricEQ::getParameterName(int index) const {
    switch (index) {
        case 0: return "Low Freq";
        case 1: return "Low Gain";
        case 2: return "Mid1 Freq";
        case 3: return "Mid1 Gain";
        case 4: return "Mid2 Freq";
        case 5: return "Mid2 Gain";
        case 6: return "High Freq";
        case 7: return "High Gain";
        default: return "";
    }
}
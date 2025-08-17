#include "VintageConsoleEQ.h"
#include "DspEngineUtilities.h"
#include <algorithm>

VintageConsoleEQ::VintageConsoleEQ() {
    // Initialize smoothed parameters with boutique defaults
    m_lowGain.setImmediate(0.5f);
    m_lowFreq.setImmediate(0.3f);
    m_midGain.setImmediate(0.5f);
    m_midFreq.setImmediate(0.5f);
    m_midQ.setImmediate(0.5f);
    m_highGain.setImmediate(0.5f);
    m_highFreq.setImmediate(0.7f);
    m_drive.setImmediate(0.3f);
    m_consoleType.setImmediate(0.0f);  // Neve default
    m_vintage.setImmediate(0.5f);      // Moderate vintage character
    m_mix.setImmediate(1.0f);          // 100% wet by default
    
    // Set smoothing rates
    m_lowGain.setSmoothingRate(0.995f);
    m_lowFreq.setSmoothingRate(0.998f);
    m_midGain.setSmoothingRate(0.995f);
    m_midFreq.setSmoothingRate(0.998f);
    m_midQ.setSmoothingRate(0.997f);
    m_highGain.setSmoothingRate(0.995f);
    m_highFreq.setSmoothingRate(0.998f);
    m_drive.setSmoothingRate(0.99f);
    m_consoleType.setSmoothingRate(0.95f);  // Slower for type changes
    m_vintage.setSmoothingRate(0.995f);
    m_mix.setSmoothingRate(0.995f);
}

void VintageConsoleEQ::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    for (auto& channel : m_channelStates) {
        channel.reset();
        channel.updateFilters(sampleRate);
    }


    
    // Reset DC blockers
    for (auto& blocker : m_inputDCBlockers) {
        blocker.reset();
    }
    for (auto& blocker : m_outputDCBlockers) {
        blocker.reset();
    }
    
    // Reset aging and thermal
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void VintageConsoleEQ::reset() {
    // Reset all internal state
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
    for (auto& blocker : m_inputDCBlockers) {
        blocker.reset();
    }
    for (auto& blocker : m_outputDCBlockers) {
        blocker.reset();
    }
}

void VintageConsoleEQ::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_lowGain.update();
    m_lowFreq.update();
    m_midGain.update();
    m_midFreq.update();
    m_midQ.update();
    m_highGain.update();
    m_highFreq.update();
    m_drive.update();
    m_consoleType.update();
    m_vintage.update();
    m_mix.update();
    
    // Convert parameters to actual values
    double lowFreq = 30.0 + m_lowFreq.current * 270.0;    // 30-300Hz
    double lowGain = (m_lowGain.current - 0.5) * 30.0;    // +/- 15dB
    
    double midFreq = 200.0 * std::pow(40.0, m_midFreq.current); // 200Hz-8kHz
    double midGain = (m_midGain.current - 0.5) * 30.0;    // +/- 15dB
    double midQ = 0.3 + m_midQ.current * 2.7;              // 0.3 to 3.0
    
    double highFreq = 3000.0 + m_highFreq.current * 13000.0; // 3k-16kHz
    double highGain = (m_highGain.current - 0.5) * 30.0;   // +/- 15dB
    
    ConsoleType consoleType = getConsoleType();
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Smooth parameter changes
        state.smoothParameters(lowFreq, lowGain, midFreq, midGain, midQ, highFreq, highGain);
        
        // Update filters with smoothed values
        state.updateFilters(m_sampleRate);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            double input = static_cast<double>(channelData[sample]);
            double dry = input; // Store dry signal
            
            // Pre-saturation (input stage coloration)
            if (m_drive.current > 0.1f && consoleType == NEVE_1073) {
                input = state.saturation.processNeve(input, m_drive.current * 0.3f);
            }
            
            // EQ processing chain
            double output = input;
            
            // Low shelf
            if (std::abs(lowGain) > 0.1) {
                output = state.lowShelf.process(output);
            }
            
            // Mid bell
            if (std::abs(midGain) > 0.1) {
                output = state.midBell.process(output);
            }
            
            // High shelf
            if (std::abs(highGain) > 0.1) {
                output = state.highShelf.process(output);
            }
            
            // Post-EQ saturation
            if (m_drive.current > 0.01f) {
                switch (consoleType) {
                    case NEVE_1073:
                        output = state.saturation.processNeve(output, m_drive.current);
                        break;
                    case API_550:
                        output = state.saturation.processAPI(output, m_drive.current);
                        break;
                    case SSL_4000:
                        output = state.saturation.processSSL(output, m_drive.current);
                        break;
                    case PULTEC:
                        // Pultec-style gentle saturation
                        output = std::tanh(output * (1.0 + m_drive.current * 0.5)) / (1.0 + m_drive.current * 0.3);
                        break;
                }
            }
            
            // Output limiting
            if (std::abs(output) > 0.95) {
                output = std::tanh(output * 0.9) * 1.055;
            }
            
            double wet = output;
            
            // Mix dry and wet signals
            output = dry * (1.0 - m_mix.current) + wet * m_mix.current;
            
            channelData[sample] = static_cast<float>(output);
        }
    }
}

VintageConsoleEQ::ConsoleType VintageConsoleEQ::getConsoleType() const {
    // Determine console type based on drive parameter
    // Lower drive = cleaner consoles, higher drive = more colored
    if (m_drive.current < 0.25f) {
        return SSL_4000;  // Clean, surgical
    } else if (m_drive.current < 0.5f) {
        return API_550;   // Punchy, musical
    } else if (m_drive.current < 0.75f) {
        return NEVE_1073; // Warm, transformer-coupled
    } else {
        return PULTEC;    // Smooth, passive curves
    }
}

void VintageConsoleEQ::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_lowGain.target = params.at(0);
    if (params.count(1)) m_lowFreq.target = params.at(1);
    if (params.count(2)) m_midGain.target = params.at(2);
    if (params.count(3)) m_midFreq.target = params.at(3);
    if (params.count(4)) m_midQ.target = params.at(4);
    if (params.count(5)) m_highGain.target = params.at(5);
    if (params.count(6)) m_highFreq.target = params.at(6);
    if (params.count(7)) m_drive.target = params.at(7);
    if (params.count(8)) m_consoleType.target = params.at(8);
    if (params.count(9)) m_vintage.target = params.at(9);
    if (params.count(10)) m_mix.target = params.at(10);
}

juce::String VintageConsoleEQ::getParameterName(int index) const {
    switch (index) {
        case 0: return "Low Gain";
        case 1: return "Low Freq";
        case 2: return "Mid Gain";
        case 3: return "Mid Freq";
        case 4: return "Mid Q";
        case 5: return "High Gain";
        case 6: return "High Freq";
        case 7: return "Drive";
        case 8: return "Console Type";
        case 9: return "Vintage";
        case 10: return "Mix";
        default: return "";
    }
}
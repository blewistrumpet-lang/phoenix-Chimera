#include "CombResonator.h"
#include <algorithm>

CombResonator::CombResonator() {
    m_channels.resize(2);
    m_dcBlockers.resize(2);
}

void CombResonator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Maximum delay for lowest frequency (20Hz)
    int maxDelay = static_cast<int>(sampleRate / 20.0);
    
    // Initialize channels
    for (int ch = 0; ch < m_channels.size(); ++ch) {
        m_channels[ch].init(maxDelay);
        m_channels[ch].reset();
        
        // Slight stereo detuning
        m_channels[ch].detuneAmount = (ch == 0) ? -0.01f : 0.01f;
    }

void CombResonator::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for CombResonator
}

}

void CombResonator::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update comb filter parameters
    for (auto& channel : m_channels) {
        for (int i = 0; i < NUM_COMBS; ++i) {
            // Calculate frequency for this harmonic
            float harmonic = HARMONIC_RATIOS[i];
            
            // Apply harmonic spread (stretch/compress harmonics)
            if (i > 0) { // Don't modify fundamental
                harmonic = 1.0f + (harmonic - 1.0f) * m_harmonicSpread;
            }
            
            float freq = m_rootFrequency * harmonic;
            
            // Apply slight detuning for stereo width
            freq *= (1.0f + channel.detuneAmount * i * 0.1f);
            
            // Clamp frequency to valid range
            freq = std::max(20.0f, std::min(freq, static_cast<float>(m_sampleRate) * 0.45f));
            
            // Set delay length
            int delay = frequencyToDelay(freq, m_sampleRate);
            channel.combs[i].setDelay(delay);
            
            // Calculate feedback from decay time
            float feedback = decayTimeToFeedback(m_decayTime, delay, m_sampleRate);
            channel.combs[i].feedback = feedback * m_resonance;
        }
    }
    
    // Process audio
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        auto& state = m_channels[ch % m_channels.size()];
        auto& dcBlocker = m_dcBlockers[ch % m_dcBlockers.size()];
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float output = 0.0f;
            
            // Process through all comb filters
            for (int i = 0; i < NUM_COMBS; ++i) {
                float combOut = state.combs[i].process(input);
                
                // Scale by harmonic number to balance output
                float scale = 1.0f / (i + 1);
                output += combOut * scale;
            }
            
            // Normalize output
            output /= NUM_COMBS * 0.5f;
            
            // DC blocking for stability
            output = dcBlocker.process(output);
            
            // Soft limiting
            output = std::tanh(output);
            
            // Mix with dry signal (50/50)
            channelData[sample] = input * 0.5f + output * 0.5f;
        }
    }
}

void CombResonator::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Root frequency: 20Hz to 2kHz (exponential)
    float freqParam = getParam(0, 0.3f);
    m_rootFrequency = 20.0f * std::pow(100.0f, freqParam);
    
    // Resonance: 0-99% feedback
    m_resonance = getParam(1, 0.9f) * 0.99f;
    
    // Harmonic spread: 0.5x to 2x spacing
    m_harmonicSpread = 0.5f + getParam(2, 0.5f) * 1.5f;
    
    // Decay time: 0.1 to 10 seconds (exponential)
    float decayParam = getParam(3, 0.3f);
    m_decayTime = 0.1f * std::pow(100.0f, decayParam);
}

juce::String CombResonator::getParameterName(int index) const {
    switch (index) {
        case 0: return "Root Freq";
        case 1: return "Resonance";
        case 2: return "Harmonic Spread";
        case 3: return "Decay Time";
        default: return "";
    }
}

void CombResonator::CombFilter::init(int maxDelay) {
    delayLine.resize(maxDelay);
    reset();
}

void CombResonator::CombFilter::setDelay(int samples) {
    delayLength = std::min(samples, static_cast<int>(delayLine.size()));
}

float CombResonator::CombFilter::process(float input) {
    if (delayLength == 0) return input;
    
    // Read from delay line
    int readPos = (writePos - delayLength + delayLine.size()) % delayLine.size();
    float delayed = delayLine[readPos];
    
    // Comb filter equation: y = x + g * y[n-d]
    float output = input * feedforward + delayed * feedback;
    
    // Write to delay line
    delayLine[writePos] = output;
    writePos = (writePos + 1) % delayLine.size();
    
    return output;
}

void CombResonator::CombFilter::reset() {
    std::fill(delayLine.begin(), delayLine.end(), 0.0f);
    writePos = 0;
}

void CombResonator::ChannelState::init(int maxDelay) {
    for (auto& comb : combs) {
        comb.init(maxDelay);
    }
}

void CombResonator::ChannelState::reset() {
    for (auto& comb : combs) {
        comb.reset();
    }
}

float CombResonator::decayTimeToFeedback(float decaySeconds, float delaySamples, double sampleRate) {
    // Calculate feedback coefficient for desired decay time
    // Using the formula: feedback = 10^(-3 * delay_time / decay_time)
    // where -3 represents -60dB decay
    
    if (decaySeconds <= 0.0f || delaySamples <= 0.0f) {
        return 0.0f;
    }
    
    float delayTime = delaySamples / sampleRate;
    float exponent = -3.0f * delayTime / decaySeconds;
    float feedback = std::pow(10.0f, exponent);
    
    // Clamp to stable range
    return std::min(0.99f, feedback);
}

float CombResonator::DCBlocker::process(float input) {
    // Simple DC blocking filter
    // y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
    
    float output = input - x1 + 0.995f * y1;
    x1 = input;
    y1 = output;
    
    return output;
}
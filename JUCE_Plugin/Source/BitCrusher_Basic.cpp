#include "BitCrusher_Basic.h"
#include <cmath>

void BitCrusher_Basic::prepareToPlay(double sampleRate, int samplesPerBlock) {
    reset();
}

void BitCrusher_Basic::reset() {
    m_heldSampleL = 0.0f;
    m_heldSampleR = 0.0f;
    m_counterL = 0.0f;
    m_counterR = 0.0f;
}

void BitCrusher_Basic::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels && ch < 2; ++ch) {
        float* data = buffer.getWritePointer(ch);
        float& heldSample = (ch == 0) ? m_heldSampleL : m_heldSampleR;
        float& counter = (ch == 0) ? m_counterL : m_counterR;
        
        for (int i = 0; i < numSamples; ++i) {
            float input = data[i];
            float dry = input;
            
            // 1. Sample rate reduction (downsample)
            counter += 1.0f;
            if (counter >= m_downsample) {
                counter -= m_downsample;
                
                // 2. Bit depth reduction (quantize)
                if (m_bits < 24.0f) {
                    float levels = std::pow(2.0f, m_bits);
                    input = std::round(input * levels) / levels;
                }
                
                heldSample = input;
            }
            
            // 3. Mix
            data[i] = dry * (1.0f - m_mix) + heldSample * m_mix;
        }
    }
}

void BitCrusher_Basic::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(0);
    if (it != params.end()) {
        // Bits: map 0-1 to useful bit depths
        float v = it->second;
        if (v < 0.2f)      m_bits = 24.0f;  // Clean
        else if (v < 0.4f) m_bits = 12.0f;  // Vintage sampler
        else if (v < 0.6f) m_bits = 8.0f;   // 8-bit
        else if (v < 0.8f) m_bits = 4.0f;   // Crunchy
        else               m_bits = 1.0f;   // Destroyed
    }
    
    it = params.find(1);
    if (it != params.end()) {
        // Downsample: map 0-1 to useful rates
        float v = it->second;
        if (v < 0.2f)      m_downsample = 1.0f;   // No downsampling
        else if (v < 0.4f) m_downsample = 2.0f;   // Half rate
        else if (v < 0.6f) m_downsample = 4.0f;   // Quarter rate
        else if (v < 0.8f) m_downsample = 8.0f;   // 1/8 rate
        else               m_downsample = 16.0f;  // 1/16 rate
    }
    
    it = params.find(2);
    if (it != params.end()) {
        m_mix = it->second;
    }
}

juce::String BitCrusher_Basic::getParameterName(int index) const {
    switch (index) {
        case 0: return "Bits";
        case 1: return "Downsample";
        case 2: return "Mix";
        default: return "";
    }
}
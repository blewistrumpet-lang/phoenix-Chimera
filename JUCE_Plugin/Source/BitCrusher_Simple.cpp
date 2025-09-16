#include "BitCrusher_Simple.h"
#include <cmath>
#include <algorithm>

BitCrusher_Simple::BitCrusher_Simple() {
    // Default values
    m_bitDepth = 16.0f;
    m_sampleRateReduction = 1.0f;
    m_aliasing = 0.0f;
    m_jitter = 0.0f;
    m_dcOffset = 0.0f;
    m_gateThreshold = 0.0f;
    m_dither = 0.0f;
    m_mix = 1.0f;
}

void BitCrusher_Simple::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    reset();
}

void BitCrusher_Simple::reset() {
    for (auto& state : m_channelStates) {
        state.heldSample = 0.0f;
        state.sampleCounter = 0.0f;
    }
}

void BitCrusher_Simple::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // Apply DC offset if enabled
            if (std::abs(m_dcOffset) > 0.01f) {
                input += m_dcOffset * 0.1f;
            }
            
            // Gate if threshold is set
            if (m_gateThreshold > 0.01f && std::abs(input) < m_gateThreshold * 0.5f) {
                input = 0.0f;
            }
            
            // Sample rate reduction (sample and hold)
            if (m_sampleRateReduction > 1.01f) {
                state.sampleCounter += 1.0f;
                if (state.sampleCounter >= m_sampleRateReduction) {
                    state.sampleCounter -= m_sampleRateReduction;
                    // Add jitter if enabled
                    if (m_jitter > 0.01f) {
                        state.sampleCounter += (((float)rand() / RAND_MAX) - 0.5f) * m_jitter;
                    }
                    state.heldSample = input;
                }
                input = state.heldSample;
            }
            
            // Apply dithering before quantization if enabled
            if (m_dither > 0.01f && m_bitDepth < 16.0f) {
                float ditherNoise = (((float)rand() / RAND_MAX) - 0.5f) * (m_dither / std::pow(2.0f, m_bitDepth));
                input += ditherNoise;
            }
            
            // BIT CRUSHING - the main effect
            input = quantize(input, m_bitDepth);
            
            // Mix dry/wet
            float output = drySignal * (1.0f - m_mix) + input * m_mix;
            
            // Write back
            channelData[sample] = output;
        }
    }
}

float BitCrusher_Simple::quantize(float input, float bits) {
    // Pass through for high bit depths
    if (bits >= 24.0f) return input;
    
    // Clamp input to valid range
    input = std::max(-1.0f, std::min(1.0f, input));
    
    // Calculate quantization levels
    float levels = std::pow(2.0f, bits);
    
    // Quantize: scale up, round to nearest level, scale back
    float scaleFactor = (levels - 1.0f) / 2.0f;
    float scaled = (input + 1.0f) * scaleFactor;
    float quantized = std::round(scaled) / scaleFactor - 1.0f;
    
    // Ensure output is in valid range
    return std::max(-1.0f, std::min(1.0f, quantized));
}

void BitCrusher_Simple::updateParameters(const std::map<int, float>& params) {
    // Update parameters directly - no smoothing!
    
    if (params.count(0)) {
        // Bits parameter - map to discrete bit depths
        float value = params.at(0);
        
        // 7 useful bit depth levels
        if (value < 0.143f)      m_bitDepth = 24.0f;  // Clean (no crushing)
        else if (value < 0.286f)  m_bitDepth = 16.0f;  // CD quality
        else if (value < 0.429f)  m_bitDepth = 12.0f;  // Vintage sampler
        else if (value < 0.571f)  m_bitDepth = 8.0f;   // 8-bit classic
        else if (value < 0.714f)  m_bitDepth = 4.0f;   // Heavy crushing
        else if (value < 0.857f)  m_bitDepth = 2.0f;   // Extreme
        else                      m_bitDepth = 1.0f;   // Maximum destruction
    }
    
    if (params.count(1)) {
        // Sample rate reduction - map to musical divisions
        float value = params.at(1);
        
        if (value < 0.125f)      m_sampleRateReduction = 1.0f;   // No reduction
        else if (value < 0.25f)   m_sampleRateReduction = 2.0f;   // Half rate
        else if (value < 0.375f)  m_sampleRateReduction = 4.0f;   // Quarter rate
        else if (value < 0.5f)    m_sampleRateReduction = 8.0f;   // 1/8 rate
        else if (value < 0.625f)  m_sampleRateReduction = 16.0f;  // 1/16 rate
        else if (value < 0.75f)   m_sampleRateReduction = 32.0f;  // 1/32 rate
        else if (value < 0.875f)  m_sampleRateReduction = 64.0f;  // 1/64 rate
        else                      m_sampleRateReduction = 100.0f; // Maximum reduction
    }
    
    if (params.count(2)) {
        m_aliasing = params.at(2);
    }
    
    if (params.count(3)) {
        m_jitter = params.at(3);
    }
    
    if (params.count(4)) {
        m_dcOffset = params.at(4) * 2.0f - 1.0f; // Map 0-1 to -1 to +1
    }
    
    if (params.count(5)) {
        m_gateThreshold = params.at(5);
    }
    
    if (params.count(6)) {
        m_dither = params.at(6);
    }
    
    if (params.count(7)) {
        m_mix = params.at(7);
    }
}

juce::String BitCrusher_Simple::getParameterName(int index) const {
    switch (index) {
        case 0: return "Bits";
        case 1: return "Downsample";
        case 2: return "Aliasing";
        case 3: return "Jitter";
        case 4: return "DC Offset";
        case 5: return "Gate";
        case 6: return "Dither";
        case 7: return "Mix";
        default: return "";
    }
}
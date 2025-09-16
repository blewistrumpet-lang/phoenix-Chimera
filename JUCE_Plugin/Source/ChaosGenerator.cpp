// Simplified Chaos Generator that actually works dramatically
#include "ChaosGenerator.h"
#include <cmath>
#include <algorithm>

ChaosGenerator::ChaosGenerator() {
    // Initialize all parameters to sensible defaults
    m_rate.target = m_rate.current = 0.3f;      // Moderate rate
    m_depth.target = m_depth.current = 0.5f;     // 50% depth
    m_type.target = m_type.current = 0.0f;       // Lorenz attractor
    m_smoothing.target = m_smoothing.current = 0.5f;  // Medium smoothing
    m_modTarget.target = m_modTarget.current = 0.0f;  // Amplitude modulation
    m_sync.target = m_sync.current = 0.0f;       // Free running
    m_seed.target = m_seed.current = 0.5f;       // Default seed
    m_mix.target = m_mix.current = 0.5f;         // 50% wet/dry mix
}

void ChaosGenerator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
        channel.reset(42);
        
        // Warm up chaos systems
        for (int i = 0; i < 500; ++i) {
            channel.lorenz.iterate(0.01, 1.0f, 0.0f);
        }
    }
}

void ChaosGenerator::reset() {
    for (auto& channel : m_channelStates) {
        channel.lorenz.x = 0.1;
        channel.lorenz.y = 0.0;
        channel.lorenz.z = 0.0;
        channel.chaosValue.current = 0.0f;
        channel.chaosValue.target = 0.0f;
        channel.sampleCounter = 0;
        
        // Warm up after reset
        for (int i = 0; i < 500; ++i) {
            channel.lorenz.iterate(0.01, 1.0f, 0.0f);
        }
    }
}

void ChaosGenerator::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update parameters
    m_rate.update();
    m_depth.update();
    m_type.update();
    m_smoothing.update();
    m_modTarget.update();
    m_sync.update();
    m_seed.update();
    m_mix.update();
    
    // Skip if dry
    if (m_mix.current < 0.001f) return;
    
    // Simple LFO-style chaos for testing
    static float phase = 0.0f;
    float rate = 0.5f + m_rate.current * 10.0f; // 0.5 to 10.5 Hz
    float depth = m_depth.current;
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        float* channelData = buffer.getWritePointer(channel);
        auto& state = m_channelStates[channel];
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float dry = input;
            
            // Update chaos every 10 samples for more dramatic effect
            if (++state.sampleCounter >= 10) {
                state.sampleCounter = 0;
                
                // Use Lorenz for real chaos
                float chaosRaw = state.lorenz.iterate(0.01, 1.0f, 0.0f);
                float chaosNorm = std::tanh(chaosRaw / 20.0f); // Normalize to roughly -1 to 1
                state.chaosValue.setTarget(chaosNorm);
            }
            
            // Get smoothed chaos
            float chaos = state.chaosValue.process();
            
            // Apply DRAMATIC modulation based on target
            float modulated = input;
            
            if (m_modTarget.current < 0.33f) {
                // AMPLITUDE - make it very obvious
                float gain = 1.0f + chaos * depth * 3.0f; // Can go from -2 to 4
                gain = std::max(0.0f, std::min(4.0f, gain));
                modulated = input * gain;
            } else if (m_modTarget.current < 0.67f) {
                // FILTER - sweep dramatically
                float cutoff = 200.0f * std::pow(10.0f, chaos * depth * 2.0f); // 20Hz to 20kHz
                cutoff = std::max(20.0f, std::min(20000.0f, cutoff));
                state.filter.setFrequency(cutoff);
                modulated = state.filter.processLowpass(input, m_sampleRate);
            } else {
                // DISTORTION - obvious waveshaping
                float drive = 1.0f + std::abs(chaos) * depth * 20.0f; // Up to 21x drive
                modulated = std::tanh(input * drive) / std::sqrt(drive);
            }
            
            // Also add some tremolo for testing
            phase += rate / m_sampleRate;
            if (phase > 1.0f) phase -= 1.0f;
            float tremolo = 1.0f + std::sin(2.0f * M_PI * phase) * depth * 0.5f;
            modulated *= tremolo;
            
            // Mix
            channelData[sample] = modulated * m_mix.current + dry * (1.0f - m_mix.current);
        }
    }
}

ChaosGenerator::ChaosType ChaosGenerator::getChaosType() const {
    return LORENZ; // Always use Lorenz for now
}

ChaosGenerator::ModTarget ChaosGenerator::getModTarget() const {
    if (m_modTarget.current < 0.33f) return AMPLITUDE;
    else if (m_modTarget.current < 0.67f) return FILTER;
    else return DISTORTION;
}

float ChaosGenerator::applyModulation(float input, float chaos, ModTarget target, ChannelState& state) {
    // Not used in simple version
    return input;
}

void ChaosGenerator::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_rate.target = params.at(0);
    if (params.count(1)) m_depth.target = params.at(1);
    if (params.count(2)) m_type.target = params.at(2);
    if (params.count(3)) m_smoothing.target = params.at(3);
    if (params.count(4)) m_modTarget.target = params.at(4);
    if (params.count(5)) m_sync.target = params.at(5);
    if (params.count(6)) m_seed.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String ChaosGenerator::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Type";
        case 3: return "Smoothing";
        case 4: return "Target";
        case 5: return "Sync";
        case 6: return "Seed";
        case 7: return "Mix";
        default: return "";
    }
}
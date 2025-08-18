#include "ChaosGenerator.h"
#include <cmath>
#include <algorithm>

ChaosGenerator::ChaosGenerator() = default;

void ChaosGenerator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
        channel.reset(42); // Default seed
    }
}

void ChaosGenerator::reset() {
    // Reset all internal state to ensure clean start
    
    for (auto& channel : m_channelStates) {
        // Reset chaos systems to initial conditions
        channel.lorenz.x = 0.1;
        channel.lorenz.y = 0.0;
        channel.lorenz.z = 0.0;
        
        channel.rossler.x = 0.1;
        channel.rossler.y = 0.0;
        channel.rossler.z = 0.0;
        
        channel.henon.x = 0.0;
        channel.henon.y = 0.0;
        
        channel.logistic.x = 0.5;
        
        channel.ikeda.x = 0.1;
        channel.ikeda.y = 0.1;
        
        channel.duffing.x = 0.1;
        channel.duffing.y = 0.0;
        channel.duffing.phase = 0.0;
        
        // Reset modulation processors
        channel.pitchShifter.prepare(); // This clears internal buffers
        
        channel.filter.state1 = 0.0f;
        channel.filter.state2 = 0.0f;
        
        // Reset DC blockers
        channel.inputDCBlocker.reset();
        channel.outputDCBlocker.reset();
        
        // Reset chaos value smoothing
        channel.chaosValue.current = 0.0f;
        channel.chaosValue.target = 0.0f;
        
        // Reset thermal model state
        channel.thermalModel.thermalNoise = 0.0f;
        
        // Reset component aging
        channel.componentAging.age = 0.0f;
        channel.componentAging.drift = 0.0f;
        channel.componentAging.nonlinearity = 0.0f;
        
        // Reset sample counters
        channel.sampleCounter = 0;
        
        // Clear chaos history
        std::fill(channel.chaosHistory.begin(), channel.chaosHistory.end(), 0.0f);
        channel.historyIndex = 0;
    }
    
    // Reset smoothed parameters to their current targets
    m_rate.current = m_rate.target;
    m_depth.current = m_depth.target;
    m_type.current = m_type.target;
    m_smoothing.current = m_smoothing.target;
    m_modTarget.current = m_modTarget.target;
    m_sync.current = m_sync.target;
    m_seed.current = m_seed.target;
    m_mix.current = m_mix.target;
    
    // Reset shared state
    m_lastSeed = m_seed.current;
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void ChaosGenerator::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Convert parameters
    float rate = 0.1f * std::pow(1000.0f, m_rate.current); // 0.1Hz to 100Hz
    float depth = m_depth.current;
    float smoothing = 0.9f + m_smoothing.current * 0.099f; // 0.9 to 0.999
    
    // Check if seed changed
    if (std::abs(m_seed.current - m_lastSeed) > 0.01f) {
        unsigned int seedValue = static_cast<unsigned int>(m_seed.current * 1000000);
        for (auto& channel : m_channelStates) {
            channel.reset(seedValue + (&channel - &m_channelStates[0]));
        }
        m_lastSeed = m_seed.current;
    }
    
    ChaosType chaosType = getChaosType();
    ModTarget modTarget = getModTarget();
    
    // Calculate update interval based on rate
    int updateInterval = static_cast<int>(m_sampleRate / rate);
    updateInterval = std::max(1, updateInterval);
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Set smoothing
        state.chaosValue.setSmoothing(smoothing);
        state.updateInterval = updateInterval;
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float dry = input;
            
            // Update chaos value at specified rate
            if (++state.sampleCounter >= state.updateInterval) {
                state.sampleCounter = 0;
                
                float chaosOutput = 0.0f;
                
                // Update thermal model and get factors
                state.thermalModel.update(m_sampleRate);
                float thermalFactor = state.thermalModel.getThermalFactor();
                float aging = state.componentAging.age;
                
                // Generate chaos based on selected type
                switch (chaosType) {
                    case LORENZ:
                        chaosOutput = state.lorenz.iterate(0.01, thermalFactor, aging);
                        break;
                    case ROSSLER:
                        chaosOutput = state.rossler.iterate(0.01);
                        break;
                    case HENON:
                        chaosOutput = state.henon.iterate();
                        break;
                    case LOGISTIC:
                        chaosOutput = state.logistic.iterate();
                        break;
                    case IKEDA:
                        chaosOutput = state.ikeda.iterate();
                        break;
                    case DUFFING:
                        chaosOutput = state.duffing.iterate(0.01);
                        break;
                }
                
                state.chaosValue.setTarget(chaosOutput * depth);
            }
            
            // Get smoothed chaos value
            float chaos = state.chaosValue.process();
            
            // Apply modulation based on target
            float modulated = applyModulation(input, chaos, modTarget, state);
            
            // Mix with dry signal
            channelData[sample] = modulated * m_mix.current + dry * (1.0f - m_mix.current);
        }
    }
    
    scrubBuffer(buffer);
}

ChaosGenerator::ChaosType ChaosGenerator::getChaosType() const {
    if (m_type.current < 0.17f) return LORENZ;
    else if (m_type.current < 0.33f) return ROSSLER;
    else if (m_type.current < 0.5f) return HENON;
    else if (m_type.current < 0.67f) return LOGISTIC;
    else if (m_type.current < 0.83f) return IKEDA;
    else return DUFFING;
}

ChaosGenerator::ModTarget ChaosGenerator::getModTarget() const {
    if (m_modTarget.current < 0.17f) return AMPLITUDE;
    else if (m_modTarget.current < 0.33f) return PITCH;
    else if (m_modTarget.current < 0.5f) return FILTER;
    else if (m_modTarget.current < 0.67f) return PAN;
    else if (m_modTarget.current < 0.83f) return DISTORTION;
    else return ALL;
}

float ChaosGenerator::applyModulation(float input, float chaos, ModTarget target, ChannelState& state) {
    float output = input;
    
    switch (target) {
        case AMPLITUDE: {
            // Amplitude modulation
            float gain = 1.0f + chaos * 0.5f; // +/- 50%
            output = input * gain;
            break;
        }
        
        case PITCH: {
            // Pitch modulation (+/- 1 octave)
            float pitchFactor = std::pow(2.0f, chaos);
            output = state.pitchShifter.process(input, pitchFactor);
            break;
        }
        
        case FILTER: {
            // Filter frequency modulation
            float baseFreq = 1000.0f;
            float modFreq = baseFreq * std::pow(10.0f, chaos); // +/- 10x
            state.filter.setFrequency(modFreq);
            output = state.filter.processLowpass(input, m_sampleRate);
            break;
        }
        
        case PAN: {
            // Stereo panning (only affects stereo field)
            // This is handled in the stereo processing
            output = input * (1.0f + chaos * 0.5f);
            break;
        }
        
        case DISTORTION: {
            // Chaos-driven waveshaping
            float drive = 1.0f + std::abs(chaos) * 10.0f;
            output = std::tanh(input * drive) / drive;
            break;
        }
        
        case ALL: {
            // Apply multiple modulations
            float gain = 1.0f + chaos * 0.3f;
            output = input * gain;
            
            float pitchFactor = std::pow(2.0f, chaos * 0.5f);
            output = state.pitchShifter.process(output, pitchFactor);
            
            float modFreq = 1000.0f * std::pow(4.0f, chaos * 0.5f);
            state.filter.setFrequency(modFreq);
            output = state.filter.processLowpass(output, m_sampleRate);
            
            float drive = 1.0f + std::abs(chaos) * 3.0f;
            output = std::tanh(output * drive) / drive;
            break;
        }
    }
    
    return output;
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
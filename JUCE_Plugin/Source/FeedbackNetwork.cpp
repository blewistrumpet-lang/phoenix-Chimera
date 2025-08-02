#include "FeedbackNetwork.h"
#include <cmath>

FeedbackNetwork::FeedbackNetwork() = default;

void FeedbackNetwork::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Delay times based on golden ratio for inharmonic relationships
    const float baseDelayTimes[NUM_DELAYS] = {0.11f, 0.17f, 0.29f, 0.47f};
    
    for (auto& channel : m_channelStates) {
        for (int i = 0; i < NUM_DELAYS; ++i) {
            auto& node = channel.nodes[i];
            node.prepare(sampleRate);
            
            // Set different modulation rates for each delay
            node.delay.lfoRate = 0.1f + i * 0.13f; // 0.1Hz to 0.49Hz
            node.delay.modDepth = 5.0f; // ±5 samples modulation
        }
        
        channel.inputGain = 1.0f / std::sqrt(static_cast<float>(NUM_DELAYS));
    }
}

void FeedbackNetwork::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Calculate delay times
    float delayTimes[NUM_DELAYS] = {
        0.11f + m_delayTime * 0.4f,  // 0.11 - 0.51 sec
        0.17f + m_delayTime * 0.6f,  // 0.17 - 0.77 sec
        0.29f + m_delayTime * 0.8f,  // 0.29 - 1.09 sec
        0.47f + m_delayTime * 1.0f   // 0.47 - 1.47 sec
    };
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // Read from all delays
            std::array<float, NUM_DELAYS> delayOutputs;
            for (int i = 0; i < NUM_DELAYS; ++i) {
                float delaySamples = delayTimes[i] * m_sampleRate;
                delayOutputs[i] = state.nodes[i].delay.read(delaySamples);
                
                // Update modulation
                state.nodes[i].delay.updateModulation(m_sampleRate);
            }
            
            // Mix matrix for cross-feeding
            mixMatrix(delayOutputs);
            
            // Process each delay line
            for (int i = 0; i < NUM_DELAYS; ++i) {
                auto& node = state.nodes[i];
                float signal = delayOutputs[i];
                
                // Add input to first two delays for stereo spread
                if (i < 2) {
                    signal += input * state.inputGain * (channel == i ? 1.0f : 0.5f);
                }
                
                // Apply diffusion
                if (m_diffusion > 0.0f) {
                    signal = node.diffusers[0].process(signal) * (1.0f - m_diffusion * 0.5f) +
                            node.diffusers[1].process(signal) * m_diffusion * 0.5f;
                }
                
                // Apply freeze (infinite sustain)
                float feedbackAmount = m_feedback;
                if (m_freeze > 0.5f) {
                    feedbackAmount = 0.99f;
                    signal *= 0.1f; // Reduce input when frozen
                }
                
                // Apply shimmer (octave up)
                if (m_shimmer > 0.0f) {
                    float shimmerSignal = node.shimmer.process(signal, 2.0f); // Octave up
                    
                    // High-pass filter the shimmer
                    float highpassed = shimmerSignal - node.highpassState;
                    node.highpassState += highpassed * 0.99f;
                    
                    signal = signal * (1.0f - m_shimmer * 0.5f) + 
                            highpassed * m_shimmer * 0.5f;
                }
                
                // Damping (gentle lowpass)
                float dampingCutoff = 0.3f + (1.0f - feedbackAmount) * 0.5f;
                node.lowpassState += (signal - node.lowpassState) * dampingCutoff;
                signal = node.lowpassState;
                
                // Cross-feed to other delays
                float crossFeedAmount = m_crossFeed * 0.5f;
                for (int j = 0; j < NUM_DELAYS; ++j) {
                    if (i != j) {
                        float crossSignal = signal * crossFeedAmount * HADAMARD[i][j];
                        state.nodes[j].delay.write(
                            state.nodes[j].delay.buffer[state.nodes[j].delay.writeIndex] + 
                            crossSignal
                        );
                    }
                }
                
                // Write back to delay with feedback
                signal = softClip(signal * feedbackAmount);
                node.delay.write(signal);
            }
            
            // Sum all delay outputs
            float output = 0.0f;
            for (int i = 0; i < NUM_DELAYS; ++i) {
                output += delayOutputs[i] * 0.5f;
            }
            
            // Final soft clipping
            output = softClip(output);
            
            // Mix with dry signal
            channelData[sample] = drySignal * (1.0f - m_mix) + output * m_mix;
        }
    }
}

void FeedbackNetwork::mixMatrix(std::array<float, NUM_DELAYS>& signals) {
    std::array<float, NUM_DELAYS> mixed;
    
    // Hadamard matrix multiplication for decorrelation
    for (int i = 0; i < NUM_DELAYS; ++i) {
        mixed[i] = 0.0f;
        for (int j = 0; j < NUM_DELAYS; ++j) {
            mixed[i] += signals[j] * HADAMARD[i][j];
        }
    }
    
    signals = mixed;
}

float FeedbackNetwork::softClip(float input) {
    // Soft clipping to prevent runaway feedback
    if (std::abs(input) < 0.7f) {
        return input;
    }
    
    float sign = input > 0.0f ? 1.0f : -1.0f;
    return sign * (0.7f + 0.3f * std::tanh((std::abs(input) - 0.7f) * 3.0f));
}

float FeedbackNetwork::analogSaturation(float input, float amount) {
    // Subtle analog-style saturation with component aging
    float agingFactor = 1.0f + m_componentAge * 0.01f;
    float driven = input * (1.0f + amount) * agingFactor;
    return std::tanh(driven * 0.85f) / (0.85f * (1.0f + amount * 0.15f));
}

float FeedbackNetwork::applyVintageWarmth(float input, float thermalFactor) {
    // Add thermal noise and subtle nonlinearity
    float noise = m_thermalModel.thermalNoise;
    float warmed = input + noise;
    
    // Very subtle saturation for warmth
    if (std::abs(warmed) > 0.1f) {
        warmed = std::tanh(warmed * 1.1f) / 1.1f;
    }
    
    return warmed * thermalFactor;
}

void FeedbackNetwork::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_delayTime = params.at(0);
    if (params.count(1)) m_feedback = params.at(1) * 0.98f; // Max 98% to prevent runaway
    if (params.count(2)) m_crossFeed = params.at(2);
    if (params.count(3)) m_diffusion = params.at(3);
    if (params.count(4)) {
        m_modulation = params.at(4);
        // Update modulation depth for all delays
        for (auto& channel : m_channelStates) {
            for (auto& node : channel.nodes) {
                node.delay.modDepth = 5.0f + m_modulation * 20.0f; // 5-25 samples
            }
        }
    }
    if (params.count(5)) m_freeze = params.at(5);
    if (params.count(6)) m_shimmer = params.at(6);
    if (params.count(7)) m_mix = params.at(7);
}

juce::String FeedbackNetwork::getParameterName(int index) const {
    switch (index) {
        case 0: return "Delay Time";
        case 1: return "Feedback";
        case 2: return "Cross Feed";
        case 3: return "Diffusion";
        case 4: return "Modulation";
        case 5: return "Freeze";
        case 6: return "Shimmer";
        case 7: return "Mix";
        default: return "";
    }
}
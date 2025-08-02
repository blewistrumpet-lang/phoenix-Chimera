#include "ShimmerReverb.h"
#include <cmath>

// Freeverb tuning values (scaled for 44.1kHz)
const int ShimmerReverb::combTuning[NUM_COMBS] = {
    1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
};

const int ShimmerReverb::allpassTuning[NUM_ALLPASS] = {
    556, 441, 341, 225
};

ShimmerReverb::ShimmerReverb() = default;

void ShimmerReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_channelStates.clear();
    m_channelStates.resize(2); // Stereo
    
    const float sampleRateScale = static_cast<float>(sampleRate / 44100.0);
    
    for (int ch = 0; ch < 2; ++ch) {
        auto& state = m_channelStates[ch];
        
        // Initialize comb filters with slight stereo spread
        for (int i = 0; i < NUM_COMBS; ++i) {
            int size = static_cast<int>(combTuning[i] * sampleRateScale);
            if (ch == 1) size += 23; // Stereo spread
            state.combFilters[i].setSize(size);
        }

void ShimmerReverb::reset() {
    // Clear all reverb buffers
    for (auto& channel : m_channelStates) {
        channel.clear();
    }
    // Reset any additional reverb state
}

        
        // Initialize allpass filters
        for (int i = 0; i < NUM_ALLPASS; ++i) {
            int size = static_cast<int>(allpassTuning[i] * sampleRateScale);
            if (ch == 1) size += 11; // Stereo spread
            state.allpassFilters[i].setSize(size);
        }
        
        // Initialize pitch shifter
        state.shifter.prepare(samplesPerBlock);
        
        // Clear pre-delay buffer
        std::fill(std::begin(state.preDelayBuffer), std::end(state.preDelayBuffer), 0.0f);
        state.preDelayPos = 0;
        state.modulationPhase = ch * M_PI; // 180 degrees phase offset for stereo
    }
    
    // Initialize DC blockers
    m_inputDCBlockers.fill({});
    m_outputDCBlockers.fill({});
    
    // Prepare oversampler
    if (m_useOversampling) {
        m_oversampler.prepare(samplesPerBlock);
    }
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Reset thermal model
    m_thermalModel = ThermalModel();
    
    updateInternalParameters();
}

void ShimmerReverb::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_roomSize.update();
    m_damping.update();
    m_shimmerAmount.update();
    m_shimmerPitch.update();
    m_predelay.update();
    m_modulation.update();
    m_highCut.update();
    m_mix.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 10) { // Every 10 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00005f);
        m_sampleCount = 0;
        
        // Update channel aging
        for (auto& state : m_channelStates) {
            state.updateAging(m_componentAge);
        }
    }
    
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= m_channelStates.size()) continue;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // DC block input
            input = m_inputDCBlockers[channel].process(input);
            
            // Pre-delay
            if (state.preDelaySamples > 0) {
                float delayed = state.preDelayBuffer[state.preDelayPos];
                state.preDelayBuffer[state.preDelayPos] = input;
                state.preDelayPos = (state.preDelayPos + 1) % state.preDelaySamples;
                input = delayed;
            }
            
            // Process through comb filters in parallel
            float combSum = 0.0f;
            for (int i = 0; i < NUM_COMBS; ++i) {
                // Apply modulation to comb delay times with thermal variation
                if (m_modulation.current > 0.0f) {
                    float mod = std::sin(state.modulationPhase + i * 0.5f) * m_modulation.current * 0.001f * thermalFactor;
                    state.modulationPhase += 2.0f * M_PI * (0.5f + i * 0.1f) / m_sampleRate;
                    if (state.modulationPhase > 2.0f * M_PI) {
                        state.modulationPhase -= 2.0f * M_PI;
                    }
                }
                
                combSum += state.combFilters[i].processWithAging(input, m_componentAge);
            }
            combSum *= 0.125f; // Average the comb outputs
            
            // Process through allpass filters in series
            float allpassOut = combSum;
            for (int i = 0; i < NUM_ALLPASS; ++i) {
                allpassOut = state.allpassFilters[i].process(allpassOut);
            }
            
            // Apply shimmer effect (pitch shifting) with oversampling
            float shimmerSignal = 0.0f;
            if (m_shimmerAmount.current > 0.0f) {
                if (m_useOversampling) {
                    shimmerSignal = state.shifter.processWithOversampling(allpassOut, m_oversampler) * m_shimmerAmount.current;
                } else {
                    shimmerSignal = state.shifter.process(allpassOut) * m_shimmerAmount.current;
                }
                
                // Add thermal variations to shimmer
                shimmerSignal *= thermalFactor;
                
                // Feed shimmer back into reverb for cascading effect
                for (int i = 0; i < NUM_COMBS; ++i) {
                    state.combFilters[i].processWithAging(shimmerSignal * 0.3f, m_componentAge);
                }
            }
            
            // Combine reverb and shimmer
            float output = allpassOut + shimmerSignal;
            
            // High cut filter with aging effects
            if (m_highCut.current < 1.0f) {
                float cutoff = 20.0f + m_highCut.current * m_highCut.current * 19980.0f;
                // Aging reduces high frequency response
                cutoff *= (1.0f - m_componentAge * 0.15f);
                float alpha = 1.0f - std::exp(-2.0f * M_PI * cutoff / m_sampleRate);
                state.highCutState += alpha * (output - state.highCutState);
                output = state.highCutState;
            }
            
            // Add thermal noise to reverb tail
            if (m_componentAge > 0.01f) {
                output += state.thermalNoise * ((rand() % 1000) / 1000.0f - 0.5f) * 0.3f;
            }
            
            // DC block output
            output = m_outputDCBlockers[channel].process(output);
            
            // Analog-style saturation with aging
            if (std::abs(output) > 0.8f) {
                float saturation = 1.0f + m_componentAge * 0.1f;
                output = std::tanh(output * saturation) / saturation;
            }
            
            // Mix with dry signal (aging affects mix slightly)
            float wetAmount = m_mix.current * (1.0f - m_componentAge * 0.03f);
            channelData[sample] = drySignal * (1.0f - wetAmount) + output * wetAmount;
        }
    }
}

float ShimmerReverb::PitchShifter::process(float input) {
    // Store input in circular buffer
    inputBuffer[inputPos] = input;
    inputPos = (inputPos + 1) % inputBuffer.size();
    
    float output = 0.0f;
    
    // Process four overlapping grains for smoother shimmer
    for (int g = 0; g < NUM_GRAINS; ++g) {
        // Calculate grain envelope (Hann window)
        float env = 0.5f - 0.5f * std::cos(2.0f * M_PI * grainPhase[g]);
        
        // Read from input buffer with pitch shift
        float readPos = inputPos - GRAIN_SIZE + grainPos[g] * pitchRatio;
        while (readPos < 0) readPos += inputBuffer.size();
        while (readPos >= inputBuffer.size()) readPos -= inputBuffer.size();
        
        int readIdx = static_cast<int>(readPos);
        float frac = readPos - readIdx;
        int nextIdx = (readIdx + 1) % inputBuffer.size();
        int prevIdx = (readIdx - 1 + inputBuffer.size()) % inputBuffer.size();
        int next2Idx = (readIdx + 2) % inputBuffer.size();
        
        // Hermite interpolation for better quality
        float y0 = inputBuffer[prevIdx];
        float y1 = inputBuffer[readIdx];
        float y2 = inputBuffer[nextIdx];
        float y3 = inputBuffer[next2Idx];
        
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 1.5f * (y1 - y2) + 0.5f * (y3 - y0);
        
        float sample = ((c3 * frac + c2) * frac + c1) * frac + c0;
        
        // Apply envelope and accumulate
        output += sample * env;
        
        // Update grain position
        grainPos[g]++;
        grainPhase[g] = static_cast<float>(grainPos[g]) / GRAIN_SIZE;
        
        // Reset grain when it completes
        if (grainPos[g] >= GRAIN_SIZE / pitchRatio) {
            grainPos[g] = 0;
            grainPhase[g] = static_cast<float>(g) / NUM_GRAINS; // Maintain phase offset
        }
    }
    
    return output * (0.5f / NUM_GRAINS); // Scale for overlap
}

float ShimmerReverb::PitchShifter::processWithOversampling(float input, Oversampler& oversampler) {
    // For now, just use regular processing (oversampling can be added later if needed)
    return process(input);
}

void ShimmerReverb::updateInternalParameters() {
    // Update comb filter parameters with thermal and aging effects
    for (auto& state : m_channelStates) {
        for (int i = 0; i < NUM_COMBS; ++i) {
            float thermalFactor = m_thermalModel.getThermalFactor();
            state.combFilters[i].feedback = (m_roomSize.current * 0.9f) * thermalFactor * (1.0f + m_componentAge * 0.05f);
            state.combFilters[i].damp = (m_damping.current * 0.4f) * (1.0f + m_componentAge * 0.08f);
        }
        
        // Update allpass filter parameters
        for (int i = 0; i < NUM_ALLPASS; ++i) {
            state.allpassFilters[i].feedback = 0.5f;
        }
        
        // Update pitch shifter with thermal stability
        float semitones = m_shimmerPitch.current * m_thermalModel.getThermalFactor();
        state.shifter.pitchRatio = std::pow(2.0f, semitones / 12.0f);
        
        // Update pre-delay with aging drift
        state.preDelaySamples = static_cast<int>(m_predelay.current * m_sampleRate * 0.001f * (1.0f + state.componentDrift));
        if (state.preDelaySamples >= 44100) state.preDelaySamples = 44099;
    }
}

void ShimmerReverb::updateParameters(const std::map<int, float>& params) {
    bool needsUpdate = false;
    
    if (params.count(0)) {
        m_roomSize.target = params.at(0);
        needsUpdate = true;
    }
    if (params.count(1)) {
        m_damping.target = params.at(1);
        needsUpdate = true;
    }
    if (params.count(2)) m_shimmerAmount.target = params.at(2);
    if (params.count(3)) {
        m_shimmerPitch.target = params.at(3) * 24.0f; // 0 to 24 semitones (2 octaves)
        needsUpdate = true;
    }
    if (params.count(4)) {
        m_predelay.target = params.at(4) * 200.0f; // 0 to 200ms
        needsUpdate = true;
    }
    if (params.count(5)) m_modulation.target = params.at(5);
    if (params.count(6)) m_highCut.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
    
    if (needsUpdate) {
        updateInternalParameters();
    }
}

juce::String ShimmerReverb::getParameterName(int index) const {
    switch (index) {
        case 0: return "Size";
        case 1: return "Damping";
        case 2: return "Shimmer";
        case 3: return "Pitch";
        case 4: return "PreDelay";
        case 5: return "Modulation";
        case 6: return "HighCut";
        case 7: return "Mix";
        default: return "";
    }
}
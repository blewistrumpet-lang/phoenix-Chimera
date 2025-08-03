#include "WaveFolder.h"
#include <cmath>

WaveFolder::WaveFolder() : m_rng(std::random_device{}()) {
    // Initialize smooth parameters
    m_foldAmount.setImmediate(0.5f);
    m_asymmetry.setImmediate(0.0f);
    m_dcOffset.setImmediate(0.0f);
    m_preGain.setImmediate(1.0f);
    m_postGain.setImmediate(1.0f);
    m_smoothing.setImmediate(0.5f);
    m_harmonics.setImmediate(0.0f);
    m_mix.setImmediate(1.0f);
    
    // Set smoothing rates
    m_foldAmount.setSmoothingRate(0.99f);
    m_asymmetry.setSmoothingRate(0.995f);
    m_dcOffset.setSmoothingRate(0.995f);
    m_preGain.setSmoothingRate(0.99f);
    m_postGain.setSmoothingRate(0.99f);
    m_smoothing.setSmoothingRate(0.999f);
    m_harmonics.setSmoothingRate(0.995f);
    m_mix.setSmoothingRate(0.999f);
}

void WaveFolder::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize channel states
    for (auto& state : m_channelStates) {
        state.lastInput = 0.0f;
        state.lastOutput = 0.0f;
        state.dcBlockerState = 0.0f;
        state.smoothState = 0.0f;
        state.harmonicFilter1 = 0.0f;
        state.harmonicFilter2 = 0.0f;
        state.harmonicFilter3 = 0.0f;
        state.componentDrift = 0.0f;
        state.thermalFactor = 1.0f;
    }
    
    // Initialize DC blockers
    for (auto& blocker : m_inputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    for (auto& blocker : m_outputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    // Prepare oversampler
    m_oversampler.prepare(samplesPerBlock);
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void WaveFolder::reset() {
    // Reset all channel states
    for (auto& state : m_channelStates) {
        state.lastInput = 0.0f;
        state.lastOutput = 0.0f;
        state.dcBlockerState = 0.0f;
        state.smoothState = 0.0f;
        state.harmonicFilter1 = 0.0f;
        state.harmonicFilter2 = 0.0f;
        state.harmonicFilter3 = 0.0f;
        state.componentDrift = 0.0f;
        state.thermalFactor = 1.0f;
    }
    
    // Reset DC blockers
    for (auto& blocker : m_inputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    for (auto& blocker : m_outputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    // Reset thermal model
    m_thermalModel.temperature = 25.0f;
    m_thermalModel.thermalNoise = 0.0f;
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Reset oversampling filters
    if (m_useOversampling) {
        m_oversampler.upsampleFilter.x.fill(0.0f);
        m_oversampler.upsampleFilter.y.fill(0.0f);
        m_oversampler.downsampleFilter.x.fill(0.0f);
        m_oversampler.downsampleFilter.y.fill(0.0f);
    }
}

void WaveFolder::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_foldAmount.update();
    m_asymmetry.update();
    m_dcOffset.update();
    m_preGain.update();
    m_postGain.update();
    m_smoothing.update();
    m_harmonics.update();
    m_mix.update();
    
    // Update thermal model periodically
    m_sampleCount += numSamples;
    if (m_sampleCount >= static_cast<int>(m_sampleRate * 0.1)) { // Every 100ms
        m_thermalModel.update(m_sampleRate);
        m_componentAge += 0.0001f; // Slow aging
        m_sampleCount = 0;
    }
    
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Apply input DC blocking
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = m_inputDCBlockers[channel].process(channelData[sample]);
        }
        
        // Update component aging for this channel
        state.componentDrift += (m_distribution(m_rng) * 0.00001f) * m_componentAge;
        state.componentDrift = std::max(-0.01f, std::min(0.01f, state.componentDrift));
        state.thermalFactor = thermalFactor * (1.0f + state.componentDrift);
        
        // Process with oversampling for high fold amounts
        if (m_useOversampling && m_foldAmount.current > 0.3f) {
            // Upsample
            m_oversampler.upsample(channelData, m_oversampler.upsampleBuffer.data(), numSamples);
            
            // Process at higher sample rate
            for (int sample = 0; sample < numSamples * 4; ++sample) {
                float input = m_oversampler.upsampleBuffer[sample];
                float drySignal = input;
                
                // Apply pre-gain and DC offset with thermal effects
                input *= m_preGain.current * state.thermalFactor;
                input += m_dcOffset.current * 0.1f * state.thermalFactor;
                
                // Smooth input transitions to reduce aliasing
                if (m_smoothing.current > 0.0f) {
                    input = smoothTransition(input, state.lastInput, m_smoothing.current);
                    state.lastInput = input;
                }
                
                // Apply wavefolding with aging
                float folded = processWavefoldingWithAging(input, m_foldAmount.current, m_asymmetry.current, m_componentAge);
                
                // Apply harmonic emphasis with aging
                if (m_harmonics.current > 0.0f) {
                    folded = processHarmonicEmphasisWithAging(folded, state, m_componentAge);
                }
                
                // Apply post-gain with thermal effects
                folded *= m_postGain.current * state.thermalFactor;
                
                // Apply soft clipping for analog warmth
                folded = softClipWithAging(folded, m_componentAge);
                
                // Mix with dry signal
                m_oversampler.downsampleBuffer[sample] = drySignal * (1.0f - m_mix.current) + folded * m_mix.current;
            }
            
            // Downsample
            m_oversampler.downsample(m_oversampler.downsampleBuffer.data(), channelData, numSamples);
        } else {
            // Standard processing without oversampling
            for (int sample = 0; sample < numSamples; ++sample) {
                float input = channelData[sample];
                float drySignal = input;
                
                // Apply pre-gain and DC offset with thermal effects
                input *= m_preGain.current * state.thermalFactor;
                input += m_dcOffset.current * 0.1f * state.thermalFactor;
                
                // Smooth input transitions to reduce aliasing
                if (m_smoothing.current > 0.0f) {
                    input = smoothTransition(input, state.lastInput, m_smoothing.current);
                    state.lastInput = input;
                }
                
                // Apply wavefolding with aging
                float folded = processWavefoldingWithAging(input, m_foldAmount.current, m_asymmetry.current, m_componentAge);
                
                // Apply harmonic emphasis with aging
                if (m_harmonics.current > 0.0f) {
                    folded = processHarmonicEmphasisWithAging(folded, state, m_componentAge);
                }
                
                // Apply post-gain with thermal effects
                folded *= m_postGain.current * state.thermalFactor;
                
                // Apply soft clipping for analog warmth
                folded = softClipWithAging(folded, m_componentAge);
                
                // Mix with dry signal
                channelData[sample] = drySignal * (1.0f - m_mix.current) + folded * m_mix.current;
            }
        }
        
        // Apply output DC blocking
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = m_outputDCBlockers[channel].process(channelData[sample]);
        }
    }
}

float WaveFolder::processWavefolding(float input, float amount, float asymmetry) {
    // Scale folding threshold based on amount
    float threshold = 1.0f - amount * 0.95f; // 0.05 to 1.0
    
    // Apply asymmetry to positive and negative thresholds
    float posThreshold = threshold * (1.0f + asymmetry);
    float negThreshold = -threshold * (1.0f - asymmetry);
    
    float output = input;
    
    // Continuous folding algorithm
    while (output > posThreshold || output < negThreshold) {
        if (output > posThreshold) {
            // Fold down from positive threshold
            float excess = output - posThreshold;
            output = posThreshold - excess;
            
            // Check if we've folded into negative territory
            if (output < negThreshold) {
                excess = negThreshold - output;
                output = negThreshold + excess;
            }
        } else if (output < negThreshold) {
            // Fold up from negative threshold
            float excess = output - negThreshold;
            output = negThreshold - excess;
            
            // Check if we've folded into positive territory
            if (output > posThreshold) {
                excess = output - posThreshold;
                output = posThreshold - excess;
            }
        }
    }
    
    // Soft clipping at the edges for smoother sound
    if (std::abs(output) > 0.95f) {
        float sign = output > 0.0f ? 1.0f : -1.0f;
        float x = std::abs(output) - 0.95f;
        output = sign * (0.95f + std::tanh(x * 5.0f) * 0.05f);
    }
    
    return output;
}

float WaveFolder::smoothTransition(float input, float lastInput, float smoothing) {
    // Anti-derivative anti-aliasing inspired smoothing
    float diff = input - lastInput;
    float maxDiff = smoothing * 0.1f;
    
    if (std::abs(diff) > maxDiff) {
        // Limit the rate of change
        input = lastInput + (diff > 0 ? maxDiff : -maxDiff);
    }
    
    // Additional low-pass filtering
    const float cutoff = 1.0f - smoothing * 0.5f;
    return input * cutoff + lastInput * (1.0f - cutoff);
}

float WaveFolder::processHarmonicEmphasis(float input, ChannelState& state) {
    // Multi-band harmonic emphasis using simple resonant filters
    
    // 2nd harmonic emphasis (around 1-2kHz for typical audio)
    float freq1 = 1500.0f / m_sampleRate;
    float res1 = 2.0f + m_harmonics.current * 3.0f;
    float band1 = input - state.harmonicFilter1;
    state.harmonicFilter1 += band1 * freq1 * 2.0f;
    float peak1 = band1 * res1;
    
    // 3rd harmonic emphasis (around 2-3kHz)
    float freq2 = 2500.0f / m_sampleRate;
    float res2 = 2.0f + m_harmonics.current * 2.5f;
    float band2 = input - state.harmonicFilter2;
    state.harmonicFilter2 += band2 * freq2 * 2.0f;
    float peak2 = band2 * res2;
    
    // 4th harmonic emphasis (around 3-4kHz)
    float freq3 = 3500.0f / m_sampleRate;
    float res3 = 2.0f + m_harmonics.current * 2.0f;
    float band3 = input - state.harmonicFilter3;
    state.harmonicFilter3 += band3 * freq3 * 2.0f;
    float peak3 = band3 * res3;
    
    // Mix emphasized harmonics back with original
    return input + (peak1 + peak2 * 0.7f + peak3 * 0.5f) * m_harmonics.current * 0.3f;
}

float WaveFolder::processDCBlocker(float input, ChannelState& state) {
    // DC blocking filter
    const float cutoff = 20.0f / m_sampleRate;
    const float alpha = 1.0f - std::exp(-2.0f * M_PI * cutoff);
    
    float output = input - state.dcBlockerState;
    state.dcBlockerState += alpha * output;
    
    return output;
}

void WaveFolder::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_foldAmount.target = params.at(0);
    if (params.count(1)) m_asymmetry.target = params.at(1) * 2.0f - 1.0f; // -1 to +1
    if (params.count(2)) m_dcOffset.target = params.at(2) * 0.5f - 0.25f; // -0.25 to +0.25
    if (params.count(3)) m_preGain.target = 0.1f + params.at(3) * 3.9f; // 0.1 to 4.0
    if (params.count(4)) m_postGain.target = 0.1f + params.at(4) * 1.9f; // 0.1 to 2.0
    if (params.count(5)) m_smoothing.target = params.at(5);
    if (params.count(6)) m_harmonics.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String WaveFolder::getParameterName(int index) const {
    switch (index) {
        case 0: return "Fold";
        case 1: return "Asymmetry";
        case 2: return "DC Offset";
        case 3: return "Pre Gain";
        case 4: return "Post Gain";
        case 5: return "Smoothing";
        case 6: return "Harmonics";
        case 7: return "Mix";
        default: return "";
    }
}

float WaveFolder::processWavefoldingWithAging(float input, float amount, float asymmetry, float aging) {
    // Apply aging effects - drift in thresholds and additional nonlinearity
    float agingFactor = 1.0f + aging * 0.15f;
    float agingAsymmetry = asymmetry + aging * 0.1f;
    
    // Scale folding threshold based on amount with aging
    float threshold = (1.0f - amount * 0.95f) * agingFactor; // 0.05 to 1.0
    
    // Apply asymmetry to positive and negative thresholds
    float posThreshold = threshold * (1.0f + agingAsymmetry);
    float negThreshold = -threshold * (1.0f - agingAsymmetry);
    
    float output = input;
    
    // Continuous folding algorithm with aging effects
    int foldCount = 0;
    const int maxFolds = 8; // Prevent infinite loops
    
    while ((output > posThreshold || output < negThreshold) && foldCount < maxFolds) {
        if (output > posThreshold) {
            // Fold down from positive threshold
            float excess = output - posThreshold;
            output = posThreshold - excess * (1.0f + aging * 0.05f); // Aging affects fold shape
            
            // Check if we've folded into negative territory
            if (output < negThreshold) {
                excess = negThreshold - output;
                output = negThreshold + excess * (1.0f + aging * 0.03f);
            }
        } else if (output < negThreshold) {
            // Fold up from negative threshold
            float excess = output - negThreshold;
            output = negThreshold - excess * (1.0f + aging * 0.05f);
            
            // Check if we've folded into positive territory
            if (output > posThreshold) {
                excess = output - posThreshold;
                output = posThreshold - excess * (1.0f + aging * 0.03f);
            }
        }
        foldCount++;
    }
    
    // Soft clipping at the edges for smoother sound with aging
    if (std::abs(output) > 0.95f) {
        float sign = output > 0.0f ? 1.0f : -1.0f;
        float x = std::abs(output) - 0.95f;
        float agingClip = 5.0f + aging * 2.0f; // Aging increases clipping intensity
        output = sign * (0.95f + std::tanh(x * agingClip) * 0.05f);
    }
    
    return output;
}

float WaveFolder::processHarmonicEmphasisWithAging(float input, ChannelState& state, float aging) {
    // Multi-band harmonic emphasis using simple resonant filters with aging
    float agingFactor = 1.0f + aging * 0.1f;
    
    // 2nd harmonic emphasis (around 1-2kHz for typical audio) with aging drift
    float freq1 = (1500.0f + aging * 200.0f) / m_sampleRate;
    float res1 = (2.0f + m_harmonics.current * 3.0f) * agingFactor;
    float band1 = input - state.harmonicFilter1;
    state.harmonicFilter1 += band1 * freq1 * 2.0f;
    float peak1 = band1 * res1;
    
    // 3rd harmonic emphasis (around 2-3kHz) with aging drift
    float freq2 = (2500.0f + aging * 300.0f) / m_sampleRate;
    float res2 = (2.0f + m_harmonics.current * 2.5f) * agingFactor;
    float band2 = input - state.harmonicFilter2;
    state.harmonicFilter2 += band2 * freq2 * 2.0f;
    float peak2 = band2 * res2;
    
    // 4th harmonic emphasis (around 3-4kHz) with aging drift
    float freq3 = (3500.0f + aging * 400.0f) / m_sampleRate;
    float res3 = (2.0f + m_harmonics.current * 2.0f) * agingFactor;
    float band3 = input - state.harmonicFilter3;
    state.harmonicFilter3 += band3 * freq3 * 2.0f;
    float peak3 = band3 * res3;
    
    // Mix emphasized harmonics back with original
    float emphasis = (peak1 + peak2 * 0.7f + peak3 * 0.5f) * m_harmonics.current * 0.3f;
    
    // Add slight nonlinearity due to aging
    if (aging > 0.01f) {
        emphasis += aging * 0.02f * std::tanh(emphasis * 3.0f);
    }
    
    return input + emphasis;
}

float WaveFolder::softClip(float input) {
    // Soft clipping using tanh for analog warmth
    return std::tanh(input * 0.7f);
}

float WaveFolder::softClipWithAging(float input, float aging) {
    // Apply aging effects - increased saturation and slight asymmetry
    float agingFactor = 1.0f + aging * 0.25f;
    float asymmetry = aging * 0.1f;
    
    // Asymmetric soft clipping with aging
    if (input > 0.0f) {
        float clipped = std::tanh(input * 0.7f * agingFactor);
        // Add aging harmonics
        if (aging > 0.01f) {
            clipped += aging * 0.05f * std::sin(input * 3.14159f * 2.0f);
        }
        return clipped;
    } else {
        float clipped = std::tanh(input * 0.7f * agingFactor * (1.0f + asymmetry));
        // Add aging harmonics
        if (aging > 0.01f) {
            clipped += aging * 0.03f * std::sin(input * 3.14159f * 3.0f);
        }
        return clipped;
    }
}
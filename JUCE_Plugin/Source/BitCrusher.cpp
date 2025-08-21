#include "BitCrusher.h"
#include "DspEngineUtilities.h"
#include <cmath>

BitCrusher::BitCrusher() : m_rng(std::random_device{}()) {
    // Initialize smooth parameters
    m_bitDepth.setImmediate(16.0f);
    m_sampleRateReduction.setImmediate(1.0f);
    m_aliasing.setImmediate(0.0f);
    m_jitter.setImmediate(0.0f);
    m_dcOffset.setImmediate(0.0f);
    m_gateThreshold.setImmediate(0.0f);
    m_dither.setImmediate(0.0f);
    m_mix.setImmediate(1.0f);
    
    // Set smoothing rates (faster for effect parameters, slower for mix)
    m_bitDepth.setSmoothingRate(0.99f);
    m_sampleRateReduction.setSmoothingRate(0.99f);
    m_aliasing.setSmoothingRate(0.995f);
    m_jitter.setSmoothingRate(0.995f);
    m_dcOffset.setSmoothingRate(0.995f);
    m_gateThreshold.setSmoothingRate(0.99f);
    m_dither.setSmoothingRate(0.995f);
    m_mix.setSmoothingRate(0.999f);
}

void BitCrusher::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize channel states
    for (auto& state : m_channelStates) {
        state.heldSample = 0.0f;
        state.sampleCounter = 0.0f;
        state.lastInput = 0.0f;
        state.lastOutput = 0.0f;
        state.dcBlockerState = 0.0f;
        state.noiseShaping = 0.0f;
        state.ditherError = 0.0f;
        state.lpf1State = 0.0f;
        state.lpf2State = 0.0f;
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

void BitCrusher::reset() {
    // Reset all smooth parameters to their current targets (no smoothing jump)
    m_bitDepth.current = m_bitDepth.target;
    m_sampleRateReduction.current = m_sampleRateReduction.target;
    m_aliasing.current = m_aliasing.target;
    m_jitter.current = m_jitter.target;
    m_dcOffset.current = m_dcOffset.target;
    m_gateThreshold.current = m_gateThreshold.target;
    m_dither.current = m_dither.target;
    m_mix.current = m_mix.target;
    
    // Reset all channel states
    for (auto& state : m_channelStates) {
        state.heldSample = 0.0f;
        state.sampleCounter = 0.0f;
        state.lastInput = 0.0f;
        state.lastOutput = 0.0f;
        state.dcBlockerState = 0.0f;
        state.noiseShaping = 0.0f;
        state.ditherError = 0.0f;
        state.lpf1State = 0.0f;
        state.lpf2State = 0.0f;
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
    
    // Reset oversampler filter states
    m_oversampler.upsampleFilter.x.fill(0.0f);
    m_oversampler.upsampleFilter.y.fill(0.0f);
    m_oversampler.downsampleFilter.x.fill(0.0f);
    m_oversampler.downsampleFilter.y.fill(0.0f);
}

void BitCrusher::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_bitDepth.update();
    m_sampleRateReduction.update();
    m_aliasing.update();
    m_jitter.update();
    m_dcOffset.update();
    m_gateThreshold.update();
    m_dither.update();
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
        
        // Process with oversampling if enabled
        if (m_useOversampling && m_bitDepth.current < 16.0f) {
            // Upsample
            m_oversampler.upsample(channelData, m_oversampler.upsampleBuffer.data(), numSamples);
            
            // Process at higher sample rate
            for (int sample = 0; sample < numSamples * 2; ++sample) {
                float input = m_oversampler.upsampleBuffer[sample];
                float drySignal = input;
                
                // Update component aging for this channel
                state.componentDrift += (m_distribution(m_rng) * 0.00001f) * m_componentAge;
                state.componentDrift = std::max(-0.01f, std::min(0.01f, state.componentDrift));
                state.thermalFactor = thermalFactor * (1.0f + state.componentDrift);
                
                // Apply DC offset with thermal drift
                input += m_dcOffset.current * 0.1f * state.thermalFactor;
                
                // Gate with aging effects
                if (m_gateThreshold.current > 0.0f && std::abs(input) < m_gateThreshold.current * 0.1f * state.thermalFactor) {
                    input = 0.0f;
                }
                
                // Sample rate reduction with jitter
                state.sampleCounter += m_sampleRateReduction.current * state.thermalFactor;
                if (state.sampleCounter >= 1.0f) {
                    // Add jitter to sample timing
                    float jitterAmount = m_jitter.current * m_distribution(m_rng) * 0.3f;
                    state.sampleCounter = jitterAmount;
                    
                    // Anti-aliasing filter (if not intentionally adding aliasing)
                    if (m_aliasing.current < 0.5f) {
                        float cutoff = 0.5f / (m_sampleRateReduction.current * state.thermalFactor);
                        float alpha = 1.0f - std::exp(-2.0f * M_PI * cutoff);
                        state.lpf1State += alpha * (input - state.lpf1State);
                        state.lpf2State += alpha * (state.lpf1State - state.lpf2State);
                        input = state.lpf2State;
                    }
                    
                    // Apply dither before quantization
                    if (m_dither.current > 0.0f) {
                        input = applyDither(input, m_dither.current, state);
                    }
                    
                    // Bit reduction with aging
                    input = quantizeWithAging(input, m_bitDepth.current, m_componentAge);
                    
                    // Apply soft clipping for analog warmth
                    input = softClipWithAging(input, m_componentAge);
                    
                    // Update held sample
                    state.heldSample = input;
                }
                
                float output = state.heldSample;
                
                // Intentional aliasing (interpolation between samples)
                if (m_aliasing.current > 0.5f) {
                    float aliasingAmount = (m_aliasing.current - 0.5f) * 2.0f;
                    output = state.lastOutput + (output - state.lastOutput) * (1.0f - aliasingAmount);
                }
                
                state.lastOutput = output;
                
                // Mix with dry signal
                m_oversampler.downsampleBuffer[sample] = drySignal * (1.0f - m_mix.current) + output * m_mix.current;
            }
            
            // Downsample
            m_oversampler.downsample(m_oversampler.downsampleBuffer.data(), channelData, numSamples);
        } else {
            // Standard processing without oversampling
            for (int sample = 0; sample < numSamples; ++sample) {
                float input = channelData[sample];
                float drySignal = input;
                
                // Update component aging for this channel
                state.componentDrift += (m_distribution(m_rng) * 0.00001f) * m_componentAge;
                state.componentDrift = std::max(-0.01f, std::min(0.01f, state.componentDrift));
                state.thermalFactor = thermalFactor * (1.0f + state.componentDrift);
                
                // Apply DC offset with thermal drift
                input += m_dcOffset.current * 0.1f * state.thermalFactor;
                
                // Gate with aging effects
                if (m_gateThreshold.current > 0.0f && std::abs(input) < m_gateThreshold.current * 0.1f * state.thermalFactor) {
                    input = 0.0f;
                }
                
                // Sample rate reduction with jitter
                state.sampleCounter += m_sampleRateReduction.current * state.thermalFactor;
                if (state.sampleCounter >= 1.0f) {
                    // Add jitter to sample timing
                    float jitterAmount = m_jitter.current * m_distribution(m_rng) * 0.3f;
                    state.sampleCounter = jitterAmount;
                    
                    // Anti-aliasing filter (if not intentionally adding aliasing)
                    if (m_aliasing.current < 0.5f) {
                        float cutoff = 0.5f / (m_sampleRateReduction.current * state.thermalFactor);
                        float alpha = 1.0f - std::exp(-2.0f * M_PI * cutoff);
                        state.lpf1State += alpha * (input - state.lpf1State);
                        state.lpf2State += alpha * (state.lpf1State - state.lpf2State);
                        input = state.lpf2State;
                    }
                    
                    // Apply dither before quantization
                    if (m_dither.current > 0.0f) {
                        input = applyDither(input, m_dither.current, state);
                    }
                    
                    // Bit reduction with aging
                    input = quantizeWithAging(input, m_bitDepth.current, m_componentAge);
                    
                    // Apply soft clipping for analog warmth
                    input = softClipWithAging(input, m_componentAge);
                    
                    // Update held sample
                    state.heldSample = input;
                }
                
                float output = state.heldSample;
                
                // Intentional aliasing (interpolation between samples)
                if (m_aliasing.current > 0.5f) {
                    float aliasingAmount = (m_aliasing.current - 0.5f) * 2.0f;
                    output = state.lastOutput + (output - state.lastOutput) * (1.0f - aliasingAmount);
                }
                
                state.lastOutput = output;
                
                // Mix with dry signal
                channelData[sample] = drySignal * (1.0f - m_mix.current) + output * m_mix.current;
            }
        }
        
        // Apply output DC blocking
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = m_outputDCBlockers[channel].process(channelData[sample]);
        }
    }
    
    scrubBuffer(buffer);
}

float BitCrusher::quantize(float input, float bits) {
    if (bits >= 32.0f) return input;
    
    // Clamp input
    input = std::max(-1.0f, std::min(1.0f, input));
    
    // Calculate number of quantization levels
    float levels = std::pow(2.0f, bits);
    
    // Quantize
    float quantized = std::round(input * levels * 0.5f) / (levels * 0.5f);
    
    // Add quantization noise shaping
    if (bits < 8.0f) {
        float noise = m_distribution(m_rng) * (1.0f / levels) * 0.5f;
        quantized += noise;
    }
    
    return quantized;
}

float BitCrusher::quantizeWithAging(float input, float bits, float aging) {
    if (bits >= 32.0f) return input;
    
    // Clamp input
    input = std::max(-1.0f, std::min(1.0f, input));
    
    // Apply aging effects - drift in bit depth and additional noise
    float agingFactor = 1.0f + aging * 0.1f;
    float effectiveBits = bits * agingFactor;
    
    // Calculate number of quantization levels
    float levels = std::pow(2.0f, effectiveBits);
    
    // Quantize with aging-induced errors
    float quantized = std::round(input * levels * 0.5f) / (levels * 0.5f);
    
    // Add quantization noise shaping with aging
    if (effectiveBits < 8.0f) {
        float noise = m_distribution(m_rng) * (1.0f / levels) * 0.5f;
        float agingNoise = m_distribution(m_rng) * aging * 0.02f;
        quantized += noise + agingNoise;
    }
    
    return quantized;
}

float BitCrusher::applyDither(float input, float ditherAmount, ChannelState& state) {
    // Triangular probability distribution dither (TPDF)
    float dither1 = m_distribution(m_rng);
    float dither2 = m_distribution(m_rng);
    float triangularDither = (dither1 + dither2) * 0.5f;
    
    // Scale dither based on bit depth
    float ditherScale = ditherAmount / std::pow(2.0f, m_bitDepth.current);
    
    // Apply noise shaping
    float shapedDither = triangularDither + state.ditherError * 0.5f;
    float ditheredInput = input + shapedDither * ditherScale;
    
    // Update error for noise shaping
    float quantized = quantize(ditheredInput, m_bitDepth.current);
    state.ditherError = ditheredInput - quantized;
    
    return ditheredInput;
}

float BitCrusher::processDCBlocker(float input, ChannelState& state) {
    // Simple DC blocking filter
    const float cutoff = 20.0f / m_sampleRate; // 20 Hz highpass
    const float alpha = 1.0f - std::exp(-2.0f * M_PI * cutoff);
    
    float output = input - state.dcBlockerState;
    state.dcBlockerState += alpha * output;
    
    return output;
}

void BitCrusher::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) {
        // Bit depth: 0.0 = bypass (32 bits), 1.0 = maximum crushing (1 bit)
        float bits = params.at(0);
        m_bitDepth.target = bits < 0.01f ? 32.0f : 32.0f - bits * 31.0f;
    }
    if (params.count(1)) {
        // Sample rate reduction: 0.0 = no reduction (bypass), 1.0 = 100x reduction
        float downsample = params.at(1);
        m_sampleRateReduction.target = downsample < 0.01f ? 1.0f : 1.0f + downsample * 99.0f;
    }
    if (params.count(2)) m_aliasing.target = params.at(2);
    if (params.count(3)) m_jitter.target = params.at(3);
    if (params.count(4)) m_dcOffset.target = params.at(4) * 2.0f - 1.0f; // -1 to +1
    if (params.count(5)) m_gateThreshold.target = params.at(5);
    if (params.count(6)) m_dither.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

float BitCrusher::softClip(float input) {
    // Soft clipping using tanh for analog warmth
    return std::tanh(input * 0.7f);
}

float BitCrusher::softClipWithAging(float input, float aging) {
    // Apply aging effects - increased saturation and slight asymmetry
    float agingFactor = 1.0f + aging * 0.2f;
    float asymmetry = aging * 0.1f;
    
    // Asymmetric soft clipping
    if (input > 0.0f) {
        return std::tanh(input * 0.7f * agingFactor);
    } else {
        return std::tanh(input * 0.7f * agingFactor * (1.0f + asymmetry));
    }
}

juce::String BitCrusher::getParameterName(int index) const {
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
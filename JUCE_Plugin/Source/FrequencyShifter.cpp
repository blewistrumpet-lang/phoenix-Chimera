#include "FrequencyShifter.h"
#include <cmath>

FrequencyShifter::FrequencyShifter() : m_rng(std::random_device{}()) {
    // Initialize smooth parameters
    m_shiftAmount.setImmediate(0.0f);
    m_feedback.setImmediate(0.0f);
    m_mix.setImmediate(0.5f);
    m_spread.setImmediate(0.0f);
    m_resonance.setImmediate(0.0f);
    m_modDepth.setImmediate(0.0f);
    m_modRate.setImmediate(0.0f);
    m_direction.setImmediate(0.5f);
    
    // Set smoothing rates
    m_shiftAmount.setSmoothingRate(0.99f);
    m_feedback.setSmoothingRate(0.995f);
    m_mix.setSmoothingRate(0.999f);
    m_spread.setSmoothingRate(0.995f);
    m_resonance.setSmoothingRate(0.995f);
    m_modDepth.setSmoothingRate(0.99f);
    m_modRate.setSmoothingRate(0.995f);
    m_direction.setSmoothingRate(0.995f);
}

void FrequencyShifter::HilbertTransformer::initialize() {
    coefficients.resize(HILBERT_LENGTH);
    delayBuffer.resize(HILBERT_LENGTH);
    
    // Design Hilbert transformer coefficients using windowed sinc
    const int center = HILBERT_LENGTH / 2;
    
    for (int i = 0; i < HILBERT_LENGTH; ++i) {
        if (i == center) {
            coefficients[i] = 0.0f;
        } else {
            int n = i - center;
            // Hilbert transformer impulse response
            float h = 2.0f / (M_PI * n);
            if (n % 2 == 0) {
                h = 0.0f; // Even coefficients are zero
            }
            
            // Apply Blackman window
            float window = 0.42f - 0.5f * std::cos(2.0f * M_PI * i / (HILBERT_LENGTH - 1)) +
                          0.08f * std::cos(4.0f * M_PI * i / (HILBERT_LENGTH - 1));
            
            coefficients[i] = h * window;
        }
    }
    
    std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
    delayIndex = 0;
}

std::complex<float> FrequencyShifter::HilbertTransformer::process(float input) {
    // Store input in delay buffer
    delayBuffer[delayIndex] = input;
    
    // Compute Hilbert transform (imaginary part)
    float hilbertOutput = 0.0f;
    for (int i = 0; i < HILBERT_LENGTH; ++i) {
        int idx = (delayIndex - i + HILBERT_LENGTH) % HILBERT_LENGTH;
        hilbertOutput += delayBuffer[idx] * coefficients[i];
    }
    
    // Get delayed real part (to compensate for Hilbert filter delay)
    int delayCompensation = HILBERT_LENGTH / 2;
    int realIdx = (delayIndex - delayCompensation + HILBERT_LENGTH) % HILBERT_LENGTH;
    float realPart = delayBuffer[realIdx];
    
    // Advance delay index
    delayIndex = (delayIndex + 1) % HILBERT_LENGTH;
    
    return std::complex<float>(realPart, hilbertOutput);
}

void FrequencyShifter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize channel states
    for (auto& state : m_channelStates) {
        state.hilbert.initialize();
        state.oscillatorPhase = 0.0f;
        state.modulatorPhase = 0.0f;
        state.feedbackBuffer.resize(static_cast<size_t>(sampleRate * 0.1)); // 100ms feedback buffer
        std::fill(state.feedbackBuffer.begin(), state.feedbackBuffer.end(), 0.0f);
        state.feedbackIndex = 0;
        state.resonatorReal = 0.0f;
        state.resonatorImag = 0.0f;
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

void FrequencyShifter::reset() {
    // Reset all smooth parameters to their current targets (no smoothing jump)
    m_shiftAmount.current = m_shiftAmount.target;
    m_feedback.current = m_feedback.target;
    m_mix.current = m_mix.target;
    m_spread.current = m_spread.target;
    m_resonance.current = m_resonance.target;
    m_modDepth.current = m_modDepth.target;
    m_modRate.current = m_modRate.target;
    m_direction.current = m_direction.target;
    
    // Reset all channel states
    for (auto& state : m_channelStates) {
        // Reset Hilbert transformer
        std::fill(state.hilbert.delayBuffer.begin(), state.hilbert.delayBuffer.end(), 0.0f);
        state.hilbert.delayIndex = 0;
        
        // Reset oscillator phases
        state.oscillatorPhase = 0.0f;
        state.modulatorPhase = 0.0f;
        
        // Reset feedback buffer
        std::fill(state.feedbackBuffer.begin(), state.feedbackBuffer.end(), 0.0f);
        state.feedbackIndex = 0;
        
        // Reset resonator state
        state.resonatorReal = 0.0f;
        state.resonatorImag = 0.0f;
        
        // Reset component aging
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

void FrequencyShifter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_shiftAmount.update();
    m_feedback.update();
    m_mix.update();
    m_spread.update();
    m_resonance.update();
    m_modDepth.update();
    m_modRate.update();
    m_direction.update();
    
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
        
        // Apply stereo spread with thermal effects
        float channelShift = m_shiftAmount.current * state.thermalFactor;
        if (numChannels == 2 && m_spread.current > 0.0f) {
            float spreadAmount = m_spread.current * 50.0f * state.thermalFactor; // ±50Hz spread
            channelShift += (channel == 0 ? -spreadAmount : spreadAmount);
        }
        
        // Process with oversampling for cleaner frequency shifting
        if (m_useOversampling && std::abs(channelShift) > 100.0f) {
            // Upsample
            m_oversampler.upsample(channelData, m_oversampler.upsampleBuffer.data(), numSamples);
            
            // Process at higher sample rate
            for (int sample = 0; sample < numSamples * 2; ++sample) {
                float input = m_oversampler.upsampleBuffer[sample];
                
                float output = processFrequencyShifterSample(input, channelShift, state, true);
                m_oversampler.downsampleBuffer[sample] = output;
            }
            
            // Downsample
            m_oversampler.downsample(m_oversampler.downsampleBuffer.data(), channelData, numSamples);
        } else {
            // Standard processing without oversampling
            for (int sample = 0; sample < numSamples; ++sample) {
                float input = channelData[sample];
                channelData[sample] = processFrequencyShifterSample(input, channelShift, state, false);
            }
        }
        
        // Apply output DC blocking
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = m_outputDCBlockers[channel].process(channelData[sample]);
        }
    }
}

std::complex<float> FrequencyShifter::processFrequencyShift(std::complex<float> analytic,
                                                           float shiftFreq,
                                                           float& phase) {
    // Single sideband modulation
    float phaseIncrement = 2.0f * M_PI * shiftFreq / m_sampleRate;
    phase += phaseIncrement;
    
    // Wrap phase
    while (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    while (phase < 0.0f) phase += 2.0f * M_PI;
    
    // Complex multiplication for frequency shift
    std::complex<float> oscillator(std::cos(phase), std::sin(phase));
    return analytic * oscillator;
}

void FrequencyShifter::processResonator(std::complex<float>& signal, 
                                       ChannelState& state,
                                       float frequency) {
    // Simple complex resonator
    float omega = 2.0f * M_PI * frequency / m_sampleRate;
    float resonanceAmount = 0.95f * m_resonance;
    
    // Complex exponential rotation
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    
    // Rotate and decay
    float newReal = state.resonatorReal * cosOmega - state.resonatorImag * sinOmega;
    float newImag = state.resonatorReal * sinOmega + state.resonatorImag * cosOmega;
    
    state.resonatorReal = newReal * resonanceAmount + signal.real() * (1.0f - resonanceAmount);
    state.resonatorImag = newImag * resonanceAmount + signal.imag() * (1.0f - resonanceAmount);
    
    // Add resonance to signal
    signal += std::complex<float>(state.resonatorReal, state.resonatorImag) * m_resonance * 0.5f;
}

void FrequencyShifter::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_shiftAmount.target = (params.at(0) - 0.5f) * 2000.0f; // -1000Hz to +1000Hz
    if (params.count(1)) m_feedback.target = params.at(1) * 0.95f;
    if (params.count(2)) m_mix.target = params.at(2);
    if (params.count(3)) m_spread.target = params.at(3);
    if (params.count(4)) m_resonance.target = params.at(4);
    if (params.count(5)) m_modDepth.target = params.at(5);
    if (params.count(6)) m_modRate.target = params.at(6) * 10.0f; // 0-10Hz
    if (params.count(7)) m_direction.target = params.at(7);
}

juce::String FrequencyShifter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Shift";
        case 1: return "Feedback";
        case 2: return "Mix";
        case 3: return "Spread";
        case 4: return "Resonance";
        case 5: return "Mod Depth";
        case 6: return "Mod Rate";
        case 7: return "Direction";
        default: return "";
    }
}

float FrequencyShifter::processFrequencyShifterSample(float input, float channelShift, ChannelState& state, bool isOversampled) {
    // Add feedback with thermal effects
    if (m_feedback.current > 0.0f) {
        input += state.feedbackBuffer[state.feedbackIndex] * m_feedback.current * 0.8f * state.thermalFactor;
    }
    
    // Generate analytic signal
    std::complex<float> analytic = state.hilbert.process(input);
    
    // Apply modulation to shift frequency with thermal effects
    float modulation = 0.0f;
    if (m_modDepth.current > 0.0f) {
        modulation = std::sin(state.modulatorPhase) * m_modDepth.current * 500.0f * state.thermalFactor; // ±500Hz mod
        float modRateWithThermal = m_modRate.current * state.thermalFactor;
        state.modulatorPhase += 2.0f * M_PI * modRateWithThermal / (isOversampled ? m_sampleRate * 2.0 : m_sampleRate);
        if (state.modulatorPhase > 2.0f * M_PI) {
            state.modulatorPhase -= 2.0f * M_PI;
        }
    }
    
    float totalShift = channelShift + modulation;
    
    // Frequency shift with aging
    std::complex<float> shiftedUp = processFrequencyShiftWithAging(analytic, totalShift, 
                                                                  state.oscillatorPhase, m_componentAge);
    std::complex<float> shiftedDown = processFrequencyShiftWithAging(analytic, -totalShift, 
                                                                    state.oscillatorPhase, m_componentAge);
    
    // Apply resonance with aging
    if (m_resonance.current > 0.0f) {
        processResonatorWithAging(shiftedUp, state, std::abs(totalShift), m_componentAge);
        processResonatorWithAging(shiftedDown, state, std::abs(totalShift), m_componentAge);
    }
    
    // Mix up/down/both based on direction
    float output;
    if (m_direction.current < 0.33f) {
        // Down only
        output = shiftedDown.real();
    } else if (m_direction.current > 0.67f) {
        // Up only
        output = shiftedUp.real();
    } else {
        // Both (ring modulation effect)
        float blend = (m_direction.current - 0.33f) * 3.0f;
        output = shiftedDown.real() * (1.0f - blend) + shiftedUp.real() * blend;
    }
    
    // Apply soft clipping for analog warmth
    output = softClipWithAging(output, m_componentAge);
    
    // Update feedback buffer
    if (m_feedback.current > 0.0f) {
        state.feedbackBuffer[state.feedbackIndex] = output;
        state.feedbackIndex = (state.feedbackIndex + 1) % state.feedbackBuffer.size();
    }
    
    // Mix with dry signal
    return input * (1.0f - m_mix.current) + output * m_mix.current;
}

std::complex<float> FrequencyShifter::processFrequencyShiftWithAging(std::complex<float> analytic,
                                                                     float shiftFreq,
                                                                     float& phase,
                                                                     float aging) {
    // Single sideband modulation with aging effects
    float agingFactor = 1.0f + aging * 0.05f;
    float phaseIncrement = 2.0f * M_PI * shiftFreq * agingFactor / m_sampleRate;
    phase += phaseIncrement;
    
    // Wrap phase
    while (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    while (phase < 0.0f) phase += 2.0f * M_PI;
    
    // Complex multiplication for frequency shift with aging-induced phase drift
    float phaseDrift = aging * 0.1f * std::sin(phase * 0.1f);
    std::complex<float> oscillator(std::cos(phase + phaseDrift), std::sin(phase + phaseDrift));
    
    return analytic * oscillator;
}

void FrequencyShifter::processResonatorWithAging(std::complex<float>& signal, 
                                                 ChannelState& state,
                                                 float frequency,
                                                 float aging) {
    // Simple complex resonator with aging effects
    float agingFactor = 1.0f + aging * 0.1f;
    float omega = 2.0f * M_PI * frequency * agingFactor / m_sampleRate;
    float resonanceAmount = 0.95f * m_resonance.current * (1.0f - aging * 0.2f); // Aging reduces Q
    
    // Complex exponential rotation with aging drift
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    
    // Rotate and decay
    float newReal = state.resonatorReal * cosOmega - state.resonatorImag * sinOmega;
    float newImag = state.resonatorReal * sinOmega + state.resonatorImag * cosOmega;
    
    state.resonatorReal = newReal * resonanceAmount + signal.real() * (1.0f - resonanceAmount);
    state.resonatorImag = newImag * resonanceAmount + signal.imag() * (1.0f - resonanceAmount);
    
    // Add resonance to signal with aging effects
    float resonanceGain = m_resonance.current * 0.5f * (1.0f + aging * 0.3f); // Aging increases resonance gain
    signal += std::complex<float>(state.resonatorReal, state.resonatorImag) * resonanceGain;
}

float FrequencyShifter::softClip(float input) {
    // Soft clipping using tanh for analog warmth
    return std::tanh(input * 0.7f);
}

float FrequencyShifter::softClipWithAging(float input, float aging) {
    // Apply aging effects - increased saturation and slight asymmetry
    float agingFactor = 1.0f + aging * 0.2f;
    float asymmetry = aging * 0.1f;
    
    // Asymmetric soft clipping with aging
    if (input > 0.0f) {
        float clipped = std::tanh(input * 0.7f * agingFactor);
        // Add aging harmonics
        if (aging > 0.01f) {
            clipped += aging * 0.03f * std::sin(input * 6.28318f);
        }
        return clipped;
    } else {
        float clipped = std::tanh(input * 0.7f * agingFactor * (1.0f + asymmetry));
        // Add aging harmonics
        if (aging > 0.01f) {
            clipped += aging * 0.02f * std::sin(input * 9.42477f);
        }
        return clipped;
    }
}
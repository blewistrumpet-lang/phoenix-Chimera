#include "AnalogRingModulator.h"
#include <algorithm>

AnalogRingModulator::AnalogRingModulator() : m_rng(std::random_device{}()) {
    m_channels.resize(2);
    
    // Initialize smooth parameters
    m_carrierFreq.setImmediate(440.0f);
    m_ringShiftBlend.setImmediate(0.0f);
    m_carrierDrift.setImmediate(0.0f);
    m_tracking.setImmediate(0.0f);
    
    // Set smoothing rates
    m_carrierFreq.setSmoothingRate(0.99f);
    m_ringShiftBlend.setSmoothingRate(0.995f);
    m_carrierDrift.setSmoothingRate(0.999f);
    m_tracking.setSmoothingRate(0.995f);
}

void AnalogRingModulator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize carrier
    m_carrier.reset();
    m_carrier.frequency = m_carrierFreq.current;
    
    // Initialize channels
    for (auto& channel : m_channels) {
        channel.init();
        channel.reset();
        channel.componentDrift = 0.0f;
        channel.thermalFactor = 1.0f;
    }
}

void AnalogRingModulator::reset() {
    // Reset all internal state
    
    // Reset carrier oscillator
    m_carrier.reset();
    
    // Reset channel states
    for (auto& channel : m_channels) {
        channel.reset();
    }
    
    // Reset DC blockers
    for (auto& dcBlocker : m_inputDCBlockers) {
        dcBlocker.x1 = 0.0f;
        dcBlocker.y1 = 0.0f;
    }
    for (auto& dcBlocker : m_outputDCBlockers) {
        dcBlocker.x1 = 0.0f;
        dcBlocker.y1 = 0.0f;
    }
    
    // Reset thermal model state
    m_thermalModel.thermalNoise = 0.0f;
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Reset parameter smoothers to current values (no reset to immediate as that would cause jumps)
    m_carrierFreq.current = m_carrierFreq.target;
    m_ringShiftBlend.current = m_ringShiftBlend.target;
    m_carrierDrift.current = m_carrierDrift.target;
    m_tracking.current = m_tracking.target;
    
    // Reset oversampler if needed
    if (m_useOversampling) {
        std::fill(m_oversampler.upsampleBuffer.begin(), m_oversampler.upsampleBuffer.end(), 0.0f);
        std::fill(m_oversampler.downsampleBuffer.begin(), m_oversampler.downsampleBuffer.end(), 0.0f);
        
        // Reset anti-aliasing filters
        m_oversampler.upsampleFilter.x.fill(0.0f);
        m_oversampler.upsampleFilter.y.fill(0.0f);
        m_oversampler.downsampleFilter.x.fill(0.0f);
        m_oversampler.downsampleFilter.y.fill(0.0f);
    }
}

void AnalogRingModulator::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_carrierFreq.update();
    m_ringShiftBlend.update();
    m_carrierDrift.update();
    m_tracking.update();
    
    // Update thermal model periodically
    m_sampleCount += numSamples;
    if (m_sampleCount >= static_cast<int>(m_sampleRate * 0.1)) { // Every 100ms
        m_thermalModel.update(m_sampleRate);
        m_componentAge += 0.0001f; // Slow aging
        m_sampleCount = 0;
    }
    
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update carrier drift with thermal effects
    m_carrier.driftAmount = m_carrierDrift.current * thermalFactor;
    m_carrier.frequency = m_carrierFreq.current;
    
    // Process each channel
    for (int ch = 0; ch < numChannels && ch < 2; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        auto& state = m_channels[ch];
        
        // Apply input DC blocking
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = m_inputDCBlockers[ch].process(channelData[sample]);
        }
        
        // Update component aging for this channel
        state.componentDrift += (m_driftDist(m_rng) * 0.00001f) * m_componentAge;
        state.componentDrift = std::max(-0.01f, std::min(0.01f, state.componentDrift));
        state.thermalFactor = thermalFactor * (1.0f + state.componentDrift);
        
        // Process with oversampling if modulation is intense
        if (m_useOversampling && m_ringShiftBlend.current > 0.5f) {
            // Upsample
            m_oversampler.upsample(channelData, m_oversampler.upsampleBuffer.data(), numSamples);
            
            // Process at higher sample rate
            for (int sample = 0; sample < numSamples * 2; ++sample) {
                float input = m_oversampler.upsampleBuffer[sample];
                
                // Generate carrier signal
                float carrier = m_carrier.tick(m_sampleRate * 2.0);
                
                float output = processRingModulation(input, carrier, state);
                m_oversampler.downsampleBuffer[sample] = output;
            }
            
            // Downsample
            m_oversampler.downsample(m_oversampler.downsampleBuffer.data(), channelData, numSamples);
        } else {
            // Standard processing without oversampling
            for (int sample = 0; sample < numSamples; ++sample) {
                float input = channelData[sample];
                
                // Generate carrier signal
                float carrier = m_carrier.tick(m_sampleRate);
                
                channelData[sample] = processRingModulation(input, carrier, state);
            }
        }
        
        // Apply output DC blocking
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = m_outputDCBlockers[ch].process(channelData[sample]);
        }
    }
}

float AnalogRingModulator::processRingModulation(float input, float carrier, ChannelState& state) {
    // Store input for pitch tracking
    state.autocorrBuffer[state.autocorrPos] = input;
    state.autocorrPos = (state.autocorrPos + 1) % 1024;
    
    // Track pitch periodically (every 512 samples)
    if (state.autocorrPos == 0 && m_tracking.current > 0.0f) {
        float detectedFreq = detectPitch(state.autocorrBuffer, 1024, m_sampleRate);
        if (detectedFreq > 0.0f) {
            state.trackedFrequency = state.trackedFrequency * 0.9f + detectedFreq * 0.1f;
        }
    }
    
    // Apply tracking to carrier frequency
    float effectiveCarrierFreq = m_carrierFreq.current;
    if (m_tracking.current > 0.0f) {
        effectiveCarrierFreq = m_carrierFreq.current * (1.0f - m_tracking.current) + 
                             state.trackedFrequency * m_tracking.current;
    }
    
    // Ring modulation
    float ringMod = input * carrier;
    
    // Frequency shifting (single sideband modulation)
    float freqShift = 0.0f;
    if (m_ringShiftBlend.current > 0.0f) {
        // Get analytic signal via Hilbert transform
        std::complex<float> analytic = state.hilbert.processAnalytic(input);
        
        // Complex multiplication for frequency shifting
        float cosCarrier = cos(state.quadraturePhase);
        float sinCarrier = sin(state.quadraturePhase);
        
        // Upper sideband (positive frequency shift)
        freqShift = analytic.real() * cosCarrier - analytic.imag() * sinCarrier;
        
        // Update quadrature phase with thermal effects
        state.quadraturePhase += 2.0f * M_PI * effectiveCarrierFreq * state.thermalFactor / m_sampleRate;
        while (state.quadraturePhase > 2.0f * M_PI) {
            state.quadraturePhase -= 2.0f * M_PI;
        }
    }
    
    // Blend ring mod and frequency shift
    float output = ringMod * (1.0f - m_ringShiftBlend.current) + freqShift * m_ringShiftBlend.current;
    
    // Apply soft clipping for analog warmth with aging
    output = softClipWithAging(output, m_componentAge);
    
    // Mix with dry signal (50/50)
    return input * 0.5f + output * 0.5f;
}

void AnalogRingModulator::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Carrier frequency: 0.1Hz to 5kHz (exponential)
    float freqParam = getParam(0, 0.5f);
    m_carrierFreq.target = 0.1f * std::pow(50000.0f, freqParam);
    
    // Ring/Shift blend
    m_ringShiftBlend.target = getParam(1, 0.0f);
    
    // Carrier drift (analog instability)
    m_carrierDrift.target = getParam(2, 0.0f);
    
    // Tracking (input frequency following)
    m_tracking.target = getParam(3, 0.0f);
}

juce::String AnalogRingModulator::getParameterName(int index) const {
    switch (index) {
        case 0: return "Carrier Freq";
        case 1: return "Ring/Shift";
        case 2: return "Drift";
        case 3: return "Tracking";
        default: return "";
    }
}

float AnalogRingModulator::softClipWithAging(float input, float aging) {
    // Apply aging effects - increased saturation and slight asymmetry
    float agingFactor = 1.0f + aging * 0.3f;
    float asymmetry = aging * 0.15f;
    
    // Asymmetric soft clipping
    if (input > 0.0f) {
        return std::tanh(input * 0.7f * agingFactor) * 1.4f;
    } else {
        return std::tanh(input * 0.7f * agingFactor * (1.0f + asymmetry)) * 1.4f;
    }
}

void AnalogRingModulator::updateCarrierWithThermal(float thermalFactor) {
    // Update carrier oscillator with thermal effects
    m_carrier.frequency = m_carrierFreq.current * thermalFactor;
    m_carrier.driftAmount = m_carrierDrift.current * thermalFactor;
}

float AnalogRingModulator::CarrierOscillator::tick(double sampleRate) {
    // Add drift
    driftPhase += 0.01f / sampleRate;
    float drift = sin(driftPhase * 2.0f * M_PI) * driftAmount * 0.05f;
    
    // Calculate frequency with drift
    float currentFreq = frequency * (1.0f + drift);
    
    // Generate sine wave
    float output = sin(phase);
    
    // Update phase
    phase += 2.0f * M_PI * currentFreq / sampleRate;
    while (phase > 2.0f * M_PI) {
        phase -= 2.0f * M_PI;
    }
    
    return output;
}

void AnalogRingModulator::CarrierOscillator::reset() {
    phase = 0.0f;
    driftPhase = 0.0f;
}

void AnalogRingModulator::HilbertTransform::init() {
    delayLine.resize(FILTER_LENGTH);
    coefficients.resize(FILTER_LENGTH);
    
    // Generate Hilbert transform coefficients
    int center = FILTER_LENGTH / 2;
    for (int i = 0; i < FILTER_LENGTH; ++i) {
        if (i == center) {
            coefficients[i] = 0.0f;
        } else {
            int n = i - center;
            if (n % 2 == 0) {
                coefficients[i] = 0.0f;
            } else {
                coefficients[i] = 2.0f / (M_PI * n);
            }
        }
        
        // Apply Blackman window
        float window = 0.42f - 0.5f * cos(2.0f * M_PI * i / (FILTER_LENGTH - 1)) +
                      0.08f * cos(4.0f * M_PI * i / (FILTER_LENGTH - 1));
        coefficients[i] *= window;
    }
}

float AnalogRingModulator::HilbertTransform::process(float input) {
    // Write to delay line
    delayLine[writePos] = input;
    
    // Convolve
    float output = 0.0f;
    for (int i = 0; i < FILTER_LENGTH; ++i) {
        int idx = (writePos - i + FILTER_LENGTH) % FILTER_LENGTH;
        output += delayLine[idx] * coefficients[i];
    }
    
    writePos = (writePos + 1) % FILTER_LENGTH;
    return output;
}

std::complex<float> AnalogRingModulator::HilbertTransform::processAnalytic(float input) {
    float imaginary = process(input);
    
    // Real part is delayed input (group delay compensation)
    int delayIdx = (writePos - FILTER_LENGTH/2 + FILTER_LENGTH) % FILTER_LENGTH;
    float real = delayLine[delayIdx];
    
    return std::complex<float>(real, imaginary);
}

void AnalogRingModulator::ChannelState::init() {
    hilbert.init();
    std::fill(std::begin(autocorrBuffer), std::end(autocorrBuffer), 0.0f);
}

void AnalogRingModulator::ChannelState::reset() {
    std::fill(hilbert.delayLine.begin(), hilbert.delayLine.end(), 0.0f);
    hilbert.writePos = 0;
    quadraturePhase = 0.0f;
    autocorrPos = 0;
    trackedFrequency = 440.0f;
}

float AnalogRingModulator::detectPitch(const float* buffer, int size, double sampleRate) {
    // Simple autocorrelation-based pitch detection
    float maxCorr = 0.0f;
    int maxLag = 0;
    
    // Search for fundamental frequency between 50Hz and 2000Hz
    int minLag = static_cast<int>(sampleRate / 2000.0);
    int maxLagSearch = static_cast<int>(sampleRate / 50.0);
    maxLagSearch = std::min(maxLagSearch, size / 2);
    
    for (int lag = minLag; lag < maxLagSearch; ++lag) {
        float corr = 0.0f;
        for (int i = 0; i < size - lag; ++i) {
            corr += buffer[i] * buffer[i + lag];
        }
        
        if (corr > maxCorr) {
            maxCorr = corr;
            maxLag = lag;
        }
    }
    
    if (maxLag > 0 && maxCorr > 0.1f) {
        return static_cast<float>(sampleRate) / maxLag;
    }
    
    return -1.0f; // No pitch detected
}
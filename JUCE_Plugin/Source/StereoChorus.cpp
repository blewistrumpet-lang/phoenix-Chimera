#include "StereoChorus.h"
#include <algorithm>

StereoChorus::StereoChorus() : m_rng(std::random_device{}()) {
    m_channels.resize(2);
    m_lfos.resize(2);
    
    // Initialize smooth parameters
    m_rate.setImmediate(1.0f);
    m_depth.setImmediate(5.0f);
    m_voices.setImmediate(2.0f);
    m_feedback.setImmediate(0.0f);
    m_width.setImmediate(1.0f);
    
    // Set smoothing rates
    m_rate.setSmoothingRate(0.995f);
    m_depth.setSmoothingRate(0.990f);
    m_voices.setSmoothingRate(0.999f); // Very slow for voice changes
    m_feedback.setSmoothingRate(0.995f);
    m_width.setSmoothingRate(0.995f);
}

void StereoChorus::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize delay lines and LFOs
    int maxDelaySamples = static_cast<int>(MAX_DELAY_MS * 0.001 * sampleRate);
    
    for (int ch = 0; ch < 2; ++ch) {
        m_channels[ch].init(MAX_VOICES, maxDelaySamples);
        m_channels[ch].reset();
        
        m_lfos[ch].resize(MAX_VOICES);
        for (int v = 0; v < MAX_VOICES; ++v) {
            m_lfos[ch][v].phase = m_phaseDist(m_rng);
            m_lfos[ch][v].phaseOffset = v * (1.0f / MAX_VOICES);
        }

void StereoChorus::reset() {
    // Reset modulation state
    m_lfoPhase = 0.0f;
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
}

    }
    
    // Initialize DC blockers
    m_inputDCBlockers.resize(2);
    m_outputDCBlockers.resize(2);
    
    // Prepare oversampler
    if (m_useOversampling) {
        m_oversampler.prepare(samplesPerBlock);
    }
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Reset thermal model
    m_thermalModel = ThermalModel();
}

void StereoChorus::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_rate.update();
    m_depth.update();
    m_voices.update();
    m_feedback.update();
    m_width.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update component aging (very slow)
    m_sampleCount += numSamples;
    if (m_sampleCount > m_sampleRate * 6) { // Every 6 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00008f);
        m_sampleCount = 0;
        
        // Update channel aging
        for (auto& state : m_channels) {
            state.updateAging(m_componentAge);
        }
    }
    
    // Calculate active voices (rounded)
    int activeVoices = static_cast<int>(m_voices.current + 0.5f);
    activeVoices = std::max(1, std::min(activeVoices, MAX_VOICES));
    
    // Base delay time in samples with thermal variation
    float baseDelaySamples = (10.0f * 0.001f * m_sampleRate) * thermalFactor; // 10ms base delay
    float depthSamples = (m_depth.current * 0.001f * m_sampleRate) * (1.0f - m_componentAge * 0.1f);
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        auto& state = m_channels[channel];
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // DC block input
            input = m_inputDCBlockers[channel].process(input);
            
            float output = 0.0f;
            
            // Process each voice with enhanced modeling
            for (int voice = 0; voice < activeVoices; ++voice) {
                output += processVoiceWithModeling(input, voice, channel, thermalFactor, m_componentAge);
            }
            
            // Normalize by active voices
            output /= activeVoices;
            
            // Add subtle noise from aging components
            if (m_componentAge > 0.01f) {
                output += state.noiseLevel * ((rand() % 1000) / 1000.0f - 0.5f) * 2.0f;
            }
            
            // DC block output
            output = m_outputDCBlockers[channel].process(output);
            
            // Analog-style saturation with aging
            if (std::abs(output) > 0.7f) {
                float saturation = 1.0f + m_componentAge * 0.1f;
                output = std::tanh(output * saturation) / saturation;
            }
            
            // Mix with dry signal (50/50 mix, modulated by aging)
            float wetAmount = 0.5f * (1.0f - m_componentAge * 0.05f);
            channelData[sample] = drySignal * (1.0f - wetAmount) + output * wetAmount;
        }
    }
}

void StereoChorus::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Rate: 0.1 to 10 Hz (exponential)
    float rateParam = getParam(0, 0.2f);
    m_rate.target = 0.1f * std::pow(100.0f, rateParam);
    
    // Depth: 0 to 20ms
    m_depth.target = getParam(1, 0.25f) * 20.0f;
    
    // Voices: 1 to 4
    m_voices.target = 1.0f + getParam(2, 0.25f) * 3.0f;
    
    // Feedback: -50% to +50%
    m_feedback.target = (getParam(3, 0.5f) - 0.5f) * 2.0f * 0.5f;
    
    // Width: 0% to 100%
    m_width.target = getParam(4, 1.0f);
}

juce::String StereoChorus::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Voices";
        case 3: return "Feedback";
        case 4: return "Width";
        default: return "";
    }
}

// DelayLine implementation
void StereoChorus::DelayLine::resize(int newSize) {
    size = newSize;
    buffer.resize(size);
    clear();
}

void StereoChorus::DelayLine::clear() {
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
}

void StereoChorus::DelayLine::write(float sample) {
    buffer[writePos] = sample;
    writePos = (writePos + 1) % size;
}

float StereoChorus::DelayLine::readInterpolated(float delaySamples) {
    float readPosFloat = writePos - delaySamples;
    while (readPosFloat < 0) {
        readPosFloat += size;
    }
    
    int readPos1 = static_cast<int>(readPosFloat);
    int readPos2 = (readPos1 + 1) % size;
    float frac = readPosFloat - readPos1;
    
    return buffer[readPos1] * (1.0f - frac) + buffer[readPos2] * frac;
}

float StereoChorus::DelayLine::readInterpolatedWithAging(float delaySamples, float aging) {
    float interpolated = readInterpolated(delaySamples);
    
    // Aging affects delay line precision and adds slight modulation
    if (aging > 0.01f) {
        // Add jitter from aging components
        float jitter = aging * 0.05f * ((rand() % 1000) / 1000.0f - 0.5f);
        float jitteredDelay = delaySamples * (1.0f + jitter);
        
        // Blend between normal and jittered read
        float jittered = readInterpolated(jitteredDelay);
        interpolated = interpolated * (1.0f - aging * 0.3f) + jittered * (aging * 0.3f);
        
        // Add slight high frequency roll-off due to aging
        static float hfState = 0.0f;
        float cutoff = 0.1f * (1.0f - aging * 0.4f);
        hfState += (interpolated - hfState) * cutoff;
        interpolated = interpolated * 0.7f + hfState * 0.3f;
    }
    
    return interpolated;
}

// ChannelState implementation
void StereoChorus::ChannelState::init(int numVoices, int maxDelaySamples) {
    voiceDelays.resize(numVoices);
    voicePhases.resize(numVoices);
    
    for (auto& delay : voiceDelays) {
        delay.resize(maxDelaySamples);
    }
}

void StereoChorus::ChannelState::reset() {
    for (auto& delay : voiceDelays) {
        delay.clear();
    }
    feedbackSample = 0.0f;
    componentDrift = 0.0f;
    thermalNoise = 0.0f;
    noiseLevel = 0.0f;
}

void StereoChorus::ChannelState::updateAging(float aging) {
    componentDrift = aging * 0.015f; // 1.5% max drift
    thermalNoise = aging * 0.002f;   // Thermal fluctuations
    noiseLevel = aging * 0.0008f;    // Very subtle noise floor
}

// LFO implementation
float StereoChorus::LFO::tick(float rate, double sampleRate) {
    // Triangle wave LFO
    phase += rate / sampleRate;
    while (phase >= 1.0f) {
        phase -= 1.0f;
    }
    
    float adjustedPhase = phase + phaseOffset;
    while (adjustedPhase >= 1.0f) {
        adjustedPhase -= 1.0f;
    }
    
    // Convert to triangle wave (0 to 1)
    if (adjustedPhase < 0.5f) {
        return adjustedPhase * 2.0f;
    } else {
        return 2.0f - adjustedPhase * 2.0f;
    }
}

float StereoChorus::LFO::tickWithThermal(float rate, double sampleRate, float thermalFactor) {
    float baseLFO = tick(rate, sampleRate);
    
    // Add thermal instability
    float thermalModulation = (thermalFactor - 1.0f) * 3.0f; // Amplify thermal effect
    baseLFO += thermalModulation * std::sin(phase * 7.3f) * 0.05f; // Subtle thermal wobble
    
    return std::max(0.0f, std::min(1.0f, baseLFO)); // Clamp to valid range
}

float StereoChorus::processVoiceWithModeling(float input, int voice, int channel, float thermalFactor, float aging) {
    auto& delay = m_channels[channel].voiceDelays[voice];
    auto& lfo = m_lfos[channel][voice];
    auto& state = m_channels[channel];
    
    // Calculate modulated delay time with thermal and aging effects
    float lfoValue = lfo.tickWithThermal(m_rate.current, m_sampleRate, thermalFactor);
    
    // Apply stereo width modulation with thermal variation
    if (channel == 1) {
        float thermalWidth = m_width.current * thermalFactor;
        lfoValue = lfoValue * thermalWidth + (1.0f - thermalWidth) * 0.5f;
    }
    
    // Add component drift to delay time
    float baseDelaySamples = (10.0f * 0.001f * m_sampleRate) * (1.0f + state.componentDrift);
    float depthSamples = (m_depth.current * 0.001f * m_sampleRate) * (1.0f - aging * 0.1f);
    float delaySamples = baseDelaySamples + depthSamples * lfoValue;
    
    // Read from delay line with aging effects
    float delayed = delay.readInterpolatedWithAging(delaySamples, aging);
    
    // Add thermal noise to delayed signal
    delayed += state.thermalNoise * ((rand() % 1000) / 1000.0f - 0.5f) * 0.5f;
    
    // Write to delay line with feedback and aging effects
    float feedbackAmount = m_feedback.current * (1.0f + aging * 0.2f); // Aging increases feedback slightly
    float toWrite = input + delayed * feedbackAmount;
    
    // Soft limiting on feedback to prevent runaway
    if (std::abs(toWrite) > 0.95f) {
        toWrite = std::tanh(toWrite * 0.9f) * 1.05f;
    }
    
    delay.write(toWrite);
    
    return delayed;
}
#include "CombResonator.h"
#include <algorithm>
#include <cstring>

// Denormal handling is done per-sample using flushDenorm from Denorm.hpp

//==============================================================================
// ProfessionalCombFilter Implementation
//==============================================================================
CombResonator::ProfessionalCombFilter::ProfessionalCombFilter() {
    delayLine.fill(0.0f);
}

void CombResonator::ProfessionalCombFilter::init(int maxDelay) {
    // Already allocated with MAX_DELAY_SAMPLES
    reset();
}

void CombResonator::ProfessionalCombFilter::setDelay(float samples) {
    delayTime = std::clamp(samples, 1.0f, static_cast<float>(MAX_DELAY_SAMPLES - 4));
}

float CombResonator::ProfessionalCombFilter::process(float input) noexcept {
    if (delayTime < 1.0f) return input;
    
    // Calculate integer and fractional parts
    int delaySamples = static_cast<int>(delayTime);
    float fraction = delayTime - delaySamples;
    
    // Read positions for interpolation
    int readPos = writePos - delaySamples;
    if (readPos < 0) readPos += MAX_DELAY_SAMPLES;
    
    // Get samples for interpolation
    int pos0 = (readPos - 1 + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
    int pos1 = readPos;
    int pos2 = (readPos + 1) % MAX_DELAY_SAMPLES;
    int pos3 = (readPos + 2) % MAX_DELAY_SAMPLES;
    
    float y0 = delayLine[pos0];
    float y1 = delayLine[pos1];
    float y2 = delayLine[pos2];
    float y3 = delayLine[pos3];
    
    // Hermite interpolation for smooth fractional delays
    float delayed = interpolate(fraction, y0, y1, y2, y3);
    
    // Apply damping filter (one-pole lowpass)
    dampingState = delayed * (1.0f - damping) + dampingState * damping;
    dampingState = flushDenorm(dampingState);
    
    // Comb filter with feedforward and feedback
    float output = input * feedforward + dampingState * feedback;
    
    // Prevent denormals in delay line
    delayLine[writePos] = flushDenorm(output);
    
    // Update write position
    writePos = (writePos + 1) % MAX_DELAY_SAMPLES;
    
    return output;
}

void CombResonator::ProfessionalCombFilter::reset() {
    delayLine.fill(0.0f);
    dampingState = 0.0f;
    writePos = 0;
}

//==============================================================================
// ChannelState Implementation
//==============================================================================
void CombResonator::ChannelState::init() {
    for (auto& comb : combs) {
        comb.init(MAX_DELAY_SAMPLES);
    }
    
    // Initialize harmonic gains with natural rolloff
    for (int i = 0; i < NUM_COMBS; ++i) {
        harmonicGains[i] = 1.0f / std::sqrt(static_cast<float>(i + 1));
    }
}

void CombResonator::ChannelState::reset() {
    for (auto& comb : combs) {
        comb.reset();
    }
    inputDC.reset();
    outputDC.reset();
    lfoPhase = 0.0f;
    chorusPhase = 0.0f;
    clipState = 0.0f;
}

//==============================================================================
// CombResonator Implementation
//==============================================================================
CombResonator::CombResonator() {
    // Initialize parameters with musical defaults
    m_rootFrequency.setImmediate(220.0f);   // A3
    m_resonance.setImmediate(0.85f);
    m_harmonicSpread.setImmediate(1.0f);
    m_decayTime.setImmediate(2.0f);
    m_damping.setImmediate(0.3f);
    m_modDepth.setImmediate(0.0f);
    m_stereoWidth.setImmediate(0.5f);
    m_mix.setImmediate(0.5f);
    
    // Initialize with 2 channels
    m_channels.resize(2);
    for (auto& channel : m_channels) {
        channel.init();
    }
}

void CombResonator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing rates
    const float smoothTime = 0.02f; // 20ms
    float rate = 1.0f - std::exp(-1.0f / (smoothTime * sampleRate));
    
    m_rootFrequency.setRate(rate);
    m_resonance.setRate(rate);
    m_harmonicSpread.setRate(rate * 0.5f); // Slower for harmonic changes
    m_decayTime.setRate(rate * 0.3f);      // Even slower for decay
    m_damping.setRate(rate);
    m_modDepth.setRate(rate);
    m_stereoWidth.setRate(rate);
    m_mix.setRate(rate);
    
    // Initialize channels
    for (int ch = 0; ch < m_channels.size(); ++ch) {
        m_channels[ch].reset();
        // Offset phases for stereo
        m_channels[ch].lfoPhase = ch * M_PI;
        m_channels[ch].chorusPhase = ch * M_PI * 0.5f;
    }
}

void CombResonator::reset() {
    for (auto& channel : m_channels) {
        channel.reset();
    }
    
    // Reset parameters to current values
    m_rootFrequency.setImmediate(m_rootFrequency.current);
    m_resonance.setImmediate(m_resonance.current);
    m_harmonicSpread.setImmediate(m_harmonicSpread.current);
    m_decayTime.setImmediate(m_decayTime.current);
    m_damping.setImmediate(m_damping.current);
    m_modDepth.setImmediate(m_modDepth.current);
    m_stereoWidth.setImmediate(m_stereoWidth.current);
    m_mix.setImmediate(m_mix.current);
}

void CombResonator::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        auto& state = m_channels[ch % m_channels.size()];
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smooth parameters
            float rootFreq = m_rootFrequency.tick();
            float resonance = m_resonance.tick();
            float harmonicSpread = m_harmonicSpread.tick();
            float decayTime = m_decayTime.tick();
            float damping = m_damping.tick();
            float modDepth = m_modDepth.tick();
            float stereoWidth = m_stereoWidth.tick();
            float mixAmount = m_mix.tick();
            
            // Get input and apply DC blocking
            float input = state.inputDC.process(channelData[sample]);
            
            // Generate modulation
            float lfoFreq = 0.5f; // 0.5 Hz base LFO
            state.lfoPhase += 2.0f * M_PI * lfoFreq / m_sampleRate;
            if (state.lfoPhase > 2.0f * M_PI) state.lfoPhase -= 2.0f * M_PI;
            
            float lfo = std::sin(state.lfoPhase) * modDepth * 0.1f;
            
            // Chorus for richness
            float chorusFreq = 0.7f;
            state.chorusPhase += 2.0f * M_PI * chorusFreq / m_sampleRate;
            if (state.chorusPhase > 2.0f * M_PI) state.chorusPhase -= 2.0f * M_PI;
            
            float chorus = std::sin(state.chorusPhase) * modDepth * 0.02f;
            
            // Process through comb filters
            float output = 0.0f;
            
            for (int i = 0; i < NUM_COMBS; ++i) {
                // Calculate frequency for this harmonic
                float harmonic = BASE_HARMONIC_RATIOS[i];
                
                // Apply harmonic spread
                if (i > 0) {
                    float spreadFactor = std::pow(harmonicSpread, static_cast<float>(i));
                    harmonic = 1.0f + (harmonic - 1.0f) * spreadFactor;
                }
                
                // Apply modulation and stereo detuning
                float stereoDetune = (ch == 0 ? -1.0f : 1.0f) * stereoWidth * 0.01f * i;
                float modulation = 1.0f + lfo + chorus * (i + 1) * 0.1f;
                
                float freq = rootFreq * harmonic * modulation * (1.0f + stereoDetune);
                freq = std::clamp(freq, MIN_FREQ, MAX_FREQ);
                
                // Set comb filter parameters
                float delaySamples = frequencyToDelay(freq, m_sampleRate);
                state.combs[i].setDelay(delaySamples);
                
                // Calculate feedback from decay time
                float feedback = decayTimeToFeedback(decayTime, delaySamples, m_sampleRate);
                state.combs[i].setFeedback(feedback * resonance);
                state.combs[i].setDamping(damping);
                
                // Process and accumulate
                float combOut = state.combs[i].process(input);
                output += combOut * state.harmonicGains[i];
            }
            
            // Normalize and apply soft saturation
            output = output / (NUM_COMBS * 0.7f);
            output = softSaturate(output, state.clipState);
            
            // DC block output
            output = state.outputDC.process(output);
            
            // Final soft limiting
            output = std::tanh(output * 0.8f) * 1.25f;
            
            // Mix with dry signal
            channelData[sample] = flushDenorm(
                channelData[sample] * (1.0f - mixAmount) + output * mixAmount
            );
        }
    }
}

float CombResonator::decayTimeToFeedback(float decaySeconds, float delaySamples, double sampleRate) noexcept {
    if (decaySeconds <= 0.0f || delaySamples <= 0.0f) {
        return 0.0f;
    }
    
    // RT60 calculation: feedback = 10^(-3 * delay_time / decay_time)
    float delayTime = delaySamples / sampleRate;
    float exponent = -3.0f * delayTime / decaySeconds;
    float feedback = std::pow(10.0f, exponent);
    
    // Ensure stability
    return std::clamp(feedback, 0.0f, 0.999f);
}

void CombResonator::updateParameters(const std::map<int, float>& params) {
    auto updateParam = [&params](int index, SmoothParam& param,
                                float min, float max, bool exponential = false) {
        auto it = params.find(index);
        if (it != params.end()) {
            float normalized = it->second;
            float value;
            
            if (exponential) {
                // Exponential mapping for frequency/time parameters
                value = min * std::pow(max / min, normalized);
            } else {
                // Linear mapping
                value = min + normalized * (max - min);
            }
            
            param.target.store(value, std::memory_order_relaxed);
        }
    };
    
    // Update all parameters
    updateParam(0, m_rootFrequency, 20.0f, 2000.0f, true);    // Root Freq (exponential)
    updateParam(1, m_resonance, 0.0f, 0.99f, false);          // Resonance
    updateParam(2, m_harmonicSpread, 0.5f, 2.0f, false);      // Harmonic Spread
    updateParam(3, m_decayTime, 0.1f, 10.0f, true);           // Decay Time (exponential)
    updateParam(4, m_damping, 0.0f, 0.9f, false);             // Damping
    updateParam(5, m_modDepth, 0.0f, 1.0f, false);            // Mod Depth
    updateParam(6, m_stereoWidth, 0.0f, 1.0f, false);         // Stereo Width
    updateParam(7, m_mix, 0.0f, 1.0f, false);                 // Mix
}

juce::String CombResonator::getParameterName(int index) const {
    switch (index) {
        case 0: return "Root Freq";
        case 1: return "Resonance";
        case 2: return "Harmonic Spread";
        case 3: return "Decay Time";
        case 4: return "Damping";
        case 5: return "Mod Depth";
        case 6: return "Stereo Width";
        case 7: return "Mix";
        default: return "";
    }
}
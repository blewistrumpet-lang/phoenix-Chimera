#include "DynamicEQ.h"
#include <cmath>
#include <algorithm>

DynamicEQ::DynamicEQ() {
    // Initialize smoothed parameters with low-THD defaults
    m_frequency.reset(0.5f);    // 1kHz default
    m_threshold.reset(0.5f);    // -30dB default
    m_ratio.reset(0.0f);        // 1:1 default (no compression, lowest THD)
    m_attack.reset(0.2f);       // 5ms default
    m_release.reset(0.4f);      // 100ms default
    m_gain.reset(0.5f);         // 0dB default
    m_mix.reset(1.0f);          // 100% wet default
    m_mode.reset(0.0f);         // Compressor mode default
}

void DynamicEQ::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times - MUCH faster for responsive control
    m_frequency.setSmoothingTime(0.5f, sampleRate);    // Was 50ms, now 0.5ms
    m_threshold.setSmoothingTime(1.0f, sampleRate);    // Was 100ms, now 1ms
    m_ratio.setSmoothingTime(2.0f, sampleRate);        // Was 200ms, now 2ms
    m_attack.setSmoothingTime(1.5f, sampleRate);       // Was 150ms, now 1.5ms
    m_release.setSmoothingTime(2.5f, sampleRate);      // Was 250ms, now 2.5ms
    m_gain.setSmoothingTime(0.5f, sampleRate);         // Was 50ms, now 0.5ms
    m_mix.setSmoothingTime(0.3f, sampleRate);          // Was 30ms, now 0.3ms
    m_mode.setSmoothingTime(5.0f, sampleRate);         // Was 500ms, now 5ms (mode changes can be slightly slower)
    
    // Initialize DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.reset();
    }
    
    // Initialize channel states
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
    }
}

void DynamicEQ::reset() {
    // Reset all internal state
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
    for (auto& blocker : m_dcBlockers) {
        blocker.reset();
    }
}

void DynamicEQ::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Early bypass check for mix parameter
    m_mix.update();
    if (m_mix.current < 0.001f) {
        // Completely dry - no processing needed, just update parameters for smooth operation
        m_frequency.update();
        m_threshold.update();
        m_ratio.update();
        m_attack.update();
        m_release.update();
        m_gain.update();
        return;
    }
    
    // Thermal modeling and component aging disabled for low THD
    // m_thermalModel.update(m_sampleRate);
    // updateComponentAging(m_sampleRate);
    
    // Update smoothed parameters once per block (more efficient)
    for (int i = 0; i < numSamples; ++i) {
        m_frequency.update();
        m_threshold.update();
        m_ratio.update();
        m_attack.update();
        m_release.update();
        m_gain.update();
        m_mode.update();
    }
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float drySignal = input;
            
            // Apply DC blocking
            input = m_dcBlockers[channel].process(input);

            // Calculate frequency from parameter (20Hz to 20kHz)
            // Use exponential scaling with safer range
            float freqParam = std::min(0.95f, m_frequency.current); // Cap at 0.95 to prevent extreme values
            float freq = 20.0f * std::pow(200.0f, freqParam); // Further reduced to 200.0f for more stable scaling

            // Clamp frequency to safe range (avoid Nyquist issues)
            freq = std::max(20.0f, std::min(freq, static_cast<float>(m_sampleRate * 0.45f)));

            // Thermal compensation disabled for low THD
            // float thermalFactor = m_thermalModel.getThermalFactor();
            // freq *= thermalFactor;

            // Ensure frequency stays in safe range
            freq = std::max(20.0f, std::min(freq, static_cast<float>(m_sampleRate * 0.48f)));
            
            // Set up filter parameters with lower Q value to reduce THD
            // Reduced from 2.0 to 0.707 (Butterworth response) for minimal distortion
            float Q = 0.707f; // Butterworth Q for flattest passband and lowest THD

            // Update filter every sample for smooth parameter changes and low THD
            // Removed threshold check to eliminate parameter discontinuities
            state.peakFilter.setParameters(freq, Q, m_sampleRate);
            
            // Oversampling disabled for low THD - process at native sample rate
            // Get filter outputs
            auto filterOutputs = state.peakFilter.process(input);

            // Set up dynamic processor timing
            float attackMs = 0.1f + m_attack.current * 99.9f; // 0.1ms to 100ms
            float releaseMs = 10.0f + m_release.current * 4990.0f; // 10ms to 5000ms
            state.dynamicProcessor.setTiming(attackMs, releaseMs, m_sampleRate);

            // Calculate threshold in dB
            float thresholdDb = -60.0f + m_threshold.current * 60.0f; // -60dB to 0dB

            // Calculate ratio
            float ratio = 0.1f + m_ratio.current * 9.9f; // 0.1:1 to 10:1

            // Determine mode
            int mode = static_cast<int>(m_mode.current * 2.99f); // 0, 1, or 2

            // Rebuild gain curve if parameters changed significantly
            // Reduced thresholds for more frequent updates and smoother THD
            static float lastThreshold[2] = {-1000.0f, -1000.0f};
            static float lastRatio[2] = {-1.0f, -1.0f};
            static int lastMode[2] = {-1, -1};

            if (std::abs(thresholdDb - lastThreshold[channel]) > 0.1f ||  // Reduced from 0.5 to 0.1
                std::abs(ratio - lastRatio[channel]) > 0.01f ||          // Reduced from 0.05 to 0.01
                mode != lastMode[channel]) {
                state.dynamicProcessor.buildGainCurve(thresholdDb, ratio, mode);
                lastThreshold[channel] = thresholdDb;
                lastRatio[channel] = ratio;
                lastMode[channel] = mode;
            }

            // Process the peak band through dynamic processor
            float processedPeak = state.dynamicProcessor.process(filterOutputs.peak, thresholdDb, ratio, mode);

            // Apply static gain
            float gainDb = -20.0f + m_gain.current * 40.0f; // -20dB to +20dB
            float gainLinear = dbToLinear(gainDb);
            processedPeak *= gainLinear;

            // Analog saturation disabled for low THD
            // processedPeak = applyAnalogSaturation(processedPeak);

            // Reconstruct: add the processed peak band back to the input
            // With biquad, filterOutputs.peak is already the isolated band
            float output = input + processedPeak;

            // Final analog saturation disabled for low THD
            // output = applyAnalogSaturation(output * 0.7f);

            // Mix with dry signal
            channelData[sample] = drySignal * (1.0f - m_mix.current) + output * m_mix.current;
        }
    }
    
    // Apply final NaN/Inf cleanup
    scrubBuffer(buffer);
}

float DynamicEQ::applyAnalogSaturation(float input) {
    // Vintage EQ-style saturation with component aging
    float agingFactor = 1.0f + m_componentAge * 0.02f;
    
    // Asymmetric saturation (more compression on positive peaks)
    if (input > 0.0f) {
        float driven = input * agingFactor;
        return std::tanh(driven * 0.8f) / 0.8f;
    } else {
        float driven = input * agingFactor * 0.9f; // Less saturation on negative
        return std::tanh(driven * 0.9f) / 0.9f;
    }
}

float DynamicEQ::applyComponentTolerance(float value, float tolerance) {
    // Simulate component tolerance (typically ±5% for audio components)
    thread_local juce::Random random;
    float randomFactor = (random.nextFloat() - 0.5f) * tolerance;
    return value * (1.0f + randomFactor);
}

void DynamicEQ::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_frequency.target = params.at(0);
    if (params.count(1)) m_threshold.target = params.at(1);
    if (params.count(2)) m_ratio.target = params.at(2);
    if (params.count(3)) m_attack.target = params.at(3);
    if (params.count(4)) m_release.target = params.at(4);
    if (params.count(5)) m_gain.target = params.at(5);
    if (params.count(6)) {
        float mixValue = params.at(6);
        if (mixValue < 0.001f) {
            // Immediate bypass - set both target and current for instant effect
            m_mix.target = mixValue;
            m_mix.current = mixValue;
        } else {
            // Normal smoothed transition
            m_mix.target = mixValue;
        }
    }
    if (params.count(7)) m_mode.target = params.at(7);
}

juce::String DynamicEQ::getParameterName(int index) const {
    switch (index) {
        case 0: return "Frequency";
        case 1: return "Threshold";
        case 2: return "Ratio";
        case 3: return "Attack";
        case 4: return "Release";
        case 5: return "Gain";
        case 6: return "Mix";
        case 7: return "Mode";
        default: return "";
    }
}
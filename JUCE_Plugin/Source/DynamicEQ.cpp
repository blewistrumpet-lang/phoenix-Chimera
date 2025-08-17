#include "DynamicEQ.h"
#include <cmath>
#include <algorithm>

DynamicEQ::DynamicEQ() {
    // Initialize smoothed parameters
    m_frequency.reset(0.5f);    // 1kHz default
    m_threshold.reset(0.5f);    // -30dB default
    m_ratio.reset(0.3f);        // 3:1 default
    m_attack.reset(0.2f);       // 5ms default
    m_release.reset(0.4f);      // 100ms default
    m_gain.reset(0.5f);         // 0dB default
    m_mix.reset(1.0f);          // 100% wet default
    m_mode.reset(0.0f);         // Compressor mode default
}

void DynamicEQ::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    m_frequency.setSmoothingTime(50.0f, sampleRate);
    m_threshold.setSmoothingTime(100.0f, sampleRate);
    m_ratio.setSmoothingTime(200.0f, sampleRate);
    m_attack.setSmoothingTime(150.0f, sampleRate);
    m_release.setSmoothingTime(250.0f, sampleRate);
    m_gain.setSmoothingTime(50.0f, sampleRate);
    m_mix.setSmoothingTime(30.0f, sampleRate);
    m_mode.setSmoothingTime(500.0f, sampleRate);
    
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
    
    // Update thermal modeling and component aging
    m_thermalModel.update(m_sampleRate);
    updateComponentAging(m_sampleRate);
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smoothed parameters
            m_frequency.update();
            m_threshold.update();
            m_ratio.update();
            m_attack.update();
            m_release.update();
            m_gain.update();
            m_mix.update();
            m_mode.update();
            
            float input = channelData[sample];
            float drySignal = input;
            
            // Apply DC blocking
            input = m_dcBlockers[channel].process(input);
            
            // Calculate frequency from parameter (20Hz to 20kHz)
            // Use exponential scaling but limit the range
            float freqParam = std::min(0.95f, m_frequency.current); // Cap at 0.95 to prevent extreme values
            float freq = 20.0f * std::pow(500.0f, freqParam); // Reduced from 1000.0f to 500.0f for safety
            
            // Clamp frequency to safe range (avoid Nyquist issues)
            freq = std::max(20.0f, std::min(freq, static_cast<float>(m_sampleRate * 0.45f)));
            
            // Apply thermal compensation to frequency
            float thermalFactor = m_thermalModel.getThermalFactor();
            freq *= thermalFactor;
            
            // Ensure frequency stays in safe range after thermal compensation
            freq = std::max(20.0f, std::min(freq, static_cast<float>(m_sampleRate * 0.48f)));
            
            // Set up filter parameters with safe Q value
            float Q = std::max(0.5f, std::min(10.0f, 2.0f)); // Bounded Q
            
            // Only update filter if frequency changed significantly to avoid clicks
            static float lastFreq = 0.0f;
            if (std::abs(freq - lastFreq) > 0.1f) {
                state.peakFilter.setParameters(freq, Q, m_sampleRate);
                lastFreq = freq;
            }
            
            // Process through oversampling for higher quality
            float oversampledInput[2];
            state.oversampler.upsample(input, oversampledInput);
            
            float processedOversampledOutput[2];
            for (int os = 0; os < 2; ++os) {
                // Get filter outputs
                auto filterOutputs = state.peakFilter.process(oversampledInput[os]);
                
                // Set up dynamic processor timing
                float attackMs = 0.1f + m_attack.current * 99.9f; // 0.1ms to 100ms
                float releaseMs = 10.0f + m_release.current * 4990.0f; // 10ms to 5000ms
                state.dynamicProcessor.setTiming(attackMs, releaseMs, m_sampleRate * 2); // Account for oversampling
                
                // Calculate threshold in dB
                float thresholdDb = -60.0f + m_threshold.current * 60.0f; // -60dB to 0dB
                
                // Calculate ratio
                float ratio = 0.1f + m_ratio.current * 9.9f; // 0.1:1 to 10:1
                
                // Determine mode
                int mode = static_cast<int>(m_mode.current * 2.99f); // 0, 1, or 2
                
                // Process the peak band through dynamic processor
                float processedPeak = state.dynamicProcessor.process(filterOutputs.peak, thresholdDb, ratio, mode);
                
                // Apply static gain
                float gainDb = -20.0f + m_gain.current * 40.0f; // -20dB to +20dB
                float gainLinear = dbToLinear(gainDb);
                processedPeak *= gainLinear;
                
                // Apply analog saturation to the processed peak
                processedPeak = applyAnalogSaturation(processedPeak);
                
                // Reconstruct the signal: original minus original peak plus processed peak
                processedOversampledOutput[os] = oversampledInput[os] - filterOutputs.peak + processedPeak;
            }
            
            // Downsample back to original rate
            float output = state.oversampler.downsample(processedOversampledOutput);
            
            // Final analog saturation for warmth
            output = applyAnalogSaturation(output * 0.5f) * 2.0f;
            
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
    if (params.count(6)) m_mix.target = params.at(6);
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
#include "RodentDistortion.h"
#include <cmath>

RodentDistortion::RodentDistortion() {
    // Initialize smoothed parameters with boutique defaults
    m_gain.setImmediate(10.0f);
    m_filter.setImmediate(2000.0f);
    m_clipping.setImmediate(0.5f);
    m_tone.setImmediate(5000.0f);
    m_output.setImmediate(0.5f);
    m_mix.setImmediate(1.0f);
    m_distortionType.setImmediate(0.0f);  // RAT style default
    m_presence.setImmediate(0.3f);
    
    // Set different smoothing rates for different parameters
    m_gain.setSmoothingRate(0.99f);       // Faster for gain changes
    m_filter.setSmoothingRate(0.995f);
    m_clipping.setSmoothingRate(0.99f);
    m_tone.setSmoothingRate(0.995f);
    m_output.setSmoothingRate(0.99f);
    m_mix.setSmoothingRate(0.995f);
    m_distortionType.setSmoothingRate(0.98f);  // Slower for mode changes
    m_presence.setSmoothingRate(0.995f);
}

void RodentDistortion::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Reset enhanced filter states
    for (auto& filter : m_inputFilters) {
        filter.reset();
        filter.updateCoefficients(2000.0f, 0.5f, sampleRate);
    }

void RodentDistortion::reset() {
    // Reset distortion state
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
}

    for (auto& filter : m_toneFilters) {
        filter.reset();
        filter.updateCoefficients(5000.0f, 0.3f, sampleRate);
    }
    
    // Reset DC blockers
    for (auto& blocker : m_inputDCBlockers) {
        blocker.reset();
    }
    for (auto& blocker : m_outputDCBlockers) {
        blocker.reset();
    }
    
    // Prepare oversampler
    m_oversampler.prepare(samplesPerBlock);
    
    // Reset aging and thermal
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void RodentDistortion::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    m_gain.update();
    m_filter.update(); 
    m_clipping.update();
    m_tone.update();
    m_output.update();
    m_mix.update();
    m_distortionType.update();
    m_presence.update();
    
    // Update thermal model
    m_thermalModel.update(m_sampleRate);
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Component aging (very slow)
    m_sampleCount++;
    if (m_sampleCount > m_sampleRate * 60) {  // Update every minute
        m_componentAge += 0.000001f;  // Very slow aging
        m_sampleCount = 0;
    }
    
    // Update filter coefficients with component tolerances and aging
    float adjustedFilterFreq = m_componentTolerances.adjustFrequency(m_filter.current) * (1.0f + m_componentAge);
    float adjustedToneFreq = m_componentTolerances.adjustFrequency(m_tone.current) * (1.0f + m_componentAge);
    
    for (int channel = 0; channel < std::min(numChannels, 2); ++channel) {
        auto& inputFilter = m_inputFilters[channel];
        auto& toneFilter = m_toneFilters[channel];
        auto& inputDCBlocker = m_inputDCBlockers[channel];
        auto& outputDCBlocker = m_outputDCBlockers[channel];
        
        inputFilter.updateCoefficients(adjustedFilterFreq, 0.5f, m_sampleRate);
        toneFilter.updateCoefficients(adjustedToneFreq, 0.3f, m_sampleRate);
        
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float dry = input;
            
            // Input DC blocking
            input = inputDCBlocker.process(input);
            
            // Input high-pass filter (removes low-end mud)
            float filtered = inputFilter.processHighpass(input);
            
            // Apply thermal-modulated gain
            float thermalGain = m_componentTolerances.adjustGain(m_gain.current) * thermalFactor;
            float gained = filtered * thermalGain;
            
            // Enhanced clipping with vintage modes
            int distortionMode = static_cast<int>(m_distortionType.current * 3.99f);  // 0-3
            float clipped = processVintageMode(gained, distortionMode, m_clipping.current);
            
            // Presence boost (high-frequency emphasis)
            if (m_presence.current > 0.01f) {
                float presenceGain = 1.0f + m_presence.current * 2.0f;
                // Simple high-shelf approximation
                static float presenceState = 0.0f;
                float highFreq = clipped - presenceState;
                presenceState += highFreq * 0.1f;
                clipped += highFreq * presenceGain * 0.3f;
            }
            
            // Tone filter (low-pass)
            float toned = toneFilter.processLowpass(clipped);
            
            // Output gain with component aging
            float output = toned * m_componentTolerances.adjustGain(m_output.current) * thermalFactor;
            
            // Output DC blocking
            output = outputDCBlocker.process(output);
            
            // Final soft limiting to prevent harsh clipping
            if (std::abs(output) > 0.95f) {
                output = std::tanh(output * 0.8f) * 1.1875f;
            }
            
            // Mix control
            channelData[sample] = output * m_mix.current + dry * (1.0f - m_mix.current);
        }
    }
}

void RodentDistortion::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Map normalized 0-1 values to actual parameter ranges with smoothing
    m_gain.target = 1.0f + getParam(0, 0.2f) * 49.0f;        // 1-50
    m_filter.target = 100.0f + getParam(1, 0.4f) * 4900.0f;  // 100-5000 Hz
    m_clipping.target = getParam(2, 0.5f);                    // 0-1
    m_tone.target = 1000.0f + getParam(3, 0.4f) * 9000.0f;   // 1000-10000 Hz
    m_output.target = getParam(4, 0.5f);                      // 0-1
    m_mix.target = getParam(5, 1.0f);                         // 0-1
    m_distortionType.target = getParam(6, 0.0f);              // 0-1 (vintage modes)
    m_presence.target = getParam(7, 0.3f);                    // 0-1 (high-freq emphasis)
}

juce::String RodentDistortion::getParameterName(int index) const {
    switch (index) {
        case 0: return "Gain";
        case 1: return "Filter";
        case 2: return "Clipping";
        case 3: return "Tone";
        case 4: return "Output";
        case 5: return "Mix";
        case 6: return "Mode";
        case 7: return "Presence";
        default: return "";
    }
}

// Oversampler implementation
void RodentDistortion::Oversampler::upsample(const float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Insert zeros between samples
        for (int j = 0; j < OVERSAMPLE_FACTOR; ++j) {
            output[i * OVERSAMPLE_FACTOR + j] = (j == 0) ? input[i] * OVERSAMPLE_FACTOR : 0.0f;
        }
    }
    
    // Apply anti-aliasing filter
    for (int i = 0; i < numSamples * OVERSAMPLE_FACTOR; ++i) {
        output[i] = upsampleFilter.process(output[i]);
    }
}

void RodentDistortion::Oversampler::downsample(const float* input, float* output, int numSamples) {
    // Apply anti-aliasing filter first
    for (int i = 0; i < numSamples * OVERSAMPLE_FACTOR; ++i) {
        downsampleBuffer[i] = downsampleFilter.process(input[i]);
    }
    
    // Decimate by taking every Nth sample
    for (int i = 0; i < numSamples; ++i) {
        output[i] = downsampleBuffer[i * OVERSAMPLE_FACTOR];
    }
}

float RodentDistortion::softClip(float x) {
    // Enhanced soft clipping with multiple stages
    // Stage 1: Gentle compression
    float stage1 = x / (1.0f + std::abs(x) * 0.3f);
    
    // Stage 2: Smooth saturation
    float stage2 = std::tanh(stage1 * 1.2f);
    
    // Blend based on input level
    float blend = std::min(1.0f, std::abs(x));
    return stage1 * (1.0f - blend) + stage2 * blend;
}

float RodentDistortion::hardClip(float x) {
    // Enhanced hard clipping with slight rounding
    const float threshold = 0.95f;
    if (x > threshold) {
        float excess = x - threshold;
        return threshold + excess * 0.1f;
    }
    if (x < -threshold) {
        float excess = -x - threshold;
        return -(threshold + excess * 0.1f);
    }
    return x;
}

float RodentDistortion::processVintageMode(float input, int mode, float drive) {
    switch (mode) {
        case VintageDistortion::RAT_STYLE:
            return m_distortion.processOpAmp(input, 1.0f + drive * 10.0f, 0.7f);
            
        case VintageDistortion::TUBE_SCREAMER:
            return m_distortion.processDiodeClipping(input * (1.0f + drive * 5.0f), 2.0f + drive * 3.0f);
            
        case VintageDistortion::BIG_MUFF:
            return m_distortion.processAsymmetric(input, 1.0f + drive * 8.0f, 0.3f);
            
        case VintageDistortion::FUZZ_FACE:
            {
                // Transistor-style fuzz
                float fuzzed = std::tanh(input * (1.0f + drive * 15.0f));
                // Add some gating effect
                if (std::abs(input) < 0.05f) {
                    fuzzed *= std::abs(input) * 20.0f;
                }
                return fuzzed;
            }
            
        default:
            return softClip(input);
    }
}
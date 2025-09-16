#include "VintageOptoCompressor.h"
#include <algorithm>

VintageOptoCompressor::VintageOptoCompressor() 
    : m_randomEngine(std::random_device{}())
    , m_uniformDist(-1.0f, 1.0f) {
    // Initialize smoothed parameters
    m_gain.reset(0.5f);
    m_peakReduction.reset(0.5f);
    m_emphasis.reset(0.5f);
    m_outputGain.reset(0.5f);
    m_mix.reset(1.0f);
    m_knee.reset(0.7f);
    m_harmonics.reset(0.3f);
    m_stereoLink.reset(1.0f);
}

void VintageOptoCompressor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    m_gain.setSmoothingTime(50.0f, sampleRate);
    m_peakReduction.setSmoothingTime(100.0f, sampleRate);
    m_emphasis.setSmoothingTime(20.0f, sampleRate);
    m_outputGain.setSmoothingTime(50.0f, sampleRate);
    m_mix.setSmoothingTime(50.0f, sampleRate);
    m_knee.setSmoothingTime(100.0f, sampleRate);
    m_harmonics.setSmoothingTime(100.0f, sampleRate);
    m_stereoLink.setSmoothingTime(20.0f, sampleRate);
    
    for (int i = 0; i < static_cast<int>(m_channelStates.size()); ++i) {
        m_channelStates[i].prepare();
    }
    
    // Prepare DC blockers
    for (auto& dcBlocker : m_dcBlockers) {
        dcBlocker.prepare(sampleRate);
    }
}

void VintageOptoCompressor::reset() {
    // Reset dynamics processing state
    for (int i = 0; i < static_cast<int>(m_channelStates.size()); ++i) {
        m_channelStates[i].optoCell.brightness = 0.0f;
        m_channelStates[i].optoCell.resistance = 1000000.0f;
        m_channelStates[i].gainSmoother.currentGain = 1.0f;
        m_channelStates[i].peakDetector.reset();
        m_channelStates[i].prevSample = 0.0f;
    }

    
    // Reset DC blockers
    for (int i = 0; i < static_cast<int>(m_dcBlockers.size()); ++i) {
        m_dcBlockers[i].reset();
    }
    
    m_stereoReduction = 0.0f;
}

void VintageOptoCompressor::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;  // Add denormal protection
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update thermal modeling and aging
    m_thermalModel.update(m_sampleRate, m_randomEngine, m_uniformDist);
    updateComponentAging(m_sampleRate);
    
    // Get thermal factor for opto cell timing
    float thermalFactor = m_thermalModel.getThermalFactor();
    
    // Update all smoothed parameters
    m_gain.update();
    m_peakReduction.update();
    m_emphasis.update();
    m_outputGain.update();
    m_mix.update();
    m_knee.update();
    m_harmonics.update();
    m_stereoLink.update();
    
    // Convert parameters to actual values with thermal compensation
    float inputGainDb = m_gain.current * 40.0f;  // 0 to 40dB
    float inputGainLinear = dbToLinear(inputGainDb) * thermalFactor;
    
    float compressionAmount = m_peakReduction.current;  // 0 to 1
    float threshold = 1.0f - compressionAmount * 0.8f;  // Higher compression = lower threshold
    
    float outputGainDb = (m_outputGain.current - 0.5f) * 40.0f;  // -20 to +20dB
    float outputGainLinear = dbToLinear(outputGainDb);
    
    bool useEmphasis = m_emphasis.current > 0.5f;
    float kneeWidth = m_knee.current * 0.3f;  // 0 to 0.3
    
    // Apply thermal factor to opto cells
    for (auto& channel : m_channelStates) {
        channel.optoCell.thermalTimeFactor = thermalFactor;
    }
    
    // First pass: detect peak levels for stereo linking
    if (m_stereoLink.current > 0.5f && numChannels >= 2) {
        float maxPeak = 0.0f;
        
        for (int channel = 0; channel < std::min(numChannels, 2); ++channel) {
            const float* channelData = buffer.getReadPointer(channel);
            auto& state = m_channelStates[channel];
            
            for (int sample = 0; sample < numSamples; ++sample) {
                float input = channelData[sample] * inputGainLinear;
                float peak = state.peakDetector.detect(input);
                maxPeak = std::max(maxPeak, peak);
            }
        }
        
        // Update stereo reduction based on maximum peak
        float targetBrightness = 0.0f;
        if (maxPeak > threshold) {
            float excess = maxPeak - threshold;
            targetBrightness = softKnee(excess, 0.0f, kneeWidth);
            targetBrightness = std::min(1.0f, targetBrightness * 2.0f);  // Scale for brightness
        }
        
        // Simple smoothing for stereo reduction
        m_stereoReduction = m_stereoReduction * 0.9f + targetBrightness * 0.1f;
    }
    
    // Process each channel with bounds checking
    const int maxChannels = std::min({numChannels, 2, static_cast<int>(m_channelStates.size()), static_cast<int>(m_dcBlockers.size())});
    
    for (int channel = 0; channel < maxChannels; ++channel) {
        if (!isChannelValid(channel, maxChannels)) continue;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Reset peak detector for actual processing
        state.peakDetector.reset();
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float dry = input;  // Capture dry signal BEFORE any processing
            
            // Early bypass check - if mix is 0, skip all processing
            if (m_mix.current < 0.001f) {
                channelData[sample] = dry;
                continue;
            }
            
            // Apply DC blocking with bounds check
            if (isChannelValid(channel, static_cast<int>(m_dcBlockers.size()))) {
                input = m_dcBlockers[channel].process(input);
            }
            
            // Apply vintage noise
            input = applyAnalogNoise(input);
            
            // Apply input gain with thermal compensation
            input *= inputGainLinear;
            
            // Tube stage on input (subtle) with aging
            if (m_harmonics.current > 0.01f) {
                float ageBoost = 1.0f + (m_componentAge / 8760.0f) * 0.1f; // Slight increase over years
                input = state.tubeStage.process(input, m_harmonics.current * 0.1f * ageBoost, this);
            }
            
            // Apply pre-emphasis if enabled
            if (useEmphasis) {
                input = state.preEmphasis.processPreEmphasis(input);
            }
            
            // Peak detection
            float peak = state.peakDetector.detect(input);
            
            // Calculate target brightness for opto cell
            float targetBrightness = 0.0f;
            
            if (m_stereoLink.current > 0.5f && numChannels >= 2) {
                // Use shared stereo reduction
                targetBrightness = m_stereoReduction;
            } else {
                // Independent channel processing
                if (peak > threshold) {
                    float excess = peak - threshold;
                    targetBrightness = softKnee(excess, 0.0f, kneeWidth);
                    targetBrightness = std::min(1.0f, targetBrightness * 2.0f);
                }
            }
            
            // Update opto cell
            state.optoCell.updateBrightness(targetBrightness * compressionAmount, m_sampleRate);
            
            // Get gain reduction from opto cell
            float gainReduction = state.optoCell.getGainReduction();
            float gain = 1.0f - (gainReduction * compressionAmount);
            
            // Smooth gain changes
            gain = state.gainSmoother.process(gain);
            
            // Apply compression
            float compressed = input * gain;
            
            // Apply de-emphasis if enabled
            if (useEmphasis) {
                compressed = state.deEmphasis.processDeEmphasis(compressed);
            }
            
            // Output tube stage (more pronounced) with aging
            if (m_harmonics.current > 0.01f) {
                float ageBoost = 1.0f + (m_componentAge / 8760.0f) * 0.15f;
                compressed = state.tubeStage.process(compressed, m_harmonics.current * 0.15f * ageBoost, this);
            }
            
            // Apply output gain
            compressed *= outputGainLinear;
            
            // Mix with dry signal
            float output = compressed * m_mix.current + dry * (1.0f - m_mix.current);
            
            // Safety limit without clipping
            output = safeFloat(output);
            
            // Final safety check
            output = safeFloat(output);
            
            channelData[sample] = output;
        }
    }
    
    // Apply final NaN/Inf cleanup
    scrubBuffer(buffer);
}

float VintageOptoCompressor::softKnee(float input, float threshold, float knee) {
    if (knee <= 0.0f) {
        // Hard knee
        return (input > threshold) ? input - threshold : 0.0f;
    }
    
    float kneeStart = threshold - knee * 0.5f;
    float kneeEnd = threshold + knee * 0.5f;
    
    if (input <= kneeStart) {
        return 0.0f;
    } else if (input >= kneeEnd) {
        return input - threshold;
    } else {
        // Quadratic curve in knee region
        float kneePosition = (input - kneeStart) / knee;
        return knee * kneePosition * kneePosition * 0.5f;
    }
}

void VintageOptoCompressor::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_gain.target = params.at(0);
    if (params.count(1)) m_peakReduction.target = params.at(1);
    if (params.count(2)) m_emphasis.target = params.at(2);
    if (params.count(3)) m_outputGain.target = params.at(3);
    if (params.count(4)) {
        m_mix.target = params.at(4);
        // For bypass (mix=0), set immediately to avoid smoothing delay
        if (params.at(4) < 0.001f) {
            m_mix.current = 0.0f;
        }
    }
    if (params.count(5)) m_knee.target = params.at(5);
    if (params.count(6)) m_harmonics.target = params.at(6);
    if (params.count(7)) m_stereoLink.target = params.at(7);
}

juce::String VintageOptoCompressor::getParameterName(int index) const {
    switch (index) {
        case 0: return "Gain";
        case 1: return "Peak Reduction";
        case 2: return "HF Emphasis";
        case 3: return "Output";
        case 4: return "Mix";
        case 5: return "Knee";
        case 6: return "Harmonics";
        case 7: return "Stereo Link";
        default: return "";
    }
}

float VintageOptoCompressor::applyAnalogNoise(float input) {
    // Add vintage noise floor that increases with age
    float noiseLevel = -120.0f; // Base noise floor in dB
    float ageNoiseBoost = (m_componentAge / 8760.0f) * 10.0f; // Up to 10dB boost over years
    
    float noiseAmp = std::pow(10.0f, (noiseLevel + ageNoiseBoost) / 20.0f);
    float noise = m_uniformDist(m_randomEngine) * noiseAmp;
    
    // Add thermal noise
    noise += m_thermalModel.thermalNoise;
    
    return safeFloat(input + noise);
}

float VintageOptoCompressor::applyVintageWarmth(float input, float amount) {
    // Subtle even harmonic generation for warmth
    float thermal = m_thermalModel.getThermalFactor();
    float driven = safeFloat(input * (1.0f + amount * thermal));
    
    // Balanced saturation for vintage character
    if (driven > 0.0f) {
        return safeFloat(std::tanh(safeFloat(driven * 0.85f)) / 0.85f);
    } else {
        return safeFloat(std::tanh(safeFloat(driven * 0.85f)) / 0.85f);
    }
}

float VintageOptoCompressor::safeFloat(float value) const {
    if (std::isnan(value) || std::isinf(value)) {
        return 0.0f;
    }
    return std::clamp(value, -10.0f, 10.0f); // Reasonable audio range
}

bool VintageOptoCompressor::isChannelValid(int channel, int maxChannels) const {
    return channel >= 0 && channel < maxChannels;
}
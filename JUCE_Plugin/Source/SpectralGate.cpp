#include "SpectralGate.h"
#include <cmath>
#include <algorithm>
#include <random>

SpectralGate::SpectralGate() {
    // Initialize smoothed parameters with professional defaults
    m_threshold.reset(0.3f);
    m_frequency.reset(0.5f);
    m_bandwidth.reset(0.5f);
    m_attack.reset(0.1f);
    m_release.reset(0.5f);
    m_sidechain.reset(0.0f);
    m_invert.reset(0.0f);
    m_mix.reset(1.0f);
}

void SpectralGate::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set smoothing times for parameters
    m_threshold.setSmoothingTime(20.0f, sampleRate);   // Fast for threshold
    m_frequency.setSmoothingTime(50.0f, sampleRate);   // Medium for frequency
    m_bandwidth.setSmoothingTime(50.0f, sampleRate);   // Medium for bandwidth
    m_attack.setSmoothingTime(10.0f, sampleRate);      // Fast for attack
    m_release.setSmoothingTime(10.0f, sampleRate);     // Fast for release
    m_sidechain.setSmoothingTime(100.0f, sampleRate);  // Slow for sidechain
    m_invert.setSmoothingTime(100.0f, sampleRate);     // Slow for mode changes
    m_mix.setSmoothingTime(30.0f, sampleRate);         // Medium for mix
    
    // Prepare enhanced channel processors
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
    }

void SpectralGate::reset() {
    // Reset dynamics processing state
    for (auto& channel : m_channelStates) {
        channel.envelope = 0.0f;
        channel.gainReduction = 0.0f;
    }
}

    
    // Calculate latency with lookahead compensation
    m_latencySamples = FFT_SIZE / 2;
    if (m_enableLookahead) {
        m_latencySamples += ChannelState::EnhancedEnvelopeFollower::LOOKAHEAD_SAMPLES;
    }
    
    // Initialize component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void SpectralGate::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    updateAllSmoothParams();
    
    // Update component aging
    updateComponentAging();
    
    // Convert parameters with enhanced scaling
    float threshold = m_threshold.current * m_threshold.current; // Squared for better response
    float attackMs = 0.05f + m_attack.current * 199.95f;   // 0.05 to 200ms
    float releaseMs = 0.5f + m_release.current * 1999.5f;  // 0.5 to 2000ms
    bool invertGate = m_invert.current > 0.5f;
    
    // Update thermal models and aging
    for (auto& channel : m_channelStates) {
        if (m_enableThermalModeling) {
            channel.thermalModel.update(m_sampleRate);
        }
        if (m_enableComponentAging) {
            channel.componentAging.update(m_componentAge);
        }
        
        // Update envelope parameters with thermal and aging compensation
        float thermalFactor = m_enableThermalModeling ? channel.thermalModel.getThermalFactor() : 1.0f;
        float agingCompensation = m_enableComponentAging ? (1.0f + channel.componentAging.drift) : 1.0f;
        
        channel.fftProcessor.setEnvelopeParams(attackMs * thermalFactor, releaseMs * agingCompensation, m_sampleRate);
        
        // Update enhanced envelope followers
        for (auto& follower : channel.envelopeFollowers) {
            follower.prepare(attackMs * thermalFactor, releaseMs * agingCompensation, m_sampleRate);
        }
    }
    
    // Process each channel with enhanced quality
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break; // Stereo only
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Get thermal and aging factors
        float thermalFactor = m_enableThermalModeling ? state.thermalModel.getThermalFactor() : 1.0f;
        float agingNoise = m_enableComponentAging ? state.componentAging.getNoiseIncrease() : 0.0f;
        
        // Process through enhanced spectral gate
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float dry = input;
            
            // Apply input DC blocking
            input = state.inputDCBlocker.process(input);
            
            // Apply spectral character enhancement
            input = applySpectralCharacter(input, thermalFactor, m_componentAge);
            
            // Process through enhanced FFT-based spectral gate
            float gated = state.fftProcessor.processSample(
                input, m_frequency.current, m_bandwidth.current, threshold, invertGate, m_sampleRate
            );
            
            // Apply output DC blocking
            gated = state.outputDCBlocker.process(gated);
            
            // Add subtle noise floor with aging
            float noiseLevel = std::pow(10.0f, (state.noiseFloor + agingNoise * 10.0f) / 20.0f);
            gated += noiseLevel * ((rand() % 1000) / 1000.0f - 0.5f) * 0.001f;
            
            // Apply mix with smooth parameter
            channelData[sample] = gated * m_mix.current + dry * (1.0f - m_mix.current);
        }
    }
}

void SpectralGate::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_threshold.target = params.at(0);
    if (params.count(1)) m_frequency.target = params.at(1);
    if (params.count(2)) m_bandwidth.target = params.at(2);
    if (params.count(3)) m_attack.target = params.at(3);
    if (params.count(4)) m_release.target = params.at(4);
    if (params.count(5)) m_sidechain.target = params.at(5);
    if (params.count(6)) m_invert.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String SpectralGate::getParameterName(int index) const {
    switch (index) {
        case 0: return "Threshold";
        case 1: return "Frequency";
        case 2: return "Bandwidth";
        case 3: return "Attack";
        case 4: return "Release";
        case 5: return "Sidechain";
        case 6: return "Invert";
        case 7: return "Mix";
        default: return "";
    }
}

// Enhanced helper methods for boutique functionality
void SpectralGate::updateAllSmoothParams() {
    m_threshold.update();
    m_frequency.update();
    m_bandwidth.update();
    m_attack.update();
    m_release.update();
    m_sidechain.update();
    m_invert.update();
    m_mix.update();
}

void SpectralGate::updateComponentAging() {
    m_sampleCount++;
    if (m_sampleCount > m_sampleRate * 15) { // Every 15 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00003f);
        m_sampleCount = 0;
    }
}

float SpectralGate::applySpectralCharacter(float input, float thermalFactor, float aging) {
    float output = input;
    
    // Apply thermal modulation
    output *= thermalFactor;
    
    // Component aging effects on spectral processing
    if (aging > 0.01f) {
        // Subtle frequency response changes with aging
        static float hfRolloff = 0.0f;
        float rolloffAmount = aging * 0.05f;
        hfRolloff += (output - hfRolloff) * (1.0f - rolloffAmount);
        output = output * (1.0f - rolloffAmount) + hfRolloff * rolloffAmount;
        
        // Add subtle nonlinear distortion from aging components
        float distortion = aging * 0.02f;
        output += distortion * output * output * output;
    }
    
    return output;
}

void SpectralGate::enhanceSpectralQuality(std::vector<std::complex<float>>& spectrum, int channel) {
    if (!m_enableSpectralEnhancement) return;
    
    auto& enhancer = m_channelStates[channel].spectralEnhancer;
    enhancer.processSpectralCoherence(spectrum);
}

float SpectralGate::calculateAdaptiveThreshold(const std::vector<float>& magnitudes, float baseThreshold) {
    // Calculate adaptive threshold based on spectral content
    float sum = 0.0f;
    float max = 0.0f;
    
    for (float mag : magnitudes) {
        sum += mag;
        max = std::max(max, mag);
    }
    
    float average = sum / magnitudes.size();
    float dynamicRange = max / (average + 1e-10f);
    
    // Adjust threshold based on dynamic range
    float adaptedThreshold = baseThreshold;
    if (dynamicRange > 10.0f) {
        adaptedThreshold *= 0.8f; // Lower threshold for high dynamic range content
    } else if (dynamicRange < 3.0f) {
        adaptedThreshold *= 1.2f; // Higher threshold for compressed content
    }
    
    return adaptedThreshold;
}

void SpectralGate::applySpectralSmoothing(std::vector<std::complex<float>>& spectrum, float amount) {
    if (amount <= 0.0f) return;
    
    // Apply spectral smoothing to reduce artifacts
    static std::vector<std::complex<float>> smoothed;
    smoothed.resize(spectrum.size());
    
    // Simple moving average smoothing
    int windowSize = static_cast<int>(amount * 5.0f) + 1;
    int halfWindow = windowSize / 2;
    
    for (size_t i = 0; i < spectrum.size(); ++i) {
        std::complex<float> sum(0.0f, 0.0f);
        int count = 0;
        
        for (int j = -halfWindow; j <= halfWindow; ++j) {
            int idx = static_cast<int>(i) + j;
            if (idx >= 0 && idx < static_cast<int>(spectrum.size())) {
                sum += spectrum[idx];
                count++;
            }
        }
        
        if (count > 0) {
            smoothed[i] = sum / static_cast<float>(count);
        } else {
            smoothed[i] = spectrum[i];
        }
    }
    
    // Blend smoothed with original
    for (size_t i = 0; i < spectrum.size(); ++i) {
        spectrum[i] = spectrum[i] * (1.0f - amount) + smoothed[i] * amount;
    }
}
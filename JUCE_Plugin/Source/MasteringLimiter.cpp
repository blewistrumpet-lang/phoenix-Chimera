#include "MasteringLimiter.h"
#include <cmath>
#include <algorithm>

MasteringLimiter::MasteringLimiter() {
    // Initialize smoothed parameters with proper defaults
    m_threshold.reset(0.9f);
    m_release.reset(0.3f);
    m_lookahead.reset(0.5f);
    m_ceiling.reset(0.95f);
    m_softKnee.reset(0.5f);
    m_truePeak.reset(1.0f);
    m_character.reset(0.5f);
    m_makeupGain.reset(1.0f);
}

void MasteringLimiter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times for mastering-grade precision
    float fastSmoothingTime = 25.0f; // 25ms for responsive parameters
    float mediumSmoothingTime = 50.0f; // 50ms for moderate parameters
    float slowSmoothingTime = 100.0f; // 100ms for critical parameters
    
    m_threshold.setSmoothingTime(mediumSmoothingTime, sampleRate);
    m_release.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_lookahead.setSmoothingTime(mediumSmoothingTime, sampleRate);
    m_ceiling.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_softKnee.setSmoothingTime(mediumSmoothingTime, sampleRate);
    m_truePeak.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_character.setSmoothingTime(mediumSmoothingTime, sampleRate);
    m_makeupGain.setSmoothingTime(fastSmoothingTime, sampleRate);
    
    // Calculate maximum lookahead samples (10ms max)
    int maxLookaheadSamples = static_cast<int>(0.01 * sampleRate);
    
    for (auto& channel : m_channelStates) {
        channel.lookaheadBuffer.prepare(maxLookaheadSamples);
        channel.truePeakDetector = TruePeakDetector();
        channel.gainComputer.reset();
        channel.peakHistory.clear();
        channel.currentPeak = 0.0f;
        
        // Initialize boutique components
        channel.inputDCBlocker.reset();
        channel.outputDCBlocker.reset();
        channel.thermalModel = ThermalModel();
        channel.componentAging = ComponentAging();
        
        // Initialize enhanced processing state
        channel.prevSample = 0.0f;
        channel.evenHarmonicState = 0.0f;
        channel.oddHarmonicState = 0.0f;
        channel.envelopeFollower = 0.0f;
        channel.spectralCentroid = 0.0f;
    }

void MasteringLimiter::reset() {
    // Reset dynamics processing state
    for (auto& channel : m_channelStates) {
        channel.envelope = 0.0f;
        channel.gainReduction = 0.0f;
    }
}

    
    m_stereoLinkGain = 1.0f;
}

void MasteringLimiter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    m_threshold.update();
    m_release.update();
    m_lookahead.update();
    m_ceiling.update();
    m_softKnee.update();
    m_truePeak.update();
    m_character.update();
    m_makeupGain.update();
    
    // Convert smoothed parameters to actual values
    float thresholdLinear = m_threshold.current; // Already 0-1, where 1 = 0dB
    float releaseMsec = 1.0f + m_release.current * 999.0f; // 1-1000ms
    int lookaheadSamples = static_cast<int>(m_lookahead.current * 0.01f * m_sampleRate); // 0-10ms
    float ceilingLinear = m_ceiling.current; // 0-1, where 1 = 0dB
    float kneeWidth = m_softKnee.current * 0.2f; // 0-0.2 (20% of threshold max)
    
    // Update gain computer settings
    for (auto& channel : m_channelStates) {
        channel.gainComputer.attackTime = lookaheadSamples / m_sampleRate * 1000.0f; // ms
        channel.gainComputer.releaseTime = releaseMsec;
    }
    
    // First pass: detect peaks with lookahead
    if (lookaheadSamples > 0) {
        for (int channel = 0; channel < numChannels; ++channel) {
            if (channel >= 2) break;
            
            auto& state = m_channelStates[channel];
            const float* channelData = buffer.getReadPointer(channel);
            
            for (int sample = 0; sample < numSamples; ++sample) {
                float input = channelData[sample];
                
                // Write to lookahead buffer
                state.lookaheadBuffer.write(input);
                
                // Apply input DC blocking
                input = state.inputDCBlocker.process(input);
                
                // Add subtle analog noise
                input = state.addAnalogNoise(input);
                
                // Update thermal and aging models
                float processingLoad = std::abs(input) * 10.0f;
                state.thermalModel.update(processingLoad);
                state.componentAging.update();
                
                // Update spectral analysis for adaptive processing
                updateSpectralAnalysis(state, input, m_sampleRate);
                
                // Detect peak (with enhanced true peak if enabled)
                float peak = m_truePeak.current > 0.5f ? 
                            state.truePeakDetector.detectTruePeak(input) : 
                            std::abs(input);
                
                // Update peak history for lookahead window
                state.peakHistory.push_back(peak);
                if (state.peakHistory.size() > static_cast<size_t>(lookaheadSamples)) {
                    state.peakHistory.pop_front();
                }
                
                // Find maximum peak in lookahead window
                state.currentPeak = *std::max_element(state.peakHistory.begin(), 
                                                     state.peakHistory.end());
            }
        }
    }
    
    // Calculate stereo-linked gain reduction
    float maxPeak = 0.0f;
    for (int channel = 0; channel < std::min(numChannels, 2); ++channel) {
        maxPeak = std::max(maxPeak, m_channelStates[channel].currentPeak);
    }
    
    // Apply thermal compensation to threshold
    float thermalDrift = m_channelStates[0].thermalModel.getTemperatureDrift();
    float compensatedThreshold = thresholdLinear * (1.0f + thermalDrift);
    
    // Apply soft knee and calculate gain reduction with thermal compensation
    float limitedPeak = m_softKneeProcessor.process(maxPeak, compensatedThreshold, kneeWidth);
    m_stereoLinkGain = calculateGainReduction(maxPeak, compensatedThreshold, ceilingLinear, thermalDrift);
    
    // Second pass: apply limiting
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Read from lookahead buffer
            float input = lookaheadSamples > 0 ? 
                         state.lookaheadBuffer.read(lookaheadSamples) : 
                         channelData[sample];
            
            // Apply input DC blocking
            input = state.inputDCBlocker.process(input);
            
            // Add subtle analog noise for realism
            input = state.addAnalogNoise(input);
            
            // Apply thermal and aging compensation
            float agingFactor = state.componentAging.getAgingFactor();
            input *= agingFactor;
            
            // Smooth gain changes with thermal-aware timing
            float thermalDrift = state.thermalModel.getTemperatureDrift();
            float adaptiveRelease = releaseMsec * (1.0f + std::abs(thermalDrift) * 5.0f);
            state.gainComputer.releaseTime = adaptiveRelease;
            
            float smoothGain = state.gainComputer.process(m_stereoLinkGain, m_sampleRate);
            
            // Apply gain reduction
            float limited = input * smoothGain;
            
            // Add advanced harmonic coloration if desired
            if (m_character.current > 0.0f) {
                limited = addAdvancedHarmonicColor(state, limited, m_character.current, m_sampleRate);
            }
            
            // Apply sophisticated analog saturation
            float temperature = state.thermalModel.temperature;
            limited = applyAnalogSaturation(limited, m_character.current, temperature);
            
            // Enhanced output ceiling with soft limiting
            if (std::abs(limited) > ceilingLinear) {
                float sign = limited > 0 ? 1.0f : -1.0f;
                float excess = std::abs(limited) - ceilingLinear;
                float softLimit = ceilingLinear + std::tanh(excess * 3.0f) * 0.02f;
                limited = sign * softLimit;
            }
            
            // Apply intelligent auto makeup gain if enabled
            if (m_makeupGain.current > 0.5f) {
                float makeup = 1.0f / (compensatedThreshold + 0.001f);
                makeup = std::min(makeup, 2.0f); // Limit to +6dB
                
                // Adaptive makeup based on spectral content
                float spectralCompensation = 1.0f + state.spectralCentroid * 0.1f;
                makeup *= spectralCompensation;
                
                limited *= makeup;
            }
            
            // Apply output DC blocking
            limited = state.outputDCBlocker.process(limited);
            
            channelData[sample] = limited;
        }
    }
}

float MasteringLimiter::calculateGainReduction(float peak, float threshold, float ceiling, float thermalDrift) {
    if (peak <= threshold) {
        return 1.0f;
    }
    
    // Apply thermal compensation to calculations
    float compensatedThreshold = threshold * (1.0f + thermalDrift * 0.5f);
    float compensatedCeiling = ceiling * (1.0f + thermalDrift * 0.1f);
    
    // Calculate required gain to bring peak down to threshold
    float gainReduction = compensatedThreshold / (peak + 0.00001f);
    
    // Ensure we don't exceed ceiling
    float maxGain = compensatedCeiling / (peak + 0.00001f);
    gainReduction = std::min(gainReduction, maxGain);
    
    // Add subtle non-linearity for analog character
    gainReduction *= (1.0f - thermalDrift * 0.01f);
    
    return gainReduction;
}

float MasteringLimiter::addAdvancedHarmonicColor(ChannelState& state, float input, float amount, double sampleRate) {
    // Update harmonic generators
    float diff = input - state.prevSample;
    state.prevSample = input;
    
    // Advanced even harmonic generation (tube-like warmth)
    float evenDrive = input * 1.2f;
    state.evenHarmonicState = std::tanh(evenDrive) * 0.8f;
    float harmonic2 = state.evenHarmonicState * state.evenHarmonicState * (input > 0 ? 1.0f : -1.0f);
    
    // Odd harmonic generation (transistor-like presence)
    float oddDrive = input * 0.9f;
    state.oddHarmonicState = oddDrive / (1.0f + std::abs(oddDrive));
    float harmonic3 = state.oddHarmonicState * std::abs(state.oddHarmonicState) * state.oddHarmonicState;
    
    // Intermodulation distortion (subtle)
    float imd = harmonic2 * harmonic3 * 0.1f;
    
    // Frequency-dependent coloration
    float spectralWeight = 1.0f + state.spectralCentroid * 0.3f;
    
    // Temperature-dependent character
    float thermalDrift = state.thermalModel.getTemperatureDrift();
    float thermalCharacter = 1.0f + thermalDrift * 2.0f;
    
    // Blend all harmonics with adaptive amounts
    float colored = input + 
                   harmonic2 * amount * 0.02f * spectralWeight * thermalCharacter +
                   harmonic3 * amount * 0.008f * spectralWeight +
                   imd * amount * 0.003f +
                   diff * amount * 0.04f; // Enhanced transient emphasis
    
    return colored;
}

float MasteringLimiter::applyAnalogSaturation(float input, float drive, float temperature) {
    if (drive < 0.01f) return input;
    
    // Temperature affects saturation characteristics
    float thermalFactor = 1.0f + (temperature - 20.0f) * 0.001f;
    float adjustedDrive = drive * thermalFactor;
    
    // Multi-stage saturation for natural sound
    float stage1 = std::tanh(input * (1.0f + adjustedDrive * 2.0f)) * 0.9f;
    float stage2 = input * 0.1f; // Clean blend
    
    // Asymmetric saturation for more character
    if (input > 0) {
        stage1 *= 1.05f; // Slight even harmonic bias
    } else {
        stage1 *= 0.98f; // Slight compression on negative
    }
    
    return stage1 + stage2;
}

void MasteringLimiter::updateSpectralAnalysis(ChannelState& state, float input, double sampleRate) {
    // Simple spectral centroid estimation using envelope following
    state.envelopeFollower = state.envelopeFollower * 0.999f + std::abs(input) * 0.001f;
    
    // High frequency content estimation
    float highFreqContent = std::abs(input - state.prevSample) * sampleRate * 0.00001f;
    state.spectralCentroid = state.spectralCentroid * 0.995f + highFreqContent * 0.005f;
    
    // Clamp to reasonable range
    state.spectralCentroid = std::max(0.0f, std::min(1.0f, state.spectralCentroid));
}

void MasteringLimiter::updateParameters(const std::map<int, float>& params) {
    // Update target values for smoothed parameters
    if (params.count(0)) m_threshold.target = 0.5f + params.at(0) * 0.5f; // 0.5 to 1.0 (-6dB to 0dB)
    if (params.count(1)) m_release.target = params.at(1);
    if (params.count(2)) m_lookahead.target = params.at(2);
    if (params.count(3)) m_ceiling.target = 0.7f + params.at(3) * 0.3f; // 0.7 to 1.0 (-3dB to 0dB)
    if (params.count(4)) m_softKnee.target = params.at(4);
    if (params.count(5)) m_truePeak.target = params.at(5);
    if (params.count(6)) m_character.target = params.at(6);
    if (params.count(7)) m_makeupGain.target = params.at(7);
}

juce::String MasteringLimiter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Threshold";
        case 1: return "Release";
        case 2: return "Lookahead";
        case 3: return "Ceiling";
        case 4: return "Soft Knee";
        case 5: return "True Peak";
        case 6: return "Character";
        case 7: return "Auto Gain";
        default: return "";
    }
}
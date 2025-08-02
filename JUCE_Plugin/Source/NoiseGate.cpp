#include "NoiseGate.h"
#include <cmath>
#include <algorithm>

NoiseGate::NoiseGate() {
    // Initialize smoothed parameters with proper defaults
    m_threshold.reset(0.1f);
    m_range.reset(0.8f);
    m_attack.reset(0.1f);
    m_hold.reset(0.3f);
    m_release.reset(0.5f);
    m_hysteresis.reset(0.3f);
    m_sidechain.reset(0.5f);
    m_lookahead.reset(0.0f);
}

void NoiseGate::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times for professional gating
    float fastSmoothingTime = 25.0f; // 25ms for responsive parameters
    float mediumSmoothingTime = 50.0f; // 50ms for moderate parameters
    float slowSmoothingTime = 100.0f; // 100ms for critical parameters
    
    m_threshold.setSmoothingTime(mediumSmoothingTime, sampleRate);
    m_range.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_attack.setSmoothingTime(fastSmoothingTime, sampleRate);
    m_hold.setSmoothingTime(mediumSmoothingTime, sampleRate);
    m_release.setSmoothingTime(slowSmoothingTime, sampleRate);
    m_hysteresis.setSmoothingTime(mediumSmoothingTime, sampleRate);
    m_sidechain.setSmoothingTime(mediumSmoothingTime, sampleRate);
    m_lookahead.setSmoothingTime(fastSmoothingTime, sampleRate);
    
    // Calculate maximum lookahead samples (10ms max)
    int maxLookaheadSamples = static_cast<int>(0.01 * sampleRate);
    
    for (auto& channel : m_channelStates) {
        // Reset envelope follower
        channel.envelopeFollower.reset();
        channel.envelopeFollower.setAttackRelease(0.1f, 50.0f, sampleRate);
        
        // Initialize enhanced sidechain filter
        channel.sidechainFilter.setCutoff(100.0f, sampleRate);
        
        // Prepare lookahead buffer
        channel.lookaheadBuffer.prepare(maxLookaheadSamples);
        
        // Initialize boutique components
        channel.inputDCBlocker.reset();
        channel.outputDCBlocker.reset();
        channel.thermalModel = ThermalModel();
        channel.componentAging = ComponentAging();
        
        // Initialize state
        channel.state = CLOSED;
        channel.currentGain = 0.0f;
        channel.targetGain = 0.0f;
        channel.holdCounter = 0;
        
        // Set initial gain transition rates
        channel.attackRate = 0.01f;
        channel.releaseRate = 0.001f;
        channel.fastAttackRate = 0.1f;
        channel.slowReleaseRate = 0.0001f;
        
        // Initialize analysis state
        channel.gateConfidence = 0.0f;
        channel.transientDetected = 0.0f;
        channel.sustainDetected = 0.0f;
    }

void NoiseGate::reset() {
    // Reset dynamics processing state
    for (auto& channel : m_channelStates) {
        channel.envelope = 0.0f;
        channel.gainReduction = 0.0f;
    }
}

}

void NoiseGate::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    m_threshold.update();
    m_range.update();
    m_attack.update();
    m_hold.update();
    m_release.update();
    m_hysteresis.update();
    m_sidechain.update();
    m_lookahead.update();
    
    // Convert smoothed parameters to actual values
    float thresholdDb = -60.0f + m_threshold.current * 60.0f;  // -60 to 0 dB
    float thresholdLinear = dbToLinear(thresholdDb);
    
    float rangeDb = -40.0f + m_range.current * 40.0f;  // -40 to 0 dB
    float rangeLinear = dbToLinear(rangeDb);
    
    float attackMs = 0.1f + m_attack.current * 99.9f;  // 0.1 to 100ms
    float holdMs = m_hold.current * 500.0f;  // 0 to 500ms
    float releaseMs = 1.0f + m_release.current * 999.0f;  // 1 to 1000ms
    
    float hysteresisDb = m_hysteresis.current * 10.0f;  // 0 to 10dB
    float hysteresisLinear = dbToLinear(hysteresisDb);
    
    float sidechainFreq = 20.0f + m_sidechain.current * 480.0f;  // 20-500Hz
    int lookaheadSamples = static_cast<int>(m_lookahead.current * 10.0f * 0.001f * m_sampleRate);  // 0-10ms
    int holdSamples = static_cast<int>(holdMs * 0.001f * m_sampleRate);
    
    // Update envelope follower settings and thermal models
    for (auto& channel : m_channelStates) {
        channel.envelopeFollower.setAttackRelease(attackMs, releaseMs, m_sampleRate);
        channel.sidechainFilter.setCutoff(sidechainFreq, m_sampleRate);
        
        // Update gain transition rates based on attack/release times
        channel.attackRate = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * m_sampleRate));
        channel.releaseRate = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * m_sampleRate));
        
        // Update thermal and aging models
        float processingLoad = std::min(1.0f, numSamples / 512.0f);
        channel.thermalModel.update(processingLoad);
        channel.componentAging.update();
    }
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // If stereo link is enabled, use maximum of both channels for detection
        float* detectionData = channelData;
        std::vector<float> linkedData;
        
        if (m_stereoLink && numChannels >= 2) {
            linkedData.resize(numSamples);
            const float* leftData = buffer.getReadPointer(0);
            const float* rightData = buffer.getReadPointer(1);
            
            for (int sample = 0; sample < numSamples; ++sample) {
                linkedData[sample] = std::max(std::abs(leftData[sample]), 
                                            std::abs(rightData[sample]));
            }
            detectionData = linkedData.data();
        }
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            
            // Apply input DC blocking
            input = state.inputDCBlocker.process(input);
            
            // Add subtle analog noise
            input = state.addAnalogNoise(input);
            
            // Apply thermal and aging compensation
            float thermalDrift = state.thermalModel.getTemperatureDrift();
            float agingFactor = state.componentAging.getAgingFactor();
            input *= (1.0f + thermalDrift * 0.5f) * agingFactor;
            
            // Write to lookahead buffer
            state.lookaheadBuffer.write(input);
            
            // Update signal analysis
            updateSignalAnalysis(state, input, m_sampleRate);
            
            // Get detection signal with enhanced filtering
            float detection = detectionData[sample];
            if (m_sidechain.current > 0.01f) {
                // Use frequency-selective detection
                if (sidechainFreq < 200.0f) {
                    detection = state.sidechainFilter.processHighpass(detection);
                } else {
                    detection = state.sidechainFilter.processBandpass(detection);
                }
            }
            
            // Update envelope follower with thermal compensation
            float compensatedThreshold = thresholdLinear * (1.0f + thermalDrift);
            float envelope = state.envelopeFollower.processRMS(detection);
            
            // Advanced gate logic with confidence and thermal compensation
            processAdvancedGateLogic(state, envelope, compensatedThreshold, hysteresisLinear, holdSamples, m_sampleRate);
            
            // Update current gain with adaptive transitions
            state.updateGain();
            
            // Apply gating with lookahead compensation
            float delayed = lookaheadSamples > 0 ? 
                          state.lookaheadBuffer.read(lookaheadSamples) : 
                          input;
            
            // Calculate final gain with thermal and range considerations
            float thermalCompensatedRange = rangeLinear * agingFactor;
            float finalGain = thermalCompensatedRange + (1.0f - thermalCompensatedRange) * state.currentGain;
            
            // Apply subtle analog saturation if gain is high
            float temperature = state.thermalModel.temperature;
            delayed = applyAnalogSaturation(delayed, finalGain * 0.1f, temperature);
            
            // Apply gain
            float gated = delayed * finalGain;
            
            // Apply output DC blocking
            gated = state.outputDCBlocker.process(gated);
            
            channelData[sample] = gated;
        }
    }
}

void NoiseGate::processAdvancedGateLogic(ChannelState& state, float envelope, float threshold, 
                                       float hysteresis, int holdSamples, double sampleRate) {
    // Update gate confidence based on signal characteristics
    float signalRatio = envelope / (threshold + 0.00001f);
    float confidenceTarget = (signalRatio > 1.2f) ? 1.0f : (signalRatio < 0.8f) ? 0.0f : 0.5f;
    state.gateConfidence = state.gateConfidence * 0.99f + confidenceTarget * 0.01f;
    
    // Adaptive threshold based on confidence and signal analysis
    float adaptiveThreshold = threshold;
    if (state.transientDetected > 0.7f) {
        adaptiveThreshold *= 0.8f; // Lower threshold for transients
    }
    if (state.sustainDetected > 0.7f && state.gateConfidence > 0.8f) {
        adaptiveThreshold *= 1.1f; // Higher threshold for sustained signals
    }
    
    switch (state.state) {
        case CLOSED:
            if (envelope > adaptiveThreshold && state.gateConfidence > 0.3f) {
                state.state = OPENING;
                state.targetGain = 1.0f;
            }
            break;
            
        case OPENING:
            if (state.currentGain >= 0.99f) {
                state.state = OPEN;
                state.holdCounter = holdSamples;
            } else if (envelope < adaptiveThreshold * (1.0f - hysteresis) && state.gateConfidence < 0.3f) {
                state.state = CLOSING;
                state.targetGain = 0.0f;
            }
            break;
            
        case OPEN:
            if (envelope < adaptiveThreshold * (1.0f - hysteresis) && state.gateConfidence < 0.5f) {
                state.state = HOLDING;
            }
            break;
            
        case HOLDING:
            if (state.holdCounter > 0) {
                state.holdCounter--;
                if (envelope > adaptiveThreshold || state.gateConfidence > 0.7f) {
                    state.state = OPEN;
                    state.holdCounter = holdSamples;
                }
            } else {
                state.state = CLOSING;
                state.targetGain = 0.0f;
            }
            break;
            
        case CLOSING:
            if (state.currentGain <= 0.01f) {
                state.state = CLOSED;
            } else if (envelope > adaptiveThreshold && state.gateConfidence > 0.4f) {
                state.state = OPENING;
                state.targetGain = 1.0f;
            }
            break;
    }
}

void NoiseGate::updateSignalAnalysis(ChannelState& state, float input, double sampleRate) {
    // Transient detection (high frequency energy)
    float highFreqContent = std::abs(input - state.envelopeFollower.lastSample);
    state.transientDetected = state.transientDetected * 0.95f + 
                             (highFreqContent > 0.1f ? 1.0f : 0.0f) * 0.05f;
    
    // Sustain detection (consistent energy)
    float consistency = 1.0f - std::abs(state.envelopeFollower.envelope - std::abs(input));
    state.sustainDetected = state.sustainDetected * 0.99f + 
                           (consistency > 0.8f ? 1.0f : 0.0f) * 0.01f;
}

float NoiseGate::applyAnalogSaturation(float input, float drive, float temperature) {
    if (drive < 0.01f) return input;
    
    // Temperature affects saturation characteristics
    float thermalFactor = 1.0f + (temperature - 20.0f) * 0.0005f;
    float adjustedDrive = drive * thermalFactor;
    
    // Gentle VCA-style saturation
    float saturated = std::tanh(input * (1.0f + adjustedDrive)) * (1.0f - adjustedDrive * 0.1f);
    
    return input * (1.0f - adjustedDrive) + saturated * adjustedDrive;
}

void NoiseGate::updateParameters(const std::map<int, float>& params) {
    // Update target values for smoothed parameters
    if (params.count(0)) m_threshold.target = params.at(0);
    if (params.count(1)) m_range.target = params.at(1);
    if (params.count(2)) m_attack.target = params.at(2);
    if (params.count(3)) m_hold.target = params.at(3);
    if (params.count(4)) m_release.target = params.at(4);
    if (params.count(5)) m_hysteresis.target = params.at(5);
    if (params.count(6)) m_sidechain.target = params.at(6);
    if (params.count(7)) m_lookahead.target = params.at(7);
}

juce::String NoiseGate::getParameterName(int index) const {
    switch (index) {
        case 0: return "Threshold";
        case 1: return "Range";
        case 2: return "Attack";
        case 3: return "Hold";
        case 4: return "Release";
        case 5: return "Hysteresis";
        case 6: return "SC Filter";
        case 7: return "Lookahead";
        default: return "";
    }
}
#include "JuceHeader.h"
#include "StereoImager.h"
#include "DspEngineUtilities.h"
#include <cmath>
#include <algorithm>

StereoImager::StereoImager() {
    // Initialize smoothed parameters with professional defaults
    m_width.reset(0.5f);          // Normal width
    m_lowWidth.reset(0.3f);       // Slightly narrow low end
    m_midWidth.reset(0.7f);       // Wide midrange
    m_highWidth.reset(0.8f);      // Wide high end
    m_crossover1.reset(0.3f);     // ~250Hz
    m_crossover2.reset(0.7f);     // ~2.5kHz
    m_phase.reset(0.5f);          // No phase shift
    m_mix.reset(1.0f);            // Full wet
}

void StereoImager::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set smoothing times for parameters
    m_width.setSmoothingTime(100.0f, sampleRate);       // Medium for width
    m_lowWidth.setSmoothingTime(150.0f, sampleRate);    // Slower for crossover bands
    m_midWidth.setSmoothingTime(150.0f, sampleRate);
    m_highWidth.setSmoothingTime(150.0f, sampleRate);
    m_crossover1.setSmoothingTime(200.0f, sampleRate);  // Slow for crossover changes
    m_crossover2.setSmoothingTime(200.0f, sampleRate);
    m_phase.setSmoothingTime(100.0f, sampleRate);       // Medium for phase
    m_mix.setSmoothingTime(50.0f, sampleRate);          // Fast for mix
    
    // Prepare channel state
    m_channelState.prepare(sampleRate);
    
    // Initialize component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void StereoImager::reset() {
    // Reset all internal state
    
    // Reset channel state components
    m_channelState.crossover.reset();
    m_channelState.inputDCBlocker.reset();
    m_channelState.outputDCBlocker.reset();
    
    // Reset binaural processor
    std::fill(m_channelState.binaural.convolutionBuffer.begin(), 
              m_channelState.binaural.convolutionBuffer.end(), 0.0f);
    m_channelState.binaural.bufferPos = 0;
    
    // Reset pseudo stereo processor
    for (auto& filter : m_channelState.pseudoStereo.leftFilters) {
        filter.delay1 = filter.delay2 = filter.delay3 = 0.0f;
    }
    for (auto& filter : m_channelState.pseudoStereo.rightFilters) {
        filter.delay1 = filter.delay2 = filter.delay3 = 0.0f;
    }
    
    // Reset phase adjuster
    for (auto& stage : m_channelState.phaseAdjuster.stages) {
        stage.x1 = stage.y1 = 0.0f;
    }
    
    // Reset thermal model
    m_channelState.thermalModel.thermalNoise = 0.0f;
    
    // Reset component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    m_channelState.componentAging.update(0.0f);
    
    // Reset correlation analyzer
    std::fill(m_correlationAnalyzer.leftHistory.begin(), 
              m_correlationAnalyzer.leftHistory.end(), 0.0f);
    std::fill(m_correlationAnalyzer.rightHistory.begin(), 
              m_correlationAnalyzer.rightHistory.end(), 0.0f);
    m_correlationAnalyzer.historyPos = 0;
    m_correlationAnalyzer.correlation = 0.0f;
    
    // Reset delay buffer
    std::fill(m_channelState.delayBuffer.begin(), 
              m_channelState.delayBuffer.end(), 0.0f);
    m_channelState.delayPos = 0;
    
    // Reset oversampler if used
    if (m_channelState.useOversampling) {
        std::fill(m_channelState.oversampler.upsampleBuffer.begin(), 
                  m_channelState.oversampler.upsampleBuffer.end(), 0.0f);
        std::fill(m_channelState.oversampler.downsampleBuffer.begin(), 
                  m_channelState.oversampler.downsampleBuffer.end(), 0.0f);
        
        // Reset anti-aliasing filters
        m_channelState.oversampler.upsampleFilter.x1 = 0.0f;
        m_channelState.oversampler.upsampleFilter.x2 = 0.0f;
        m_channelState.oversampler.upsampleFilter.y1 = 0.0f;
        m_channelState.oversampler.upsampleFilter.y2 = 0.0f;
        m_channelState.oversampler.downsampleFilter.x1 = 0.0f;
        m_channelState.oversampler.downsampleFilter.x2 = 0.0f;
        m_channelState.oversampler.downsampleFilter.y1 = 0.0f;
        m_channelState.oversampler.downsampleFilter.y2 = 0.0f;
    }
    
    // Reset smoothed parameters to current values (maintain continuity)
    m_width.current = m_width.target;
    m_lowWidth.current = m_lowWidth.target;
    m_midWidth.current = m_midWidth.target;
    m_highWidth.current = m_highWidth.target;
    m_crossover1.current = m_crossover1.target;
    m_crossover2.current = m_crossover2.target;
    m_phase.current = m_phase.target;
    m_mix.current = m_mix.target;
}


void StereoImager::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    updateAllSmoothParams();
    
    // Update component aging
    updateComponentAging();
    
    // Update thermal and aging models
    if (m_enableThermalModeling) {
        m_channelState.thermalModel.update(m_sampleRate);
    }
    if (m_enableComponentAging) {
        m_channelState.componentAging.update(m_componentAge);
    }
    
    // Update crossover frequencies with thermal drift
    float thermalFactor = m_enableThermalModeling ? m_channelState.thermalModel.getThermalFactor() : 1.0f;
    float lowFreq = (50.0f + m_crossover1.current * 450.0f) * thermalFactor;  // 50-500Hz
    float highFreq = (1500.0f + m_crossover2.current * 6500.0f) * thermalFactor; // 1.5-8kHz
    m_channelState.crossover.prepare(lowFreq, highFreq, m_sampleRate);
    
    // Update phase adjuster with aging compensation
    float phaseShift = (m_phase.current - 0.5f) * 2.0f; // -1 to +1
    if (m_enableComponentAging) {
        phaseShift += m_channelState.componentAging.getPhaseShift();
    }
    m_channelState.phaseAdjuster.setPhase(phaseShift);
    
    // Process samples
    for (int sample = 0; sample < numSamples; ++sample) {
        float left = 0.0f, right = 0.0f;
        float dryLeft = 0.0f, dryRight = 0.0f;
        
        if (numChannels >= 2) {
            // Stereo processing
            left = buffer.getReadPointer(0)[sample];
            right = buffer.getReadPointer(1)[sample];
            dryLeft = left;
            dryRight = right;
            
            // Apply input DC blocking
            left = m_channelState.inputDCBlocker.process(left);
            right = m_channelState.inputDCBlocker.process(right);
            
            // Analyze correlation for intelligent processing
            m_correlationAnalyzer.update(left, right);
            
            // Apply thermal and aging characteristics
            float thermalFactor = m_enableThermalModeling ? m_channelState.thermalModel.getThermalFactor() : 1.0f;
            left = applyAnalogCharacter(left, thermalFactor, m_componentAge);
            right = applyAnalogCharacter(right, thermalFactor, m_componentAge);
            
            // Choose processing mode based on correlation
            if (m_correlationAnalyzer.isMono()) {
                // Mono source - use pseudo stereo
                float mono = (left + right) * 0.5f;
                processPseudoStereo(mono, left, right);
            } else if (m_correlationAnalyzer.isUncorrelated()) {
                // Already stereo - use binaural enhancement
                float mono = (left + right) * 0.5f;
                processBinaural(mono, left, right);
            } else {
                // Regular stereo - use multiband processing
                processMultiband(left, right);
            }
            
            // Apply phase coherence improvement
            applyPhaseCoherence(left, right, phaseShift);
            
            // Compensate for channel imbalance from aging
            if (m_enableComponentAging) {
                compensateChannelImbalance(left, right, m_componentAge);
            }
            
            // Apply output DC blocking
            left = m_channelState.outputDCBlocker.process(left);
            right = m_channelState.outputDCBlocker.process(right);
            
            // Add subtle noise floor for realism
            float noiseLevel = std::pow(10.0f, m_channelState.noiseFloor / 20.0f);
            left += noiseLevel * m_channelState.thermalModel.dist(m_channelState.thermalModel.rng) * 0.001f;
            right += noiseLevel * m_channelState.thermalModel.dist(m_channelState.thermalModel.rng) * 0.001f;
            
            // Mix with dry signal
            left = dryLeft * (1.0f - m_mix.current) + left * m_mix.current;
            right = dryRight * (1.0f - m_mix.current) + right * m_mix.current;
            
        } else if (numChannels >= 1) {
            // Mono to stereo conversion
            float mono = buffer.getReadPointer(0)[sample];
            dryLeft = dryRight = mono;
            
            // Apply input DC blocking
            mono = m_channelState.inputDCBlocker.process(mono);
            
            // Apply analog character
            float thermalFactor = m_enableThermalModeling ? m_channelState.thermalModel.getThermalFactor() : 1.0f;
            mono = applyAnalogCharacter(mono, thermalFactor, m_componentAge);
            
            // Create stereo from mono using pseudo stereo
            processPseudoStereo(mono, left, right);
            
            // Apply phase adjustment
            right = m_channelState.phaseAdjuster.process(right);
            
            // Apply output DC blocking
            left = m_channelState.outputDCBlocker.process(left);
            right = m_channelState.outputDCBlocker.process(right);
            
            // Mix with dry
            left = dryLeft * (1.0f - m_mix.current) + left * m_mix.current;
            right = dryRight * (1.0f - m_mix.current) + right * m_mix.current;
        }
        
        // Write outputs
        if (numChannels >= 1) buffer.getWritePointer(0)[sample] = left;
        if (numChannels >= 2) buffer.getWritePointer(1)[sample] = right;
    }
    
    // Apply final NaN/Inf cleanup
    scrubBuffer(buffer);
}

void StereoImager::processClassicMS(float& left, float& right, float width) {
    // Convert to Mid-Side
    float mid = (left + right) * 0.5f;
    float side = (left - right) * 0.5f;
    
    // Apply width control
    side *= width;
    
    // Convert back to L-R
    left = mid + side;
    right = mid - side;
}

void StereoImager::processMultiband(float& left, float& right) {
    // Split into frequency bands
    double leftLow, leftMid, leftHigh;
    double rightLow, rightMid, rightHigh;
    
    m_channelState.crossover.process(left, leftLow, leftMid, leftHigh);
    m_channelState.crossover.process(right, rightLow, rightMid, rightHigh);
    
    // Apply different width settings per band
    float leftLowF = static_cast<float>(leftLow);
    float rightLowF = static_cast<float>(rightLow);
    processClassicMS(leftLowF, rightLowF, m_lowWidth.current * 2.0f);
    
    float leftMidF = static_cast<float>(leftMid);
    float rightMidF = static_cast<float>(rightMid);
    processClassicMS(leftMidF, rightMidF, m_midWidth.current * 2.0f);
    
    float leftHighF = static_cast<float>(leftHigh);
    float rightHighF = static_cast<float>(rightHigh);
    processClassicMS(leftHighF, rightHighF, m_highWidth.current * 2.0f);
    
    // Recombine bands
    left = leftLowF + leftMidF + leftHighF;
    right = rightLowF + rightMidF + rightHighF;
}

void StereoImager::processBinaural(float input, float& left, float& right) {
    // Process through binaural processor
    m_channelState.binaural.process(input, left, right);
    
    // Apply overall width control
    processClassicMS(left, right, m_width.current * 2.0f);
}

void StereoImager::processPseudoStereo(float input, float& left, float& right) {
    // Create pseudo stereo from mono
    m_channelState.pseudoStereo.process(input, left, right);
    
    // Apply width control
    processClassicMS(left, right, m_width.current * 2.0f);
}

void StereoImager::enhanceStereoField(float& left, float& right, float width, float correlation) {
    // Adaptive stereo enhancement based on correlation
    float adaptiveWidth = width;
    
    if (correlation > 0.8f) {
        // Highly correlated - enhance separation
        adaptiveWidth *= 1.5f;
    } else if (correlation < -0.5f) {
        // Anti-correlated - reduce processing
        adaptiveWidth *= 0.7f;
    }
    
    processClassicMS(left, right, adaptiveWidth * 2.0f);
}

void StereoImager::applyPhaseCoherence(float& left, float& right, float phase) {
    // Apply phase shift to right channel for coherence control
    if (std::abs(phase) > 0.01f) {
        right = m_channelState.phaseAdjuster.process(right);
    }
    
    // Enhance phase relationship
    float mid = (left + right) * 0.5f;
    float side = (left - right) * 0.5f;
    
    // Apply subtle phase correction to side signal
    static float phaseCorrection = 0.0f;
    phaseCorrection += (side - phaseCorrection) * 0.05f;
    side = side * 0.98f + phaseCorrection * 0.02f;
    
    left = mid + side;
    right = mid - side;
}

void StereoImager::compensateChannelImbalance(float& left, float& right, float aging) {
    if (aging > 0.01f) {
        m_channelState.componentAging.applyImbalance(left, right);
    }
}

float StereoImager::applyAnalogCharacter(float input, float thermalFactor, float aging) {
    float output = input;
    
    // Apply thermal modulation
    output *= thermalFactor;
    
    // Component aging effects on stereo processing
    if (aging > 0.01f) {
        // Subtle nonlinearity from aging components
        float nonlinearity = aging * 0.015f;
        output += nonlinearity * output * output * (output > 0 ? 1.0f : -1.0f);
        
        // Slight frequency response changes
        static float hfRolloff = 0.0f;
        float rolloffAmount = aging * 0.02f;
        hfRolloff += (output - hfRolloff) * (1.0f - rolloffAmount);
        output = output * (1.0f - rolloffAmount * 0.5f) + hfRolloff * rolloffAmount * 0.5f;
    }
    
    return output;
}

void StereoImager::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_width.target = params.at(0);
    if (params.count(1)) m_lowWidth.target = params.at(1);
    if (params.count(2)) m_midWidth.target = params.at(2);
    if (params.count(3)) m_highWidth.target = params.at(3);
    if (params.count(4)) m_crossover1.target = params.at(4);
    if (params.count(5)) m_crossover2.target = params.at(5);
    if (params.count(6)) m_phase.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String StereoImager::getParameterName(int index) const {
    switch (index) {
        case 0: return "Width";
        case 1: return "Low Width";
        case 2: return "Mid Width";
        case 3: return "High Width";
        case 4: return "Low X-over";
        case 5: return "High X-over";
        case 6: return "Phase";
        case 7: return "Mix";
        default: return "";
    }
}

// Helper methods for boutique functionality
void StereoImager::updateAllSmoothParams() {
    m_width.update();
    m_lowWidth.update();
    m_midWidth.update();
    m_highWidth.update();
    m_crossover1.update();
    m_crossover2.update();
    m_phase.update();
    m_mix.update();
}

void StereoImager::updateComponentAging() {
    m_sampleCount++;
    if (m_sampleCount > m_sampleRate * 12) { // Every 12 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00004f);
        m_sampleCount = 0;
    }
}
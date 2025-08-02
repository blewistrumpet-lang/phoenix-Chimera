#include "ClassicCompressor.h"
#include <algorithm>

ClassicCompressor::ClassicCompressor() {
    // Initialize with classic compressor defaults
    m_threshold.setImmediate(-12.0f);
    m_ratio.setImmediate(4.0f);
    m_attack.setImmediate(10.0f);
    m_release.setImmediate(100.0f);
    m_knee.setImmediate(0.5f);
    m_makeupGain.setImmediate(0.0f);
    m_dryWetMix.setImmediate(1.0f); // 100% wet by default
    m_vintage.setImmediate(0.5f);   // Moderate vintage character
    m_warmth.setImmediate(0.3f);    // Subtle warmth
    
    // Set different smoothing rates
    m_threshold.setSmoothingRate(0.995f);
    m_ratio.setSmoothingRate(0.99f);
    m_attack.setSmoothingRate(0.9f);
    m_release.setSmoothingRate(0.9f);
    m_knee.setSmoothingRate(0.99f);
    m_makeupGain.setSmoothingRate(0.995f);
    m_dryWetMix.setSmoothingRate(0.995f);
    m_vintage.setSmoothingRate(0.995f);
    m_warmth.setSmoothingRate(0.995f);
}

void ClassicCompressor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Reset envelope states
    for (auto& env : m_envelopes) {
        env.reset();
    }
    
    // Reset sidechain processors
    for (auto& sidechain : m_sidechainProcessors) {
        sidechain.reset();
        sidechain.highpass.updateCoefficients(80.0f, sampleRate); // 80Hz highpass
    }
    
    // Reset DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    // Prepare oversampler
    m_oversampler.prepare(samplesPerBlock);
    
    m_currentGainReduction = 0.0f;
    m_channelGainReduction.fill(0.0f);
}

void ClassicCompressor::reset() {
    // Reset envelope followers
    for (auto& env : m_envelopes) {
        env.reset();
    }
    
    // Reset sidechain processors
    for (auto& sidechain : m_sidechainProcessors) {
        sidechain.reset();
    }
    
    // Reset DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    // Reset gain reduction meters
    m_currentGainReduction = 0.0f;
    m_channelGainReduction.fill(0.0f);
    
    // Reset thermal model
    m_enhancedThermal.temperature = 25.0f;
    m_enhancedThermal.thermalNoise = 0.0f;
    m_enhancedThermal.componentDrift = 0.0f;
    
    // Reset component age
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Reset oversampling filters
    m_oversampler.upsampleFilter.x.fill(0.0f);
    m_oversampler.upsampleFilter.y.fill(0.0f);
    m_oversampler.downsampleFilter.x.fill(0.0f);
    m_oversampler.downsampleFilter.y.fill(0.0f);
}

void ClassicCompressor::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    m_threshold.update();
    m_ratio.update();
    m_attack.update();
    m_release.update();
    m_knee.update();
    m_makeupGain.update();
    m_dryWetMix.update();
    m_vintage.update();
    m_warmth.update();
    
    // Update enhanced thermal model
    m_enhancedThermal.update(m_sampleRate, m_threshold.current);
    
    // Calculate time constants with proper analog modeling
    const float attackTime = m_attack.current * 0.001f; // Convert to seconds
    const float releaseTime = m_release.current * 0.001f;
    
    // Non-linear attack/release curves (like analog)
    const float attackCoeff = 1.0f - std::exp(-2.2f / (attackTime * m_sampleRate));
    const float releaseCoeff = 1.0f - std::exp(-2.2f / (releaseTime * m_sampleRate));
    
    // Convert makeup gain to linear
    const float makeupGainLinear = dbToLinear(m_makeupGain.current);
    
    float maxGainReduction = 0.0f;
    
    // Process stereo linked or dual mono
    if (numChannels >= 2 && m_stereoMode == StereoMode::STEREO_LINK) {
        float* leftData = buffer.getWritePointer(0);
        float* rightData = buffer.getWritePointer(1);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Store dry signal for parallel compression
            float dryLeft = leftData[sample];
            float dryRight = rightData[sample];
            
            // Sidechain processing
            float sidechainLeft = m_sidechainProcessors[0].highpass.process(leftData[sample]);
            float sidechainRight = m_sidechainProcessors[1].highpass.process(rightData[sample]);
            
            // Stereo linked detection (use max of both channels)
            float peakLevel = std::max(std::abs(sidechainLeft), std::abs(sidechainRight));
            
            // Process envelope (RMS mode for smoother compression)
            float envelope = m_envelopes[0].processRMS(peakLevel, attackCoeff, releaseCoeff);
            
            // Update auto-release
            m_envelopes[0].updateAutoRelease(linearToDb(envelope), m_threshold.current);
            
            // Calculate gain reduction
            float gainReduction = calculateGainReduction(envelope, 0);
            float gainLinear = dbToLinear(-gainReduction);
            
            // Apply transformer saturation (subtle)
            leftData[sample] = m_analogStage.processTransformer(leftData[sample]);
            rightData[sample] = m_analogStage.processTransformer(rightData[sample]);
            
            // Apply compression with makeup gain
            leftData[sample] *= gainLinear * makeupGainLinear;
            rightData[sample] *= gainLinear * makeupGainLinear;
            
            // DC blocking
            leftData[sample] = m_dcBlockers[0].process(leftData[sample]);
            rightData[sample] = m_dcBlockers[1].process(rightData[sample]);
            
            // Parallel compression mix
            leftData[sample] = dryLeft * (1.0f - m_dryWetMix.current) + 
                              leftData[sample] * m_dryWetMix.current;
            rightData[sample] = dryRight * (1.0f - m_dryWetMix.current) + 
                               rightData[sample] * m_dryWetMix.current;
            
            // Track gain reduction
            maxGainReduction = std::max(maxGainReduction, gainReduction);
        }
    }
    else {
        // Process each channel independently
        for (int channel = 0; channel < numChannels; ++channel) {
            float* channelData = buffer.getWritePointer(channel);
            auto& envelope = m_envelopes[channel % m_envelopes.size()];
            auto& sidechain = m_sidechainProcessors[channel % m_sidechainProcessors.size()];
            auto& dcBlocker = m_dcBlockers[channel % m_dcBlockers.size()];
            
            for (int sample = 0; sample < numSamples; ++sample) {
                float dry = channelData[sample];
                
                // Sidechain processing
                float sidechainSignal = sidechain.highpass.process(channelData[sample]);
                
                // Envelope detection
                float env = envelope.processRMS(std::abs(sidechainSignal), attackCoeff, releaseCoeff);
                
                // Auto-release
                envelope.updateAutoRelease(linearToDb(env), m_threshold.current);
                
                // Calculate gain reduction
                float gainReduction = calculateGainReduction(env, channel);
                float gainLinear = dbToLinear(-gainReduction);
                
                // Apply transformer saturation
                channelData[sample] = m_analogStage.processTransformer(channelData[sample]);
                
                // Apply compression
                channelData[sample] *= gainLinear * makeupGainLinear;
                
                // DC blocking
                channelData[sample] = dcBlocker.process(channelData[sample]);
                
                // Parallel compression
                channelData[sample] = dry * (1.0f - m_dryWetMix.current) + 
                                     channelData[sample] * m_dryWetMix.current;
                
                // Track gain reduction
                m_channelGainReduction[channel % 2] = gainReduction;
                maxGainReduction = std::max(maxGainReduction, gainReduction);
            }
        }
    }
    
    // Update metering with smooth ballistics
    m_currentGainReduction = m_currentGainReduction * 0.95f + maxGainReduction * 0.05f;
}

float ClassicCompressor::calculateGainReduction(float inputLevel, int channel) {
    if (inputLevel <= 0.0f) return 0.0f;
    
    float inputDb = linearToDb(inputLevel);
    
    // Calculate knee compression
    float kneeWidth = m_knee.current * 12.0f; // 0-12 dB knee width
    float gainReduction = calculateKneeGain(inputDb, m_threshold.current, 
                                           m_ratio.current, kneeWidth);
    
    return std::max(0.0f, gainReduction);
}

float ClassicCompressor::calculateKneeGain(float inputDb, float threshold, 
                                          float ratio, float kneeWidth) {
    // No compression below threshold - knee/2
    if (inputDb < threshold - kneeWidth / 2.0f) {
        return 0.0f;
    }
    
    // Full ratio above threshold + knee/2
    if (inputDb > threshold + kneeWidth / 2.0f) {
        float excess = inputDb - threshold;
        return excess * (1.0f - 1.0f / ratio);
    }
    
    // Knee region - smooth polynomial transition
    float x = inputDb - threshold + kneeWidth / 2.0f;
    float t = x / kneeWidth;
    
    // Quadratic knee function
    float kneeGain = t * t * (inputDb - threshold + kneeWidth / 2.0f);
    return kneeGain * (1.0f - 1.0f / ratio);
}

// EnvelopeFollower implementation
float ClassicCompressor::EnvelopeFollower::processPeak(float input, 
                                                       float attackCoeff, 
                                                       float releaseCoeff) {
    float rectified = std::abs(input);
    
    // Adaptive release
    float adaptedRelease = releaseCoeff * releaseMultiplier;
    
    if (rectified > envelope) {
        // Attack
        envelope = rectified + attackCoeff * (envelope - rectified);
    } else {
        // Release
        envelope = rectified + adaptedRelease * (envelope - rectified);
    }
    
    return envelope;
}

float ClassicCompressor::EnvelopeFollower::processRMS(float input, 
                                                      float attackCoeff, 
                                                      float releaseCoeff) {
    // Update RMS window
    float squared = input * input;
    rmsAccumulator -= rmsWindow[rmsWindowPos];
    rmsAccumulator += squared;
    rmsWindow[rmsWindowPos] = squared;
    rmsWindowPos = (rmsWindowPos + 1) % RMS_WINDOW_SIZE;
    
    // Calculate RMS
    float rms = std::sqrt(rmsAccumulator / RMS_WINDOW_SIZE);
    
    // Apply peak envelope to RMS signal
    return processPeak(rms, attackCoeff, releaseCoeff);
}

void ClassicCompressor::EnvelopeFollower::updateAutoRelease(float currentLevel, 
                                                            float threshold) {
    // Program-dependent release
    float levelAboveThreshold = currentLevel - threshold;
    
    if (levelAboveThreshold > 10.0f) {
        // Fast release for transients
        releaseMultiplier = 0.1f;
    } else if (levelAboveThreshold > 0.0f) {
        // Medium release for sustained content
        releaseMultiplier = 0.5f;
    } else {
        // Slow release when below threshold
        releaseMultiplier = 2.0f;
    }
    
    // Smooth the multiplier changes
    static float smoothedMultiplier = 1.0f;
    smoothedMultiplier += (releaseMultiplier - smoothedMultiplier) * 0.01f;
    releaseMultiplier = smoothedMultiplier;
}

// SidechainProcessor::ButterworthHP implementation
void ClassicCompressor::SidechainProcessor::ButterworthHP::updateCoefficients(float freq, 
                                                                             double sampleRate) {
    float omega = 2.0f * M_PI * freq / sampleRate;
    float sin_omega = std::sin(omega);
    float cos_omega = std::cos(omega);
    float alpha = sin_omega / std::sqrt(2.0f);
    
    float norm = 1.0f / (1.0f + alpha);
    
    a0 = (1.0f + cos_omega) / 2.0f * norm;
    a1 = -(1.0f + cos_omega) * norm;
    a2 = (1.0f + cos_omega) / 2.0f * norm;
    b1 = -2.0f * cos_omega * norm;
    b2 = (1.0f - alpha) * norm;
}

float ClassicCompressor::SidechainProcessor::ButterworthHP::process(float input) {
    float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
    
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}

// AnalogStage implementation
float ClassicCompressor::AnalogStage::processTransformer(float input) {
    // Subtle transformer saturation
    float drive = 1.0f + transformerDrive * 0.5f;
    float x = input * drive;
    
    // Asymmetric saturation (like real transformers)
    float positive = x > 0.0f ? x : 0.0f;
    float negative = x < 0.0f ? -x : 0.0f;
    
    positive = std::tanh(positive * 0.7f) * 1.428f;
    negative = std::tanh(negative * 0.8f) * 1.25f;
    
    float output = positive - negative;
    
    // Add subtle harmonics
    float harmonics = std::sin(output * 3.0f) * transformerColor * 0.02f;
    
    return output + harmonics;
}

// Oversampler implementation
void ClassicCompressor::Oversampler::prepare(int blockSize) {
    upsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
    downsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
}

float ClassicCompressor::Oversampler::AAFilter::process(float input) {
    // 4th order Butterworth at Nyquist/4
    float output = input * 0.0625f + x[0] * 0.25f + x[1] * 0.375f + 
                   x[2] * 0.25f + x[3] * 0.0625f;
    
    // Shift buffer
    x[3] = x[2];
    x[2] = x[1];
    x[1] = x[0];
    x[0] = input;
    
    return output;
}

void ClassicCompressor::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Map normalized 0-1 values to actual parameter ranges
    m_threshold.target = -60.0f + getParam(0, 0.8f) * 60.0f;      // -60 to 0 dB
    
    // Ratio with special handling for infinity
    float ratioParam = getParam(1, 0.2f);
    if (ratioParam > 0.95f) {
        m_ratio.target = 1000.0f; // Effectively infinity
    } else {
        m_ratio.target = 1.0f + ratioParam * 19.0f; // 1:1 to 20:1
    }
    
    m_attack.target = 0.1f + getParam(2, 0.1f) * 99.9f;           // 0.1 to 100ms
    m_release.target = 10.0f + getParam(3, 0.05f) * 1990.0f;      // 10 to 2000ms
    m_knee.target = getParam(4, 0.5f);                            // 0-1 (hard to soft)
    m_makeupGain.target = getParam(5, 0.0f) * 24.0f;              // 0 to +24 dB
    m_vintage.target = getParam(6, 0.5f);                         // 0-1 (vintage character)
    m_warmth.target = getParam(7, 0.3f);                          // 0-1 (harmonic warmth)
}

juce::String ClassicCompressor::getParameterName(int index) const {
    switch (index) {
        case 0: return "Threshold";
        case 1: return "Ratio";
        case 2: return "Attack";
        case 3: return "Release";
        case 4: return "Knee";
        case 5: return "Makeup";
        case 6: return "Vintage";
        case 7: return "Warmth";
        default: return "";
    }
}
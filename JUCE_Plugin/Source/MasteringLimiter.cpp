// MasteringLimiter.cpp - Professional Studio-Quality Implementation
#include "MasteringLimiter.h"
#include <algorithm>
#include <cmath>

MasteringLimiter::MasteringLimiter() {
    // Initialize all parameter smoothers
    m_threshold = std::make_unique<ParameterSmoother>();
    m_ceiling = std::make_unique<ParameterSmoother>();
    m_release = std::make_unique<ParameterSmoother>();
    m_lookahead = std::make_unique<ParameterSmoother>();
    m_knee = std::make_unique<ParameterSmoother>();
    m_makeup = std::make_unique<ParameterSmoother>();
    m_saturation = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
    m_stereoLink = std::make_unique<ParameterSmoother>();
    m_truePeak = std::make_unique<ParameterSmoother>();
    
    // Set professional defaults for mastering
    m_threshold->reset(-12.0f);   // -12dB threshold
    m_ceiling->reset(-0.3f);      // -0.3dB ceiling (safe for all platforms)
    m_release->reset(50.0f);      // 50ms release
    m_lookahead->reset(2.0f);     // 2ms lookahead
    m_knee->reset(0.5f);          // Soft knee (0-1)
    m_makeup->reset(0.0f);        // Auto makeup gain
    m_saturation->reset(0.0f);    // No saturation by default
    m_mix->reset(1.0f);           // 100% wet
    m_stereoLink->reset(1.0f);    // Fully linked stereo
    m_truePeak->reset(1.0f);      // True peak detection enabled
}

void MasteringLimiter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Configure parameter smoothers with appropriate rates
    m_threshold->setSampleRate(sampleRate, 10.0f);    // Fast for threshold
    m_ceiling->setSampleRate(sampleRate, 10.0f);      // Fast for ceiling
    m_release->setSampleRate(sampleRate, 20.0f);      // Moderate for release
    m_lookahead->setSampleRate(sampleRate, 50.0f);    // Slow for lookahead
    m_knee->setSampleRate(sampleRate, 30.0f);         // Moderate for knee
    m_makeup->setSampleRate(sampleRate, 20.0f);       // Moderate for makeup
    m_saturation->setSampleRate(sampleRate, 30.0f);   // Moderate for saturation
    m_mix->setSampleRate(sampleRate, 20.0f);          // Moderate for mix
    m_stereoLink->setSampleRate(sampleRate, 50.0f);   // Slow for stereo link
    m_truePeak->setSampleRate(sampleRate, 100.0f);    // Very slow for true peak mode
    
    // Initialize all DSP components
    for (int ch = 0; ch < 2; ++ch) {
        // Lookahead buffers
        m_lookaheadBuffers[ch].prepare(MAX_LOOKAHEAD_SAMPLES);
        
        // True peak detectors
        m_truePeakDetectors[ch].reset();
        
        // Envelope followers
        m_envelopeFollowers[ch].setSampleRate(sampleRate);
        
        // DC blockers (20Hz high-pass)
        m_dcBlockers[ch].setCutoff(20.0, sampleRate);
        
        // Saturators
        m_saturators[ch].setDrive(0.0f);
        m_saturators[ch].setAsymmetry(0.0f);
        
        // Oversampling
        m_oversamplers[ch].prepare(samplesPerBlock, sampleRate);
    }
    
    // Configure gain computer
    m_gainComputer.setKneeWidth(2.0f); // 2dB soft knee width
    
    reset();
}

void MasteringLimiter::reset() {
    // Reset all DSP components
    for (int ch = 0; ch < 2; ++ch) {
        m_lookaheadBuffers[ch].reset();
        m_truePeakDetectors[ch].reset();
        m_envelopeFollowers[ch].reset();
        m_dcBlockers[ch].reset();
        m_oversamplers[ch].reset();
        
        m_currentGain[ch] = 1.0f;
        m_grMeter[ch] = 0.0f;
    }
    
    // Clear work buffers
    for (auto& buffer : m_oversampledBuffers) {
        buffer.fill(0.0f);
    }
}

void MasteringLimiter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Process stereo or mono
    if (numChannels == 1) {
        // Mono processing
        float* data = buffer.getWritePointer(0);
        processStereo(data, data, numSamples);
    } else {
        // Stereo processing
        float* left = buffer.getWritePointer(0);
        float* right = numChannels > 1 ? buffer.getWritePointer(1) : left;
        processStereo(left, right, numSamples);
    }
}

void MasteringLimiter::processStereo(float* left, float* right, int numSamples) {
    // Update all parameters once per block
    float threshold = m_threshold->process();
    float ceiling = m_ceiling->process();
    float release = m_release->process();
    float lookahead = m_lookahead->process();
    float knee = m_knee->process();
    float makeup = m_makeup->process();
    float saturation = m_saturation->process();
    float mix = m_mix->process();
    float stereoLink = m_stereoLink->process();
    float truePeakMode = m_truePeak->process();
    
    // Convert parameters to usable values
    float thresholdLinear = dbToLinear(threshold);
    float ceilingLinear = dbToLinear(ceiling);
    float makeupLinear = dbToLinear(makeup);
    
    // Calculate lookahead samples
    int lookaheadSamples = static_cast<int>(lookahead * 0.001f * m_sampleRate);
    lookaheadSamples = std::min(lookaheadSamples, MAX_LOOKAHEAD_SAMPLES);
    
    // Update lookahead delay
    for (int ch = 0; ch < 2; ++ch) {
        m_lookaheadBuffers[ch].setDelay(lookaheadSamples);
    }
    
    // Update envelope followers release time
    for (int ch = 0; ch < 2; ++ch) {
        m_envelopeFollowers[ch].setReleaseTime(release, m_sampleRate);
    }
    
    // Update saturators
    for (int ch = 0; ch < 2; ++ch) {
        m_saturators[ch].setDrive(saturation);
    }
    
    // Determine if we need oversampling
    bool useOversampling = m_useOversampling && (truePeakMode > 0.5f);
    
    // Process each channel
    float* channelData[2] = {left, right};
    
    if (useOversampling) {
        // Process with oversampling for true peak compliance
        for (int ch = 0; ch < 2; ++ch) {
            float* data = channelData[ch];
            float* oversampledData = m_oversampledBuffers[ch].data();
            
            // Store dry signal
            std::vector<float> drySignal(data, data + numSamples);
            
            // Upsample to 8x
            m_oversamplers[ch].processUpsample(data, oversampledData, numSamples);
            
            // Process at 8x sample rate
            int oversampledSamples = numSamples * OVERSAMPLE_FACTOR;
            
            for (int i = 0; i < oversampledSamples; ++i) {
                float input = oversampledData[i];
                
                // DC blocking
                input = m_dcBlockers[ch].process(input);
                
                // Lookahead delay
                float delayed = m_lookaheadBuffers[ch].process(input);
                
                // True peak detection
                float truePeak = m_truePeakDetectors[ch].detectTruePeak(input);
                
                // Envelope following with adaptive release
                float envelope = m_envelopeFollowers[ch].process(truePeak, 
                    m_releaseMode == ReleaseMode::ADAPTIVE);
                
                // Calculate gain reduction
                float inputDb = linearToDb(envelope);
                float gainReductionDb = m_gainComputer.computeGain(
                    inputDb, threshold, 1000.0f, knee > 0.1f);
                
                // Apply ceiling
                float outputLevel = envelope * dbToLinear(gainReductionDb);
                if (outputLevel > ceilingLinear) {
                    float additionalReduction = ceilingLinear / outputLevel;
                    gainReductionDb += linearToDb(additionalReduction);
                }
                
                // Smooth gain changes
                float targetGain = dbToLinear(gainReductionDb);
                float gainDiff = targetGain - m_currentGain[ch];
                m_currentGain[ch] += gainDiff * 0.01f; // Smooth transition
                
                // Apply gain reduction to delayed signal
                float limited = delayed * m_currentGain[ch];
                
                // Apply makeup gain
                limited *= makeupLinear;
                
                // Optional saturation for analog warmth
                if (saturation > 0.01f) {
                    limited = m_saturators[ch].process(limited);
                }
                
                // Final safety limiting
                if (std::abs(limited) > ceilingLinear) {
                    limited = std::copysign(ceilingLinear, limited);
                }
                
                oversampledData[i] = limited;
                
                // Update GR meter
                m_grMeter[ch] = m_currentGain[ch] * 0.001f + m_grMeter[ch] * 0.999f;
            }
            
            // Downsample back to original rate
            m_oversamplers[ch].processDownsample(oversampledData, numSamples);
            
            // Mix dry/wet
            for (int i = 0; i < numSamples; ++i) {
                data[i] = oversampledData[i] * mix + drySignal[i] * (1.0f - mix);
            }
        }
        
    } else {
        // Non-oversampled processing (lower quality but lower CPU)
        
        // Stereo linking detection
        std::vector<float> linkedEnvelope(numSamples);
        
        if (stereoLink > 0.01f) {
            // Calculate linked detection signal
            for (int i = 0; i < numSamples; ++i) {
                float leftLevel = std::abs(left[i]);
                float rightLevel = std::abs(right[i]);
                linkedEnvelope[i] = std::max(leftLevel, rightLevel) * stereoLink +
                                   ((leftLevel + rightLevel) * 0.5f) * (1.0f - stereoLink);
            }
        }
        
        for (int ch = 0; ch < 2; ++ch) {
            float* data = channelData[ch];
            
            // Store dry signal
            std::vector<float> drySignal(data, data + numSamples);
            
            for (int i = 0; i < numSamples; ++i) {
                float input = data[i];
                
                // DC blocking
                input = m_dcBlockers[ch].process(input);
                
                // Lookahead delay
                float delayed = m_lookaheadBuffers[ch].process(input);
                
                // Detection signal (use linked if enabled)
                float detectionSignal = stereoLink > 0.01f ? 
                    linkedEnvelope[i] : std::abs(input);
                
                // Envelope following
                float envelope = m_envelopeFollowers[ch].process(detectionSignal,
                    m_releaseMode == ReleaseMode::ADAPTIVE);
                
                // Calculate gain reduction
                float inputDb = linearToDb(envelope);
                float gainReductionDb = m_gainComputer.computeGain(
                    inputDb, threshold, 1000.0f, knee > 0.1f);
                
                // Apply ceiling
                float outputLevel = envelope * dbToLinear(gainReductionDb);
                if (outputLevel > ceilingLinear) {
                    float additionalReduction = ceilingLinear / outputLevel;
                    gainReductionDb += linearToDb(additionalReduction);
                }
                
                // Smooth gain changes (faster in non-oversampled mode)
                float targetGain = dbToLinear(gainReductionDb);
                float gainDiff = targetGain - m_currentGain[ch];
                m_currentGain[ch] += gainDiff * 0.1f;
                
                // Apply gain reduction
                float limited = delayed * m_currentGain[ch];
                
                // Apply makeup gain
                limited *= makeupLinear;
                
                // Optional saturation
                if (saturation > 0.01f) {
                    limited = m_saturators[ch].process(limited);
                }
                
                // Final safety limiting
                if (std::abs(limited) > ceilingLinear) {
                    limited = std::copysign(ceilingLinear, limited);
                }
                
                // Mix dry/wet
                data[i] = limited * mix + drySignal[i] * (1.0f - mix);
                
                // Update GR meter
                m_grMeter[ch] = m_currentGain[ch] * 0.001f + m_grMeter[ch] * 0.999f;
            }
        }
    }
    
    // Apply Mid/Side processing if requested
    if (m_stereoMode == StereoMode::MID_SIDE) {
        // Convert to M/S
        for (int i = 0; i < numSamples; ++i) {
            float mid = (left[i] + right[i]) * 0.5f;
            float side = (left[i] - right[i]) * 0.5f;
            
            // Process M/S separately would go here...
            
            // Convert back to L/R
            left[i] = mid + side;
            right[i] = mid - side;
        }
    }
}

void MasteringLimiter::updateParameters(const std::map<int, float>& params) {
    // Update parameter targets
    auto it = params.find(0);
    if (it != params.end()) {
        // Threshold: -60 to 0 dB
        float normalized = std::clamp(it->second, 0.0f, 1.0f);
        m_threshold->setTarget(-60.0f + normalized * 60.0f);
    }
    
    it = params.find(1);
    if (it != params.end()) {
        // Ceiling: -3 to 0 dB
        float normalized = std::clamp(it->second, 0.0f, 1.0f);
        m_ceiling->setTarget(-3.0f + normalized * 3.0f);
    }
    
    it = params.find(2);
    if (it != params.end()) {
        // Release: 10 to 2500 ms (logarithmic)
        float normalized = std::clamp(it->second, 0.0f, 1.0f);
        float releaseMs = 10.0f * std::pow(250.0f, normalized);
        m_release->setTarget(releaseMs);
    }
    
    it = params.find(3);
    if (it != params.end()) {
        // Lookahead: 0 to 10 ms
        float normalized = std::clamp(it->second, 0.0f, 1.0f);
        m_lookahead->setTarget(normalized * 10.0f);
    }
    
    it = params.find(4);
    if (it != params.end()) {
        // Knee: 0 to 1 (hard to soft)
        m_knee->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    }
    
    it = params.find(5);
    if (it != params.end()) {
        // Makeup gain: -12 to +12 dB
        float normalized = std::clamp(it->second, 0.0f, 1.0f);
        m_makeup->setTarget(-12.0f + normalized * 24.0f);
    }
    
    it = params.find(6);
    if (it != params.end()) {
        // Saturation: 0 to 1
        m_saturation->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    }
    
    it = params.find(7);
    if (it != params.end()) {
        // Stereo link: 0 to 1
        m_stereoLink->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    }
    
    it = params.find(8);
    if (it != params.end()) {
        // True peak mode: 0 or 1
        m_truePeak->setTarget(it->second > 0.5f ? 1.0f : 0.0f);
    }
    
    it = params.find(9);
    if (it != params.end()) {
        // Mix: 0 to 1
        m_mix->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    }
}

juce::String MasteringLimiter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Threshold";
        case 1: return "Ceiling";
        case 2: return "Release";
        case 3: return "Lookahead";
        case 4: return "Knee";
        case 5: return "Makeup";
        case 6: return "Saturation";
        case 7: return "Stereo Link";
        case 8: return "True Peak";
        case 9: return "Mix";
        default: return "";
    }
}
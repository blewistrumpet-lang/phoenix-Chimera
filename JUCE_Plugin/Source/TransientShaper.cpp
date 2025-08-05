// TransientShaper.cpp - Professional Studio-Quality Implementation
#include "TransientShaper.h"
#include <cmath>
#include <algorithm>

TransientShaper::TransientShaper() {
    // Initialize all parameter smoothers
    m_attack = std::make_unique<ParameterSmoother>();
    m_sustain = std::make_unique<ParameterSmoother>();
    m_attackTime = std::make_unique<ParameterSmoother>();
    m_releaseTime = std::make_unique<ParameterSmoother>();
    m_separation = std::make_unique<ParameterSmoother>();
    m_threshold = std::make_unique<ParameterSmoother>();
    m_knee = std::make_unique<ParameterSmoother>();
    m_lookahead = std::make_unique<ParameterSmoother>();
    m_detection = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
    
    // Set professional defaults
    m_attack->reset(0.5f);         // Neutral attack
    m_sustain->reset(0.5f);        // Neutral sustain  
    m_attackTime->reset(0.1f);     // 1ms attack time
    m_releaseTime->reset(0.3f);    // 30ms release time
    m_separation->reset(0.7f);     // 70% separation
    m_threshold->reset(0.5f);      // -6dB threshold
    m_knee->reset(0.2f);          // 20% knee width
    m_lookahead->reset(0.1f);      // 1ms lookahead
    m_detection->reset(0.75f);     // Hybrid detection
    m_mix->reset(1.0f);           // 100% wet
}

void TransientShaper::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Configure parameter smoothers with appropriate rates
    m_attack->setSampleRate(sampleRate, 20.0f);       // Fast for attack
    m_sustain->setSampleRate(sampleRate, 20.0f);      // Fast for sustain
    m_attackTime->setSampleRate(sampleRate, 30.0f);   // Moderate for timing
    m_releaseTime->setSampleRate(sampleRate, 30.0f);  // Moderate for timing
    m_separation->setSampleRate(sampleRate, 30.0f);   // Moderate for separation
    m_threshold->setSampleRate(sampleRate, 30.0f);    // Moderate for threshold
    m_knee->setSampleRate(sampleRate, 40.0f);         // Slow for knee
    m_lookahead->setSampleRate(sampleRate, 40.0f);    // Slow for lookahead
    m_detection->setSampleRate(sampleRate, 50.0f);    // Very slow for detection mode
    m_mix->setSampleRate(sampleRate, 20.0f);          // Fast for mix
    
    // Initialize all DSP components
    for (int ch = 0; ch < 2; ++ch) {
        // Transient separators
        m_separators[ch].prepare(sampleRate);
        
        // Envelope detectors
        m_detectors[ch].prepare(sampleRate);
        
        // Lookahead processors
        m_lookaheadProcessors[ch].prepare(LOOKAHEAD_SAMPLES);
        
        // Soft knee processors
        m_kneeProcessors[ch].setThreshold(0.5f);
        m_kneeProcessors[ch].setKnee(0.2f);
        
        // Oversampling
        m_oversamplers[ch].prepare(sampleRate);
    }
    
    reset();
}

void TransientShaper::reset() {
    // Reset all DSP components
    for (int ch = 0; ch < 2; ++ch) {
        m_separators[ch].reset();
        m_detectors[ch].reset();
        m_lookaheadProcessors[ch].reset();
        m_oversamplers[ch].reset();
    }
    
    // Clear work buffers
    for (auto& buffer : m_oversampledBuffers) {
        buffer.fill(0.0f);
    }
}

void TransientShaper::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Process stereo or mono
    if (numChannels == 1) {
        // Mono processing - duplicate to stereo for consistent processing
        float* data = buffer.getWritePointer(0);
        processStereo(data, data, numSamples);
    } else {
        // Stereo processing
        float* left = buffer.getWritePointer(0);
        float* right = numChannels > 1 ? buffer.getWritePointer(1) : left;
        processStereo(left, right, numSamples);
    }
}

void TransientShaper::processStereo(float* left, float* right, int numSamples) {
    // Update all parameters once per block
    float attack = m_attack->process();
    float sustain = m_sustain->process();
    float attackTime = m_attackTime->process();
    float releaseTime = m_releaseTime->process();
    float separation = m_separation->process();
    float threshold = m_threshold->process();
    float knee = m_knee->process();
    float lookaheadParam = m_lookahead->process();
    float detectionParam = m_detection->process();
    float mix = m_mix->process();
    
    // Convert parameters to usable values
    float attackGain = 0.1f + attack * 3.9f;      // 0.1 to 4.0 (up to +12dB)
    float sustainGain = 0.1f + sustain * 3.9f;    // 0.1 to 4.0 (up to +12dB)
    float attackTimeMs = 0.1f + attackTime * 49.9f;  // 0.1ms to 50ms
    float releaseTimeMs = 1.0f + releaseTime * 499.0f; // 1ms to 500ms
    int lookaheadSamples = static_cast<int>(lookaheadParam * LOOKAHEAD_SAMPLES);
    
    // Determine detection mode
    m_detectionMode = getDetectionModeFromParam(detectionParam);
    
    // Configure components for this block
    for (int ch = 0; ch < 2; ++ch) {
        // Set separator parameters
        m_separators[ch].setGains(attackGain, sustainGain);
        m_separators[ch].setSeparation(separation);
        
        // Set detector timing
        m_detectors[ch].setTimes(attackTimeMs, releaseTimeMs, m_sampleRate);
        
        // Set lookahead delay
        m_lookaheadProcessors[ch].setDelay(lookaheadSamples);
        
        // Set soft knee parameters
        m_kneeProcessors[ch].setThreshold(threshold);
        m_kneeProcessors[ch].setKnee(knee);
        m_kneeProcessors[ch].setRatio(2.0f + separation * 8.0f); // 2:1 to 10:1 based on separation
    }
    
    // Process each channel
    float* channelData[2] = {left, right};
    
    if (m_useOversampling && (attackGain > 2.0f || sustainGain > 2.0f)) {
        // Use oversampling when heavy processing to prevent aliasing
        for (int ch = 0; ch < 2; ++ch) {
            float* data = channelData[ch];
            float* oversampledData = m_oversampledBuffers[ch].data();
            
            // Store dry signal
            std::vector<float> drySignal(data, data + numSamples);
            
            // Upsample to 4x
            m_oversamplers[ch].processUpsample(data, oversampledData, numSamples);
            
            // Process at 4x sample rate
            int oversampledSamples = numSamples * OVERSAMPLE_FACTOR;
            
            for (int i = 0; i < oversampledSamples; ++i) {
                float input = oversampledData[i];
                
                // Apply lookahead if enabled
                float lookaheadInput = input;
                if (lookaheadSamples > 0) {
                    lookaheadInput = m_lookaheadProcessors[ch].process(input);
                    
                    // Peek ahead for transient detection
                    float peekAhead = 0.0f;
                    for (int j = 0; j < std::min(8, lookaheadSamples); ++j) {
                        float peek = m_lookaheadProcessors[ch].peek(j);
                        peekAhead = std::max(peekAhead, std::abs(peek));
                    }
                    
                    // Use peak-ahead for better transient detection
                    if (peekAhead > std::abs(lookaheadInput)) {
                        lookaheadInput = lookaheadInput * (peekAhead / (std::abs(lookaheadInput) + 0.001f));
                    }
                }
                
                // Detect envelope based on mode
                float envelope = 0.0f;
                switch (m_detectionMode) {
                    case DetectionMode::PEAK:
                        envelope = m_detectors[ch].processPeak(lookaheadInput);
                        break;
                    case DetectionMode::RMS:
                        envelope = m_detectors[ch].processRMS(lookaheadInput);
                        break;
                    case DetectionMode::SPECTRAL:
                        envelope = m_detectors[ch].processSpectralFlux(lookaheadInput);
                        break;
                    case DetectionMode::HYBRID:
                    default:
                        // Combine peak and RMS for hybrid detection
                        float peakEnv = m_detectors[ch].processPeak(lookaheadInput);
                        float rmsEnv = m_detectors[ch].processRMS(lookaheadInput);
                        envelope = peakEnv * 0.7f + rmsEnv * 0.3f;
                        break;
                }
                
                // Process through transient separator
                float processed = m_separators[ch].process(input);
                
                // Apply soft knee processing for smooth transitions
                processed = m_kneeProcessors[ch].process(processed);
                
                // Soft saturation to prevent harsh clipping
                if (std::abs(processed) > 0.9f) {
                    processed = std::tanh(processed * 1.1f) * 0.909f;
                }
                
                oversampledData[i] = processed;
            }
            
            // Downsample back to original rate
            m_oversamplers[ch].processDownsample(oversampledData, numSamples);
            
            // Mix dry/wet
            for (int i = 0; i < numSamples; ++i) {
                data[i] = oversampledData[i] * mix + drySignal[i] * (1.0f - mix);
            }
        }
        
    } else {
        // Non-oversampled processing (lower CPU usage)
        for (int ch = 0; ch < 2; ++ch) {
            float* data = channelData[ch];
            
            // Store dry signal
            std::vector<float> drySignal(data, data + numSamples);
            
            for (int i = 0; i < numSamples; ++i) {
                float input = data[i];
                
                // Apply lookahead if enabled
                float lookaheadInput = input;
                if (lookaheadSamples > 0) {
                    lookaheadInput = m_lookaheadProcessors[ch].process(input);
                }
                
                // Detect envelope based on mode
                float envelope = 0.0f;
                switch (m_detectionMode) {
                    case DetectionMode::PEAK:
                        envelope = m_detectors[ch].processPeak(lookaheadInput);
                        break;
                    case DetectionMode::RMS:
                        envelope = m_detectors[ch].processRMS(lookaheadInput);
                        break;
                    case DetectionMode::SPECTRAL:
                        envelope = m_detectors[ch].processSpectralFlux(lookaheadInput);
                        break;
                    case DetectionMode::HYBRID:
                    default:
                        float peakEnv = m_detectors[ch].processPeak(lookaheadInput);
                        float rmsEnv = m_detectors[ch].processRMS(lookaheadInput);
                        envelope = peakEnv * 0.7f + rmsEnv * 0.3f;
                        break;
                }
                
                // Process through transient separator
                float processed = m_separators[ch].process(input);
                
                // Apply soft knee processing
                processed = m_kneeProcessors[ch].process(processed);
                
                // Soft saturation
                if (std::abs(processed) > 0.9f) {
                    processed = std::tanh(processed * 1.1f) * 0.909f;
                }
                
                // Mix dry/wet
                data[i] = processed * mix + drySignal[i] * (1.0f - mix);
            }
        }
    }
    
    // Apply subtle stereo enhancement for width
    if (separation > 0.7f) {
        float width = (separation - 0.7f) * 0.1f; // Very subtle width enhancement
        for (int i = 0; i < numSamples; ++i) {
            float mid = (left[i] + right[i]) * 0.5f;
            float side = (left[i] - right[i]) * 0.5f;
            
            // Enhance transients in the side channel
            side *= (1.0f + width * 2.0f);
            
            left[i] = mid + side;
            right[i] = mid - side;
        }
    }
}

TransientShaper::DetectionMode TransientShaper::getDetectionModeFromParam(float param) const {
    if (param < 0.25f) return DetectionMode::PEAK;
    else if (param < 0.5f) return DetectionMode::RMS;
    else if (param < 0.75f) return DetectionMode::SPECTRAL;
    else return DetectionMode::HYBRID;
}

void TransientShaper::updateParameters(const std::map<int, float>& params) {
    // Update parameter targets
    auto it = params.find(0);
    if (it != params.end()) m_attack->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(1);
    if (it != params.end()) m_sustain->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(2);
    if (it != params.end()) m_attackTime->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(3);
    if (it != params.end()) m_releaseTime->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(4);
    if (it != params.end()) m_separation->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(5);
    if (it != params.end()) m_threshold->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(6);
    if (it != params.end()) m_knee->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(7);
    if (it != params.end()) m_lookahead->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(8);
    if (it != params.end()) m_detection->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(9);
    if (it != params.end()) m_mix->setTarget(std::clamp(it->second, 0.0f, 1.0f));
}

juce::String TransientShaper::getParameterName(int index) const {
    switch (index) {
        case 0: return "Attack";
        case 1: return "Sustain";
        case 2: return "Attack Time";
        case 3: return "Release Time";
        case 4: return "Separation";
        case 5: return "Threshold";
        case 6: return "Knee";
        case 7: return "Lookahead";
        case 8: return "Detection";
        case 9: return "Mix";
        default: return "";
    }
}
// StateVariableFilter.cpp - Professional Studio-Quality Implementation
#include "StateVariableFilter.h"
#include <cmath>
#include <algorithm>

StateVariableFilter::StateVariableFilter() {
    // Initialize all parameter smoothers
    m_frequency = std::make_unique<ParameterSmoother>();
    m_resonance = std::make_unique<ParameterSmoother>();
    m_drive = std::make_unique<ParameterSmoother>();
    m_filterType = std::make_unique<ParameterSmoother>();
    m_slope = std::make_unique<ParameterSmoother>();
    m_envelope = std::make_unique<ParameterSmoother>();
    m_envAttack = std::make_unique<ParameterSmoother>();
    m_envRelease = std::make_unique<ParameterSmoother>();
    m_analog = std::make_unique<ParameterSmoother>();
    m_mix = std::make_unique<ParameterSmoother>();
    
    // Set professional defaults
    m_frequency->reset(0.5f);      // Mid frequency
    m_resonance->reset(0.2f);      // Moderate resonance
    m_drive->reset(0.0f);          // No drive initially
    m_filterType->reset(0.0f);     // Lowpass
    m_slope->reset(0.25f);         // 12dB/oct (2 stages)
    m_envelope->reset(0.0f);       // No envelope following
    m_envAttack->reset(0.1f);      // 10ms attack
    m_envRelease->reset(0.3f);     // 100ms release
    m_analog->reset(0.2f);         // Subtle analog character
    m_mix->reset(1.0f);           // 100% wet
}

void StateVariableFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Configure parameter smoothers with appropriate rates
    m_frequency->setSampleRate(sampleRate, 20.0f);    // Fast for frequency
    m_resonance->setSampleRate(sampleRate, 30.0f);    // Moderate for resonance
    m_drive->setSampleRate(sampleRate, 30.0f);        // Moderate for drive
    m_filterType->setSampleRate(sampleRate, 50.0f);   // Slow for type changes
    m_slope->setSampleRate(sampleRate, 50.0f);        // Slow for slope changes
    m_envelope->setSampleRate(sampleRate, 30.0f);     // Moderate for envelope
    m_envAttack->setSampleRate(sampleRate, 40.0f);    // Moderate for attack
    m_envRelease->setSampleRate(sampleRate, 40.0f);   // Moderate for release
    m_analog->setSampleRate(sampleRate, 50.0f);       // Slow for analog amount
    m_mix->setSampleRate(sampleRate, 20.0f);          // Fast for mix
    
    // Initialize all DSP components
    for (int ch = 0; ch < 2; ++ch) {
        // Multi-mode filters
        m_filters[ch].prepare(sampleRate);
        
        // Envelope followers
        m_envelopeFollowers[ch].setTimes(10.0f, 100.0f, sampleRate);
        
        // Analog modeling
        m_analogModels[ch].setAnalogAmount(0.2f);
        
        // Oversampling
        m_oversamplers[ch].prepare(sampleRate);
    }
    
    reset();
}

void StateVariableFilter::reset() {
    // Reset all DSP components
    for (int ch = 0; ch < 2; ++ch) {
        m_filters[ch].reset();
        m_envelopeFollowers[ch].reset();
        m_oversamplers[ch].reset();
    }
    
    // Clear work buffers
    for (auto& buffer : m_oversampledBuffers) {
        buffer.fill(0.0f);
    }
}

void StateVariableFilter::process(juce::AudioBuffer<float>& buffer) {
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

void StateVariableFilter::processStereo(float* left, float* right, int numSamples) {
    // Update all parameters once per block
    float frequency = m_frequency->process();
    float resonance = m_resonance->process();
    float drive = m_drive->process();
    float filterTypeParam = m_filterType->process();
    float slopeParam = m_slope->process();
    float envelopeAmount = m_envelope->process();
    float envAttack = m_envAttack->process();
    float envRelease = m_envRelease->process();
    float analogAmount = m_analog->process();
    float mix = m_mix->process();
    
    // Convert parameters to usable values
    float baseFreq = 20.0f * std::pow(1000.0f, frequency); // 20Hz to 20kHz
    float q = 0.5f + resonance * 19.5f; // 0.5 to 20
    float driveAmount = 1.0f + drive * 9.0f; // 1 to 10x gain
    
    // Determine filter type and morphing
    FilterType currentType = getFilterTypeFromParam(filterTypeParam);
    float morphAmount = 0.0f;
    
    if (currentType == FilterType::MORPHING) {
        morphAmount = filterTypeParam * 4.0f; // 0-4 for full morph range
        while (morphAmount > 1.0f) morphAmount -= 1.0f;
    }
    
    // Set number of filter stages based on slope parameter
    int numStages = 1;
    if (slopeParam < 0.25f) numStages = 1;      // 6dB/oct
    else if (slopeParam < 0.5f) numStages = 2;  // 12dB/oct
    else if (slopeParam < 0.75f) numStages = 3; // 18dB/oct
    else numStages = 4;                         // 24dB/oct
    
    // Configure envelope followers
    float attackMs = 0.1f + envAttack * 99.9f;  // 0.1ms to 100ms
    float releaseMs = 1.0f + envRelease * 999.0f; // 1ms to 1000ms
    
    for (int ch = 0; ch < 2; ++ch) {
        m_envelopeFollowers[ch].setTimes(attackMs, releaseMs, m_sampleRate);
        m_analogModels[ch].setAnalogAmount(analogAmount);
        m_filters[ch].setNumStages(numStages);
        m_filters[ch].setMode(true); // Serial mode for classic response
    }
    
    // Process each channel
    float* channelData[2] = {left, right};
    
    if (m_useOversampling && drive > 0.1f) {
        // Use oversampling when drive is engaged to prevent aliasing
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
                
                // Apply drive with soft clipping
                if (driveAmount > 1.01f) {
                    input *= driveAmount;
                    // Soft saturation
                    input = std::tanh(input * 0.7f) / 0.7f;
                }
                
                // Get envelope value for modulation
                float envelope = 0.0f;
                if (envelopeAmount > 0.01f) {
                    envelope = m_envelopeFollowers[ch].process(input);
                }
                
                // Calculate modulated frequency
                double modulatedFreq = baseFreq;
                double modulatedResonance = q;
                
                if (envelopeAmount > 0.01f) {
                    // Envelope can modulate up to 4 octaves
                    float envMod = envelope * envelopeAmount * 4.0f;
                    modulatedFreq *= std::pow(2.0f, envMod);
                    
                    // Also slightly modulate resonance
                    modulatedResonance *= (1.0f + envelope * envelopeAmount * 0.5f);
                }
                
                // Apply analog modeling
                m_analogModels[ch].modulateParameters(modulatedFreq, modulatedResonance);
                
                // Update filter parameters
                m_filters[ch].updateAllStages(modulatedFreq, modulatedResonance, 
                                             m_sampleRate * OVERSAMPLE_FACTOR,
                                             currentType, morphAmount);
                
                // Process through filter
                float filtered = m_filters[ch].process(input);
                
                // Apply analog saturation
                filtered = m_analogModels[ch].processSaturation(filtered);
                
                // Add analog noise
                if (analogAmount > 0.01f) {
                    float noise = m_analogModels[ch].processNoise();
                    filtered += noise * m_analogModels[ch].m_noiseLevel;
                }
                
                oversampledData[i] = filtered;
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
                
                // Apply drive
                if (driveAmount > 1.01f) {
                    input *= driveAmount;
                    input = std::tanh(input * 0.7f) / 0.7f;
                }
                
                // Get envelope value
                float envelope = 0.0f;
                if (envelopeAmount > 0.01f) {
                    envelope = m_envelopeFollowers[ch].process(input);
                }
                
                // Calculate modulated frequency
                double modulatedFreq = baseFreq;
                double modulatedResonance = q;
                
                if (envelopeAmount > 0.01f) {
                    float envMod = envelope * envelopeAmount * 4.0f;
                    modulatedFreq *= std::pow(2.0f, envMod);
                    modulatedResonance *= (1.0f + envelope * envelopeAmount * 0.5f);
                }
                
                // Apply analog modeling
                m_analogModels[ch].modulateParameters(modulatedFreq, modulatedResonance);
                
                // Update filter (less frequently for efficiency)
                if (i % 8 == 0) {
                    m_filters[ch].updateAllStages(modulatedFreq, modulatedResonance,
                                                 m_sampleRate, currentType, morphAmount);
                }
                
                // Process through filter
                float filtered = m_filters[ch].process(input);
                
                // Apply analog saturation
                filtered = m_analogModels[ch].processSaturation(filtered);
                
                // Add subtle analog noise
                if (analogAmount > 0.01f && (i % 4 == 0)) {
                    float noise = m_analogModels[ch].processNoise();
                    filtered += noise * m_analogModels[ch].m_noiseLevel * 0.5f;
                }
                
                // Mix dry/wet
                data[i] = filtered * mix + drySignal[i] * (1.0f - mix);
            }
        }
    }
    
    // Apply subtle stereo enhancement when using analog modeling
    if (analogAmount > 0.5f) {
        float width = 0.02f * analogAmount; // Very subtle width
        for (int i = 0; i < numSamples; ++i) {
            float mid = (left[i] + right[i]) * 0.5f;
            float side = (left[i] - right[i]) * 0.5f;
            left[i] = mid + side * (1.0f + width);
            right[i] = mid - side * (1.0f + width);
        }
    }
}

StateVariableFilter::FilterType StateVariableFilter::getFilterTypeFromParam(float param) const {
    // Map 0-1 parameter to filter types
    if (param < 0.111f) return FilterType::LOWPASS;
    else if (param < 0.222f) return FilterType::HIGHPASS;
    else if (param < 0.333f) return FilterType::BANDPASS;
    else if (param < 0.444f) return FilterType::NOTCH;
    else if (param < 0.555f) return FilterType::ALLPASS;
    else if (param < 0.666f) return FilterType::PEAK;
    else if (param < 0.777f) return FilterType::LOWSHELF;
    else if (param < 0.888f) return FilterType::HIGHSHELF;
    else return FilterType::MORPHING;
}

void StateVariableFilter::updateParameters(const std::map<int, float>& params) {
    // Update parameter targets
    auto it = params.find(0);
    if (it != params.end()) m_frequency->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(1);
    if (it != params.end()) m_resonance->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(2);
    if (it != params.end()) m_drive->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(3);
    if (it != params.end()) m_filterType->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(4);
    if (it != params.end()) m_slope->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(5);
    if (it != params.end()) m_envelope->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(6);
    if (it != params.end()) m_envAttack->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(7);
    if (it != params.end()) m_envRelease->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(8);
    if (it != params.end()) m_analog->setTarget(std::clamp(it->second, 0.0f, 1.0f));
    
    it = params.find(9);
    if (it != params.end()) m_mix->setTarget(std::clamp(it->second, 0.0f, 1.0f));
}

juce::String StateVariableFilter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Frequency";
        case 1: return "Resonance";
        case 2: return "Drive";
        case 3: return "Filter Type";
        case 4: return "Slope";
        case 5: return "Envelope";
        case 6: return "Env Attack";
        case 7: return "Env Release";
        case 8: return "Analog";
        case 9: return "Mix";
        default: return "";
    }
}
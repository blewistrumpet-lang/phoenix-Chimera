#include "ResonantChorus.h"
#include <algorithm>
#include <random>

ResonantChorus::ResonantChorus() {
    // Initialize smoothed parameters with boutique defaults
    m_mode.reset(0.25f);
    m_depth.reset(0.5f);
    m_resonance.reset(0.3f);
    m_brightness.reset(0.5f);
    m_stereoWidth.reset(0.8f);
    m_warmth.reset(0.5f);
    m_mix.reset(0.5f);
    m_level.reset(0.5f);
}

void ResonantChorus::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set smoothing times for parameters
    float smoothingTime = 50.0f;  // 50ms for most parameters
    m_mode.setSmoothingTime(200.0f, sampleRate);      // Slower for mode changes
    m_depth.setSmoothingTime(smoothingTime, sampleRate);
    m_resonance.setSmoothingTime(smoothingTime, sampleRate);
    m_brightness.setSmoothingTime(smoothingTime, sampleRate);
    m_stereoWidth.setSmoothingTime(smoothingTime, sampleRate);
    m_warmth.setSmoothingTime(smoothingTime, sampleRate);
    m_mix.setSmoothingTime(30.0f, sampleRate);         // Faster for mix
    m_level.setSmoothingTime(30.0f, sampleRate);       // Faster for level
    
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
        
        // Initialize filters with boutique characteristics
        channel.inputFilter.calculateCoefficients(8000.0, 0.3, sampleRate);
        channel.outputFilter.calculateCoefficients(10000.0, 0.2, sampleRate);
    }

void ResonantChorus::reset() {
    // Reset modulation state
    m_lfoPhase = 0.0f;
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
}

    
    // Initialize component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
    
    // Set initial mode
    updateMode(0);
}

void ResonantChorus::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    updateAllSmoothParams();
    
    // Update component aging (very slow)
    updateComponentAging();
    
    // Determine current mode
    int newMode = static_cast<int>(m_mode.current * 3.99f);
    newMode = std::clamp(newMode, 0, 3);
    
    if (newMode != m_currentMode) {
        updateMode(newMode);
        m_currentMode = newMode;
    }
    
    // Update thermal models
    for (auto& channel : m_channelStates) {
        if (m_enableThermalModeling) {
            channel.thermalModel.update(m_sampleRate);
        }
        if (m_enableComponentAging) {
            channel.componentAging.update(m_componentAge);
        }
    }
    
    // Update filter parameters with thermal and aging effects
    for (auto& channel : m_channelStates) {
        float thermalFactor = m_enableThermalModeling ? channel.thermalModel.getThermalFactor() : 1.0f;
        
        // Apply thermal drift to filter frequencies
        float brightFreq = (6000.0f + m_brightness.current * 8000.0f) * thermalFactor;
        float warmFreq = (80.0f + m_warmth.current * 220.0f) * thermalFactor;
        
        // Apply component aging to resonance
        float resonance = m_enableComponentAging ? 
            channel.componentAging.applyDrift(m_resonance.current) : m_resonance.current;
        
        channel.inputFilter.calculateCoefficients(brightFreq, resonance * 0.5f, m_sampleRate);
        channel.outputFilter.calculateCoefficients(warmFreq, resonance * 0.3f, m_sampleRate);
        
        // Update delay unit parameters with aging effects
        for (auto& unit : channel.delayUnits) {
            float baseDepth = DIMENSION_MODES[m_currentMode].modDepths[&unit - &channel.delayUnits[0]] * m_depth.current * 2.0f;
            unit.modDepth = m_enableComponentAging ? 
                channel.componentAging.applyTolerance(baseDepth) : baseDepth;
        }
    }
    
    // Process audio with enhanced quality
    for (int sample = 0; sample < numSamples; ++sample) {
        float outputs[2] = {0.0f, 0.0f};
        
        for (int ch = 0; ch < std::min(numChannels, 2); ++ch) {
            auto& state = m_channelStates[ch];
            float input = buffer.getReadPointer(ch)[sample];
            float dry = input;
            
            // Apply DC blocking to input
            input = state.inputDCBlocker.process(input);
            
            // Apply thermal and aging characteristics
            float thermalFactor = m_enableThermalModeling ? state.thermalModel.getThermalFactor() : 1.0f;
            input = applyAnalogCharacter(input, thermalFactor, m_componentAge);
            
            // Input filtering for character with thermal drift
            float filtered = state.inputFilter.process(input);
            
            // Process through delay units
            float wetSum = 0.0f;
            const auto& mode = DIMENSION_MODES[m_currentMode];
            
            for (int i = 0; i < NUM_DELAY_LINES; ++i) {
                float delayed = state.delayUnits[i].process(filtered, m_sampleRate);
                
                // Apply stereo routing based on mode
                if (numChannels >= 2) {
                    if (mode.stereoConfig[i]) {
                        outputs[0] += delayed * mode.gains[i];
                    } else {
                        outputs[1] += delayed * mode.gains[i];
                    }
                } else {
                    wetSum += delayed;
                }
            }
            
            if (numChannels < 2) {
                // Mono processing
                float wet = state.outputFilter.process(wetSum);
                
                // Apply output DC blocking
                wet = state.outputDCBlocker.process(wet);
                
                // Level compensation with aging
                float compensation = 0.7f + m_level.current * 0.6f;
                if (m_enableComponentAging) {
                    compensation = state.componentAging.applyDrift(compensation);
                }
                wet *= compensation;
                
                // Add subtle noise floor for realism
                wet += state.noiseFloor * ((rand() % 1000) / 1000.0f - 0.5f) * 0.0001f;
                
                outputs[ch] = dry * (1.0f - m_mix.current) + wet * m_mix.current;
            }
        }
        
        // Stereo processing
        if (numChannels >= 2) {
            // Apply warmth filter to outputs
            outputs[0] = m_channelStates[0].outputFilter.process(outputs[0]);
            outputs[1] = m_channelStates[1].outputFilter.process(outputs[1]);
            
            // Enhanced stereo width processing with aging
            processEnhancedStereo(outputs[0], outputs[1], m_stereoWidth.current, m_componentAge);
            
            // Mix with dry and write back
            for (int ch = 0; ch < 2; ++ch) {
                float dry = buffer.getReadPointer(ch)[sample];
                
                // Apply output DC blocking
                auto& state = m_channelStates[ch];
                outputs[ch] = state.outputDCBlocker.process(outputs[ch]);
                
                // Level compensation with aging
                float compensation = 0.7f + m_level.current * 0.6f;
                if (m_enableComponentAging) {
                    compensation = state.componentAging.applyDrift(compensation);
                }
                outputs[ch] *= compensation;
                
                // Add subtle noise floor
                outputs[ch] += state.noiseFloor * ((rand() % 1000) / 1000.0f - 0.5f) * 0.0001f;
                
                buffer.getWritePointer(ch)[sample] = dry * (1.0f - m_mix.current) + outputs[ch] * m_mix.current;
            }
        } else if (numChannels >= 1) {
            buffer.getWritePointer(0)[sample] = outputs[0];
        }
    }
}

void ResonantChorus::updateMode(int mode) {
    const auto& modeData = DIMENSION_MODES[mode];
    
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < NUM_DELAY_LINES; ++i) {
            auto& unit = m_channelStates[ch].delayUnits[i];
            
            unit.baseDelay = modeData.delays[i];
            unit.modDepth = modeData.modDepths[i];
            unit.lfo.rate = modeData.modRates[i];
            unit.lfo.setPhase(modeData.phases[i]);
            unit.gain = modeData.gains[i];
            
            // Initialize filter for each delay line
            float filterFreq = 8000.0f + i * 1000.0f; // Slightly different for each
            // Apply thermal drift to filter frequency
            float thermalDrift = 1.0f;
            if (m_enableThermalModeling) {
                thermalDrift = m_channelStates[0].thermalModel.getThermalFactor();
            }
            
            unit.filter.calculateCoefficients(filterFreq * thermalDrift, 0.2f, m_sampleRate);
        }
    }
}

void ResonantChorus::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_mode.target = params.at(0);
    if (params.count(1)) m_depth.target = params.at(1);
    if (params.count(2)) m_resonance.target = params.at(2);
    if (params.count(3)) m_brightness.target = params.at(3);
    if (params.count(4)) m_stereoWidth.target = params.at(4);
    if (params.count(5)) m_warmth.target = params.at(5);
    if (params.count(6)) m_mix.target = params.at(6);
    if (params.count(7)) m_level.target = params.at(7);
}

juce::String ResonantChorus::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mode";
        case 1: return "Depth";
        case 2: return "Resonance";
        case 3: return "Brightness";
        case 4: return "Width";
        case 5: return "Warmth";
        case 6: return "Mix";
        case 7: return "Level";
        default: return "";
    }
}

// New helper methods for boutique functionality
void ResonantChorus::updateAllSmoothParams() {
    m_mode.update();
    m_depth.update();
    m_resonance.update();
    m_brightness.update();
    m_stereoWidth.update();
    m_warmth.update();
    m_mix.update();
    m_level.update();
}

void ResonantChorus::updateComponentAging() {
    m_sampleCount++;
    if (m_sampleCount > m_sampleRate * 10) { // Every 10 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00005f);
        m_sampleCount = 0;
    }
}

float ResonantChorus::applyAnalogCharacter(float input, float thermalFactor, float aging) {
    // Apply subtle analog nonlinearities
    float output = input;
    
    // Thermal modulation of signal
    output *= thermalFactor;
    
    // Component aging effects
    if (aging > 0.01f) {
        // Subtle saturation from aging components
        float saturation = 1.0f + aging * 0.1f;
        output = std::tanh(output * saturation) / saturation;
        
        // Add subtle harmonic content
        output += aging * 0.02f * input * input * (input > 0 ? 1.0f : -1.0f);
    }
    
    return output;
}

float ResonantChorus::processBBDWithAging(float input, float& clockNoise, float aging, float thermalFactor) {
    // Enhanced BBD modeling with component variations
    float output = input;
    
    // Clock noise increases with aging
    float noiseAmount = 0.0001f * (1.0f + aging * 2.0f);
    clockNoise = (clockNoise * 0.999f) + ((rand() / (float)RAND_MAX) - 0.5f) * noiseAmount;
    output += clockNoise;
    
    // Thermal effects on BBD transfer function
    output *= thermalFactor;
    
    // Component aging affects linearity
    if (aging > 0.05f) {
        float nonlinearity = aging * 0.05f;
        output = output + nonlinearity * output * output * output;
    }
    
    return output;
}

void ResonantChorus::processEnhancedStereo(float& left, float& right, float width, float aging) {
    // Calculate mid and side components
    float mid = (left + right) * 0.5f;
    float side = (left - right) * 0.5f;
    
    // Apply width with aging compensation
    float effectiveWidth = width;
    if (aging > 0.1f) {
        // Stereo width decreases slightly with aging
        effectiveWidth *= (1.0f - aging * 0.1f);
    }
    
    // Enhanced side signal processing
    side *= (1.0f + effectiveWidth);
    
    // Add subtle phase coherence improvement
    static float phaseCorrection = 0.0f;
    phaseCorrection += (side - phaseCorrection) * 0.1f;
    side = side * 0.95f + phaseCorrection * 0.05f;
    
    // Reconstruct stereo
    left = mid + side;
    right = mid - side;
}
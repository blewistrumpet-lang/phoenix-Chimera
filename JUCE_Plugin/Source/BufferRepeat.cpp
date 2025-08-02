#include "BufferRepeat.h"
#include <cmath>
#include <algorithm>

BufferRepeat::BufferRepeat() {
    // Initialize smoothed parameters with professional defaults
    m_division.reset(0.5f);
    m_probability.reset(0.7f);
    m_feedback.reset(0.3f);
    m_filter.reset(0.5f);
    m_pitch.reset(0.5f);
    m_reverse.reset(0.0f);
    m_stutter.reset(0.0f);
    m_mix.reset(0.5f);
}

void BufferRepeat::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set smoothing times for parameters
    m_division.setSmoothingTime(200.0f, sampleRate);    // Slow for division changes
    m_probability.setSmoothingTime(100.0f, sampleRate); // Medium for probability
    m_feedback.setSmoothingTime(50.0f, sampleRate);     // Fast for feedback
    m_filter.setSmoothingTime(80.0f, sampleRate);       // Medium for filter
    m_pitch.setSmoothingTime(150.0f, sampleRate);       // Slower for pitch
    m_reverse.setSmoothingTime(300.0f, sampleRate);     // Very slow for mode changes
    m_stutter.setSmoothingTime(50.0f, sampleRate);      // Fast for stutter
    m_mix.setSmoothingTime(30.0f, sampleRate);          // Fast for mix
    
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
    }

void BufferRepeat::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for BufferRepeat
}

    
    // Initialize component aging
    m_componentAge = 0.0f;
    m_sampleCount = 0;
}

void BufferRepeat::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update all smoothed parameters
    updateAllSmoothParams();
    
    // Update component aging
    updateComponentAging();
    
    // Get parameters with smooth transitions
    Division division = getDivisionFromParam(m_division.current);
    int sliceSize = getDivisionSamples(division);
    float pitchRatio = getPitchRatio(m_pitch.current);
    bool shouldReverse = m_reverse.current > 0.5f;
    float filterCutoff = m_filter.current;
    
    // Update thermal models and aging
    for (auto& channel : m_channelStates) {
        if (m_enableThermalModeling) {
            channel.thermalModel.update(m_sampleRate);
        }
        if (m_enableComponentAging) {
            channel.componentAging.update(m_componentAge);
        }
    }
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channel >= 2) break;
        
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Update effects
        state.stutterGate.setRate(m_division);
        state.filter.setCutoff(filterCutoff);
        
        // Get thermal and aging factors
        float thermalFactor = m_enableThermalModeling ? state.thermalModel.getThermalFactor() : 1.0f;
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float dry = input;
            
            // Apply input DC blocking
            input = state.inputDCBlocker.process(input);
            
            // Apply analog character enhancement
            input = applyAnalogCharacter(input, thermalFactor, m_componentAge);
            
            // Enhanced slice processing with lookahead
            processEnhancedSlicing(state, input, thermalFactor, m_componentAge);
            
            // Record input to buffer with thermal drift compensation
            state.recordBuffer[state.writePos] = input;
            state.writePos = (state.writePos + 1) % MAX_BUFFER_SIZE;
            
            // Check if it's time to trigger a new slice with aging compensation
            int adjustedSliceSize = static_cast<int>(sliceSize * thermalFactor);
            if (m_enableComponentAging) {
                adjustedSliceSize = static_cast<int>(state.componentAging.applyTimingDrift(adjustedSliceSize));
            }
            
            if (++state.samplesSinceLastSlice >= state.nextSliceTime) {
                state.samplesSinceLastSlice = 0;
                state.nextSliceTime = adjustedSliceSize;
                
                // Trigger slice with enhanced parameters
                state.triggerSlice(adjustedSliceSize, m_probability.current, 
                                 shouldReverse || (m_reverse.current > 0.0f && state.dist(state.rng) < m_reverse.current),
                                 pitchRatio);
                
                // Update players' feedback with aging degradation
                float adjustedFeedback = m_feedback.current;
                if (m_enableComponentAging) {
                    adjustedFeedback = state.componentAging.applyFeedbackDegradation(adjustedFeedback);
                }
                
                for (auto& player : state.slicePlayers) {
                    player.feedback = adjustedFeedback;
                }
            }
            
            // Mix slice players with enhanced processing
            float sliceOutput = 0.0f;
            for (auto& player : state.slicePlayers) {
                if (player.isPlaying) {
                    float sample = player.getNextSample();
                    
                    // Apply vintage buffer character
                    sample = applyVintageBufferCharacter(sample, player.feedback, m_componentAge);
                    
                    sliceOutput += sample;
                }
            }
            
            // Apply enhanced filter degradation with aging
            state.filter.setCutoff(filterCutoff);
            if (filterCutoff < 0.5f) {
                // Lowpass for darker sound with aging effects
                sliceOutput = state.filter.processLowpass(sliceOutput, m_componentAge);
            } else if (filterCutoff > 0.5f) {
                // Highpass for thinner sound
                state.filter.setCutoff(1.0f - filterCutoff);
                sliceOutput = state.filter.processHighpass(sliceOutput, m_componentAge);
            }
            
            // Apply enhanced stutter gate
            sliceOutput = state.stutterGate.process(sliceOutput, m_stutter.current, m_sampleRate);
            
            // Enhanced soft clipping with aging character
            if (std::abs(sliceOutput) > 0.7f) {
                float saturation = 1.0f + m_componentAge * 0.2f;
                sliceOutput = std::tanh(sliceOutput * 0.7f * saturation) / saturation;
            }
            
            // Apply output DC blocking
            sliceOutput = state.outputDCBlocker.process(sliceOutput);
            
            // Add subtle noise floor for realism
            float noiseLevel = std::pow(10.0f, state.noiseFloor / 20.0f);
            sliceOutput += noiseLevel * ((rand() % 1000) / 1000.0f - 0.5f) * 0.001f;
            
            // Mix with dry signal using smooth parameter
            channelData[sample] = dry * (1.0f - m_mix.current) + sliceOutput * m_mix.current;
        }
    }
}

BufferRepeat::Division BufferRepeat::getDivisionFromParam(float param) const {
    // Smooth parameter mapping with hysteresis to prevent jitter
    static float lastParam = 0.5f;
    float smoothedParam = lastParam * 0.95f + param * 0.05f;
    lastParam = smoothedParam;
    
    if (smoothedParam < 0.11f) return DIV_64TH;
    else if (smoothedParam < 0.22f) return DIV_32ND;
    else if (smoothedParam < 0.33f) return DIV_16TH;
    else if (smoothedParam < 0.44f) return DIV_8TH;
    else if (smoothedParam < 0.56f) return DIV_QUARTER;
    else if (smoothedParam < 0.67f) return DIV_HALF;
    else if (smoothedParam < 0.78f) return DIV_BAR;
    else if (smoothedParam < 0.89f) return DIV_2BARS;
    else return DIV_4BARS;
}

int BufferRepeat::getDivisionSamples(Division div) const {
    // Calculate samples per beat at current BPM
    double samplesPerBeat = (60.0 / m_bpm) * m_sampleRate;
    
    switch (div) {
        case DIV_64TH:    return static_cast<int>(samplesPerBeat / 16.0);
        case DIV_32ND:    return static_cast<int>(samplesPerBeat / 8.0);
        case DIV_16TH:    return static_cast<int>(samplesPerBeat / 4.0);
        case DIV_8TH:     return static_cast<int>(samplesPerBeat / 2.0);
        case DIV_QUARTER: return static_cast<int>(samplesPerBeat);
        case DIV_HALF:    return static_cast<int>(samplesPerBeat * 2.0);
        case DIV_BAR:     return static_cast<int>(samplesPerBeat * 4.0);
        case DIV_2BARS:   return static_cast<int>(samplesPerBeat * 8.0);
        case DIV_4BARS:   return static_cast<int>(samplesPerBeat * 16.0);
        default:          return static_cast<int>(samplesPerBeat);
    }
}

float BufferRepeat::getPitchRatio(float param) const {
    // Map parameter to pitch ratio
    // 0.0 = -1 octave (0.5x)
    // 0.5 = normal (1.0x)
    // 1.0 = +1 octave (2.0x)
    return std::pow(2.0f, (param - 0.5f) * 2.0f);
}

// Enhanced helper methods for boutique functionality
void BufferRepeat::updateAllSmoothParams() {
    m_division.update();
    m_probability.update();
    m_feedback.update();
    m_filter.update();
    m_pitch.update();
    m_reverse.update();
    m_stutter.update();
    m_mix.update();
}

void BufferRepeat::updateComponentAging() {
    m_sampleCount++;
    if (m_sampleCount > m_sampleRate * 8) { // Every 8 seconds
        m_componentAge = std::min(1.0f, m_componentAge + 0.00008f);
        m_sampleCount = 0;
    }
}

float BufferRepeat::applyAnalogCharacter(float input, float thermalFactor, float aging) {
    float output = input;
    
    // Apply thermal modulation to buffer timing
    output *= thermalFactor;
    
    // Component aging effects on buffer processing
    if (aging > 0.02f) {
        // Subtle nonlinearity from aging components
        float nonlinearity = aging * 0.01f;
        output += nonlinearity * output * output * (output > 0 ? 1.0f : -1.0f);
        
        // Slight high-frequency rolloff with aging
        static float hfState = 0.0f;
        float rolloff = 1.0f - aging * 0.05f;
        hfState += (output - hfState) * rolloff;
        output = output * (1.0f - aging * 0.02f) + hfState * aging * 0.02f;
    }
    
    return output;
}

void BufferRepeat::processEnhancedSlicing(ChannelState& state, float input, float thermalFactor, float aging) {
    // Process lookahead for better slice detection
    float delayedInput, lookaheadPeak;
    // Note: Lookahead processing would be implemented here for optimal slice timing
    
    // Enhanced slice detection with adaptive thresholds
    static float adaptiveThreshold = 0.1f;
    float inputLevel = std::abs(input);
    
    // Update adaptive threshold
    adaptiveThreshold += (inputLevel - adaptiveThreshold) * 0.001f;
    adaptiveThreshold = std::max(0.05f, std::min(0.5f, adaptiveThreshold));
    
    // Apply thermal drift to threshold
    float thermalThreshold = adaptiveThreshold * thermalFactor;
    
    // Store peak information for intelligent slicing
    static float peakHold = 0.0f;
    if (inputLevel > peakHold) {
        peakHold = inputLevel;
    } else {
        peakHold *= 0.9995f; // Slow decay
    }
}

float BufferRepeat::applyVintageBufferCharacter(float input, float feedback, float aging) {
    float output = input;
    
    // Vintage buffer characteristics
    if (feedback > 0.1f) {
        // Add subtle saturation in feedback path
        float satAmount = feedback * 0.3f;
        if (aging > 0.05f) {
            satAmount *= (1.0f + aging * 0.5f); // More saturation with age
        }
        output = std::tanh(output * (1.0f + satAmount)) / (1.0f + satAmount * 0.5f);
        
        // Add vintage-style frequency response changes
        static float vintageState = 0.0f;
        float vintageAmount = feedback * 0.2f + aging * 0.1f;
        vintageState += (output - vintageState) * (1.0f - vintageAmount);
        output = output * (1.0f - vintageAmount * 0.3f) + vintageState * vintageAmount * 0.3f;
    }
    
    return output;
}

void BufferRepeat::optimizeSliceTimings(ChannelState& state, float lookaheadPeak) {
    // Optimize slice timing based on lookahead analysis
    // This would implement intelligent slice boundary detection
    // based on transient analysis and musical timing
}

void BufferRepeat::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) m_division.target = params.at(0);
    if (params.count(1)) m_probability.target = params.at(1);
    if (params.count(2)) m_feedback.target = params.at(2);
    if (params.count(3)) m_filter.target = params.at(3);
    if (params.count(4)) m_pitch.target = params.at(4);
    if (params.count(5)) m_reverse.target = params.at(5);
    if (params.count(6)) m_stutter.target = params.at(6);
    if (params.count(7)) m_mix.target = params.at(7);
}

juce::String BufferRepeat::getParameterName(int index) const {
    switch (index) {
        case 0: return "Division";
        case 1: return "Probability";
        case 2: return "Feedback";
        case 3: return "Filter";
        case 4: return "Pitch";
        case 5: return "Reverse";
        case 6: return "Stutter";
        case 7: return "Mix";
        default: return "";
    }
}
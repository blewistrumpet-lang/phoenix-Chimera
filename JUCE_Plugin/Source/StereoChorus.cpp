#include "StereoChorus.h"
#include <cmath>
#include <algorithm>

StereoChorus::StereoChorus() {
    // Initialize with musical defaults
    m_rate.reset(0.3f);       // 0.3 Hz - gentle movement
    m_depth.reset(0.4f);      // Moderate depth
    m_feedback.reset(0.2f);   // Subtle feedback
    m_delay.reset(0.3f);      // ~15ms base delay
    m_width.reset(0.7f);      // Good stereo spread
    m_mix.reset(0.5f);        // 50/50 mix
}

void StereoChorus::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set smoothing times
    float smoothingMs = 10.0f;
    m_rate.setSmoothingTime(smoothingMs, sampleRate);
    m_depth.setSmoothingTime(smoothingMs, sampleRate);
    m_feedback.setSmoothingTime(smoothingMs, sampleRate);
    m_delay.setSmoothingTime(smoothingMs, sampleRate);
    m_width.setSmoothingTime(smoothingMs, sampleRate);
    m_mix.setSmoothingTime(smoothingMs, sampleRate);
    
    // Initialize delay lines (max 50ms)
    int maxDelaySamples = static_cast<int>(0.05f * sampleRate);
    
    for (int ch = 0; ch < 2; ++ch) {
        m_delayLines[ch].resize(maxDelaySamples, 0.0f);
        m_writePos[ch] = 0;
        
        // Initialize LFOs with different phases for stereo
        m_lfoPhase[ch] = ch * 0.25f; // 90 degree phase offset
        
        // Initialize filters
        m_highpass[ch].reset();
        m_lowpass[ch].reset();
        m_feedbackState[ch] = 0.0f;
    }
}

void StereoChorus::reset() {
    // Clear delay lines
    for (int ch = 0; ch < 2; ++ch) {
        std::fill(m_delayLines[ch].begin(), m_delayLines[ch].end(), 0.0f);
        m_writePos[ch] = 0;
        m_lfoPhase[ch] = ch * 0.25f;
        m_feedbackState[ch] = 0.0f;
        m_highpass[ch].reset();
        m_lowpass[ch].reset();
    }
}

void StereoChorus::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smoothed parameters
    m_rate.update();
    m_depth.update();
    m_feedback.update();
    m_delay.update();
    m_width.update();
    m_mix.update();
    
    // Calculate LFO rate in Hz
    float lfoRate = 0.1f + m_rate.current * 9.9f; // 0.1 to 10 Hz
    float lfoIncrement = lfoRate / m_sampleRate;
    
    // Calculate base delay in samples
    float baseDelayMs = 5.0f + m_delay.current * 25.0f; // 5-30ms
    float baseDelaySamples = baseDelayMs * m_sampleRate * 0.001f;
    
    // Calculate modulation depth in samples
    float modDepthSamples = m_depth.current * baseDelaySamples * 0.5f;
    
    // Process stereo or mono
    if (numChannels >= 2) {
        // True stereo processing
        for (int sample = 0; sample < numSamples; ++sample) {
            for (int ch = 0; ch < 2; ++ch) {
                float* channelData = buffer.getWritePointer(ch);
                float input = channelData[sample];
                
                // Calculate LFO modulation
                float lfoValue = std::sin(2.0f * M_PI * m_lfoPhase[ch]);
                
                // Add width modulation (different for each channel)
                float widthMod = ch == 0 ? (1.0f - m_width.current * 0.5f) : 
                                          (1.0f + m_width.current * 0.5f);
                
                // Calculate modulated delay
                float modulatedDelay = baseDelaySamples + 
                                      (lfoValue * modDepthSamples * widthMod);
                
                // Ensure delay is within valid range
                modulatedDelay = std::max(1.0f, 
                    std::min(modulatedDelay, static_cast<float>(m_delayLines[ch].size() - 2)));
                
                // Read from delay line with linear interpolation
                float delaySamplesInt = std::floor(modulatedDelay);
                float fraction = modulatedDelay - delaySamplesInt;
                
                int readPos1 = m_writePos[ch] - static_cast<int>(delaySamplesInt);
                int readPos2 = readPos1 - 1;
                
                if (readPos1 < 0) readPos1 += m_delayLines[ch].size();
                if (readPos2 < 0) readPos2 += m_delayLines[ch].size();
                
                float delayed = m_delayLines[ch][readPos1] * (1.0f - fraction) +
                               m_delayLines[ch][readPos2] * fraction;
                
                // Apply feedback with filtering
                float feedback = delayed * m_feedback.current;
                
                // Highpass filter to prevent low frequency buildup
                feedback = m_highpass[ch].process(feedback, 100.0f, m_sampleRate);
                
                // Lowpass filter for warmth
                feedback = m_lowpass[ch].process(feedback, 8000.0f, m_sampleRate);
                
                // Add slight cross-feedback for extra width
                float crossFeedback = 0.0f;
                if (numChannels >= 2) {
                    int otherCh = 1 - ch;
                    crossFeedback = m_feedbackState[otherCh] * m_feedback.current * 0.3f;
                }
                
                // Write to delay line
                m_delayLines[ch][m_writePos[ch]] = input + feedback + crossFeedback;
                
                // Store feedback state
                m_feedbackState[ch] = delayed;
                
                // Mix dry and wet signals
                channelData[sample] = input * (1.0f - m_mix.current) + 
                                     delayed * m_mix.current;
                
                // Update LFO phase
                m_lfoPhase[ch] += lfoIncrement;
                if (m_lfoPhase[ch] >= 1.0f) m_lfoPhase[ch] -= 1.0f;
                
                // Update write position
                m_writePos[ch] = (m_writePos[ch] + 1) % m_delayLines[ch].size();
            }
        }
    }
    else if (numChannels == 1) {
        // Mono processing
        float* channelData = buffer.getWritePointer(0);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            
            // Use channel 0 for mono
            float lfoValue = std::sin(2.0f * M_PI * m_lfoPhase[0]);
            float modulatedDelay = baseDelaySamples + (lfoValue * modDepthSamples);
            
            modulatedDelay = std::max(1.0f, 
                std::min(modulatedDelay, static_cast<float>(m_delayLines[0].size() - 2)));
            
            float delaySamplesInt = std::floor(modulatedDelay);
            float fraction = modulatedDelay - delaySamplesInt;
            
            int readPos1 = m_writePos[0] - static_cast<int>(delaySamplesInt);
            int readPos2 = readPos1 - 1;
            
            if (readPos1 < 0) readPos1 += m_delayLines[0].size();
            if (readPos2 < 0) readPos2 += m_delayLines[0].size();
            
            float delayed = m_delayLines[0][readPos1] * (1.0f - fraction) +
                           m_delayLines[0][readPos2] * fraction;
            
            // Apply feedback
            float feedback = delayed * m_feedback.current;
            feedback = m_highpass[0].process(feedback, 100.0f, m_sampleRate);
            feedback = m_lowpass[0].process(feedback, 8000.0f, m_sampleRate);
            
            // Write to delay line
            m_delayLines[0][m_writePos[0]] = input + feedback;
            
            // Mix dry and wet
            channelData[sample] = input * (1.0f - m_mix.current) + 
                                 delayed * m_mix.current;
            
            // Update LFO phase
            m_lfoPhase[0] += lfoIncrement;
            if (m_lfoPhase[0] >= 1.0f) m_lfoPhase[0] -= 1.0f;
            
            // Update write position
            m_writePos[0] = (m_writePos[0] + 1) % m_delayLines[0].size();
        }
    }
    
    scrubBuffer(buffer);
}

void StereoChorus::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    m_rate.target = getParam(0, 0.3f);
    m_depth.target = getParam(1, 0.4f);
    m_feedback.target = getParam(2, 0.2f);
    m_delay.target = getParam(3, 0.3f);
    m_width.target = getParam(4, 0.7f);
    m_mix.target = getParam(5, 0.5f);
}

juce::String StereoChorus::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Feedback";
        case 3: return "Delay";
        case 4: return "Width";
        case 5: return "Mix";
        default: return "";
    }
}

// SimpleFilter implementation
void StereoChorus::SimpleFilter::reset() {
    state = 0.0f;
}

float StereoChorus::SimpleFilter::process(float input, float cutoff, double sampleRate) {
    float rc = 1.0f / (2.0f * M_PI * cutoff);
    float dt = 1.0f / sampleRate;
    float alpha = dt / (rc + dt);
    
    if (isHighpass) {
        float output = input - state;
        state += alpha * output;
        return output;
    } else {
        state += alpha * (input - state);
        return state;
    }
}
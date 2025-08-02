#include "DigitalDelay.h"
#include <cmath>
#include <algorithm>

DigitalDelay::DigitalDelay() {
    // Initialize parameters with sensible defaults
    m_delayTime.setImmediate(0.25f);
    m_feedback.setImmediate(0.3f);
    m_mix.setImmediate(0.3f);
    m_highCut.setImmediate(0.8f);
}

void DigitalDelay::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Reset delay lines
    for (auto& line : m_delayLines) {
        line.reset();
    }
    
    // Reset filters
    for (auto& filter : m_highCutFilters) {
        filter.reset();
    }
    
    // Reset DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    // Reset crossfeed
    m_crossfeed.leftToRight = 0.0f;
    m_crossfeed.rightToLeft = 0.0f;
}

void DigitalDelay::reset() {
    // Reset all smooth parameters to their current targets (no smoothing jump)
    m_delayTime.current = m_delayTime.target;
    m_feedback.current = m_feedback.target;
    m_mix.current = m_mix.target;
    m_highCut.current = m_highCut.target;
    
    // Reset delay lines
    for (auto& line : m_delayLines) {
        line.reset();
    }
    
    // Reset filters
    for (auto& filter : m_highCutFilters) {
        filter.reset();
    }
    
    // Reset DC blockers
    for (auto& blocker : m_dcBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    // Reset crossfeed
    m_crossfeed.leftToRight = 0.0f;
    m_crossfeed.rightToLeft = 0.0f;
}

void DigitalDelay::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update parameter smoothing
    m_delayTime.update();
    m_feedback.update();
    m_mix.update();
    m_highCut.update();
    
    // Update filter coefficients if needed
    float cutoffFreq = 1000.0f + m_highCut.current * 19000.0f;
    for (auto& filter : m_highCutFilters) {
        filter.updateCoefficients(cutoffFreq, m_sampleRate);
    }
    
    // Process mono input as dual mono
    if (numChannels == 1) {
        float* channelData = buffer.getWritePointer(0);
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = processSample(channelData[sample], 0);
        }
    }
    // Process stereo with crossfeed
    else if (numChannels >= 2) {
        float* leftData = buffer.getWritePointer(0);
        float* rightData = buffer.getWritePointer(1);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float left = processSample(leftData[sample], 0);
            float right = processSample(rightData[sample], 1);
            
            // Apply crossfeed for ping-pong effect
            m_crossfeed.process(left, right);
            
            leftData[sample] = left;
            rightData[sample] = right;
        }
    }
}

float DigitalDelay::processSample(float input, int channel) {
    auto& line = m_delayLines[channel];
    
    // Calculate delay time in samples (1ms to 2000ms)
    float delayMs = 1.0f + m_delayTime.current * 1999.0f;
    double delaySamples = delayMs * m_sampleRate / 1000.0;
    
    // Add subtle modulation for organic feel (like BBD chips)
    line.modDepth = 0.5f; // Very subtle pitch wobble
    
    // Read delayed signal
    float delayed = line.read(delaySamples, m_sampleRate);
    
    // Apply high-cut filtering
    delayed = m_highCutFilters[channel].process(delayed);
    
    // Apply feedback with soft clipping
    float feedback = softClip(delayed * m_feedback.current);
    
    // Write new sample to delay line
    line.write(input + feedback);
    
    // Apply DC blocking to remove any offset
    float output = m_dcBlockers[channel].process(
        input * (1.0f - m_mix.current) + delayed * m_mix.current
    );
    
    return output;
}

// DelayLine implementation
float DigitalDelay::DelayLine::read(double delaySamples, double sampleRate) {
    // Apply modulation
    modPhase += modRate / sampleRate;
    if (modPhase >= 1.0f) modPhase -= 1.0f;
    
    float modulation = std::sin(2.0f * M_PI * modPhase) * modDepth;
    double actualDelay = delaySamples + modulation;
    
    // Calculate read position
    readPos = writePos - actualDelay;
    while (readPos < 0) readPos += BUFFER_SIZE;
    while (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;
    
    // Ensure minimum delay to prevent overlap
    double minDelay = 64.0;
    double currentDelay = writePos - readPos;
    if (currentDelay < 0) currentDelay += BUFFER_SIZE;
    if (currentDelay < minDelay) {
        readPos = writePos - minDelay;
        if (readPos < 0) readPos += BUFFER_SIZE;
    }
    
    return hermiteInterpolate(readPos);
}

void DigitalDelay::DelayLine::write(float sample) {
    buffer[static_cast<int>(writePos)] = sample;
    writePos += 1.0;
    if (writePos >= BUFFER_SIZE) writePos -= BUFFER_SIZE;
}

float DigitalDelay::DelayLine::hermiteInterpolate(double position) {
    int idx0 = static_cast<int>(position) - 1;
    int idx1 = idx0 + 1;
    int idx2 = idx0 + 2;
    int idx3 = idx0 + 3;
    
    // Wrap indices
    idx0 = ((idx0 % BUFFER_SIZE) + BUFFER_SIZE) % BUFFER_SIZE;
    idx1 = ((idx1 % BUFFER_SIZE) + BUFFER_SIZE) % BUFFER_SIZE;
    idx2 = ((idx2 % BUFFER_SIZE) + BUFFER_SIZE) % BUFFER_SIZE;
    idx3 = ((idx3 % BUFFER_SIZE) + BUFFER_SIZE) % BUFFER_SIZE;
    
    float y0 = buffer[idx0];
    float y1 = buffer[idx1];
    float y2 = buffer[idx2];
    float y3 = buffer[idx3];
    
    float x = position - std::floor(position);
    
    // Hermite interpolation for pristine quality
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    
    return ((c3 * x + c2) * x + c1) * x + c0;
}

// HighCutFilter implementation
void DigitalDelay::HighCutFilter::updateCoefficients(float cutoffFreq, double sampleRate) {
    // 2nd order Butterworth lowpass
    float omega = 2.0f * M_PI * cutoffFreq / sampleRate;
    float cos_omega = std::cos(omega);
    float sin_omega = std::sin(omega);
    float alpha = sin_omega / std::sqrt(2.0f);
    
    float norm = 1.0f / (1.0f + alpha);
    
    a0 = (1.0f - cos_omega) / 2.0f * norm;
    a1 = (1.0f - cos_omega) * norm;
    a2 = (1.0f - cos_omega) / 2.0f * norm;
    b1 = -2.0f * cos_omega * norm;
    b2 = (1.0f - alpha) * norm;
}

float DigitalDelay::HighCutFilter::process(float input) {
    float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
    
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}

float DigitalDelay::softClip(float input) {
    // Soft clipping for musical feedback
    if (std::abs(input) < 0.5f) {
        return input;
    }
    
    float sign = (input > 0.0f) ? 1.0f : -1.0f;
    float abs_input = std::abs(input);
    
    // Soft knee compression
    if (abs_input < 0.95f) {
        return sign * (0.5f + (abs_input - 0.5f) * 0.7f);
    }
    
    // Asymptotic limiting
    return sign * (0.95f + (1.0f - std::exp(-(abs_input - 0.95f) * 5.0f)) * 0.05f);
}

void DigitalDelay::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_delayTime.target = params.at(0);
    if (params.find(1) != params.end()) m_feedback.target = params.at(1);
    if (params.find(2) != params.end()) m_mix.target = params.at(2);
    if (params.find(3) != params.end()) m_highCut.target = params.at(3);
}

juce::String DigitalDelay::getParameterName(int index) const {
    switch (index) {
        case 0: return "Delay Time";
        case 1: return "Feedback";
        case 2: return "Mix";
        case 3: return "High Cut";
        default: return "";
    }
}
#include "MagneticDrumEcho.h"
#include <algorithm>

MagneticDrumEcho::MagneticDrumEcho() {
    // Initialize with classic drum echo settings
    m_delayTime.setImmediate(0.3f);
    m_head2.setImmediate(0.5f);
    m_head3.setImmediate(0.3f);
    m_feedback.setImmediate(0.4f);
    m_saturation.setImmediate(0.3f);
    m_mix.setImmediate(0.35f);
}

void MagneticDrumEcho::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Reset all heads
    for (auto& head : m_heads) {
        head.reset();
    }
}

void MagneticDrumEcho::reset() {
    // Reset all heads
    for (auto& head : m_heads) {
        head.reset();
    }
    
    // Reset tube stages
    for (auto& tube : m_tubeStages) {
        tube.prevSample = 0.0f;
        tube.dcBlockerX1 = 0.0f;
        tube.dcBlockerY1 = 0.0f;
    }
    
    // Reset filters
    for (auto& filter : m_filters) {
        filter.lpX1 = filter.lpX2 = filter.lpY1 = filter.lpY2 = 0.0f;
        filter.hpX1 = filter.hpY1 = 0.0f;
        filter.wobblePhase = static_cast<float>(rand()) / RAND_MAX; // Random start phase
    }
    
    // Reset feedback processors
    for (auto& fb : m_feedbackProcessors) {
        fb.prevSample = 0.0f;
        fb.saturation = 0.0f;
    }
    
    // Initialize motor
    m_motor.currentSpeed = 1.0f;
    m_motor.targetSpeed = 1.0f;
}

void MagneticDrumEcho::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smoothed parameters
    m_delayTime.update();
    m_head2.update();
    m_head3.update();
    m_feedback.update();
    m_saturation.update();
    m_mix.update();
    
    // Update motor simulation
    m_motor.update();
    
    // Update head delays based on motor speed
    updateHeadDelays();
    
    // Process each channel
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            float dry = channelData[sample];
            float wet = processDrumEcho(dry, channel);
            
            channelData[sample] = dry * (1.0f - m_mix.current) + wet * m_mix.current;
        }
    }
}

float MagneticDrumEcho::processDrumEcho(float input, int channel) {
    auto& tube = m_tubeStages[channel];
    auto& filter = m_filters[channel];
    auto& feedback = m_feedbackProcessors[channel];
    
    // Tube preamp stage (input saturation)
    float preampOut = tube.processTriode(input, m_saturation.current);
    
    // Apply speed modulation from motor
    float speedMod = m_motor.getSpeedVariation(m_sampleRate);
    
    // Write to record head (head 0)
    m_heads[0].write(preampOut);
    
    // Mix playback heads
    float echoMix = mixHeads(channel);
    
    // Feedback path with saturation
    float feedbackSignal = feedback.process(echoMix, m_feedback.current);
    
    // Add feedback to input
    m_heads[0].write(m_heads[0].buffer[static_cast<int>(m_heads[0].writePos)] + feedbackSignal);
    
    // Vintage filtering
    float filtered = filter.processHighpass(echoMix, m_sampleRate);
    float cutoffMod = 1.0f + filter.getSpeedModulation(m_sampleRate);
    filter.updateLowpass(4000.0f * cutoffMod, m_sampleRate);
    filtered = filter.processLowpass(filtered);
    
    // Output stage tube coloration
    float output = tube.processPentode(filtered, m_saturation.current * 0.5f);
    
    // DC blocking
    return tube.dcBlock(output);
}

void MagneticDrumEcho::updateHeadDelays() {
    // Base delay time in samples
    float baseDelay = (50.0f + m_delayTime.current * 750.0f) * m_sampleRate / 1000.0f;
    
    // Apply motor speed variation
    float speedFactor = m_motor.currentSpeed;
    
    // Set head positions (like physical spacing on drum)
    for (int i = 1; i < 4; ++i) {
        float headDelay = baseDelay * (m_headPositions[i] / 360.0f) * speedFactor;
        m_heads[i].readPos = m_heads[0].writePos - headDelay;
        
        // Wrap read position
        while (m_heads[i].readPos < 0) {
            m_heads[i].readPos += MagneticHead::MAX_BUFFER_SIZE;
        }
    }
}

float MagneticDrumEcho::mixHeads(int channel) {
    float mix = 0.0f;
    
    // Head 1 (always active)
    float head1 = m_heads[1].read(0.0);
    mix += head1;
    
    // Head 2 (variable level)
    if (m_head2.current > 0.01f) {
        float head2 = m_heads[2].read(0.0);
        mix += head2 * m_head2.current;
    }
    
    // Head 3 (variable level)
    if (m_head3.current > 0.01f) {
        float head3 = m_heads[3].read(0.0);
        mix += head3 * m_head3.current;
    }
    
    // Normalize based on active heads
    float totalLevel = 1.0f + m_head2.current + m_head3.current;
    return mix / std::sqrt(totalLevel);
}

// MagneticHead implementation
float MagneticDrumEcho::MagneticHead::read(double delaySamples) {
    // Calculate actual read position
    double actualReadPos = readPos + delaySamples;
    while (actualReadPos >= MAX_BUFFER_SIZE) actualReadPos -= MAX_BUFFER_SIZE;
    while (actualReadPos < 0) actualReadPos += MAX_BUFFER_SIZE;
    
    // Hermite interpolation for smooth playback
    int idx0 = static_cast<int>(actualReadPos);
    int idx1 = (idx0 + 1) % MAX_BUFFER_SIZE;
    int idx2 = (idx0 + 2) % MAX_BUFFER_SIZE;
    int idx3 = (idx0 + 3) % MAX_BUFFER_SIZE;
    
    float frac = actualReadPos - idx0;
    float y0 = buffer[idx0];
    float y1 = buffer[idx1];
    float y2 = buffer[idx2];
    float y3 = buffer[idx3];
    
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    
    float output = ((c3 * frac + c2) * frac + c1) * frac + c0;
    
    // Apply magnetic tape characteristics
    return processMagnetic(output);
}

void MagneticDrumEcho::MagneticHead::write(float sample) {
    buffer[static_cast<int>(writePos)] = sample;
    writePos += 1.0;
    if (writePos >= MAX_BUFFER_SIZE) writePos = 0.0;
}

float MagneticDrumEcho::MagneticHead::processMagnetic(float input) {
    // Simple magnetic tape hysteresis model
    float drive = 0.3f;
    magnetization = magnetization * 0.8f + input * 0.2f;
    
    float output = input;
    if (std::abs(magnetization) > 0.5f) {
        output = std::tanh(input * (1.0f + drive)) / (1.0f + drive * 0.5f);
    }
    
    // Add subtle compression (tape compression)
    float threshold = 0.7f;
    if (std::abs(output) > threshold) {
        float excess = std::abs(output) - threshold;
        float compressed = threshold + std::tanh(excess * 2.0f) * 0.3f;
        output = compressed * (output > 0 ? 1.0f : -1.0f);
    }
    
    hysteresis = output * 0.1f + hysteresis * 0.9f;
    return output + hysteresis * 0.05f;
}

// TubeStage implementation
float MagneticDrumEcho::TubeStage::processTriode(float input, float drive) {
    if (drive < 0.01f) return input;
    
    // Triode-style asymmetric clipping
    float biased = input + 0.1f * drive; // Grid bias
    float amplified = biased * (1.0f + drive * 3.0f);
    
    // Soft clipping with different curves for positive/negative
    float output;
    if (amplified > 0) {
        output = std::tanh(amplified * 0.7f) * 1.428f;
    } else {
        output = std::tanh(amplified * 0.9f) * 1.111f;
    }
    
    // Add 2nd harmonic (triode characteristic)
    float harmonic = output * output * (output > 0 ? 1.0f : -1.0f);
    output += harmonic * drive * 0.1f;
    
    return output;
}

float MagneticDrumEcho::TubeStage::processPentode(float input, float drive) {
    if (drive < 0.01f) return input;
    
    // Pentode has more linear region, harder clipping
    float amplified = input * (1.0f + drive * 5.0f);
    
    // Harder clipping threshold
    float threshold = 0.8f;
    float output = amplified;
    
    if (std::abs(amplified) > threshold) {
        float excess = std::abs(amplified) - threshold;
        float clipped = threshold + std::atan(excess * 3.0f) / 3.0f;
        output = clipped * (amplified > 0 ? 1.0f : -1.0f);
    }
    
    // Add 3rd harmonic (pentode characteristic)
    float harmonic = output * output * output;
    output += harmonic * drive * 0.03f;
    
    return output;
}

float MagneticDrumEcho::TubeStage::dcBlock(float input) {
    const float R = 0.995f;
    float output = input - dcBlockerX1 + R * dcBlockerY1;
    dcBlockerX1 = input;
    dcBlockerY1 = output;
    return output;
}

// VintageFilter implementation
void MagneticDrumEcho::VintageFilter::updateLowpass(float cutoffHz, double sampleRate) {
    float omega = 2.0f * M_PI * cutoffHz / sampleRate;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float Q = 0.7071f; // Butterworth
    float alpha = sinOmega / (2.0f * Q);
    
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosOmega;
    float a2 = 1.0f - alpha;
    float b0 = (1.0f - cosOmega) / 2.0f;
    float b1 = 1.0f - cosOmega;
    float b2 = (1.0f - cosOmega) / 2.0f;
    
    // Store normalized coefficients
    lpX1 = b1 / a0;
    lpX2 = b0 / a0;
    lpY1 = -a1 / a0;
    lpY2 = -a2 / a0;
}

float MagneticDrumEcho::VintageFilter::processLowpass(float input) {
    float output = input * lpX2 + lpX1 * lpX1 + lpX2 * lpX2 + lpY1 * lpY1 + lpY2 * lpY2;
    
    lpX2 = lpX1;
    lpX1 = input;
    lpY2 = lpY1;
    lpY1 = output;
    
    return output;
}

float MagneticDrumEcho::VintageFilter::processHighpass(float input, double sampleRate) {
    // Fixed 50Hz highpass (AC coupling)
    const float R = 0.9995f;
    float output = input - hpX1 + R * hpY1;
    hpX1 = input;
    hpY1 = output;
    return output;
}

float MagneticDrumEcho::VintageFilter::getSpeedModulation(double sampleRate) {
    wobblePhase += wobbleRate / sampleRate;
    if (wobblePhase >= 1.0f) wobblePhase -= 1.0f;
    
    return std::sin(2.0f * M_PI * wobblePhase) * wobbleDepth;
}

// DrumMotor implementation
float MagneticDrumEcho::DrumMotor::getSpeedVariation(double sampleRate) {
    resonancePhase += resonanceFreq / sampleRate;
    if (resonancePhase >= 1.0f) resonancePhase -= 1.0f;
    
    float resonance = std::sin(2.0f * M_PI * resonancePhase) * resonanceAmount;
    return currentSpeed + resonance;
}

// FeedbackProcessor implementation
float MagneticDrumEcho::FeedbackProcessor::process(float input, float amount) {
    // Soft limiting in feedback path
    float feedback = input * amount;
    
    if (std::abs(feedback) > 0.8f) {
        feedback = std::tanh(feedback * 1.25f) * 0.8f;
    }
    
    // Add some low-frequency emphasis (tape head bump)
    float diff = feedback - prevSample;
    prevSample = feedback;
    
    return feedback + diff * 0.1f;
}

void MagneticDrumEcho::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_delayTime.target = params.at(0);
    if (params.find(1) != params.end()) m_head2.target = params.at(1);
    if (params.find(2) != params.end()) m_head3.target = params.at(2);
    if (params.find(3) != params.end()) m_feedback.target = params.at(3);
    if (params.find(4) != params.end()) m_saturation.target = params.at(4);
    if (params.find(5) != params.end()) m_mix.target = params.at(5);
}

juce::String MagneticDrumEcho::getParameterName(int index) const {
    switch (index) {
        case 0: return "Delay Time";
        case 1: return "Head 2";
        case 2: return "Head 3";
        case 3: return "Feedback";
        case 4: return "Saturation";
        case 5: return "Mix";
        default: return "";
    }
}
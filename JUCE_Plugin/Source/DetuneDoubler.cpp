#include "DetuneDoubler.h"
#include <cmath>

DetuneDoubler::DetuneDoubler() {
}

void DetuneDoubler::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize delay buffers (max 50ms + modulation headroom)
    int maxDelaySamples = static_cast<int>(sampleRate * 0.1); // 100ms headroom
    m_delayBuffers.clear();
    m_writePositions.clear();
    m_readPositions.clear();
    m_lfoPhase.clear();
    m_crossfadePhase.clear();
    m_lowpassState.clear();
    m_highpassState.clear();
    m_allPassChains.clear();
    
    for (int ch = 0; ch < 2; ++ch) {
        m_delayBuffers.push_back(std::vector<float>(maxDelaySamples, 0.0f));
        m_writePositions.push_back(0.0f);
        m_readPositions.push_back(0.0f);
        m_lfoPhase.push_back(ch * M_PI); // Phase offset for stereo
        m_crossfadePhase.push_back(0.0f);
        m_lowpassState.push_back(0.0f);
        m_highpassState.push_back(0.0f);
        
        // Create all-pass filter chains for phase decoration
        std::vector<AllPassFilter> chainL, chainR;
        
        // Chain 1: Short delays for early reflections
        AllPassFilter ap1, ap2, ap3;
        ap1.delayLength = 89;  // Prime numbers to avoid periodicity
        ap2.delayLength = 97;
        ap3.delayLength = 101;
        ap1.gain = 0.7f;
        ap2.gain = -0.6f;
        ap3.gain = 0.5f;
        
        ap1.buffer.resize(ap1.delayLength, 0.0f);
        ap2.buffer.resize(ap2.delayLength, 0.0f);
        ap3.buffer.resize(ap3.delayLength, 0.0f);
        
        chainL.push_back(ap1);
        chainL.push_back(ap2);
        chainL.push_back(ap3);
        
        // Chain 2: Different delays for right channel
        AllPassFilter ap4, ap5, ap6;
        ap4.delayLength = 83;
        ap5.delayLength = 103;
        ap6.delayLength = 107;
        ap4.gain = 0.65f;
        ap5.gain = -0.65f;
        ap6.gain = 0.55f;
        
        ap4.buffer.resize(ap4.delayLength, 0.0f);
        ap5.buffer.resize(ap5.delayLength, 0.0f);
        ap6.buffer.resize(ap6.delayLength, 0.0f);
        
        chainR.push_back(ap4);
        chainR.push_back(ap5);
        chainR.push_back(ap6);
        
        m_allPassChains.push_back(chainL);
        m_allPassChains.push_back(chainR);
    }

void DetuneDoubler::reset() {
    // Reset all internal state
    // TODO: Implement specific reset logic for DetuneDoubler
}

    
    updateDelayTimes();
}

void DetuneDoubler::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = processSample(channelData[sample], channel);
        }
    }
}

float DetuneDoubler::processSample(float input, int channel) {
    // Apply tape-style input filtering
    float filtered = tapeFilter(input, channel);
    
    // Generate LFO for pitch modulation
    float lfo = generateLFO(channel);
    
    // Calculate detune in samples (pitch shift via delay modulation)
    float detuneInCents = m_detuneAmount * 50.0f; // 0-50 cents
    float pitchRatio = std::pow(2.0f, detuneInCents / 1200.0f);
    
    // Base delay time (5ms to 50ms)
    float baseDelayMs = 5.0f + m_delayTime * 45.0f;
    float baseDelaySamples = baseDelayMs * m_sampleRate / 1000.0f;
    
    // Modulate delay time for pitch shifting
    float modulatedDelay = baseDelaySamples * (1.0f + (1.0f - pitchRatio) + lfo * 0.001f);
    
    // Apply variable delay
    float delayed = variableDelay(filtered, channel, modulatedDelay);
    
    // Apply all-pass filtering for phase decoration
    float decorated = allPassChain(delayed, channel, 0);
    
    // Create second voice with different characteristics
    float secondVoiceDelay = baseDelaySamples * (1.0f + (1.0f - 1.0f/pitchRatio) - lfo * 0.0008f);
    float secondVoice = variableDelay(filtered, channel, secondVoiceDelay);
    secondVoice = allPassChain(secondVoice, channel, 1);
    
    // Apply stereo width
    float leftWeight = 1.0f;
    float rightWeight = 1.0f;
    
    if (channel == 0) {
        // Left channel: main voice left, second voice right
        leftWeight = 1.0f;
        rightWeight = m_stereoWidth;
    } else {
        // Right channel: main voice right, second voice left  
        leftWeight = m_stereoWidth;
        rightWeight = 1.0f;
    }
    
    // Mix the voices
    float doubledOutput = (decorated * leftWeight + secondVoice * rightWeight) * 0.5f;
    
    // Add subtle feedback for thickness
    float feedback = doubledOutput * m_feedback * 0.1f;
    
    // Mix with dry signal
    float dryLevel = 0.7f;
    float wetLevel = 0.6f;
    
    return input * dryLevel + (doubledOutput + feedback) * wetLevel;
}

float DetuneDoubler::variableDelay(float input, int channel, float delaySamples) {
    int bufferSize = static_cast<int>(m_delayBuffers[channel].size());
    
    // Write input to delay buffer
    int writePos = static_cast<int>(m_writePositions[channel]);
    m_delayBuffers[channel][writePos] = input;
    
    // Calculate read position
    float readPos = m_writePositions[channel] - delaySamples;
    while (readPos < 0) readPos += bufferSize;
    
    // Read with interpolation
    int readPos1 = static_cast<int>(readPos);
    int readPos2 = (readPos1 + 1) % bufferSize;
    float frac = readPos - readPos1;
    
    float sample1 = m_delayBuffers[channel][readPos1];
    float sample2 = m_delayBuffers[channel][readPos2];
    float interpolated = sample1 * (1.0f - frac) + sample2 * frac;
    
    // Update write position
    m_writePositions[channel] += 1.0f;
    if (m_writePositions[channel] >= bufferSize) {
        m_writePositions[channel] -= bufferSize;
    }
    
    return interpolated;
}

float DetuneDoubler::allPassChain(float input, int channel, int chainIndex) {
    int actualChainIndex = channel * 2 + chainIndex; // 2 chains per channel
    if (actualChainIndex >= m_allPassChains.size()) return input;
    
    float output = input;
    
    // Process through all-pass filter chain
    for (auto& filter : m_allPassChains[actualChainIndex]) {
        // All-pass filter: output = -gain * input + delayed_input + gain * delayed_output
        float delayed = filter.buffer[filter.writePos];
        float filterOutput = -filter.gain * output + delayed;
        
        // Store input + gain * output in delay line
        filter.buffer[filter.writePos] = output + filter.gain * filterOutput;
        
        // Update write position
        filter.writePos = (filter.writePos + 1) % filter.delayLength;
        
        output = filterOutput;
    }
    
    return output;
}

float DetuneDoubler::tapeFilter(float input, int channel) {
    // Simulate ADT tape filtering characteristics
    
    // High-cut filter (~8kHz) for tape warmth
    float hfCutoff = 8000.0f / m_sampleRate;
    hfCutoff = std::min(hfCutoff, 0.45f);
    m_lowpassState[channel] += hfCutoff * (input - m_lowpassState[channel]);
    float highCut = m_lowpassState[channel];
    
    // Subtle high-pass (~40Hz) to remove DC and rumble
    float lfCutoff = 40.0f / m_sampleRate;
    m_highpassState[channel] += lfCutoff * (highCut - m_highpassState[channel]);
    float filtered = highCut - m_highpassState[channel];
    
    return filtered;
}

float DetuneDoubler::generateLFO(int channel) {
    // Generate subtle LFO for tape speed variations
    float lfo = std::sin(m_lfoPhase[channel]);
    
    // Update phase
    m_lfoPhase[channel] += 2.0f * M_PI * m_lfoRate / m_sampleRate;
    if (m_lfoPhase[channel] > 2.0f * M_PI) {
        m_lfoPhase[channel] -= 2.0f * M_PI;
    }
    
    return lfo;
}

void DetuneDoubler::updateDelayTimes() {
    // This would be called when parameters change significantly
    // For now, delay times are calculated per-sample for smooth modulation
}

void DetuneDoubler::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_detuneAmount = params.at(0);
    if (params.find(1) != params.end()) m_delayTime = params.at(1);
    if (params.find(2) != params.end()) m_feedback = params.at(2);
    if (params.find(3) != params.end()) m_stereoWidth = params.at(3);
    
    updateDelayTimes();
}

juce::String DetuneDoubler::getParameterName(int index) const {
    switch (index) {
        case 0: return "Detune Amount";
        case 1: return "Delay Time";
        case 2: return "Feedback";
        case 3: return "Stereo Width";
        default: return "";
    }
}
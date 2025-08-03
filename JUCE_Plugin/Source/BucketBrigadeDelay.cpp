#include "BucketBrigadeDelay.h"
#include <cmath>
#include <random>
#include <algorithm>

BucketBrigadeDelay::BucketBrigadeDelay() {
    // Initialize smoothed parameters
    m_delayTime.reset(0.3f);
    m_feedback.reset(0.4f);
    m_mix.reset(0.5f);
    m_clockNoise.reset(0.3f);
    m_age.reset(0.0f);
}

void BucketBrigadeDelay::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set smoothing time for parameters
    float smoothingTime = 50.0f; // 50ms smoothing
    m_delayTime.setSmoothingTime(smoothingTime, sampleRate);
    m_feedback.setSmoothingTime(smoothingTime, sampleRate);
    m_mix.setSmoothingTime(smoothingTime, sampleRate);
    m_clockNoise.setSmoothingTime(smoothingTime, sampleRate);
    m_age.setSmoothingTime(1000.0f, sampleRate); // Slower for aging
    
    // Prepare channel states
    for (auto& channel : m_channelStates) {
        channel.prepare(sampleRate);
    }
}

void BucketBrigadeDelay::reset() {
    // Clear delay buffers and reset indices
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
}

void BucketBrigadeDelay::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update analog modeling drift
    m_analogModeling.update(m_sampleRate);
    
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update smoothed parameters
            m_delayTime.update();
            m_feedback.update();
            m_mix.update();
            m_clockNoise.update();
            m_age.update();
            
            channelData[sample] = processSample(channelData[sample], channel);
        }
    }
}

float BucketBrigadeDelay::processSample(float input, int channel) {
    auto& state = m_channelStates[channel];
    
    // Apply DC blocking to input
    float cleanInput = applyDCBlocking(input, channel);
    
    // Pre-filter input (anti-aliasing and aging characteristics)
    float filtered = state.filtering.processInput(cleanInput, m_age.current, m_sampleRate);
    
    // Companding - compress before BBD processing
    float compressed = state.companding.processCompress(filtered);
    
    // Calculate delay time with analog modeling
    float baseDelayMs = 5.0f + m_delayTime.current * (MAX_DELAY_MS - 5.0f);
    float tempDrift = m_analogModeling.temperatureDrift * baseDelayMs * 0.05f;
    float actualDelayMs = baseDelayMs + tempDrift;
    float delaySamples = (actualDelayMs / 1000.0f) * m_sampleRate;
    
    // Process through BBD stages
    float bbdOutput = state.bbdProcessor.process(compressed, actualDelayMs, 
                                                m_clockNoise.current, m_age.current, m_sampleRate);
    
    // Expand the signal (decompress)
    float expanded = state.companding.processExpand(bbdOutput, m_age.current);
    
    // Apply output filtering
    float outputFiltered = state.filtering.processOutput(expanded, m_age.current, m_sampleRate);
    
    // Process feedback
    float feedbackSignal = processFeedback(outputFiltered, channel);
    
    // Store delayed signal in delay line for proper feedback
    state.delayLine.write(compressed + feedbackSignal);
    
    // Read delayed signal with interpolation
    float delayedSignal = state.delayLine.readInterpolated(delaySamples);
    
    // Mix dry and wet signals with soft limiting
    float dryLevel = 1.0f - m_mix.current;
    float wetLevel = m_mix.current;
    
    float output = cleanInput * dryLevel + delayedSignal * wetLevel;
    return softLimit(output, 0.95f);
}

float BucketBrigadeDelay::processFeedback(float sample, int channel) {
    auto& state = m_channelStates[channel];
    
    // Apply feedback gain with saturation modeling
    float feedbackGain = m_feedback.current;
    float feedbackSignal = sample * feedbackGain;
    
    // High-pass filter to prevent low-frequency buildup
    float hpCutoff = 20.0f / m_sampleRate; // 20Hz highpass
    state.feedbackHighpass += hpCutoff * (feedbackSignal - state.feedbackHighpass);
    feedbackSignal -= state.feedbackHighpass;
    
    // Soft saturation in feedback path
    feedbackSignal = softClip(feedbackSignal);
    
    // Store for next iteration
    state.feedbackSample = feedbackSignal;
    
    return feedbackSignal;
}

float BucketBrigadeDelay::applyDCBlocking(float input, int channel) {
    auto& state = m_channelStates[channel];
    
    // High-pass filter with R=0.995 (approximately 3.4Hz @ 44.1kHz)
    float output = input - state.dcBlockerX + 0.995f * state.dcBlockerY;
    state.dcBlockerX = input;
    state.dcBlockerY = output;
    
    return output;
}

// BBD Stage Processor Implementation
float BucketBrigadeDelay::BBDStageProcessor::ClockGenerator::generateClock(float amount, double sampleRate) {
    // Generate realistic BBD clock noise
    phase += 2.0f * M_PI / sampleRate;
    if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    
    // High-frequency switching noise (around 100kHz)
    noisePhase1 += 2.0f * M_PI * 50000.0f / sampleRate;
    if (noisePhase1 > 2.0f * M_PI) noisePhase1 -= 2.0f * M_PI;
    highFreqNoise = std::sin(noisePhase1) * amount * 0.01f;
    
    // Low-frequency drift (sub-audio)
    noisePhase2 += 2.0f * M_PI * 0.5f / sampleRate;
    if (noisePhase2 > 2.0f * M_PI) noisePhase2 -= 2.0f * M_PI;
    lowFreqNoise = std::sin(noisePhase2) * amount * 0.005f;
    
    // Clock jitter (affects delay time slightly)
    static std::mt19937 rng(42);
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    jitter = dist(rng) * amount * 0.002f;
    
    return highFreqNoise + lowFreqNoise + jitter;
}

float BucketBrigadeDelay::BBDStageProcessor::process(float input, float delayTime, 
                                                   float clockNoise, float aging, double sampleRate) {
    // Generate clock modulation
    float clockMod = clock1.generateClock(clockNoise, sampleRate);
    
    // Calculate effective delay with clock variations
    float effectiveDelay = delayTime * (1.0f + clockMod);
    
    // Simplified BBD processing - in reality this would involve complex
    // charge transfer calculations across all stages
    float delayedSample = input; // Placeholder for stage processing
    
    // Apply transfer efficiency loss (increases with aging)
    float efficiency = transferEfficiency * (1.0f - aging * 0.1f);
    delayedSample *= efficiency;
    
    // Add capacitive feedthrough
    delayedSample += input * feedthrough * (1.0f + aging * 0.5f);
    
    return delayedSample;
}

// Companding Processor Implementation
float BucketBrigadeDelay::CompandingProcessor::processCompress(float input) {
    // VCA-based compression similar to dbx systems
    float envelope = std::abs(input);
    
    // Update envelope follower
    float attack = 0.01f;
    float release = 0.1f;
    
    if (envelope > compEnvelope) {
        compEnvelope += (envelope - compEnvelope) * attack;
    } else {
        compEnvelope += (envelope - compEnvelope) * release;
    }
    
    // Calculate gain reduction
    float threshold = 0.1f;
    float ratio = 2.0f;
    
    if (compEnvelope > threshold) {
        float excess = compEnvelope - threshold;
        compressorGain = threshold / compEnvelope + (excess / ratio) / compEnvelope;
    } else {
        compressorGain = 1.0f;
    }
    
    return input * compressorGain;
}

float BucketBrigadeDelay::CompandingProcessor::processExpand(float input, float aging) {
    // Expansion to restore dynamics
    float envelope = std::abs(input);
    
    if (envelope > expEnvelope) {
        expEnvelope += (envelope - expEnvelope) * 0.01f;
    } else {
        expEnvelope += (envelope - expEnvelope) * 0.1f;
    }
    
    // Inverse of compression with aging compensation
    float expanderRatio = 2.0f * (1.0f - aging * 0.3f);
    expanderGain = std::pow(expEnvelope + 1e-6f, 1.0f / expanderRatio - 1.0f);
    expanderGain = std::max(0.1f, std::min(3.0f, expanderGain));
    
    return input * expanderGain * 0.9f; // Slight attenuation
}

// Filter implementations
void BucketBrigadeDelay::BBDFilter::ButterworthLP::updateCoefficients(float freq, double sampleRate) {
    float omega = 2.0f * M_PI * freq / sampleRate;
    float cos_omega = std::cos(omega);
    float sin_omega = std::sin(omega);
    float Q = 0.707f; // Butterworth response
    float alpha = sin_omega / (2.0f * Q);
    
    a0 = 1.0f + alpha;
    a1 = -2.0f * cos_omega;
    a2 = 1.0f - alpha;
    b1 = 1.0f - cos_omega;
    float b0 = b1 * 0.5f;
    float b2 = b0;
    
    // Normalize
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
    a0 = 1.0f;
}

float BucketBrigadeDelay::BBDFilter::ButterworthLP::process(float input) {
    float output = input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    // Shift delays
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}

float BucketBrigadeDelay::BBDFilter::processInput(float input, float aging, double sampleRate) {
    // BBD input characteristics: limited bandwidth, pre-emphasis
    
    // Bandwidth decreases with aging (3kHz down to 1kHz)
    float cutoffFreq = 3000.0f * (1.0f - aging * 0.67f);
    inputFilter.updateCoefficients(cutoffFreq, sampleRate);
    
    // Apply anti-aliasing filter
    float filtered = inputFilter.process(input);
    
    // Pre-emphasis (slight high-frequency boost)
    float preEmphFreq = 1000.0f / sampleRate;
    float preEmphGain = 1.2f * (1.0f - aging * 0.3f);
    preEmphZ1 += preEmphFreq * (filtered - preEmphZ1);
    float preEmphasized = filtered + (filtered - preEmphZ1) * (preEmphGain - 1.0f);
    
    return preEmphasized;
}

float BucketBrigadeDelay::BBDFilter::processOutput(float input, float aging, double sampleRate) {
    // BBD output characteristics: bandwidth limitation, de-emphasis
    
    // Output bandwidth (slightly different from input)
    float cutoffFreq = 2500.0f * (1.0f - aging * 0.6f);
    outputFilter.updateCoefficients(cutoffFreq, sampleRate);
    
    // Apply reconstruction filter
    float filtered = outputFilter.process(input);
    
    // De-emphasis (compensate for pre-emphasis)
    float deEmphFreq = 1200.0f / sampleRate;
    deEmphZ1 += deEmphFreq * (filtered - deEmphZ1);
    float deEmphasized = filtered - (filtered - deEmphZ1) * 0.15f * (1.0f - aging * 0.3f);
    
    return deEmphasized;
}

void BucketBrigadeDelay::BBDFilter::reset() {
    inputFilter.reset();
    outputFilter.reset();
    preEmphZ1 = deEmphZ1 = 0.0f;
}

void BucketBrigadeDelay::updateParameters(const std::map<int, float>& params) {
    if (params.find(0) != params.end()) m_delayTime.target = params.at(0);
    if (params.find(1) != params.end()) m_feedback.target = params.at(1);
    if (params.find(2) != params.end()) m_mix.target = params.at(2);
    if (params.find(3) != params.end()) m_clockNoise.target = params.at(3);
    if (params.find(4) != params.end()) m_age.target = params.at(4);
}

juce::String BucketBrigadeDelay::getParameterName(int index) const {
    switch (index) {
        case 0: return "Delay Time";
        case 1: return "Feedback";
        case 2: return "Mix";
        case 3: return "Clock Noise";
        case 4: return "Age";
        default: return "";
    }
}
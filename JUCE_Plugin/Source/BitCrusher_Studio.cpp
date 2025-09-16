// BitCrusher_Studio.cpp - Professional studio-quality bit crusher
// Properly implemented with anti-aliasing, correct quantization, and no unnecessary complexity

#include "BitCrusher.h"
#include <cmath>
#include <algorithm>
#include <immintrin.h>

// ==================== Constructor ====================
BitCrusher::BitCrusher() : m_rng(std::random_device{}()) {
    // Initialize parameters with sensible defaults
    m_bitDepth.setImmediate(16.0f);          // 16-bit (no crushing)
    m_sampleRateReduction.setImmediate(1.0f); // No downsampling
    m_aliasing.setImmediate(0.0f);           // No intentional aliasing
    m_jitter.setImmediate(0.0f);             // No jitter
    m_dcOffset.setImmediate(0.5f);           // No DC offset (centered)
    m_gateThreshold.setImmediate(0.0f);      // No gating
    m_dither.setImmediate(0.0f);             // No dither
    m_mix.setImmediate(0.5f);                // 50% wet (not 100%!)
    
    // Set appropriate smoothing rates
    m_bitDepth.setSmoothingRate(0.995f);
    m_sampleRateReduction.setSmoothingRate(0.995f);
    m_aliasing.setSmoothingRate(0.998f);
    m_jitter.setSmoothingRate(0.998f);
    m_dcOffset.setSmoothingRate(0.998f);
    m_gateThreshold.setSmoothingRate(0.995f);
    m_dither.setSmoothingRate(0.998f);
    m_mix.setSmoothingRate(0.999f);
}

// ==================== Prepare ====================
void BitCrusher::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize channel states
    for (auto& state : m_channelStates) {
        state = ChannelState(); // Zero-initialize
    }
    
    // Initialize DC blockers with proper cutoff
    const float dcCutoff = 10.0f / sampleRate; // 10 Hz highpass
    const float dcAlpha = 1.0f - std::exp(-2.0f * M_PI * dcCutoff);
    
    for (auto& blocker : m_inputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
        blocker.R = 1.0f - dcAlpha;
    }
    
    for (auto& blocker : m_outputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
        blocker.R = 1.0f - dcAlpha;
    }
    
    // Prepare high-quality oversampler
    prepareOversampler(sampleRate, samplesPerBlock);
}

// ==================== Prepare Oversampler ====================
void BitCrusher::prepareOversampler(double sampleRate, int samplesPerBlock) {
    // We'll use 4x oversampling for quality
    constexpr int oversampleFactor = 4;
    
    // Allocate buffers
    m_oversampler.upsampleBuffer.resize(samplesPerBlock * oversampleFactor);
    m_oversampler.downsampleBuffer.resize(samplesPerBlock * oversampleFactor);
    
    // Design proper anti-aliasing filters
    // Using Butterworth coefficients for steep rolloff at Nyquist/4
    const double cutoffFreq = 0.45 * sampleRate / 2.0; // 45% of Nyquist
    const double oversampledRate = sampleRate * oversampleFactor;
    const double wc = 2.0 * M_PI * cutoffFreq / oversampledRate;
    const double wc2 = wc * wc;
    const double wc3 = wc2 * wc;
    const double wc4 = wc2 * wc2;
    const double k = wc / std::tan(wc / 2.0);
    const double k2 = k * k;
    const double k3 = k2 * k;
    const double k4 = k2 * k2;
    const double sq2 = std::sqrt(2.0);
    
    // Calculate normalized coefficients
    const double b0 = wc4;
    const double b1 = 4.0 * wc4;
    const double b2 = 6.0 * wc4;
    const double b3 = 4.0 * wc4;
    const double b4 = wc4;
    
    const double a0 = k4 + 2.0 * sq2 * k3 * wc + 4.0 * k2 * wc2 + 2.0 * sq2 * k * wc3 + wc4;
    const double a1 = 4.0 * (wc4 + 2.0 * sq2 * k * wc3 - 2.0 * k2 * wc2 - 2.0 * k4);
    const double a2 = 6.0 * wc4 - 8.0 * k2 * wc2 + 6.0 * k4;
    const double a3 = 4.0 * (wc4 - 2.0 * sq2 * k * wc3 - 2.0 * k2 * wc2 + 2.0 * k4);
    const double a4 = k4 - 2.0 * sq2 * k3 * wc + 4.0 * k2 * wc2 - 2.0 * sq2 * k * wc3 + wc4;
    
    // Store normalized coefficients
    // These will be used in the actual filtering
    m_oversampleCoeffs.a0 = static_cast<float>(b0 / a0);
    m_oversampleCoeffs.a1 = static_cast<float>(b1 / a0);
    m_oversampleCoeffs.a2 = static_cast<float>(b2 / a0);
    m_oversampleCoeffs.a3 = static_cast<float>(b3 / a0);
    m_oversampleCoeffs.a4 = static_cast<float>(b4 / a0);
    m_oversampleCoeffs.b1 = static_cast<float>(a1 / a0);
    m_oversampleCoeffs.b2 = static_cast<float>(a2 / a0);
    m_oversampleCoeffs.b3 = static_cast<float>(a3 / a0);
    m_oversampleCoeffs.b4 = static_cast<float>(a4 / a0);
}

// ==================== Reset ====================
void BitCrusher::reset() {
    // Reset parameters to current values
    m_bitDepth.current = m_bitDepth.target;
    m_sampleRateReduction.current = m_sampleRateReduction.target;
    m_aliasing.current = m_aliasing.target;
    m_jitter.current = m_jitter.target;
    m_dcOffset.current = m_dcOffset.target;
    m_gateThreshold.current = m_gateThreshold.target;
    m_dither.current = m_dither.target;
    m_mix.current = m_mix.target;
    
    // Clear all states
    for (auto& state : m_channelStates) {
        state = ChannelState();
    }
    
    // Clear DC blockers
    for (auto& blocker : m_inputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    for (auto& blocker : m_outputDCBlockers) {
        blocker.x1 = 0.0f;
        blocker.y1 = 0.0f;
    }
    
    // Clear oversampler filter states
    m_oversampler.upsampleFilter.x.fill(0.0f);
    m_oversampler.upsampleFilter.y.fill(0.0f);
    m_oversampler.downsampleFilter.x.fill(0.0f);
    m_oversampler.downsampleFilter.y.fill(0.0f);
}

// ==================== Main Process ====================
void BitCrusher::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update smooth parameters
    m_bitDepth.update();
    m_sampleRateReduction.update();
    m_aliasing.update();
    m_jitter.update();
    m_dcOffset.update();
    m_gateThreshold.update();
    m_dither.update();
    m_mix.update();
    
    // Process each channel
    for (int channel = 0; channel < numChannels && channel < 2; ++channel) {
        auto& state = m_channelStates[channel];
        float* channelData = buffer.getWritePointer(channel);
        
        // Apply input DC blocking FIRST
        for (int i = 0; i < numSamples; ++i) {
            channelData[i] = m_inputDCBlockers[channel].process(channelData[i]);
        }
        
        // Decide whether to use oversampling based on settings
        const bool needsOversampling = (m_bitDepth.current < 12.0f || 
                                       m_sampleRateReduction.current > 1.5f) &&
                                       m_aliasing.current < 0.5f;
        
        if (needsOversampling && m_useOversampling) {
            processWithOversampling(channelData, numSamples, state, channel);
        } else {
            processWithoutOversampling(channelData, numSamples, state, channel);
        }
        
        // Apply output DC blocking LAST
        for (int i = 0; i < numSamples; ++i) {
            channelData[i] = m_outputDCBlockers[channel].process(channelData[i]);
            
            // Final safety clamp
            channelData[i] = std::max(-1.0f, std::min(1.0f, channelData[i]));
        }
    }
}

// ==================== Process Without Oversampling ====================
void BitCrusher::processWithoutOversampling(float* data, int numSamples, 
                                           ChannelState& state, int channel) {
    for (int i = 0; i < numSamples; ++i) {
        const float drySignal = data[i];
        float wetSignal = drySignal;
        
        // Apply gate threshold
        if (m_gateThreshold.current > 0.0f) {
            if (std::abs(wetSignal) < m_gateThreshold.current * 0.1f) {
                wetSignal = 0.0f;
            }
        }
        
        // Sample rate reduction with PROPER handling of edge cases
        const float reductionFactor = std::max(1.0f, m_sampleRateReduction.current);
        const float increment = 1.0f / reductionFactor;
        
        state.sampleCounter += increment;
        
        if (state.sampleCounter >= 1.0f) {
            // Add jitter if enabled
            if (m_jitter.current > 0.0f) {
                float jitter = m_distribution(m_rng) * m_jitter.current * 0.1f;
                state.sampleCounter = std::abs(jitter); // Never negative!
            } else {
                state.sampleCounter -= 1.0f;
            }
            
            // Apply bit reduction
            wetSignal = quantizeProperly(wetSignal, m_bitDepth.current, state);
            
            // Update held sample
            state.heldSample = wetSignal;
        } else {
            // Use held sample
            wetSignal = state.heldSample;
        }
        
        // Apply DC offset (properly centered at 0.5 = no offset)
        const float dcOffset = (m_dcOffset.current - 0.5f) * 0.1f;
        wetSignal += dcOffset;
        
        // Mix dry and wet
        data[i] = drySignal * (1.0f - m_mix.current) + wetSignal * m_mix.current;
    }
}

// ==================== Process With Oversampling ====================
void BitCrusher::processWithOversampling(float* data, int numSamples, 
                                        ChannelState& state, int channel) {
    constexpr int factor = 4;
    
    // Upsample
    for (int i = 0; i < numSamples; ++i) {
        // Insert zeros between samples
        m_oversampler.upsampleBuffer[i * factor] = data[i] * factor;
        for (int j = 1; j < factor; ++j) {
            m_oversampler.upsampleBuffer[i * factor + j] = 0.0f;
        }
    }
    
    // Apply anti-aliasing filter to upsampled signal
    applyButterworthFilter(m_oversampler.upsampleBuffer.data(), 
                          numSamples * factor, 
                          m_oversampler.upsampleFilter);
    
    // Process at higher sample rate
    for (int i = 0; i < numSamples * factor; ++i) {
        float sample = m_oversampler.upsampleBuffer[i];
        const float drySample = sample;
        
        // Apply gate threshold
        if (m_gateThreshold.current > 0.0f) {
            if (std::abs(sample) < m_gateThreshold.current * 0.1f) {
                sample = 0.0f;
            }
        }
        
        // Sample rate reduction at oversampled rate
        const float reductionFactor = std::max(1.0f, m_sampleRateReduction.current);
        const float increment = 1.0f / (reductionFactor * factor);
        
        state.sampleCounter += increment;
        
        if (state.sampleCounter >= 1.0f) {
            // Add jitter
            if (m_jitter.current > 0.0f) {
                float jitter = m_distribution(m_rng) * m_jitter.current * 0.1f;
                state.sampleCounter = std::abs(jitter);
            } else {
                state.sampleCounter -= 1.0f;
            }
            
            // Apply bit reduction
            sample = quantizeProperly(sample, m_bitDepth.current, state);
            state.heldSample = sample;
        } else {
            sample = state.heldSample;
        }
        
        // Apply DC offset
        const float dcOffset = (m_dcOffset.current - 0.5f) * 0.1f;
        sample += dcOffset;
        
        // Mix at oversampled rate
        m_oversampler.downsampleBuffer[i] = drySample * (1.0f - m_mix.current) + 
                                           sample * m_mix.current;
    }
    
    // Apply anti-aliasing filter before downsampling
    applyButterworthFilter(m_oversampler.downsampleBuffer.data(), 
                          numSamples * factor, 
                          m_oversampler.downsampleFilter);
    
    // Downsample (decimation)
    for (int i = 0; i < numSamples; ++i) {
        data[i] = m_oversampler.downsampleBuffer[i * factor];
    }
}

// ==================== Proper Quantization ====================
float BitCrusher::quantizeProperly(float input, float bits, ChannelState& state) {
    if (bits >= 24.0f) return input; // No quantization needed
    
    // Clamp input to valid range
    input = std::max(-1.0f, std::min(1.0f, input));
    
    // Apply dither before quantization if enabled
    if (m_dither.current > 0.0f) {
        input = applyProperDither(input, bits, state);
    }
    
    // Calculate quantization levels (2^bits total levels)
    const float levels = std::pow(2.0f, bits);
    const float halfLevels = levels * 0.5f;
    
    // Scale to quantization range
    float scaled = input * halfLevels;
    
    // Quantize
    float quantized = std::round(scaled) / halfLevels;
    
    // Ensure output is in valid range
    return std::max(-1.0f, std::min(1.0f, quantized));
}

// ==================== Proper Dithering ====================
float BitCrusher::applyProperDither(float input, float bits, ChannelState& state) {
    // TPDF (Triangular Probability Density Function) dither
    // This completely decorrelates quantization noise from the signal
    
    const float lsb = 1.0f / std::pow(2.0f, bits - 1);
    
    // Generate two uniform random values
    float r1 = m_distribution(m_rng) * 0.5f;
    float r2 = m_distribution(m_rng) * 0.5f;
    
    // Triangular distribution (sum of two uniform)
    float triangularDither = (r1 + r2) * lsb * m_dither.current;
    
    // Apply noise shaping (first-order)
    float shapedDither = triangularDither - state.ditherError * 0.5f;
    float ditheredInput = input + shapedDither;
    
    // Store error for next sample
    state.ditherError = shapedDither;
    
    return ditheredInput;
}

// ==================== Butterworth Filter ====================
void BitCrusher::applyButterworthFilter(float* data, int numSamples, 
                                       Oversampler::AAFilter& filter) {
    // 4th-order Butterworth using Direct Form II
    for (int i = 0; i < numSamples; ++i) {
        float input = data[i];
        
        // Apply filter
        float output = m_oversampleCoeffs.a0 * input + 
                      m_oversampleCoeffs.a1 * filter.x[0] + 
                      m_oversampleCoeffs.a2 * filter.x[1] + 
                      m_oversampleCoeffs.a3 * filter.x[2] + 
                      m_oversampleCoeffs.a4 * filter.x[3] -
                      m_oversampleCoeffs.b1 * filter.y[0] - 
                      m_oversampleCoeffs.b2 * filter.y[1] - 
                      m_oversampleCoeffs.b3 * filter.y[2] - 
                      m_oversampleCoeffs.b4 * filter.y[3];
        
        // Update delay lines
        filter.x[3] = filter.x[2];
        filter.x[2] = filter.x[1];
        filter.x[1] = filter.x[0];
        filter.x[0] = input;
        
        filter.y[3] = filter.y[2];
        filter.y[2] = filter.y[1];
        filter.y[1] = filter.y[0];
        filter.y[0] = output;
        
        // Denormal protection
        if (std::abs(output) < 1e-30f) output = 0.0f;
        
        data[i] = output;
    }
}

// ==================== Update Parameters ====================
void BitCrusher::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) {
        // Bits: 0.0 = 24-bit (no crushing), 1.0 = 1-bit (maximum crushing)
        float value = params.at(0);
        m_bitDepth.target = 24.0f - value * 23.0f;
    }
    if (params.count(1)) {
        // Downsample: 0.0 = no reduction, 1.0 = 100x reduction
        float value = params.at(1);
        // Ensure we never get 0 which would cause division issues
        m_sampleRateReduction.target = 1.0f + value * 99.0f;
    }
    if (params.count(2)) {
        m_aliasing.target = params.at(2);
    }
    if (params.count(3)) {
        m_jitter.target = params.at(3);
    }
    if (params.count(4)) {
        // DC Offset: 0.5 = no offset (centered)
        m_dcOffset.target = params.at(4);
    }
    if (params.count(5)) {
        m_gateThreshold.target = params.at(5);
    }
    if (params.count(6)) {
        m_dither.target = params.at(6);
    }
    if (params.count(7)) {
        m_mix.target = params.at(7);
    }
}

// ==================== Parameter Names ====================
juce::String BitCrusher::getParameterName(int index) const {
    switch (index) {
        case 0: return "Bits";
        case 1: return "Downsample";
        case 2: return "Aliasing";
        case 3: return "Jitter";
        case 4: return "DC Offset";
        case 5: return "Gate";
        case 6: return "Dither";
        case 7: return "Mix";
        default: return "";
    }
}

// ==================== Helper Functions ====================
float BitCrusher::quantize(float input, float bits) {
    // Legacy function - redirects to proper implementation
    ChannelState dummyState;
    return quantizeProperly(input, bits, dummyState);
}

float BitCrusher::quantizeWithAging(float input, float bits, float aging) {
    // Removed aging nonsense - just do proper quantization
    ChannelState dummyState;
    return quantizeProperly(input, bits, dummyState);
}

float BitCrusher::applyDither(float input, float ditherAmount, ChannelState& state) {
    // Legacy function - redirects to proper implementation
    return applyProperDither(input, m_bitDepth.current, state);
}

float BitCrusher::processDCBlocker(float input, ChannelState& state) {
    // Not used anymore - we use the DCBlocker struct
    return input;
}

float BitCrusher::softClip(float input) {
    // Simple soft clipping
    return std::tanh(input);
}

float BitCrusher::softClipWithAging(float input, float aging) {
    // Removed aging nonsense
    return softClip(input);
}
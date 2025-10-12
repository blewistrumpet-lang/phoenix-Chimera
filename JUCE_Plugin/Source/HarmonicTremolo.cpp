#include "HarmonicTremolo.h"
#include "DspEngineUtilities.h"
#include <cstring>
#include <cstdint>  // For uint32_t in denormal handling
#include <functional>  // For std::function
// Platform-specific SIMD includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif

HarmonicTremolo::HarmonicTremolo() {
    // Initialize default parameter values
    m_rate.reset(0.3f);
    m_depth.reset(0.5f);
    m_harmonics.reset(0.4f);
    m_stereoPhase.reset(0.25f);
}

void HarmonicTremolo::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Initialize smoothing for all parameters
    const float smoothTime = PARAMETER_SMOOTH_MS;
    m_rate.setSmoothingTime(smoothTime, sampleRate);
    m_depth.setSmoothingTime(smoothTime, sampleRate);
    m_harmonics.setSmoothingTime(smoothTime * 2.0f, sampleRate); // Slower for crossover
    m_stereoPhase.setSmoothingTime(smoothTime, sampleRate);
    
    // Initialize per-channel processing
    const int numChannels = 2;
    m_lfoState.resize(numChannels);
    m_crossover.resize(numChannels);
    m_oversampling.resize(numChannels);
    m_tubeState.resize(numChannels);
    
    // Initialize LFO tables
    for (auto& lfo : m_lfoState) {
        lfo.initializeTables();
    }
    
    // Initialize oversampling
    for (auto& os : m_oversampling) {
        os.initialize(OVERSAMPLE_FACTOR);
    }
    
    // Update crossover coefficients
    updateCrossoverCoefficients();
    
    // Reset all DSP states
    reset();
}

void HarmonicTremolo::reset() {
    // Reset LFO phases with stereo offset
    for (size_t ch = 0; ch < m_lfoState.size(); ++ch) {
        m_lfoState[ch].phase = ch * m_stereoPhase.currentValue;
        m_lfoState[ch].previousValue = 0.0f;
    }
    
    // Reset filters
    for (auto& xover : m_crossover) {
        xover.reset();
    }
    
    // Reset oversampling filters
    for (auto& os : m_oversampling) {
        os.upsampler.reset();
        os.downsampler.reset();
    }
    
    // Reset tube states
    for (auto& tube : m_tubeState) {
        tube.reset();
    }
}

void HarmonicTremolo::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Process in blocks for efficiency
    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        for (int offset = 0; offset < numSamples; offset += BLOCK_SIZE) {
            const int blockSamples = std::min(BLOCK_SIZE, numSamples - offset);
            
            // Check if we have SSE support
            #ifdef __SSE2__
            processBlockSSE(channelData + offset, blockSamples, channel);
            #else
            processBlock(channelData + offset, blockSamples, channel);
            #endif
        }
    }
    
    scrubBuffer(buffer);
}

void HarmonicTremolo::processBlock(float* channelData, int numSamples, int channel) {
    // Process each sample
    for (int i = 0; i < numSamples; ++i) {
        channelData[i] = processSample(channelData[i], channel);
    }
}

void HarmonicTremolo::processBlockSSE(float* channelData, int numSamples, int channel) {
#ifdef __SSE2__
    // SSE optimized version for 4-sample vectors
    const int vectorSamples = numSamples & ~3; // Round down to multiple of 4
    
    // Process vectorized samples
    for (int i = 0; i < vectorSamples; i += 4) {
        __m128 samples = _mm_loadu_ps(&channelData[i]);
        
        // Process each sample individually (due to state dependencies)
        alignas(16) float temp[4];
        _mm_store_ps(temp, samples);
        
        for (int j = 0; j < 4; ++j) {
            temp[j] = processSample(temp[j], channel);
        }
        
        samples = _mm_load_ps(temp);
        _mm_storeu_ps(&channelData[i], samples);
    }
    
    // Process remaining samples
    for (int i = vectorSamples; i < numSamples; ++i) {
        channelData[i] = processSample(channelData[i], channel);
    }
#else
    // Fallback to scalar processing
    processBlock(channelData, numSamples, channel);
#endif
}

float HarmonicTremolo::processSample(float input, int channel) {
    // Get smoothed parameters
    const float depth = m_depth.getNextValue();
    const float rate = m_rate.getNextValue();
    const float stereoPhase = m_stereoPhase.getNextValue();
    
    // Apply tube coloration with optional oversampling
    float colored;
    if (depth > 0.3f) {  // Only oversample when depth is significant
        colored = m_oversampling[channel].processWithOversampling(
            input, 
            [this, channel](float x) { return m_tubeState[channel].process(x); }
        );
    } else {
        colored = m_tubeState[channel].process(input);
    }
    
    // Split into frequency bands
    float lowBand, highBand;
    m_crossover[channel].process(colored, lowBand, highBand);
    
    // Generate LFO
    const float rateHz = 0.1f + rate * 9.9f; // 0.1Hz to 10Hz (proper tremolo range)
    const float phaseOffset = channel == 1 ? stereoPhase : 0.0f;
    float lfo = m_lfoState[channel].process(rateHz, m_sampleRate, phaseOffset);
    
    // Apply complementary modulation
    float lowMod = 1.0f + lfo * depth;
    float highMod = 1.0f - lfo * depth;
    
    // Clamp modulation to prevent negative values
    lowMod = std::max(0.0f, std::min(2.0f, lowMod));
    highMod = std::max(0.0f, std::min(2.0f, highMod));
    
    // Apply modulation and recombine
    float output = lowBand * lowMod + highBand * highMod;
    
    // Apply slight gain compensation
    const float compensation = 1.0f / (1.0f + depth * 0.25f);
    output *= compensation;
    
    // Final denormal flush
    return HarmonicTremolo::flushDenormal(output);
}

void HarmonicTremolo::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(0);
    if (it != params.end()) {
        m_rate.setTarget(it->second);
    }
    
    it = params.find(1);
    if (it != params.end()) {
        m_depth.setTarget(it->second);
    }
    
    it = params.find(2);
    if (it != params.end()) {
        m_harmonics.setTarget(it->second);
        // Update coefficients immediately for responsiveness
        updateCrossoverCoefficients();
    }
    
    it = params.find(3);
    if (it != params.end()) {
        m_stereoPhase.setTarget(it->second);
    }
}

juce::String HarmonicTremolo::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Harmonics";
        case 3: return "Stereo Phase";
        default: return "";
    }
}

void HarmonicTremolo::updateCrossoverCoefficients() {
    const float freq = 200.0f + m_harmonics.getNextValue() * 1800.0f;
    
    for (auto& xover : m_crossover) {
        xover.updateCoefficients(freq, m_sampleRate);
    }
}

// LFOState implementation
void HarmonicTremolo::LFOState::initializeTables() {
    // Generate band-limited triangle wave using additive synthesis
    for (int i = 0; i < LFO_TABLE_SIZE; ++i) {
        float phase = 2.0f * M_PI * i / LFO_TABLE_SIZE;
        
        // Band-limited triangle using additive synthesis
        float triangle = 0.0f;
        const int maxHarmonic = 31; // Limit harmonics for LFO rates up to 20Hz
        
        for (int h = 1; h <= maxHarmonic; h += 2) {
            float amplitude = 8.0f / (M_PI * M_PI * h * h);
            amplitude *= (h % 4 == 1) ? 1.0f : -1.0f;
            triangle += amplitude * std::sin(h * phase);
        }
        
        triangleTable[i] = triangle;
    }
}

float HarmonicTremolo::LFOState::process(float rateHz, float sampleRate, float phaseOffset) {
    // Update phase with high precision
    double phaseIncrement = rateHz / sampleRate;
    phase += phaseIncrement;
    
    // Wrap phase using fmod for accuracy
    phase = std::fmod(phase, 1.0);
    
    // Calculate table position with phase offset
    double tablePhase = std::fmod(phase + phaseOffset, 1.0) * LFO_TABLE_SIZE;
    
    // Cubic interpolation indices
    int i0 = static_cast<int>(tablePhase);
    int i1 = (i0 + 1) & (LFO_TABLE_SIZE - 1);
    int i2 = (i0 + 2) & (LFO_TABLE_SIZE - 1);
    int i3 = (i0 + 3) & (LFO_TABLE_SIZE - 1);
    
    float frac = tablePhase - i0;
    
    // Catmull-Rom cubic interpolation
    float y0 = triangleTable[i0];
    float y1 = triangleTable[i1];
    float y2 = triangleTable[i2];
    float y3 = triangleTable[i3];
    
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    
    float value = c0 + frac * (c1 + frac * (c2 + frac * c3));
    
    // Apply slight smoothing to prevent clicks
    const float smoothing = 0.95f;
    value = previousValue + (value - previousValue) * (1.0f - smoothing);
    previousValue = value;
    
    return value * 0.5f; // Scale to reasonable range
}

// LinkwitzRileyCrossover implementation
void HarmonicTremolo::LinkwitzRileyCrossover::updateCoefficients(
    float freq, float sampleRate) {
    
    // Pre-warp frequency for bilinear transform
    float wc = 2.0f * sampleRate * std::tan(M_PI * freq / sampleRate);
    float wc2 = wc * wc;
    float sqrt2 = std::sqrt(2.0f);
    float norm = 1.0f / (wc2 + sqrt2 * wc * sampleRate + sampleRate * sampleRate);
    
    // Butterworth lowpass coefficients
    lowCoeffs.b0 = wc2 * norm;
    lowCoeffs.b1 = 2.0f * wc2 * norm;
    lowCoeffs.b2 = wc2 * norm;
    lowCoeffs.a1 = 2.0f * (wc2 - sampleRate * sampleRate) * norm;
    lowCoeffs.a2 = (wc2 - sqrt2 * wc * sampleRate + sampleRate * sampleRate) * norm;
    
    // Butterworth highpass coefficients
    float s2 = sampleRate * sampleRate;
    highCoeffs.b0 = s2 * norm;
    highCoeffs.b1 = -2.0f * s2 * norm;
    highCoeffs.b2 = s2 * norm;
    highCoeffs.a1 = lowCoeffs.a1; // Same denominator
    highCoeffs.a2 = lowCoeffs.a2;
}

void HarmonicTremolo::LinkwitzRileyCrossover::process(
    float input, float& low, float& high) {
    
    // Process through cascaded biquads for 4th order response
    float low1 = lowpass1.process(input, lowCoeffs);
    low = lowpass2.process(low1, lowCoeffs);
    
    float high1 = highpass1.process(input, highCoeffs);
    high = highpass2.process(high1, highCoeffs);
}

void HarmonicTremolo::LinkwitzRileyCrossover::reset() {
    lowpass1.reset();
    lowpass2.reset();
    highpass1.reset();
    highpass2.reset();
}

// OversamplingProcessor implementation
void HarmonicTremolo::OversamplingProcessor::PolyphaseFilter::designFilter(
    float cutoff, bool isUpsampler) {
    
    // Design linear-phase FIR filter using Kaiser window
    const float beta = 8.6f; // Kaiser beta for ~96dB stopband
    const float fc = cutoff * 0.5f; // Normalized cutoff frequency
    
    // Generate sinc function
    std::array<float, FIR_ORDER> sinc;
    for (int i = 0; i < FIR_ORDER; ++i) {
        float n = i - (FIR_ORDER - 1) * 0.5f;
        if (std::abs(n) < 1e-6f) {
            sinc[i] = 2.0f * fc;
        } else {
            sinc[i] = std::sin(2.0f * M_PI * fc * n) / (M_PI * n);
        }
    }
    
    // Apply Kaiser window
    for (int i = 0; i < FIR_ORDER; ++i) {
        float n = 2.0f * i / (FIR_ORDER - 1) - 1.0f;
        float kaiser = 0.0f;
        
        // Modified Bessel function I0 approximation
        float x = beta * std::sqrt(1.0f - n * n);
        float sum = 1.0f;
        float term = 1.0f;
        
        for (int k = 1; k < 20; ++k) {
            term *= (x * x) / (4.0f * k * k);
            sum += term;
        }
        
        kaiser = sum / sum; // Normalized
        coefficients[i] = sinc[i] * kaiser;
    }
    
    // Normalize filter
    float sum = 0.0f;
    for (float coeff : coefficients) {
        sum += coeff;
    }
    
    float gain = isUpsampler ? OVERSAMPLE_FACTOR : 1.0f;
    for (float& coeff : coefficients) {
        coeff *= gain / sum;
    }
}

float HarmonicTremolo::OversamplingProcessor::PolyphaseFilter::process(float input) {
    // Write to circular buffer
    delayLine[writeIndex] = input;
    writeIndex = (writeIndex + 1) & (FIR_ORDER - 1);
    
    // Convolution
    float output = 0.0f;
    int readIndex = writeIndex;
    
    // Unroll loop for better performance
    for (int i = 0; i < FIR_ORDER; i += 4) {
        output += coefficients[i] * delayLine[readIndex];
        readIndex = (readIndex + 1) & (FIR_ORDER - 1);
        
        output += coefficients[i + 1] * delayLine[readIndex];
        readIndex = (readIndex + 1) & (FIR_ORDER - 1);
        
        output += coefficients[i + 2] * delayLine[readIndex];
        readIndex = (readIndex + 1) & (FIR_ORDER - 1);
        
        output += coefficients[i + 3] * delayLine[readIndex];
        readIndex = (readIndex + 1) & (FIR_ORDER - 1);
    }
    
    return output;
}

void HarmonicTremolo::OversamplingProcessor::PolyphaseFilter::reset() {
    std::fill(delayLine.begin(), delayLine.end(), 0.0f);
    writeIndex = 0;
}

void HarmonicTremolo::OversamplingProcessor::initialize(int factor) {
    upsampleBuffer.resize(factor);
    downsampleBuffer.resize(factor);
    
    // Design anti-aliasing filters
    upsampler.designFilter(0.45f, true);   // 45% of Nyquist for steep transition
    downsampler.designFilter(0.45f, false);
}

float HarmonicTremolo::OversamplingProcessor::processWithOversampling(
    float input, std::function<float(float)> processor) {
    
    // Upsample
    upsampleBuffer[0] = upsampler.process(input * OVERSAMPLE_FACTOR);
    for (int i = 1; i < OVERSAMPLE_FACTOR; ++i) {
        upsampleBuffer[i] = upsampler.process(0.0f);
    }
    
    // Process at higher sample rate
    for (int i = 0; i < OVERSAMPLE_FACTOR; ++i) {
        downsampleBuffer[i] = processor(upsampleBuffer[i]);
    }
    
    // Downsample
    float output = downsampler.process(downsampleBuffer[0]);
    for (int i = 1; i < OVERSAMPLE_FACTOR; ++i) {
        downsampler.process(downsampleBuffer[i]); // Update filter state
    }
    
    return output;
}

// TubeState implementation
float HarmonicTremolo::TubeState::process(float input) {
    // DC blocker first
    float dcBlocked = input - dcBlockerState + 0.995f * dcBlockerState;
    dcBlockerState = dcBlocked;
    
    // Asymmetric tube saturation
    const float drive = 1.5f;
    float driven = dcBlocked * drive;
    float shaped;
    
    if (driven > 0.0f) {
        // Softer clipping for positive samples
        shaped = std::tanh(driven * 0.7f) / 0.7f;
    } else {
        // Harder clipping for negative samples (tube asymmetry)
        shaped = std::tanh(driven * 0.9f) / 0.9f;
    }
    
    // Add even harmonics
    float harmonics = shaped + 0.02f * shaped * shaped - 0.005f * shaped * shaped * shaped;
    
    // Warmth filter (gentle high-frequency rolloff)
    const float warmthCutoff = 0.15f;
    warmthFilterState += warmthCutoff * (harmonics - warmthFilterState);
    
    return warmthFilterState / drive;
}
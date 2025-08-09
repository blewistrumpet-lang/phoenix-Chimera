#include "LadderFilter.h"
#include <cstring>
// Platform-specific SIMD includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Initialize static members
std::array<float, LadderFilter::SaturationModel::LUT_SIZE> LadderFilter::SaturationModel::saturationLUT;
std::array<float, LadderFilter::SaturationModel::LUT_SIZE> LadderFilter::SaturationModel::vintageLUT;

LadderFilter::LadderFilter() {
    // Initialize parameters with default values
    m_cutoffFreq.reset(0.5f);
    m_resonance.reset(0.3f);
    m_drive.reset(0.2f);
    m_filterType.reset(0.0f);  // Lowpass
    m_asymmetry.reset(0.0f);
    m_vintageMode.reset(0.0f);
    m_mix.reset(1.0f);
    
    // Initialize lookup tables
    SaturationModel::initializeLUTs();
}

void LadderFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    
    // Set parameter smoothing times
    m_cutoffFreq.setSmoothingTime(5.0f, sampleRate);    // Fast for cutoff
    m_resonance.setSmoothingTime(10.0f, sampleRate);    // Medium for resonance
    m_drive.setSmoothingTime(50.0f, sampleRate);        // Slower for drive
    m_filterType.setSmoothingTime(20.0f, sampleRate);   // Medium for type
    m_asymmetry.setSmoothingTime(100.0f, sampleRate);   // Slow for asymmetry
    m_vintageMode.setSmoothingTime(200.0f, sampleRate); // Very slow for mode
    m_mix.setSmoothingTime(20.0f, sampleRate);          // Medium for mix
    
    // Initialize oversamplers
    for (auto& oversampler : m_oversamplers) {
        oversampler.initialize();
    }
    
    // Reset all states
    reset();
    
    // Initialize component spread
    bool isVintage = m_vintageMode.getCurrentValue() > 0.5f;
    for (auto& channel : m_channelStates) {
        m_componentModel.randomizeComponents(channel.componentSpread, isVintage);
    }
    
    // Update coefficients
    m_coeffs.update(m_cutoffFreq.getCurrentValue(), m_resonance.getCurrentValue(), 
                    isVintage, sampleRate, OVERSAMPLE_FACTOR);
    
    // Track vintage mode for smooth transitions
    m_lastVintageMode = m_vintageMode.getCurrentValue();
}

void LadderFilter::reset() {
    // Reset all channel states
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
    
    // Reset oversamplers
    for (auto& oversampler : m_oversamplers) {
        oversampler.reset();
    }
    
    // Reset thermal model
    m_thermalModel.reset();
    
    // Reset coefficient tracking
    m_lastCutoff = -1.0f;
    m_lastResonance = -1.0f;
    m_lastVintageMode = -1.0f;
}

void LadderFilter::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;  // RAII denormal protection for entire process block
    
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Update thermal model once per block
    m_thermalModel.update(m_sampleRate);
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        // Process in blocks for efficiency
        for (int offset = 0; offset < numSamples; offset += BLOCK_SIZE) {
            int blockSamples = std::min(BLOCK_SIZE, numSamples - offset);
            
            #ifdef __SSE2__
            processBlockSSE(channelData + offset, blockSamples, channel);
            #else
            processBlock(channelData + offset, blockSamples, channel);
            #endif
        }
    }
    
    // Scrub buffer for NaN/Inf protection at end of processing
    scrubBuffer(buffer);
}

void LadderFilter::processBlock(float* channelData, int numSamples, int channel) {
    // Get current parameter values
    float cutoff = m_cutoffFreq.getNextValue();
    float resonance = m_resonance.getNextValue();
    float drive = m_drive.getNextValue();
    float filterType = m_filterType.getNextValue();
    float asymmetry = m_asymmetry.getNextValue();
    float vintageMode = m_vintageMode.getNextValue();
    float mix = m_mix.getNextValue();
    
    // Update coefficients if needed
    if (std::abs(cutoff - m_lastCutoff) > 0.001f || 
        std::abs(resonance - m_lastResonance) > 0.001f) {
        m_coeffs.update(cutoff, resonance, vintageMode > 0.5f, 
                       m_sampleRate, OVERSAMPLE_FACTOR);
        m_lastCutoff = cutoff;
        m_lastResonance = resonance;
        
        // Smoothly handle component spread changes on vintage mode transitions
        if (std::abs(vintageMode - m_lastVintageMode) > 0.1f) {
            // Only update if we've crossed a significant threshold
            bool shouldBeVintage = vintageMode > 0.5f;
            bool wasVintage = m_lastVintageMode > 0.5f;
            
            if (shouldBeVintage != wasVintage) {
                // Smooth transition: blend between old and new component values
                std::array<float, 4> newSpread;
                m_componentModel.randomizeComponents(newSpread, shouldBeVintage);
                
                // Blend with existing values for smooth transition
                auto& currentSpread = m_channelStates[channel].componentSpread;
                float blendFactor = 0.1f; // Slow blend to avoid clicks
                
                for (int i = 0; i < 4; ++i) {
                    currentSpread[i] = currentSpread[i] * (1.0f - blendFactor) + 
                                     newSpread[i] * blendFactor;
                }
                
                m_lastVintageMode = vintageMode;
            }
        }
    }
    
    // Process each sample
    for (int i = 0; i < numSamples; ++i) {
        float dry = channelData[i];
        float wet = processSample(dry, channel);
        channelData[i] = dry * (1.0f - mix) + wet * mix;
    }
}

float LadderFilter::processSample(float input, int channel) {
    auto& state = m_channelStates[channel];
    
    // DC blocking
    float dcBlocked = state.processDCBlocker(input);
    
    // Process with oversampling
    return m_oversamplers[channel].process(dcBlocked, 
        [this, channel](float x) { return processLadderCore(x, channel); });
}

float LadderFilter::processLadderCore(float input, int channel) {
    auto& state = m_channelStates[channel];
    
    // Get current parameters
    float drive = m_drive.getCurrentValue();
    float asymmetry = m_asymmetry.getCurrentValue();
    float filterType = m_filterType.getCurrentValue();
    bool isVintage = m_vintageMode.getCurrentValue() > 0.5f;
    
    // Apply input saturation
    float saturatedInput;
    if (isVintage) {
        saturatedInput = SaturationModel::vintageSaturation(input, 1.0f + drive * 4.0f);
    } else {
        saturatedInput = SaturationModel::transistorSaturation(input, 1.0f + drive * 4.0f, asymmetry);
    }
    
    // Solve zero-delay feedback
    float ladderOutput = solveZeroDelayFeedback(saturatedInput, state, m_coeffs.g, m_coeffs.k);
    
    // Calculate filter response
    float output = calculateFilterResponse(state, saturatedInput, filterType);
    
    // Apply gain compensation
    output *= m_coeffs.gCompensation;
    
    // Final soft limiting
    output = fastTanh(output * 0.8f) / 0.8f;
    
    return DSPUtils::flushDenorm(output);
}

float LadderFilter::solveZeroDelayFeedback(float input, ChannelState& state, float g, float k) {
    // Newton-Raphson solver for zero-delay feedback
    const int MAX_ITERATIONS = 3;
    float y = state.previousOutput;
    
    for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
        // Calculate feedback
        float feedback = k * fastTanh(y * 0.8f);
        float x = input - feedback;
        
        // Process through stages with component variations
        float stageInput = x;
        for (int s = 0; s < 4; ++s) {
            float thermalFactor = m_thermalModel.getDriftForStage(s);
            float componentFactor = state.componentSpread[s];
            float effectiveG = g * thermalFactor * componentFactor;
            
            // Clamp for stability
            effectiveG = std::clamp(effectiveG, 0.0f, 0.99f);
            
            // Process stage with saturation
            stageInput = state.stages[s].process(stageInput, effectiveG, 
                                                m_coeffs.stageSaturation[s]);
            stageInput = DSPUtils::flushDenorm(stageInput);
        }
        
        // Update estimate
        y = stageInput;
    }
    
    state.previousOutput = y;
    return y;
}

float LadderFilter::calculateFilterResponse(const ChannelState& state, float input, float filterType) {
    // Get outputs from each stage
    float y1 = state.stages[0].state;
    float y2 = state.stages[1].state;
    float y3 = state.stages[2].state;
    float y4 = state.stages[3].state;
    
    // Calculate different filter responses
    float lp24 = y4;                    // 24dB/oct lowpass
    float lp12 = y2;                    // 12dB/oct lowpass
    float bp12 = y2 - y4;               // 12dB/oct bandpass
    float bp6 = y1 - y2;                // 6dB/oct bandpass
    float hp24 = input - y4;            // 24dB/oct highpass
    float hp12 = input - y2;            // 12dB/oct highpass
    float notch = input - 2.0f * y2 + y4; // Notch
    float allpass = input - 4.0f * y2 + 6.0f * y3 - 4.0f * y4; // Allpass
    
    // Smooth morphing between filter types
    if (filterType < 0.125f) {
        // LP24 to LP12
        float morph = filterType * 8.0f;
        return lp24 * (1.0f - morph) + lp12 * morph;
    } else if (filterType < 0.25f) {
        // LP12 to BP12
        float morph = (filterType - 0.125f) * 8.0f;
        return lp12 * (1.0f - morph) + bp12 * morph;
    } else if (filterType < 0.375f) {
        // BP12 to BP6
        float morph = (filterType - 0.25f) * 8.0f;
        return bp12 * (1.0f - morph) + bp6 * morph;
    } else if (filterType < 0.5f) {
        // BP6 to HP12
        float morph = (filterType - 0.375f) * 8.0f;
        return bp6 * (1.0f - morph) + hp12 * morph;
    } else if (filterType < 0.625f) {
        // HP12 to HP24
        float morph = (filterType - 0.5f) * 8.0f;
        return hp12 * (1.0f - morph) + hp24 * morph;
    } else if (filterType < 0.75f) {
        // HP24 to Notch
        float morph = (filterType - 0.625f) * 8.0f;
        return hp24 * (1.0f - morph) + notch * morph;
    } else if (filterType < 0.875f) {
        // Notch to Allpass
        float morph = (filterType - 0.75f) * 8.0f;
        return notch * (1.0f - morph) + allpass * morph;
    } else {
        // Allpass to LP24 (full circle)
        float morph = (filterType - 0.875f) * 8.0f;
        return allpass * (1.0f - morph) + lp24 * morph;
    }
}

#ifdef __SSE2__
void LadderFilter::processBlockSSE(float* channelData, int numSamples, int channel) {
    // Process 4 samples at a time where possible
    const int simdSamples = numSamples & ~3;
    
    // Process SIMD samples
    for (int i = 0; i < simdSamples; i += 4) {
        __m128 samples = _mm_loadu_ps(&channelData[i]);
        
        // Process each sample (can't vectorize filter stages due to feedback)
        alignas(16) float temp[4];
        _mm_store_ps(temp, samples);
        
        for (int j = 0; j < 4; ++j) {
            temp[j] = processSample(temp[j], channel);
        }
        
        samples = _mm_load_ps(temp);
        _mm_storeu_ps(&channelData[i], samples);
    }
    
    // Process remaining samples
    for (int i = simdSamples; i < numSamples; ++i) {
        channelData[i] = processSample(channelData[i], channel);
    }
}
#endif

// FilterCoefficients implementation
void LadderFilter::FilterCoefficients::update(float cutoffNorm, float resonance, 
                                            bool vintageMode, double sampleRate, int oversampleFactor) {
    // Calculate cutoff frequency with proper scaling
    float cutoffHz = LadderFilter::MIN_CUTOFF * std::pow(LadderFilter::MAX_CUTOFF / LadderFilter::MIN_CUTOFF, cutoffNorm);
    
    // Account for oversampling
    float effectiveSR = sampleRate * oversampleFactor;
    
    // Pre-warp frequency for bilinear transform
    float wc = 2.0f * effectiveSR * std::tan(M_PI * cutoffHz / effectiveSR);
    
    // Calculate g coefficient (integration rate)
    g = wc / (wc + 2.0f * effectiveSR);
    
    // Calculate feedback amount with compensation
    if (vintageMode) {
        // Vintage mode - musical self-oscillation
        // Clamp resonance to safe range to prevent instability
        float safeResonance = clampSafe(resonance, 0.0f, 0.95f);
        k = safeResonance * safeResonance * 4.1f;
        
        // Gain compensation for vintage mode
        gCompensation = 1.0f + k * 0.1f;
        
        // Stage saturations for vintage character
        stageSaturation[0] = 1.3f;
        stageSaturation[1] = 1.2f;
        stageSaturation[2] = 1.1f;
        stageSaturation[3] = 1.0f;
    } else {
        // Modern mode - controlled resonance
        // Clamp resonance to safe range to prevent instability
        float safeResonance = clampSafe(resonance, 0.0f, 0.95f);
        k = safeResonance * 4.0f;
        
        // Better gain compensation for modern mode
        gCompensation = std::sqrt(1.0f + k * k * 0.05f);
        
        // Cleaner saturation for modern mode
        stageSaturation[0] = 1.1f;
        stageSaturation[1] = 1.05f;
        stageSaturation[2] = 1.02f;
        stageSaturation[3] = 1.0f;
    }
    
    // Ensure stability
    ensureStability();
}

void LadderFilter::FilterCoefficients::ensureStability() {
    // Limit g to prevent numerical instability
    g = clampSafe(g, 0.0f, 0.98f);
    
    // Dynamic k limiting based on g (Nyquist stability criterion)
    float maxK = 4.0f * (1.0f - g) / (1.0f + g);
    k = clampSafe(k, 0.0f, maxK * 0.95f); // 5% safety margin, clamped to safe range
    
    // Clamp feedback parameter k to absolute safe range (-0.95 to 0.95)
    k = clampSafe(k, -0.95f, 0.95f);
    
    // Adjust input saturation based on resonance
    inputSaturation = clampSafe(1.0f + k * 0.2f, 0.1f, 10.0f);
}

// SaturationModel implementation
void LadderFilter::SaturationModel::initializeLUTs() {
    // Generate saturation lookup tables
    for (int i = 0; i < LUT_SIZE; ++i) {
        float x = (i - LUT_SIZE / 2) / float(LUT_SIZE / 8); // -4 to +4 range
        
        // Transistor saturation LUT
        saturationLUT[i] = transistorSaturation(x, 1.0f, 0.0f);
        
        // Vintage saturation LUT
        vintageLUT[i] = vintageSaturation(x, 1.0f);
    }
}

float LadderFilter::SaturationModel::transistorSaturation(float input, float drive, float asymmetry) {
    // Realistic transistor model based on Ebers-Moll equations
    float v = input * drive;
    
    // Clamp to prevent numerical overflow
    v = std::clamp(v, -4.0f, 4.0f);
    
    // Asymmetric saturation
    float pos_factor = 1.0f / (1.0f + asymmetry * 0.3f);
    float neg_factor = 1.0f / (1.0f - asymmetry * 0.5f);
    
    if (v > 0.0f) {
        // Positive saturation (softer)
        float vt = v * pos_factor;
        float exp_vt = std::exp(std::min(vt / LadderFilter::THERMAL_VOLTAGE, 10.0f));
        return (exp_vt - 1.0f) / (std::exp(1.0f / LadderFilter::THERMAL_VOLTAGE) - 1.0f) / drive;
    } else {
        // Negative saturation (harder)
        float vt = v * neg_factor;
        float exp_vt = std::exp(std::max(vt / LadderFilter::THERMAL_VOLTAGE, -10.0f));
        return (exp_vt - 1.0f) / (std::exp(-1.0f / LadderFilter::THERMAL_VOLTAGE) - 1.0f) / drive;
    }
}

float LadderFilter::SaturationModel::vintageSaturation(float input, float drive) {
    // Moog-style saturation with even harmonics
    float v = input * drive;
    
    // Polynomial waveshaping for vintage character
    float v2 = v * v;
    float v3 = v2 * v;
    
    // Coefficients tuned to match vintage Moog ladder
    float output = v - 0.15f * v3 + 0.05f * v2;
    
    // Soft limiting
    output = std::tanh(output * 0.7f) / 0.7f;
    
    return output / drive;
}

float LadderFilter::SaturationModel::lookupSaturation(float input, bool vintage) {
    // Fast LUT-based saturation
    float scaled = (input + 4.0f) * (LUT_SIZE / 8.0f);
    int index = std::clamp(int(scaled), 0, LUT_SIZE - 1);
    
    return vintage ? vintageLUT[index] : saturationLUT[index];
}

// Oversampler implementation
void LadderFilter::Oversampler::initialize() {
    upsampler.designFilter(true);
    downsampler.designFilter(false);
    reset();
}

void LadderFilter::Oversampler::reset() {
    upsampler.reset();
    downsampler.reset();
    std::fill(workBuffer.begin(), workBuffer.end(), 0.0f);
}

void LadderFilter::Oversampler::PolyphaseFilter::designFilter(bool isUpsampler) {
    // Design linear-phase FIR filter using Kaiser window
    const float beta = 7.0f; // Kaiser beta for ~80dB stopband
    const float cutoff = 0.45f; // Normalized cutoff
    
    // Generate windowed sinc filter
    for (int i = 0; i < FIR_LENGTH; ++i) {
        float n = i - (FIR_LENGTH - 1) * 0.5f;
        
        // Sinc function
        float sinc;
        if (std::abs(n) < 1e-6f) {
            sinc = 2.0f * cutoff;
        } else {
            sinc = std::sin(2.0f * M_PI * cutoff * n) / (M_PI * n);
        }
        
        // Kaiser window
        float x = 2.0f * i / (FIR_LENGTH - 1) - 1.0f;
        float kaiser = modifiedBessel0(beta * std::sqrt(1.0f - x * x)) / modifiedBessel0(beta);
        
        coefficients[i] = sinc * kaiser;
    }
    
    // Normalize
    float sum = 0.0f;
    for (float c : coefficients) sum += c;
    
    float gain = isUpsampler ? float(LadderFilter::OVERSAMPLE_FACTOR) : 1.0f;
    for (float& c : coefficients) c *= gain / sum;
}

float LadderFilter::Oversampler::PolyphaseFilter::process(float input) {
    // Update circular buffer
    delayLine[writeIndex] = input;
    writeIndex = (writeIndex + 1) % FIR_LENGTH;
    
    // Convolution
    float output = 0.0f;
    int readIndex = writeIndex;
    
    for (int i = 0; i < FIR_LENGTH; ++i) {
        output += coefficients[i] * delayLine[readIndex];
        readIndex = (readIndex + 1) % FIR_LENGTH;
    }
    
    return output;
}

void LadderFilter::Oversampler::PolyphaseFilter::reset() {
    std::fill(delayLine.begin(), delayLine.end(), 0.0f);
    writeIndex = 0;
}

// Modified Bessel function of first kind, order 0 (for Kaiser window)
float LadderFilter::Oversampler::PolyphaseFilter::modifiedBessel0(float x) {
    float sum = 1.0f;
    float term = 1.0f;
    
    for (int k = 1; k < 20; ++k) {
        term *= (x * x) / (4.0f * k * k);
        sum += term;
        if (term < 1e-8f) break;
    }
    
    return sum;
}

// Parameter update
void LadderFilter::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(0);
    if (it != params.end()) m_cutoffFreq.setTarget(it->second);
    
    it = params.find(1);
    if (it != params.end()) m_resonance.setTarget(it->second);
    
    it = params.find(2);
    if (it != params.end()) m_drive.setTarget(it->second);
    
    it = params.find(3);
    if (it != params.end()) m_filterType.setTarget(it->second);
    
    it = params.find(4);
    if (it != params.end()) m_asymmetry.setTarget(it->second);
    
    it = params.find(5);
    if (it != params.end()) m_vintageMode.setTarget(it->second);
    
    it = params.find(6);
    if (it != params.end()) m_mix.setTarget(it->second);
}

juce::String LadderFilter::getParameterName(int index) const {
    switch (index) {
        case 0: return "Cutoff";
        case 1: return "Resonance";
        case 2: return "Drive";
        case 3: return "Filter Type";
        case 4: return "Asymmetry";
        case 5: return "Vintage Mode";
        case 6: return "Mix";
        default: return "";
    }
}
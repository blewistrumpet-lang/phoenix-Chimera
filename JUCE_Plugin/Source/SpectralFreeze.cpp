// SpectralFreeze_Ultimate.cpp
#include "SpectralFreeze.h"
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Platform-specific SIMD support (should match header)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif

//==============================================================================
// SmoothParam Implementation
//==============================================================================
void SpectralFreeze::SmoothParam::update() {
    float t = target.load(std::memory_order_relaxed);
    current = DSPUtils::flushDenorm(current + (t - current) * (1.0f - smoothing));
}

void SpectralFreeze::SmoothParam::setImmediate(float value) {
    target.store(value, std::memory_order_relaxed);
    current = value;
}

void SpectralFreeze::SmoothParam::setSmoothingRate(float timeMs, double sampleRate) {
    smoothing = expf(-1.0f / (timeMs * 0.001f * sampleRate));
}

// Custom DenormalDisabler replaced with DspEngineUtilities DenormalGuard

//==============================================================================
// FFTProcessor Implementation
//==============================================================================
void SpectralFreeze::FFTProcessor::init(int fftOrder) {
    fft = std::make_unique<juce::dsp::FFT>(fftOrder);
    reset();
}

void SpectralFreeze::FFTProcessor::reset() {
    spectrum.fill(std::complex<float>(0.0f, 0.0f));
    frozenSpectrum.fill(std::complex<float>(0.0f, 0.0f));
    fftBuffer.fill(0.0f);
    decayState = 1.0f;
}

void SpectralFreeze::FFTProcessor::unpackRealFFT() {
    // JUCE format: [DC, Nyquist, Re(1), Im(1), Re(2), Im(2), ...]
    spectrum[0] = std::complex<float>(fftBuffer[0], 0.0f);  // DC
    spectrum[HALF_FFT_SIZE] = std::complex<float>(fftBuffer[1], 0.0f);  // Nyquist
    
    // Unpack the rest
    for (int i = 1; i < HALF_FFT_SIZE; ++i) {
        spectrum[i] = std::complex<float>(fftBuffer[2*i], fftBuffer[2*i + 1]);
    }
}

void SpectralFreeze::FFTProcessor::packToRealFFT() {
    fftBuffer[0] = spectrum[0].real();  // DC (imag must be 0)
    fftBuffer[1] = spectrum[HALF_FFT_SIZE].real();  // Nyquist (imag must be 0)
    
    for (int i = 1; i < HALF_FFT_SIZE; ++i) {
        fftBuffer[2*i] = spectrum[i].real();
        fftBuffer[2*i + 1] = spectrum[i].imag();
    }
}

//==============================================================================
// ChannelState Implementation
//==============================================================================
void SpectralFreeze::ChannelState::init(int fftSize) {
    fftProcessor.init(FFT_ORDER);
    phaseAccumulator.fill(0.0f);
    reset();
}

void SpectralFreeze::ChannelState::reset() {
    inputBuffer.fill(0.0f);
    outputBuffer.fill(0.0f);
    windowedFrame.fill(0.0f);
    tempSpectrum.fill(std::complex<float>(0.0f, 0.0f));
    phaseAccumulator.fill(0.0f);
    inputPos = 0;
    outputPos = 0;
    hopCounter = 0;
    isFrozen = false;
    freezeCounter = 0;
    fftProcessor.reset();
}

//==============================================================================
// SpectralFreeze Implementation
//==============================================================================
SpectralFreeze::SpectralFreeze() {
    // Initialize parameters with musical defaults
    m_freezeAmount.setImmediate(0.0f);
    m_spectralSmear.setImmediate(0.0f);
    m_spectralShift.setImmediate(0.0f);
    m_resonance.setImmediate(0.0f);
    m_decay.setImmediate(1.0f);
    m_brightness.setImmediate(0.5f);
    m_density.setImmediate(1.0f);
    m_shimmer.setImmediate(0.0f);
}

void SpectralFreeze::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_blockSize = samplesPerBlock;
    
    // Set parameter smoothing rates
    m_freezeAmount.setSmoothingRate(50.0f, sampleRate);
    m_spectralSmear.setSmoothingRate(100.0f, sampleRate);
    m_spectralShift.setSmoothingRate(20.0f, sampleRate);
    m_resonance.setSmoothingRate(100.0f, sampleRate);
    m_decay.setSmoothingRate(200.0f, sampleRate);
    m_brightness.setSmoothingRate(50.0f, sampleRate);
    m_density.setSmoothingRate(100.0f, sampleRate);
    m_shimmer.setSmoothingRate(50.0f, sampleRate);
    
    // Generate window with exact overlap compensation
    generateWindowWithCompensation();
    
    // Validate unity gain
    float gain = validateUnityGain();
    jassert(std::abs(gain - 1.0f) < 0.001f);
    
    // Initialize all channels
    for (auto& channel : m_channels) {
        channel.init(FFT_SIZE);
    }
}

void SpectralFreeze::generateWindowWithCompensation() {
    // Generate Hann window
    std::array<float, FFT_SIZE> window;
    for (int i = 0; i < FFT_SIZE; ++i) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
    }
    
    // Calculate exact overlap compensation at each sample
    m_overlapCompensation.fill(0.0f);
    
    // Sum overlapping windows
    for (int hop = 0; hop < FFT_SIZE; hop += HOP_SIZE) {
        for (int i = 0; i < FFT_SIZE; ++i) {
            int idx = (hop + i) % FFT_SIZE;
            m_overlapCompensation[idx] += window[i] * window[i];
        }
    }
    
    // Pre-multiply window by normalization factor
    for (int i = 0; i < FFT_SIZE; ++i) {
        float compensation = (m_overlapCompensation[i] > 0.0f) ? 
                           1.0f / (m_overlapCompensation[i] * FFT_SIZE) : 0.0f;
        m_windowNormalized[i] = window[i] * compensation;
    }
}

float SpectralFreeze::validateUnityGain() {
    // Test overlap-add compensation by summing all window values
    float totalGain = 0.0f;
    for (int hop = 0; hop < FFT_SIZE; hop += HOP_SIZE) {
        for (int i = 0; i < FFT_SIZE; ++i) {
            int idx = (hop + i) % FFT_SIZE;
            if (idx < HOP_SIZE) {
                totalGain += m_windowNormalized[i] * m_windowNormalized[i] * FFT_SIZE;
            }
        }
    }
    return totalGain / HOP_SIZE;
}

void SpectralFreeze::reset() {
    for (int ch = 0; ch < m_activeChannels; ++ch) {
        m_channels[ch].reset();
    }
    m_smoothCounter = 0;
}

void SpectralFreeze::process(juce::AudioBuffer<float>& buffer) {
    // Enable flush-to-zero for entire process block
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update active channel count
    m_activeChannels = std::min(numChannels, MAX_CHANNELS);
    
    for (int sample = 0; sample < numSamples; ++sample) {
        // Sub-block parameter smoothing
        bool updateParams = false;
        if (m_smoothCounter >= SMOOTH_INTERVAL) {
            m_freezeAmount.update();
            m_spectralSmear.update();
            m_spectralShift.update();
            m_resonance.update();
            m_decay.update();
            m_brightness.update();
            m_density.update();
            m_shimmer.update();
            m_smoothCounter = 0;
            updateParams = true;
        }
        m_smoothCounter++;
        
        // Process all active channels
        for (int ch = 0; ch < m_activeChannels; ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            auto& state = m_channels[ch];
            
            // Update processing flags once per parameter update
            if (updateParams) {
                state.enableSmear = m_spectralSmear.current > 0.01f;
                state.enableShift = fabsf(m_spectralShift.current) > 0.01f;
                state.enableResonance = m_resonance.current > 0.01f;
                state.enableDensity = m_density.current < 0.99f;
                state.enableShimmer = m_shimmer.current > 0.01f;
                state.shiftBins = static_cast<int>(m_spectralShift.current * HALF_FFT_SIZE * 0.1f);
            }
            
            // Add to input buffer
            state.inputBuffer[state.inputPos] = channelData[sample];
            state.inputPos = (state.inputPos + 1) % FFT_SIZE;
            state.hopCounter++;
            
            // Process FFT frame when we've accumulated enough samples
            if (state.hopCounter >= HOP_SIZE) {
                state.hopCounter = 0;
                
                // Fill windowed frame from circular buffer
                int readPos = (state.inputPos - FFT_SIZE + FFT_SIZE) % FFT_SIZE;
                for (int i = 0; i < FFT_SIZE; ++i) {
                    state.windowedFrame[i] = state.inputBuffer[(readPos + i) % FFT_SIZE] * 
                                           m_windowNormalized[i];
                }
                
                // Copy to FFT buffer (real part)
                std::copy(state.windowedFrame.begin(), state.windowedFrame.end(), 
                         state.fftProcessor.fftBuffer.begin());
                
                // Forward FFT
                state.fftProcessor.fft->performRealOnlyForwardTransform(
                    state.fftProcessor.fftBuffer.data());
                
                // Unpack JUCE's real FFT format to complex spectrum
                state.fftProcessor.unpackRealFFT();
                
                // Freeze logic
                bool shouldFreeze = m_freezeAmount.current > 0.5f;
                if (shouldFreeze && !state.isFrozen) {
                    // Capture spectrum
                    std::copy(state.fftProcessor.spectrum.begin(),
                             state.fftProcessor.spectrum.begin() + HALF_FFT_SIZE + 1,
                             state.fftProcessor.frozenSpectrum.begin());
                    state.isFrozen = true;
                    state.freezeCounter = 0;
                } else if (!shouldFreeze) {
                    state.isFrozen = false;
                }
                
                // Use frozen or live spectrum
                if (state.isFrozen) {
                    std::copy(state.fftProcessor.frozenSpectrum.begin(),
                             state.fftProcessor.frozenSpectrum.begin() + HALF_FFT_SIZE + 1,
                             state.fftProcessor.spectrum.begin());
                    
                    // Apply decay with leak prevention
                    float decay = m_decay.current;
                    state.fftProcessor.decayState = 
                        state.fftProcessor.decayState * FFTProcessor::DECAY_LEAK + 
                        decay * FFTProcessor::DECAY_GAIN;
                    
                    // Apply decay to frozen spectrum
                    for (int i = 0; i <= HALF_FFT_SIZE; ++i) {
                        state.fftProcessor.frozenSpectrum[i] *= state.fftProcessor.decayState;
                    }
                    
                    state.freezeCounter++;
                }
                
                // Apply spectral processing
                processSpectrum(state);
                
                // Pack back to JUCE's format
                state.fftProcessor.packToRealFFT();
                
                // Inverse FFT
                state.fftProcessor.fft->performRealOnlyInverseTransform(
                    state.fftProcessor.fftBuffer.data());
                
                // Apply window and overlap-add
                for (int i = 0; i < FFT_SIZE; ++i) {
                    float sample = state.fftProcessor.fftBuffer[i] * m_windowNormalized[i];
                    
                    int outIdx = (state.outputPos + i) % FFT_SIZE;
                    state.outputBuffer[outIdx] += sample;
                }
                state.outputPos = (state.outputPos + HOP_SIZE) % FFT_SIZE;
            }
            
            // Read from output buffer
            int outReadPos = (state.outputPos + FFT_SIZE - HOP_SIZE * 3) % FFT_SIZE;
            float output = state.outputBuffer[outReadPos];
            state.outputBuffer[outReadPos] = 0.0f; // Clear after reading
            
            // Prevent denormals in output using DspEngineUtilities
            output = DSPUtils::flushDenorm(output);
            
            // Mix with dry signal based on freeze amount
            float wetAmount = m_freezeAmount.current;
            channelData[sample] = channelData[sample] * (1.0f - wetAmount) + output * wetAmount;
        }
    }
    
    // Scrub NaN/Inf values from output buffer
    scrubBuffer(buffer);
}

void SpectralFreeze::processSpectrum(ChannelState& state) {
    // Work with the unpacked complex spectrum
    auto* spectrum = state.fftProcessor.spectrum.data();
    
    // Branch-free processing based on pre-computed flags
    if (state.enableSmear) {
        applySpectralSmear(spectrum, m_spectralSmear.current, state);
    }
    
    if (state.enableShift && state.shiftBins != 0) {
        applySpectralShift(spectrum, state.shiftBins, state);
    }
    
    if (state.enableResonance) {
        applyResonance(spectrum, m_resonance.current);
    }
    
    // Always apply brightness (it's just a tilt)
    applyBrightness(spectrum, m_brightness.current);
    
    if (state.enableDensity) {
        applyDensity(spectrum, m_density.current);
    }
    
    if (state.enableShimmer) {
        applyShimmer(spectrum, m_shimmer.current, state);
    }
}

void SpectralFreeze::applySpectralSmear(std::complex<float>* spectrum, float amount, 
                                       ChannelState& state) {
    // Use channel's own temp buffer
    auto& temp = state.tempSpectrum;
    
    // Smear radius
    int radius = static_cast<int>(amount * 5.0f) + 1;
    
    // Simple scalar implementation
    for (int i = 0; i <= HALF_FFT_SIZE; ++i) {
        std::complex<float> sum(0.0f, 0.0f);
        int count = 0;
        
        for (int j = -radius; j <= radius; ++j) {
            int idx = i + j;
            if (idx >= 0 && idx <= HALF_FFT_SIZE) {
                sum += spectrum[idx];
                count++;
            }
        }
        
        temp[i] = (count > 0) ? sum / float(count) : spectrum[i];
    }
    
    // Copy back
    std::copy(temp.begin(), temp.begin() + HALF_FFT_SIZE + 1, spectrum);
}

void SpectralFreeze::applySpectralShift(std::complex<float>* spectrum, int shiftBins,
                                       ChannelState& state) {
    // Use channel's own temp buffer
    auto& temp = state.tempSpectrum;
    temp.fill(std::complex<float>(0.0f, 0.0f));
    
    // Shift with bounds checking
    for (int i = 0; i <= HALF_FFT_SIZE; ++i) {
        int targetIdx = i + shiftBins;
        if (targetIdx >= 0 && targetIdx <= HALF_FFT_SIZE) {
            temp[targetIdx] = spectrum[i];
        }
    }
    
    std::copy(temp.begin(), temp.begin() + HALF_FFT_SIZE + 1, spectrum);
}

void SpectralFreeze::applyResonance(std::complex<float>* spectrum, float resonance) {
    // Simple peak enhancement
    float enhancement = 1.0f + resonance * 3.0f;
    
    for (int i = 1; i < HALF_FFT_SIZE; ++i) {
        float mag_prev = std::abs(spectrum[i-1]);
        float mag_curr = std::abs(spectrum[i]);
        float mag_next = std::abs(spectrum[i+1]);
        
        // Enhance peaks
        if (mag_curr > mag_prev && mag_curr > mag_next) {
            spectrum[i] *= enhancement;
        }
    }
}

void SpectralFreeze::applyBrightness(std::complex<float>* spectrum, float brightness) {
    float tilt = (brightness - 0.5f) * 2.0f;
    
    // Simple scalar implementation
    for (int i = 0; i <= HALF_FFT_SIZE; ++i) {
        float freq = float(i) / HALF_FFT_SIZE;
        float gain = 1.0f + tilt * freq * 2.0f;
        gain = std::max(0.1f, std::min(gain, 4.0f));
        spectrum[i] *= gain;
    }
}

void SpectralFreeze::applyDensity(std::complex<float>* spectrum, float density) {
    // Calculate magnitudes and indices
    std::array<std::pair<float, int>, HALF_FFT_SIZE + 1> magIndex;
    
    for (int i = 0; i <= HALF_FFT_SIZE; ++i) {
        magIndex[i] = {std::abs(spectrum[i]), i};
    }
    
    // Partial sort to find threshold
    int keepBins = static_cast<int>((HALF_FFT_SIZE + 1) * density);
    std::nth_element(magIndex.begin(), magIndex.begin() + keepBins, 
                     magIndex.begin() + HALF_FFT_SIZE + 1,
                     std::greater<>());
    
    float threshold = (keepBins < HALF_FFT_SIZE + 1) ? magIndex[keepBins].first : 0.0f;
    
    // Zero out bins below threshold
    for (int i = 0; i <= HALF_FFT_SIZE; ++i) {
        if (std::abs(spectrum[i]) < threshold) {
            spectrum[i] = std::complex<float>(0.0f, 0.0f);
        }
    }
}

void SpectralFreeze::applyShimmer(std::complex<float>* spectrum, float shimmer, 
                                 ChannelState& state) {
    // Incremental phase randomization (not full random each time)
    float shimmerAmount = shimmer * 0.2f;  // Reduced amount
    
    // Only apply to upper frequencies for more musical results
    int startBin = HALF_FFT_SIZE / 4;  // Start at 1/4 spectrum
    
    for (int i = startBin; i < HALF_FFT_SIZE; ++i) {
        float mag = std::abs(spectrum[i]);
        if (mag > 0.0001f) {  // Only process audible bins
            float phase = std::arg(spectrum[i]);
            
            // Add small incremental phase jitter
            float jitter = state.phaseDist(state.rng) * shimmerAmount;
            state.phaseAccumulator[i] += jitter;
            
            // Wrap phase accumulator
            while (state.phaseAccumulator[i] > M_PI) state.phaseAccumulator[i] -= 2.0f * M_PI;
            while (state.phaseAccumulator[i] < -M_PI) state.phaseAccumulator[i] += 2.0f * M_PI;
            
            // Apply accumulated phase offset
            spectrum[i] = std::polar(mag, phase + state.phaseAccumulator[i]);
        }
    }
}

void SpectralFreeze::updateParameters(const std::map<int, float>& params) {
    auto getParam = [&params](int index, float defaultValue) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    };
    
    // Thread-safe parameter updates
    m_freezeAmount.target.store(getParam(0, 0.0f));
    m_spectralSmear.target.store(getParam(1, 0.0f));
    m_spectralShift.target.store(getParam(2, 0.5f) * 2.0f - 1.0f);
    m_resonance.target.store(getParam(3, 0.0f));
    m_decay.target.store(0.9f + getParam(4, 1.0f) * 0.1f);
    m_brightness.target.store(getParam(5, 0.5f));
    m_density.target.store(getParam(6, 1.0f));
    m_shimmer.target.store(getParam(7, 0.0f));
}

juce::String SpectralFreeze::getParameterName(int index) const {
    switch (index) {
        case 0: return "Freeze";
        case 1: return "Smear";
        case 2: return "Shift";
        case 3: return "Resonance";
        case 4: return "Decay";
        case 5: return "Brightness";
        case 6: return "Density";
        case 7: return "Shimmer";
        default: return "";
    }
}
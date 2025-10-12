#include "SpectralGate.h"

SpectralGate::SpectralGate() 
    : m_fft(std::make_unique<juce::dsp::FFT>(juce::roundToInt(std::log2(FFT_SIZE))))
{
    generateHannWindow();
}

void SpectralGate::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_samplesPerBlock = samplesPerBlock;
    
    // Initialize parameter smoothing
    const double smoothingTime = 0.05; // 50ms smoothing
    m_threshold.reset(sampleRate, smoothingTime);
    m_ratio.reset(sampleRate, smoothingTime);
    m_attack.reset(sampleRate, smoothingTime);
    m_release.reset(sampleRate, smoothingTime);
    m_range.reset(sampleRate, smoothingTime);
    m_lookahead.reset(sampleRate, smoothingTime);
    m_frequency.reset(sampleRate, smoothingTime);
    m_mix.reset(sampleRate, smoothingTime);
    
    reset();
}

void SpectralGate::setNumChannels(int numIn, int numOut) {
    m_numChannels = juce::jmin(numIn, (int)m_channels.size());
    reset(); // Clear state when channel count changes
}

void SpectralGate::reset() {
    juce::ScopedLock lock(m_parameterLock);
    
    for (auto& channel : m_channels) {
        // Clear all buffers
        std::fill(channel.inputBuffer.begin(), channel.inputBuffer.end(), 0.0f);
        std::fill(channel.outputBuffer.begin(), channel.outputBuffer.end(), 0.0f);
        std::fill(channel.overlapBuffer.begin(), channel.overlapBuffer.end(), 0.0f);
        std::fill(channel.fftData.begin(), channel.fftData.end(), 0.0f);
        std::fill(channel.ifftData.begin(), channel.ifftData.end(), 0.0f);
        
        // Clear spectral data
        std::fill(channel.magnitude.begin(), channel.magnitude.end(), 0.0f);
        std::fill(channel.phase.begin(), channel.phase.end(), 0.0f);
        std::fill(channel.gateMask.begin(), channel.gateMask.end(), 1.0f);
        std::fill(channel.smoothedMask.begin(), channel.smoothedMask.end(), 1.0f);
        std::fill(channel.prevMagnitude.begin(), channel.prevMagnitude.end(), 0.0f);
        
        // Reset gate state
        std::fill(channel.gateOpen.begin(), channel.gateOpen.end(), false);
        std::fill(channel.attackFrames.begin(), channel.attackFrames.end(), 0.0f);
        std::fill(channel.releaseFrames.begin(), channel.releaseFrames.end(), 0.0f);
        
        channel.writePos = 0;
        channel.readPos = 0;
        channel.isReady = false;
    }
}

void SpectralGate::process(juce::AudioBuffer<float>& buffer) {
    juce::ScopedNoDenormals noDenormals; // DenormalGuard
    juce::ScopedLock lock(m_parameterLock);
    
    // CPU monitoring
    const auto startTime = juce::Time::getHighResolutionTicks();
    
    const int numChannels = juce::jmin(buffer.getNumChannels(), (int)m_channels.size());
    const int numSamples = buffer.getNumSamples();
    
    // Create dry buffer for mix
    juce::AudioBuffer<float> dryBuffer(numChannels, numSamples);
    dryBuffer.makeCopyOf(buffer);
    
    // Early bypass check for mix parameter
    // NOTE: Removed early return that was causing Bug #3 (SpectralGate appearing "crashed")
    // The early return prevented any output when mix < 0.001, but we need to process
    // the signal even at low mix values for proper dry/wet blending below.
    const float mixValue = m_mix.getNextValue();
    // if (mixValue < 0.001f) {
    //     // Completely dry - advance remaining mix parameter calls for smooth operation
    //     for (int i = 1; i < numSamples; ++i) {
    //         m_mix.getNextValue();
    //     }
    //     return;
    // }
    
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        auto& channel = m_channels[ch];
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Store input sample
            const float inputSample = channelData[sample];
            channel.inputBuffer[channel.writePos] = inputSample;
            
            // Check if we have enough samples for processing
            const int samplesReady = (channel.writePos - channel.readPos + FFT_SIZE) % FFT_SIZE;
            
            if (samplesReady >= HOP_SIZE || (!channel.isReady && samplesReady >= FFT_SIZE)) {
                processFrame(channel);
                channel.isReady = true;
                channel.readPos = (channel.readPos + HOP_SIZE) % FFT_SIZE;
            }
            
            // Output processed sample
            channelData[sample] = channel.outputBuffer[channel.writePos];
            channel.outputBuffer[channel.writePos] = 0.0f; // Clear for next use
            
            channel.writePos = (channel.writePos + 1) % FFT_SIZE;
        }
    }
    
    // Apply dry/wet mix
    const float wetGain = mixValue;
    const float dryGain = 1.0f - mixValue;
    
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* wetData = buffer.getWritePointer(ch);
        const auto* dryData = dryBuffer.getReadPointer(ch);
        
        for (int sample = 0; sample < numSamples; ++sample) {
            wetData[sample] = wetData[sample] * wetGain + dryData[sample] * dryGain;
        }
    }
    
    // Update CPU usage estimate
    const auto endTime = juce::Time::getHighResolutionTicks();
    const double processingTime = juce::Time::highResolutionTicksToSeconds(endTime - startTime);
    const double blockTime = numSamples / m_sampleRate;
    m_cpuUsage = static_cast<float>(processingTime / blockTime);
}

void SpectralGate::processFrame(ChannelState& channel) {
    // Copy windowed input to FFT buffer
    for (int i = 0; i < FFT_SIZE; ++i) {
        const int bufferIndex = (channel.readPos + i) % FFT_SIZE;
        channel.fftData[i * 2] = channel.inputBuffer[bufferIndex] * m_window[i];
        channel.fftData[i * 2 + 1] = 0.0f; // Imaginary part
    }
    
    // Forward FFT
    m_fft->performFrequencyOnlyForwardTransform(channel.fftData.data());
    
    // Extract magnitude and phase
    for (int bin = 0; bin < SPECTRUM_SIZE; ++bin) {
        const float real = channel.fftData[bin * 2];
        const float imag = channel.fftData[bin * 2 + 1];
        
        channel.magnitude[bin] = std::sqrt(real * real + imag * imag);
        channel.phase[bin] = std::atan2(imag, real);
    }
    
    // Compute spectral gate
    computeSpectralGate(channel);
    
    // Apply frequency smoothing
    applyFrequencySmoothing(channel.smoothedMask);
    
    // Reconstruct spectrum with gating applied
    for (int bin = 0; bin < SPECTRUM_SIZE; ++bin) {
        const float gatedMagnitude = channel.magnitude[bin] * channel.smoothedMask[bin];
        channel.ifftData[bin * 2] = gatedMagnitude * std::cos(channel.phase[bin]);
        channel.ifftData[bin * 2 + 1] = gatedMagnitude * std::sin(channel.phase[bin]);
    }
    
    // Mirror negative frequencies for real IFFT
    for (int bin = 1; bin < SPECTRUM_SIZE - 1; ++bin) {
        const int mirrorBin = FFT_SIZE - bin;
        channel.ifftData[mirrorBin * 2] = channel.ifftData[bin * 2];
        channel.ifftData[mirrorBin * 2 + 1] = -channel.ifftData[bin * 2 + 1];
    }
    
    // Inverse FFT
    m_fft->performRealOnlyInverseTransform(channel.ifftData.data());
    
    // Overlap-add with windowing
    for (int i = 0; i < FFT_SIZE; ++i) {
        const float windowedSample = channel.ifftData[i] * m_window[i];
        channel.overlapBuffer[i] += windowedSample;
        
        // Output current hop
        if (i < HOP_SIZE) {
            const int outputIndex = (channel.readPos + i) % FFT_SIZE;
            channel.outputBuffer[outputIndex] += channel.overlapBuffer[i];
            channel.overlapBuffer[i] = 0.0f; // Clear processed portion
        }
    }
    
    // Shift overlap buffer
    for (int i = 0; i < FFT_SIZE - HOP_SIZE; ++i) {
        channel.overlapBuffer[i] = channel.overlapBuffer[i + HOP_SIZE];
    }
    for (int i = FFT_SIZE - HOP_SIZE; i < FFT_SIZE; ++i) {
        channel.overlapBuffer[i] = 0.0f;
    }
}

void SpectralGate::computeSpectralGate(ChannelState& channel) {
    const float thresholdLinear = juce::Decibels::decibelsToGain(m_threshold.getNextValue());
    const float ratio = m_ratio.getNextValue();
    const float rangeDb = m_range.getNextValue();
    const float attackMs = m_attack.getNextValue();
    const float releaseMs = m_release.getNextValue();
    const float frequencyFactor = m_frequency.getNextValue();
    
    // Convert times to frames (at hop rate)
    const float hopRate = static_cast<float>(m_sampleRate) / HOP_SIZE;
    const float attackFrames = (attackMs / 1000.0f) * hopRate;
    const float releaseFrames = (releaseMs / 1000.0f) * hopRate;
    
    // Hysteresis thresholds
    const float openThreshold = thresholdLinear;
    const float closeThreshold = thresholdLinear * 0.7f; // 3dB hysteresis
    
    for (int bin = 0; bin < SPECTRUM_SIZE; ++bin) {
        const float magnitude = channel.magnitude[bin];
        
        // Frequency-dependent threshold adjustment
        const float freq = (static_cast<float>(bin) / FFT_SIZE) * static_cast<float>(m_sampleRate);
        const float freqWeight = 1.0f + (frequencyFactor - 1.0f) * (freq / (static_cast<float>(m_sampleRate) * 0.5f));
        const float adjustedOpenThreshold = openThreshold * freqWeight;
        const float adjustedCloseThreshold = closeThreshold * freqWeight;
        
        // Gate state machine with hysteresis
        bool& gateOpen = channel.gateOpen[bin];
        if (!gateOpen && magnitude > adjustedOpenThreshold) {
            gateOpen = true;
        } else if (gateOpen && magnitude < adjustedCloseThreshold) {
            gateOpen = false;
        }
        
        // Target gate value
        float targetMask;
        if (gateOpen) {
            targetMask = 1.0f; // Gate open: pass through
        } else {
            // Gate closed: apply reduction
            const float reductionLinear = juce::Decibels::decibelsToGain(-rangeDb);
            const float compressionRatio = 1.0f / ratio;
            targetMask = std::pow(reductionLinear, compressionRatio);
        }
        
        // Smooth mask transitions
        float& currentMask = channel.gateMask[bin];
        if (targetMask > currentMask) {
            // Attack
            if (attackFrames > 0.0f) {
                const float attackCoeff = 1.0f - std::exp(-1.0f / attackFrames);
                currentMask += (targetMask - currentMask) * attackCoeff;
            } else {
                currentMask = targetMask;
            }
        } else {
            // Release
            if (releaseFrames > 0.0f) {
                const float releaseCoeff = 1.0f - std::exp(-1.0f / releaseFrames);
                currentMask += (targetMask - currentMask) * releaseCoeff;
            } else {
                currentMask = targetMask;
            }
        }
        
        // Store for next frame
        channel.prevMagnitude[bin] = magnitude;
    }
    
    // Copy gate mask to smoothed mask for frequency filtering
    for (int bin = 0; bin < SPECTRUM_SIZE; ++bin) {
        channel.smoothedMask[bin] = channel.gateMask[bin];
    }
}

void SpectralGate::applyFrequencySmoothing(std::array<float, SPECTRUM_SIZE>& mask) {
    // 3-bin median filter for frequency smoothing
    std::array<float, SPECTRUM_SIZE> smoothedMask = mask;
    
    for (int bin = 1; bin < SPECTRUM_SIZE - 1; ++bin) {
        smoothedMask[bin] = medianFilter3(mask[bin - 1], mask[bin], mask[bin + 1]);
    }
    
    mask = smoothedMask;
}

float SpectralGate::medianFilter3(float a, float b, float c) {
    if (a > b) std::swap(a, b);
    if (b > c) std::swap(b, c);
    if (a > b) std::swap(a, b);
    return b; // Median value
}

void SpectralGate::generateHannWindow() {
    for (int i = 0; i < FFT_SIZE; ++i) {
        m_window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (FFT_SIZE - 1)));
    }
    
    // Normalize for perfect reconstruction with 75% overlap
    const float normalizationFactor = 2.0f / 3.0f; // For 4x overlap (75%)
    for (int i = 0; i < FFT_SIZE; ++i) {
        m_window[i] *= normalizationFactor;
    }
}

void SpectralGate::updateParameters(const std::map<int, float>& params) {
    juce::ScopedLock lock(m_parameterLock);
    
    for (const auto& [index, value] : params) {
        switch (index) {
            case Parameters::Threshold:
                m_threshold.setTargetValue(value);
                break;
            case Parameters::Ratio:
                m_ratio.setTargetValue(juce::jlimit(1.0f, 100.0f, value));
                break;
            case Parameters::Attack:
                m_attack.setTargetValue(juce::jlimit(0.1f, 1000.0f, value));
                break;
            case Parameters::Release:
                m_release.setTargetValue(juce::jlimit(1.0f, 5000.0f, value));
                break;
            case Parameters::Range:
                m_range.setTargetValue(juce::jlimit(0.0f, 80.0f, value));
                break;
            case Parameters::Lookahead:
                m_lookahead.setTargetValue(juce::jlimit(0.0f, 10.0f, value));
                break;
            case Parameters::Frequency:
                m_frequency.setTargetValue(juce::jlimit(0.1f, 10.0f, value));
                break;
            case Parameters::Mix:
                m_mix.setTargetValue(juce::jlimit(0.0f, 1.0f, value));
                break;
        }
    }
}

juce::String SpectralGate::getParameterName(int index) const {
    switch (index) {
        case Parameters::Threshold: return "Threshold";
        case Parameters::Ratio: return "Ratio";
        case Parameters::Attack: return "Attack";
        case Parameters::Release: return "Release";
        case Parameters::Range: return "Range";
        case Parameters::Lookahead: return "Lookahead";
        case Parameters::Frequency: return "Frequency";
        case Parameters::Mix: return "Mix";
        default: return "Param " + juce::String(index + 1);
    }
}

bool SpectralGate::supportsFeature(Feature f) const noexcept {
    switch (f) {
        case Feature::LatencyCompensation:
            return true; // We report latency correctly
        case Feature::Bypass:
            return true; // Mix parameter can act as bypass
        case Feature::Sidechain:
        case Feature::TempoSync:
        case Feature::Oversampling:
        case Feature::DoublePrecision:
            return false;
        default:
            return false;
    }
}
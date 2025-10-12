#include "SpectralGate_Platinum.h"
#include "DspEngineUtilities.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------
SpectralGate_Platinum::SpectralGate_Platinum() {
    // reasonable defaults
    pThreshold.snap(-30.0f);   // dB
    pRatio.snap(4.0f);         // 4:1
    pAttack.snap(5.0f);        // ms
    pRelease.snap(50.0f);      // ms
    pFreqLow.snap(20.0f);      // Hz
    pFreqHigh.snap(20000.0f);  // Hz
    pLookahead.snap(0.0f);     // ms
    pMix.snap(1.0f);           // 100% wet
}

void SpectralGate_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // SAFETY: Clamp sample rate to valid range
    sr_ = std::clamp(sampleRate, 8000.0, 192000.0);
    maxBlock_ = std::clamp(samplesPerBlock, 16, 8192);

    // Set parameter smoothing times
    pThreshold.setTimeMs(10.f, sr_);
    pRatio.setTimeMs(10.f, sr_);
    pAttack.setTimeMs(10.f, sr_);
    pRelease.setTimeMs(10.f, sr_);
    pFreqLow.setTimeMs(20.f, sr_);
    pFreqHigh.setTimeMs(20.f, sr_);
    pLookahead.setTimeMs(20.f, sr_);
    pMix.setTimeMs(10.f, sr_);

    // Bounded iteration limit (prevent infinite loops)
    maxProcessingIterations_ = std::min(10000, maxBlock_ * 10);

    // Initialize channels (ensure at least stereo)
    if (channels_.size() < 2) {
        channels_.resize(2);
    }

    // CRITICAL: Initialize FFT windows for all channels
    for (auto& ch : channels_) {
        ch.fftProc.prepareWindow();
        ch.reset();

        // Prepare lookahead delay buffer
        int maxLookaheadSamples = static_cast<int>(0.010 * sr_); // 10ms max
        ch.delayBuf.resize(maxLookaheadSamples + 1, 0.0f);
    }

    // Initialize state
    reset();
}

void SpectralGate_Platinum::reset() {
    for (auto& ch : channels_)
        ch.reset();
}

// -------------------------------------------------------
void SpectralGate_Platinum::updateParameters(const std::map<int, float>& params) {
    auto get = [&](int idx, float def){ auto it=params.find(idx); return it!=params.end()? it->second : def; };

    // Map 0..1 to meaningful ranges
    float thresh01 = clamp01(get((int)ParamID::Threshold, 0.25f));  // 0..1
    float ratio01  = clamp01(get((int)ParamID::Ratio, 0.3f));       // 0..1
    float att01    = clamp01(get((int)ParamID::Attack, 0.3f));      // 0..1
    float rel01    = clamp01(get((int)ParamID::Release, 0.3f));     // 0..1
    float fLo01    = clamp01(get((int)ParamID::FreqLow, 0.0f));     // 0..1
    float fHi01    = clamp01(get((int)ParamID::FreqHigh, 1.0f));    // 0..1
    float look01   = clamp01(get((int)ParamID::Lookahead, 0.0f));   // 0..1
    float mix01    = clamp01(get((int)ParamID::Mix, 1.0f));         // 0..1

    // Convert to actual values
    float threshDb = -60.0f + 60.0f * thresh01;       // -60..0 dB
    float ratio    = 1.0f + 19.0f * ratio01;          // 1:1 to 20:1
    float attackMs = 0.1f + 49.9f * att01;            // 0.1..50 ms
    float releaseMs= 1.0f + 499.0f * rel01;           // 1..500 ms
    float freqLow  = 20.0f * std::pow(10.0f, 3.0f * fLo01);  // 20Hz..20kHz
    float freqHigh = 20.0f * std::pow(10.0f, 3.0f * fHi01);  // 20Hz..20kHz
    float lookMs   = 10.0f * look01;                  // 0..10 ms

    pThreshold.target.store(threshDb, std::memory_order_relaxed);
    pRatio.target.store(ratio, std::memory_order_relaxed);
    pAttack.target.store(attackMs, std::memory_order_relaxed);
    pRelease.target.store(releaseMs, std::memory_order_relaxed);
    pFreqLow.target.store(std::min(freqLow, freqHigh - 10.0f), std::memory_order_relaxed);
    pFreqHigh.target.store(std::max(freqHigh, freqLow + 10.0f), std::memory_order_relaxed);
    pLookahead.target.store(lookMs, std::memory_order_relaxed);
    pMix.target.store(mix01, std::memory_order_relaxed);
}

// -------------------------------------------------------
void SpectralGate_Platinum::process(juce::AudioBuffer<float>& buffer) {
    // SAFETY: Early validation checks
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // SAFETY: Bounds check
    if (numChannels <= 0 || numSamples <= 0 || numSamples > maxBlock_) {
        return; // Invalid buffer, passthrough
    }

    // SAFETY: Ensure channels are initialized
    if (channels_.empty()) {
        return; // Not prepared, passthrough
    }

    // SAFETY: Denormal protection
    juce::ScopedNoDenormals noDenormals;

    // Tick parameters once per block
    const float threshDb = pThreshold.tick();
    const float ratio = pRatio.tick();
    const float attackMs = pAttack.tick();
    const float releaseMs = pRelease.tick();
    const float freqLow = pFreqLow.tick();
    const float freqHigh = pFreqHigh.tick();
    const float lookaheadMs = pLookahead.tick();
    const float mixValue = pMix.tick();

    // SAFETY: Validate parameter ranges
    if (!std::isfinite(threshDb) || !std::isfinite(ratio) ||
        !std::isfinite(mixValue) || ratio < 1.0f) {
        return; // Invalid parameters, passthrough to prevent crash
    }

    // Create dry buffer for mix
    juce::AudioBuffer<float> dryBuffer(numChannels, numSamples);
    dryBuffer.makeCopyOf(buffer);

    // Process each channel independently
    const int channelsToProcess = std::min(numChannels, (int)channels_.size());
    for (int ch = 0; ch < channelsToProcess; ++ch) {
        float* channelData = buffer.getWritePointer(ch);

        // SAFETY: Null pointer check
        if (channelData == nullptr) {
            continue;
        }

        // Process channel with safety wrapper
        try {
            processChannel(channels_[ch], channelData, numSamples);
        } catch (...) {
            // SAFETY: On any exception, copy dry signal
            const float* dryData = dryBuffer.getReadPointer(ch);
            if (dryData) {
                std::copy(dryData, dryData + numSamples, channelData);
            }
        }
    }

    // Apply dry/wet mix
    const float wetGain = std::clamp(mixValue, 0.0f, 1.0f);
    const float dryGain = 1.0f - wetGain;

    for (int ch = 0; ch < channelsToProcess; ++ch) {
        float* wetData = buffer.getWritePointer(ch);
        const float* dryData = dryBuffer.getReadPointer(ch);

        if (wetData && dryData) {
            for (int i = 0; i < numSamples; ++i) {
                // SAFETY: NaN check and clamping
                float wet = wetData[i];
                if (!std::isfinite(wet)) {
                    wet = 0.0f;
                }

                float mixed = wet * wetGain + dryData[i] * dryGain;

                // SAFETY: Final output clamping
                wetData[i] = std::clamp(mixed, -2.0f, 2.0f);
            }
        }
    }
}

void SpectralGate_Platinum::processChannel(Channel& ch, float* data, int numSamples) {
    // SAFETY: Validate input
    if (!data || numSamples <= 0) return;

    // Get current parameters
    const float threshDb = pThreshold.current;
    const float ratio    = std::max(1.0f, pRatio.current);  // SAFETY: Ensure ratio >= 1
    const float attackMs = std::clamp(pAttack.current, 0.1f, 1000.0f);  // SAFETY: Clamp
    const float releaseMs= std::clamp(pRelease.current, 1.0f, 5000.0f); // SAFETY: Clamp
    const float freqLow  = std::clamp(pFreqLow.current, 20.0f, static_cast<float>(sr_ * 0.5));
    const float freqHigh = std::clamp(pFreqHigh.current, 20.0f, static_cast<float>(sr_ * 0.5));

    // SAFETY: Convert threshold with bounds checking
    const float threshLin = std::clamp(std::pow(10.0f, std::clamp(threshDb, -80.0f, 0.0f) / 20.0f),
                                       1e-10f, 10.0f);

    const int binLow  = freqToBin(freqLow, sr_);
    const int binHigh = freqToBin(freqHigh, sr_);

    // SAFETY: Envelope coefficients with bounds checking to prevent NaN
    const float attackDenom = std::max(0.01f, attackMs * static_cast<float>(sr_));
    const float releaseDenom = std::max(0.01f, releaseMs * static_cast<float>(sr_));
    const float attackCoeff = std::clamp(std::exp(-1000.0f / attackDenom), 0.0f, 0.9999f);
    const float releaseCoeff = std::clamp(std::exp(-1000.0f / releaseDenom), 0.0f, 0.9999f);

    // Bounded processing loop
    int iterations = 0;
    for (int n = 0; n < numSamples; ++n) {
        // Safety: prevent infinite loops
        if (++iterations > maxProcessingIterations_) break;

        float input = data[n];

        // Apply lookahead delay
        float delayed = input;
        if (ch.delaySamples > 0) {
            delayed = ch.delayBuf[ch.delayWrite];
            ch.delayBuf[ch.delayWrite] = input;
            ch.delayWrite = (ch.delayWrite + 1) % (int)ch.delayBuf.size();
        }

        // Write to input buffer
        ch.inputBuf[ch.writePos] = delayed;
        ch.writePos = (ch.writePos + 1) % kFFTSize;

        // Process FFT frame when ready
        if (++ch.hopCounter >= kHopSize) {
            ch.hopCounter = 0;

            // Prepare frame (bounded copy)
            float frame[kFFTSize];
            for (int i = 0; i < kFFTSize && i < maxProcessingIterations_; ++i) {
                int idx = (ch.writePos - kFFTSize + i + kFFTSize) % kFFTSize;
                frame[i] = ch.inputBuf[idx];
            }

            // Process with spectral gate (internally bounded)
            float output[kFFTSize];
            ch.fftProc.processFrame(frame, output, threshLin, ratio, binLow, binHigh);

            // Write to output buffer with overlap-add
            for (int i = 0; i < kFFTSize && i < maxProcessingIterations_; ++i) {
                int idx = (ch.readPos + i) % kFFTSize;
                ch.outputBuf[idx] += output[i];
            }
        }

        // Read output
        // Simple passthrough for initial latency period
        float output = delayed;  
        
        // After initial hop, use output buffer
        if (ch.hopCounter == 0 && ch.readPos < ch.writePos) {
            output = ch.outputBuf[ch.readPos];
            ch.outputBuf[ch.readPos] = 0.0f; // Clear after reading
        }
        ch.readPos = (ch.readPos + 1) % kFFTSize;

        // Update per-bin envelopes (simplified, bounded)
        if (n % 64 == 0) { // Decimate for efficiency
            for (int b = 0; b < kFFTBins && b < 100; ++b) { // Limit bins
                float target = (b >= binLow && b <= binHigh) ? 1.0f : 0.0f;
                float coeff = target > ch.binEnv[b] ? attackCoeff : releaseCoeff;
                ch.binEnv[b] += (1.0f - coeff) * (target - ch.binEnv[b]);
                ch.binEnv[b] = flushDenorm(ch.binEnv[b]);
            }
        }

        // Safety and denormal protection
        output = flushDenorm(output);
        if (!std::isfinite(output)) output = 0.0f;
        output = clamp(output, -2.0f, 2.0f);

        data[n] = output;
    }
}

// -------------------------------------------------------
void SpectralGate_Platinum::FFTProcessor::prepareWindow() {
    // Hann window for smooth overlap
    for (int i = 0; i < kFFTSize; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (kFFTSize - 1.0f)));
    }
}

void SpectralGate_Platinum::FFTProcessor::processFrame(const float* input, float* output,
                                                       float threshold, float ratio,
                                                       int binLow, int binHigh) {
    // SAFETY: Validate inputs
    if (!input || !output) {
        if (output) {
            std::fill(output, output + kFFTSize, 0.0f);
        }
        return;
    }

    // SAFETY: Clamp bin ranges
    binLow = std::clamp(binLow, 0, kFFTBins - 1);
    binHigh = std::clamp(binHigh, binLow, kFFTBins - 1);
    threshold = std::max(1e-10f, threshold);  // SAFETY: Prevent division by zero
    ratio = std::clamp(ratio, 1.0f, 100.0f);  // SAFETY: Reasonable ratio range

    // Copy and window input with NaN protection
    for (int i = 0; i < kFFTSize; ++i) {
        float sample = input[i];
        // SAFETY: NaN/Inf check
        if (!std::isfinite(sample)) {
            sample = 0.0f;
        }
        fftData[i] = sample * window[i];
    }

    // Forward FFT
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Apply spectral gating with full safety checks
    for (int bin = 0; bin < kFFTBins; ++bin) {
        float real = fftData[bin * 2];
        float imag = fftData[bin * 2 + 1];

        // SAFETY: NaN check on FFT output
        if (!std::isfinite(real) || !std::isfinite(imag)) {
            fftData[bin * 2] = 0.0f;
            fftData[bin * 2 + 1] = 0.0f;
            continue;
        }

        float mag = std::sqrt(real * real + imag * imag);

        // SAFETY: Check magnitude is finite
        if (!std::isfinite(mag)) {
            mag = 0.0f;
        }

        float gain = 1.0f;

        // Gate logic: only process bins in frequency range
        if (bin >= binLow && bin <= binHigh) {
            if (mag < threshold) {
                // Below threshold: apply full gating
                gain = 0.0f;
            } else if (ratio > 1.0f) {
                // Above threshold: apply ratio
                float excess = mag - threshold;
                float gated = threshold + excess / ratio;
                // SAFETY: Prevent division by zero
                gain = gated / std::max(mag, 1e-10f);
                // SAFETY: Clamp gain to valid range
                gain = std::clamp(gain, 0.0f, 1.0f);
            }
        }

        // Apply gain to complex components
        fftData[bin * 2] *= gain;
        fftData[bin * 2 + 1] *= gain;
    }

    // Inverse FFT
    fft.performRealOnlyInverseTransform(fftData.data());

    // Overlap-add with windowing and safety checks
    // JUCE FFT includes 1/N scaling; Hann window with 75% overlap sums to ~1.5
    const float scaleFactor = 1.0f / 1.5f;

    for (int i = 0; i < kFFTSize; ++i) {
        float ifftSample = fftData[i];

        // SAFETY: NaN check on IFFT output
        if (!std::isfinite(ifftSample)) {
            ifftSample = 0.0f;
        }

        float windowed = ifftSample * window[i] * scaleFactor;

        // SAFETY: Clamp windowed output
        windowed = std::clamp(windowed, -10.0f, 10.0f);

        // Overlap-add
        int pos = (overlapPos + i) % kFFTSize;
        if (i < kHopSize) {
            output[i] = overlapBuf[pos] + windowed;
            // SAFETY: Final output sanitation
            if (!std::isfinite(output[i])) {
                output[i] = 0.0f;
            }
            output[i] = std::clamp(output[i], -2.0f, 2.0f);
            overlapBuf[pos] = 0.0f;
        } else {
            overlapBuf[pos] += windowed;
            // SAFETY: Clamp overlap buffer to prevent accumulation
            overlapBuf[pos] = std::clamp(overlapBuf[pos], -10.0f, 10.0f);
            output[i] = 0.0f;
        }
    }

    overlapPos = (overlapPos + kHopSize) % kFFTSize;
}

// -------------------------------------------------------
juce::String SpectralGate_Platinum::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::Threshold:  return "Threshold";
        case ParamID::Ratio:      return "Ratio";
        case ParamID::Attack:     return "Attack";
        case ParamID::Release:    return "Release";
        case ParamID::FreqLow:    return "Freq Low";
        case ParamID::FreqHigh:   return "Freq High";
        case ParamID::Lookahead:  return "Lookahead";
        case ParamID::Mix:        return "Mix";
        default:                  return {};
    }
}

int SpectralGate_Platinum::getLatencySamples() const noexcept {
    // FFT processing latency is the hop size plus any lookahead delay
    int fftLatency = kHopSize;
    
    // Add lookahead delay if enabled
    float lookaheadMs = pLookahead.current;
    int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * sr_);
    
    return fftLatency + lookaheadSamples;
}
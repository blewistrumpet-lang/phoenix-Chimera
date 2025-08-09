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
    sr_ = std::max(8000.0, sampleRate);
    maxBlock_ = std::max(16, samplesPerBlock);

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
    maxProcessingIterations_ = std::min(1000, maxBlock_ * 10);

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
    // RAII denormal protection for entire block
    DenormalGuard guard;
    
    const int numCh = buffer.getNumChannels();
    const int N = buffer.getNumSamples();
    if (N <= 0) return;

    // Ensure we have enough channels
    if ((int)channels_.size() < numCh) {
        channels_.resize(numCh);
        for (auto& ch : channels_) {
            ch.fftProc.prepareWindow();
            ch.delayBuf.resize(size_t(sr_ * 0.011), 0.0f); // up to 11ms
            ch.reset();
        }
    }

    // Pull smoothed params (block-rate)
    const float threshDb = pThreshold.tick();
    const float ratio    = pRatio.tick();
    const float attackMs = pAttack.tick();
    const float releaseMs= pRelease.tick();
    const float freqLow  = pFreqLow.tick();
    const float freqHigh = pFreqHigh.tick();
    const float lookMs   = pLookahead.tick();
    const float mix01    = pMix.tick();

    // Convert to linear threshold
    const float threshLin = std::pow(10.0f, threshDb / 20.0f);

    // Convert to bin indices
    const int binLow  = freqToBin(freqLow, sr_);
    const int binHigh = freqToBin(freqHigh, sr_);

    // Update lookahead samples
    const int lookSamples = std::min((int)(lookMs * 0.001 * sr_), (int)channels_[0].delayBuf.size() - 1);
    for (auto& ch : channels_)
        ch.delaySamples = lookSamples;

    // Process each channel with bounded iterations
    for (int c = 0; c < numCh; ++c) {
        processChannel(channels_[c], buffer.getWritePointer(c), N);
    }

    // Apply mix
    if (mix01 < 0.999f) {
        // Store dry signal first
        juce::AudioBuffer<float> dry(buffer);
        
        // After processing, blend
        for (int c = 0; c < numCh; ++c) {
            float* wet = buffer.getWritePointer(c);
            const float* dryPtr = dry.getReadPointer(c);
            for (int n = 0; n < N; ++n) {
                wet[n] = dryPtr[n] * (1.0f - mix01) + wet[n] * mix01;
                // Safety
                if (!std::isfinite(wet[n])) wet[n] = 0.0f;
                wet[n] = clamp(wet[n], -2.0f, 2.0f);
            }
        }
    }
    
    // Final safety scrub (catches any NaN/Inf that slipped through)
    scrubBuffer(buffer);
}

void SpectralGate_Platinum::processChannel(Channel& ch, float* data, int numSamples) {
    // Get current parameters
    const float threshDb = pThreshold.current;
    const float ratio    = pRatio.current;
    const float attackMs = pAttack.current;
    const float releaseMs= pRelease.current;
    const float freqLow  = pFreqLow.current;
    const float freqHigh = pFreqHigh.current;

    const float threshLin = std::pow(10.0f, threshDb / 20.0f);
    const int binLow  = freqToBin(freqLow, sr_);
    const int binHigh = freqToBin(freqHigh, sr_);

    // Envelope coefficients
    const float attackCoeff = std::exp(-1000.0f / (attackMs * sr_));
    const float releaseCoeff = std::exp(-1000.0f / (releaseMs * sr_));

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
        float output = ch.outputBuf[ch.readPos];
        ch.outputBuf[ch.readPos] = 0.0f; // Clear after reading
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
    // Copy and window input (bounded)
    for (int i = 0; i < kFFTSize; ++i) {
        fftData[i] = input[i] * window[i];
    }

    // Forward FFT
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Apply spectral gating (bounded iteration)
    for (int bin = 0; bin < kFFTBins && bin < 512; ++bin) { // Limit to 512 bins
        float mag = std::abs(fftData[bin]);
        
        float gain = 1.0f;
        if (bin >= binLow && bin <= binHigh) {
            if (mag < threshold) {
                gain = 0.0f;
            } else if (ratio > 1.0f) {
                float excess = mag - threshold;
                float gated = threshold + excess / ratio;
                gain = gated / std::max(mag, 1e-10f);
                gain = clamp(gain, 0.0f, 1.0f);
            }
        }

        fftData[bin] *= gain;
    }

    // Inverse FFT
    fft.performRealOnlyInverseTransform(fftData.data());

    // Overlap-add with windowing (bounded)
    for (int i = 0; i < kFFTSize; ++i) {
        float windowed = fftData[i] * window[i] / (kFFTSize * kOverlap / 2);
        
        // Simple overlap-add
        int pos = (overlapPos + i) % kFFTSize;
        if (i < kHopSize) {
            output[i] = overlapBuf[pos] + windowed;
            overlapBuf[pos] = 0.0f;
        } else {
            overlapBuf[pos] += windowed;
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
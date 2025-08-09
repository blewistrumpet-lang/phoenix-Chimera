#include "GranularCloud.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
struct FTZGuard {
    FTZGuard() {
       #if defined(__SSE__)
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
       #endif
    }
} s_ftzGuard;
}

// -------------------------------------------------------
GranularCloud::GranularCloud() {
    // reasonable defaults
    pGrainSize.snap(50.0f);     // ms
    pDensity.snap(10.0f);        // grains/sec
    pPitchScatter.snap(0.0f);    // octaves
    pCloudPosition.snap(0.5f);   // center
}

void GranularCloud::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sr_ = std::max(8000.0, sampleRate);
    maxBlock_ = std::max(16, samplesPerBlock);

    // Smoothing times
    pGrainSize.setTimeMs(20.f, sr_);
    pDensity.setTimeMs(20.f, sr_);
    pPitchScatter.setTimeMs(30.f, sr_);
    pCloudPosition.setTimeMs(30.f, sr_);

    // Circular buffer: 2 seconds
    bufferSize_ = (int)std::ceil(2.0 * sr_);
    circularBuffer_.resize(bufferSize_, 0.0f);

    // Window table
    windowSize_ = 8192;
    windowTable_.resize(windowSize_);
    for (int i = 0; i < windowSize_; ++i) {
        float phase = float(i) / float(windowSize_ - 1);
        windowTable_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
    }

    reset();
}

void GranularCloud::reset() {
    std::fill(circularBuffer_.begin(), circularBuffer_.end(), 0.0f);
    writePos_ = 0;
    grainTimer_ = 0.0;
    nextGrainTime_ = 0.0;

    for (auto& g : grains_) {
        g.active = false;
        g.pos = 0.0;
        g.increment = 1.0;
        g.length = 0;
        g.elapsed = 0;
        g.amp = 0.0f;
        g.pan = 0.5f;
    }
}

// -------------------------------------------------------
void GranularCloud::updateParameters(const std::map<int, float>& params) {
    auto get = [&](int idx, float def){ auto it=params.find(idx); return it!=params.end()? it->second : def; };

    // Map 0..1 to meaningful ranges
    float size01    = clamp01(get((int)ParamID::GrainSize, 0.5f));
    float dens01    = clamp01(get((int)ParamID::Density, 0.3f));
    float pitch01   = clamp01(get((int)ParamID::PitchScatter, 0.0f));
    float pos01     = clamp01(get((int)ParamID::CloudPosition, 0.5f));

    // Convert to actual values
    float grainMs   = 5.0f + 195.0f * size01;      // 5..200 ms
    float density   = 1.0f + 49.0f * dens01;       // 1..50 grains/sec
    float scatter   = 2.0f * pitch01;              // 0..2 octaves scatter
    float position  = pos01;                        // 0..1 stereo position

    pGrainSize.target.store(grainMs, std::memory_order_relaxed);
    pDensity.target.store(density, std::memory_order_relaxed);
    pPitchScatter.target.store(scatter, std::memory_order_relaxed);
    pCloudPosition.target.store(position, std::memory_order_relaxed);
}

// -------------------------------------------------------
void GranularCloud::process(juce::AudioBuffer<float>& buffer) {
    const int numCh = std::min(buffer.getNumChannels(), 2);
    const int N = buffer.getNumSamples();
    if (N <= 0) return;

    // Pull smoothed params (block-rate)
    const float grainMs  = pGrainSize.tick();
    const float density  = pDensity.tick();
    const float scatter  = pPitchScatter.tick();
    const float position = pCloudPosition.tick();

    // Derive grain spawn rate
    const double grainInterval = 1.0 / std::max(0.1, (double)density);

    float* Lp = buffer.getWritePointer(0);
    float* Rp = (numCh > 1) ? buffer.getWritePointer(1) : nullptr;

    // Bounded main loop
    int iterations = 0;
    const int maxIterations = N * 10; // Safety limit

    for (int n = 0; n < N; ++n) {
        if (++iterations > maxIterations) break; // Prevent infinite loops

        const float inL = Lp[n];
        const float inR = (Rp ? Rp[n] : inL);
        const float inMono = 0.5f * (inL + inR);

        // Write to circular buffer
        circularBuffer_[writePos_] = inMono;
        writePos_ = (writePos_ + 1) % bufferSize_;

        // Grain spawning with bounded attempts
        grainTimer_ += 1.0 / sr_;
        if (grainTimer_ >= nextGrainTime_) {
            triggerGrain(grainMs, scatter, position);
            nextGrainTime_ = grainTimer_ + grainInterval * (0.5 + rng_.uniform() * 1.0);
        }

        // Process active grains (bounded)
        float outL = 0.0f, outR = 0.0f;
        int activeCount = 0;
        for (auto& g : grains_) {
            if (!g.active) continue;
            if (++activeCount > kMaxActiveGrains) break; // Limit active grains

            // Read from buffer with linear interpolation
            const double readPos = writePos_ - g.pos;
            const int idx0 = ((int)readPos + bufferSize_) % bufferSize_;
            const int idx1 = (idx0 + 1) % bufferSize_;
            const float frac = (float)(readPos - floor(readPos));
            const float sample = circularBuffer_[idx0] * (1.0f - frac) + circularBuffer_[idx1] * frac;

            // Apply window
            const float windowPhase = (float)g.elapsed / (float)g.length;
            const int winIdx = std::min((int)(windowPhase * windowSize_), windowSize_ - 1);
            const float windowed = sample * windowTable_[winIdx] * g.amp;

            // Pan and accumulate
            const float panL = std::sqrt(1.0f - g.pan);
            const float panR = std::sqrt(g.pan);
            outL += windowed * panL * 0.3f; // Scale down for headroom
            outR += windowed * panR * 0.3f;

            // Update grain
            g.pos += g.increment;
            g.elapsed++;
            if (g.elapsed >= g.length) {
                g.active = false;
            }
        }

        // Mix with dry (simple pass-through for now, could add mix param)
        outL = inL * 0.5f + outL * 0.5f;
        outR = (Rp ? inR : inL) * 0.5f + outR * 0.5f;

        // Safety and output
        if (!std::isfinite(outL)) outL = 0.0f;
        if (!std::isfinite(outR)) outR = 0.0f;
        outL = clamp(outL, -1.5f, 1.5f);
        outR = clamp(outR, -1.5f, 1.5f);

        Lp[n] = flushDenorm(outL);
        if (Rp) Rp[n] = flushDenorm(outR);
    }
}

void GranularCloud::triggerGrain(float grainMs, float scatter, float position) {
    // Find free grain (bounded search)
    Grain* grain = nullptr;
    int attempts = 0;
    for (auto& g : grains_) {
        if (++attempts > kMaxActiveGrains) break; // Limit search
        if (!g.active) {
            grain = &g;
            break;
        }
    }
    if (!grain) return; // All grains busy

    // Set grain parameters
    grain->active = true;
    grain->length = std::max(64, (int)(grainMs * 0.001 * sr_));
    grain->elapsed = 0;

    // Random position in buffer (last 500ms)
    const double maxDelay = std::min(0.5 * sr_, (double)bufferSize_ * 0.9);
    grain->pos = rng_.uniform() * maxDelay;

    // Pitch variation
    if (scatter > 0.001f) {
        const float octaves = (rng_.uniform() - 0.5f) * 2.0f * scatter;
        grain->increment = std::pow(2.0f, octaves);
        grain->increment = clamp(grain->increment, 0.25, 4.0); // Limit pitch range
    } else {
        grain->increment = 1.0f;
    }

    // Amplitude and pan
    grain->amp = 0.7f + rng_.uniform() * 0.3f;
    grain->pan = clamp(position + (rng_.uniform() - 0.5f) * 0.3f, 0.0f, 1.0f);
}

// -------------------------------------------------------
juce::String GranularCloud::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::GrainSize:      return "Grain Size";
        case ParamID::Density:        return "Density";
        case ParamID::PitchScatter:   return "Pitch Scatter";
        case ParamID::CloudPosition:  return "Position";
        default:                      return {};
    }
}
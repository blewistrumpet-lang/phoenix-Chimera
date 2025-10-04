#include "JuceHeader.h"
#include "GranularCloud.h"
#include "DspEngineUtilities.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>

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
    pMix.snap(0.7f);             // 70% wet default for prominent effect
}

void GranularCloud::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sr_ = std::max(8000.0, sampleRate);
    maxBlock_ = std::max(16, samplesPerBlock);

    // Smoothing times
    pGrainSize.setTimeMs(20.f, sr_);
    pDensity.setTimeMs(20.f, sr_);
    pPitchScatter.setTimeMs(30.f, sr_);
    pCloudPosition.setTimeMs(30.f, sr_);
    pMix.setTimeMs(10.f, sr_);  // Fast mix response

    // Circular buffer: 2 seconds
    bufferSize_ = (int)std::ceil(2.0 * sr_);
    circularBuffer_.resize(bufferSize_, 0.0f);

    // Window table - IMPROVED Tukey window for better grain characteristics
    windowSize_ = 8192;
    windowTable_.resize(windowSize_);
    const float alpha = 0.25f; // Tukey window parameter (0.25 = 25% fade in/out)
    for (int i = 0; i < windowSize_; ++i) {
        float phase = float(i) / float(windowSize_ - 1);
        float window;
        
        if (phase < alpha * 0.5f) {
            // Fade in
            window = 0.5f * (1.0f + std::cos(2.0f * M_PI * (phase / alpha - 0.5f)));
        } else if (phase > (1.0f - alpha * 0.5f)) {
            // Fade out
            window = 0.5f * (1.0f + std::cos(2.0f * M_PI * ((phase - 1.0f) / alpha + 0.5f)));
        } else {
            // Sustain at full amplitude
            window = 1.0f;
        }
        
        windowTable_[i] = window;
    }

    reset();
}

void GranularCloud::reset() {
    std::fill(circularBuffer_.begin(), circularBuffer_.end(), 0.0f);
    writePos_ = 0;
    grainTimer_ = 0.0;
    nextGrainTime_ = 0.0;
    
    // Reset debug statistics
    grainStats_.reset();

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
    float mix01     = clamp01(get((int)ParamID::Mix, 0.7f));

    // Convert to actual values - EXPANDED RANGES for more dramatic effect
    float grainMs   = 2.0f + 298.0f * size01;      // 2..300 ms (wider range)
    float density   = 1.0f + 199.0f * dens01;      // 1..200 grains/sec (4x increase for dense clouds)
    float scatter   = 4.0f * pitch01;              // 0..4 octaves scatter (2x increase)
    float position  = pos01;                        // 0..1 stereo position

    pGrainSize.target.store(grainMs, std::memory_order_relaxed);
    pDensity.target.store(density, std::memory_order_relaxed);
    pPitchScatter.target.store(scatter, std::memory_order_relaxed);
    pCloudPosition.target.store(position, std::memory_order_relaxed);
    pMix.target.store(mix01, std::memory_order_relaxed);
}

// -------------------------------------------------------
void GranularCloud::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numCh = std::min(buffer.getNumChannels(), 2);
    const int N = buffer.getNumSamples();
    if (N <= 0) return;

    // Pull smoothed params (block-rate)
    const float grainMs  = pGrainSize.tick();
    const float density  = pDensity.tick();
    const float scatter  = pPitchScatter.tick();
    const float position = pCloudPosition.tick();
    const float mixAmount = pMix.tick();

    // Derive grain spawn rate
    const double grainInterval = 1.0 / std::max(0.1, (double)density);

    float* Lp = buffer.getWritePointer(0);
    float* Rp = (numCh > 1) ? buffer.getWritePointer(1) : nullptr;

    // SAFETY: Bounded main loop with multiple safety mechanisms
    // These limits prevent infinite loops that could cause CPU spikes and audio dropouts:
    // 1. maxIterations: Conservative limit (2x buffer size, max 8192) for normal operation
    // 2. emergencyBreak: Last resort fallback (10x buffer size) for runaway conditions
    // 3. Time-based limit: Prevents excessive processing time per audio block
    int iterations = 0;
    const int maxIterations = std::min(N * 2, 8192); // Conservative safety limit
    const int emergencyBreak = N * 10; // Emergency fallback
    
    // Additional safety: track processing time per block
    auto blockStartTime = std::chrono::high_resolution_clock::now();
    const auto maxProcessingTime = std::chrono::microseconds(1000); // 1ms max

    for (int n = 0; n < N; ++n) {
        // Primary iteration safety check
        if (++iterations > maxIterations) {
            // Log this occurrence for debugging (would be logged in real implementation)
            break;
        }
        
        // Emergency break for runaway conditions
        if (iterations > emergencyBreak) {
            // This should never happen - indicates serious bug
            grainStats_.emergencyBreaks++; // Track for debugging
            reset(); // Reset state to prevent continued issues
            break;
        }
        
        // Time-based safety check (every 64 samples)
        if ((n & 63) == 0) {
            auto elapsed = std::chrono::high_resolution_clock::now() - blockStartTime;
            if (elapsed > maxProcessingTime) {
                // Processing is taking too long - abort to prevent audio dropouts
                break;
            }
        }

        const float inL = Lp[n];
        const float inR = (Rp ? Rp[n] : inL);
        const float inMono = 0.5f * (inL + inR);

        // Write to circular buffer
        circularBuffer_[writePos_] = inMono;
        writePos_ = (writePos_ + 1) % bufferSize_;

        // SAFETY: Grain spawning with runaway protection
        // Prevents infinite grain spawning that could exhaust memory and CPU
        grainTimer_ += 1.0 / sr_;
        if (grainTimer_ >= nextGrainTime_) {
            // Count active grains before spawning to prevent runaway allocation
            int currentActiveGrains = 0;
            for (const auto& g : grains_) {
                if (g.active) currentActiveGrains++;
            }
            
            // Only trigger new grain if we're below the safety threshold
            if (currentActiveGrains < kMaxActiveGrains) {
                triggerGrain(grainMs, scatter, position);
                grainStats_.totalGrainsSpawned++;
            }
            
            // Update statistics
            grainStats_.currentActiveGrains = currentActiveGrains;
            grainStats_.peakActiveGrains = std::max(grainStats_.peakActiveGrains, currentActiveGrains);
            
            // Always advance timer to prevent stuck state
            // More variation in grain spawning for organic texture
            const double minInterval = 0.0005; // Minimum 0.5ms between attempts (allow denser)
            const double jitter = 0.2 + rng_.uniform() * 1.6; // 20% to 180% variation
            const double randomizedInterval = grainInterval * jitter;
            nextGrainTime_ = grainTimer_ + std::max(minInterval, randomizedInterval);
        }

        // SAFETY: Process active grains with multiple bounds to prevent infinite loops
        // grainIterations: Prevents infinite grain processing loops
        // activeCount: Limits concurrent grain processing to prevent CPU spikes
        // maxGrainIterations: Double the expected active count as safety margin
        float outL = 0.0f, outR = 0.0f;
        int activeCount = 0;
        int grainIterations = 0;
        const int maxGrainIterations = std::min(kMaxGrains, kMaxActiveGrains * 2); // Safety limit
        
        for (auto& g : grains_) {
            // Prevent infinite loops in grain processing
            if (++grainIterations > maxGrainIterations) {
                // Emergency brake - this should never happen in normal operation
                break;
            }
            
            if (!g.active) continue;
            
            // Strict limit on active grains to prevent CPU spikes
            if (++activeCount > kMaxActiveGrains) {
                // Mark excess grains as inactive to prevent runaway processing
                g.active = false;
                break;
            }

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

            // Pan and accumulate - INCREASED AMPLITUDE with density compensation
            const float panL = std::sqrt(1.0f - g.pan);
            const float panR = std::sqrt(g.pan);
            
            // Gain compensation based on grain density to prevent buildup
            // Higher density = lower individual grain volume
            const float densityCompensation = 1.0f / std::sqrt(1.0f + density * 0.01f);
            const float grainGain = 1.2f * densityCompensation; // Base gain increased for presence
            
            outL += windowed * panL * grainGain;
            outR += windowed * panR * grainGain;

            // Update grain
            g.pos += g.increment;
            g.elapsed++;
            if (g.elapsed >= g.length) {
                g.active = false;
            }
        }

        // Mix with dry - USER CONTROLLABLE via mix parameter
        const float dryGain = 1.0f - mixAmount;
        const float wetGain = mixAmount;
        outL = inL * dryGain + outL * wetGain;
        outR = (Rp ? inR : inL) * dryGain + outR * wetGain;

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
    // Find free grain with improved bounded search and recycling
    Grain* grain = nullptr;
    int attempts = 0;
    int activeGrainCount = 0;
    
    // First pass: look for truly inactive grains
    for (auto& g : grains_) {
        if (++attempts > kMaxGrains) break; // Safety limit on search iterations
        
        if (!g.active) {
            grain = &g;
            break;
        } else {
            activeGrainCount++;
        }
    }
    
    // If no free grain found and we have too many active grains, recycle oldest
    if (!grain && activeGrainCount >= kMaxActiveGrains) {
        // Find the grain that's closest to completion for recycling
        Grain* oldestGrain = nullptr;
        float maxProgress = -1.0f;
        
        attempts = 0;
        for (auto& g : grains_) {
            if (++attempts > kMaxGrains) break; // Safety limit
            
            if (g.active && g.length > 0) {
                float progress = float(g.elapsed) / float(g.length);
                if (progress > maxProgress) {
                    maxProgress = progress;
                    oldestGrain = &g;
                }
            }
        }
        
        if (oldestGrain && maxProgress > 0.7f) { // Only recycle if >70% complete
            grain = oldestGrain;
            grain->active = false; // Mark for reuse
            grainStats_.grainsRecycled++; // Track recycling for debugging
        }
    }
    
    if (!grain) return; // No grain available (safety fallback)

    // Set grain parameters with safety bounds
    grain->active = true;
    
    // SAFETY: Bound grain length to prevent excessive processing
    const int minGrainLength = 64;  // Minimum grain size (prevents clicks)
    const int maxGrainLength = (int)(0.5 * sr_); // Maximum 500ms grain
    grain->length = clamp((int)(grainMs * 0.001 * sr_), minGrainLength, maxGrainLength);
    grain->elapsed = 0;

    // SAFETY: Random position in buffer with bounds checking
    const double maxDelay = std::min(0.5 * sr_, (double)bufferSize_ * 0.9);
    grain->pos = clamp(rng_.uniform() * maxDelay, 0.0, maxDelay);

    // Pitch variation - ENHANCED for more dramatic effect
    if (scatter > 0.001f) {
        // Use gaussian distribution for more musical pitch variations
        const float gaussian = (rng_.uniform() + rng_.uniform() + rng_.uniform() - 1.5f) / 1.5f;
        const float octaves = gaussian * scatter;
        grain->increment = std::pow(2.0f, octaves);
        // Expanded pitch range for more dramatic variations
        grain->increment = clamp(grain->increment, 0.125, 8.0); // ±3 octaves
    } else {
        grain->increment = 1.0f;
    }

    // Amplitude and pan - MORE VARIATION for texture
    // Use bell curve for amplitude distribution (most grains at medium volume)
    const float ampRandom = (rng_.uniform() + rng_.uniform()) * 0.5f; // Simple approximation of gaussian
    grain->amp = 0.4f + ampRandom * 0.6f; // Range: 0.4 to 1.0
    
    // Wider stereo spread for more spacious effect
    grain->pan = clamp(position + (rng_.uniform() - 0.5f) * 0.5f, 0.0f, 1.0f);
}

// -------------------------------------------------------
juce::String GranularCloud::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::GrainSize:      return "Grain Size";
        case ParamID::Density:        return "Density";
        case ParamID::PitchScatter:   return "Pitch Scatter";
        case ParamID::CloudPosition:  return "Position";
        case ParamID::Mix:            return "Mix";
        default:                      return {};
    }
}
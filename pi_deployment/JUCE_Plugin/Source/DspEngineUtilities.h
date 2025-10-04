// DspEngineUtilities.h
// Shared DSP utilities and guardrails for all Chimera Phoenix engines
// Provides denormal protection, NaN scrubbing, parameter smoothing, and other studio-grade essentials

#pragma once

// Ensure JUCE is included first for inline and other macros
#ifndef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
    #include "../JuceLibraryCode/JuceHeader.h"
#endif

#include <atomic>
#include <cmath>
#include <array>

#if JUCE_USE_SSE_INTRINSICS || defined(__SSE__)
    #include <xmmintrin.h>
#endif

// ========== Denormal Protection ==========

// RAII guard for FTZ/DAZ mode - use at the start of process() blocks
struct DenormalGuard
{
#if JUCE_USE_SSE_INTRINSICS || defined(__SSE__)
    uint32_t oldMXCSR = 0;
    DenormalGuard() noexcept
    {
        oldMXCSR = _mm_getcsr();
        _mm_setcsr(oldMXCSR | 0x8040); // FTZ (bit 15) | DAZ (bit 6)
    }
    ~DenormalGuard() noexcept { 
        _mm_setcsr(oldMXCSR); 
    }
#else
    DenormalGuard() noexcept = default;
#endif
};

// Flush denormals to zero for scalar values
// Wrapped in namespace to avoid conflicts with existing implementations
namespace DSPUtils {
    template <typename T>
    inline T flushDenorm(T x) noexcept
    {
        constexpr T tiny = (T)1.0e-30;
        return std::abs(x) < tiny ? (T)0 : x;
    }
}

// To use flushDenorm from DspEngineUtilities, call DSPUtils::flushDenorm()

// ========== NaN/Inf Protection ==========

// Scrub NaN/Inf from a buffer (in-place)
inline void scrubBuffer(juce::AudioBuffer<float>& buf) noexcept
{
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
    {
        auto* data = buf.getWritePointer(ch);
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            const float v = data[i];
            data[i] = std::isfinite(v) ? v : 0.0f;
        }
    }
}

// Check if value is safe (finite and not denormal)
template <typename T>
inline bool isSafe(T x) noexcept
{
    return std::isfinite(x) && std::abs(x) >= (T)1.0e-30;
}

// Clamp value to safe range
template <typename T>
inline T clampSafe(T x, T minVal, T maxVal) noexcept
{
    x = std::isfinite(x) ? x : (T)0;
    return std::max(minVal, std::min(maxVal, x));
}

// ========== Parameter Smoothing ==========

// Sample-rate aware exponential smoother with configurable time
struct ParamSmoother
{
    void setTimeMs(double timeMs, double sampleRate) noexcept
    {
        timeMs = std::max(0.01, timeMs);
        sr = sampleRate;
        // One-pole smoothing coefficient: y = a*y + (1-a)*x
        a = std::exp(-1.0 / (0.001 * timeMs * sr));
        b = 1.0 - a;
    }
    
    float process(float target) noexcept
    {
        state = (float)(a * state + b * target);
        return DSPUtils::flushDenorm(state);
    }
    
    float processSample(float target) noexcept  // Alias for clarity
    {
        return process(target);
    }
    
    void reset(float value = 0.0f) noexcept 
    { 
        state = value; 
    }
    
    void snap(float value) noexcept  // Instantly jump to value
    {
        state = value;
    }
    
    float getCurrentValue() const noexcept { return state; }

private:
    double sr = 48000.0;
    double a = 0.99, b = 0.01;
    float state = 0.0f;
};

// Multi-rate smoother for different parameter types
struct MultiRateSmoother
{
    enum Type { 
        Instant,    // No smoothing
        Fast,       // 2-5ms (gain, threshold)
        Medium,     // 10-20ms (frequency, resonance)
        Slow        // 50-100ms (room size, character)
    };
    
    void prepare(double sampleRate, Type type = Medium) noexcept
    {
        sr = sampleRate;
        switch (type)
        {
            case Instant: smoother.setTimeMs(0.01, sr); break;
            case Fast:    smoother.setTimeMs(3.0, sr); break;
            case Medium:  smoother.setTimeMs(15.0, sr); break;
            case Slow:    smoother.setTimeMs(75.0, sr); break;
        }
    }
    
    float process(float target) noexcept { return smoother.process(target); }
    void reset(float value = 0.0f) noexcept { smoother.reset(value); }
    void snap(float value) noexcept { smoother.snap(value); }
    
private:
    ParamSmoother smoother;
    double sr = 48000.0;
};

// ========== Lock-free Parameter Cache ==========

// Atomic float wrapper for lock-free parameter updates
struct AtomicParam
{
    std::atomic<float> value{0.0f};
    
    void set(float x) noexcept 
    { 
        value.store(x, std::memory_order_relaxed); 
    }
    
    float get() const noexcept 
    { 
        return value.load(std::memory_order_relaxed); 
    }
    
    operator float() const noexcept { return get(); }
    AtomicParam& operator=(float x) noexcept { set(x); return *this; }
};

// ========== Crossfading & Mixing ==========

// Equal-power crossfade between dry and wet signals
inline float equalPowerMix(float dry, float wet, float mix) noexcept
{
    mix = clampSafe(mix, 0.0f, 1.0f);
    const float dryGain = std::cos(mix * 0.5f * M_PI);
    const float wetGain = std::sin(mix * 0.5f * M_PI);
    return dry * dryGain + wet * wetGain;
}

// Linear crossfade (cheaper but can have perceived volume dip at 50%)
inline float linearMix(float dry, float wet, float mix) noexcept
{
    mix = clampSafe(mix, 0.0f, 1.0f);
    return dry * (1.0f - mix) + wet * mix;
}

// ========== Bypass Ramping ==========

// Clickless bypass with configurable ramp time
class BypassRamp
{
public:
    void prepare(double sampleRate, double rampMs = 5.0) noexcept
    {
        sr = sampleRate;
        rampSamples = (int)(sr * rampMs * 0.001);
        rampSamples = std::max(1, rampSamples);
    }
    
    void setBypass(bool shouldBypass) noexcept
    {
        targetBypass = shouldBypass;
    }
    
    float processSample() noexcept
    {
        if (currentBypass == targetBypass)
            return currentBypass ? 0.0f : 1.0f;
        
        // Ramping
        if (targetBypass && rampCounter < rampSamples)
        {
            rampCounter++;
            return 1.0f - ((float)rampCounter / (float)rampSamples);
        }
        else if (!targetBypass && rampCounter > 0)
        {
            rampCounter--;
            return 1.0f - ((float)rampCounter / (float)rampSamples);
        }
        
        currentBypass = targetBypass;
        return currentBypass ? 0.0f : 1.0f;
    }
    
    bool isBypassed() const noexcept { return currentBypass && (rampCounter >= rampSamples); }
    bool isRamping() const noexcept { return currentBypass != targetBypass; }
    
private:
    double sr = 48000.0;
    int rampSamples = 256;
    int rampCounter = 0;
    bool currentBypass = false;
    bool targetBypass = false;
};

// ========== Buffer Utilities ==========

// Clear a buffer to silence
inline void clearBuffer(juce::AudioBuffer<float>& buffer) noexcept
{
    buffer.clear();
}

// Copy buffer with safety checks
inline void copyBufferSafe(const juce::AudioBuffer<float>& source,
                                       juce::AudioBuffer<float>& dest) noexcept
{
    const int numChannels = std::min(source.getNumChannels(), dest.getNumChannels());
    const int numSamples = std::min(source.getNumSamples(), dest.getNumSamples());
    
    for (int ch = 0; ch < numChannels; ++ch)
    {
        dest.copyFrom(ch, 0, source, ch, 0, numSamples);
    }
}

// Apply gain to buffer with denormal flushing
inline void applyGain(juce::AudioBuffer<float>& buffer, float gain) noexcept
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            data[i] = DSPUtils::flushDenorm(data[i] * gain);
        }
    }
}

// ========== Circular Buffer (for delays/reverbs) ==========

template <typename T>
class CircularBuffer
{
public:
    void setSize(int newSize)
    {
        size = std::max(1, newSize);
        buffer.resize(size);
        clear();
    }
    
    void clear()
    {
        std::fill(buffer.begin(), buffer.end(), T(0));
        writePos = 0;
    }
    
    void write(T sample)
    {
        if (buffer.empty()) return;
        buffer[writePos] = sample;
        writePos = (writePos + 1) % size;
    }
    
    T read(int delaySamples) const
    {
        if (buffer.empty()) return T(0);
        delaySamples = std::max(0, std::min(delaySamples, size - 1));
        int readPos = (writePos - delaySamples + size) % size;
        return buffer[readPos];
    }
    
    T readInterpolated(float delaySamples) const
    {
        if (buffer.empty()) return T(0);
        
        delaySamples = std::max(0.0f, std::min(delaySamples, (float)(size - 1)));
        int delay0 = (int)delaySamples;
        int delay1 = (delay0 + 1) % size;
        float frac = delaySamples - delay0;
        
        T sample0 = read(delay0);
        T sample1 = read(delay1);
        
        return sample0 + (sample1 - sample0) * frac;
    }
    
private:
    std::vector<T> buffer;
    int size = 1;
    int writePos = 0;
};

// ========== Simple One-Pole Filter ==========

class OnePoleFilter
{
public:
    void setCoefficient(float coeff) noexcept
    {
        a = clampSafe(coeff, 0.0f, 0.9999f);
        b = 1.0f - a;
    }
    
    void setCutoff(float hz, double sampleRate) noexcept
    {
        float rc = 1.0f / (2.0f * M_PI * hz);
        a = std::exp(-1.0f / (rc * sampleRate));
        b = 1.0f - a;
    }
    
    float process(float input) noexcept
    {
        state = a * state + b * input;
        return DSPUtils::flushDenorm(state);
    }
    
    void reset() noexcept { state = 0.0f; }
    
private:
    float a = 0.0f, b = 1.0f;
    float state = 0.0f;
};

// ========== DC Blocker ==========

class DCBlocker
{
public:
    void prepare(double sampleRate) noexcept
    {
        // Set cutoff to ~20Hz
        float fc = 20.0f / sampleRate;
        R = 1.0f - (2.0f * M_PI * fc);
        R = clampSafe(R, 0.9f, 0.9999f);
    }
    
    float process(float input) noexcept
    {
        float output = input - x1 + R * y1;
        x1 = input;
        y1 = DSPUtils::flushDenorm(output);
        return output;
    }
    
    void reset() noexcept
    {
        x1 = y1 = 0.0f;
    }
    
private:
    float R = 0.995f;
    float x1 = 0.0f, y1 = 0.0f;
};

// ========== Peak/RMS Meter ==========

class LevelMeter
{
public:
    void prepare(double sampleRate, double attackMs = 0.1, double releaseMs = 100.0) noexcept
    {
        attackCoeff = std::exp(-1.0 / (attackMs * 0.001 * sampleRate));
        releaseCoeff = std::exp(-1.0 / (releaseMs * 0.001 * sampleRate));
        reset();
    }
    
    void processSample(float sample) noexcept
    {
        float rectified = std::abs(sample);
        
        if (rectified > envelope)
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * rectified;
        else
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * rectified;
        
        envelope = DSPUtils::flushDenorm(envelope);
        peak = std::max(peak * 0.9999f, rectified);  // Slow peak decay
    }
    
    float getEnvelope() const noexcept { return envelope; }
    float getPeak() const noexcept { return peak; }
    float getEnvelopeDb() const noexcept { return 20.0f * std::log10(std::max(1e-6f, envelope)); }
    float getPeakDb() const noexcept { return 20.0f * std::log10(std::max(1e-6f, peak)); }
    
    void reset() noexcept
    {
        envelope = 0.0f;
        peak = 0.0f;
    }
    
private:
    float attackCoeff = 0.99f;
    float releaseCoeff = 0.999f;
    float envelope = 0.0f;
    float peak = 0.0f;
};
#pragma once
#include "EngineBase.h"
#include <JuceHeader.h>
#include <atomic>
#include <map>
#include <cmath>

/**
 * K-Style Overdrive (Stable)
 * Params (unchanged indices):
 * 0 Drive [0..1]  -> input drive
 * 1 Tone  [0..1]  -> tilt EQ (dark <-> bright)
 * 2 Level [0..1]  -> output level
 * 3 Mix   [0..1]  -> dry/wet
 */
class KStyleOverdrive : public EngineBase
{
public:
    KStyleOverdrive();
    ~KStyleOverdrive() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    juce::String getName() const override { return "K-Style Overdrive"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;

private:
    // ---- utils ----
    static inline float clamp01(float v) noexcept { return juce::jlimit(0.0f, 1.0f, v); }
    static inline bool finitef(float x) noexcept { return std::isfinite(x); }
    static inline float fromdB(float x) noexcept { 
        // Clamp input to reasonable range to prevent NaN/Inf
        x = juce::jlimit(-100.0f, 20.0f, x);
        float result = std::pow(10.0f, x / 20.0f);
        return std::isfinite(result) ? result : 0.0f;
    }

    // simple exp-based smoother in samples
    struct Smoothed {
        std::atomic<float> target{0.f};
        float current = 0.f;
        float coeff = 0.f;
        void setTimeConst(float seconds, float fs) {
            seconds = std::max(1.0e-4f, seconds);
            coeff = std::exp(-1.0f / (seconds * fs));
        }
        float next() { float t = target.load(std::memory_order_relaxed); current = t + (current - t) * coeff; return current; }
        void snap()  { current = target.load(std::memory_order_relaxed); }
    };

    // Zavalishin TPT one-pole (stable)
    struct OnePoleTPT {
        float g=0.f, z=0.f; // z is integrator state
        void setLowpass(float cutoff, float fs) {
            cutoff = juce::jlimit(20.0f, 0.47f*fs, cutoff);
            g = std::tan(juce::MathConstants<float>::pi * (cutoff / fs));
            z = 0.f;
        }
        void setHighpass(float cutoff, float fs) {
            setLowpass(cutoff, fs); // same g, just use HP topology in process
        }
        float processLP(float x) { const float v = (x - z) / (1.0f + g); const float y = v + z; z = y + g * v; return y; }
        float processHP(float x) { const float v = (x - z) / (1.0f + g); const float lp = v + z; z = lp + g * v; return x - lp; }
        void reset() { z = 0.f; }
    };

    // Tone = tilt (lowpass+highpass crossfade), stable and bounded
    struct TiltTone {
        OnePoleTPT lp, hp;
        float mix = 0.5f; // 0 = dark (more LP), 1 = bright (more HP)
        void prepare(double fs) {
            lp.setLowpass(1000.0f, (float)fs);
            hp.setHighpass(1000.0f, (float)fs);
        }
        void setMix(float t) { mix = juce::jlimit(0.0f, 1.0f, t); }
        float process(float x) {
            const float l = lp.processLP(x);
            const float h = hp.processHP(x);
            // equal power-ish crossfade to keep perceived loudness stable
            const float a = std::cos(juce::MathConstants<float>::halfPi * mix);
            const float b = std::sin(juce::MathConstants<float>::halfPi * mix);
            return a * l + b * h;
        }
        void reset() { lp.reset(); hp.reset(); }
    };

    // parameters (smoothed)
    Smoothed pDrive_, pTone_, pLevel_, pMix_;

    // runtime
    double sampleRate_ = 44100.0;
    int    numCh_ = 2;

    // DSP blocks
    TiltTone tone_[2];

    // nonlinearity: smooth, bounded
    inline float waveshaper(float x, float drive) const noexcept {
        // Map Drive [0..1] -> effective gain ~ [1.0 .. ~20 dB]
        const float pre = fromdB(juce::jmap(drive, 0.0f, 1.0f, 0.0f, 20.0f));
        const float y = std::tanh(x * pre);
        // Simple makeup to roughly preserve level across drive range
        const float makeup = fromdB(juce::jmap(drive, 0.0f, 1.0f, 0.0f, -6.0f));
        return y * makeup;
    }
};
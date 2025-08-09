#pragma once
#include "EngineBase.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <map>
#include <cmath>

class DimensionExpander : public EngineBase
{
public:
    DimensionExpander();
    ~DimensionExpander() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    juce::String getName() const override { return "Dimension Expander"; }
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;

private:
    enum ParamID {
        kWidth = 0,
        kDepth,
        kCrossfeed,
        kBassRetention,
        kAmbience,
        kMovement,
        kClarity,
        kMix
    };

    // --- utils ---
    static inline float clamp01(float v) noexcept { return juce::jlimit(0.0f, 1.0f, v); }
    static inline bool  finitef(float x) noexcept { return std::isfinite(x); }
    static inline float fromdB(float x) noexcept  { return std::pow(10.0f, x / 20.0f); }

    struct Smoothed {
        std::atomic<float> target{0.f};
        float current = 0.f, coeff = 0.f;
        void setTau(float sec, float fs) {
            sec = std::max(1.0e-4f, sec);
            coeff = std::exp(-1.0f / (sec * fs));
        }
        float next() { const float t = target.load(std::memory_order_relaxed);
                       current = t + (current - t) * coeff; return current; }
        void snap()  { current = target.load(std::memory_order_relaxed); }
    };

    // Topology-preserving one-pole (stable)
    struct OnePoleTPT {
        float g=0.f, z=0.f;
        void setLowpass(float cutoff, float fs) {
            cutoff = juce::jlimit(20.0f, 0.47f*fs, cutoff);
            g = std::tan(juce::MathConstants<float>::pi * (cutoff / fs));
            z = 0.f;
        }
        float processLP(float x) {
            const float v = (x - z) / (1.0f + g);
            const float y = v + z;
            z = y + g * v;
            return y;
        }
        void reset() { z = 0.f; }
    };

    // First-order allpass (stable)
    struct Allpass1 {
        float a=0.f, z=0.f;
        // a in (-1,1) for stability
        void setCoefficient(float newA) { a = juce::jlimit(-0.95f, 0.95f, newA); z = 0.f; }
        float process(float x) { const float y = -a * x + z; z = x + a * y; return y; }
        void reset() { z = 0.f; }
    };

    // Simple circular delay line for micro-delays (Haas)
    struct MicroDelay {
        std::vector<float> buf;
        int w=0, size=0;
        void prepare(int maxSamples) {
            size = std::max(8, maxSamples);
            buf.assign(size, 0.0f);
            w = 0;
        }
        void reset() { std::fill(buf.begin(), buf.end(), 0.0f); w=0; }
        inline void push(float x) { buf[w] = x; if (++w >= size) w = 0; }
        inline float readInt(int delay) const {
            delay = juce::jlimit(1, size-2, delay);
            int r = w - delay; while (r < 0) r += size;
            return buf[r];
        }
    };

    // params (smoothed)
    Smoothed pWidth_, pDepth_, pCross_, pBassKeep_, pAmb_, pMove_, pClar_, pMix_;

    // runtime
    double sampleRate_ = 44100.0;
    int    maxBlock_   = 512;

    // processing state
    OnePoleTPT lowKeeperL_, lowKeeperR_;  // bass retention LP
    OnePoleTPT clarityLP_, clarityHP_;    // build a tilt/high-shelf-ish
    Allpass1   apL1_, apL2_, apR1_, apR2_;
    MicroDelay dL_, dR_;
    float      lfoPhase_ = 0.f, lfoInc_ = 0.f;

    // helpers
    void updateFilters();
};
#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include <map>

class SpringReverb_Platinum : public EngineBase
{
public:
    SpringReverb_Platinum();
    ~SpringReverb_Platinum() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    juce::String getName() const override { return "Spring Reverb Platinum"; }
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;

private:
    enum ParamID {
        kTension = 0,   // pitch / dispersion flavor
        kDamping,       // HF damping in the tank
        kDecay,         // loop gain
        kMod,           // modulation depth
        kChirp,         // excitation chirp amount
        kDrive,         // pre-drive into tank
        kWidth,         // stereo width
        kMix            // wet/dry
    };

    // -------- utils ----------
    static inline float clamp01(float v) noexcept { return juce::jlimit(0.0f, 1.0f, v); }
    static inline bool finitef(float x) noexcept { return std::isfinite(x); }
    static inline float fromdB(float x) noexcept { 
        // Clamp input to reasonable range to prevent NaN/Inf
        x = juce::jlimit(-100.0f, 20.0f, x);
        float result = std::pow(10.0f, x / 20.0f);
        return std::isfinite(result) ? result : 0.0f;
    }
    static inline float sat(float x) noexcept { return std::tanh(x); }

    struct Smoothed {
        std::atomic<float> target{0.f};
        float current = 0.f, coeff = 0.f;
        void setTau(float sec, float fs) {
            sec = std::max(1.0e-4f, sec);
            coeff = std::exp(-1.0f / (sec * fs));
        }
        float next() { float t = target.load(std::memory_order_relaxed); current = t + (current - t) * coeff; return current; }
        void snap()  { current = target.load(std::memory_order_relaxed); }
    };

    // TPT one-pole damping (stable)
    struct OnePoleTPT {
        float g=0.f, z=0.f; // integrator state
        void setLowpass(float cutoff, float fs) {
            cutoff = juce::jlimit(20.0f, 0.47f*fs, cutoff);
            g = std::tan(juce::MathConstants<float>::pi * (cutoff / fs));
            z = 0.f;
        }
        float processLP(float x) {
            const float v = (x - z) / (1.f + g);
            const float y = v + z;
            z = y + g * v;
            return y;
        }
        void reset() { z = 0.f; }
    };

    // Modulated circular delay with wrapped Catmull-Rom (4-tap) interpolation
    class ModDelay {
    public:
        void prepare(double fs, float maxMs) {
            fs_ = fs;
            maxSamp_ = std::max(1, (int) std::ceil(maxMs * 0.001 * fs) + 8);
            buffer_.assign(maxSamp_, 0.0f);
            w_ = 0;
            frac_ = 0.0;
        }
        void reset() {
            std::fill(buffer_.begin(), buffer_.end(), 0.0f);
            w_ = 0; frac_ = 0.0;
        }
        inline void push(float x) noexcept {
            buffer_[w_] = x;
            if (++w_ >= (int)buffer_.size()) w_ = 0;
        }
        inline float readInterp(double delaySamp) const noexcept {
            // delaySamp in [1 .. maxSamp_-4], handle safely
            const int size = (int)buffer_.size();
            double rp = (double)w_ - delaySamp;
            while (rp < 0.0) rp += size;
            // integer/fraction
            int i1 = (int) rp;
            double f = rp - (double) i1;
            // need i0..i3 wrapped
            int i0 = (i1 - 1 + size) % size;
            int i2 = (i1 + 1) % size;
            int i3 = (i1 + 2) % size;

            const float y0 = buffer_[i0];
            const float y1 = buffer_[i1];
            const float y2 = buffer_[i2];
            const float y3 = buffer_[i3];

            // Catmull-Rom spline
            const float a0 = -0.5f*y0 + 1.5f*y1 - 1.5f*y2 + 0.5f*y3;
            const float a1 = y0 - 2.5f*y1 + 2.0f*y2 - 0.5f*y3;
            const float a2 = -0.5f*y0 + 0.5f*y2;
            const float a3 = y1;

            const float out = ((a0 * (float)f + a1) * (float)f + a2) * (float)f + a3;
            return finitef(out) ? out : 0.0f;
        }
        inline int capacity() const noexcept { return (int)buffer_.size(); }
    private:
        std::vector<float> buffer_;
        int w_ = 0;
        double fs_ = 44100.0;
        int maxSamp_ = 1;
        double frac_ = 0.0;
    };

    struct APF {
        // Simple first-order allpass (stable)
        float a=0.f, z=0.f;
        void set(float delaySamples) {
            // map samples to coefficient ~ (N-1)/(N+1) style; we keep it gentle
            float x = juce::jlimit(0.0f, 1.0f, delaySamples * 0.001f);
            a = juce::jlimit(-0.7f, 0.7f, 0.4f + 0.4f * x);
            z = 0.f;
        }
        float process(float x) {
            const float y = -a * x + z;
            z = x + a * y;
            return y;
        }
        void reset() { z = 0.f; }
    };

    struct TankLine {
        ModDelay delay;
        OnePoleTPT dampLP;
        APF apf1, apf2;
        float lastOut=0.f;
        void reset() { delay.reset(); dampLP.reset(); apf1.reset(); apf2.reset(); lastOut=0.f; }
    };

    // 2-channel tank: 3 lines per channel for density
    static constexpr int kLines = 3;
    std::array<TankLine, kLines> L_, R_;

    // params (smoothed)
    Smoothed pTension_, pDamp_, pDecay_, pMod_, pChirp_, pDrive_, pWidth_, pMix_;

    // runtime
    double sampleRate_ = 44100.0;
    int    maxBlock_   = 512;

    // modulation
    float lfoPhase_ = 0.0f;
    float lfoIncr_  = 0.0f;

    // chirp state
    float chirpPhase_ = 0.0f;
    float chirpGain_  = 0.0f;

    // helpers
    void updateTankCoeffs();
    float lineProcess(TankLine& line, float in, float baseDelaySamp, float modDepthSamp, float tensionDisp);
};
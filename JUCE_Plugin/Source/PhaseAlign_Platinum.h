#pragma once
#include "EngineBase.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <map>
#include <cmath>

class PhaseAlign_Platinum : public EngineBase
{
public:
    PhaseAlign_Platinum();
    ~PhaseAlign_Platinum() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    juce::String getName() const override { return "Phase Align Platinum"; }
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;

private:
    enum ParamID {
        AUTO_ALIGN = 0,   // [0,1] off/on
        REFERENCE,        // [0,1] 0=Left, 1=Right
        LOW_PHASE,        // [-180..+180] mapped from [0..1]
        LOW_MID_PHASE,
        HIGH_MID_PHASE,
        HIGH_PHASE,
        LOW_FREQ,         // [50..400 Hz]
        MID_FREQ,         // [400..3k Hz]
        HIGH_FREQ,        // [3k..12k Hz]
        MIX               // [0..1]
    };

    // ----------------- utils -----------------
    static inline float clamp01(float v) noexcept { return juce::jlimit(0.0f, 1.0f, v); }
    static inline bool  finitef(float x) noexcept { return std::isfinite(x); }
    static inline float deg2rad(float d) noexcept { return d * (juce::MathConstants<float>::pi / 180.0f); }

    struct Smoothed {
        std::atomic<float> target{0.f};
        float current=0.f, coeff=0.f;
        void setTau(float sec, float fs) {
            sec = std::max(1.0e-4f, sec);
            coeff = std::exp(-1.0f / (sec * fs));
        }
        float next() { float t = target.load(std::memory_order_relaxed); current = t + (current - t) * coeff; return current; }
        void snap()  { current = target.load(std::memory_order_relaxed); }
    };

    // TPT Linkwitz–Riley (cascaded one-pole) style split
    struct OnePoleTPT {
        float g=0.f, z=0.f;
        void setLP(float fc, float fs) {
            fc = juce::jlimit(20.0f, 0.47f*fs, fc);
            g = std::tan(juce::MathConstants<float>::pi * (fc / fs));
            z = 0.f;
        }
        float lp(float x){ const float v=(x - z)/(1.f+g); const float y=v+z; z=y + g*v; return y; }
        float hp(float x){ const float v=(x - z)/(1.f+g); const float y=v+z; z=y + g*v; return x - y; }
        void reset(){ z=0.f; }
    };

    // 2nd-order all-pass (biquad form) with stable parameterization
    struct AP2 {
        float a1=0.f, a2=0.f, b0=0.f, b1=0.f, b2=0.f;
        float x1=0.f,x2=0.f,y1=0.f,y2=0.f;
        // set by pole radius r (0..0.999) and angle theta (rad)
        void set(float theta, float r){
            r = juce::jlimit(0.0f, 0.999f, r);
            const float c = std::cos(theta);
            a1 = -2.0f*r*c;
            a2 =  r*r;
            b0 =  a2;
            b1 =  a1;
            b2 = 1.0f;
        }
        float process(float x){
            const float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
            x2=x1; x1=x; y2=y1; y1=y;
            return y;
        }
        void reset(){ x1=x2=y1=y2=0.f; }
    };

    // 3rd-order Thiran all-pass for fractional delay (stable for 0<=D<=3)
    struct Thiran3 {
        float a1=0, a2=0, a3=0; // denom (1 + a1 z^-1 + a2 z^-2 + a3 z^-3)
        float b0=0, b1=0, b2=0, b3=0; // numer reversed
        float x1=0,x2=0,x3=0, y1=0,y2=0,y3=0;
        // fractional delay D in [0..3]
        void set(float D){
            D = juce::jlimit(0.0f, 2.999f, D);
            // coefficients (see Thiran 1987)
            const float N=3.f;
            const float a1n = -3.0f + 3.0f*D;
            const float a2n =  3.0f - 6.0f*D + 3.0f*D*D;
            const float a3n = -1.0f + 3.0f*D - 3.0f*D*D + D*D*D;

            a1 = a1n / (N - D);
            a2 = a2n / ((N - D)*(N - D - 1.0f));
            a3 = a3n / ((N - D)*(N - D - 1.0f)*(N - D - 2.0f));

            b0 = a3; b1 = a2; b2 = a1; b3 = 1.0f; // reversed for all-pass
        }
        float process(float x){
            const float y = b0*x + b1*x1 + b2*x2 + b3*x3 - a1*y1 - a2*y2 - a3*y3;
            x3=x2; x2=x1; x1=x; y3=y2; y2=y1; y1=y;
            return y;
        }
        void reset(){ x1=x2=x3=y1=y2=y3=0.f; }
    };

    // Per-channel band splitter + phase rotators
    struct BandChain {
        OnePoleTPT lp1, lp2; // for LR4-like split
        AP2        apLow, apLM, apHM, apHigh;
        void prepare(double fs){
            lp1.setLP(200.0f,(float)fs);
            lp2.setLP(200.0f,(float)fs);
            apLow.reset(); apLM.reset(); apHM.reset(); apHigh.reset();
        }
        void reset(){ lp1.reset(); lp2.reset(); apLow.reset(); apLM.reset(); apHM.reset(); apHigh.reset(); }
    };

    // Alignment state
    struct AlignState {
        int   intDelay = 0;   // integer samples (can be negative)
        float fracDelay = 0;  // [0..3) samples
        Thiran3 fracAP;
        void reset(){ intDelay=0; fracDelay=0; fracAP.reset(); }
    };

    // parameters (smoothed)
    Smoothed pAuto_, pRef_, pLoDeg_, pLmDeg_, pHmDeg_, pHiDeg_, pLoHz_, pMidHz_, pHiHz_, pMix_;

    // runtime
    double sampleRate_ = 44100.0;
    int    maxBlock_   = 512;

    // band processors
    BandChain L_, R_;

    // alignment per channel (apply to the non-reference)
    AlignState align_;

    // cross-corr scratch (fixed max lag ±10ms)
    std::vector<float> delayBufL_, delayBufR_;
    int delayIdx_ = 0, delaySize_ = 0, maxLag_ = 0;

    // helpers
    void updateXovers();
    void updateAllpassPhases();
    void computeAutoAlign(const float* L, const float* R, int n);
    inline void pushDelayRing(float L, float R);
    inline float readDelay(const std::vector<float>& buf, int center, int offset) const;
};
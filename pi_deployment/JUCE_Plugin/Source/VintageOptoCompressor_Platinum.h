#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include <map>

class VintageOptoCompressor_Platinum : public EngineBase
{
public:
    VintageOptoCompressor_Platinum();
    ~VintageOptoCompressor_Platinum() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    juce::String getName() const override { return "Vintage Opto Platinum"; }
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;

    // keep your original names (we read these from your cpp)
    static const std::array<juce::String, 8> kParameterNames;

    // diagnostics (optional)
    struct Metrics { std::atomic<float> cpu{0}, peak{0}; };
    const Metrics& getMetrics() const noexcept { return metrics_; }

private:
    // ---- parameter indices (unchanged) ----
    enum ParamID {
        kParamGain = 0,
        kParamPeakReduction,
        kParamEmphasis,
        kParamOutput,
        kParamMix,
        kParamKnee,
        kParamHarmonics,
        kParamStereoLink
    };

    // ---- utility ----
    static inline float clamp01(float v) noexcept { return juce::jlimit(0.0f, 1.0f, v); }
    static inline bool finitef(float x) noexcept { return std::isfinite(x); }
    static inline float toDB(float x) noexcept   { return 20.0f * std::log10(std::max(x, 1.0e-20f)); }
    static inline float fromDB(float x) noexcept { 
        // Clamp input to reasonable range to prevent NaN/Inf
        x = juce::jlimit(-100.0f, 20.0f, x);
        float result = std::pow(10.0f, x / 20.0f);
        return std::isfinite(result) ? result : 0.0f;
    }

    struct Smoothed {
        std::atomic<float> target{0.f};
        float current = 0.f, coeff = 0.0f; // Start with no smoothing (instant response)
        void setTau(float seconds, float fs) {
            seconds = std::max(1.0e-4f, seconds);
            coeff = std::exp(-1.0f / (seconds * fs));
        }
        float next() { 
            const float t = target.load(std::memory_order_relaxed);
            // If coeff is 0 (not initialized), just return target directly
            if (coeff == 0.0f) {
                current = t;
            } else {
                current = t + (current - t) * coeff;
            }
            return current; 
        }
        void snap()  { current = target.load(std::memory_order_relaxed); }
    };

    // Zavalishin TPT SVF (stable)
    struct TptSVF {
        float g=0, R=0.5f, hp=0, bp=0, lp=0;
        void set(float cutoff, float Q, float fs) {
            cutoff = juce::jlimit(20.0f, 0.47f * fs, cutoff);
            Q      = std::max(0.05f, Q);
            g = std::tan(juce::MathConstants<float>::pi * (cutoff / fs));
            R = 1.0f / (2.0f * Q);
            hp=bp=lp=0.0f;
        }
        float processHP(float x) { const float v1=(x - R*bp - lp)/(1.f + g*(g+R));
                                   const float v2=g*v1; hp=x - R*bp - lp - g*v1;
                                   bp+=v2; lp+=g*v2; return hp; }
        float processLP(float x) { const float v1=(x - R*bp - lp)/(1.f + g*(g+R));
                                   const float v2=g*v1; hp=x - R*bp - lp - g*v1;
                                   bp+=v2; lp+=g*v2; return lp; }
        void reset() { hp=bp=lp=0.f; }
    };

    struct OnePole {
        float a=0.f, z=0.f;
        void setTau(float tauSec, float fs) {
            tauSec = std::max(1.0e-4f, tauSec);
            float e = -1.0f / (tauSec * fs);
            e = juce::jlimit(-60.0f, 0.0f, e);
            a = std::exp(e);
        }
        float process(float x) { z = a*z + (1.0f - a)*x; return z; }
        void reset() { z = 0.f; }
    };

    // params (atomic targets)
    Smoothed pGain_, pPeakReduction_, pEmph_, pOut_, pMix_, pKnee_, pHarm_, pLink_;

    // sidechain EQ
    TptSVF scHP_, scLP_;
    float scTilt_ = 0.0f; // [-1..+1]

    // detector + GR smoothing
    OnePole envAtk_, envRel_;
    float   env_ = 0.0f;
    OnePole grSmooth_; // smooths GR in dB

    // runtime
    double sampleRate_ = 44100.0;
    int    numCh_ = 2;

    // meters
    Metrics metrics_{};

    // helpers
    inline float detectMono(float L, float R) noexcept;
    inline float gainReductionDB(float envLin, float peakRed, float ratio, float kneeDB) noexcept;
};
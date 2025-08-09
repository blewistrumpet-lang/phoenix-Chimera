#pragma once

#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

//============================= TapeEcho =======================================
//
// Safe, DAW-friendly tape delay with wow/flutter, saturation, head bump,
// gap loss, feedback conditioning, and dry/wet mix.
//
//==============================================================================
class TapeEcho : public EngineBase
{
public:
    TapeEcho();
    ~TapeEcho() override = default;

    // EngineBase
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    juce::String getName() const override      { return "Tape Echo"; }
    int          getNumParameters() const override { return 6; }
    juce::String getParameterName(int index) const override;

    // Extended EngineBase API
    void setTransportInfo(const TransportInfo& info) override;
    bool supportsFeature(Feature f) const noexcept override;

    // Param order: 0 Time, 1 Feedback, 2 WowFlutter, 3 Saturation, 4 Mix, 5 Sync

private:
    // --------------------------------- constants
    static constexpr float kMinDelayMs = 10.0f;
    static constexpr float kMaxDelayMs = 2000.0f;
    static constexpr int   kExtraGuard = 4; // delay line guard samples
    static constexpr int   kMaxChannels = 2;

    // --------------------------------- utils
    static inline float flushDenorm(float x) noexcept {
        return (std::abs(x) < 1.0e-30f) ? 0.0f : x;
    }

    struct SmoothParam {
        std::atomic<float> target { 0.0f };
        float current = 0.0f;
        float coeff   = 0.0f; // exp(-1/(tau*fs))

        void setTimeConst(float seconds, float fs) {
            seconds = std::max(1.0e-4f, seconds);
            coeff   = std::exp(-1.0f / (seconds * fs));
        }
        inline float next() noexcept {
            const float t = target.load(std::memory_order_relaxed);
            current = t + (current - t) * coeff;
            return current;
        }
        inline void snap() noexcept { current = target.load(std::memory_order_relaxed); }
    };

    // --------------------------------- safe TPT SVF (lp/hp/bp)
    struct TptSVF {
        float g = 0.0f, R = 0.5f; // R = 1/(2Q)
        float hp = 0.0f, bp = 0.0f, lp = 0.0f;

        void setParams(float cutoff, float Q, float fs) {
            cutoff = juce::jlimit(20.0f, 0.49f * fs, cutoff);
            Q = std::max(0.05f, Q);
            g = std::tan(juce::MathConstants<float>::pi * (cutoff / fs));
            R = 1.0f / (2.0f * Q);
            hp = bp = lp = 0.0f;
        }
        inline float processLP(float x) noexcept {
            // Zavalishin TPT with division-by-zero protection
            const float denom = 1.0f + g * (g + R);
            if (denom < 0.0001f || !std::isfinite(denom)) {
                reset();
                return 0.0f;
            }
            const float v1 = (x - R * bp - lp) / denom;
            const float v2 = g * v1;
            hp = x - R * bp - lp - g * v1;
            bp = bp + v2;
            lp = lp + g * v2;
            return flushDenorm(lp);
        }
        inline float processHP(float x) noexcept {
            const float denom = 1.0f + g * (g + R);
            if (denom < 0.0001f || !std::isfinite(denom)) {
                reset();
                return 0.0f;
            }
            const float v1 = (x - R * bp - lp) / denom;
            const float v2 = g * v1;
            hp = x - R * bp - lp - g * v1;
            bp = bp + v2;
            lp = lp + g * v2;
            return flushDenorm(hp);
        }
        inline float processBP(float x) noexcept {
            const float denom = 1.0f + g * (g + R);
            if (denom < 0.0001f || !std::isfinite(denom)) {
                reset();
                return 0.0f;
            }
            const float v1 = (x - R * bp - lp) / denom;
            const float v2 = g * v1;
            hp = x - R * bp - lp - g * v1;
            bp = bp + v2;
            lp = lp + g * v2;
            return flushDenorm(bp);
        }
        void reset() noexcept { hp = bp = lp = 0.0f; }
    };

    // --------------------------------- Delay line with safe wrapped 4-tap cubic
    class DelayLine {
    public:
        void prepare(double sampleRate, float maxDelayMs);
        void clear();
        inline void write(float x) noexcept {
            if (!buf_.empty() && w_ >= 0 && w_ < static_cast<int>(buf_.size())) {
                buf_[w_] = x;
                w_ = (w_ + 1) % static_cast<int>(buf_.size());
            }
        }
        float readCubic(float delaySamples) const noexcept;
        inline int capacity() const noexcept { 
            return buf_.empty() ? 0 : std::max(0, static_cast<int>(buf_.size()) - kExtraGuard); 
        }

    private:
        std::vector<float> buf_;
        int size_ = 0;
        int w_ = 0;
    };

    // --------------------------------- Modulation (wow/flutter/slow drift)
    struct Modulators {
        float phWow=0, phFlut1=0, phFlut2=0, phDrift=0, phScrape=0;
        float inc = 0.0f; // 2π/fs cached
        float rndTarget = 0.0f;
        float rndState  = 0.0f;
        uint32_t rng = 1;

        static constexpr float kWowRate = 0.5f;
        static constexpr float kFl1     = 5.2f;
        static constexpr float kFl2     = 6.7f;
        static constexpr float kDrift   = 0.08f;
        static constexpr float kScrape  = 47.0f;

        static constexpr float dWow   = 0.015f;   // ±1.5%
        static constexpr float dFl1   = 0.004f;
        static constexpr float dFl2   = 0.003f;
        static constexpr float dDrift = 0.008f;
        static constexpr float dScr   = 0.0005f;

        inline float fastRandBi() noexcept {
            rng = rng * 1664525u + 1013904223u;
            return (float)((rng & 0x7fffffff) / 1073741824.0f - 1.0f); // ~[-1,1]
        }

        void prepare(double fs) { inc = 2.0f * juce::MathConstants<float>::pi / (float)fs; }
        void reset() { phWow=phFlut1=phFlut2=phDrift=phScrape=0.0f; rndTarget=rndState=0.0f; rng=1; }

        inline void updateRandomOncePerBlock() { rndTarget = 0.3f * fastRandBi(); }

        inline float process(float amt) {
            phWow    += kWowRate   * inc;
            phFlut1  += kFl1       * inc;
            phFlut2  += kFl2       * inc;
            phDrift  += kDrift     * inc;
            phScrape += kScrape    * inc;
            auto wrap = [](float& p){ 
                p = std::fmod(p, 2.0f * juce::MathConstants<float>::pi);
                if (p < 0.0f) p += 2.0f * juce::MathConstants<float>::pi;
                if (!std::isfinite(p)) p = 0.0f; // Safety check
            };
            wrap(phWow); wrap(phFlut1); wrap(phFlut2); wrap(phDrift); wrap(phScrape);

            rndState += (rndTarget - rndState) * 0.001f;

            const float sum =
                std::sin(phWow)   * dWow +
                std::sin(phFlut1) * dFl1 +
                std::sin(phFlut2) * dFl2 +
                std::sin(phDrift) * dDrift +
                std::sin(phScrape)* dScr +
                rndState * 0.002f;

            // Safety check for NaN/Inf
            if (!std::isfinite(sum)) return 0.0f;
            
            // Map as tape speed modulation: delay ∝ 1/speed -> approx (1 / (1 + s))
            const float s = juce::jlimit(-0.05f, 0.05f, sum * amt);
            return -s; // negative for delay change (increase speed -> shorter delay)
        }
    };

    // --------------------------------- "Tape" cluster per channel
    struct ChannelState {
        DelayLine delay;
        Modulators mod;

        // EQ stages
        TptSVF preEmphHP;    // record pre-emphasis-ish
        TptSVF headBumpBP;   // low-mid bump (BP)
        TptSVF gapLossLP;    // playback HF rolloff

        // Feedback conditioning
        float hpState = 0.0f;
        float hpAlpha = 0.0f;
        float lpState = 0.0f;
        float lpAlpha = 0.0f;

        void prepare(double fs) {
            delay.prepare(fs, kMaxDelayMs);
            mod.prepare(fs);
            preEmphHP.setParams(3000.0f, 0.707f, (float)fs);
            headBumpBP.setParams(120.0f, 1.2f,  (float)fs);
            gapLossLP.setParams(10000.0f, 0.707f, (float)fs);
            // fixed HP in feedback around 100 Hz
            const float hpFreq = 100.0f;
            hpAlpha = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * hpFreq / (float)fs);
            // LP alpha will be set dynamically per feedback amount
            lpAlpha = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 6000.0f / (float)fs);
            hpState = lpState = 0.0f;
        }
        void reset() {
            delay.clear();
            mod.reset();
            preEmphHP.reset(); headBumpBP.reset(); gapLossLP.reset();
            hpState = lpState = 0.0f;
        }
    };

    // --------------------------------- parameters
    SmoothParam pTime_;       // [0..1] -> 10..2000 ms
    SmoothParam pFeedback_;   // [0..1]
    SmoothParam pWowFlutter_; // [0..1]
    SmoothParam pSaturation_; // [0..1]
    SmoothParam pMix_;        // [0..1]
    SmoothParam pSync_;       // [0..1] -> sync on/off

    // --------------------------------- runtime
    double sampleRate_ = 44100.0;
    std::array<ChannelState, kMaxChannels> ch_{};
    
    // --------------------------------- transport sync
    TransportInfo transportInfo_;
    
    // Beat division mapping (same as BufferRepeat_Platinum)
    enum class BeatDivision {
        DIV_1_64,    // 1/64 note
        DIV_1_32,    // 1/32 note  
        DIV_1_16,    // 1/16 note
        DIV_1_8,     // 1/8 note
        DIV_1_4,     // 1/4 note (quarter)
        DIV_1_2,     // 1/2 note (half)
        DIV_1_1,     // 1 bar
        DIV_2_1,     // 2 bars
        DIV_4_1      // 4 bars
    };
    
    // Helper methods
    float calculateSyncedDelayTime(float timeParam, float syncParam) const;
    float getBeatDivisionMs(BeatDivision division) const;

    // --------------------------------- helpers
    inline float softSaturate(float x) noexcept {
        // mild symmetric limiter
        return std::tanh(x * 1.5f) * (1.0f / 1.5f);
    }
    inline float saturateTape(float x, float amt) noexcept {
        // simple tape-ish curve with bias-less soft knee
        const float drive = 1.0f + 4.0f * juce::jlimit(0.0f, 1.0f, amt);
        const float y = std::tanh(x * drive * 0.8f);
        return y / (0.9f * drive);
    }

    float processSample(float in, ChannelState& cs) noexcept;
};
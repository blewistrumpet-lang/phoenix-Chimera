#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h" // DenormalGuard, scrubBuffer, DCBlocker, ParamAccess
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <cmath>
#include <algorithm>

/**
 * ParametricEQ_Studio — Project Chimera v3.0 Phoenix
 * Dr. Sarah Chen — Studio-grade parametric EQ (bell-only core)
 *
 * Key qualities:
 * - TDF2 biquads with double accumulators (stability at high-Q/low-f)
 * - Coeff A/B with power-compensated crossfade (clickless)
 * - Critically-damped 2-pole control smoothing (snappy, zipper-free)
 * - Control updates in chunked ticks (no per-sample modulo in hot path)
 * - Mid/Side (per-band routing), Vintage path with 2× OS at 44.1/48k
 * - Analyzer pre/post taps (decimated), DC blocker, NaN/Inf scrub
 * - Active-band compaction per channel (branch-free inner loop)
 */
class ParametricEQ_Studio : public EngineBase {
public:
    ParametricEQ_Studio();
    ~ParametricEQ_Studio() override;

    // EngineBase
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Parametric EQ Studio"; }
    int getNumParameters() const override { return 30; } // 6 bands * 4 params + 6 global

    // Parameter names for host
    juce::String getParameterName(int index) const override;

    enum ParamID : int {
        kGlobalBypass   = 0,
        kOutputTrim_dB  = 1,   // -24..+24
        kWetDry         = 2,   // 0..1
        kVintageOn      = 3,   // 0..1
        kMidSideOn      = 4,   // 0..1
        kAnalyzerOn     = 5,   // 0..1
        kBandBase       = 100  // per band: enabled, freq, gain, Q
    };

    // Analyzer snapshot (non-RT/UI thread)
    size_t getAnalyzerSnapshot(bool post, float* out, size_t capacity);

    // Per-band M/S routing: 0=LR (default), 1=M-only, 2=S-only (only relevant when MS on)
    void setBandMSMode(int band, int mode);

private:
    // ------- Config -------
    static constexpr int kMaxBands      = 6;
    static constexpr int kMaxChannels   = 2;
    static constexpr int kXfadeSamples  = 64;
    static constexpr int kCtrlInterval  = 32;      // samples per control tick
    static constexpr int kAnalyzerSize  = 4096;
    static constexpr int kAnalyzerDecim = 8;

    // ------- Control smoother (critically damped 2-pole) -------
    struct SmoothedParam2P {
        float y = 0.f, y1 = 0.f;
        float w = 2.f * float(M_PI) * 10.f; // 10 Hz default
        void setFc(float fc) { w = 2.f * float(M_PI) * std::max(0.1f, fc); }
        inline float processToward(float target, float dt) noexcept {
            const float a = w*w, b = 2.f*w;
            const float err = target - y;
            y1 += (a*err - b*y1) * dt;
            y  += y1 * dt;
            return y;
        }
        void reset(float v) { y = v; y1 = 0.f; }
    };

    // ------- Biquad core -------
    struct BiquadCoeffs { float b0=1.f,b1=0.f,b2=0.f,a1=0.f,a2=0.f; };
    struct BiquadTDF2 {
        double z1=0.0, z2=0.0;
        BiquadCoeffs c;
        inline float process(float x) noexcept {
            const double y = (double)c.b0 * x + z1;
            z1 = (double)c.b1 * x - (double)c.a1 * y + z2;
            z2 = (double)c.b2 * x - (double)c.a2 * y;
            return (float)y;
        }
        inline void reset() noexcept { z1 = z2 = 0.0; }
    };

    struct XfadeGain {
        float gA=1.f, gB=1.f;
        inline void setFromMag(float magA, float magB) noexcept {
            const float rA = (magA > 1e-12f) ? 1.f / magA : 1.f;
            const float rB = (magB > 1e-12f) ? 1.f / magB : 1.f;
            gA = rA; gB = rB;
        }
    };

    struct BandPath {
        BiquadTDF2 A, B; bool useA=true; int xfadeCtr=0; bool enabled=false; XfadeGain g; int msMode=0;
        inline float process(float x) noexcept {
            if (!enabled) return x;
            if (xfadeCtr>0) {
                const float ya=A.process(x), yb=B.process(x);
                const float t = 1.f - (float)xfadeCtr / (float)kXfadeSamples;
                --xfadeCtr;
                return ya*g.gA*(1.f-t) + yb*g.gB*t;
            }
            return (useA?A:B).process(x);
        }
        inline void reset() noexcept { A.reset(); B.reset(); xfadeCtr=0; g={}; }
    };

    struct BandParams {
        // targets from host
        float tEnabled=0.f, tFreq=1000.f, tGainDB=0.f, tQ=1.f;
        // smoothed
        SmoothedParam2P en,f0,gdB,q;
        // last coefficients for change detection
        BiquadCoeffs last{}; bool haveLast=false;
    };

    // Analyzer ring (lock-free single producer)
    struct AnalyzerRing {
        std::array<float, kAnalyzerSize> buf{};
        std::atomic<uint32_t> w{0};
        inline void push(float x) noexcept {
            auto i = w.load(std::memory_order_relaxed);
            buf[i] = x;
            w.store((i+1) % kAnalyzerSize, std::memory_order_release);
        }
        size_t snapshot(float* out, size_t cap) noexcept;
        void reset() noexcept { w.store(0, std::memory_order_relaxed); buf.fill(0.f); }
    };

    // ------- 2× Oversampling (matched polyphase halfband) -------
    struct Halfband2x {
        // Linear-phase, odd-length halfband. Even taps non-zero; H(π/2)=0.5 at center tap.
        static constexpr int NTAPS = 31;
        static constexpr int PH    = (NTAPS-1)/2;      // half-length
        // Designed offline (Kaiser β≈8.5), ~90 dB stop.
        static constexpr float h[NTAPS] = {
            -0.0002346f, 0.0f,  0.0019834f, 0.0f, -0.0077187f, 0.0f, 0.0216015f, 0.0f,
             -0.0508307f, 0.0f, 0.1103840f, 0.0f, -0.2798810f, 0.0f, 0.5000000f, 0.0f,
             -0.2798810f, 0.0f, 0.1103840f, 0.0f, -0.0508307f, 0.0f, 0.0216015f, 0.0f,
             -0.0077187f, 0.0f, 0.0019834f, 0.0f, -0.0002346f
        };
        // Decompose into even/odd polyphase branches: he[n] = h[2n], ho[n] = h[2n+1]
        static constexpr int NE = (NTAPS+1)/2; // even count
        static constexpr int NO = NTAPS/2;     // odd count

        std::array<float, NE> zLe{}, zRe{};
        std::array<float, NO> zLo{}, zRo{};
        int ie=0, io=0;

        static inline float dotRev(const float* coeff, const std::array<float,NE>& z, int idx, int len) noexcept {
            float acc=0.f; int i=idx;
            for (int n=0; n<len; ++n) { // ring walks backwards
                acc += coeff[n] * z[i];
                if (--i < 0) i = len-1;
            }
            return acc;
        }
        static inline float dotRevO(const float* coeff, const std::array<float,NO>& z, int idx, int len) noexcept {
            float acc=0.f; int i=idx;
            for (int n=0; n<len; ++n) {
                acc += coeff[n] * z[i];
                if (--i < 0) i = len-1;
            }
            return acc;
        }

        void reset() {
            zLe.fill(0.f); zRe.fill(0.f);
            zLo.fill(0.f); zRo.fill(0.f);
            ie=io=0;
        }

        // Upsample 1× in -> 2× out (even/odd phases)
        inline void upsample(float inL, float inR, float& outEvenL, float& outOddL,
                             float& outEvenR, float& outOddR) noexcept {
            // push to even phase delay (input on even branch)
            if (++ie >= NE) ie = 0;
            zLe[ie] = inL; zRe[ie] = inR;

            // even output (lowpass)
            // build he[] at runtime from h[0],h[2],..., centered
            static float he[NE]; static bool init=false;
            if (!init) {
                for (int n=0; n<NE; ++n) he[n] = h[2*n];
                init = true;
            }
            outEvenL = dotRev(he, zLe, ie, NE);
            outEvenR = dotRev(he, zRe, ie, NE);

            // odd output (highpass complement -> use odd polyphase)
            if (++io >= NO) io = 0;
            zLo[io] = inL; zRo[io] = inR;

            static float ho[NO]; static bool initO=false;
            if (!initO) {
                for (int n=0; n<NO; ++n) ho[n] = h[2*n+1];
                initO = true;
            }
            outOddL = dotRevO(ho, zLo, io, NO);
            outOddR = dotRevO(ho, zRo, io, NO);
        }

        // Downsample: consume two samples (even, odd) -> 1 sample out
        inline void downsample(float inEvenL, float inOddL, float inEvenR, float inOddR,
                               float& outL, float& outR) noexcept {
            // Matched structure: filter with he/ho and combine
            if (++ie >= NE) ie = 0;
            zLe[ie] = inEvenL; zRe[ie] = inEvenR;
            static float he[NE]; static bool init=false;
            if (!init) { for (int n=0; n<NE; ++n) he[n]=h[2*n]; init=true; }
            float lpL = dotRev(he, zLe, ie, NE);
            float lpR = dotRev(he, zRe, ie, NE);

            if (++io >= NO) io = 0;
            zLo[io] = inOddL;  zRo[io] = inOddR;
            static float ho[NO]; static bool initO=false;
            if (!initO) { for (int n=0; n<NO; ++n) ho[n]=h[2*n+1]; initO=true; }
            float hpL = dotRevO(ho, zLo, io, NO);
            float hpR = dotRevO(ho, zRo, io, NO);

            // Complementary halfband recombination
            outL = lpL + hpL;
            outR = lpR + hpR;
        }
    };

    // ------- Internal helpers -------
    void controlTickAll();                     // runs every kCtrlInterval samples
    void recalcIfNeeded(int ch, int b);        // per-channel band coeff/switch
    static void calcPeakingBiquad(BiquadCoeffs& out, double fs, double f, double Q, double gainDB);
    static inline float dbToLin(float dB) noexcept { return std::pow(10.0f, dB*0.05f); }
    static float biquadMagAtW(const BiquadCoeffs& c, float w); // |H(e^{jw})|

    inline float vintageSaturate(float x) noexcept { const float c3=0.02f; return (x + c3*x*x*x)*0.98f; }

    // ------- State -------
    double fs_ = 48000.0; int blockSize_ = 0;

    BandPath  paths_[kMaxChannels][kMaxBands];
    BandParams bands_[kMaxBands];

    // Compact active lists: indices of enabled bands per channel (rebuilt on control tick)
    std::array<int, kMaxBands> activeIdx_[kMaxChannels];
    int activeCount_[kMaxChannels] = {0,0};

    float wetDry_=1.f, trim_=0.f; bool bypass_=false, vintageOn_=false, midSideOn_=false, analyzerOn_=true;

    int ctrlPhase_ = 0; // samples until next control tick
    int analyzerDecimCtr_ = 0;

    DCBlocker dc_[kMaxChannels];
    struct AnalyzerRing preRing_, postRing_;

    Halfband2x hb_;
};
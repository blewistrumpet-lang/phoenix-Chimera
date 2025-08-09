// PlatinumRingModulator.h — hardened, RT-safe rewrite (APVTS unchanged)
#pragma once
#include "EngineBase.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <memory>

// Cross-platform ALWAYS_INLINE
#if defined(_MSC_VER)
  #define ALWAYS_INLINE __forceinline
#else
  #define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

class PlatinumRingModulator final : public EngineBase {
public:
    PlatinumRingModulator();
    ~PlatinumRingModulator() override = default;

    // EngineBase
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    juce::String getName() const override { return "Platinum Ring Modulator"; }
    int getNumParameters() const override { return 12; }
    juce::String getParameterName(int index) const override;

private:
    // ---------- Utilities ----------
    template<typename T>
    static ALWAYS_INLINE T clampFinite(T v, T lo, T hi) noexcept {
        if (!std::isfinite(v)) return T{0};
        return v < lo ? lo : (v > hi ? hi : v);
    }
    template<typename T>
    static ALWAYS_INLINE T flushDenorm(T v) noexcept {
       #if defined(__SSE2__)
        // add 0 to flush denormals under FTZ/DAZ
        return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(static_cast<float>(v)), _mm_set_ss(0.0f)));
       #else
        const T thr = static_cast<T>(1e-30);
        return std::abs(v) < thr ? T(0) : v;
       #endif
    }

    // ---------- Smoothed parameter (atomic target) ----------
    struct SmoothParam {
        std::atomic<float> target{0.0f};
        float current{0.0f};
        float a{0.995f};
        void setTimeMs(float ms, double sr) {
            const double tc = std::max(1e-3, double(ms))*0.001;
            a = std::exp(-1.0 / (tc*sr));
        }
        ALWAYS_INLINE float tick() noexcept {
            const float t = target.load(std::memory_order_relaxed);
            current = t + (current - t) * a;
            return flushDenorm(current);
        }
        void snap(float v) noexcept { target.store(v, std::memory_order_relaxed); current = v; }
    };

    // ---------- Carrier osc (simple, band-limitedish via tanh soft clip) ----------
    struct CarrierOsc {
        double phase{0.0}, inc{0.0};
        float pulseWidth{0.5f};
        float subMix{0.0f};
        float stretch{1.0f};
        std::array<double,8> harmPhase{}; // 0..1 cycles

        void setFreq(float hz, double sr) noexcept {
            hz = clampFinite(hz, 0.0f, float(sr*0.45));
            inc = double(hz) / sr; // [cycles per sample]
        }
        void reset() noexcept { phase = 0.0; harmPhase.fill(0.0); }

        ALWAYS_INLINE float tick() noexcept {
            // Additive + pulse + sub (cheap but stable)
            float s = 0.0f;
            for (int h=0; h<8; ++h) {
                harmPhase[h] += inc * (double(h+1) * double(stretch));
                if (harmPhase[h] >= 1.0) harmPhase[h] -= 1.0;
                s += std::sin(float(harmPhase[h] * 2.0 * M_PI)) * (1.0f / float(h+1));
            }
            // crude pulse (pre-bandlimited by tanh at the end)
            const float pulse = (float(phase) < pulseWidth) ? 1.0f : -1.0f;
            s = s*0.7f + pulse*0.3f;

            // sub osc (one octave down)
            double subP = phase * 0.5;
            if (subP >= 1.0) subP -= 1.0;
            s = s*(1.0f - subMix) + std::sin(float(subP * 2.0 * M_PI))*subMix;

            // advance
            phase += inc;
            if (phase >= 1.0) phase -= 1.0;

            // tanh soft clip to tame harmonics
            const float x = s * 0.8f;
            return std::tanh(x);
        }
    };

    // ---------- Minimal Hilbert (odd-length FIR, windowed, stable) ----------
    struct HilbertFIR {
        static constexpr int N = 63; // odd
        std::array<float,N> z{}; // delay
        std::array<float,N> h{}; // coeffs (90°)
        int w{0};

        void prepare() {
            // Ideal Hilbert impulse: h[n] = 2/(pi*n) for n odd, 0 otherwise, center=0
            // Then window (Blackman) and normalize for safety.
            const int C = N/2;
            for (int i=0;i<N;++i) {
                const int n = i - C;
                float v = 0.0f;
                if (n != 0 && (n & 1)) v = 2.0f / (float(M_PI) * float(n));
                // Blackman
                const float wBlack =
                    0.42f - 0.5f*std::cos(2.0f*float(M_PI)*float(i)/(N-1))
                          + 0.08f*std::cos(4.0f*float(M_PI)*float(i)/(N-1));
                h[i] = v * wBlack;
            }
            std::fill(z.begin(), z.end(), 0.0f);
            w = 0;
        }

        // Returns analytic pair {real=delayed, imag=hilbert(real)}
        ALWAYS_INLINE std::complex<float> process(float x) noexcept {
            z[w] = x;
            // FIR (imag)
            float im = 0.0f;
            int idx = w;
            for (int i=0;i<N;++i) {
                im += z[idx] * h[i];
                if (--idx < 0) idx = N-1;
            }
            // real is delayed by (N-1)/2 to match group delay
            int rd = w - (N-1)/2; if (rd < 0) rd += N;
            const float re = z[rd];
            if (++w == N) w = 0;
            return { flushDenorm(re), flushDenorm(im) };
        }

        void reset() { std::fill(z.begin(), z.end(), 0.0f); w=0; }
    };

    // ---------- Lightweight YIN (bounded, decimated, safe) ----------
    struct Yin {
        static constexpr int BUF = 1024;
        static constexpr int HALF = BUF/2;
        static constexpr float THRESH = 0.15f;

        std::array<float,BUF> x{}; int wp{0};
        float lastHz{440.0f};
        int filled{0};

        void reset() { x.fill(0.0f); wp=0; lastHz=440.0f; filled=0; }

        // run every k samples (decimation) not per sample
        float detectPush(float s, double sr, int decimCounter) noexcept {
            x[wp] = flushDenorm(s);
            if (++wp == BUF) wp = 0;
            if (filled < BUF) { ++filled; return lastHz; }
            // do YIN only sporadically to bound CPU
            if ((decimCounter & 31) != 0) return lastHz;

            // difference
            std::array<float,HALF> d{};
            for (int tau=0; tau<HALF; ++tau) {
                float sum=0.0f;
                // cap inner loop, aligned to HALF
                for (int i=0;i<HALF;++i) {
                    int a = (wp - i); if (a<0) a+=BUF;
                    int b = (wp - i - tau); if (b<0) b+=BUF;
                    const float diff = x[a]-x[b];
                    sum += diff*diff;
                }
                d[tau] = sum;
            }
            // cumulative mean normalize
            float cum=0.0f;
            std::array<float,HALF> yin{};
            yin[0] = 1.0f;
            for (int tau=1; tau<HALF; ++tau) {
                cum += d[tau];
                const float v = (cum <= 1e-20f ? 1.0f : d[tau] * (float)tau / cum);
                yin[tau] = v;
            }
            // absolute threshold
            int best=-1;
            for (int tau=2; tau<HALF; ++tau) {
                if (yin[tau] < THRESH) { best=tau; break; }
            }
            if (best < 0) return lastHz;

            // parabolic refine
            if (best <= 1 || best >= HALF-2) {
                lastHz = clampFinite(float(sr / best), 20.0f, 20000.0f);
                return lastHz;
            }
            const float s0 = yin[best-1], s1 = yin[best], s2 = yin[best+1];
            const float denom = (s0 + s2 - 2.0f*s1);
            const float shift = (std::abs(denom) > 1e-12f) ? 0.5f*(s0 - s2)/denom : 0.0f;
            const float tauR = float(best) + clampFinite(shift, -1.0f, 1.0f);
            const float hz = clampFinite(float(sr / std::max(1.0f, tauR)), 20.0f, 20000.0f);
            lastHz = hz;
            return lastHz;
        }
    };

    // ---------- Simple state-variable bandpass (stable) ----------
    struct SVF {
        float g{0}, k{0.5f};
        float s1{0}, s2{0};
        void set(float hz, float q, double sr) {
            hz = clampFinite(hz, 10.0f, float(sr*0.45));
            q = std::max(0.2f, q);
            g = std::tan(float(M_PI) * hz / float(sr));
            k = 1.0f / q;
        }
        ALWAYS_INLINE float bp(float x) noexcept {
            // Zavalishin style SVF
            const float v1 = (x - k*s1 - s2) / (1.0f + g*(g + k));
            const float v2 = g*v1 + s1;
            const float v3 = g*v2 + s2;
            s1 = v2 + g*v1;
            s2 = v3 + g*v2;
            return flushDenorm(v2);
        }
        void reset(){ s1=s2=0; }
    };

    // ---------- Per-channel state ----------
    struct Channel {
        HilbertFIR hilb;
        Yin yin;
        SVF svf;
        std::array<float,8192> fbDelay{}; // feedback
        int fbW{0};
        std::array<float,8192> shim{}; // shimmer delay
        int shW{0};
        float dcX{0}, dcY{0};
        int yinDecim{0};

        void prepare(double /*sr*/) { hilb.prepare(); reset(); }
        void reset() {
            hilb.reset(); yin.reset(); svf.reset();
            std::fill(fbDelay.begin(), fbDelay.end(), 0.0f); fbW=0;
            std::fill(shim.begin(), shim.end(), 0.0f); shW=0;
            dcX=dcY=0; yinDecim=0;
        }
        ALWAYS_INLINE float dcBlock(float x) noexcept {
            // y[n] = x[n] - x[n-1] + R*y[n-1]
            const float R = 0.995f;
            const float y = x - dcX + R*dcY;
            dcX = x; dcY = flushDenorm(y);
            return y;
        }
    };

    // ---------- Engine state ----------
    double sr_{44100.0};
    int maxBlock_{512};
    bool usePitchTrack_{true};

    // Smoothed params (targets fed from APVTS upstream via updateParameters)
    SmoothParam p_carrierHz;      // idx 0 (mapped 20..5k in cpp)
    SmoothParam p_ringAmt;        // idx 1
    SmoothParam p_freqShiftNorm;  // idx 2 (-1..+1)
    SmoothParam p_feedback;       // idx 3
    SmoothParam p_pulseWidth;     // idx 4
    SmoothParam p_phaseMod;       // idx 5 (not used heavily; kept for compat)
    SmoothParam p_stretch;        // idx 6 (harmonic stretch)
    SmoothParam p_tilt;           // idx 7 (-1..+1)
    SmoothParam p_resonance;      // idx 8
    SmoothParam p_shimmer;        // idx 9
    SmoothParam p_thermal;        // idx 10
    SmoothParam p_pitchTrack;     // idx 11 [0..1] mix

    CarrierOsc carrier_;
    std::array<Channel,2> ch_;

    // helpers
    static ALWAYS_INLINE float softClip(float x) noexcept {
        // fast tanh-ish
        const float x2 = x*x;
        return x * (27.0f + x2) / (27.0f + 9.0f*x2);
    }
    float processRing(float in, float carrier, float amt) noexcept;
    float processFreqShift(float in, float norm, Channel& c) noexcept;
    void  processFeedback(float& x, float fbAmt, Channel& c) noexcept;
    void  processResonance(float& x, float resAmt, float baseHz, Channel& c) noexcept;
    void  processShimmer(float& x, float shimAmt, Channel& c) noexcept;
};
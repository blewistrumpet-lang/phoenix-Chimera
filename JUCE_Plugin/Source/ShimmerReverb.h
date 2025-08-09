// ShimmerReverb.h — hardened, RT-safe rewrite (APVTS unchanged)
#pragma once
#include "EngineBase.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>

class ShimmerReverb final : public EngineBase {
public:
    ShimmerReverb();
    ~ShimmerReverb() override = default;

    // EngineBase
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Shimmer Reverb"; }

    // Must match your original APVTS indices
    enum class ParamID : int {
        Size = 0,       // room size / base delay scale
        Shimmer = 1,    // level of octave-up return
        Pitch = 2,      // 0..1 mapped to 0..+12 semitones target for shimmer
        Damping = 3,    // HF damping in loop
        Diffusion = 4,  // allpass amount
        Modulation = 5, // LFO depth/rate coupling
        Predelay = 6,   // 0..250ms
        Width = 7,      // mid/side mix
        Freeze = 8,     // hold tail
        Mix = 9         // dry/wet
    };

private:
    // --------- helpers ----------
    template<typename T>
    static inline T flushDenorm(T v) noexcept {
       #if defined(__SSE2__)
        return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(static_cast<float>(v)),
                                        _mm_set_ss(0.0f)));
       #else
        const T tiny = static_cast<T>(1e-30);
        return std::abs(v) < tiny ? T(0) : v;
       #endif
    }
    template<typename T>
    static inline T clamp01(T v) noexcept { return v < T(0) ? T(0) : (v > T(1) ? T(1) : v); }
    template<typename T>
    static inline T clamp(T v, T lo, T hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }

    struct Smooth {
        std::atomic<float> target{0.0f};
        float current{0.0f};
        float a{0.995f};
        void setTimeMs(float ms, double sr) {
            const double tc = std::max(1e-3, double(ms)) * 0.001;
            a = std::exp(-1.0 / (tc * sr));
        }
        inline float tick() noexcept {
            const float t = target.load(std::memory_order_relaxed);
            current = t + (current - t) * a;
            return flushDenorm(current);
        }
        void snap(float v) noexcept { target.store(v, std::memory_order_relaxed); current = v; }
    };

    // --------- DSP blocks ----------
    struct OnePoleLPF {
        float a{0.0f}, z{0.0f}; // y += a*(x - y)
        void setCutoff(float hz, double sr) {
            hz = std::max(5.0f, std::min(hz, float(sr*0.45)));
            const float r = std::exp(-2.0f * float(M_PI) * hz / float(sr));
            a = 1.0f - r;
        }
        inline float process(float x) noexcept { z += a * (x - z); return flushDenorm(z); }
        void reset() { z = 0.0f; }
    };

    struct AllpassMod {
        std::vector<float> buf;
        int w{0};
        float g{0.6f};
        // mod state
        float lfo{0.0f}, lfoInc{0.0f}, depth{0.0f};
        double sr{44100.0};
        void prepare(int maxDelay, double srIn) {
            buf.assign(std::max(64, maxDelay), 0.0f);
            w = 0; sr = srIn; lfo = 0.0f; lfoInc = 0.0f; depth = 0.0f;
        }
        void set(float gain, float rateHz, float depthSamples) {
            g = clamp(gain, -0.999f, 0.999f);
            lfoInc = 2.0f * float(M_PI) * clamp(rateHz, 0.01f, 10.0f) / float(sr);
            depth = std::max(0.0f, depthSamples);
        }
        inline float process(float x) noexcept {
            // fractional delay via simple linear (cheap and stable)
            const float dBase = 40.0f; // samples
            lfo += lfoInc; if (lfo > float(M_PI)*2.0f) lfo -= float(M_PI)*2.0f;
            const float d = dBase + depth * std::sin(lfo);

            const int size = (int)buf.size();
            const int di = (int) d;
            const float frac = d - di;

            int r1 = w - di; if (r1 < 0) r1 += size;
            int r2 = r1 - 1; if (r2 < 0) r2 += size;

            const float v1 = buf[r1];
            const float v2 = buf[r2];
            const float dlin = v1 + (v2 - v1) * frac;

            // allpass: y = -x + d + g*z; z = x + g*y
            const float y = flushDenorm(-x + dlin + g * buf[w]);
            buf[w] = flushDenorm(x + g * y);
            if (++w == size) w = 0;
            return y;
        }
        void reset() { std::fill(buf.begin(), buf.end(), 0.0f); w = 0; lfo = 0.0f; }
    };

    struct Delay {
        std::vector<float> buf;
        int w{0};
        void prepare(int samples) { buf.assign(std::max(64, samples), 0.0f); w = 0; }
        inline float read(int d) const noexcept {
            const int size = (int)buf.size();
            int r = w - d; if (r < 0) r += size;
            return buf[r];
        }
        inline void write(float x) noexcept {
            buf[w] = flushDenorm(x);
            if (++w == (int)buf.size()) w = 0;
        }
        void reset() { std::fill(buf.begin(), buf.end(), 0.0f); w = 0; }
    };

    // Simple, robust 12-semitone pitch shifter: dual-head resampler with crossfade
    struct OctaveUpShifter {
        // Two read heads run at >1x; when a head nears buffer end, crossfade to the other.
        std::vector<float> buf;
        int w{0};
        double sr{44100.0};
        // heads
        double rA{0.0}, rB{0.5}; // positions (0..buf.size)
        double rate{2.0};        // +12 semitones ~2.0
        float xfade{0.0f};       // 0..1 crossfade weight
        float xfadeStep{0.0f};
        int  xfadeSamples{256};  // short crossfade

        void prepare(int maxSamples, double srIn) {
            buf.assign(std::max(2048, maxSamples), 0.0f);
            w = 0; sr = srIn;
            rA = 0.0; rB = buf.size() * 0.5;
            rate = 2.0; xfade = 0.0f; xfadeStep = 1.0f / float(std::max(8, xfadeSamples));
        }
        inline void push(float x) noexcept {
            buf[w] = flushDenorm(x);
            if (++w == (int)buf.size()) w = 0;
        }
        inline float tap(double pos) const noexcept {
            // linear interpolation (safe)
            const int size = (int)buf.size();
            while (pos < 0.0) pos += size;
            while (pos >= size) pos -= size;
            int i0 = (int)pos;
            int i1 = (i0 + 1) % size;
            const float frac = (float)(pos - i0);
            const float y0 = buf[i0], y1 = buf[i1];
            return y0 + (y1 - y0) * frac;
        }
        inline float process() noexcept {
            // advance heads in write-domain timeline
            rA += rate; rB += rate;
            const int size = (int)buf.size();
            if (rA >= size) { rA -= size; xfade = 0.0f; }
            if (rB >= size) { rB -= size; xfade = 0.0f; }

            const float a = tap(rA);
            const float b = tap(rB);
            // slow crossfade to avoid phasing; increase when we detect wrap
            xfade = clamp01(xfade + xfadeStep);
            const float y = a * (1.0f - xfade) + b * xfade;
            return flushDenorm(y * 0.8f); // headroom
        }
        void setSemitones(float st) {
            // clamp 0..12 semitones
            st = clamp(st, 0.0f, 12.0f);
            rate = std::pow(2.0, st / 12.0);
        }
        void reset() {
            std::fill(buf.begin(), buf.end(), 0.0f);
            w = 0; rA = 0.0; rB = buf.size()*0.5; xfade = 0.0f;
        }
    };

    // --------- Reverb core (4 lines, modulated diffusion) ----------
    struct Line {
        Delay delay;
        AllpassMod ap1, ap2;
        OnePoleLPF damp;
        float state{0.0f};
        void reset() { delay.reset(); ap1.reset(); ap2.reset(); damp.reset(); state = 0.0f; }
    };

    // --------- State ----------
    double sr_{44100.0};
    int maxBlock_{512};

    // smoothed params
    Smooth pSize, pShimmer, pPitch, pDamp, pDiff, pMod, pPredelay, pWidth, pFreeze, pMix;

    // predelay
    Delay preDelay_;

    // four FDN-ish lines
    static constexpr int kLines = 4;
    std::array<Line, kLines> L_;
    // fixed base lengths (in samples @ 48k), scaled by Size
    std::array<int, kLines> baseLen48_ { 1499, 1733, 1949, 2179 };

    // feedback gains per line (stable & <1)
    std::array<float, kLines> fbGain_ { 0.72f, 0.73f, 0.74f, 0.75f };

    // shimmer path (mono-in stereo-out summing later)
    OctaveUpShifter shimmer_;

    // width mid/side mix
    inline void stereoWidth(float& L, float& R, float width01) noexcept {
        width01 = clamp01(width01);
        const float mid = 0.5f * (L + R);
        const float side = 0.5f * (L - R) * (0.001f + 2.0f * width01); // 0..~2x
        L = mid + side; R = mid - side;
    }
};
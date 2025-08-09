// SpectralGate_Platinum.h — Hardened, RT-safe rewrite (APVTS mapping preserved)
#pragma once
#include "EngineBase.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>

class SpectralGate_Platinum final : public EngineBase {
public:
    SpectralGate_Platinum();
    ~SpectralGate_Platinum() override = default;

    // EngineBase implementation
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Spectral Gate Platinum"; }

    // Must match APVTS parameter order
    enum class ParamID : int {
        Threshold = 0,     // dB threshold
        Ratio = 1,         // gate ratio
        Attack = 2,        // ms
        Release = 3,       // ms
        FreqLow = 4,       // Hz
        FreqHigh = 5,      // Hz
        Lookahead = 6,     // ms
        Mix = 7            // dry/wet
    };

private:
    // --------- Helpers ----------
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

    // Parameter smoothing
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

    // --------- DSP components ----------
    // Simple, stable FFT without hanging (bounded iteration counts)
    static constexpr int kFFTOrder = 10;       // 1024 points (reduced for stability)
    static constexpr int kFFTSize = 1 << kFFTOrder;
    static constexpr int kFFTBins = kFFTSize / 2 + 1;
    static constexpr int kOverlap = 4;         // 75% overlap
    static constexpr int kHopSize = kFFTSize / kOverlap;

    struct FFTProcessor {
        juce::dsp::FFT fft{kFFTOrder};
        std::array<float, kFFTSize * 2> fftData{};
        std::array<float, kFFTSize> window{};
        std::array<float, kFFTSize> overlapBuf{};
        int overlapPos{0};

        void prepareWindow();
        void processFrame(const float* input, float* output,
                         float threshold, float ratio,
                         int binLow, int binHigh);
        void reset() {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::fill(overlapBuf.begin(), overlapBuf.end(), 0.0f);
            overlapPos = 0;
        }
    };

    struct Channel {
        FFTProcessor fftProc;
        std::array<float, kFFTSize> inputBuf{};
        std::array<float, kFFTSize> outputBuf{};
        int writePos{0}, readPos{0};
        int hopCounter{0};

        // Simple lookahead delay
        std::vector<float> delayBuf;
        int delayWrite{0};
        int delaySamples{0};

        // Per-bin envelope followers (bounded)
        std::array<float, kFFTBins> binEnv{};

        void reset() {
            fftProc.reset();
            std::fill(inputBuf.begin(), inputBuf.end(), 0.0f);
            std::fill(outputBuf.begin(), outputBuf.end(), 0.0f);
            std::fill(binEnv.begin(), binEnv.end(), 0.0f);
            writePos = readPos = hopCounter = 0;
            if (!delayBuf.empty()) std::fill(delayBuf.begin(), delayBuf.end(), 0.0f);
            delayWrite = 0;
        }
    };

    // --------- State ----------
    double sr_{44100.0};
    int maxBlock_{512};

    // Smoothed parameters
    Smooth pThreshold, pRatio, pAttack, pRelease;
    Smooth pFreqLow, pFreqHigh, pLookahead, pMix;

    // DSP channels
    std::vector<Channel> channels_;

    // Bounded iteration guard
    int maxProcessingIterations_{0};

    // --------- Methods ----------
    void processChannel(Channel& ch, float* data, int numSamples);
    static int freqToBin(float hz, double sr) {
        const float binHz = float(sr) / float(kFFTSize);
        return clamp(int(hz / binHz), 0, kFFTBins - 1);
    }
};
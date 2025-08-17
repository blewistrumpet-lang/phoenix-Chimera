// GranularCloud.h — Hardened, RT-safe rewrite (APVTS mapping preserved)
#pragma once
#include "EngineBase.h"
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

class GranularCloud final : public EngineBase {
public:
    GranularCloud();
    ~GranularCloud() override = default;

    // EngineBase implementation
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Granular Cloud"; }

    // Must match APVTS parameter order
    enum class ParamID : int {
        GrainSize = 0,       // ms
        Density = 1,         // grains/sec
        PitchScatter = 2,    // octaves
        CloudPosition = 3    // stereo position
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

    // --------- Grain structure ----------
    struct Grain {
        bool active{false};
        double pos{0.0};        // Position in buffer (samples)
        double increment{1.0};  // Playback rate
        int length{0};          // Grain length in samples
        int elapsed{0};         // Samples played
        float amp{1.0f};        // Amplitude
        float pan{0.5f};        // Stereo position
    };

    // Simple thread-safe RNG
    struct SimpleRNG {
        uint32_t state{0x12345678};
        float uniform() noexcept {
            // Simple LCG
            state = state * 1103515245 + 12345;
            return (state >> 16) / 65536.0f;
        }
    };

    // --------- State ----------
    double sr_{44100.0};
    int maxBlock_{512};

    // Smoothed parameters
    Smooth pGrainSize, pDensity, pPitchScatter, pCloudPosition;

    // Circular buffer for input
    std::vector<float> circularBuffer_;
    int bufferSize_{0};
    int writePos_{0};

    // Grain pool (bounded for CPU safety)
    // kMaxGrains: Total grain objects in pool (64 = reasonable memory usage)
    // kMaxActiveGrains: Maximum concurrent processing limit (32 = prevents CPU spikes)
    // These limits prevent infinite loops and runaway grain allocation that could
    // cause audio dropouts or system instability
    static constexpr int kMaxGrains = 64;
    static constexpr int kMaxActiveGrains = 32;
    std::array<Grain, kMaxGrains> grains_;

    // Grain scheduling
    double grainTimer_{0.0};
    double nextGrainTime_{0.0};

    // Window table
    std::vector<float> windowTable_;
    int windowSize_{0};

    // RNG
    SimpleRNG rng_;
    
    // DEBUG: Grain statistics for monitoring and debugging
    struct GrainStats {
        int currentActiveGrains{0};
        int peakActiveGrains{0};
        int totalGrainsSpawned{0};
        int grainsRecycled{0};
        int emergencyBreaks{0};
        void reset() {
            currentActiveGrains = peakActiveGrains = 0;
            totalGrainsSpawned = grainsRecycled = emergencyBreaks = 0;
        }
    } grainStats_;

    // --------- Methods ----------
    void triggerGrain(float grainMs, float scatter, float position);
};
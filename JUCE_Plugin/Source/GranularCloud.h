#pragma once
#include "EngineBase.h"
#include "Denorm.hpp"
#include <vector>
#include <random>
#include <cmath>
#include <array>
#include <atomic>
#include <memory>
#include <functional>

// Simple aligned array template
template<typename T, size_t N, size_t Alignment = 32>
struct AlignedArray {
    alignas(Alignment) std::array<T, N> data;
    
    T& operator[](size_t i) { return data[i]; }
    const T& operator[](size_t i) const { return data[i]; }
    
    T* begin() { return data.begin(); }
    T* end() { return data.end(); }
    const T* begin() const { return data.begin(); }
    const T* end() const { return data.end(); }
    
    void fill(const T& value) { data.fill(value); }
    void clear() { data.fill(T{}); }
};

class GranularCloud : public EngineBase {
public:
    GranularCloud();
    ~GranularCloud() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Granular Cloud"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    
    // Quality monitoring API
    struct QualityReport {
        float cpuUsage = 0.0f;
        float thd = 0.0f;
        float peakLevel = 0.0f;
        float rmsLevel = 0.0f;
        int activeGrains = 0;
        int droppedGrains = 0;
    };
    
    QualityReport getQualityReport() const;
    
private:
    // Per-sample parameter smoothing with denormal handling
    struct SmoothParam {
        std::atomic<double> target{0.0};
        double current = 0.0;
        double coeff = 0.995;
        
        inline double tick() noexcept {
            current += coeff * (target.load(std::memory_order_relaxed) - current);
            return flushDenorm(current);
        }
        
        void setSmoothingRate(double rate) { 
            coeff = std::clamp(rate, 0.0, 0.9999); 
        }
        
        void setImmediate(double v) { 
            target.store(v, std::memory_order_relaxed);
            current = v; 
        }
        
        float getValue() const { 
            return static_cast<float>(current); 
        }
    };
    
    SmoothParam m_grainSize;      // ms (0.1-2000)
    SmoothParam m_density;        // grains/sec (0.1-500)  
    SmoothParam m_pitchScatter;   // ±4 octaves
    SmoothParam m_cloudPosition;  // stereo spread
    
    // Pre-computed parameter atomics (updated in updateParameters)
    std::atomic<float> m_grainSizeTarget{50.0f};
    std::atomic<float> m_densityTarget{20.0f};
    std::atomic<float> m_pitchScatterTarget{0.0f};
    std::atomic<float> m_cloudPositionTarget{0.5f};
    
    // DSP State
    double m_sampleRate = 44100.0;
    
    // High-quality anti-aliasing filter with aligned storage
    class AntiAliasingFilter {
        static constexpr int ORDER = 8;
        AlignedArray<double, ORDER + 1> a, b;
        AlignedArray<double, ORDER> x_history, y_history;
        
    public:
        void designButterworth(double cutoffFreq, double sampleRate);
        
        inline double process(double input) noexcept {
            // Direct Form II with automatic denormal flushing
            double w = input;
            
            // Unrolled for SIMD potential
            w -= a[1] * y_history[0] + a[2] * y_history[1] + 
                 a[3] * y_history[2] + a[4] * y_history[3] +
                 a[5] * y_history[4] + a[6] * y_history[5] + 
                 a[7] * y_history[6] + a[8] * y_history[7];
            
            double output = b[0] * w + b[1] * x_history[0] + 
                           b[2] * x_history[1] + b[3] * x_history[2] + 
                           b[4] * x_history[3] + b[5] * x_history[4] + 
                           b[6] * x_history[5] + b[7] * x_history[6] + 
                           b[8] * x_history[7];
            
            // Shift history (vectorizable)
            for (int i = ORDER - 1; i > 0; --i) {
                x_history[i] = x_history[i-1];
                y_history[i] = y_history[i-1];
            }
            
            x_history[0] = flushDenorm(w);
            y_history[0] = flushDenorm(output);
            
            return output;
        }
        
        void reset() {
            x_history.clear();
            y_history.clear();
        }
    };
    
    // Optimized Sinc interpolator with aligned tables
    class SincInterpolator {
        static constexpr int SINC_POINTS = 32;
        static constexpr int TABLE_SIZE = 8192;
        static constexpr size_t TABLE_ALIGNMENT = 64; // Cache line
        
        float* sincTable = nullptr;
        
    public:
        void initialize();
        ~SincInterpolator();
        
        inline double interpolate(const float* buffer, int bufferSize, 
                                 double position) const noexcept;
    };
    
    // Grain with integrated oversampling
    struct Grain {
        float* bufferPtr = nullptr;
        int bufferSize = 0;
        double readPosAccumulator = 0.0;
        int grainLength = 0;
        double pitchRatio = 1.0;
        float amplitude = 1.0f;
        float pan = 0.5f;
        std::atomic<bool> active{false};
        
        // Envelope state
        int envelopePos = 0;
        static constexpr int FADE_SAMPLES = 64;
        
        // Per-grain filters
        AntiAliasingFilter inputFilter;
        AntiAliasingFilter outputFilter;
        bool useInputFilter = false;
        bool useOutputFilter = false;
        
        // Per-sample processing
        inline float processSample(double sampleRate, const SincInterpolator& interp) noexcept;
        double calculateEnvelope() const noexcept;
        void reset() noexcept;
    };
    
    // Lock-free grain pool
    static constexpr int MAX_GRAINS = 128;
    AlignedArray<Grain, MAX_GRAINS, 64> m_grains; // Cache-line aligned
    std::atomic<uint32_t> m_grainAllocationIndex{0};
    
    // Aligned circular buffers
    static constexpr int BUFFER_SIZE = 88200 * 4; // 4 seconds
    std::array<float*, 2> m_circularBufferPtrs;
    struct AlignedDeleter {
        void operator()(float* ptr) const;
    };
    std::unique_ptr<float, AlignedDeleter> m_circularBufferMemory;
    std::atomic<int> m_writePos{0};
    
    // Grain scheduling
    double m_grainTimer = 0.0;
    double m_nextGrainTime = 0.0;
    
    // RNG
    std::mt19937_64 m_rng;
    std::uniform_real_distribution<double> m_uniformDist{0.0, 1.0};
    std::normal_distribution<double> m_normalDist{0.0, 1.0};
    
    // DSP components
    SincInterpolator m_interpolator;
    
    // Window table (aligned)
    static constexpr size_t MAX_WINDOW_SIZE = 88200 * 2;
    float* m_windowTable = nullptr;
    
    // DC blocking with per-sample denormal handling
    class DCBlocker {
        double x1 = 0.0;
        double y1 = 0.0;
        static constexpr double R = 0.995;
        
    public:
        inline float process(float input) noexcept {
            double in = static_cast<double>(input);
            double output = in - x1 + R * y1;
            x1 = in;
            y1 = flushDenorm(output);
            return static_cast<float>(output);
        }
        
        void reset() noexcept {
            x1 = 0.0;
            y1 = 0.0;
        }
    };
    
    AlignedArray<DCBlocker, 2> m_inputDCBlockers;
    AlignedArray<DCBlocker, 2> m_outputDCBlockers;
    
    // Advanced oversampling with per-grain support
    class Oversampler {
    public:
        static constexpr int FACTOR = 4;
        
        void initialize(double sampleRate);
        
        // Process a single sample through oversampling
        inline float processSample(float input, 
                                  std::function<float(float)> processor) noexcept;
        
    private:
        // Polyphase FIR implementation
        static constexpr int TAPS_PER_PHASE = 32;
        AlignedArray<float, TAPS_PER_PHASE * FACTOR> upCoeffs;
        AlignedArray<float, TAPS_PER_PHASE * FACTOR> downCoeffs;
        AlignedArray<float, TAPS_PER_PHASE * FACTOR> upDelayLine;
        AlignedArray<float, TAPS_PER_PHASE * FACTOR> downDelayLine;
        int delayIndex = 0;
    };
    
    Oversampler m_oversampler;
    bool m_useOversampling = true;
    
    // Quality metrics
    struct QualityMetrics {
        std::atomic<float> cpuUsage{0.0f};
        std::atomic<int> activeGrainCount{0};
        std::atomic<int> droppedGrains{0};
        std::atomic<float> peakLevel{0.0f};
        std::atomic<float> rmsLevel{0.0f};
        
        // THD measurement
        std::atomic<float> thd{0.0f};
        AlignedArray<double, 8192> fftBuffer;
        
        void updateCPU(double processingTime, double availableTime) noexcept;
        void updateLevels(const float* buffer, int numSamples) noexcept;
        void measureTHD(const float* input, const float* output, int numSamples);
    };
    
    mutable QualityMetrics m_metrics;
    
    // Helper methods
    void triggerGrain() noexcept;
    Grain* allocateGrain() noexcept;
    double calculatePitchRatio(double scatterAmount) noexcept;
    void calculateStereoPan(float& leftGain, float& rightGain, float pan) noexcept;
    float validateParameter(float value, float min, float max) noexcept;
    void generateWindowTable();
    
    // Optimized mixing
    inline void mixGrainToOutput(float grainSample, float leftGain, float rightGain,
                                float& leftOut, float& rightOut) noexcept {
        leftOut += flushDenorm(grainSample * leftGain * 0.25f);
        rightOut += flushDenorm(grainSample * rightGain * 0.25f);
    }
    
    // CPU features
    bool m_hasSSE2 = false;
    bool m_hasAVX = false;
    bool m_hasAVX2 = false;
    void detectCPUFeatures() noexcept;
};
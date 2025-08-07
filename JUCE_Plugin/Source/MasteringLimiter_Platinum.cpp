// MasteringLimiter_Platinum.cpp - Ultimate Studio-Grade Implementation
#include "MasteringLimiter_Platinum.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <deque>
#include <cstring>
#include <memory>
#include <array>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
    #define HAS_AVX2 (defined(__AVX2__) || defined(_M_AVX2))
#else
    #define HAS_SSE2 0
    #define HAS_AVX2 0
#endif

// Unified denormal threshold
constexpr double DENORM_THRESHOLD = 1e-25;
constexpr float DENORM_THRESHOLD_F = 1e-25f;

// Platform-specific optimizations with bias injection
#if HAS_AVX2
    inline float flushDenormalAVX(float x) noexcept {
        const __m128 tiny = _mm_set1_ps(1e-30f);
        __m128 v = _mm_set_ss(x);
        v = _mm_add_ss(v, tiny);  // Add bias
        v = _mm_sub_ss(v, tiny);  // Remove bias
        return _mm_cvtss_f32(v);
    }
    #define FLUSH_DENORM(x) flushDenormalAVX(x)
#else
    #define FLUSH_DENORM(x) (std::fabs(x) < DENORM_THRESHOLD_F ? 0.0f : x)
#endif

// Anonymous namespace for internal implementation
namespace {
    
// Global denormal protection with CPU mode forcing
struct DenormalGuard {
    DenormalGuard() {
#if HAS_SSE2
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
} static g_denormGuard;

// Constants
constexpr int OVERSAMPLE_FACTOR = 16;
constexpr int MAX_LOOKAHEAD_MS = 20;
constexpr int MAX_BLOCK_SIZE = 2048;
constexpr double TRUE_PEAK_THRESHOLD = 0.9999;

// Platform-specific aligned memory allocation
template<typename T>
T* alignedAlloc(size_t count, size_t alignment = 32) {
    void* ptr = nullptr;
    #ifdef _WIN32
    ptr = _aligned_malloc(count * sizeof(T), alignment);
    #else
    if (posix_memalign(&ptr, alignment, count * sizeof(T)) != 0) {
        ptr = nullptr;
    }
    #endif
    return static_cast<T*>(ptr);
}

template<typename T>
void alignedFree(T* ptr) {
    #ifdef _WIN32
    _aligned_free(ptr);
    #else
    free(ptr);
    #endif
}

// Fast approximations for real-time use
inline float fastTanh(float x) noexcept {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Professional parameter smoother with double precision
class ParameterSmoother {
    alignas(16) double m_current{0.0};
    alignas(16) double m_target{0.0};
    double m_coeff{0.995};
    
public:
    void setSampleRate(double sr, double smoothMs) noexcept {
        double fc = 1000.0 / (2.0 * M_PI * smoothMs);
        m_coeff = std::exp(-2.0 * M_PI * fc / sr);
    }
    
    void setTarget(double value) noexcept {
        m_target = value;
    }
    
    double process() noexcept {
        m_current = m_target + (m_current - m_target) * m_coeff;
        return m_current < DENORM_THRESHOLD ? 0.0 : m_current;
    }
    
    void reset(double value) noexcept {
        m_current = m_target = value;
    }
};

// ITU-R BS.1770-4 compliant true peak detector with SSE2/AVX2 optimization
class TruePeakDetectorAVX {
    static constexpr int SINC_TAPS = 48;
    static constexpr int PHASES = 8192;
    static constexpr int FLUSH_INTERVAL = 256;
    
    alignas(64) float m_sincTable[PHASES][SINC_TAPS];
    alignas(64) float m_history[SINC_TAPS + 8]; // Extra for AVX
    int m_writeIndex{0};
    int m_flushCounter{0};
    
public:
    TruePeakDetectorAVX() {
        // Pre-compute windowed sinc for all phases
        for (int p = 0; p < PHASES; ++p) {
            float phase = static_cast<float>(p) / PHASES;
            for (int i = 0; i < SINC_TAPS; ++i) {
                float x = i - SINC_TAPS/2 + phase;
                
                // Sinc with Blackman-Harris window
                float sinc = (x == 0) ? 1.0f : std::sin(M_PI * x) / (M_PI * x);
                float n = static_cast<float>(i) / (SINC_TAPS - 1);
                float window = 0.35875f - 0.48829f * std::cos(2*M_PI*n) + 
                              0.14128f * std::cos(4*M_PI*n) - 0.01168f * std::cos(6*M_PI*n);
                
                m_sincTable[p][i] = sinc * window;
            }
        }
        reset();
    }
    
    float detectTruePeak(float input) noexcept {
        // Update circular buffer
        m_history[m_writeIndex] = input;
        m_writeIndex = (m_writeIndex + 1) % SINC_TAPS;
        
        // Periodic denormal flush
        if (++m_flushCounter >= FLUSH_INTERVAL) {
            m_flushCounter = 0;
            for (int i = 0; i < SINC_TAPS; ++i) {
                m_history[i] = FLUSH_DENORM(m_history[i]);
            }
        }
        
        float truePeak = std::abs(input);
        
#if HAS_AVX2
        // AVX2 optimized convolution for 8 phases
        for (int phase = 1; phase < 8; ++phase) {
            int tableIndex = phase * (PHASES / 8);
            __m256 sum = _mm256_setzero_ps();
            
            // Process 8 taps at a time
            for (int i = 0; i < SINC_TAPS; i += 8) {
                int idx = (m_writeIndex - i - 1 + SINC_TAPS) % SINC_TAPS;
                __m256 hist = _mm256_loadu_ps(&m_history[idx]);
                __m256 coef = _mm256_load_ps(&m_sincTable[tableIndex][i]);
                sum = _mm256_fmadd_ps(hist, coef, sum);
            }
            
            // Horizontal sum
            __m128 sum_low = _mm256_castps256_ps128(sum);
            __m128 sum_high = _mm256_extractf128_ps(sum, 1);
            __m128 sum128 = _mm_add_ps(sum_low, sum_high);
            sum128 = _mm_hadd_ps(sum128, sum128);
            sum128 = _mm_hadd_ps(sum128, sum128);
            
            float interpolated = _mm_cvtss_f32(sum128);
            truePeak = std::max(truePeak, std::abs(interpolated));
        }
#else
        // Scalar fallback
        for (int phase = 1; phase < 8; ++phase) {
            int tableIndex = phase * (PHASES / 8);
            float interpolated = 0.0f;
            
            for (int i = 0; i < SINC_TAPS; ++i) {
                int idx = (m_writeIndex - i - 1 + SINC_TAPS) % SINC_TAPS;
                interpolated += m_history[idx] * m_sincTable[tableIndex][i];
            }
            
            truePeak = std::max(truePeak, std::abs(interpolated));
        }
#endif
        
        return truePeak;
    }
    
    void reset() noexcept {
        std::fill(std::begin(m_history), std::end(m_history), 0.0f);
        m_writeIndex = 0;
        m_flushCounter = 0;
    }
};

// Optimized predictive lookahead with sliding window maximum
class PredictiveLookaheadOptimized {
    alignas(64) float* m_buffer{nullptr};
    int m_size{0};
    int m_writePos{0};
    int m_readPos{0};
    int m_delaySamples{0};
    
    // Double precision for derivatives
    double m_slope{0.0};
    double m_acceleration{0.0};
    double m_jerk{0.0};
    
    // Sliding window maximum using deque
    std::deque<std::pair<float, int>> m_maxWindow;
    int m_windowCounter{0};
    
public:
    ~PredictiveLookaheadOptimized() {
        if (m_buffer) alignedFree(m_buffer);
    }
    
    void prepare(int maxSamples) {
        if (m_buffer) alignedFree(m_buffer);
        m_size = maxSamples + 16; // Extra for SIMD
        m_buffer = alignedAlloc<float>(m_size);
        std::fill(m_buffer, m_buffer + m_size, 0.0f);
        reset();
    }
    
    void setDelay(int samples) noexcept {
        m_delaySamples = std::min(samples, m_size - 16);
        m_readPos = (m_writePos - m_delaySamples + m_size) % m_size;
        m_maxWindow.clear();
        m_windowCounter = 0;
    }
    
    float process(float input, float& delayedOut) noexcept {
        // Write to buffer
        m_buffer[m_writePos] = input;
        delayedOut = m_buffer[m_readPos];
        
        float absInput = std::abs(input);
        
        // Update sliding window maximum
        while (!m_maxWindow.empty() && m_maxWindow.back().first <= absInput) {
            m_maxWindow.pop_back();
        }
        m_maxWindow.push_back({absInput, m_windowCounter++});
        
        // Remove old values outside window
        while (!m_maxWindow.empty() && 
               m_maxWindow.front().second < m_windowCounter - m_delaySamples) {
            m_maxWindow.pop_front();
        }
        
        float maxPeak = m_maxWindow.empty() ? absInput : m_maxWindow.front().first;
        
        // Calculate derivatives in double precision
        double samples[4];
        for (int i = 0; i < 4; ++i) {
            int idx = (m_readPos + m_delaySamples - 3 + i) % m_size;
            samples[i] = static_cast<double>(std::abs(m_buffer[idx]));
        }
        
        // 3rd order finite differences
        double d1 = samples[3] - samples[2];
        double d2 = samples[3] - 2*samples[2] + samples[1];
        double d3 = samples[3] - 3*samples[2] + 3*samples[1] - samples[0];
        
        // IIR smoothing of derivatives
        m_slope = m_slope * 0.9 + d1 * 0.1;
        m_acceleration = m_acceleration * 0.9 + d2 * 0.1;
        m_jerk = m_jerk * 0.9 + d3 * 0.1;
        
        // Predictive peak using Taylor expansion
        double t = m_delaySamples * 0.5;
        double prediction = maxPeak + m_slope * t + 
                           0.5 * m_acceleration * t * t +
                           0.166667 * m_jerk * t * t * t;
        
        // Update positions
        m_writePos = (m_writePos + 1) % m_size;
        m_readPos = (m_readPos + 1) % m_size;
        
        return static_cast<float>(std::max(static_cast<double>(maxPeak), prediction));
    }
    
    void reset() noexcept {
        if (m_buffer) std::fill(m_buffer, m_buffer + m_size, 0.0f);
        m_writePos = 0;
        m_readPos = 0;
        m_slope = m_acceleration = m_jerk = 0.0;
        m_maxWindow.clear();
        m_windowCounter = 0;
    }
};

// Adaptive envelope follower with crest factor analysis
class EnvelopeFollower {
    double m_envelope{0.0};
    double m_attackCoeff{0.0};
    double m_releaseCoeff{0.0};
    double m_adaptiveReleaseCoeff{0.0};
    double m_maxAdaptiveRelease{0.1}; // Safety cap
    
    // Program analysis
    alignas(64) float m_history[1024];
    int m_historyIndex{0};
    float m_rms{0.0f};
    float m_peak{0.0f};
    float m_crestFactor{1.0f};
    int m_flushCounter{0};
    
public:
    void setSampleRate(double sr) noexcept {
        setAttackTime(0.1, sr);
        setReleaseTime(50.0, sr);
    }
    
    void setAttackTime(double ms, double sr) noexcept {
        m_attackCoeff = 1.0 - std::exp(-1.0 / (ms * 0.001 * sr));
    }
    
    void setReleaseTime(double ms, double sr) noexcept {
        m_releaseCoeff = 1.0 - std::exp(-1.0 / (ms * 0.001 * sr));
        m_adaptiveReleaseCoeff = m_releaseCoeff;
    }
    
    float process(float input, bool adaptive) noexcept {
        float absInput = std::abs(input);
        
        // Periodic denormal flush
        if (++m_flushCounter >= 256) {
            m_flushCounter = 0;
            m_envelope = (m_envelope < DENORM_THRESHOLD) ? 0.0 : m_envelope;
        }
        
        // Update history for adaptive release
        if (adaptive) {
            m_history[m_historyIndex] = absInput * absInput;
            m_historyIndex = (m_historyIndex + 1) % 1024;
            
            // Calculate RMS with AVX2 if available
#if HAS_AVX2
            __m256 sum = _mm256_setzero_ps();
            for (int i = 0; i < 1024; i += 8) {
                __m256 values = _mm256_load_ps(&m_history[i]);
                sum = _mm256_add_ps(sum, values);
            }
            float sumArray[8];
            _mm256_store_ps(sumArray, sum);
            float totalSum = 0.0f;
            for (int i = 0; i < 8; ++i) totalSum += sumArray[i];
            m_rms = std::sqrt(totalSum / 1024.0f);
#else
            float sum = 0.0f;
            for (int i = 0; i < 1024; ++i) {
                sum += m_history[i];
            }
            m_rms = std::sqrt(sum / 1024.0f);
#endif
            
            // Update peak with slow decay
            m_peak = m_peak * 0.9999f + absInput * 0.0001f;
            
            // Calculate crest factor
            m_crestFactor = (m_rms > 0.001f) ? m_peak / m_rms : 1.0f;
            
            // Adaptive release based on crest factor
            if (m_crestFactor > 10.0f) {
                m_adaptiveReleaseCoeff = m_releaseCoeff * 10.0f;
            } else if (m_crestFactor > 5.0f) {
                m_adaptiveReleaseCoeff = m_releaseCoeff * 2.0f;
            } else {
                m_adaptiveReleaseCoeff = m_releaseCoeff * 0.5f;
            }
            
            // Cap per-sample to prevent runaway
            m_adaptiveReleaseCoeff = std::min(m_adaptiveReleaseCoeff, m_maxAdaptiveRelease);
        }
        
        // Envelope detection
        double coeff = (absInput > m_envelope) ? m_attackCoeff : 
                      (adaptive ? m_adaptiveReleaseCoeff : m_releaseCoeff);
        
        m_envelope = absInput + (m_envelope - absInput) * (1.0 - coeff);
        
        return static_cast<float>(m_envelope);
    }
    
    void reset() noexcept {
        m_envelope = 0.0;
        std::fill(std::begin(m_history), std::end(m_history), 0.0f);
        m_historyIndex = 0;
        m_rms = m_peak = 0.0f;
        m_crestFactor = 1.0f;
        m_flushCounter = 0;
    }
};

// Professional oversampler with AVX2-optimized FIR
class LinearPhaseOversamplerAVX {
    static constexpr int FIR_TAPS = 256;
    static constexpr int FIR_TAPS_ALIGNED = (FIR_TAPS + 7) & ~7; // Round up to 8
    
    alignas(64) float m_coeffs[FIR_TAPS_ALIGNED];
    alignas(64) float* m_bufferUp{nullptr};
    alignas(64) float* m_bufferDown{nullptr};
    alignas(64) float* m_workBuffer{nullptr};
    int m_maxSamples{0};
    
    // Precomputed decimation indices
    std::vector<int> m_decimationIndices;
    
public:
    ~LinearPhaseOversamplerAVX() {
        if (m_bufferUp) alignedFree(m_bufferUp);
        if (m_bufferDown) alignedFree(m_bufferDown);
        if (m_workBuffer) alignedFree(m_workBuffer);
    }
    
    void prepare(int maxBlockSize, double sampleRate) {
        m_maxSamples = maxBlockSize;
        
        // Free old buffers
        if (m_bufferUp) alignedFree(m_bufferUp);
        if (m_bufferDown) alignedFree(m_bufferDown);
        if (m_workBuffer) alignedFree(m_workBuffer);
        
        // Allocate aligned buffers
        size_t upSize = maxBlockSize * OVERSAMPLE_FACTOR + FIR_TAPS_ALIGNED;
        m_bufferUp = alignedAlloc<float>(upSize);
        m_bufferDown = alignedAlloc<float>(upSize);
        m_workBuffer = alignedAlloc<float>(upSize);
        
        // Precompute decimation indices
        m_decimationIndices.clear();
        m_decimationIndices.reserve(maxBlockSize);
        for (int i = 0; i < maxBlockSize; ++i) {
            m_decimationIndices.push_back(i * OVERSAMPLE_FACTOR);
        }
        
        // Design Kaiser-windowed sinc filter
        designKaiser(0.45, 140.0);
        reset();
    }
    
    void processUpsample(const float* input, float* output, int numSamples) noexcept {
        // Zero-stuff
        std::fill(m_workBuffer, m_workBuffer + numSamples * OVERSAMPLE_FACTOR, 0.0f);
        for (int i = 0; i < numSamples; ++i) {
            m_workBuffer[i * OVERSAMPLE_FACTOR] = input[i] * OVERSAMPLE_FACTOR;
        }
        
        // Apply anti-imaging filter with AVX2
        applyFIRAVX(m_workBuffer, output, numSamples * OVERSAMPLE_FACTOR);
    }
    
    void processDownsample(const float* input, float* output, int numSamples) noexcept {
        // Apply anti-aliasing filter
        applyFIRAVX(input, m_workBuffer, numSamples * OVERSAMPLE_FACTOR);
        
        // Decimate using precomputed indices
        for (int i = 0; i < numSamples; ++i) {
            output[i] = m_workBuffer[m_decimationIndices[i]];
        }
    }
    
    void reset() noexcept {
        if (m_bufferUp) {
            size_t upSize = m_maxSamples * OVERSAMPLE_FACTOR + FIR_TAPS_ALIGNED;
            std::fill(m_bufferUp, m_bufferUp + upSize, 0.0f);
            std::fill(m_bufferDown, m_bufferDown + upSize, 0.0f);
            std::fill(m_workBuffer, m_workBuffer + upSize, 0.0f);
        }
    }
    
private:
    void designKaiser(double cutoff, double attenuation) {
        double beta = 0.1102 * (attenuation - 8.7);
        double sum = 0.0;
        
        for (int i = 0; i < FIR_TAPS; ++i) {
            double n = i - (FIR_TAPS - 1) / 2.0;
            double sinc = (n == 0) ? 1.0 : std::sin(M_PI * cutoff * n) / (M_PI * n);
            
            double x = 2.0 * i / (FIR_TAPS - 1) - 1.0;
            double kaiser = besselI0(beta * std::sqrt(1.0 - x * x)) / besselI0(beta);
            
            m_coeffs[i] = static_cast<float>(sinc * kaiser);
            sum += m_coeffs[i];
        }
        
        // Normalize
        float norm = static_cast<float>(1.0 / sum);
        for (int i = 0; i < FIR_TAPS; ++i) {
            m_coeffs[i] *= norm;
        }
        
        // Zero pad for alignment
        for (int i = FIR_TAPS; i < FIR_TAPS_ALIGNED; ++i) {
            m_coeffs[i] = 0.0f;
        }
    }
    
    double besselI0(double x) {
        double sum = 1.0, term = 1.0, x2 = x * x / 4.0;
        for (int k = 1; k < 100; ++k) {
            term *= x2 / (k * k);
            sum += term;
            if (term < 1e-15) break;
        }
        return sum;
    }
    
    void applyFIRAVX(const float* input, float* output, int numSamples) noexcept {
#if HAS_AVX2
        // Process with 8-way unrolled AVX2
        for (int i = numSamples - 1; i >= FIR_TAPS; --i) {
            __m256 sum = _mm256_setzero_ps();
            
            // Process 8 taps at a time
            for (int j = 0; j < FIR_TAPS_ALIGNED; j += 8) {
                __m256 data = _mm256_loadu_ps(&input[i - j]);
                __m256 coef = _mm256_load_ps(&m_coeffs[j]);
                sum = _mm256_fmadd_ps(data, coef, sum);
            }
            
            // Horizontal sum
            __m128 sum_low = _mm256_castps256_ps128(sum);
            __m128 sum_high = _mm256_extractf128_ps(sum, 1);
            __m128 sum128 = _mm_add_ps(sum_low, sum_high);
            sum128 = _mm_hadd_ps(sum128, sum128);
            sum128 = _mm_hadd_ps(sum128, sum128);
            
            output[i] = FLUSH_DENORM(_mm_cvtss_f32(sum128));
        }
        
        // Handle initial samples
        for (int i = 0; i < FIR_TAPS && i < numSamples; ++i) {
            float sum = 0.0f;
            for (int j = 0; j <= i && j < FIR_TAPS; ++j) {
                sum += input[i - j] * m_coeffs[j];
            }
            output[i] = FLUSH_DENORM(sum);
        }
#else
        // Scalar fallback
        for (int i = 0; i < numSamples; ++i) {
            float sum = 0.0f;
            int startJ = std::max(0, i - FIR_TAPS + 1);
            for (int j = startJ; j <= i; ++j) {
                sum += input[j] * m_coeffs[i - j];
            }
            output[i] = FLUSH_DENORM(sum);
        }
#endif
    }
};

// Soft clipper with multiple algorithms
class SoftClipper {
    float m_knee{0.1f};
    
public:
    void setKnee(float knee) noexcept {
        m_knee = std::clamp(knee, 0.0f, 1.0f);
    }
    
    float process(float input, float threshold) noexcept {
        float absInput = std::abs(input);
        
        if (absInput < threshold - m_knee) {
            return input;
        }
        
        float sign = (input < 0) ? -1.0f : 1.0f;
        
        // Cubic soft clipping
        if (absInput > threshold) {
            float over = (absInput - threshold) / (1.0f - threshold);
            over = std::clamp(over, 0.0f, 1.0f);
            float soft = threshold + (1.0f - threshold) * (over - over * over * over / 3.0f);
            return sign * soft;
        }
        
        // Knee transition
        float x = (absInput - threshold + m_knee) / m_knee;
        x = std::clamp(x, 0.0f, 1.0f);
        float hardClipped = std::min(absInput, threshold);
        float softClipped = threshold * fastTanh(absInput / threshold);
        
        return sign * (hardClipped * (1.0f - x) + softClipped * x);
    }
};

} // anonymous namespace

// Main implementation structure
struct MasteringLimiter_Platinum::Impl {
    // DSP state
    double sampleRate{44100.0};
    int maxBlockSize{2048};
    
    // Parameters with per-param smooth times
    struct {
        ParameterSmoother threshold;
        ParameterSmoother ceiling;
        ParameterSmoother release;
        ParameterSmoother lookahead;
        ParameterSmoother knee;
        ParameterSmoother makeup;
        ParameterSmoother saturation;
        ParameterSmoother stereoLink;
        ParameterSmoother truePeak;
        ParameterSmoother mix;
    } params;
    
    // User-configurable smooth times (ms)
    struct {
        double threshold = 10.0;
        double ceiling = 10.0;
        double release = 20.0;
        double lookahead = 50.0;
        double knee = 30.0;
        double makeup = 20.0;
        double saturation = 30.0;
        double stereoLink = 50.0;
        double truePeak = 100.0;
        double mix = 20.0;
    } smoothTimes;
    
    // DSP components (per channel)
    std::array<TruePeakDetectorAVX, 2> truePeakDetectors;
    std::array<PredictiveLookaheadOptimized, 2> lookaheads;
    std::array<EnvelopeFollower, 2> envelopes;
    std::array<LinearPhaseOversamplerAVX, 2> oversamplers;
    std::array<SoftClipper, 2> clippers;
    
    // Work buffers (aligned, allocated once)
    float* oversampledBuffer[2]{nullptr, nullptr};
    float* processBuffer[2]{nullptr, nullptr};
    
    // Current state
    float currentGain[2]{1.0f, 1.0f};
    
    // Linear meters (converted to dB only in accessors)
    float inputPeakLinear{0.0f};
    float outputPeakLinear{0.0f};
    float grLinear{1.0f};
    float truePeakLinear{0.0f};
    
    // Atomic parameters
    std::array<std::atomic<float>, 10> atomicParams;
    
    Impl() {
        // Set default values
        atomicParams[kThreshold] = 0.4f;    // -12 dB default
        atomicParams[kCeiling] = 0.9f;      // -0.3 dB default
        atomicParams[kRelease] = 0.3f;      // 50ms default
        atomicParams[kLookahead] = 0.2f;    // 2ms default
        atomicParams[kKnee] = 0.5f;         // 0.5 default
        atomicParams[kMakeup] = 0.5f;       // 0 dB default
        atomicParams[kSaturation] = 0.0f;   // No saturation default
        atomicParams[kStereoLink] = 1.0f;   // Fully linked default
        atomicParams[kTruePeak] = 1.0f;     // True peak enabled
        atomicParams[kMix] = 1.0f;          // 100% wet
        
        params.threshold.reset(-12.0);
        params.ceiling.reset(-0.3);
        params.release.reset(50.0);
        params.lookahead.reset(2.0);
        params.knee.reset(0.5);
        params.makeup.reset(0.0);
        params.saturation.reset(0.0);
        params.stereoLink.reset(1.0);
        params.truePeak.reset(1.0);
        params.mix.reset(1.0);
    }
    
    ~Impl() {
        for (int ch = 0; ch < 2; ++ch) {
            if (oversampledBuffer[ch]) alignedFree(oversampledBuffer[ch]);
            if (processBuffer[ch]) alignedFree(processBuffer[ch]);
        }
    }
    
    void prepare(double sr, int blockSize) {
        sampleRate = sr;
        maxBlockSize = blockSize;
        
        // Configure smoothers with user-adjustable times
        params.threshold.setSampleRate(sr, smoothTimes.threshold);
        params.ceiling.setSampleRate(sr, smoothTimes.ceiling);
        params.release.setSampleRate(sr, smoothTimes.release);
        params.lookahead.setSampleRate(sr, smoothTimes.lookahead);
        params.knee.setSampleRate(sr, smoothTimes.knee);
        params.makeup.setSampleRate(sr, smoothTimes.makeup);
        params.saturation.setSampleRate(sr, smoothTimes.saturation);
        params.stereoLink.setSampleRate(sr, smoothTimes.stereoLink);
        params.truePeak.setSampleRate(sr, smoothTimes.truePeak);
        params.mix.setSampleRate(sr, smoothTimes.mix);
        
        // Prepare DSP components
        int lookaheadSamples = static_cast<int>(MAX_LOOKAHEAD_MS * 0.001 * sr);
        
        for (int ch = 0; ch < 2; ++ch) {
            lookaheads[ch].prepare(lookaheadSamples);
            envelopes[ch].setSampleRate(sr);
            oversamplers[ch].prepare(blockSize, sr);
            clippers[ch].setKnee(0.1f);
            
            // Allocate aligned work buffers once
            if (oversampledBuffer[ch]) alignedFree(oversampledBuffer[ch]);
            if (processBuffer[ch]) alignedFree(processBuffer[ch]);
            
            size_t oversampledSize = blockSize * OVERSAMPLE_FACTOR;
            oversampledBuffer[ch] = alignedAlloc<float>(oversampledSize);
            processBuffer[ch] = alignedAlloc<float>(blockSize);
            
            // Initialize to zero
            std::fill(oversampledBuffer[ch], oversampledBuffer[ch] + oversampledSize, 0.0f);
            std::fill(processBuffer[ch], processBuffer[ch] + blockSize, 0.0f);
        }
    }
    
    void reset() {
        for (int ch = 0; ch < 2; ++ch) {
            truePeakDetectors[ch].reset();
            lookaheads[ch].reset();
            envelopes[ch].reset();
            oversamplers[ch].reset();
            currentGain[ch] = 1.0f;
            
            if (oversampledBuffer[ch]) {
                std::fill(oversampledBuffer[ch], 
                         oversampledBuffer[ch] + maxBlockSize * OVERSAMPLE_FACTOR, 0.0f);
            }
            if (processBuffer[ch]) {
                std::fill(processBuffer[ch], processBuffer[ch] + maxBlockSize, 0.0f);
            }
        }
        
        inputPeakLinear = outputPeakLinear = truePeakLinear = 0.0f;
        grLinear = 1.0f;
    }
};

// Public interface implementation
MasteringLimiter_Platinum::MasteringLimiter_Platinum() : pimpl(std::make_unique<Impl>()) {}
MasteringLimiter_Platinum::~MasteringLimiter_Platinum() = default;

void MasteringLimiter_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
    reset();
}

void MasteringLimiter_Platinum::reset() {
    pimpl->reset();
    m_grMeter = 0.0f;
    m_inputMeter = 0.0f;
    m_outputMeter = 0.0f;
    m_truePeakMeter = 0.0f;
}

void MasteringLimiter_Platinum::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Update smoother targets from atomics
    pimpl->params.threshold.setTarget(-60.0 + pimpl->atomicParams[kThreshold].load(std::memory_order_relaxed) * 60.0);
    pimpl->params.ceiling.setTarget(-3.0 + pimpl->atomicParams[kCeiling].load(std::memory_order_relaxed) * 3.0);
    pimpl->params.release.setTarget(10.0 * std::pow(250.0, pimpl->atomicParams[kRelease].load(std::memory_order_relaxed)));
    pimpl->params.lookahead.setTarget(pimpl->atomicParams[kLookahead].load(std::memory_order_relaxed) * 10.0);
    pimpl->params.knee.setTarget(pimpl->atomicParams[kKnee].load(std::memory_order_relaxed));
    pimpl->params.makeup.setTarget(-12.0 + pimpl->atomicParams[kMakeup].load(std::memory_order_relaxed) * 24.0);
    pimpl->params.saturation.setTarget(pimpl->atomicParams[kSaturation].load(std::memory_order_relaxed));
    pimpl->params.stereoLink.setTarget(pimpl->atomicParams[kStereoLink].load(std::memory_order_relaxed));
    pimpl->params.truePeak.setTarget(pimpl->atomicParams[kTruePeak].load(std::memory_order_relaxed) > 0.5 ? 1.0 : 0.0);
    pimpl->params.mix.setTarget(pimpl->atomicParams[kMix].load(std::memory_order_relaxed));
    
    // Get parameter values (double precision)
    double thresholdDb = pimpl->params.threshold.process();
    double ceilingDb = pimpl->params.ceiling.process();
    double releaseMs = pimpl->params.release.process();
    double lookaheadMs = pimpl->params.lookahead.process();
    double kneeValue = pimpl->params.knee.process();
    double makeupDb = pimpl->params.makeup.process();
    double saturationAmt = pimpl->params.saturation.process();
    double stereoLinkAmt = pimpl->params.stereoLink.process();
    double truePeakMode = pimpl->params.truePeak.process();
    double mixAmt = pimpl->params.mix.process();
    
    // Convert to linear (float for processing)
    float thresholdLin = static_cast<float>(std::pow(10.0, thresholdDb / 20.0));
    float ceilingLin = static_cast<float>(std::pow(10.0, ceilingDb / 20.0));
    float makeupLin = static_cast<float>(std::pow(10.0, makeupDb / 20.0));
    
    // Update lookahead delay
    int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * pimpl->sampleRate);
    for (int ch = 0; ch < 2; ++ch) {
        pimpl->lookaheads[ch].setDelay(lookaheadSamples);
        pimpl->clippers[ch].setKnee(static_cast<float>(kneeValue));
        pimpl->envelopes[ch].setReleaseTime(releaseMs, pimpl->sampleRate);
    }
    
    // Track input peak for metering
    pimpl->inputPeakLinear = 0.0f;
    for (int ch = 0; ch < std::min(numChannels, 2); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            pimpl->inputPeakLinear = std::max(pimpl->inputPeakLinear, std::abs(data[i]));
        }
    }
    
    // Process each channel
    int channelsToProcess = std::min(numChannels, 2);
    bool useTruePeak = truePeakMode > 0.5;
    
    for (int ch = 0; ch < channelsToProcess; ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        float* workBuffer = pimpl->processBuffer[ch];
        
        // Store dry signal
        std::copy(channelData, channelData + numSamples, workBuffer);
        
        if (useTruePeak) {
            // Oversample for true peak compliance
            float* osBuffer = pimpl->oversampledBuffer[ch];
            pimpl->oversamplers[ch].processUpsample(channelData, osBuffer, numSamples);
            
            // Process at higher rate
            int osSamples = numSamples * OVERSAMPLE_FACTOR;
            for (int i = 0; i < osSamples; ++i) {
                float input = osBuffer[i];
                float delayed = 0.0f;
                
                // True peak detection with lookahead
                float peak = pimpl->truePeakDetectors[ch].detectTruePeak(input);
                float lookaheadPeak = pimpl->lookaheads[ch].process(peak, delayed);
                
                // Envelope following
                float envelope = pimpl->envelopes[ch].process(lookaheadPeak, true);
                
                // Gain calculation
                float gainReduction = 1.0f;
                if (envelope > thresholdLin) {
                    float excessDb = 20.0f * std::log10(envelope / thresholdLin);
                    float reducedDb = -excessDb * 0.9f; // 10:1 ratio
                    gainReduction = std::pow(10.0f, reducedDb / 20.0f);
                }
                
                // Apply ceiling
                float outputLevel = envelope * gainReduction;
                if (outputLevel > ceilingLin) {
                    gainReduction *= ceilingLin / outputLevel;
                }
                
                // Smooth gain changes
                float targetGain = gainReduction;
                pimpl->currentGain[ch] += (targetGain - pimpl->currentGain[ch]) * 0.01f;
                
                // Apply processing
                float processed = input * pimpl->currentGain[ch];
                
                // Soft clipping for saturation
                if (saturationAmt > 0.01) {
                    processed = pimpl->clippers[ch].process(
                        processed * (1.0f + static_cast<float>(saturationAmt) * 2.0f), 
                        ceilingLin);
                }
                
                // Apply makeup gain
                processed *= makeupLin;
                
                // Final safety limiting
                processed = pimpl->clippers[ch].process(processed, ceilingLin);
                
                osBuffer[i] = processed;
                
                // Update true peak meter
                pimpl->truePeakLinear = std::max(pimpl->truePeakLinear, std::abs(processed));
            }
            
            // Downsample back
            pimpl->oversamplers[ch].processDownsample(osBuffer, channelData, numSamples);
            
        } else {
            // Non-oversampled processing
            for (int i = 0; i < numSamples; ++i) {
                float input = channelData[i];
                float delayed = 0.0f;
                
                // Standard peak detection with lookahead
                float peak = std::abs(input);
                float lookaheadPeak = pimpl->lookaheads[ch].process(peak, delayed);
                
                // Envelope following
                float envelope = pimpl->envelopes[ch].process(lookaheadPeak, true);
                
                // Gain calculation
                float gainReduction = 1.0f;
                if (envelope > thresholdLin) {
                    float excessDb = 20.0f * std::log10(envelope / thresholdLin);
                    float reducedDb = -excessDb * 0.9f;
                    gainReduction = std::pow(10.0f, reducedDb / 20.0f);
                }
                
                // Apply ceiling
                float outputLevel = envelope * gainReduction;
                if (outputLevel > ceilingLin) {
                    gainReduction *= ceilingLin / outputLevel;
                }
                
                // Smooth gain changes
                float targetGain = gainReduction;
                pimpl->currentGain[ch] += (targetGain - pimpl->currentGain[ch]) * 0.1f;
                
                // Apply processing
                float processed = delayed * pimpl->currentGain[ch];
                
                // Soft clipping for saturation
                if (saturationAmt > 0.01) {
                    processed = pimpl->clippers[ch].process(
                        processed * (1.0f + static_cast<float>(saturationAmt) * 2.0f), 
                        ceilingLin);
                }
                
                // Apply makeup gain
                processed *= makeupLin;
                
                // Final safety limiting
                processed = pimpl->clippers[ch].process(processed, ceilingLin);
                
                channelData[i] = processed;
            }
        }
        
        // Mix dry/wet
        for (int i = 0; i < numSamples; ++i) {
            channelData[i] = channelData[i] * static_cast<float>(mixAmt) + 
                           workBuffer[i] * (1.0f - static_cast<float>(mixAmt));
        }
    }
    
    // Stereo linking
    if (channelsToProcess == 2 && stereoLinkAmt > 0.01) {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        
        for (int i = 0; i < numSamples; ++i) {
            float avgGain = (pimpl->currentGain[0] + pimpl->currentGain[1]) * 0.5f;
            float linkedGain = avgGain * static_cast<float>(stereoLinkAmt) + 
                              pimpl->currentGain[0] * (1.0f - static_cast<float>(stereoLinkAmt));
            
            left[i] = left[i] / pimpl->currentGain[0] * linkedGain;
            right[i] = right[i] / pimpl->currentGain[1] * linkedGain;
        }
    }
    
    // Update meters (keep linear internally)
    pimpl->grLinear = (pimpl->currentGain[0] + pimpl->currentGain[1]) * 0.5f;
    
    // Calculate output peak
    pimpl->outputPeakLinear = 0.0f;
    for (int ch = 0; ch < channelsToProcess; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            pimpl->outputPeakLinear = std::max(pimpl->outputPeakLinear, std::abs(data[i]));
        }
    }
    
    // Update atomic meters for GUI
    m_inputMeter.store(pimpl->inputPeakLinear, std::memory_order_relaxed);
    m_outputMeter.store(pimpl->outputPeakLinear, std::memory_order_relaxed);
    m_grMeter.store(pimpl->grLinear, std::memory_order_relaxed);
    m_truePeakMeter.store(pimpl->truePeakLinear, std::memory_order_relaxed);
}

void MasteringLimiter_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index >= 0 && index < 10) {
            pimpl->atomicParams[index].store(value, std::memory_order_relaxed);
        }
    }
}

juce::String MasteringLimiter_Platinum::getParameterName(int index) const {
    switch (index) {
        case kThreshold:  return "Threshold";
        case kCeiling:    return "Ceiling";
        case kRelease:    return "Release";
        case kLookahead:  return "Lookahead";
        case kKnee:       return "Knee";
        case kMakeup:     return "Makeup";
        case kSaturation: return "Saturation";
        case kStereoLink: return "Stereo Link";
        case kTruePeak:   return "True Peak";
        case kMix:        return "Mix";
        default:          return "";
    }
}

// Convert linear meters to dB only in accessors
float MasteringLimiter_Platinum::getGainReduction() const noexcept {
    float linear = m_grMeter.load(std::memory_order_relaxed);
    return 20.0f * std::log10(linear + 1e-10f);
}

float MasteringLimiter_Platinum::getInputLevel() const noexcept {
    float linear = m_inputMeter.load(std::memory_order_relaxed);
    return 20.0f * std::log10(linear + 1e-10f);
}

float MasteringLimiter_Platinum::getOutputLevel() const noexcept {
    float linear = m_outputMeter.load(std::memory_order_relaxed);
    return 20.0f * std::log10(linear + 1e-10f);
}

float MasteringLimiter_Platinum::getTruePeakLevel() const noexcept {
    float linear = m_truePeakMeter.load(std::memory_order_relaxed);
    return 20.0f * std::log10(linear + 1e-10f);
}
#include "NoiseGate_Platinum.h"
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <vector>
#include <array>
#include <atomic>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

// ============================================================================
// Platform Configuration
// ============================================================================

#ifdef _MSC_VER
    #define ALWAYS_INLINE __forceinline
    #define RESTRICT __restrict
    #define ALIGNED(x) __declspec(align(x))
#else
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
    #define RESTRICT __restrict__
    #define ALIGNED(x) __attribute__((aligned(x)))
#endif


// ============================================================================
// Constants and Utilities
// ============================================================================

namespace {
    constexpr double PI = 3.14159265358979323846;
    constexpr float MINUS_INF_DB = -144.0f;
    constexpr double DENORMAL_THRESHOLD = 1e-30;
    constexpr int SIMD_ALIGNMENT = 32;
    constexpr int SIMD_WIDTH = 4; // SSE processes 4 floats
    
    // Fast math utilities
    ALWAYS_INLINE double flushDenormal(double x) noexcept {
        return (std::abs(x) < DENORMAL_THRESHOLD) ? 0.0 : x;
    }
    
#if HAS_SSE2
    ALWAYS_INLINE __m128 flushDenormalsSIMD(__m128 x) noexcept {
        const __m128 threshold = _mm_set1_ps(1e-30f);
        const __m128 abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
        const __m128 abs_x = _mm_and_ps(x, abs_mask);
        const __m128 mask = _mm_cmplt_ps(abs_x, threshold);
        return _mm_andnot_ps(mask, x);
    }
#endif
    
    ALWAYS_INLINE float dbToLinear(float db) noexcept {
        return (db > MINUS_INF_DB) ? std::pow(10.0f, db * 0.05f) : 0.0f;
    }
    
    ALWAYS_INLINE float linearToDb(float linear) noexcept {
        return (linear > 0.00001f) ? 20.0f * std::log10(linear) : MINUS_INF_DB;
    }
    
    // Branchless smoothstep
    ALWAYS_INLINE float smoothstep(float edge0, float edge1, float x) noexcept {
        float t = std::clamp((x - edge0) / (edge1 - edge0 + 1e-10f), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
    
#if HAS_SSE2
    ALWAYS_INLINE __m128 smoothstepSIMD(__m128 edge0, __m128 edge1, __m128 x) noexcept {
        const __m128 zero = _mm_setzero_ps();
        const __m128 one = _mm_set1_ps(1.0f);
        const __m128 two = _mm_set1_ps(2.0f);
        const __m128 three = _mm_set1_ps(3.0f);
        const __m128 epsilon = _mm_set1_ps(1e-10f);
        
        __m128 denom = _mm_add_ps(_mm_sub_ps(edge1, edge0), epsilon);
        __m128 t = _mm_div_ps(_mm_sub_ps(x, edge0), denom);
        t = _mm_max_ps(zero, _mm_min_ps(one, t));
        
        __m128 t2 = _mm_mul_ps(t, t);
        return _mm_mul_ps(t2, _mm_sub_ps(three, _mm_mul_ps(two, t)));
    }
#endif
    
    // Aligned memory allocator
    template<typename T, size_t Alignment = SIMD_ALIGNMENT>
    class AlignedAllocator {
    public:
        using value_type = T;
        
        T* allocate(size_t n) {
            void* ptr = nullptr;
            #ifdef _MSC_VER
                ptr = _aligned_malloc(n * sizeof(T), Alignment);
                if (!ptr) throw std::bad_alloc();
            #else
                if (posix_memalign(&ptr, Alignment, n * sizeof(T)) != 0) {
                    throw std::bad_alloc();
                }
            #endif
            return static_cast<T*>(ptr);
        }
        
        void deallocate(T* p, size_t) {
            #ifdef _MSC_VER
                _aligned_free(p);
            #else
                free(p);
            #endif
        }
        
        template<typename U>
        struct rebind {
            using other = AlignedAllocator<U, Alignment>;
        };
    };
}

// ============================================================================
// Thread-Safe Parameter Smoothing
// ============================================================================

class SmoothedParameter {
    std::atomic<float> target{0.5f};
    double current = 0.5;
    double smoothing = 0.995;
    
public:
    void setSmoothingTime(float ms, double sampleRate) noexcept {
        const double samples = ms * 0.001 * sampleRate;
        smoothing = std::exp(-1.0 / samples);
    }
    
    void setTarget(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
    }
    
    void reset(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
        current = value;
    }
    
    ALWAYS_INLINE double tick() noexcept {
        const double t = target.load(std::memory_order_relaxed);
        current += (1.0 - smoothing) * (t - current);
        return current;
    }
    
    double getValue() const noexcept { return current; }
};

// ============================================================================
// DSP Components (with fallbacks for non-SSE2)
// ============================================================================

// Use DCBlocker from DspEngineUtilities

#if HAS_SSE2
class alignas(16) DCBlockerSIMD {
    __m128 x1 = _mm_setzero_ps();
    __m128 y1 = _mm_setzero_ps();
    __m128 r_vec = _mm_set1_ps(0.995f);
    
public:
    void reset() noexcept { 
        x1 = y1 = _mm_setzero_ps(); 
    }
    
    ALWAYS_INLINE __m128 process4(const float* input) noexcept {
        __m128 in = _mm_loadu_ps(input);
        __m128 output = _mm_add_ps(_mm_sub_ps(in, x1), _mm_mul_ps(r_vec, y1));
        x1 = in;
        y1 = flushDenormalsSIMD(output);
        return y1;
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        __m128 in_vec = _mm_set_ss(input);
        __m128 output = _mm_add_ss(_mm_sub_ss(in_vec, x1), _mm_mul_ss(r_vec, y1));
        x1 = _mm_shuffle_ps(x1, in_vec, _MM_SHUFFLE(0,0,0,1));
        y1 = _mm_shuffle_ps(y1, flushDenormalsSIMD(output), _MM_SHUFFLE(0,0,0,1));
        return _mm_cvtss_f32(output);
    }
};
#endif

// ============================================================================
// Optimized Envelope Follower
// ============================================================================

class EnvelopeFollower {
    float envelope = 0.0f;
    float oneMinusAttack = 0.01f;
    float oneMinusRelease = 0.001f;
    
    // Optimized RMS with running sum
    static constexpr int RMS_SIZE = 128;
    static constexpr int RMS_MASK = RMS_SIZE - 1;
    ALIGNED(32) float rmsBuffer[RMS_SIZE] = {};
    float rmsRunningSum = 0.0f;
    int rmsWritePos = 0;
    
    // Peak detection
    float peakHold = 0.0f;
    float peakDecay = 0.9999f;
    
public:
    void reset() noexcept {
        envelope = peakHold = rmsRunningSum = 0.0f;
        std::memset(rmsBuffer, 0, sizeof(rmsBuffer));
        rmsWritePos = 0;
    }
    
    void setAttackRelease(float attackMs, float releaseMs, double sampleRate) noexcept {
        float ac = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
        float rc = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
        oneMinusAttack = 1.0f - ac;
        oneMinusRelease = 1.0f - rc;
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        float rectified = std::abs(input);
        
        // RMS calculation
        float squared = rectified * rectified;
        float oldValue = rmsBuffer[rmsWritePos];
        rmsBuffer[rmsWritePos] = squared;
        rmsWritePos = (rmsWritePos + 1) & RMS_MASK;
        
        rmsRunningSum = rmsRunningSum - oldValue + squared;
        if (std::abs(rmsRunningSum) < 1e-30f) rmsRunningSum = 0.0f;
        
        float rms = std::sqrt(rmsRunningSum / RMS_SIZE);
        
        // Peak detection
        if (rectified > peakHold) {
            peakHold = rectified;
        } else {
            peakHold *= peakDecay;
            if (peakHold < 1e-30f) peakHold = 0.0f;
        }
        
        // Combine RMS and Peak
        float target = 0.7f * rms + 0.3f * peakHold;
        
        // Adaptive envelope
        float rate = (target > envelope) ? oneMinusAttack : oneMinusRelease;
        envelope += (target - envelope) * rate;
        
        if (std::abs(envelope) < 1e-30f) envelope = 0.0f;
        
        return envelope;
    }
};

#if HAS_SSE2
class alignas(32) EnvelopeFollowerSIMD {
    __m128 envelope = _mm_setzero_ps();
    __m128 oneMinusAttack = _mm_set1_ps(0.01f);
    __m128 oneMinusRelease = _mm_set1_ps(0.001f);
    
    // Optimized RMS with running sum
    static constexpr int RMS_SIZE = 128;
    static constexpr int RMS_MASK = RMS_SIZE - 1;
    ALIGNED(32) float rmsBuffer[RMS_SIZE] = {};
    __m128 rmsRunningSum = _mm_setzero_ps();
    int rmsWritePos = 0;
    
    // Peak detection
    __m128 peakHold = _mm_setzero_ps();
    __m128 peakDecayVec = _mm_set1_ps(0.9999f);
    
public:
    void reset() noexcept {
        envelope = peakHold = rmsRunningSum = _mm_setzero_ps();
        std::memset(rmsBuffer, 0, sizeof(rmsBuffer));
        rmsWritePos = 0;
    }
    
    void setAttackRelease(float attackMs, float releaseMs, double sampleRate) noexcept {
        float ac = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
        float rc = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
        oneMinusAttack = _mm_set1_ps(1.0f - ac);
        oneMinusRelease = _mm_set1_ps(1.0f - rc);
    }
    
    ALWAYS_INLINE __m128 process4(const float* input) noexcept {
        __m128 in = _mm_loadu_ps(input);
        
        // Rectify
        const __m128 abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
        __m128 rectified = _mm_and_ps(in, abs_mask);
        
        // Optimized RMS with running sum
        __m128 squared = _mm_mul_ps(rectified, rectified);
        
        // Load old values that will be replaced
        __m128 oldValues = _mm_load_ps(&rmsBuffer[rmsWritePos & RMS_MASK]);
        
        // Store new values
        _mm_store_ps(&rmsBuffer[rmsWritePos & RMS_MASK], squared);
        rmsWritePos = (rmsWritePos + SIMD_WIDTH) & RMS_MASK;
        
        // Update running sum: add new, subtract old
        rmsRunningSum = _mm_sub_ps(_mm_add_ps(rmsRunningSum, squared), oldValues);
        
        // Flush denormals from running sum
        rmsRunningSum = flushDenormalsSIMD(rmsRunningSum);
        
        // Calculate RMS from running sum
        __m128 avgSum = _mm_mul_ps(rmsRunningSum, _mm_set1_ps(1.0f / RMS_SIZE));
        __m128 rms = _mm_sqrt_ps(avgSum);
        
        // Peak detection with decay
        __m128 peakMask = _mm_cmpgt_ps(rectified, peakHold);
        peakHold = _mm_blendv_ps(_mm_mul_ps(peakHold, peakDecayVec), rectified, peakMask);
        peakHold = flushDenormalsSIMD(peakHold);
        
        // Combine RMS and Peak
        const __m128 rmsWeight = _mm_set1_ps(0.7f);
        const __m128 peakWeight = _mm_set1_ps(0.3f);
        __m128 target = _mm_add_ps(_mm_mul_ps(rms, rmsWeight), 
                                   _mm_mul_ps(peakHold, peakWeight));
        
        // Adaptive envelope
        __m128 attackMask = _mm_cmpgt_ps(target, envelope);
        __m128 rate = _mm_blendv_ps(oneMinusRelease, oneMinusAttack, attackMask);
        __m128 delta = _mm_mul_ps(_mm_sub_ps(target, envelope), rate);
        envelope = _mm_add_ps(envelope, delta);
        
        // Final denormal flush
        envelope = flushDenormalsSIMD(envelope);
        
        return envelope;
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        float rectified = std::abs(input);
        
        // RMS calculation
        float squared = rectified * rectified;
        float oldValue = rmsBuffer[rmsWritePos & RMS_MASK];
        rmsBuffer[rmsWritePos & RMS_MASK] = squared;
        rmsWritePos = (rmsWritePos + 1) & RMS_MASK;
        
        // Update running sum (convert to scalar operations)
        float runningSum = _mm_cvtss_f32(rmsRunningSum);
        runningSum = runningSum - oldValue + squared;
        if (std::abs(runningSum) < 1e-30f) runningSum = 0.0f;
        rmsRunningSum = _mm_set_ss(runningSum);
        
        float rms = std::sqrt(runningSum / RMS_SIZE);
        
        // Peak detection with decay
        float currentPeak = _mm_cvtss_f32(peakHold);
        if (rectified > currentPeak) {
            currentPeak = rectified;
        } else {
            currentPeak *= _mm_cvtss_f32(peakDecayVec);
            if (currentPeak < 1e-30f) currentPeak = 0.0f;
        }
        peakHold = _mm_set_ss(currentPeak);
        
        // Combine RMS and Peak
        float target = 0.7f * rms + 0.3f * currentPeak;
        
        // Adaptive envelope
        float currentEnvelope = _mm_cvtss_f32(envelope);
        float rate = (target > currentEnvelope) ? _mm_cvtss_f32(oneMinusAttack) : _mm_cvtss_f32(oneMinusRelease);
        currentEnvelope += (target - currentEnvelope) * rate;
        
        // Denormal protection
        if (std::abs(currentEnvelope) < 1e-30f) currentEnvelope = 0.0f;
        envelope = _mm_set_ss(currentEnvelope);
        
        return currentEnvelope;
    }
};
#endif

// ============================================================================
// Branchless Sidechain Filter
// ============================================================================

class SidechainFilter {
    float s1 = 0.0f;
    float s2 = 0.0f;
    float g = 0.0f;
    float k = 1.414f;
    float denomInv = 1.0f;
    
public:
    void reset() noexcept {
        s1 = s2 = 0.0f;
    }
    
    void setCutoff(float freqHz, double sampleRate) noexcept {
        freqHz = std::clamp(freqHz, 20.0f, static_cast<float>(sampleRate * 0.49));
        
        const float wd = 2.0f * PI * freqHz;
        const float T = 1.0f / sampleRate;
        const float wa = (2.0f / T) * std::tan(wd * T / 2.0f);
        const float g_val = std::clamp(wa * T / 2.0f, 0.0f, 0.9999f);
        
        g = g_val;
        float denom = 1.0f + 1.414f * g_val + g_val * g_val;
        denomInv = 1.0f / denom;
    }
    
    ALWAYS_INLINE float processHighpass(float input) noexcept {
        // SVF highpass
        float hp = (input - k * s1 - s2) * denomInv;
        float bp = g * hp + s1;
        float lp = g * bp + s2;
        
        // Update states with denormal flush
        s1 = (std::abs(g * hp + bp) < 1e-30f) ? 0.0f : (g * hp + bp);
        s2 = (std::abs(g * bp + lp) < 1e-30f) ? 0.0f : (g * bp + lp);
        
        return hp;
    }
};

#if HAS_SSE2
class alignas(16) SidechainFilterSIMD {
    __m128 s1 = _mm_setzero_ps();
    __m128 s2 = _mm_setzero_ps();
    __m128 g = _mm_setzero_ps();
    __m128 k = _mm_set1_ps(1.414f);
    __m128 denomInv = _mm_set1_ps(1.0f);
    
public:
    void reset() noexcept {
        s1 = s2 = _mm_setzero_ps();
    }
    
    void setCutoff(float freqHz, double sampleRate) noexcept {
        freqHz = std::clamp(freqHz, 20.0f, static_cast<float>(sampleRate * 0.49));
        
        const float wd = 2.0f * PI * freqHz;
        const float T = 1.0f / sampleRate;
        const float wa = (2.0f / T) * std::tan(wd * T / 2.0f);
        const float g_val = std::clamp(wa * T / 2.0f, 0.0f, 0.9999f);
        
        g = _mm_set1_ps(g_val);
        float denom = 1.0f + 1.414f * g_val + g_val * g_val;
        denomInv = _mm_set1_ps(1.0f / denom);
    }
    
    ALWAYS_INLINE __m128 processHighpass4(const float* input) noexcept {
        __m128 in = _mm_loadu_ps(input);
        
        // SVF highpass
        __m128 hp = _mm_mul_ps(_mm_sub_ps(_mm_sub_ps(in, _mm_mul_ps(k, s1)), s2), denomInv);
        __m128 bp = _mm_add_ps(_mm_mul_ps(g, hp), s1);
        __m128 lp = _mm_add_ps(_mm_mul_ps(g, bp), s2);
        
        // Update states with denormal flush
        s1 = flushDenormalsSIMD(_mm_add_ps(_mm_mul_ps(g, hp), bp));
        s2 = flushDenormalsSIMD(_mm_add_ps(_mm_mul_ps(g, bp), lp));
        
        return hp;
    }
    
    ALWAYS_INLINE float processHighpass(float input) noexcept {
        __m128 in_vec = _mm_set_ss(input);
        
        // SVF highpass
        __m128 hp = _mm_mul_ss(_mm_sub_ss(_mm_sub_ss(in_vec, _mm_mul_ss(k, s1)), s2), denomInv);
        __m128 bp = _mm_add_ss(_mm_mul_ss(g, hp), s1);
        __m128 lp = _mm_add_ss(_mm_mul_ss(g, bp), s2);
        
        // Update states with denormal flush
        s1 = _mm_shuffle_ps(s1, flushDenormalsSIMD(_mm_add_ss(_mm_mul_ss(g, hp), bp)), _MM_SHUFFLE(0,0,0,1));
        s2 = _mm_shuffle_ps(s2, flushDenormalsSIMD(_mm_add_ss(_mm_mul_ss(g, bp), lp)), _MM_SHUFFLE(0,0,0,1));
        
        return _mm_cvtss_f32(hp);
    }
};
#endif

// ============================================================================
// Lookahead Buffer
// ============================================================================

class LookaheadBuffer {
    std::vector<float, AlignedAllocator<float, SIMD_ALIGNMENT>> buffer;
    std::atomic<int> writePos{0};
    int size = 0;
    int mask = 0;
    
public:
    void prepare(int maxSamples) {
        // Round up to power of 2
        size = 1;
        while (size < maxSamples + SIMD_WIDTH) size <<= 1;
        mask = size - 1;
        
        buffer.resize(size + SIMD_WIDTH * 2);
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }
    
    ALWAYS_INLINE void write(float sample) noexcept {
        const int pos = writePos.load(std::memory_order_relaxed);
        buffer[pos & mask] = sample;
        writePos.store((pos + 1) & mask, std::memory_order_relaxed);
    }
    
    ALWAYS_INLINE float read(int delaySamples) const noexcept {
        if (delaySamples == 0) return 0.0f;
        const int pos = writePos.load(std::memory_order_relaxed);
        const int readPos = (pos - delaySamples) & mask;
        return buffer[readPos];
    }
    
    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }
};

#if HAS_SSE2
class alignas(32) LookaheadBufferSIMD {
    std::vector<float, AlignedAllocator<float, SIMD_ALIGNMENT>> buffer;
    std::atomic<int> writePos{0};
    int size = 0;
    int mask = 0;
    
public:
    void prepare(int maxSamples) {
        // Round up to power of 2
        size = 1;
        while (size < maxSamples + SIMD_WIDTH) size <<= 1;
        mask = size - 1;
        
        buffer.resize(size + SIMD_WIDTH * 2);
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }
    
    ALWAYS_INLINE void write4(const float* samples) noexcept {
        const int pos = writePos.load(std::memory_order_relaxed);
        _mm_store_ps(&buffer[pos & mask], _mm_loadu_ps(samples));
        writePos.store((pos + SIMD_WIDTH) & mask, std::memory_order_relaxed);
    }
    
    ALWAYS_INLINE void write(float sample) noexcept {
        const int pos = writePos.load(std::memory_order_relaxed);
        buffer[pos & mask] = sample;
        writePos.store((pos + 1) & mask, std::memory_order_relaxed);
    }
    
    ALWAYS_INLINE __m128 read4(int delaySamples) const noexcept {
        if (delaySamples == 0) return _mm_setzero_ps();
        const int pos = writePos.load(std::memory_order_relaxed);
        const int readPos = (pos - delaySamples) & mask;
        return _mm_load_ps(&buffer[readPos]);
    }
    
    ALWAYS_INLINE float read(int delaySamples) const noexcept {
        if (delaySamples == 0) return 0.0f;
        const int pos = writePos.load(std::memory_order_relaxed);
        const int readPos = (pos - delaySamples) & mask;
        return buffer[readPos];
    }
    
    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }
};
#endif

// ============================================================================
// Main Implementation Structure
// ============================================================================

struct NoiseGate_Platinum::Impl {
    // Per-channel state (cache-aligned)
    struct alignas(64) ChannelState {
        // DSP components (SSE2 or fallback)
#if HAS_SSE2
        DCBlockerSIMD dcBlockerIn;
        DCBlockerSIMD dcBlockerOut;
        EnvelopeFollowerSIMD envelope;
        SidechainFilterSIMD sidechain;
        LookaheadBufferSIMD lookahead;
        
        // Continuous gain state
        __m128 gainVec = _mm_setzero_ps();
        __m128 targetVec = _mm_setzero_ps();
        
        // Per-block precomputed constants
        __m128 attackRate = _mm_set1_ps(0.01f);
        __m128 releaseRate = _mm_set1_ps(0.001f);
        __m128 openThreshold = _mm_set1_ps(0.1f);
        __m128 closeThreshold = _mm_set1_ps(0.05f);
        
        // Scalar accessors for compatibility
        float gain = 0.0f;
        float target = 0.0f;
#else
        DCBlocker dcBlockerIn;
        DCBlocker dcBlockerOut;
        EnvelopeFollower envelope;
        SidechainFilter sidechain;
        LookaheadBuffer lookahead;
        
        float gain = 0.0f;
        float target = 0.0f;
        float attackRate = 0.01f;
        float releaseRate = 0.001f;
        float openThreshold = 0.1f;
        float closeThreshold = 0.05f;
#endif
        
        // Hold logic
        int holdCounter = 0;
        int holdSamples = 0;
        
        void reset() noexcept {
            dcBlockerIn.reset();
            dcBlockerOut.reset();
            envelope.reset();
            sidechain.reset();
            lookahead.reset();
#if HAS_SSE2
            gainVec = targetVec = _mm_setzero_ps();
            gain = target = 0.0f;
#else
            gain = target = 0.0f;
#endif
            holdCounter = 0;
        }
        
        void updateRates(float attackMs, float releaseMs, double sampleRate) noexcept {
            float ar = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
            float rr = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
#if HAS_SSE2
            attackRate = _mm_set1_ps(ar);
            releaseRate = _mm_set1_ps(rr);
#else
            attackRate = ar;
            releaseRate = rr;
#endif
        }
        
        void setThresholds(float threshold, float hysteresis) noexcept {
#if HAS_SSE2
            openThreshold = _mm_set1_ps(threshold);
            closeThreshold = _mm_set1_ps(threshold * (1.0f - hysteresis));
#else
            openThreshold = threshold;
            closeThreshold = threshold * (1.0f - hysteresis);
#endif
        }
        
        // Helper methods to sync scalar/SIMD state
        void syncGainToScalar() noexcept {
#if HAS_SSE2
            gain = _mm_cvtss_f32(gainVec);
#endif
        }
        
        void syncScalarToGain() noexcept {
#if HAS_SSE2
            gainVec = _mm_set1_ps(gain);
#endif
        }
    };
    
    // State
    std::array<ChannelState, 2> channels;
    double sampleRate = 44100.0;
    bool stereoLink = true;
    
    // Parameters
    SmoothedParameter threshold;
    SmoothedParameter range;
    SmoothedParameter attack;
    SmoothedParameter hold;
    SmoothedParameter release;
    SmoothedParameter hysteresis;
    SmoothedParameter sidechainFreq;
    SmoothedParameter lookaheadTime;
    
    // Per-block constants
#if HAS_SSE2
    __m128 rangeMin = _mm_setzero_ps();
    __m128 rangeScale = _mm_set1_ps(1.0f);
#else
    float rangeMin = 0.0f;
    float rangeScale = 1.0f;
#endif
    
    // Performance monitoring
    mutable std::atomic<float> cpuLoad{0.0f};
    std::chrono::high_resolution_clock::time_point lastTime;
    
    // Main processing functions
#if HAS_SSE2
    void processSIMD(float* left, float* right, int numSamples,
                     float threshold, float range, float hysteresis,
                     int holdSamples, int lookaheadSamples, float scMix);
#endif
    void processScalar(float* left, float* right, int numSamples,
                       float threshold, float range, float hysteresis,
                       int holdSamples, int lookaheadSamples, float scMix);
};

// ============================================================================
// Processing Implementation
// ============================================================================

#if HAS_SSE2
void NoiseGate_Platinum::Impl::processSIMD(float* left, float* right, int numSamples,
                                           float thresholdLin, float rangeLin, float hysteresisLin,
                                           int holdSamples, int lookaheadSamples, float scMix) {
    // Precompute per-block constants
    rangeMin = _mm_set1_ps(rangeLin);
    rangeScale = _mm_set1_ps(1.0f - rangeLin);
    
    // Branchless sidechain mix vectors
    __m128 scMixVec = _mm_set1_ps(scMix);
    __m128 scDryVec = _mm_set1_ps(1.0f - scMix);
    
    // Update both channels' thresholds (stereo link consistency)
    channels[0].setThresholds(thresholdLin, hysteresisLin);
    channels[0].holdSamples = holdSamples;
    
    // Mirror to channel 1 for perfect stereo link
    channels[1].openThreshold = channels[0].openThreshold;
    channels[1].closeThreshold = channels[0].closeThreshold;
    channels[1].holdSamples = channels[0].holdSamples;
    
    // Flush any denormals from previous block
    channels[0].gainVec = flushDenormalsSIMD(channels[0].gainVec);
    channels[1].gainVec = stereoLink ? channels[0].gainVec : flushDenormalsSIMD(channels[1].gainVec);
    
    // Process in SIMD chunks
    const int simdSamples = (numSamples / SIMD_WIDTH) * SIMD_WIDTH;
    
    for (int i = 0; i < simdSamples; i += SIMD_WIDTH) {
        // Load samples
        __m128 leftIn = _mm_loadu_ps(&left[i]);
        __m128 rightIn = _mm_loadu_ps(&right[i]);
        
        // DC blocking
        leftIn = channels[0].dcBlockerIn.process4(&left[i]);
        rightIn = channels[1].dcBlockerIn.process4(&right[i]);
        
        // Detection (stereo link uses maximum)
        __m128 detection;
        if (stereoLink) {
            const __m128 abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
            __m128 leftAbs = _mm_and_ps(leftIn, abs_mask);
            __m128 rightAbs = _mm_and_ps(rightIn, abs_mask);
            detection = _mm_max_ps(leftAbs, rightAbs);
        } else {
            const __m128 abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
            detection = _mm_and_ps(leftIn, abs_mask);
        }
        
        // Store detection for sidechain
        ALIGNED(16) float detectionArray[SIMD_WIDTH];
        _mm_store_ps(detectionArray, detection);
        
        // Branchless sidechain filter
        __m128 scFiltered = channels[0].sidechain.processHighpass4(detectionArray);
        detection = _mm_add_ps(_mm_mul_ps(scFiltered, scMixVec), 
                              _mm_mul_ps(detection, scDryVec));
        
        // Envelope detection
        _mm_store_ps(detectionArray, detection);
        __m128 env = channels[0].envelope.process4(detectionArray);
        
        // Calculate target gain using smoothstep
        __m128 targetGain = smoothstepSIMD(channels[0].closeThreshold, 
                                          channels[0].openThreshold, env);
        
        // Enhanced hold logic
        if (channels[0].holdCounter > 0) {
            channels[0].holdCounter--;
            targetGain = _mm_max_ps(targetGain, _mm_set1_ps(0.9f));
        } else {
            // Check if we should start holding
            float currentGain = _mm_cvtss_f32(channels[0].gainVec);
            float envMin = _mm_cvtss_f32(env);
            float closeThresh = _mm_cvtss_f32(channels[0].closeThreshold);
            
            if (currentGain > 0.5f && envMin < closeThresh) {
                channels[0].holdCounter = channels[0].holdSamples;
            }
        }
        
        // Reset hold counter if gate is fully closed
        if (_mm_cvtss_f32(channels[0].gainVec) < 0.01f) {
            channels[0].holdCounter = 0;
        }
        
        // Update gain with rate selection
        __m128 gainDiff = _mm_sub_ps(targetGain, channels[0].gainVec);
        __m128 rateMask = _mm_cmpgt_ps(gainDiff, _mm_setzero_ps());
        __m128 rate = _mm_blendv_ps(channels[0].releaseRate, channels[0].attackRate, rateMask);
        channels[0].gainVec = _mm_add_ps(channels[0].gainVec, _mm_mul_ps(gainDiff, rate));
        
        // Flush denormals from gain
        channels[0].gainVec = flushDenormalsSIMD(channels[0].gainVec);
        
        // Clamp gain to [0, 1]
        channels[0].gainVec = _mm_max_ps(_mm_setzero_ps(), 
                                        _mm_min_ps(_mm_set1_ps(1.0f), channels[0].gainVec));
        
        // Sync to scalar for compatibility
        channels[0].syncGainToScalar();
        
        // Lookahead buffer operations
        channels[0].lookahead.write4(&left[i]);
        channels[1].lookahead.write4(&right[i]);
        
        __m128 leftDelayed = (lookaheadSamples > 0) 
            ? channels[0].lookahead.read4(lookaheadSamples) : leftIn;
        __m128 rightDelayed = (lookaheadSamples > 0)
            ? channels[1].lookahead.read4(lookaheadSamples) : rightIn;
        
        // Apply gain
        __m128 finalGain = _mm_add_ps(rangeMin, _mm_mul_ps(rangeScale, channels[0].gainVec));
        finalGain = flushDenormalsSIMD(finalGain);
        
        __m128 leftOut = _mm_mul_ps(leftDelayed, finalGain);
        __m128 rightOut = _mm_mul_ps(rightDelayed, finalGain);
        
        // DC blocking output
        ALIGNED(16) float leftArray[SIMD_WIDTH];
        ALIGNED(16) float rightArray[SIMD_WIDTH];
        _mm_store_ps(leftArray, leftOut);
        _mm_store_ps(rightArray, rightOut);
        
        leftOut = channels[0].dcBlockerOut.process4(leftArray);
        rightOut = channels[1].dcBlockerOut.process4(rightArray);
        
        // Store results
        _mm_storeu_ps(&left[i], leftOut);
        _mm_storeu_ps(&right[i], rightOut);
        
        // Mirror gain state for stereo link
        if (stereoLink) {
            channels[1].gainVec = channels[0].gainVec;
            channels[1].gain = channels[0].gain;
            channels[1].holdCounter = channels[0].holdCounter;
        }
    }
    
    // Process remaining samples (non-SIMD)
    for (int i = simdSamples; i < numSamples; ++i) {
        float l = left[i];
        float r = right[i];
        
        // DC block
        l = channels[0].dcBlockerIn.process(l);
        r = channels[1].dcBlockerIn.process(r);
        
        // Use last SIMD gain
        float gain = _mm_cvtss_f32(channels[0].gainVec);
        float finalGain = rangeLin + (1.0f - rangeLin) * gain;
        
        left[i] = channels[0].dcBlockerOut.process(l * finalGain);
        right[i] = channels[1].dcBlockerOut.process(r * finalGain);
    }
}
#endif

void NoiseGate_Platinum::Impl::processScalar(float* left, float* right, int numSamples,
                                             float thresholdLin, float rangeLin, float hysteresisLin,
                                             int holdSamples, int lookaheadSamples, float scMix) {
    // Precompute per-block constants
    #if HAS_SSE2
    // For scalar processing in SIMD build, use local variables
    float scalarRangeMin = rangeLin;
    float scalarRangeScale = 1.0f - rangeLin;
    #else
    rangeMin = rangeLin;
    rangeScale = 1.0f - rangeLin;
    #endif
    
    // Update thresholds
    channels[0].setThresholds(thresholdLin, hysteresisLin);
    channels[0].holdSamples = holdSamples;
    
    // Copy settings to channel 1
    channels[1].setThresholds(thresholdLin, hysteresisLin);
    channels[1].holdSamples = holdSamples;
    channels[1].attackRate = channels[0].attackRate;
    channels[1].releaseRate = channels[0].releaseRate;
    
    for (int i = 0; i < numSamples; ++i) {
        // DC blocking
        float l = channels[0].dcBlockerIn.process(left[i]);
        float r = channels[1].dcBlockerIn.process(right[i]);
        
        // Detection
        float detection = stereoLink ? std::max(std::abs(l), std::abs(r)) : std::abs(l);
        
        // Sidechain filter
        float scFiltered = channels[0].sidechain.processHighpass(detection);
        detection = scFiltered * scMix + detection * (1.0f - scMix);
        
        // Envelope
        float env = channels[0].envelope.process(detection);
        
        // Calculate target gain
        #if HAS_SSE2
        float closeThresh = _mm_cvtss_f32(channels[0].closeThreshold);
        float openThresh = _mm_cvtss_f32(channels[0].openThreshold);
        float targetGain = smoothstep(closeThresh, openThresh, env);
        #else
        float targetGain = smoothstep(channels[0].closeThreshold, channels[0].openThreshold, env);
        #endif
        
        // Hold logic
        if (channels[0].holdCounter > 0) {
            channels[0].holdCounter--;
            targetGain = std::max(targetGain, 0.9f);
        } else if (channels[0].gain > 0.5f && env < 
        #if HAS_SSE2
        _mm_cvtss_f32(channels[0].closeThreshold)
        #else
        channels[0].closeThreshold
        #endif
        ) {
            channels[0].holdCounter = channels[0].holdSamples;
        }
        
        if (channels[0].gain < 0.01f) {
            channels[0].holdCounter = 0;
        }
        
        // Update gain
        #if HAS_SSE2
        float attackRate = _mm_cvtss_f32(channels[0].attackRate);
        float releaseRate = _mm_cvtss_f32(channels[0].releaseRate);
        float rate = (targetGain > channels[0].gain) ? attackRate : releaseRate;
        #else
        float rate = (targetGain > channels[0].gain) ? channels[0].attackRate : channels[0].releaseRate;
        #endif
        channels[0].gain += (targetGain - channels[0].gain) * rate;
        channels[0].gain = std::clamp(channels[0].gain, 0.0f, 1.0f);
        
        // Denormal protection
        if (std::abs(channels[0].gain) < 1e-30f) channels[0].gain = 0.0f;
        
        // Lookahead
        channels[0].lookahead.write(l);
        channels[1].lookahead.write(r);
        
        float leftDelayed = (lookaheadSamples > 0) ? channels[0].lookahead.read(lookaheadSamples) : l;
        float rightDelayed = (lookaheadSamples > 0) ? channels[1].lookahead.read(lookaheadSamples) : r;
        
        // Apply gain
        #if HAS_SSE2
        float finalGain = scalarRangeMin + scalarRangeScale * channels[0].gain;
        #else
        float finalGain = rangeMin + rangeScale * channels[0].gain;
        #endif
        
        // Output with DC blocking
        left[i] = channels[0].dcBlockerOut.process(leftDelayed * finalGain);
        right[i] = channels[1].dcBlockerOut.process(rightDelayed * finalGain);
        
        // Mirror for stereo link
        if (stereoLink) {
            channels[1].gain = channels[0].gain;
            channels[1].holdCounter = channels[0].holdCounter;
        }
    }
}

// ============================================================================
// Public Interface
// ============================================================================

NoiseGate_Platinum::NoiseGate_Platinum() : pimpl(std::make_unique<Impl>()) {
    // Initialize with sensible defaults
    pimpl->threshold.reset(0.1f);      // -48dB
    pimpl->range.reset(0.8f);          // -8dB
    pimpl->attack.reset(0.1f);         // 10ms
    pimpl->hold.reset(0.3f);           // 150ms
    pimpl->release.reset(0.5f);        // 500ms
    pimpl->hysteresis.reset(0.3f);     // 3dB
    pimpl->sidechainFreq.reset(0.1f);  // 68Hz
    pimpl->lookaheadTime.reset(0.0f);  // Zero latency
}

NoiseGate_Platinum::~NoiseGate_Platinum() = default;

void NoiseGate_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = sampleRate;
    
    // Configure smoothing
    const float smoothingMs = 20.0f;
    pimpl->threshold.setSmoothingTime(smoothingMs, sampleRate);
    pimpl->range.setSmoothingTime(smoothingMs * 2, sampleRate);
    pimpl->attack.setSmoothingTime(smoothingMs * 0.5f, sampleRate);
    pimpl->hold.setSmoothingTime(smoothingMs, sampleRate);
    pimpl->release.setSmoothingTime(smoothingMs * 2, sampleRate);
    pimpl->hysteresis.setSmoothingTime(smoothingMs, sampleRate);
    pimpl->sidechainFreq.setSmoothingTime(smoothingMs, sampleRate);
    pimpl->lookaheadTime.setSmoothingTime(smoothingMs * 0.5f, sampleRate);
    
    // Prepare channels
    const int maxLookahead = static_cast<int>(0.01 * sampleRate); // 10ms max
    
    for (auto& ch : pimpl->channels) {
        ch.reset();
        ch.envelope.setAttackRelease(10.0f, 50.0f, sampleRate);
        ch.sidechain.setCutoff(100.0f, sampleRate);
        ch.lookahead.prepare(maxLookahead);
    }
    
    pimpl->lastTime = std::chrono::high_resolution_clock::now();
}

void NoiseGate_Platinum::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
}

void NoiseGate_Platinum::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    const auto startTime = std::chrono::high_resolution_clock::now();
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Update parameters once per block
    const float thresholdNorm = static_cast<float>(pimpl->threshold.tick());
    const float rangeNorm = static_cast<float>(pimpl->range.tick());
    const float attackNorm = static_cast<float>(pimpl->attack.tick());
    const float holdNorm = static_cast<float>(pimpl->hold.tick());
    const float releaseNorm = static_cast<float>(pimpl->release.tick());
    const float hysteresisNorm = static_cast<float>(pimpl->hysteresis.tick());
    const float sidechainNorm = static_cast<float>(pimpl->sidechainFreq.tick());
    const float lookaheadNorm = static_cast<float>(pimpl->lookaheadTime.tick());
    
    // Convert to actual values
    const float thresholdDb = -60.0f + thresholdNorm * 60.0f;
    const float threshold = dbToLinear(thresholdDb);
    const float rangeDb = -40.0f + rangeNorm * 40.0f;
    const float range = dbToLinear(rangeDb);
    const float attackMs = 0.1f + attackNorm * 99.9f;
    const float holdMs = holdNorm * 500.0f;
    const float releaseMs = 1.0f + releaseNorm * 999.0f;
    const float hysteresisRatio = hysteresisNorm * 0.5f; // 0-50% of threshold
    const float sidechainHz = 20.0f + sidechainNorm * 1980.0f;
    const int lookaheadSamples = static_cast<int>(lookaheadNorm * 0.01f * pimpl->sampleRate);
    const int holdSamples = static_cast<int>(holdMs * 0.001f * pimpl->sampleRate);
    
    // Update DSP settings
    for (auto& ch : pimpl->channels) {
        ch.envelope.setAttackRelease(attackMs, releaseMs, pimpl->sampleRate);
        ch.sidechain.setCutoff(sidechainHz, pimpl->sampleRate);
        ch.updateRates(attackMs, releaseMs, pimpl->sampleRate);
    }
    
    // Process
    if (numChannels >= 2) {
#if HAS_SSE2
        pimpl->processSIMD(buffer.getWritePointer(0),
                          buffer.getWritePointer(1),
                          numSamples,
                          threshold, range, hysteresisRatio,
                          holdSamples, lookaheadSamples,
                          sidechainNorm);
#else
        pimpl->processScalar(buffer.getWritePointer(0),
                            buffer.getWritePointer(1),
                            numSamples,
                            threshold, range, hysteresisRatio,
                            holdSamples, lookaheadSamples,
                            sidechainNorm);
#endif
    } else if (numChannels == 1) {
        // Mono: duplicate to stereo
        float* mono = buffer.getWritePointer(0);
#if HAS_SSE2
        pimpl->processSIMD(mono, mono, numSamples,
                          threshold, range, hysteresisRatio,
                          holdSamples, lookaheadSamples,
                          sidechainNorm);
#else
        pimpl->processScalar(mono, mono, numSamples,
                            threshold, range, hysteresisRatio,
                            holdSamples, lookaheadSamples,
                            sidechainNorm);
#endif
    }
    
    // Update CPU load
    const auto endTime = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration<double>(endTime - startTime).count();
    const double theoretical = numSamples / pimpl->sampleRate;
    const float load = static_cast<float>(elapsed / theoretical) * 100.0f;
    pimpl->cpuLoad.store(load, std::memory_order_relaxed);
    
    scrubBuffer(buffer);
}

void NoiseGate_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (index) {
            case kThreshold:  pimpl->threshold.setTarget(value); break;
            case kRange:      pimpl->range.setTarget(value); break;
            case kAttack:     pimpl->attack.setTarget(value); break;
            case kHold:       pimpl->hold.setTarget(value); break;
            case kRelease:    pimpl->release.setTarget(value); break;
            case kHysteresis: pimpl->hysteresis.setTarget(value); break;
            case kSidechain:  pimpl->sidechainFreq.setTarget(value); break;
            case kLookahead:  pimpl->lookaheadTime.setTarget(value); break;
        }
    }
}

juce::String NoiseGate_Platinum::getParameterName(int index) const {
    switch (index) {
        case kThreshold:  return "Threshold";
        case kRange:      return "Range";
        case kAttack:     return "Attack";
        case kHold:       return "Hold";
        case kRelease:    return "Release";
        case kHysteresis: return "Hysteresis";
        case kSidechain:  return "SC Filter";
        case kLookahead:  return "Lookahead";
        default:          return "";
    }
}

float NoiseGate_Platinum::getCurrentGainReduction(int channel) const noexcept {
    if (channel < 0 || channel >= 2) return 0.0f;
#if HAS_SSE2
    return 1.0f - _mm_cvtss_f32(pimpl->channels[channel].gainVec);
#else
    return 1.0f - pimpl->channels[channel].gain;
#endif
}

bool NoiseGate_Platinum::isGateOpen(int channel) const noexcept {
    if (channel < 0 || channel >= 2) return false;
#if HAS_SSE2
    return _mm_cvtss_f32(pimpl->channels[channel].gainVec) > 0.5f;
#else
    return pimpl->channels[channel].gain > 0.5f;
#endif
}

float NoiseGate_Platinum::getCPULoad() const noexcept {
    return pimpl->cpuLoad.load(std::memory_order_relaxed);
}
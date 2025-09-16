#pragma once
#include <cmath>
#include <algorithm>

// Only include immintrin.h on x86/x64 architectures
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
#endif

/**
 * Universal Denormal Protection for ChimeraPhoenix DSP
 * 
 * Denormal numbers can cause massive CPU spikes in audio processing.
 * This header provides multiple strategies for preventing denormals.
 */

namespace DenormalProtection {

// Threshold below which we consider a number denormal
static constexpr float DENORMAL_THRESHOLD = 1e-8f;
static constexpr double DENORMAL_THRESHOLD_DOUBLE = 1e-15;

// DC offset method - adds tiny DC to prevent zeros
static constexpr float DENORMAL_DC = 1e-10f;

/**
 * Inline functions for maximum performance
 */

// Method 1: Flush to zero (fastest, may affect very quiet reverb tails)
inline float flushDenormal(float x) noexcept {
    return (std::abs(x) < DENORMAL_THRESHOLD) ? 0.0f : x;
}

inline double flushDenormal(double x) noexcept {
    return (std::abs(x) < DENORMAL_THRESHOLD_DOUBLE) ? 0.0 : x;
}

// Method 2: Add DC offset (preserves quiet signals but adds noise)
inline float addDenormalDC(float x) noexcept {
    return x + DENORMAL_DC;
}

// Method 3: Noise injection (best for reverbs, maintains tail character)
inline float injectDenormalNoise(float x, float& noiseState) noexcept {
    // Use a simple LFSR for fast pseudo-random generation
    noiseState = noiseState * 1103515245 + 12345;
    float noise = (noiseState / float(0x7FFFFFFF)) * 1e-12f;
    return x + noise;
}

// Method 4: Quantization (rounds very small values)
inline float quantizeDenormal(float x) noexcept {
    const float quantLevel = 1e-7f;
    return std::round(x / quantLevel) * quantLevel;
}

/**
 * SIMD optimized denormal protection for buffers
 */
#ifdef __SSE__
inline void flushDenormalBuffer_SSE(float* buffer, int numSamples) noexcept {
    const __m128 threshold = _mm_set1_ps(DENORMAL_THRESHOLD);
    const __m128 negThreshold = _mm_set1_ps(-DENORMAL_THRESHOLD);
    const __m128 zero = _mm_setzero_ps();
    
    int simdSamples = numSamples & ~3; // Process 4 samples at a time
    
    for (int i = 0; i < simdSamples; i += 4) {
        __m128 samples = _mm_loadu_ps(&buffer[i]);
        
        // Create mask for denormal values
        __m128 gtMask = _mm_cmpgt_ps(samples, negThreshold);
        __m128 ltMask = _mm_cmplt_ps(samples, threshold);
        __m128 denormalMask = _mm_and_ps(gtMask, ltMask);
        
        // Select: if denormal, use zero, else use original
        samples = _mm_blendv_ps(samples, zero, denormalMask);
        
        _mm_storeu_ps(&buffer[i], samples);
    }
    
    // Handle remaining samples
    for (int i = simdSamples; i < numSamples; ++i) {
        buffer[i] = flushDenormal(buffer[i]);
    }
}
#endif

/**
 * Class-based denormal protection with state
 */
class DenormalProtector {
private:
    float dcOffset = DENORMAL_DC;
    float noiseState = 0.12345f;
    bool useDCOffset = false;
    bool useNoise = false;
    
public:
    enum Mode {
        FLUSH,      // Flush to zero (default)
        DC_OFFSET,  // Add DC offset
        NOISE,      // Add noise
        QUANTIZE    // Quantize small values
    };
    
    void setMode(Mode mode) {
        useDCOffset = (mode == DC_OFFSET);
        useNoise = (mode == NOISE);
    }
    
    inline float process(float x) noexcept {
        if (useDCOffset) {
            return x + dcOffset;
        } else if (useNoise) {
            return injectDenormalNoise(x, noiseState);
        } else {
            return flushDenormal(x);
        }
    }
    
    void processBuffer(float* buffer, int numSamples) noexcept {
        #ifdef __SSE__
        if (!useDCOffset && !useNoise) {
            flushDenormalBuffer_SSE(buffer, numSamples);
            return;
        }
        #endif
        
        for (int i = 0; i < numSamples; ++i) {
            buffer[i] = process(buffer[i]);
        }
    }
};

/**
 * RAII guard for setting CPU denormal flags
 */
class DenormalDisabler {
private:
    #ifdef __SSE__
    unsigned int oldMXCSR;
    #endif
    
public:
    DenormalDisabler() noexcept {
        #ifdef __SSE__
        oldMXCSR = _mm_getcsr();
        // Set flush-to-zero and denormals-are-zero modes
        _mm_setcsr(oldMXCSR | 0x8040);
        #endif
    }
    
    ~DenormalDisabler() noexcept {
        #ifdef __SSE__
        _mm_setcsr(oldMXCSR);
        #endif
    }
    
    // Prevent copying
    DenormalDisabler(const DenormalDisabler&) = delete;
    DenormalDisabler& operator=(const DenormalDisabler&) = delete;
};

/**
 * Convenience RAII guard for process blocks
 */
class DenormalGuard {
private:
    DenormalDisabler disabler;
    
public:
    DenormalGuard() = default;
    ~DenormalGuard() = default;
    
    // Static method for quick inline protection
    static inline float protect(float x) noexcept {
        return flushDenormal(x);
    }
    
    // Static method for buffer protection
    static void protectBuffer(float* buffer, int numSamples) noexcept {
        #ifdef __SSE__
        flushDenormalBuffer_SSE(buffer, numSamples);
        #else
        for (int i = 0; i < numSamples; ++i) {
            buffer[i] = flushDenormal(buffer[i]);
        }
        #endif
    }
};

/**
 * Template for protecting any numeric type
 */
template<typename T>
inline T protectDenormal(T x) noexcept {
    if constexpr (std::is_same_v<T, float>) {
        return flushDenormal(x);
    } else if constexpr (std::is_same_v<T, double>) {
        return flushDenormal(static_cast<double>(x));
    } else {
        return x; // No protection for non-floating types
    }
}

} // namespace DenormalProtection
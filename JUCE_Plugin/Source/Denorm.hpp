#pragma once
#include <cmath>
#include <algorithm>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE 1
#else
    #define HAS_SSE 0
#endif

// Force inline macro
#ifndef ALWAYS_INLINE
    #ifdef _MSC_VER
        #define ALWAYS_INLINE __forceinline
    #else
        #define ALWAYS_INLINE __attribute__((always_inline)) inline
    #endif
#endif

// Denormal prevention utilities
template<typename T>
ALWAYS_INLINE T flushDenorm(T value) noexcept {
#if HAS_SSE
    if constexpr (std::is_same_v<T, float>) {
        return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(value), _mm_set_ss(0.0f)));
    }
#endif
    
    // Fallback for non-SSE or double
    constexpr T tiny = static_cast<T>(1.0e-38);
    return std::fabs(value) < tiny ? static_cast<T>(0.0) : value;
}

// Batch denormal flush
template<typename T>
ALWAYS_INLINE void flushDenormArray(T* data, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        data[i] = flushDenorm(data[i]);
    }
}

// Check for denormals
template<typename T>
ALWAYS_INLINE bool hasDenormal(T value) noexcept {
    return value != 0 && std::fabs(value) < static_cast<T>(1.0e-30);
}

// Check array for denormals
template<typename T>
bool checkDenormals(const T* data, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        if (hasDenormal(data[i])) {
            return true;
        }
    }
    return false;
}
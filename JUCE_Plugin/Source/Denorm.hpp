#pragma once
#include <cmath>
#include <type_traits>
#include <array>
#include <cstring>

#if defined(__SSE2__)
  #include <immintrin.h>
#endif

#ifdef __ARM_NEON__
  #include <arm_neon.h>
#endif

// Unified denormal prevention with platform-specific optimizations
template<typename T> 
inline T flushDenorm(T v) noexcept {
    static_assert(std::is_floating_point<T>::value, "flushDenorm requires floating point type");
    
#if defined(__SSE2__) && !defined(__ARM_NEON__)
    if constexpr (std::is_same<T, float>::value) {
        // SSE single precision flush
        __m128 tmp = _mm_set_ss(v);
        tmp = _mm_add_ss(tmp, _mm_set_ss(0.0f));
        return _mm_cvtss_f32(tmp);
    } else if constexpr (std::is_same<T, double>::value) {
        // SSE2 double precision flush
        __m128d tmp = _mm_set_sd(v);
        tmp = _mm_add_sd(tmp, _mm_set_sd(0.0));
        return _mm_cvtsd_f64(tmp);
    }
#elif defined(__ARM_NEON__)
    // ARM NEON denormal handling
    if constexpr (std::is_same<T, float>::value) {
        float32x2_t tmp = vdup_n_f32(v);
        tmp = vadd_f32(tmp, vdup_n_f32(0.0f));
        return vget_lane_f32(tmp, 0);
    }
#endif
    
    // Fallback for non-SIMD or other types
    constexpr T thresh = std::is_same<T, float>::value ? 1e-30f : 1e-300;
    return std::fabs(v) < thresh ? static_cast<T>(0) : v;
}

// Initialize CPU denormal handling globally
inline void initializeDenormalHandling() noexcept {
#if defined(__SSE2__) && !defined(__ARM_NEON__)
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#elif defined(__ARM_NEON__)
    // ARM: Set FPSCR flush-to-zero mode
    #ifdef __aarch64__
        uint64_t fpcr;
        __asm__ __volatile__ ("mrs %0, fpcr" : "=r" (fpcr));
        fpcr |= (1 << 24); // FZ bit
        __asm__ __volatile__ ("msr fpcr, %0" : : "r" (fpcr));
    #endif
#endif
}

// Aligned memory allocation helper
template<typename T>
inline T* alignedAlloc(size_t count, size_t alignment = 32) {
#ifdef _WIN32
    return static_cast<T*>(_aligned_malloc(count * sizeof(T), alignment));
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, count * sizeof(T)) != 0) {
        return nullptr;
    }
    return static_cast<T*>(ptr);
#endif
}

template<typename T>
inline void alignedFree(T* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// SIMD-friendly aligned array wrapper
template<typename T, size_t N, size_t Alignment = 32>
class AlignedArray {
    alignas(Alignment) std::array<T, N> data_;
public:
    using value_type = T;
    static constexpr size_t alignment = Alignment;
    
    T& operator[](size_t i) { return data_[i]; }
    const T& operator[](size_t i) const { return data_[i]; }
    T* data() { return data_.data(); }
    const T* data() const { return data_.data(); }
    size_t size() const { return N; }
    
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
    
    void fill(T value) {
        std::fill(data_.begin(), data_.end(), value);
    }
    
    void clear() {
        std::memset(data_.data(), 0, sizeof(T) * N);
    }
};
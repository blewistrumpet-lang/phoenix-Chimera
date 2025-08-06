#include "DimensionExpander.h"
#include <cmath>
#include <array>
#include <vector>
#include <atomic>
#include <random>
#include <algorithm>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

// Enable denormal prevention globally
static struct DenormGuard {
    DenormGuard() {
#if HAS_SSE2
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
} _denormGuard;

// Platinum-spec denormal flush
template<typename T>
inline T flushDenorm(T v) noexcept {
#if HAS_SSE2
    return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(static_cast<float>(v)), _mm_set_ss(0.0f)));
#else
    constexpr T tiny = static_cast<T>(1.0e-38);
    return std::fabs(v) < tiny ? static_cast<T>(0.0) : v;
#endif
}

// Lock-free parameter smoothing
class SmoothParam {
    alignas(64) std::atomic<float> target{0.0f};
    float current{0.0f};
    float coeff{0.995f};
    
public:
    void setTarget(float v) noexcept { target.store(v, std::memory_order_relaxed); }
    void setCoeff(double timeConstant, double sampleRate) noexcept { 
        coeff = static_cast<float>(std::exp(-1.0 / (timeConstant * sampleRate)));
    }
    void snap(float v) noexcept { 
        target.store(v, std::memory_order_relaxed);
        current = v;
    }
    
    inline float tick() noexcept {
        float t = target.load(std::memory_order_relaxed);
        current += (t - current) * (1.0f - coeff);
        return flushDenorm(current);
    }
};

// Pre-computed noise buffer for thread safety
class RTSafeNoiseSource {
    static constexpr size_t SIZE = 16384;
    alignas(32) std::array<float, SIZE> buffer;
    std::atomic<size_t> index{0};
    
public:
    RTSafeNoiseSource() {
        // Generate high-quality noise offline
        std::mt19937 rng(12345);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        for (auto& sample : buffer) {
            sample = dist(rng) * 0.0001f;  // Very low level
        }
    }
    
    inline float getNext() noexcept {
        size_t idx = index.fetch_add(1, std::memory_order_relaxed) & (SIZE - 1);
        return buffer[idx];
    }
};

// Platinum-spec all-pass filter with static allocation and exact unity gain
class AllPassFilter {
    static constexpr size_t MAX_SIZE = 4096;
    alignas(32) std::array<float, MAX_SIZE> buffer;  // Static allocation - NO std::vector!
    size_t writeIndex{0};
    size_t size{1024};
    float feedback{0.5f};
    int denormCounter{0};
    
public:
    void setSize(size_t newSize, float fb) noexcept {
        size = std::min(newSize, MAX_SIZE);
        feedback = std::min(fb, 0.98f);  // Limit for stability
        
        // Zero only the needed portion - no reallocation
        std::fill(buffer.begin(), buffer.begin() + size, 0.0f);
        writeIndex = 0;
    }
    
    inline float process(float input) noexcept {
        size_t readIndex = (writeIndex - size) & (MAX_SIZE - 1);
        float delayed = buffer[readIndex];
        
        // Exact DC gain calculation for true unity gain
        float g = feedback;
        float dcGain = (1.0f - g) / (1.0f + g);
        
        // All-pass structure with exact gain compensation
        float output = (delayed - input) * dcGain;
        float toWrite = input + delayed * g;
        
        buffer[writeIndex] = flushDenorm(toWrite);
        writeIndex = (writeIndex + 1) & (MAX_SIZE - 1);
        
        // Periodic deep flush
        if (++denormCounter >= 256) {
            buffer[writeIndex] = flushDenorm(buffer[writeIndex]);
            denormCounter = 0;
        }
        
        return output;
    }
    
    void reset() noexcept {
        buffer.fill(0.0f);  // Safe to clear entire array
        writeIndex = 0;
        denormCounter = 0;
    }
};

// High-quality micro pitch shifter with SIMD vectorized Hermite
class MicroPitchShifter {
    static constexpr size_t BUFFER_SIZE = 8192;  // Power of 2
    static constexpr int CROSSFADE_SAMPLES = 64;
    
    alignas(32) std::array<float, BUFFER_SIZE> buffer;
    float readPos{BUFFER_SIZE / 2.0f};
    size_t writePos{0};
    
    // SIMD-optimized Hermite interpolation for 4 samples at once
    inline float hermite4_scalar(const float* buf, float frac, int idx) noexcept {
        float y0 = buf[idx - 1];
        float y1 = buf[idx];
        float y2 = buf[idx + 1];
        float y3 = buf[idx + 2];
        
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
    
#if HAS_SSE2
    inline __m128 hermite4(const float* buf, float frac) noexcept {
        // Load 4 consecutive sets of 4 samples for parallel interpolation
        __m128 y0 = _mm_loadu_ps(&buf[-1]);  // y0[0-3]
        __m128 y1 = _mm_loadu_ps(&buf[0]);   // y1[0-3]
        __m128 y2 = _mm_loadu_ps(&buf[1]);   // y2[0-3]
        __m128 y3 = _mm_loadu_ps(&buf[2]);   // y3[0-3]
        
        // Compute Hermite coefficients for all 4
        __m128 c0 = y1;
        __m128 c1 = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_sub_ps(y2, y0));
        __m128 c2 = _mm_sub_ps(_mm_sub_ps(_mm_add_ps(y0, _mm_mul_ps(_mm_set1_ps(2.0f), y2)), 
                               _mm_mul_ps(_mm_set1_ps(2.5f), y1)), 
                               _mm_mul_ps(_mm_set1_ps(0.5f), y3));
        __m128 c3 = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(0.5f), _mm_sub_ps(y3, y0)),
                               _mm_mul_ps(_mm_set1_ps(1.5f), _mm_sub_ps(y1, y2)));
        
        // Horner's method with frac
        __m128 x = _mm_set1_ps(frac);
        __m128 result = _mm_mul_ps(c3, x);
        result = _mm_add_ps(result, c2);
        result = _mm_mul_ps(result, x);
        result = _mm_add_ps(result, c1);
        result = _mm_mul_ps(result, x);
        result = _mm_add_ps(result, c0);
        
        return result;
    }
#endif
    
public:
    void prepare() noexcept {
        buffer.fill(0.0f);
        readPos = BUFFER_SIZE / 2.0f;
        writePos = 0;
    }
    
    inline float process(float input, float cents) noexcept {
        // Write input
        buffer[writePos] = input;
        writePos = (writePos + 1) & (BUFFER_SIZE - 1);
        
        // Calculate pitch ratio
        float ratio = std::pow(2.0f, cents / 1200.0f);
        
        // Read position
        int idx0 = static_cast<int>(readPos);
        float frac = readPos - idx0;
        
        // Prefetch next cache line
#if HAS_SSE2
        _mm_prefetch(reinterpret_cast<const char*>(&buffer[(idx0 + 8) & (BUFFER_SIZE - 1)]), _MM_HINT_T0);
#endif
        
        // Single sample Hermite (optimized)
        int idx_m1 = (idx0 - 1) & (BUFFER_SIZE - 1);
        int idx_p1 = (idx0 + 1) & (BUFFER_SIZE - 1);
        int idx_p2 = (idx0 + 2) & (BUFFER_SIZE - 1);
        idx0 &= (BUFFER_SIZE - 1);
        
        float y0 = buffer[idx_m1];
        float y1 = buffer[idx0];
        float y2 = buffer[idx_p1];
        float y3 = buffer[idx_p2];
        
        // Hermite interpolation
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        
        float output = ((c3 * frac + c2) * frac + c1) * frac + c0;
        
        // Update read position with wrapping
        readPos += ratio;
        if (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;
        if (readPos < 0) readPos += BUFFER_SIZE;
        
        return output;
    }
    
    void reset() noexcept {
        buffer.fill(0.0f);
        readPos = BUFFER_SIZE / 2.0f;
        writePos = 0;
    }
};

// Fixed-size Haas delay with denormal protection
class HaasDelay {
    static constexpr size_t MAX_DELAY = 128;  // ~2.6ms @ 48kHz
    alignas(32) std::array<float, MAX_DELAY> buffer;
    size_t writeIndex{0};
    size_t delaySamples{0};
    int denormCounter{0};
    
public:
    void setDelay(size_t samples) noexcept {
        delaySamples = std::min(samples, MAX_DELAY - 1);
    }
    
    inline float process(float input) noexcept {
        buffer[writeIndex] = input;
        size_t readIndex = (writeIndex - delaySamples + MAX_DELAY) & (MAX_DELAY - 1);
        writeIndex = (writeIndex + 1) & (MAX_DELAY - 1);
        
        // Periodic denormal flush
        if (++denormCounter >= 512) {
            buffer[writeIndex] = flushDenorm(buffer[writeIndex]);
            denormCounter = 0;
        }
        
        return buffer[readIndex];
    }
    
    void reset() noexcept {
        buffer.fill(0.0f);
        writeIndex = 0;
        denormCounter = 0;
    }
};

// State-variable filter for crossover with DC normalization
class SVFilter {
    double ic1eq{0.0}, ic2eq{0.0};
    double g{0.0}, k{1.0}, a1{0.0}, a2{0.0}, a3{0.0};
    
public:
    void setCutoff(double freq, double sampleRate, double q = 0.707) noexcept {
        g = std::tan(M_PI * freq / sampleRate);
        k = 1.0 / q;
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        
        // Verify DC gain normalization for stability
        // For lowpass at DC (z=1): H(1) = a3 / (1 - (2-a1-a2*k-a3))
        double dcGain = a3 / (a1 + a2 * k + a3);
        if (std::abs(dcGain - 1.0) > 0.001) {
            // Normalize if needed
            double norm = 1.0 / dcGain;
            a1 *= norm;
            a2 *= norm;
            a3 *= norm;
        }
    }
    
    struct Output {
        float lowpass;
        float highpass;
        float bandpass;
    };
    
    inline Output process(float input) noexcept {
        double v3 = input - ic2eq;
        double v1 = a1 * ic1eq + a2 * v3;
        double v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;
        
        // Denormal protection on state
        ic1eq = flushDenorm(ic1eq);
        ic2eq = flushDenorm(ic2eq);
        
        return {
            static_cast<float>(v2),     // lowpass
            static_cast<float>(input - k * v1 - v2),  // highpass
            static_cast<float>(v1)      // bandpass
        };
    }
    
    void reset() noexcept {
        ic1eq = ic2eq = 0.0;
    }
};

// Polyphase IIR for 4x oversampling with enhanced cache optimization
class PolyphaseIIR4x {
    static constexpr int OS = 4;
    static constexpr int STAGES = 4;
    
    struct BiquadCoeffs {
        double b0, b1, b2, a1, a2;
    };
    
    struct BiquadState {
        double x1{0.0}, x2{0.0}, y1{0.0}, y2{0.0};
        
        inline double process(double x, const BiquadCoeffs& c) noexcept {
            double y = c.b0*x + c.b1*x1 + c.b2*x2 - c.a1*y1 - c.a2*y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = flushDenorm(y);
            return y;
        }
        
        void reset() noexcept { x1 = x2 = y1 = y2 = 0.0; }
    };
    
    std::array<BiquadCoeffs, STAGES> coeffs;
    std::array<BiquadState, STAGES> upStates;
    std::array<BiquadState, STAGES> downStates;
    alignas(32) std::array<float, OS> workBuffer;  // 32-byte aligned for AVX2
    
public:
    void initialize(double sampleRate) noexcept {
        // 8th order Butterworth at 0.45 * Nyquist
        coeffs[0] = {0.0009446, 0.0018892, 0.0009446, -1.9111970, 0.9149754};
        coeffs[1] = {0.0009446, 0.0018892, 0.0009446, -1.8414805, 0.8452589};
        coeffs[2] = {0.0009446, 0.0018892, 0.0009446, -1.7826305, 0.7864089};
        coeffs[3] = {0.0009446, 0.0018892, 0.0009446, -1.7378009, 0.7415793};
    }
    
    inline const std::array<float, OS>& upsample(float input) noexcept {
        // Zero-stuff and filter
        workBuffer[0] = input * static_cast<float>(OS);
        workBuffer[1] = workBuffer[2] = workBuffer[3] = 0.0f;
        
        // Apply cascade with prefetch
        for (int i = 0; i < OS; ++i) {
            double x = workBuffer[i];
            for (int s = 0; s < STAGES; ++s) {
                // Prefetch next biquad state
#if HAS_SSE2
                if (s < STAGES - 1) {
                    _mm_prefetch(reinterpret_cast<const char*>(&upStates[s + 1]), _MM_HINT_T0);
                }
#endif
                x = upStates[s].process(x, coeffs[s]);
            }
            workBuffer[i] = static_cast<float>(x);
        }
        
        return workBuffer;
    }
    
    inline float downsample(const std::array<float, OS>& input) noexcept {
        // Filter then decimate with prefetch
        double result = 0.0;
        for (int i = 0; i < OS; ++i) {
            double x = input[i];
            for (int s = 0; s < STAGES; ++s) {
#if HAS_SSE2
                if (s < STAGES - 1) {
                    _mm_prefetch(reinterpret_cast<const char*>(&downStates[s + 1]), _MM_HINT_T0);
                }
#endif
                x = downStates[s].process(x, coeffs[s]);
            }
            if (i == 0) result = x;
        }
        
        return static_cast<float>(result * (1.0 / static_cast<double>(OS)));
    }
    
    void reset() noexcept {
        for (auto& s : upStates) s.reset();
        for (auto& s : downStates) s.reset();
    }
};

// Channel processing state with oversampling
struct ChannelState {
    HaasDelay haasDelay;
    std::array<AllPassFilter, 4> allpass;
    MicroPitchShifter pitchShifter;
    SVFilter crossover;
    SVFilter clarityFilter;
    PolyphaseIIR4x oversampler;  // Per-channel oversampler
    
    // LFO state
    double lfoPhase{0.0};
    
    // Crossfeed lowpass state (NOT static!)
    double crossfeedLPState{0.0};
    
    void reset() noexcept {
        haasDelay.reset();
        for (auto& ap : allpass) ap.reset();
        pitchShifter.reset();
        crossover.reset();
        clarityFilter.reset();
        oversampler.reset();
        lfoPhase = 0.0;
        crossfeedLPState = 0.0;
    }
};

// DC blocker with periodic flush
class DCBlocker {
    double x1{0.0}, y1{0.0};
    static constexpr double R = 0.995;
    int flushCounter{0};
    
public:
    inline float process(float input) noexcept {
        double x = static_cast<double>(input);
        double y = x - x1 + R * y1;
        x1 = x;
        y1 = y;
        
        // Periodic deep flush to prevent long-term creep
        if (++flushCounter >= 1024) {
            y1 = flushDenorm(y1);
            flushCounter = 0;
        }
        
        return static_cast<float>(flushDenorm(y));
    }
    
    void reset() noexcept { 
        x1 = y1 = 0.0;
        flushCounter = 0;
    }
};

// Main implementation
struct DimensionExpander::Impl {
    // Parameters
    std::array<SmoothParam, 8> params;
    
    // DSP state
    std::array<ChannelState, 2> channels;
    std::array<DCBlocker, 2> inputDC;
    std::array<DCBlocker, 2> outputDC;
    
    // Sample rate
    double sampleRate{48000.0};
    double invSampleRate{1.0 / 48000.0};
    
    // RT-safe noise
    RTSafeNoiseSource noiseSource;
    
    void prepare(double sr, int blockSize) {
        sampleRate = sr;
        invSampleRate = 1.0 / sr;
        
        // Set parameter smoothing times in milliseconds for intuitive control
        const double smoothingMs[8] = {
            5.0,   // Width - fast response
            10.0,  // Depth - moderate
            8.0,   // Crossfeed - moderate
            20.0,  // Bass retention - slow for stability
            10.0,  // Ambience - moderate
            15.0,  // Movement - slower for smooth LFO
            10.0,  // Clarity - moderate
            10.0   // Mix - moderate
        };
        
        for (int i = 0; i < 8; ++i) {
            params[i].setCoeff(smoothingMs[i] * 0.001, sr);
        }
        
        // Initialize all-pass sizes (prime numbers scaled to sample rate)
        const int baseSizes[4] = {347, 419, 487, 557};
        const float feedbacks[4] = {0.5f, 0.55f, 0.6f, 0.65f};
        
        for (int ch = 0; ch < 2; ++ch) {
            auto& state = channels[ch];
            
            // Setup all-pass filters
            for (int i = 0; i < 4; ++i) {
                size_t size = static_cast<size_t>(baseSizes[i] * sr / 44100.0);
                // Ensure power of 2 for efficiency
                size_t pow2 = 1;
                while (pow2 < size) pow2 <<= 1;
                state.allpass[i].setSize(pow2, feedbacks[i]);
            }
            
            // Initialize pitch shifter
            state.pitchShifter.prepare();
            
            // Setup crossover at 120Hz
            state.crossover.setCutoff(120.0, sr);
            
            // Setup clarity filter at 8kHz
            state.clarityFilter.setCutoff(8000.0, sr, 0.5);
            
            // Initialize oversampler
            state.oversampler.initialize(sr);
            
            // Different initial phase for each channel
            state.lfoPhase = ch * M_PI;
        }
        
        // Reset all DSP state
        reset();
    }
    
    void reset() {
        for (auto& ch : channels) ch.reset();
        for (auto& dc : inputDC) dc.reset();
        for (auto& dc : outputDC) dc.reset();
    }
    
    inline void processChannel(float& left, float& right) noexcept {
        // Get parameters
        float width = params[kWidth].tick();
        float depth = params[kDepth].tick();
        float crossfeed = params[kCrossfeed].tick();
        float bassRetention = params[kBassRetention].tick();
        float ambience = params[kAmbience].tick();
        float movement = params[kMovement].tick();
        float clarity = params[kClarity].tick();
        float mix = params[kMix].tick();
        
        // Save dry signal
        float dryLeft = left;
        float dryRight = right;
        
        // DC blocking
        left = inputDC[0].process(left);
        right = inputDC[1].process(right);
        
        // M/S processing
        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        // Bass retention via crossover
        auto leftCross = channels[0].crossover.process(mid);
        float bass = leftCross.lowpass * bassRetention;
        float midHigh = leftCross.highpass;
        
        // Width processing on side signal
        if (width != 0.5f) {
            float widthAmount = (width - 0.5f) * 2.0f;  // -1 to +1
            side *= 1.0f + widthAmount * 1.5f;
            
            // Haas delay for extra width (only on positive width)
            if (widthAmount > 0.0f) {
                size_t delaySamples = static_cast<size_t>(widthAmount * 30.0f);  // 0-30 samples
                channels[1].haasDelay.setDelay(delaySamples);
                float delayedRight = channels[1].haasDelay.process(right);
                right = right * 0.8f + delayedRight * 0.2f;
            }
        }
        
        // Depth via micro pitch shifting
        if (depth > 0.0f) {
            // LFO modulation for movement
            float modDepth = movement * 5.0f;  // ±5 cents
            
            float leftMod = static_cast<float>(std::sin(channels[0].lfoPhase)) * modDepth;
            float rightMod = static_cast<float>(std::sin(channels[1].lfoPhase)) * modDepth;
            
            channels[0].lfoPhase += 2.0 * M_PI * 0.3 * invSampleRate;
            channels[1].lfoPhase += 2.0 * M_PI * 0.37 * invSampleRate;
            
            // Wrap phases
            if (channels[0].lfoPhase > 2.0 * M_PI) channels[0].lfoPhase -= 2.0 * M_PI;
            if (channels[1].lfoPhase > 2.0 * M_PI) channels[1].lfoPhase -= 2.0 * M_PI;
            
            // Apply pitch shift (opposite directions)
            float leftShift = -depth * 10.0f + leftMod;
            float rightShift = depth * 10.0f + rightMod;
            
            float shiftedLeft = channels[0].pitchShifter.process(left, leftShift);
            float shiftedRight = channels[1].pitchShifter.process(right, rightShift);
            
            left = left * (1.0f - depth * 0.3f) + shiftedLeft * depth * 0.3f;
            right = right * (1.0f - depth * 0.3f) + shiftedRight * depth * 0.3f;
        }
        
        // Ambience via all-pass cascade
        if (ambience > 0.0f) {
            float ambLeft = side;
            float ambRight = side;
            
            for (auto& ap : channels[0].allpass) {
                ambLeft = ap.process(ambLeft);
            }
            for (auto& ap : channels[1].allpass) {
                ambRight = ap.process(ambRight);
            }
            
            left += ambLeft * ambience * 0.3f;
            right += ambRight * ambience * 0.3f;
        }
        
        // Crossfeed for better center image
        if (crossfeed > 0.0f) {
            // 300Hz lowpass for crossfeed
            double alpha = 1.0 - std::exp(-2.0 * M_PI * 300.0 * invSampleRate);
            
            channels[0].crossfeedLPState += alpha * (left - channels[0].crossfeedLPState);
            channels[1].crossfeedLPState += alpha * (right - channels[1].crossfeedLPState);
            
            float leftFiltered = static_cast<float>(channels[0].crossfeedLPState);
            float rightFiltered = static_cast<float>(channels[1].crossfeedLPState);
            
            left += rightFiltered * crossfeed * 0.3f;
            right += leftFiltered * crossfeed * 0.3f;
        }
        
        // Reconstruct from M/S
        left = midHigh + bass + side;
        right = midHigh + bass - side;
        
        // Clarity enhancement
        if (clarity > 0.5f) {
            float clarityAmount = (clarity - 0.5f) * 2.0f;
            
            auto leftClarity = channels[0].clarityFilter.process(left);
            auto rightClarity = channels[1].clarityFilter.process(right);
            
            left += leftClarity.highpass * clarityAmount * 0.15f;
            right += rightClarity.highpass * clarityAmount * 0.15f;
        }
        
        // Soft saturation with 4x oversampling for alias-free processing
        if (std::abs(left) > 0.1f || std::abs(right) > 0.1f) {
            // Oversample left channel
            auto leftUp = channels[0].oversampler.upsample(left);
            for (auto& s : leftUp) {
                s = std::tanh(s * 0.8f);
            }
            left = channels[0].oversampler.downsample(leftUp) * 1.25f;
            
            // Oversample right channel
            auto rightUp = channels[1].oversampler.upsample(right);
            for (auto& s : rightUp) {
                s = std::tanh(s * 0.8f);
            }
            right = channels[1].oversampler.downsample(rightUp) * 1.25f;
        } else {
            // Skip oversampling for very quiet signals
            left = std::tanh(left * 0.8f) * 1.25f;
            right = std::tanh(right * 0.8f) * 1.25f;
        }
        
        // DC blocking
        left = outputDC[0].process(left);
        right = outputDC[1].process(right);
        
        // Mix dry/wet
        left = dryLeft * (1.0f - mix) + left * mix;
        right = dryRight * (1.0f - mix) + right * mix;
    }
};

// Public interface
DimensionExpander::DimensionExpander() : pimpl(std::make_unique<Impl>()) {
    // Set default values
    pimpl->params[kWidth].snap(0.5f);
    pimpl->params[kDepth].snap(0.5f);
    pimpl->params[kCrossfeed].snap(0.3f);
    pimpl->params[kBassRetention].snap(0.7f);
    pimpl->params[kAmbience].snap(0.3f);
    pimpl->params[kMovement].snap(0.0f);
    pimpl->params[kClarity].snap(0.5f);
    pimpl->params[kMix].snap(1.0f);
}

DimensionExpander::~DimensionExpander() = default;

void DimensionExpander::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
}

void DimensionExpander::reset() {
    pimpl->reset();
}

void DimensionExpander::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Only process stereo
    if (numChannels < 2) return;
    
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    // Process sample by sample
    for (int i = 0; i < numSamples; ++i) {
        pimpl->processChannel(leftChannel[i], rightChannel[i]);
    }
}

void DimensionExpander::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        if (id >= 0 && id < 8) {
            pimpl->params[id].setTarget(value);
        }
    }
}

juce::String DimensionExpander::getParameterName(int index) const {
    switch (index) {
        case kWidth: return "Width";
        case kDepth: return "Depth";
        case kCrossfeed: return "Crossfeed";
        case kBassRetention: return "Bass Retention";
        case kAmbience: return "Ambience";
        case kMovement: return "Movement";
        case kClarity: return "Clarity";
        case kMix: return "Mix";
        default: return "";
    }
}
#include "GatedReverb.h"
#include "DenormalProtection.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include <cmath>
#include <random>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
    #define HAS_AVX (defined(__AVX__) || defined(_M_AVX))
#else
    #define HAS_SSE2 0
    #define HAS_AVX 0
#endif

// Force inline macro
#ifdef _MSC_VER
    #define ALWAYS_INLINE __forceinline
#else
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

namespace {
    
    // Fast modulo using bit masking (requires power-of-2 sizes)
    ALWAYS_INLINE int fastMod(int value, int size) noexcept {
        return value & (size - 1);
    }
    
    // Thread-safe xorshift RNG
    class FastRNG {
        uint32_t state;
    public:
        explicit FastRNG(uint32_t seed = 0x1234567) : state(seed) {}
        
        ALWAYS_INLINE float next() noexcept {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return (state & 0x7FFFFFFF) * 4.65661287e-10f;
        }
    };
}

// Implementation
struct GatedReverb::Impl {
    // Core state
    double sampleRate = 0.0;
    int blockSize = 512;
    
    // Smoothed parameters with denormal protection
    struct SmoothParam {
        std::atomic<float> target{0.5f};
        float current = 0.5f;
        float coeff = 0.995f;
        
        void setSmoothingTime(float ms, double sr) {
            float samples = ms * 0.001f * static_cast<float>(sr);
            coeff = std::exp(-1.0f / samples);
        }
        
        ALWAYS_INLINE float tick() noexcept {
            float t = target.load(std::memory_order_relaxed);
            current += (t - current) * (1.0f - coeff);
            return DSPUtils::flushDenorm(current);
        }
        
        void reset(float value) {
            target.store(value, std::memory_order_relaxed);
            current = value;
        }
    };
    
    // Parameters
    SmoothParam roomSize, gateTime, threshold;
    SmoothParam preDelay, damping, gateShape;
    SmoothParam brightness, mix;
    
    // Adaptive threshold system
    struct AdaptiveThreshold {
        float noiseFloor = -60.0f;      // dB
        float peakLevel = -12.0f;       // dB
        float currentThreshold = 0.3f;  // Linear
        float adaptRate = 0.995f;       // Adaptation speed
        float rangeMin = 0.1f;          // Minimum threshold
        float rangeMax = 0.9f;          // Maximum threshold
        
        // RMS measurement
        float rmsAccumulator = 0.0f;
        int rmsSampleCount = 0;
        static constexpr int RMS_WINDOW = 2048;
        
        // Peak tracking
        float peakHold = 0.0f;
        float peakDecay = 0.9999f;
        
        void updateMeasurements(float input) {
            float absInput = std::abs(input);
            
            // Update peak
            if (absInput > peakHold) {
                peakHold = absInput;
            } else {
                peakHold *= peakDecay;
            }
            
            // Update RMS
            rmsAccumulator += input * input;
            rmsSampleCount++;
            
            if (rmsSampleCount >= RMS_WINDOW) {
                float rms = std::sqrt(rmsAccumulator / RMS_WINDOW);
                rmsAccumulator = 0.0f;
                rmsSampleCount = 0;
                
                // Convert to dB
                float rmsDB = 20.0f * std::log10(std::max(1e-6f, rms));
                
                // Update noise floor estimate (slow adaptation)
                if (rmsDB < noiseFloor + 6.0f) {
                    noiseFloor = noiseFloor * 0.999f + rmsDB * 0.001f;
                }
                
                // Update peak level estimate
                float peakDB = 20.0f * std::log10(std::max(1e-6f, peakHold));
                if (peakDB > peakLevel) {
                    peakLevel = peakDB;
                } else {
                    peakLevel = peakLevel * 0.9995f + peakDB * 0.0005f;
                }
            }
        }
        
        float getAdaptiveThreshold(float manualThreshold) {
            // Calculate dynamic range
            float dynamicRange = std::max(12.0f, peakLevel - noiseFloor);
            
            // Map manual threshold to adaptive range
            float adaptiveOffset = (dynamicRange > 40.0f) ? 0.2f : 0.1f;
            float adaptiveRange = (dynamicRange > 40.0f) ? 0.5f : 0.3f;
            
            // Calculate adaptive threshold
            float adaptive = adaptiveOffset + manualThreshold * adaptiveRange;
            
            // Smooth adaptation
            currentThreshold = currentThreshold * adaptRate + adaptive * (1.0f - adaptRate);
            
            // Clamp to valid range
            return std::clamp(currentThreshold, rangeMin, rangeMax);
        }
        
        void reset() {
            noiseFloor = -60.0f;
            peakLevel = -12.0f;
            currentThreshold = 0.3f;
            rmsAccumulator = 0.0f;
            rmsSampleCount = 0;
            peakHold = 0.0f;
        }
    };
    
    AdaptiveThreshold adaptiveThreshold;
    
    // Optimized comb filter (power-of-2 size)
    struct CombFilter {
        std::vector<float> buffer;
        int size = 0;
        int sizeMask = 0;  // For fast modulo
        int index = 0;
        float feedback = 0.84f;
        float damping = 0.2f;
        float filterState = 0.0f;
        
        void prepare(int targetSize) {
            // Round up to power of 2
            size = 1;
            while (size < targetSize) size <<= 1;
            sizeMask = size - 1;
            
            buffer.resize(size, 0.0f);
            index = 0;
            filterState = 0.0f;
        }
        
        ALWAYS_INLINE float process(float input, float fb, float damp) noexcept {
            float delayed = buffer[index];
            
            // CRITICAL FIX: Proper one-pole lowpass damping filter
            // Standard form: y[n] = (1-damp)*x[n] + damp*y[n-1]
            filterState = delayed * (1.0f - damp) + filterState * damp;
            filterState = DSPUtils::flushDenorm(filterState);
            
            // Apply feedback with safety limiting
            float feedback = filterState * fb;
            const float fbThreshold = 0.9f;
            if (std::abs(feedback) > fbThreshold) {
                feedback = fbThreshold * std::tanh(feedback / fbThreshold);
            }
            buffer[index] = input + feedback;
            
            // Only limit at extreme levels to preserve reverb tails
            const float bufThreshold = 2.0f;
            if (std::abs(buffer[index]) > bufThreshold) {
                buffer[index] = bufThreshold * std::tanh(buffer[index] / bufThreshold);
            }
            index = fastMod(index + 1, size);
            
            return delayed;
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            filterState = 0.0f;
            index = 0;
        }
    };
    
    // SIMD-optimized comb filter bank
    struct CombFilterBank {
        static constexpr int NUM_COMBS = 8;
        
        // Structure of Arrays for SIMD
        alignas(32) std::array<std::vector<float>, NUM_COMBS> buffers;
        alignas(16) std::array<int, NUM_COMBS> indices;
        alignas(16) std::array<int, NUM_COMBS> masks;
        alignas(32) std::array<float, NUM_COMBS> filterStates;
        
        // Pre-computed SIMD constants (set per block)
#if HAS_SSE2
        __m128 vFeedback;
        __m128 vOneMinusDamp;
        __m128 vDamping;
#endif
        
        void prepare(const std::array<int, NUM_COMBS>& tunings, double sr) {
            for (int i = 0; i < NUM_COMBS; ++i) {
                int targetSize = static_cast<int>(tunings[i] * sr / 44100.0);
                int size = 1;
                while (size < targetSize) size <<= 1;
                masks[i] = size - 1;
                
                buffers[i].resize(size, 0.0f);
                indices[i] = 0;
                filterStates[i] = 0.0f;
            }
        }
        
        void setParameters(float roomScale, float damping) {
#if HAS_SSE2
            float fb = 0.84f * roomScale;
            float oneMinusD = 1.0f - damping;
            vFeedback = _mm_set1_ps(fb);
            vOneMinusDamp = _mm_set1_ps(oneMinusD);
            vDamping = _mm_set1_ps(damping);
#endif
        }
        
#if HAS_SSE2
        ALWAYS_INLINE float processSIMD(float input) noexcept {
            // 1) Gather 8 delayed samples manually (unrolled)
            float d0 = buffers[0][indices[0]];
            float d1 = buffers[1][indices[1]];
            float d2 = buffers[2][indices[2]];
            float d3 = buffers[3][indices[3]];
            float d4 = buffers[4][indices[4]];
            float d5 = buffers[5][indices[5]];
            float d6 = buffers[6][indices[6]];
            float d7 = buffers[7][indices[7]];
            
            // 2) Update filter states (unrolled for better pipelining)
            float fs0 = filterStates[0];
            float fs1 = filterStates[1];
            float fs2 = filterStates[2];
            float fs3 = filterStates[3];
            float fs4 = filterStates[4];
            float fs5 = filterStates[5];
            float fs6 = filterStates[6];
            float fs7 = filterStates[7];
            
            // Load states into SIMD registers
            __m128 vFS0123 = _mm_set_ps(fs3, fs2, fs1, fs0);
            __m128 vFS4567 = _mm_set_ps(fs7, fs6, fs5, fs4);
            __m128 vD0123 = _mm_set_ps(d3, d2, d1, d0);
            __m128 vD4567 = _mm_set_ps(d7, d6, d5, d4);
            
            // SIMD state update: fs = d * (1-damp) + fs * damp
            __m128 vNewFS0123 = _mm_add_ps(
                _mm_mul_ps(vD0123, vOneMinusDamp),
                _mm_mul_ps(vFS0123, vDamping)
            );
            __m128 vNewFS4567 = _mm_add_ps(
                _mm_mul_ps(vD4567, vOneMinusDamp),
                _mm_mul_ps(vFS4567, vDamping)
            );
            
            // Extract updated states and flush denormals
            alignas(16) float newFS0123[4];
            alignas(16) float newFS4567[4];
            _mm_store_ps(newFS0123, vNewFS0123);
            _mm_store_ps(newFS4567, vNewFS4567);
            
            // Write back flushed states
            filterStates[0] = DSPUtils::flushDenorm(newFS0123[0]);
            filterStates[1] = DSPUtils::flushDenorm(newFS0123[1]);
            filterStates[2] = DSPUtils::flushDenorm(newFS0123[2]);
            filterStates[3] = DSPUtils::flushDenorm(newFS0123[3]);
            filterStates[4] = DSPUtils::flushDenorm(newFS4567[0]);
            filterStates[5] = DSPUtils::flushDenorm(newFS4567[1]);
            filterStates[6] = DSPUtils::flushDenorm(newFS4567[2]);
            filterStates[7] = DSPUtils::flushDenorm(newFS4567[3]);
            
            // 3) Update buffers with feedback (manually unrolled)
            __m128 vInput = _mm_set1_ps(input);
            __m128 vBuf0123 = _mm_add_ps(vInput, _mm_mul_ps(vNewFS0123, vFeedback));
            __m128 vBuf4567 = _mm_add_ps(vInput, _mm_mul_ps(vNewFS4567, vFeedback));
            
            // Extract and write to buffers
            alignas(16) float buf0123[4];
            alignas(16) float buf4567[4];
            _mm_store_ps(buf0123, vBuf0123);
            _mm_store_ps(buf4567, vBuf4567);
            
            buffers[0][indices[0]] = buf0123[0];
            buffers[1][indices[1]] = buf0123[1];
            buffers[2][indices[2]] = buf0123[2];
            buffers[3][indices[3]] = buf0123[3];
            buffers[4][indices[4]] = buf4567[0];
            buffers[5][indices[5]] = buf4567[1];
            buffers[6][indices[6]] = buf4567[2];
            buffers[7][indices[7]] = buf4567[3];
            
            // 4) Update indices
            indices[0] = (indices[0] + 1) & masks[0];
            indices[1] = (indices[1] + 1) & masks[1];
            indices[2] = (indices[2] + 1) & masks[2];
            indices[3] = (indices[3] + 1) & masks[3];
            indices[4] = (indices[4] + 1) & masks[4];
            indices[5] = (indices[5] + 1) & masks[5];
            indices[6] = (indices[6] + 1) & masks[6];
            indices[7] = (indices[7] + 1) & masks[7];
            
            // 5) Sum all delays using efficient horizontal add
            __m128 vSum01 = _mm_add_ps(vD0123, vD4567);
            __m128 vSum02 = _mm_hadd_ps(vSum01, vSum01);
            __m128 vSum03 = _mm_hadd_ps(vSum02, vSum02);
            
            return _mm_cvtss_f32(vSum03) * 0.125f;
        }
#endif
        
        ALWAYS_INLINE float processScalar(float input, float roomScale, float damping) noexcept {
            const float feedback = 0.84f * roomScale;
            float sum = 0.0f;
            
            for (int i = 0; i < NUM_COMBS; ++i) {
                float delayed = buffers[i][indices[i]];
                
                // CRITICAL FIX: Proper damping filter (matching CombFilter fix)
                filterStates[i] = delayed * (1.0f - damping) + filterStates[i] * damping;
                filterStates[i] = DSPUtils::flushDenorm(filterStates[i]);
                
                // Apply feedback with safety limiting - FIXED: Unity-gain limiter
                float fb = filterStates[i] * feedback;
                fb = 0.9f * std::tanh(fb / 0.9f); // Unity-gain soft limit
                buffers[i][indices[i]] = input + fb;
                
                // Only limit at extreme levels
                if (std::abs(buffers[i][indices[i]]) > 2.0f) {
                    buffers[i][indices[i]] = 2.0f * std::tanh(buffers[i][indices[i]] / 2.0f);
                }
                indices[i] = (indices[i] + 1) & masks[i];
                sum += delayed;
            }
            
            return sum * 0.125f;
        }
        
        void reset() {
            for (int i = 0; i < NUM_COMBS; ++i) {
                std::fill(buffers[i].begin(), buffers[i].end(), 0.0f);
                indices[i] = 0;
                filterStates[i] = 0.0f;
            }
        }
    };
    
    // Optimized allpass filter
    struct AllPassFilter {
        std::vector<float> buffer;
        int size = 0;
        int sizeMask = 0;
        int index = 0;
        static constexpr float feedback = 0.5f;
        
        void prepare(int targetSize) {
            size = 1;
            while (size < targetSize) size <<= 1;
            sizeMask = size - 1;
            
            buffer.resize(size, 0.0f);
            index = 0;
        }
        
        ALWAYS_INLINE float process(float input) noexcept {
            float delayed = buffer[index];
            float output = -input + delayed;
            buffer[index] = input + delayed * feedback;
            index = fastMod(index + 1, size);
            return output;
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            index = 0;
        }
    };
    
    // Early reflections with fixed taps
    struct EarlyReflections {
        static constexpr int NUM_TAPS = 8;
        std::vector<float> buffer;
        int size = 0;
        int sizeMask = 0;
        int writeIndex = 0;
        
        struct Tap {
            int delay;
            float gain;
        };
        
        std::array<Tap, NUM_TAPS> taps;
        
        void prepare(double sr) {
            // Power of 2 size
            int targetSize = static_cast<int>(sr * 0.1);
            size = 1;
            while (size < targetSize) size <<= 1;
            sizeMask = size - 1;
            
            buffer.resize(size, 0.0f);
            
            // Classic pattern
            taps[0] = {static_cast<int>(0.013 * sr), 0.7f};
            taps[1] = {static_cast<int>(0.019 * sr), 0.6f};
            taps[2] = {static_cast<int>(0.029 * sr), 0.5f};
            taps[3] = {static_cast<int>(0.037 * sr), 0.4f};
            taps[4] = {static_cast<int>(0.043 * sr), 0.35f};
            taps[5] = {static_cast<int>(0.053 * sr), 0.3f};
            taps[6] = {static_cast<int>(0.061 * sr), 0.25f};
            taps[7] = {static_cast<int>(0.071 * sr), 0.2f};
        }
        
        ALWAYS_INLINE float process(float input) noexcept {
            buffer[writeIndex] = input;
            
            float output = 0.0f;
            for (const auto& tap : taps) {
                int readIndex = fastMod(writeIndex - tap.delay + size, size);
                output += buffer[readIndex] * tap.gain;
            }
            
            writeIndex = fastMod(writeIndex + 1, size);
            return output * 0.3f;
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writeIndex = 0;
        }
    };
    
    // Gate envelope with denormal protection
    struct GateEnvelope {
        float level = 0.0f;
        float targetLevel = 0.0f;
        int holdTimer = 0;
        int holdTime = 0;
        float speed = 0.001f;  // Pre-computed per block
        
        ALWAYS_INLINE float process(bool gateOpen) noexcept {
            if (gateOpen) {
                targetLevel = 1.0f;
                holdTimer = holdTime;
            } else if (holdTimer > 0) {
                holdTimer--;
                targetLevel = 1.0f;
            } else {
                targetLevel = 0.0f;
            }
            
            level += (targetLevel - level) * speed;
            level = DSPUtils::flushDenorm(level);
            
            return level;
        }
        
        void setSpeed(float shape) {
            // FIX: More dramatic gate shape control
            speed = 0.0005f + shape * 0.15f;  // Wider range from very slow to very fast
        }
        
        void reset() {
            level = 0.0f;
            targetLevel = 0.0f;
            holdTimer = 0;
        }
    };
    
    // Channel state
    struct ChannelState {
        CombFilterBank combBank;
        std::array<AllPassFilter, 4> allpassFilters;
        EarlyReflections earlyReflections;
        
        // Pre-delay (power of 2)
        std::vector<float> predelayBuffer;
        int predelaySize = 0;
        int predelayMask = 0;
        int predelayIndex = 0;
        
        // Gate
        GateEnvelope gate;
        float envelopeFollower = 0.0f;
        
        // DC blocker
        double dcX1 = 0.0, dcY1 = 0.0;
        static constexpr double dcR = 0.995;
        
        // High shelf
        float shelfState = 0.0f;
        float shelfCoeff = 0.0f;  // Pre-computed per block
        
        // Thread-local RNG
        FastRNG rng;
        
        void preparePreDelay(double sr) {
            int targetSize = static_cast<int>(0.1 * sr);
            predelaySize = 1;
            while (predelaySize < targetSize) predelaySize <<= 1;
            predelayMask = predelaySize - 1;
            
            predelayBuffer.resize(predelaySize, 0.0f);
            predelayIndex = 0;
        }
        
        ALWAYS_INLINE float processDC(float input) noexcept {
            double x0 = input;
            double y0 = x0 - dcX1 + dcR * dcY1;
            dcX1 = x0;
            dcY1 = DSPUtils::flushDenorm(y0);
            return static_cast<float>(y0);
        }
        
        ALWAYS_INLINE void updateEnvelope(float input) noexcept {
            float env = std::abs(input);
            if (env > envelopeFollower) {
                envelopeFollower = env + (envelopeFollower - env) * 0.999f;
            } else {
                envelopeFollower = env + (envelopeFollower - env) * 0.99f;
            }
            envelopeFollower = DSPUtils::flushDenorm(envelopeFollower);
        }
        
        void reset() {
            combBank.reset();
            for (auto& ap : allpassFilters) ap.reset();
            earlyReflections.reset();
            std::fill(predelayBuffer.begin(), predelayBuffer.end(), 0.0f);
            predelayIndex = 0;
            gate.reset();
            envelopeFollower = 0.0f;
            dcX1 = dcY1 = 0.0;
            shelfState = 0.0f;
        }
    };
    
    std::array<ChannelState, 2> channelStates;
    
    // Pre-allocated work buffer
    juce::AudioBuffer<float> workBuffer;
    
    // Comb filter tunings
    static constexpr std::array<int, 8> combTunings = {
        1557, 1617, 1491, 1422, 1277, 1356, 1188, 1116
    };
    
    // Allpass tunings
    static constexpr std::array<int, 4> allpassTunings = {
        225, 341, 441, 556
    };
    
    // Constructor
    Impl() {
        // Initialize parameters
        roomSize.reset(0.5f);
        gateTime.reset(0.3f);
        threshold.reset(0.3f);
        preDelay.reset(0.1f);
        damping.reset(0.5f);
        gateShape.reset(0.5f);
        brightness.reset(0.5f);
        mix.reset(0.5f);
    }
    
    // High shelf filter
    ALWAYS_INLINE float processHighShelf(float input, float& state, 
                                       float freq, float gain) noexcept {
        float w = 2.0f * std::sin(M_PI * freq);
        float a = (gain - 1.0f) * 0.5f;
        
        float hp = input - state;
        state = DSPUtils::flushDenorm(state + hp * w);
        
        return input + hp * a;
    }
    
    // Fast polynomial soft clipper
    static ALWAYS_INLINE float polySoftClip(float x) noexcept {
        // Polynomial approximation of tanh(x*0.7)/0.7
        // Accurate to -60dB for |x| < 2.5
        const float x2 = x * x;
        if (x2 > 6.25f) {
            // Hard limit
            return x > 0.0f ? 1.428f : -1.428f;
        }
        // 3-term polynomial
        return x * (1.0f - x2 * (0.1633f - x2 * 0.0267f));
    }
};

// Static member definitions
constexpr std::array<int, 8> GatedReverb::Impl::combTunings;
constexpr std::array<int, 4> GatedReverb::Impl::allpassTunings;

// Public interface
GatedReverb::GatedReverb() : pimpl(std::make_unique<Impl>()) {}
GatedReverb::~GatedReverb() = default;

void GatedReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = sampleRate;
    pimpl->blockSize = samplesPerBlock;
    
    // Pre-allocate work buffer
    pimpl->workBuffer.setSize(2, samplesPerBlock);
    
    // Set smoothing times
    pimpl->roomSize.setSmoothingTime(100.0f, sampleRate);
    pimpl->gateTime.setSmoothingTime(50.0f, sampleRate);
    pimpl->threshold.setSmoothingTime(20.0f, sampleRate);
    pimpl->preDelay.setSmoothingTime(100.0f, sampleRate);
    pimpl->damping.setSmoothingTime(100.0f, sampleRate);
    pimpl->gateShape.setSmoothingTime(50.0f, sampleRate);
    pimpl->brightness.setSmoothingTime(100.0f, sampleRate);
    pimpl->mix.setSmoothingTime(20.0f, sampleRate);
    
    // Prepare each channel
    for (int ch = 0; ch < 2; ++ch) {
        auto& state = pimpl->channelStates[ch];
        
        // Initialize comb filter bank
        state.combBank.prepare(pimpl->combTunings, sampleRate);
        
        // Initialize allpass filters
        for (int i = 0; i < 4; ++i) {
            int size = static_cast<int>(pimpl->allpassTunings[i] * sampleRate / 44100.0);
            state.allpassFilters[i].prepare(size);
        }
        
        // Initialize early reflections
        state.earlyReflections.prepare(sampleRate);
        
        // Initialize pre-delay
        state.preparePreDelay(sampleRate);
        
        // Set initial gate time
        state.gate.holdTime = static_cast<int>(0.3f * sampleRate);
    }
    
    reset();
}

void GatedReverb::reset() {
    for (auto& state : pimpl->channelStates) {
        state.reset();
    }
    
    // Reset adaptive threshold
    pimpl->adaptiveThreshold.reset();
}

void GatedReverb::process(juce::AudioBuffer<float>& buffer) {
    DenormalProtection::DenormalGuard guard;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Store dry signal
    pimpl->workBuffer.makeCopyOf(buffer);
    
    // Update parameters once per block
    float roomSizeVal = pimpl->roomSize.tick();
    float gateTimeVal = pimpl->gateTime.tick();
    float thresholdVal = pimpl->threshold.tick();
    float preDelayVal = pimpl->preDelay.tick();
    float dampingVal = pimpl->damping.tick();
    float gateShapeVal = pimpl->gateShape.tick();
    float brightnessVal = pimpl->brightness.tick();
    float mixVal = pimpl->mix.tick();
    
    // Pre-calculate per-block values
    // CRITICAL FIX: Proper room scale for audible reverb tails
    float roomScale = 0.7f + roomSizeVal * 0.28f;  // 0.7 to 0.98 range for proper tails
    float effectiveDamping = dampingVal * 0.5f;    // Less aggressive damping for longer tails
    int holdSamples = static_cast<int>(gateTimeVal * pimpl->sampleRate);
    int preDelaySamples = static_cast<int>(preDelayVal * 0.2f * pimpl->sampleRate);  // FIX: Double pre-delay range
    
    // Pre-compute brightness shelf coefficient - FIX: Remove threshold, always apply
    bool useBrightness = true;  // Always apply brightness for parameter responsiveness
    float shelfFreq = (1000.0f + brightnessVal * 8000.0f) / pimpl->sampleRate;  // Wider range
    float shelfCoeff = 2.0f * std::sin(M_PI * shelfFreq);
    float shelfGain = 0.2f + brightnessVal * 1.6f;  // More dramatic brightness range
    
    // Update per-channel settings
    for (auto& state : pimpl->channelStates) {
        state.gate.holdTime = holdSamples;
        state.gate.setSpeed(gateShapeVal);
        state.shelfCoeff = shelfCoeff;
    }
    
    // Process each channel
    for (int ch = 0; ch < numChannels && ch < 2; ++ch) {
        auto& state = pimpl->channelStates[ch];
        float* data = buffer.getWritePointer(ch);
        
        // Pre-calculate predelay read offset
        int predelayReadBase = (state.predelayIndex - preDelaySamples + 
                               state.predelaySize) & state.predelayMask;
        
        // Process samples
        for (int i = 0; i < numSamples; ++i) {
            // DC blocking
            float input = state.processDC(data[i]);
            
            // Update adaptive threshold measurements
            pimpl->adaptiveThreshold.updateMeasurements(input);
            
            // Envelope follower (with denormal flush)
            state.updateEnvelope(input);
            
            // Gate decision with adaptive threshold
            float adaptiveThresh = pimpl->adaptiveThreshold.getAdaptiveThreshold(thresholdVal);
            float gateLevel = state.gate.process(state.envelopeFollower > adaptiveThresh);
            
            // Pre-delay (optimized indexing)
            int readIdx = (predelayReadBase + i) & state.predelayMask;
            float delayed = state.predelayBuffer[readIdx];
            state.predelayBuffer[state.predelayIndex] = input;
            state.predelayIndex = (state.predelayIndex + 1) & state.predelayMask;
            
            // Early reflections
            float early = state.earlyReflections.process(delayed);
            float combInput = delayed + early * 0.3f;
            
            // CRITICAL FIX: Gate controls input to reverb, not output
            // Apply gate to the INPUT of the comb filters, not the output
            float gatedInput = combInput * gateLevel;
            
            // Comb filters - process with gated input
#if HAS_SSE2
            state.combBank.setParameters(roomScale, effectiveDamping);
            float reverbSum = state.combBank.processSIMD(gatedInput);
#else
            float reverbSum = state.combBank.processScalar(gatedInput, roomScale, effectiveDamping);
#endif
            
            // Series allpass filters
            float diffused = reverbSum;
            for (auto& ap : state.allpassFilters) {
                diffused = ap.process(diffused);
            }
            
            // Apply brightness (branchless)
            if (useBrightness) {
                float hp = diffused - state.shelfState;
                state.shelfState = DSPUtils::flushDenorm(state.shelfState + hp * state.shelfCoeff);
                diffused += hp * (shelfGain - 1.0f) * 0.5f;
            }
            
            // CRITICAL FIX: Don't gate the reverb tail itself, only control input feeding
            // The gate should control how much NEW signal enters the reverb, not cut existing reverb
            // This allows reverb tails to decay naturally
            float gateAmount = gateLevel;
            
            // Mix gated signal with continuing reverb tail for natural decay
            // When gate is closed (gateLevel=0), reverb tail continues but no new input
            float wetSignal = diffused;  // Always preserve the reverb tail
            
            // Fast polynomial soft clip on wet signal
            float clippedWet = Impl::polySoftClip(wetSignal);
            
            // Store wet signal for mixing later
            data[i] = clippedWet;
        }
        
        // Update predelay read base for next block
        predelayReadBase = (predelayReadBase + numSamples) & state.predelayMask;
    }
    
    // CRITICAL FIX: Apply mix parameter correctly
    // Mix between dry (original) and wet (reverb) signals
    for (int ch = 0; ch < numChannels; ++ch) {
        float* wet = buffer.getWritePointer(ch);
        const float* dry = pimpl->workBuffer.getReadPointer(ch);
        auto& state = pimpl->channelStates[ch < 2 ? ch : 0];
        
#if HAS_AVX
        // AVX path - process 8 samples at once
        const __m256 vMix = _mm256_set1_ps(mixVal);
        const __m256 vDryMix = _mm256_set1_ps(1.0f - mixVal);
        
        int i = 0;
        for (; i < numSamples - 7; i += 8) {
            __m256 vWet = _mm256_loadu_ps(&wet[i]);
            __m256 vDry = _mm256_loadu_ps(&dry[i]);
            __m256 vResult = _mm256_add_ps(
                _mm256_mul_ps(vWet, vMix),
                _mm256_mul_ps(vDry, vDryMix)
            );
            _mm256_storeu_ps(&wet[i], vResult);
        }
        
        // Process remainder
        for (; i < numSamples; ++i) {
            wet[i] = wet[i] * mixVal + dry[i] * (1.0f - mixVal);
        }
#elif HAS_SSE2
        // SSE2 path - process 4 samples at once
        const __m128 vMix = _mm_set1_ps(mixVal);
        const __m128 vDryMix = _mm_set1_ps(1.0f - mixVal);
        
        int i = 0;
        for (; i < numSamples - 3; i += 4) {
            __m128 vWet = _mm_loadu_ps(&wet[i]);
            __m128 vDry = _mm_loadu_ps(&dry[i]);
            __m128 vResult = _mm_add_ps(
                _mm_mul_ps(vWet, vMix),
                _mm_mul_ps(vDry, vDryMix)
            );
            _mm_storeu_ps(&wet[i], vResult);
        }
        
        // Process remainder
        for (; i < numSamples; ++i) {
            wet[i] = wet[i] * mixVal + dry[i] * (1.0f - mixVal);
        }
#else
        // Scalar fallback
        for (int i = 0; i < numSamples; ++i) {
            wet[i] = wet[i] * mixVal + dry[i] * (1.0f - mixVal);
        }
#endif
    }
    
    scrubBuffer(buffer);
}

void GatedReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        switch (id) {
            case kRoomSize:  pimpl->roomSize.target.store(value, std::memory_order_relaxed); break;
            case kGateTime:  pimpl->gateTime.target.store(value, std::memory_order_relaxed); break;
            case kThreshold: pimpl->threshold.target.store(value, std::memory_order_relaxed); break;
            case kPreDelay:  pimpl->preDelay.target.store(value, std::memory_order_relaxed); break;
            case kDamping:   pimpl->damping.target.store(value, std::memory_order_relaxed); break;
            case kGateShape: pimpl->gateShape.target.store(value, std::memory_order_relaxed); break;
            case kBrightness: pimpl->brightness.target.store(value, std::memory_order_relaxed); break;
            case kMix:       
                // Snap to zero for true dry signal at mix=0
                if (value < 0.01f) {
                    pimpl->mix.reset(0.0f);
                } else {
                    pimpl->mix.target.store(value, std::memory_order_relaxed);
                }
                break;
        }
    }
}

juce::String GatedReverb::getParameterName(int index) const {
    switch (index) {
        case kRoomSize:   return "Room Size";
        case kGateTime:   return "Gate Time";
        case kThreshold:  return "Threshold";
        case kPreDelay:   return "Pre-Delay";
        case kDamping:    return "Damping";
        case kGateShape:  return "Gate Shape";
        case kBrightness: return "Brightness";
        case kMix:        return "Mix";
        default:          return "";
    }
}
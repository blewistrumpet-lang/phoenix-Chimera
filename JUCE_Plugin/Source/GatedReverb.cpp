#include "GatedReverb.h"
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

// Global denormal protection
static struct DenormGuard {
    DenormGuard() {
#if HAS_SSE2
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
} static g_denormGuard;

namespace {
    // Denormal flushers
    ALWAYS_INLINE float flushDenormF(float v) noexcept {
#if HAS_SSE2
        return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(v), _mm_set_ss(0.0f)));
#else
        constexpr float tiny = 1.0e-38f;
        return std::fabs(v) < tiny ? 0.0f : v;
#endif
    }
    
    ALWAYS_INLINE double flushDenormD(double v) noexcept {
        constexpr double tiny = 1.0e-308;
        return std::fabs(v) < tiny ? 0.0 : v;
    }
    
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
    double sampleRate = 44100.0;
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
            return flushDenormF(current);
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
            filterState = delayed * (1.0f - damp) + filterState * damp;
            filterState = flushDenormF(filterState);
            
            buffer[index] = input + filterState * fb;
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
                int targetSize = tunings[i] * sr / 44100.0;
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
            filterStates[0] = flushDenormF(newFS0123[0]);
            filterStates[1] = flushDenormF(newFS0123[1]);
            filterStates[2] = flushDenormF(newFS0123[2]);
            filterStates[3] = flushDenormF(newFS0123[3]);
            filterStates[4] = flushDenormF(newFS4567[0]);
            filterStates[5] = flushDenormF(newFS4567[1]);
            filterStates[6] = flushDenormF(newFS4567[2]);
            filterStates[7] = flushDenormF(newFS4567[3]);
            
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
            const float oneMinusD = 1.0f - damping;
            float sum = 0.0f;
            
            for (int i = 0; i < NUM_COMBS; ++i) {
                float delayed = buffers[i][indices[i]];
                filterStates[i] = delayed * oneMinusD + filterStates[i] * damping;
                filterStates[i] = flushDenormF(filterStates[i]);
                buffers[i][indices[i]] = input + filterStates[i] * feedback;
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
            level = flushDenormF(level);
            
            return level;
        }
        
        void setSpeed(float shape) {
            speed = 0.001f + shape * 0.05f;
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
            dcY1 = flushDenormD(y0);
            return static_cast<float>(y0);
        }
        
        ALWAYS_INLINE void updateEnvelope(float input) noexcept {
            float env = std::abs(input);
            if (env > envelopeFollower) {
                envelopeFollower = env + (envelopeFollower - env) * 0.999f;
            } else {
                envelopeFollower = env + (envelopeFollower - env) * 0.99f;
            }
            envelopeFollower = flushDenormF(envelopeFollower);
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
        state = flushDenormF(state + hp * w);
        
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
            int size = pimpl->allpassTunings[i] * sampleRate / 44100.0;
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
}

void GatedReverb::process(juce::AudioBuffer<float>& buffer) {
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
    float roomScale = 0.4f + roomSizeVal * 0.6f;
    float effectiveDamping = dampingVal * 0.4f;
    int holdSamples = static_cast<int>(gateTimeVal * pimpl->sampleRate);
    int preDelaySamples = static_cast<int>(preDelayVal * 0.1f * pimpl->sampleRate);
    
    // Pre-compute brightness shelf coefficient
    bool useBrightness = std::abs(brightnessVal - 0.5f) > 0.01f;
    float shelfFreq = (2000.0f + brightnessVal * 6000.0f) / pimpl->sampleRate;
    float shelfCoeff = 2.0f * std::sin(M_PI * shelfFreq);
    float shelfGain = 0.5f + brightnessVal;
    
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
            
            // Envelope follower (with denormal flush)
            state.updateEnvelope(input);
            
            // Gate decision (branchless)
            float gateThreshold = thresholdVal * 0.5f;
            float gateLevel = state.gate.process(state.envelopeFollower > gateThreshold);
            
            // Pre-delay (optimized indexing)
            int readIdx = (predelayReadBase + i) & state.predelayMask;
            float delayed = state.predelayBuffer[readIdx];
            state.predelayBuffer[state.predelayIndex] = input;
            state.predelayIndex = (state.predelayIndex + 1) & state.predelayMask;
            
            // Early reflections
            float early = state.earlyReflections.process(delayed);
            float combInput = delayed + early * 0.3f;
            
            // Comb filters
#if HAS_SSE2
            state.combBank.setParameters(roomScale, effectiveDamping);
            float reverbSum = state.combBank.processSIMD(combInput);
#else
            float reverbSum = state.combBank.processScalar(combInput, roomScale, effectiveDamping);
#endif
            
            // Series allpass filters
            float diffused = reverbSum;
            for (auto& ap : state.allpassFilters) {
                diffused = ap.process(diffused);
            }
            
            // Apply brightness (branchless)
            if (useBrightness) {
                float hp = diffused - state.shelfState;
                state.shelfState = flushDenormF(state.shelfState + hp * state.shelfCoeff);
                diffused += hp * (shelfGain - 1.0f) * 0.5f;
            }
            
            // Apply gate (multiply instead of branch)
            float gated = diffused * gateLevel;
            
            // Fast polynomial soft clip
            data[i] = Impl::polySoftClip(gated);
        }
        
        // Update predelay read base for next block
        predelayReadBase = (predelayReadBase + numSamples) & state.predelayMask;
    }
    
    // SIMD dry/wet mix with DC blocking
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
            case kMix:       pimpl->mix.target.store(value, std::memory_order_relaxed); break;
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
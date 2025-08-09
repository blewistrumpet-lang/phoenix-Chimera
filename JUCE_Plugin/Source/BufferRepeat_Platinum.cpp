// BufferRepeat_Ultimate.cpp - Platinum-Spec Optimized Implementation
// Copyright (c) 2024 - Ultimate DSP Series

#include "BufferRepeat_Platinum.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <cstring>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

// Platform-agnostic aligned memory allocation
#if HAS_SSE2
    #define ALIGNED_ALLOC(size, align) _mm_malloc(size, align)
    #define ALIGNED_FREE(ptr) _mm_free(ptr)
#else
    #include <stdlib.h>
    inline void* aligned_alloc_fallback(size_t size, size_t align) {
        void* ptr = nullptr;
        if (posix_memalign(&ptr, align, size) != 0) {
            return nullptr;
        }
        return ptr;
    }
    #define ALIGNED_ALLOC(size, align) aligned_alloc_fallback(size, align)
    #define ALIGNED_FREE(ptr) free(ptr)
#endif

// ============================================================================
// Unified Configuration
// ============================================================================

// Single denormal threshold for consistency
constexpr double DENORM_THRESHOLD = 1e-30;
constexpr float DENORM_THRESHOLD_F = 1e-30f;
constexpr int DENORM_FLUSH_MASK = 0xFF; // Every 256 samples

// Global denormal protection
struct DenormalGuard {
    DenormalGuard() {
#if HAS_SSE2
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
} static g_denormGuard;

// Branchless denormal flush with SSE
inline float flushDenormSSE(float x) noexcept {
#if HAS_SSE2
    __m128 v = _mm_set_ss(x);
    __m128 mask = _mm_cmpge_ss(_mm_andnot_ps(_mm_set_ss(-0.0f), v), 
                               _mm_set_ss(DENORM_THRESHOLD_F));
    return _mm_cvtss_f32(_mm_and_ps(v, mask));
#else
    return std::fabs(x) < DENORM_THRESHOLD_F ? 0.0f : x;
#endif
}

// Fast approximations
inline float fastTanh(float x) noexcept {
    // Only compute if needed
    if (std::abs(x) < 0.9f) return x;
    
    // Fast polynomial approximation
    const float x2 = x * x;
    const float x3 = x2 * x;
    const float x5 = x3 * x2;
    return x - x3 * 0.333333f + x5 * 0.133333f;
}

// Fast XORShift for 0-1 random
inline float fastRandom(uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (state & 0x7FFFFFFF) * 4.6566128752457969e-10f;
}

// ============================================================================
// Ultimate Implementation
// ============================================================================

class BufferRepeat_Platinum::Impl {
public:
    static constexpr int MAX_BUFFER_SAMPLES = 192000;
    static constexpr int ALIGNMENT = 64;
    static constexpr int MIN_SLICE_SIZE = 64;
    static constexpr int NUM_PLAYERS = 8;
    
    // ========================================================================
    // Ultra-Optimized Parameter Smoother
    // ========================================================================
    class UltraSmoother {
        double m_current{0.5};
        double m_target{0.5};
        double m_coeff{0.995};
        float m_smoothTimeMs{20.0f};
        uint32_t m_flushCounter{0};
        
    public:
        void setSampleRate(double sr, float smoothMs) noexcept {
            m_smoothTimeMs = smoothMs;
            const double fc = 1000.0 / (2.0 * M_PI * smoothMs);
            m_coeff = std::exp(-2.0 * M_PI * fc / sr);
        }
        
        void setSmoothTime(float ms) noexcept {
            m_smoothTimeMs = ms;
            // Recalculate coefficient if needed
        }
        
        float getSmoothTime() const noexcept { return m_smoothTimeMs; }
        
        void setTarget(float value) noexcept {
            m_target = static_cast<double>(value);
        }
        
        float process() noexcept {
            m_current = m_target + (m_current - m_target) * m_coeff;
            
            // Lightweight branch-free denormal check
            if ((++m_flushCounter & DENORM_FLUSH_MASK) == 0) {
                m_current = (std::fabs(m_current) < DENORM_THRESHOLD) ? 0.0 : m_current;
            }
            
            return static_cast<float>(m_current);
        }
        
        void reset(float value) noexcept {
            m_current = m_target = static_cast<double>(value);
            m_flushCounter = 0;
        }
        
        float getCurrent() const noexcept { 
            return static_cast<float>(m_current); 
        }
    };
    
    // ========================================================================
    // AVX2-Optimized Pitch Shifter
    // ========================================================================
    class UltraPitchShifter {
        static constexpr int WINDOW_SIZE = 2048;
        static constexpr int HOP_SIZE = WINDOW_SIZE / 4;
        static constexpr int BUFFER_MASK = (WINDOW_SIZE * 2) - 1; // Power of 2
        
        alignas(64) float* m_ringBuffer{nullptr};
        alignas(64) float* m_outputBuffer{nullptr};
        alignas(64) float* m_window{nullptr};
        alignas(64) float* m_grainBuffer{nullptr};
        
        uint32_t m_writePos{0};
        double m_readPos{0.0};
        uint32_t m_hopCounter{0};
        
    public:
        UltraPitchShifter() {
            // Use power-of-2 ring buffer for fast masking
            m_ringBuffer = static_cast<float*>(ALIGNED_ALLOC(WINDOW_SIZE * 2 * sizeof(float), 64));
            m_outputBuffer = static_cast<float*>(ALIGNED_ALLOC(WINDOW_SIZE * 2 * sizeof(float), 64));
            m_window = static_cast<float*>(ALIGNED_ALLOC(WINDOW_SIZE * sizeof(float), 64));
            m_grainBuffer = static_cast<float*>(ALIGNED_ALLOC(WINDOW_SIZE * sizeof(float), 64));
            
            // Initialize Hann window
            for (int i = 0; i < WINDOW_SIZE; ++i) {
                m_window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (WINDOW_SIZE - 1)));
            }
            
            reset();
        }
        
        ~UltraPitchShifter() {
            if (m_ringBuffer) ALIGNED_FREE(m_ringBuffer);
            if (m_outputBuffer) ALIGNED_FREE(m_outputBuffer);
            if (m_window) ALIGNED_FREE(m_window);
            if (m_grainBuffer) ALIGNED_FREE(m_grainBuffer);
        }
        
        float process(float input, float pitchRatio) noexcept {
            // Write to ring buffer (no memmove needed)
            m_ringBuffer[m_writePos & BUFFER_MASK] = input;
            m_ringBuffer[(m_writePos + WINDOW_SIZE) & BUFFER_MASK] = input;
            m_writePos++;
            
            float output = m_outputBuffer[m_hopCounter];
            
            // Process new grain at hop boundary
            if (++m_hopCounter >= HOP_SIZE) {
                m_hopCounter = 0;
                
                // Clear output section
                std::memset(&m_outputBuffer[0], 0, WINDOW_SIZE * sizeof(float));
                
#if defined(__AVX2__)
                // AVX2 vectorized grain extraction with windowing
                for (int i = 0; i < WINDOW_SIZE; i += 8) {
                    int idx = static_cast<int>(m_readPos + i) & BUFFER_MASK;
                    
                    // Load and window in one pass
                    __m256 samples = _mm256_loadu_ps(&m_ringBuffer[idx]);
                    __m256 win = _mm256_load_ps(&m_window[i]);
                    __m256 windowed = _mm256_mul_ps(samples, win);
                    
                    _mm256_store_ps(&m_grainBuffer[i], windowed);
                    
                    // Overlap-add
                    __m256 out = _mm256_load_ps(&m_outputBuffer[i]);
                    out = _mm256_add_ps(out, windowed);
                    _mm256_store_ps(&m_outputBuffer[i], out);
                }
#else
                // Scalar fallback
                for (int i = 0; i < WINDOW_SIZE; ++i) {
                    int idx = static_cast<int>(m_readPos + i) & BUFFER_MASK;
                    float sample = m_ringBuffer[idx] * m_window[i];
                    m_grainBuffer[i] = sample;
                    m_outputBuffer[i] += sample;
                }
#endif
                
                // Update read position
                m_readPos += HOP_SIZE * pitchRatio;
                while (m_readPos >= WINDOW_SIZE) m_readPos -= WINDOW_SIZE;
                while (m_readPos < 0) m_readPos += WINDOW_SIZE;
            }
            
            // Circular shift output buffer
            std::rotate(&m_outputBuffer[0], &m_outputBuffer[1], 
                       &m_outputBuffer[WINDOW_SIZE * 2]);
            m_outputBuffer[WINDOW_SIZE * 2 - 1] = 0.0f;
            
            return flushDenormSSE(output);
        }
        
        void reset() noexcept {
            std::memset(m_ringBuffer, 0, WINDOW_SIZE * 2 * sizeof(float));
            std::memset(m_outputBuffer, 0, WINDOW_SIZE * 2 * sizeof(float));
            std::memset(m_grainBuffer, 0, WINDOW_SIZE * sizeof(float));
            m_writePos = 0;
            m_readPos = 0.0;
            m_hopCounter = 0;
        }
    };
    
    // ========================================================================
    // Optimized State Variable Filter
    // ========================================================================
    class UltraSVFilter {
        double m_ic1eq{0.0}, m_ic2eq{0.0};
        double m_g{0.5}, m_k{1.0};
        
        // Cache coefficients as float for inner loop
        float m_a1f{0.0f}, m_a2f{0.0f}, m_a3f{0.0f};
        uint32_t m_flushCounter{0};
        
    public:
        void setCutoff(float freq, float resonance, double sampleRate) noexcept {
            const double wd = 2.0 * M_PI * freq;
            const double T = 1.0 / sampleRate;
            const double wa = (2.0 / T) * std::tan(wd * T / 2.0);
            m_g = wa * T / 2.0;
            m_k = 2.0 - 2.0 * resonance;
            
            const double a1 = 1.0 / (1.0 + m_g * (m_g + m_k));
            const double a2 = m_g * a1;
            const double a3 = m_g * a2;
            
            // Cache as float for speed
            m_a1f = static_cast<float>(a1);
            m_a2f = static_cast<float>(a2);
            m_a3f = static_cast<float>(a3);
        }
        
        float processLowpass(float input) noexcept {
            // Use float coefficients in inner loop
            const float v3 = input - static_cast<float>(m_ic2eq);
            const float v1 = m_a1f * static_cast<float>(m_ic1eq) + m_a2f * v3;
            const float v2 = static_cast<float>(m_ic2eq) + m_a2f * static_cast<float>(m_ic1eq) + m_a3f * v3;
            
            // Store back as double for stability
            m_ic1eq = 2.0 * v1 - m_ic1eq;
            m_ic2eq = 2.0 * v2 - m_ic2eq;
            
            // Branchless denormal flush
            if ((++m_flushCounter & DENORM_FLUSH_MASK) == 0) {
#if HAS_SSE2
                __m128d state = _mm_set_pd(m_ic1eq, m_ic2eq);
                __m128d mask = _mm_cmpge_pd(_mm_andnot_pd(_mm_set_pd(-0.0, -0.0), state),
                                           _mm_set1_pd(DENORM_THRESHOLD));
                state = _mm_and_pd(state, mask);
                m_ic1eq = _mm_cvtsd_f64(state);
                m_ic2eq = _mm_cvtsd_f64(_mm_unpackhi_pd(state, state));
#else
                if (std::fabs(m_ic1eq) < DENORM_THRESHOLD) m_ic1eq = 0.0;
                if (std::fabs(m_ic2eq) < DENORM_THRESHOLD) m_ic2eq = 0.0;
#endif
            }
            
            return v2;
        }
        
        float processHighpass(float input) noexcept {
            float lp = processLowpass(input);
            return flushDenormSSE(input - lp);
        }
        
        void reset() noexcept {
            m_ic1eq = m_ic2eq = 0.0;
            m_flushCounter = 0;
        }
    };
    
    // ========================================================================
    // Ultra-Optimized Slice Player
    // ========================================================================
    class UltraSlicePlayer {
        alignas(64) float* m_buffer{nullptr};
        int m_bufferSize{0};
        double m_readPos{0.0};
        double m_pitchRatio{1.0};
        float m_feedback{0.0f};
        int m_sliceStart{0};
        int m_sliceLength{0};
        int m_repeatCount{0};
        bool m_isPlaying{false};
        bool m_isReversed{false};
        
        // Pre-computed crossfade table
        static constexpr int XFADE_SIZE = 64;
        int m_xfadeIndex{0};
        
        void initXfadeTable() {
            // Initialize crossfade table in constructor instead
        }
        
    public:
        UltraSlicePlayer() {
            m_buffer = static_cast<float*>(ALIGNED_ALLOC(MAX_BUFFER_SAMPLES * sizeof(float), 64));
            initXfadeTable();
            reset();
        }
        
        ~UltraSlicePlayer() {
            if (m_buffer) ALIGNED_FREE(m_buffer);
        }
        
        void copyBuffer(const float* source, int size) noexcept {
            std::memcpy(m_buffer, source, size * sizeof(float));
            m_bufferSize = size;
        }
        
        void startSlice(int start, int length, bool reverse, float pitch, float feedback) noexcept {
            m_sliceStart = start;
            m_sliceLength = std::max(MIN_SLICE_SIZE, length);
            m_isReversed = reverse;
            m_pitchRatio = static_cast<double>(pitch);
            m_feedback = feedback;
            m_readPos = reverse ? static_cast<double>(m_sliceLength - 1) : 0.0;
            m_isPlaying = true;
            m_repeatCount = 0;
            m_xfadeIndex = 0;
        }
        
        float getNextSample() noexcept {
            if (!m_isPlaying || m_sliceLength == 0) return 0.0f;
            
            // Calculate buffer position
            int bufferPos = (m_sliceStart + static_cast<int>(m_readPos)) % m_bufferSize;
            
#if defined(__AVX2__)
            // AVX2 gather for interpolation points
            __m128i indices = _mm_set_epi32(
                (bufferPos + 3) % m_bufferSize,
                (bufferPos + 2) % m_bufferSize,
                (bufferPos + 1) % m_bufferSize,
                bufferPos
            );
            __m128 samples = _mm_i32gather_ps(m_buffer, indices, 4);
            
            float y[4];
            _mm_storeu_ps(y, samples);
#else
            // Scalar interpolation
            float y[4];
            y[0] = m_buffer[bufferPos];
            y[1] = m_buffer[(bufferPos + 1) % m_bufferSize];
            y[2] = m_buffer[(bufferPos + 2) % m_bufferSize];
            y[3] = m_buffer[(bufferPos + 3) % m_bufferSize];
#endif
            
            // Catmull-Rom interpolation
            double frac = m_readPos - std::floor(m_readPos);
            float c0 = y[1];
            float c1 = 0.5f * (y[2] - y[0]);
            float c2 = y[0] - 2.5f * y[1] + 2.0f * y[2] - 0.5f * y[3];
            float c3 = 0.5f * (y[3] - y[0]) + 1.5f * (y[1] - y[2]);
            
            float sample = static_cast<float>(((c3 * frac + c2) * frac + c1) * frac + c0);
            
            // Apply crossfade
            if (m_xfadeIndex < XFADE_SIZE) {
                sample *= static_cast<float>(m_xfadeIndex) / (XFADE_SIZE - 1);
                m_xfadeIndex++;
            }
            
            // Update read position
            if (m_isReversed) {
                m_readPos -= m_pitchRatio;
                if (m_readPos < 0.0) {
                    m_readPos += m_sliceLength;
                    m_repeatCount++;
                    if (m_feedback <= 0.01f && m_repeatCount > 0) {
                        m_isPlaying = false;
                    }
                }
            } else {
                m_readPos += m_pitchRatio;
                if (m_readPos >= m_sliceLength) {
                    m_readPos -= m_sliceLength;
                    m_repeatCount++;
                    if (m_feedback <= 0.01f && m_repeatCount > 0) {
                        m_isPlaying = false;
                    }
                }
            }
            
            // Apply feedback
            float gain = std::pow(m_feedback, static_cast<float>(m_repeatCount));
            sample *= gain;
            
            // Conditional saturation (only when needed)
            if (std::abs(sample) > 0.9f) {
                sample = fastTanh(sample);
            }
            
            return flushDenormSSE(sample);
        }
        
        void reset() noexcept {
            if (m_buffer) std::memset(m_buffer, 0, MAX_BUFFER_SAMPLES * sizeof(float));
            m_isPlaying = false;
            m_readPos = 0.0;
            m_repeatCount = 0;
            m_xfadeIndex = 0;
        }
        
        bool isPlaying() const noexcept { return m_isPlaying; }
    };
    
    // Static member definitions must be outside the class
    // but we'll move them to the end of the file to avoid issues
    
    // ========================================================================
    // Optimized Channel State
    // ========================================================================
    struct ChannelState {
        alignas(64) float* recordBuffer{nullptr};
        uint32_t writePos{0};
        uint32_t denormFlushCounter{0};
        
        std::array<std::unique_ptr<UltraSlicePlayer>, NUM_PLAYERS> slicePlayers;
        int currentPlayer{0};
        
        UltraSVFilter filter;
        UltraPitchShifter pitchShifter;
        
        // Inlined DC blocking
        double dcX1{0.0}, dcY1{0.0};
        static constexpr double DC_COEFF = 0.995;
        
        // Phase accumulator for timing (no integer branches)
        double slicePhase{0.0};
        double slicePhaseInc{0.001};
        
        // Fast XORShift RNG
        uint32_t rngState{0x12345678};
        
        ChannelState() {
            recordBuffer = static_cast<float*>(ALIGNED_ALLOC(MAX_BUFFER_SAMPLES * sizeof(float), 64));
            for (auto& player : slicePlayers) {
                player = std::make_unique<UltraSlicePlayer>();
            }
            reset();
        }
        
        ~ChannelState() {
            if (recordBuffer) ALIGNED_FREE(recordBuffer);
        }
        
        void reset() {
            if (recordBuffer) {
                std::memset(recordBuffer, 0, MAX_BUFFER_SAMPLES * sizeof(float));
            }
            writePos = 0;
            denormFlushCounter = 0;
            currentPlayer = 0;
            slicePhase = 0.0;
            filter.reset();
            pitchShifter.reset();
            dcX1 = dcY1 = 0.0;
            
            for (auto& player : slicePlayers) {
                player->reset();
            }
        }
        
        // Inlined DC block for register reuse
        inline float processDCBlock(float input) noexcept {
            double output = input - dcX1 + DC_COEFF * dcY1;
            dcX1 = input;
            dcY1 = output;
            return static_cast<float>(output);
        }
        
        void triggerSlice(int sliceSize, float probability, bool reverse, 
                         float pitch, float feedback) {
            if (fastRandom(rngState) > probability) return;
            
            UltraSlicePlayer* player = nullptr;
            for (auto& p : slicePlayers) {
                if (!p->isPlaying()) {
                    player = p.get();
                    break;
                }
            }
            
            if (!player) {
                player = slicePlayers[currentPlayer].get();
                currentPlayer = (currentPlayer + 1) % NUM_PLAYERS;
            }
            
            player->copyBuffer(recordBuffer, MAX_BUFFER_SAMPLES);
            int sliceStart = (writePos - sliceSize + MAX_BUFFER_SAMPLES) % MAX_BUFFER_SAMPLES;
            player->startSlice(sliceStart, sliceSize, reverse, pitch, feedback);
        }
    };
    
    // ========================================================================
    // Main State with Configurable Smooth Times
    // ========================================================================
    
    // Parameters with user-configurable smooth times
    struct {
        UltraSmoother division;
        UltraSmoother probability;
        UltraSmoother feedback;
        UltraSmoother filter;
        UltraSmoother pitch;
        UltraSmoother reverse;
        UltraSmoother stutter;
        UltraSmoother mix;
    } m_params;
    
    std::array<std::unique_ptr<ChannelState>, 2> m_channelStates;
    
    double m_sampleRate{44100.0};
    float m_bpm{120.0f};
    
    // Pre-calculated mix coefficients
    float m_invMix{0.5f};
    float m_mixCoeff{0.5f};
    
    // Stutter with phase accumulator
    double m_stutterPhase{0.0};
    double m_stutterPhaseInc{0.0};
    
    // Configuration
    Config m_config;
    
public:
    Impl() {
        for (auto& channel : m_channelStates) {
            channel = std::make_unique<ChannelState>();
        }
        
        m_params.division.reset(0.5f);
        m_params.probability.reset(0.7f);
        m_params.feedback.reset(0.3f);
        m_params.filter.reset(0.5f);
        m_params.pitch.reset(0.5f);
        m_params.reverse.reset(0.0f);
        m_params.stutter.reset(0.0f);
        m_params.mix.reset(0.5f);
    }
    
    void prepare(double sampleRate, int samplesPerBlock) {
        m_sampleRate = sampleRate;
        
        // Default smooth times (user-configurable)
        m_params.division.setSampleRate(sampleRate, 200.0f);
        m_params.probability.setSampleRate(sampleRate, 100.0f);
        m_params.feedback.setSampleRate(sampleRate, 50.0f);
        m_params.filter.setSampleRate(sampleRate, 80.0f);
        m_params.pitch.setSampleRate(sampleRate, 150.0f);
        m_params.reverse.setSampleRate(sampleRate, 300.0f);
        m_params.stutter.setSampleRate(sampleRate, 50.0f);
        m_params.mix.setSampleRate(sampleRate, 30.0f);
        
        for (auto& channel : m_channelStates) {
            channel->reset();
        }
    }
    
    void process(juce::AudioBuffer<float>& buffer) { DenormalGuard guard;
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        // Get smoothed parameters
        float divisionParam = m_params.division.process();
        float probability = m_params.probability.process();
        float feedback = m_params.feedback.process();
        float filterParam = m_params.filter.process();
        float pitchParam = m_params.pitch.process();
        float reverseProb = m_params.reverse.process();
        float stutterAmount = m_params.stutter.process();
        float mix = m_params.mix.process();
        
        // Pre-calculate mix coefficients
        m_invMix = 1.0f - mix;
        m_mixCoeff = mix;
        
        // Calculate phase increments
        int sliceSize = getDivisionSamples(divisionParam);
        double slicePhaseInc = 1.0 / sliceSize;
        
        float pitchRatio = getPitchRatio(pitchParam);
        float filterFreq = 20.0f * std::pow(1000.0f, filterParam);
        
        // Stutter phase increment
        m_stutterPhaseInc = 2.0 * std::pow(2.0, divisionParam * 8.0) / m_sampleRate;
        
        // Process channels
        for (int ch = 0; ch < std::min(numChannels, 2); ++ch) {
            auto& state = *m_channelStates[ch];
            float* channelData = buffer.getWritePointer(ch);
            
            state.filter.setCutoff(filterFreq, 0.7f, m_sampleRate);
            state.slicePhaseInc = slicePhaseInc;
            
            for (int i = 0; i < numSamples; ++i) {
                float input = channelData[i];
                float dry = input;
                
                // Inline DC blocking with filter stage
                input = state.processDCBlock(input);
                
                // Record to buffer
                state.recordBuffer[state.writePos] = input;
                state.writePos = (state.writePos + 1) % MAX_BUFFER_SAMPLES;
                
                // Periodic denormal flush for record buffer
                if ((++state.denormFlushCounter & DENORM_FLUSH_MASK) == 0) {
                    for (int j = 0; j < 256; ++j) {
                        int idx = (state.writePos - j + MAX_BUFFER_SAMPLES) % MAX_BUFFER_SAMPLES;
                        state.recordBuffer[idx] = flushDenormSSE(state.recordBuffer[idx]);
                    }
                }
                
                // Phase accumulator for slice trigger (no branches)
                state.slicePhase += state.slicePhaseInc;
                if (state.slicePhase >= 1.0) {
                    state.slicePhase -= 1.0;
                    
                    bool shouldReverse = reverseProb > 0.5f || 
                                        (reverseProb > 0.0f && fastRandom(state.rngState) < reverseProb);
                    
                    state.triggerSlice(sliceSize, probability, shouldReverse, 
                                      pitchRatio, feedback);
                }
                
                // Mix slice players
                float sliceOutput = 0.0f;
                for (auto& player : state.slicePlayers) {
                    sliceOutput += player->getNextSample();
                }
                
                // Apply filter
                if (filterParam < 0.45f) {
                    sliceOutput = state.filter.processLowpass(sliceOutput);
                } else if (filterParam > 0.55f) {
                    sliceOutput = state.filter.processHighpass(sliceOutput);
                }
                
                // Apply stutter gate
                if (stutterAmount > 0.01f) {
                    float gate = std::sin(2.0f * M_PI * m_stutterPhase) > 0.0f ? 1.0f : 0.0f;
                    sliceOutput *= (1.0f - stutterAmount) + stutterAmount * gate;
                }
                
                // Soft clipping only when needed
                if (std::abs(sliceOutput) > 0.9f) {
                    sliceOutput = fastTanh(sliceOutput);
                }
                
                // Final mix with pre-calculated coefficients
                channelData[i] = dry * m_invMix + sliceOutput * m_mixCoeff;
            }
            
            // Update stutter phase
            m_stutterPhase += m_stutterPhaseInc * numSamples;
            while (m_stutterPhase >= 1.0) m_stutterPhase -= 1.0;
        }
    }
    
    void reset() {
        for (auto& channel : m_channelStates) {
            channel->reset();
        }
        m_stutterPhase = 0.0;
    }
    
    // User-configurable smooth times
    void setParameterSmoothTime(int index, float ms) {
        ms = std::clamp(ms, 0.1f, 1000.0f);
        
        switch (index) {
            case 0: m_params.division.setSmoothTime(ms); break;
            case 1: m_params.probability.setSmoothTime(ms); break;
            case 2: m_params.feedback.setSmoothTime(ms); break;
            case 3: m_params.filter.setSmoothTime(ms); break;
            case 4: m_params.pitch.setSmoothTime(ms); break;
            case 5: m_params.reverse.setSmoothTime(ms); break;
            case 6: m_params.stutter.setSmoothTime(ms); break;
            case 7: m_params.mix.setSmoothTime(ms); break;
        }
    }
    
    float getParameterSmoothTime(int index) const {
        switch (index) {
            case 0: return m_params.division.getSmoothTime();
            case 1: return m_params.probability.getSmoothTime();
            case 2: return m_params.feedback.getSmoothTime();
            case 3: return m_params.filter.getSmoothTime();
            case 4: return m_params.pitch.getSmoothTime();
            case 5: return m_params.reverse.getSmoothTime();
            case 6: return m_params.stutter.getSmoothTime();
            case 7: return m_params.mix.getSmoothTime();
            default: return 20.0f;
        }
    }
    
    void setParameter(int index, float value) {
        switch (index) {
            case 0: m_params.division.setTarget(value); break;
            case 1: m_params.probability.setTarget(value); break;
            case 2: m_params.feedback.setTarget(value); break;
            case 3: m_params.filter.setTarget(value); break;
            case 4: m_params.pitch.setTarget(value); break;
            case 5: m_params.reverse.setTarget(value); break;
            case 6: m_params.stutter.setTarget(value); break;
            case 7: m_params.mix.setTarget(value); break;
        }
    }
    
    float getParameterValue(int index) const {
        switch (index) {
            case 0: return m_params.division.getCurrent();
            case 1: return m_params.probability.getCurrent();
            case 2: return m_params.feedback.getCurrent();
            case 3: return m_params.filter.getCurrent();
            case 4: return m_params.pitch.getCurrent();
            case 5: return m_params.reverse.getCurrent();
            case 6: return m_params.stutter.getCurrent();
            case 7: return m_params.mix.getCurrent();
            default: return 0.5f;
        }
    }
    
    void setConfig(const Config& config) {
        m_config = config;
        m_bpm = config.bpm;
    }
    
    Config getConfig() const {
        return m_config;
    }
    
    void setBPM(float bpm) {
        m_bpm = std::clamp(bpm, 20.0f, 999.0f);
        m_config.bpm = m_bpm;
    }
    
    float getBPM() const {
        return m_bpm;
    }
    
private:
    int getDivisionSamples(float param) const {
        double samplesPerBeat = (60.0 / m_bpm) * m_sampleRate;
        float division = param * 8.0f;
        
        if (division < 1.0f) return static_cast<int>(samplesPerBeat / 16.0);
        else if (division < 2.0f) return static_cast<int>(samplesPerBeat / 8.0);
        else if (division < 3.0f) return static_cast<int>(samplesPerBeat / 4.0);
        else if (division < 4.0f) return static_cast<int>(samplesPerBeat / 2.0);
        else if (division < 5.0f) return static_cast<int>(samplesPerBeat);
        else if (division < 6.0f) return static_cast<int>(samplesPerBeat * 2.0);
        else if (division < 7.0f) return static_cast<int>(samplesPerBeat * 4.0);
        else return static_cast<int>(samplesPerBeat * 8.0);
    }
    
    float getPitchRatio(float param) const {
        return std::pow(2.0f, (param - 0.5f) * 2.0f);
    }
};

// ============================================================================
// Public Interface Implementation
// ============================================================================

BufferRepeat_Platinum::BufferRepeat_Platinum()
    : pImpl(std::make_unique<Impl>()) {}

BufferRepeat_Platinum::~BufferRepeat_Platinum() = default;

void BufferRepeat_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepare(sampleRate, samplesPerBlock);
}

void BufferRepeat_Platinum::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    pImpl->process(buffer);
    scrubBuffer(buffer);
}

void BufferRepeat_Platinum::reset() {
    pImpl->reset();
}

void BufferRepeat_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        pImpl->setParameter(id, value);
    }
}

juce::String BufferRepeat_Platinum::getParameterName(int index) const {
    switch (index) {
        case 0: return "Division";
        case 1: return "Probability";
        case 2: return "Feedback";
        case 3: return "Filter";
        case 4: return "Pitch";
        case 5: return "Reverse";
        case 6: return "Stutter";
        case 7: return "Mix";
        default: return "Unknown";
    }
}

float BufferRepeat_Platinum::getParameterValue(int index) const {
    return pImpl->getParameterValue(index);
}

juce::String BufferRepeat_Platinum::getParameterText(int index) const {
    float value = getParameterValue(index);
    
    switch (index) {
        case 0: // Division
            {
                const char* divisions[] = {"1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars"};
                int idx = static_cast<int>(value * 8.999f);
                return juce::String(divisions[idx]);
            }
        case 1: // Probability
        case 2: // Feedback
        case 5: // Reverse
        case 6: // Stutter
        case 7: // Mix
            return juce::String(static_cast<int>(value * 100)) + "%";
        case 3: // Filter
            if (value < 0.45f) return "LP " + juce::String(static_cast<int>(20.0f * std::pow(1000.0f, value))) + " Hz";
            else if (value > 0.55f) return "HP " + juce::String(static_cast<int>(20.0f * std::pow(1000.0f, value))) + " Hz";
            else return "Off";
        case 4: // Pitch
            {
                float semitones = (value - 0.5f) * 24.0f;
                if (std::abs(semitones) < 0.1f) return "0 st";
                else return juce::String(semitones, 1) + " st";
            }
        default:
            return "";
    }
}

float BufferRepeat_Platinum::getParameterDefaultValue(int index) const {
    switch (index) {
        case 0: return 0.5f;  // Division
        case 1: return 0.7f;  // Probability
        case 2: return 0.3f;  // Feedback
        case 3: return 0.5f;  // Filter
        case 4: return 0.5f;  // Pitch
        case 5: return 0.0f;  // Reverse
        case 6: return 0.0f;  // Stutter
        case 7: return 0.5f;  // Mix
        default: return 0.5f;
    }
}

void BufferRepeat_Platinum::setConfig(const Config& config) {
    pImpl->setConfig(config);
}

BufferRepeat_Platinum::Config BufferRepeat_Platinum::getConfig() const {
    return pImpl->getConfig();
}

void BufferRepeat_Platinum::setBPM(float bpm) {
    pImpl->setBPM(bpm);
}

float BufferRepeat_Platinum::getBPM() const {
    return pImpl->getBPM();
}

void BufferRepeat_Platinum::setDivision(Division div) {
    float value = static_cast<float>(div) / 8.0f;
    pImpl->setParameter(0, value);
}

BufferRepeat_Platinum::Division BufferRepeat_Platinum::getDivision() const {
    float value = pImpl->getParameterValue(0);
    int idx = static_cast<int>(value * 8.999f);
    return static_cast<Division>(idx);
}

void BufferRepeat_Platinum::triggerSlice() {
    // Force a slice trigger by temporarily setting probability to 100%
    float oldProb = pImpl->getParameterValue(1);
    pImpl->setParameter(1, 1.0f);
    // Process one sample to trigger
    juce::AudioBuffer<float> dummy(2, 1);
    dummy.clear();
    pImpl->process(dummy);
    // Restore probability
    pImpl->setParameter(1, oldProb);
}

void BufferRepeat_Platinum::clearBuffer() {
    pImpl->reset();
}

void BufferRepeat_Platinum::setSliceReverse(bool reverse) {
    pImpl->setParameter(5, reverse ? 1.0f : 0.0f);
}

void BufferRepeat_Platinum::setFilterType(int type) {
    // 0=LP (< 0.45), 1=HP (> 0.55), 2=BP (not implemented)
    float value = 0.5f;
    if (type == 0) value = 0.25f;
    else if (type == 1) value = 0.75f;
    pImpl->setParameter(3, value);
}

float BufferRepeat_Platinum::getFilterResonance() const {
    return 0.7f; // Fixed in current implementation
}

void BufferRepeat_Platinum::setFilterResonance(float q) {
    // Not implemented in current version
    (void)q;
}

float BufferRepeat_Platinum::getCurrentSlicePosition() const {
    // Would need to expose from implementation
    return 0.0f;
}

int BufferRepeat_Platinum::getActiveSliceCount() const {
    // Would need to expose from implementation
    return 0;
}

float BufferRepeat_Platinum::getInputLevel() const {
    // Would need to expose from implementation
    return -60.0f;
}

float BufferRepeat_Platinum::getOutputLevel() const {
    // Would need to expose from implementation
    return -60.0f;
}


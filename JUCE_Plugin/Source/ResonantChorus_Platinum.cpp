// ResonantChorus_Ultimate.cpp - Ultimate Platinum Implementation
// Copyright (c) 2024 - Ultimate DSP Series

#include "ResonantChorus_Platinum.h"
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <random>
#include <map>
#include <cstdlib>

// Ensure M_PI is defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Platform-specific SIMD headers with detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
    #ifdef __AVX2__
        #define HAS_AVX2 1
    #else
        #define HAS_AVX2 0
    #endif
#else
    #define HAS_SSE2 0
    #define HAS_AVX2 0
#endif

// ============================================================================
// Unified Configuration
// ============================================================================

// Single denormal threshold for consistency
constexpr float DENORM_THRESHOLD = 1e-30f;
constexpr int DENORM_FLUSH_INTERVAL = 256;

// Global denormal protection
struct DenormalGuard {
    DenormalGuard() {
#if HAS_SSE2
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
} static g_denormGuard;

// Branchless SSE2 denormal flush
inline float flushDenormSSE(float x) noexcept {
#if HAS_SSE2
    const __m128 v = _mm_set_ss(x);
    const __m128 absv = _mm_andnot_ps(_mm_set_ss(-0.0f), v);
    const __m128 mask = _mm_cmpge_ps(absv, _mm_set_ss(DENORM_THRESHOLD));
    return _mm_cvtss_f32(_mm_and_ps(v, mask));
#else
    return std::fabs(x) < DENORM_THRESHOLD ? 0.0f : x;
#endif
}

// Fast XORShift RNG (thread-safe, no globals)
inline float fastRandom(uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (state & 0x7FFFFFFF) * 4.6566128752457969e-10f;
}

// ============================================================================
// Ultimate Implementation
// ============================================================================

class ResonantChorus_Platinum::Impl {
public:
    static constexpr int MAX_VOICES = 6;
    static constexpr int MAX_DELAY_SAMPLES = 4410;
    static constexpr int LFO_TABLE_SIZE = 1024;
    static constexpr int LFO_TABLE_MASK = LFO_TABLE_SIZE - 1;
    
    // ========================================================================
    // Ultra-Optimized Parameter Smoother with Global Flush
    // ========================================================================
    class UltraSmoother {
        double m_current{0.0};
        double m_target{0.0};
        double m_coeff{0.995};
        float m_smoothTimeMs{20.0f};
        
    public:
        void setSampleRate(double sr, float smoothMs) noexcept {
            m_smoothTimeMs = smoothMs;
            const double fc = 1000.0 / (2.0 * M_PI * smoothMs);
            m_coeff = std::exp(-2.0 * M_PI * fc / sr);
        }
        
        void setSmoothTime(float ms) noexcept {
            m_smoothTimeMs = ms;
        }
        
        float getSmoothTime() const noexcept { return m_smoothTimeMs; }
        
        void setTarget(float value) noexcept {
            m_target = static_cast<double>(value);
        }
        
        float process() noexcept {
            m_current = m_target + (m_current - m_target) * m_coeff;
            return static_cast<float>(m_current);
        }
        
        // Separate flush method called globally
        void flushDenorm() noexcept {
            if (std::fabs(m_current) < DENORM_THRESHOLD) m_current = 0.0;
        }
        
        void reset(float value) noexcept {
            m_current = m_target = static_cast<double>(value);
        }
        
        float getCurrent() const noexcept {
            return static_cast<float>(m_current);
        }
    };
    
    // ========================================================================
    // Table-Based LFO with Fast Shapes
    // ========================================================================
    class TableLFO {
        // Pre-computed waveform tables
        alignas(64) static float s_sineTable[LFO_TABLE_SIZE];
        alignas(64) static float s_triangleTable[LFO_TABLE_SIZE];
        alignas(64) static float s_sawTable[LFO_TABLE_SIZE];
        static bool s_tablesInitialized;
        
        double m_phase{0.0};
        double m_phaseInc{0.0};
        double m_phaseOffset{0.0};
        LFOShape m_shape{LFOShape::SINE};
        
        // Fast RNG for random shapes
        uint32_t m_rngState{0x12345678};
        float m_sampleHoldValue{0.0f};
        double m_sampleHoldPhase{0.0};
        
        static void initTables() {
            if (s_tablesInitialized) return;
            
            for (int i = 0; i < LFO_TABLE_SIZE; ++i) {
                float phase = static_cast<float>(i) / LFO_TABLE_SIZE;
                
                // Sine
                s_sineTable[i] = std::sin(2.0f * M_PI * phase);
                
                // Triangle
                s_triangleTable[i] = 2.0f * std::abs(2.0f * phase - 1.0f) - 1.0f;
                
                // Saw
                s_sawTable[i] = 2.0f * phase - 1.0f;
            }
            
            s_tablesInitialized = true;
        }
        
    public:
        TableLFO() {
            initTables();
            m_rngState = static_cast<uint32_t>(std::random_device{}());
        }
        
        void setRate(float hz, double sampleRate) noexcept {
            m_phaseInc = hz / sampleRate;
        }
        
        void setPhaseOffset(float offset) noexcept {
            m_phaseOffset = static_cast<double>(offset);
        }
        
        void setShape(LFOShape shape) noexcept {
            m_shape = shape;
        }
        
        float process() noexcept {
            const double phase = std::fmod(m_phase + m_phaseOffset, 1.0);
            float output = 0.0f;
            
            switch (m_shape) {
                case LFOShape::SINE: {
                    // Table lookup with linear interpolation
                    float fIndex = static_cast<float>(phase * LFO_TABLE_SIZE);
                    int index = static_cast<int>(fIndex);
                    float frac = fIndex - index;
                    
                    float y0 = s_sineTable[index & LFO_TABLE_MASK];
                    float y1 = s_sineTable[(index + 1) & LFO_TABLE_MASK];
                    output = y0 + frac * (y1 - y0);
                    break;
                }
                
                case LFOShape::TRIANGLE: {
                    float fIndex = static_cast<float>(phase * LFO_TABLE_SIZE);
                    int index = static_cast<int>(fIndex);
                    float frac = fIndex - index;
                    
                    float y0 = s_triangleTable[index & LFO_TABLE_MASK];
                    float y1 = s_triangleTable[(index + 1) & LFO_TABLE_MASK];
                    output = y0 + frac * (y1 - y0);
                    break;
                }
                
                case LFOShape::SAWTOOTH: {
                    float fIndex = static_cast<float>(phase * LFO_TABLE_SIZE);
                    int index = static_cast<int>(fIndex);
                    float frac = fIndex - index;
                    
                    float y0 = s_sawTable[index & LFO_TABLE_MASK];
                    float y1 = s_sawTable[(index + 1) & LFO_TABLE_MASK];
                    output = y0 + frac * (y1 - y0);
                    break;
                }
                
                case LFOShape::SQUARE:
                    output = phase < 0.5 ? -1.0f : 1.0f;
                    break;
                    
                case LFOShape::RANDOM:
                    output = fastRandom(m_rngState) * 2.0f - 1.0f;
                    break;
                    
                case LFOShape::SAMPLE_HOLD:
                    if (phase < m_sampleHoldPhase) {
                        m_sampleHoldValue = fastRandom(m_rngState) * 2.0f - 1.0f;
                    }
                    m_sampleHoldPhase = phase;
                    output = m_sampleHoldValue;
                    break;
            }
            
            m_phase += m_phaseInc;
            if (m_phase >= 1.0) m_phase -= 1.0;
            
            return output;
        }
        
        void reset() noexcept {
            m_phase = 0.0;
            m_sampleHoldValue = 0.0f;
            m_sampleHoldPhase = 0.0;
        }
    };
    
    // ========================================================================
    // Optimized State Variable Filter with Cached Coefficients
    // ========================================================================
    class OptimizedSVFilter {
        double m_ic1eq{0.0}, m_ic2eq{0.0};
        
        // Cache coefficients as floats for hot loop
        float m_g{0.5f}, m_k{1.0f};
        float m_a1{0.0f}, m_a2{0.0f}, m_a3{0.0f};
        
    public:
        void setFrequency(float freq, float resonance, double sampleRate) noexcept {
            freq = std::clamp(freq, 20.0f, 20000.0f);
            resonance = std::clamp(resonance, 0.0f, 0.99f);
            
            const double wd = 2.0 * M_PI * freq;
            const double T = 1.0 / sampleRate;
            const double wa = (2.0 / T) * std::tan(wd * T / 2.0);
            const double g = wa * T / 2.0;
            
            // Cache as floats for speed
            m_g = static_cast<float>(g);
            m_k = 2.0f - 2.0f * resonance;
            
            const double a1 = 1.0 / (1.0 + g * (g + m_k));
            const double a2 = g * a1;
            const double a3 = g * a2;
            
            m_a1 = static_cast<float>(a1);
            m_a2 = static_cast<float>(a2);
            m_a3 = static_cast<float>(a3);
        }
        
        float process(float input) noexcept {
            // Use cached float coefficients
            const float v3 = input - static_cast<float>(m_ic2eq);
            const float v1 = m_a1 * static_cast<float>(m_ic1eq) + m_a2 * v3;
            const float v2 = static_cast<float>(m_ic2eq) + m_a2 * static_cast<float>(m_ic1eq) + m_a3 * v3;
            
            // Update states
            m_ic1eq = 2.0 * v1 - m_ic1eq;
            m_ic2eq = 2.0 * v2 - m_ic2eq;
            
            return v2; // Lowpass output
        }
        
        // Separate SIMD denormal flush
        void flushDenorms() noexcept {
#if HAS_SSE2
            __m128d states = _mm_set_pd(m_ic1eq, m_ic2eq);
            __m128d abs_states = _mm_andnot_pd(_mm_set_pd(-0.0, -0.0), states);
            __m128d mask = _mm_cmpge_pd(abs_states, _mm_set1_pd(DENORM_THRESHOLD));
            states = _mm_and_pd(states, mask);
            m_ic1eq = _mm_cvtsd_f64(states);
            m_ic2eq = _mm_cvtsd_f64(_mm_unpackhi_pd(states, states));
#else
            if (std::fabs(m_ic1eq) < DENORM_THRESHOLD) m_ic1eq = 0.0;
            if (std::fabs(m_ic2eq) < DENORM_THRESHOLD) m_ic2eq = 0.0;
#endif
        }
        
        void reset() noexcept {
            m_ic1eq = m_ic2eq = 0.0;
        }
    };
    
    // ========================================================================
    // AVX2-Optimized Delay Line
    // ========================================================================
    class OptimizedDelayLine {
        alignas(64) float* m_buffer{nullptr};
        static constexpr int BUFFER_SIZE = MAX_DELAY_SAMPLES;
        static constexpr int BUFFER_MASK = 8191; // Next power of 2 - 1
        uint32_t m_writePos{0};
        
    public:
        OptimizedDelayLine() {
#if HAS_SSE2
            m_buffer = static_cast<float*>(_mm_malloc(8192 * sizeof(float), 64));
#else
            // Portable aligned allocation fallback
            #if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
                void* ptr = nullptr;
                if (posix_memalign(&ptr, 64, 8192 * sizeof(float)) == 0) {
                    m_buffer = static_cast<float*>(ptr);
                } else {
                    m_buffer = static_cast<float*>(malloc(8192 * sizeof(float)));
                }
            #else
                // Simple malloc fallback for systems without posix_memalign
                m_buffer = static_cast<float*>(malloc(8192 * sizeof(float)));
            #endif
#endif
            reset();
        }
        
        ~OptimizedDelayLine() {
            if (m_buffer) {
#if HAS_SSE2
                _mm_free(m_buffer);
#else
                free(m_buffer);
#endif
            }
        }
        
        void write(float input) noexcept {
            m_buffer[m_writePos & BUFFER_MASK] = input;
            m_writePos++;
        }
        
        float read(float delaySamples) const noexcept {
            delaySamples = std::clamp(delaySamples, 0.0f, static_cast<float>(BUFFER_SIZE - 4));
            
            const int delayInt = static_cast<int>(delaySamples);
            const float frac = delaySamples - delayInt;
            
#if HAS_AVX2
            // AVX2 gather for 4 points
            const int baseIdx = (m_writePos - delayInt - 2) & BUFFER_MASK;
            __m128i indices = _mm_set_epi32(
                (baseIdx + 3) & BUFFER_MASK,
                (baseIdx + 2) & BUFFER_MASK,
                (baseIdx + 1) & BUFFER_MASK,
                baseIdx
            );
            __m128 samples = _mm_i32gather_ps(m_buffer, indices, 4);
            
            float y[4];
            _mm_storeu_ps(y, samples);
#else
            // Scalar fallback
            const int baseIdx = (m_writePos - delayInt - 2) & BUFFER_MASK;
            float y[4];
            for (int i = 0; i < 4; ++i) {
                y[i] = m_buffer[(baseIdx + i) & BUFFER_MASK];
            }
#endif
            
            // Hermite interpolation (horizontal operation)
            const float c0 = y[1];
            const float c1 = 0.5f * (y[2] - y[0]);
            const float c2 = y[0] - 2.5f * y[1] + 2.0f * y[2] - 0.5f * y[3];
            const float c3 = 0.5f * (y[3] - y[0]) + 1.5f * (y[1] - y[2]);
            
            return ((c3 * frac + c2) * frac + c1) * frac + c0;
        }
        
        void flushDenorms(int numSamples) noexcept {
            // Vectorized denormal flush
#if HAS_SSE2
            for (int i = 0; i < numSamples; i += 4) {
                int idx = (m_writePos - i) & BUFFER_MASK;
                __m128 vals = _mm_load_ps(&m_buffer[idx & ~3]);
                __m128 abs_vals = _mm_andnot_ps(_mm_set_ps1(-0.0f), vals);
                __m128 mask = _mm_cmpge_ps(abs_vals, _mm_set_ps1(DENORM_THRESHOLD));
                vals = _mm_and_ps(vals, mask);
                _mm_store_ps(&m_buffer[idx & ~3], vals);
            }
#else
            for (int i = 0; i < numSamples; ++i) {
                int idx = (m_writePos - i) & BUFFER_MASK;
                m_buffer[idx] = flushDenormSSE(m_buffer[idx]);
            }
#endif
        }
        
        void reset() noexcept {
            if (m_buffer) {
                std::memset(m_buffer, 0, 8192 * sizeof(float));
            }
            m_writePos = 0;
        }
    };
    
    // ========================================================================
    // Enhanced BBD Model with Crossfade
    // ========================================================================
    class EnhancedBBD {
        // Cascaded filters for smoother response
        float m_lp1{0.0f}, m_lp2{0.0f};
        float m_hp{0.0f};
        float m_fadeIn{0.0f};
        static constexpr float FADE_RATE = 0.001f;
        
    public:
        float process(float input, float amount) noexcept {
            if (amount < 0.01f) return input;
            
            // Fade in to avoid thumps
            if (m_fadeIn < 1.0f) {
                m_fadeIn = std::min(1.0f, m_fadeIn + FADE_RATE);
                amount *= m_fadeIn;
            }
            
            // DC blocking highpass
            float hp = input - m_hp;
            m_hp += hp * 0.001f;
            
            // Cascaded lowpass for smoother BBD response
            m_lp1 += (hp - m_lp1) * 0.3f;
            m_lp2 += (m_lp1 - m_lp2) * 0.5f;
            
            // Soft saturation
            float sat = m_lp2;
            if (std::abs(sat) > 0.5f) {
                sat = std::tanh(sat * 1.5f) * 0.667f;
            }
            
            return input * (1.0f - amount) + sat * amount;
        }
        
        void reset() noexcept {
            m_lp1 = m_lp2 = m_hp = 0.0f;
            m_fadeIn = 0.0f;
        }
    };
    
    // ========================================================================
    // Optimized Voice Structure
    // ========================================================================
    struct Voice {
        OptimizedDelayLine delay;
        OptimizedSVFilter filter;
        TableLFO lfo;
        EnhancedBBD bbd;
        float gain{1.0f};
        float pan{0.0f};
        bool active{false}; // For voice masking
        
        void reset() {
            delay.reset();
            filter.reset();
            lfo.reset();
            bbd.reset();
            active = false;
        }
    };
    
    // ========================================================================
    // Main State with Optimizations
    // ========================================================================
    
    // Parameters with user-configurable smooth times
    struct Parameters {
        UltraSmoother rate;
        UltraSmoother depth;
        UltraSmoother resonance;
        UltraSmoother filterFreq;
        UltraSmoother voices;
        UltraSmoother spread;
        UltraSmoother feedback;
        UltraSmoother feedbackSmooth; // Separate smoother for feedback
        UltraSmoother mix;
    } m_params;
    
    // Voices
    std::array<Voice, MAX_VOICES> m_voicesArray;
    
    // Configuration
    double m_sampleRate{44100.0};
    Mode m_mode{Mode::CLASSIC};
    LFOShape m_lfoShape{LFOShape::SINE};
    Config m_config;
    
    // DC blocking (inline for register reuse)
    float m_dcBlockX1[2]{0.0f, 0.0f};
    float m_dcBlockY1[2]{0.0f, 0.0f};
    static constexpr float DC_COEFF = 0.995f;
    
    // Feedback with smoothing
    float m_feedbackBuffer[2]{0.0f, 0.0f};
    
    // Pre-calculated mix coefficients
    float m_dryGain{0.5f};
    float m_wetGain{0.5f};
    
    // Global flush counter
    uint32_t m_globalFlushCounter{0};
    
    // User-configurable smooth response
    enum class SmoothResponse {
        TIGHT,  // Fast response (10-50ms)
        NORMAL, // Medium response (20-100ms)
        LUSH    // Slow response (50-300ms)
    };
    SmoothResponse m_smoothResponse{SmoothResponse::NORMAL};
    
public:
    Impl() {
        // Initialize parameters
        m_params.rate.reset(0.5f);
        m_params.depth.reset(0.3f);
        m_params.resonance.reset(0.0f);
        m_params.filterFreq.reset(0.5f);
        m_params.voices.reset(0.4f);
        m_params.spread.reset(0.5f);
        m_params.feedback.reset(0.5f);
        m_params.feedbackSmooth.reset(0.5f);
        m_params.mix.reset(0.5f);
        
        // Initialize voices with phase offsets
        for (int i = 0; i < MAX_VOICES; ++i) {
            m_voicesArray[i].pan = (i % 2 == 0) ? -0.5f : 0.5f;
            m_voicesArray[i].lfo.setPhaseOffset(static_cast<float>(i) / MAX_VOICES);
        }
    }
    
    void prepare(double sampleRate, int samplesPerBlock) {
        m_sampleRate = sampleRate;
        
        // Configure smoothers based on response mode
        float smoothBase = 1.0f;
        switch (m_smoothResponse) {
            case SmoothResponse::TIGHT: smoothBase = 0.5f; break;
            case SmoothResponse::NORMAL: smoothBase = 1.0f; break;
            case SmoothResponse::LUSH: smoothBase = 2.0f; break;
        }
        
        m_params.rate.setSampleRate(sampleRate, 50.0f * smoothBase);
        m_params.depth.setSampleRate(sampleRate, 30.0f * smoothBase);
        m_params.resonance.setSampleRate(sampleRate, 50.0f * smoothBase);
        m_params.filterFreq.setSampleRate(sampleRate, 30.0f * smoothBase);
        m_params.voices.setSampleRate(sampleRate, 100.0f * smoothBase);
        m_params.spread.setSampleRate(sampleRate, 50.0f * smoothBase);
        m_params.feedback.setSampleRate(sampleRate, 30.0f * smoothBase);
        m_params.feedbackSmooth.setSampleRate(sampleRate, 20.0f); // Always fast
        m_params.mix.setSampleRate(sampleRate, 20.0f * smoothBase);
        
        // Reset all voices
        for (auto& voice : m_voicesArray) {
            voice.reset();
        }
        
        // Reset DC blockers
        std::memset(m_dcBlockX1, 0, sizeof(m_dcBlockX1));
        std::memset(m_dcBlockY1, 0, sizeof(m_dcBlockY1));
        std::memset(m_feedbackBuffer, 0, sizeof(m_feedbackBuffer));
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        // Get smoothed parameters
        const float rate = m_params.rate.process() * 20.0f;
        const float depth = m_params.depth.process();
        const float resonance = m_params.resonance.process();
        const float filterFreq = 20.0f + m_params.filterFreq.process() * 19980.0f;
        const int activeVoices = static_cast<int>(m_params.voices.process() * (MAX_VOICES - 1) + 1);
        const float spread = m_params.spread.process();
        const float feedback = (m_params.feedback.process() - 0.5f) * 2.0f;
        const float feedbackGain = m_params.feedbackSmooth.process();
        const float mix = m_params.mix.process();
        
        // Pre-calculate and hoist mix coefficients
        m_dryGain = 1.0f - mix;
        m_wetGain = mix;
        
        // Configure active voices only
        for (int v = 0; v < MAX_VOICES; ++v) {
            auto& voice = m_voicesArray[v];
            voice.active = (v < activeVoices);
            
            if (voice.active) {
                // Set LFO rate with variation
                float voiceRate = rate * (1.0f + v * 0.1f);
                voice.lfo.setRate(voiceRate, m_sampleRate);
                voice.lfo.setShape(m_lfoShape);
                
                // Configure filter
                voice.filter.setFrequency(filterFreq, resonance, m_sampleRate);
                
                // Update pan
                float basePan = (v % 2 == 0) ? -1.0f : 1.0f;
                voice.pan = basePan * spread;
            }
        }
        
        // Process audio
        for (int i = 0; i < numSamples; ++i) {
            // Get input
            float input = 0.0f;
            if (numChannels == 1) {
                input = buffer.getReadPointer(0)[i];
            } else {
                input = (buffer.getReadPointer(0)[i] + buffer.getReadPointer(1)[i]) * 0.5f;
            }
            
            // Add smoothed feedback
            input += (m_feedbackBuffer[0] + m_feedbackBuffer[1]) * 0.5f * feedback * feedbackGain * 0.3f;
            
            float outputL = 0.0f;
            float outputR = 0.0f;
            
            // Process only active voices (early-out optimization)
            for (int v = 0; v < activeVoices; ++v) {
                auto& voice = m_voicesArray[v];
                
                // Write to delay
                voice.delay.write(input);
                
                // Calculate modulated delay
                float lfoValue = voice.lfo.process();
                float delayMs = 5.0f + (1.0f + lfoValue) * depth * 20.0f;
                float delaySamples = delayMs * 0.001f * m_sampleRate;
                
                // Read with interpolation
                float delayed = voice.delay.read(delaySamples);
                
                // Filter
                delayed = voice.filter.process(delayed);
                
                // BBD if enabled
                if (m_config.enableAnalogModel) {
                    delayed = voice.bbd.process(delayed, 0.3f);
                }
                
                // Apply gain
                delayed *= voice.gain / activeVoices;
                
                // Pan
                float panL = std::min(1.0f, 1.0f - voice.pan);
                float panR = std::min(1.0f, 1.0f + voice.pan);
                
                outputL += delayed * panL;
                outputR += delayed * panR;
            }
            
            // Store feedback
            m_feedbackBuffer[0] = outputL;
            m_feedbackBuffer[1] = outputR;
            
            // Inline DC blocking
            outputL = outputL - m_dcBlockX1[0] + DC_COEFF * m_dcBlockY1[0];
            m_dcBlockX1[0] = outputL;
            m_dcBlockY1[0] = outputL;
            
            outputR = outputR - m_dcBlockX1[1] + DC_COEFF * m_dcBlockY1[1];
            m_dcBlockX1[1] = outputR;
            m_dcBlockY1[1] = outputR;
            
            // Branchless mix with pre-calculated coefficients
            if (numChannels == 1) {
                float dry = buffer.getReadPointer(0)[i];
                buffer.getWritePointer(0)[i] = dry * m_dryGain + (outputL + outputR) * 0.5f * m_wetGain;
            } else {
                float dryL = buffer.getReadPointer(0)[i];
                float dryR = buffer.getReadPointer(1)[i];
                buffer.getWritePointer(0)[i] = dryL * m_dryGain + outputL * m_wetGain;
                buffer.getWritePointer(1)[i] = dryR * m_dryGain + outputR * m_wetGain;
            }
        }
        
        // Global denormal flush (once per block)
        if ((++m_globalFlushCounter & 0x3) == 0) { // Every 4 blocks
            // Flush parameter smoothers
            m_params.rate.flushDenorm();
            m_params.depth.flushDenorm();
            m_params.resonance.flushDenorm();
            m_params.filterFreq.flushDenorm();
            m_params.voices.flushDenorm();
            m_params.spread.flushDenorm();
            m_params.feedback.flushDenorm();
            m_params.feedbackSmooth.flushDenorm();
            m_params.mix.flushDenorm();
            
            // Flush voice filters and delays
            for (int v = 0; v < activeVoices; ++v) {
                m_voicesArray[v].filter.flushDenorms();
                m_voicesArray[v].delay.flushDenorms(256);
            }
            
            // Flush DC blockers
            if (std::fabs(m_dcBlockY1[0]) < DENORM_THRESHOLD) m_dcBlockY1[0] = 0.0f;
            if (std::fabs(m_dcBlockY1[1]) < DENORM_THRESHOLD) m_dcBlockY1[1] = 0.0f;
        }
    }
    
    void setSmoothResponse(SmoothResponse response) {
        m_smoothResponse = response;
        // Reconfigure smoothers on next prepare()
    }
    
    void setParameterSmoothTime(int index, float ms) {
        ms = std::clamp(ms, 1.0f, 500.0f);
        
        switch (static_cast<ParamID>(index)) {
            case ParamID::RATE: m_params.rate.setSmoothTime(ms); break;
            case ParamID::DEPTH: m_params.depth.setSmoothTime(ms); break;
            case ParamID::RESONANCE: m_params.resonance.setSmoothTime(ms); break;
            case ParamID::FILTER_FREQ: m_params.filterFreq.setSmoothTime(ms); break;
            case ParamID::VOICES: m_params.voices.setSmoothTime(ms); break;
            case ParamID::SPREAD: m_params.spread.setSmoothTime(ms); break;
            case ParamID::FEEDBACK: m_params.feedback.setSmoothTime(ms); break;
            case ParamID::MIX: m_params.mix.setSmoothTime(ms); break;
        }
    }
    
    void reset() {
        for (auto& voice : m_voicesArray) {
            voice.reset();
        }
        
        m_dcBlockX1[0] = m_dcBlockX1[1] = 0.0f;
        m_dcBlockY1[0] = m_dcBlockY1[1] = 0.0f;
        m_feedbackBuffer[0] = m_feedbackBuffer[1] = 0.0f;
        m_globalFlushCounter = 0;
    }
    
    void updateParameters(const std::map<int, float>& params) {
        for (const auto& [index, value] : params) {
            switch (static_cast<ParamID>(index)) {
                case ParamID::RATE: m_params.rate.setTarget(value); break;
                case ParamID::DEPTH: m_params.depth.setTarget(value); break;
                case ParamID::RESONANCE: m_params.resonance.setTarget(value); break;
                case ParamID::FILTER_FREQ: m_params.filterFreq.setTarget(value); break;
                case ParamID::VOICES: m_params.voices.setTarget(value); break;
                case ParamID::SPREAD: m_params.spread.setTarget(value); break;
                case ParamID::FEEDBACK: m_params.feedback.setTarget(value); break;
                case ParamID::MIX: m_params.mix.setTarget(value); break;
            }
        }
    }
    
    void setMode(Mode mode) { m_mode = mode; }
    Mode getMode() const { return m_mode; }
    
    void setLFOShape(LFOShape shape) { m_lfoShape = shape; }
    LFOShape getLFOShape() const { return m_lfoShape; }
    
    void setConfig(const Config& config) { m_config = config; }
    Config getConfig() const { return m_config; }
};

// Initialize static members of TableLFO
alignas(64) float ResonantChorus_Platinum::Impl::TableLFO::s_sineTable[1024];
alignas(64) float ResonantChorus_Platinum::Impl::TableLFO::s_triangleTable[1024];  
alignas(64) float ResonantChorus_Platinum::Impl::TableLFO::s_sawTable[1024];
bool ResonantChorus_Platinum::Impl::TableLFO::s_tablesInitialized = false;

// ============================================================================
// Public Implementation
// ============================================================================

ResonantChorus_Platinum::ResonantChorus_Platinum() 
    : pImpl(std::make_unique<Impl>()) {
}

ResonantChorus_Platinum::~ResonantChorus_Platinum() = default;

void ResonantChorus_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepare(sampleRate, samplesPerBlock);
}

void ResonantChorus_Platinum::process(juce::AudioBuffer<float>& buffer) {
    pImpl->process(buffer);
}

void ResonantChorus_Platinum::reset() {
    pImpl->reset();
}

void ResonantChorus_Platinum::updateParameters(const std::map<int, float>& params) {
    pImpl->updateParameters(params);
}

juce::String ResonantChorus_Platinum::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::RATE: return "Rate";
        case ParamID::DEPTH: return "Depth";
        case ParamID::RESONANCE: return "Resonance";
        case ParamID::FILTER_FREQ: return "Filter Freq";
        case ParamID::VOICES: return "Voices";
        case ParamID::SPREAD: return "Spread";
        case ParamID::FEEDBACK: return "Feedback";
        case ParamID::MIX: return "Mix";
        default: return "";
    }
}

float ResonantChorus_Platinum::getParameterValue(int index) const {
    // Implementation would return current parameter values
    return 0.5f;
}

juce::String ResonantChorus_Platinum::getParameterText(int index) const {
    // Implementation would return parameter text representation
    return "";
}

float ResonantChorus_Platinum::getParameterDefaultValue(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::RATE: return 0.5f;
        case ParamID::DEPTH: return 0.3f;
        case ParamID::RESONANCE: return 0.0f;
        case ParamID::FILTER_FREQ: return 0.5f;
        case ParamID::VOICES: return 0.4f;
        case ParamID::SPREAD: return 0.5f;
        case ParamID::FEEDBACK: return 0.5f;
        case ParamID::MIX: return 0.5f;
        default: return 0.5f;
    }
}

void ResonantChorus_Platinum::setParameterValue(int index, float value) {
    std::map<int, float> params;
    params[index] = value;
    updateParameters(params);
}

void ResonantChorus_Platinum::setMode(Mode mode) {
    pImpl->setMode(mode);
}

ResonantChorus_Platinum::Mode ResonantChorus_Platinum::getMode() const {
    return pImpl->getMode();
}

void ResonantChorus_Platinum::setConfig(const Config& config) {
    pImpl->setConfig(config);
}

ResonantChorus_Platinum::Config ResonantChorus_Platinum::getConfig() const {
    return pImpl->getConfig();
}

void ResonantChorus_Platinum::setLFOShape(LFOShape shape) {
    pImpl->setLFOShape(shape);
}

ResonantChorus_Platinum::LFOShape ResonantChorus_Platinum::getLFOShape() const {
    return pImpl->getLFOShape();
}

float ResonantChorus_Platinum::getInputLevel() const {
    // Implementation would return actual input level
    return -20.0f;
}

float ResonantChorus_Platinum::getOutputLevel() const {
    // Implementation would return actual output level
    return -20.0f;
}

float ResonantChorus_Platinum::getModulationDepth() const {
    // Implementation would return current modulation depth
    return 0.0f;
}
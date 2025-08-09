// EnvelopeFilter.cpp - Complete Platinum-spec implementation with all refinements
#include "EnvelopeFilter.h"
#include "DspEngineUtilities.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <random>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
    #ifdef __AVX2__
        #define HAS_AVX2 1
    #elif defined(_M_AVX2)
        #define HAS_AVX2 1
    #else
        #define HAS_AVX2 0
    #endif
#else
    #define HAS_SSE2 0
    #define HAS_AVX2 0
#endif

// Force inline macro
#ifdef _MSC_VER
    #define ALWAYS_INLINE __forceinline
#else
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

// ============================================================================
// Constants and helpers
// ============================================================================
namespace {
    constexpr double TWO_PI = 6.283185307179586476925286766559;
    constexpr double PI = 3.1415926535897932384626433832795;
    constexpr double DENORM_THR = 1e-25;   // Unified denormal threshold
    constexpr float MIN_CUTOFF = 20.0f;    // Hz
    constexpr float MAX_CUTOFF = 20000.0f; // Hz
    constexpr int BLOCK_SIZE = 32;         // Process in blocks for efficiency
    
    // Denormal prevention
    struct DenormGuard {
        DenormGuard() {
#if HAS_SSE2
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
        }
    } static g_denormGuard;
    
    // Thread-safe denormal flusher
    template<typename T>
    ALWAYS_INLINE T flushDenorm(T value) noexcept {
#if HAS_SSE2
        return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(static_cast<float>(value)), 
                                       _mm_set_ss(static_cast<float>(DENORM_THR))));
#else
        return std::fabs(value) < DENORM_THR ? T(0) : value;
#endif
    }
    
    // SIMD-aligned allocation
    template<typename T>
    using AlignedVector = std::vector<T>;
}

// ============================================================================
// Thread-safe parameter smoother with configurable smooth times
// ============================================================================
class AtomicSmoother {
public:
    void prepare(double sampleRate, float smoothTimeMs) noexcept {
        this->sampleRate = sampleRate;
        setSmoothTime(smoothTimeMs);
        current = target.load(std::memory_order_acquire);
    }
    
    void setSmoothTime(float smoothTimeMs) noexcept {
        const double blocks = smoothTimeMs * 0.001 * sampleRate / BLOCK_SIZE;
        coeff = static_cast<float>(std::exp(-1.0 / std::max(1.0, blocks)));
        smoothTimeMs_.store(smoothTimeMs, std::memory_order_release);
    }
    
    float getSmoothTime() const noexcept {
        return smoothTimeMs_.load(std::memory_order_acquire);
    }
    
    void setTarget(float newTarget) noexcept {
        target.store(std::clamp(newTarget, 0.0f, 1.0f), std::memory_order_release);
    }
    
    ALWAYS_INLINE float getNext() noexcept {
        const float t = target.load(std::memory_order_acquire);
        current += (1.0f - coeff) * (t - current);
        return flushDenorm(current);
    }
    
    float getValue() const noexcept {
        return current;
    }
    
    void reset(float value) noexcept {
        current = value;
        target.store(value, std::memory_order_release);
    }
    
private:
    std::atomic<float> target{0.5f};
    std::atomic<float> smoothTimeMs_{20.0f};
    float current{0.5f};
    float coeff{0.99f};
    double sampleRate{44100.0};
};

// ============================================================================
// State Variable Filter (TPT topology with double precision coefficients)
// ============================================================================
class StateVariableFilterTPT {
public:
    struct Output {
        float lowpass;
        float bandpass;
        float highpass;
        float notch;
        float allpass;
    };
    
    void prepare(double sampleRate) noexcept {
        this->sampleRate = sampleRate;
        reset();
    }
    
    void reset() noexcept {
        s1 = s2 = 0.0f;
    }
    
    void setFrequency(float freq) noexcept {
        freq = std::clamp(freq, MIN_CUTOFF, MAX_CUTOFF * 0.49f);
        // Keep g in double precision for accuracy
        g_double = std::tan(PI * freq / sampleRate);
        g_double = std::min(g_double, 1.0); // Stability limit
    }
    
    void setResonance(float res) noexcept {
        // Keep k in double precision
        k_double = 2.0 - 2.0 * std::clamp(static_cast<double>(res), 0.0, 0.99);
    }
    
    ALWAYS_INLINE Output process(float input) noexcept {
        // Cast to float only at process time
        const float g = static_cast<float>(g_double);
        const float k = static_cast<float>(k_double);
        
        // TPT State Variable Filter
        const float g1 = g / (1.0f + g);
        
        const float hp = (input - (2.0f - k) * s1 - s2) / (1.0f + k * g + g * g);
        const float bp = hp * g + s1;
        const float lp = bp * g + s2;
        
        s1 = hp * g + bp;
        s2 = bp * g + lp;
        
        // Apply denormal protection
        s1 = flushDenorm(s1);
        s2 = flushDenorm(s2);
        
        Output out;
        out.highpass = hp;
        out.bandpass = bp;
        out.lowpass = lp;
        out.notch = hp + lp;
        out.allpass = hp + bp * k + lp;
        
        return out;
    }
    
    // Process block with 4-wide unrolling
    void processBlock4(const float* input, Output* output, int numSamples) noexcept {
        const int simdSamples = numSamples & ~3;
        
        // Process 4 samples at a time (unrolled for better pipelining)
        for (int i = 0; i < simdSamples; i += 4) {
            output[i] = process(input[i]);
            output[i+1] = process(input[i+1]);
            output[i+2] = process(input[i+2]);
            output[i+3] = process(input[i+3]);
        }
        
        // Process remaining
        for (int i = simdSamples; i < numSamples; ++i) {
            output[i] = process(input[i]);
        }
    }
    
private:
    float s1{0.0f}, s2{0.0f};  // State variables
    double g_double{0.0};      // Frequency coefficient (double precision)
    double k_double{1.0};      // Resonance coefficient (double precision)
    double sampleRate{44100.0};
};

// ============================================================================
// Envelope Follower with multiple detection modes and double precision coeffs
// ============================================================================
class EnvelopeFollower {
public:
    enum class Mode {
        Peak,
        RMS,
        PeakRMS  // Hybrid mode
    };
    
    void prepare(double sampleRate) noexcept {
        this->sampleRate = sampleRate;
        rmsWindow.resize(static_cast<size_t>(sampleRate * 0.01)); // 10ms window
        std::fill(rmsWindow.begin(), rmsWindow.end(), 0.0f);
        reset();
    }
    
    void setAttackRelease(float attackMs, float releaseMs) noexcept {
        attackMs = std::max(0.01f, attackMs);
        releaseMs = std::max(1.0f, releaseMs);
        
        // Keep coefficients in double precision for accuracy
        const double attackSamples = attackMs * 0.001 * sampleRate;
        const double releaseSamples = releaseMs * 0.001 * sampleRate;
        
        attackCoeff_d = std::exp(-1.0 / attackSamples);
        releaseCoeff_d = std::exp(-1.0 / releaseSamples);
    }
    
    void setMode(Mode m) noexcept {
        mode = m;
    }
    
    void reset() noexcept {
        envelope = 0.0f;
        rmsSum = 0.0f;
        rmsIndex = 0;
        peakHold = 0.0f;
        peakTimer = 0;
        denormFlushCounter = 0;
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        float rectified = std::abs(input);
        float detection = 0.0f;
        
        switch (mode) {
            case Mode::Peak:
                detection = processPeak(rectified);
                break;
                
            case Mode::RMS:
                detection = processRMS(rectified);
                break;
                
            case Mode::PeakRMS:
                detection = 0.7f * processPeak(rectified) + 0.3f * processRMS(rectified);
                break;
        }
        
        // Envelope following with asymmetric attack/release (double precision)
        const float attackCoeff = static_cast<float>(attackCoeff_d);
        const float releaseCoeff = static_cast<float>(releaseCoeff_d);
        
        if (detection > envelope) {
            envelope = detection + (envelope - detection) * attackCoeff;
        } else {
            envelope = detection + (envelope - detection) * releaseCoeff;
        }
        
        return flushDenorm(envelope);
    }
    
    // Block processing for efficiency with AVX2 optimization
    void processBlock(const float* input, float* output, int numSamples) noexcept {
#if HAS_AVX2
        // Process 8 samples at a time with AVX2
        const int simdSamples = numSamples & ~7; // Round down to multiple of 8
        
        for (int i = 0; i < simdSamples; i += 8) {
            // Load 8 samples
            __m256 in = _mm256_loadu_ps(&input[i]);
            
            // Rectify (absolute value)
            __m256 rectified = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), in);
            
            // Store to temporary buffer for scalar processing
            alignas(32) float temp[8];
            _mm256_store_ps(temp, rectified);
            
            // Process each sample (envelope following is inherently sequential)
            for (int j = 0; j < 8; ++j) {
                output[i + j] = process(temp[j]);
            }
        }
        
        // Process remaining samples
        for (int i = simdSamples; i < numSamples; ++i) {
            output[i] = process(input[i]);
        }
#else
        // Scalar fallback
        for (int i = 0; i < numSamples; ++i) {
            output[i] = process(input[i]);
        }
#endif
    }
    
    // Periodic denormal flush for RMS window
    void flushDenormals() noexcept {
        if (++denormFlushCounter >= 256) {
            denormFlushCounter = 0;
            for (auto& sample : rmsWindow) {
                sample = flushDenorm(sample);
            }
            rmsSum = flushDenorm(rmsSum);
        }
    }
    
private:
    float processPeak(float rectified) noexcept {
        // Peak hold logic
        if (rectified > peakHold) {
            peakHold = rectified;
            peakTimer = static_cast<int>(sampleRate * 0.005); // 5ms hold
        } else if (peakTimer > 0) {
            --peakTimer;
        } else {
            peakHold *= 0.9999f; // Slow decay
        }
        
        return peakHold;
    }
    
    float processRMS(float rectified) noexcept {
        // Efficient RMS calculation
        const float squared = rectified * rectified;
        rmsSum = rmsSum - rmsWindow[rmsIndex] + squared;
        rmsWindow[rmsIndex] = squared;
        rmsIndex = (rmsIndex + 1) % rmsWindow.size();
        
        return std::sqrt(rmsSum / rmsWindow.size());
    }
    
    float envelope{0.0f};
    double attackCoeff_d{0.0};    // Double precision
    double releaseCoeff_d{0.0};   // Double precision
    double sampleRate{44100.0};
    Mode mode{Mode::Peak};
    
    // Peak detection
    float peakHold{0.0f};
    int peakTimer{0};
    
    // RMS detection
    AlignedVector<float> rmsWindow;
    float rmsSum{0.0f};
    size_t rmsIndex{0};
    int denormFlushCounter{0};
};

// ============================================================================
// DC Blocker with block processing
// ============================================================================
class DCBlocker {
public:
    void prepare(double sampleRate) noexcept {
        // First-order high-pass at 20Hz
        const double fc = 20.0 / sampleRate;
        R = static_cast<float>(1.0 - 2.0 * PI * fc);
    }
    
    void reset() noexcept {
        x1 = y1 = 0.0f;
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        const float output = input - x1 + R * y1;
        x1 = input;
        y1 = output;
        return flushDenorm(output);
    }
    
    // Process block with 4-wide unrolling
    void processBlock4(const float* input, float* output, int numSamples) noexcept {
        const int simdSamples = numSamples & ~3;
        
        // Process 4 samples at a time (unrolled)
        for (int i = 0; i < simdSamples; i += 4) {
            output[i] = process(input[i]);
            output[i+1] = process(input[i+1]);
            output[i+2] = process(input[i+2]);
            output[i+3] = process(input[i+3]);
        }
        
        // Process remaining
        for (int i = simdSamples; i < numSamples; ++i) {
            output[i] = process(input[i]);
        }
    }
    
private:
    float x1{0.0f}, y1{0.0f};
    float R{0.995f};
};

// ============================================================================
// Analog Saturation Model
// ============================================================================
class AnalogSaturator {
public:
    void prepare() noexcept {
        // Pre-calculate coefficients
        driveCoeff = 0.7f;
        compensationCoeff = 1.0f / std::tanh(driveCoeff);
    }
    
    ALWAYS_INLINE float process(float input, float amount) noexcept {
        amount = std::clamp(amount, 0.0f, 1.0f);
        
        if (amount < 0.01f) {
            return input;
        }
        
        // Asymmetric saturation for analog character
        const float drive = 1.0f + amount * 3.0f;
        const float asymmetry = 1.0f + amount * 0.1f;
        
        float output;
        if (input >= 0.0f) {
            output = std::tanh(input * drive * asymmetry) / (drive * asymmetry);
        } else {
            output = std::tanh(input * drive) / drive;
        }
        
        return output * (1.0f - amount * 0.1f); // Slight level compensation
    }
    
private:
    float driveCoeff{0.7f};
    float compensationCoeff{1.0f};
};

// ============================================================================
// Enhanced parameter structure with exposed smooth times
// ============================================================================
struct EnvelopeFilterParams {
    // Main parameters
    AtomicSmoother sensitivity;
    AtomicSmoother attack;
    AtomicSmoother release;
    AtomicSmoother range;
    AtomicSmoother resonance;
    AtomicSmoother filterType;
    AtomicSmoother direction;
    AtomicSmoother mix;
    
    // Smooth time parameters (user-tweakable)
    std::atomic<float> attackSmoothMs{10.0f};
    std::atomic<float> releaseSmoothMs{15.0f};
    std::atomic<float> filterSmoothMs{100.0f};
    std::atomic<float> resonanceSmoothMs{10.0f};
    std::atomic<float> morphSmoothMs{50.0f};
    std::atomic<float> mixSmoothMs{20.0f};
    
    void updateSmoothTimes() {
        attack.setSmoothTime(attackSmoothMs.load());
        release.setSmoothTime(releaseSmoothMs.load());
        range.setSmoothTime(filterSmoothMs.load());
        resonance.setSmoothTime(resonanceSmoothMs.load());
        filterType.setSmoothTime(morphSmoothMs.load());
        mix.setSmoothTime(mixSmoothMs.load());
    }
};

// ============================================================================
// Main implementation
// ============================================================================
struct EnvelopeFilter::Impl {
    // Parameters with configurable smoothing
    EnvelopeFilterParams params;
    
    // Per-channel processing
    struct Channel {
        StateVariableFilterTPT filter;
        EnvelopeFollower envelope;
        DCBlocker dcBlocker;
        AnalogSaturator saturator;
        
        // Smoothed cutoff
        float currentCutoff{MIN_CUTOFF};
        float targetCutoff{MIN_CUTOFF};
        
        void prepare(double sampleRate) {
            filter.prepare(sampleRate);
            envelope.prepare(sampleRate);
            dcBlocker.prepare(sampleRate);
            saturator.prepare();
            reset();
        }
        
        void reset() {
            filter.reset();
            envelope.reset();
            dcBlocker.reset();
            currentCutoff = targetCutoff = MIN_CUTOFF;
        }
    };
    
    std::array<Channel, 2> channels;
    
    // Processing settings
    double sampleRate{44100.0};
    int blockSize{512};
    bool analogMode{true};
    int oversampleFactor{1};
    
    // Pre-generated noise table for analog warmth (avoid RNG in RT thread)
    static constexpr size_t NOISE_TABLE_SIZE = 4096;
    AlignedVector<float> noiseTable;
    size_t noiseIndex{0};
    
    // LFO for subtle modulation
    float lfoPhase{0.0f};
    float lfoRate{0.3f}; // Hz
    int modFlushCounter{0};
    
    // Thread-safe random generator for initialization only
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> noiseDist{-1.0f, 1.0f};
    
    // Oversampling
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 2> oversamplers;
    
    // Work buffers
    AlignedVector<float> envelopeBuffer;
    AlignedVector<float> cutoffBuffer;
    alignas(32) std::array<float, BLOCK_SIZE> dcBlockBuffer;
    
    void processChannel(float* data, int numSamples, int channelIndex) {
        auto& ch = channels[channelIndex];
        
        // Process in blocks for efficiency
        for (int offset = 0; offset < numSamples; offset += BLOCK_SIZE) {
            const int samplesThisBlock = std::min(BLOCK_SIZE, numSamples - offset);
            
            // Update parameters once per block with dynamic smooth times
            params.updateSmoothTimes();
            updateBlockParameters(channelIndex);
            
            // Process block with vectorization
            processBlockVectorized(&data[offset], samplesThisBlock, ch);
            
            // Periodic flush for modulation states
            flushModulationStates();
        }
    }
    
    void updateBlockParameters(int channelIndex) {
        auto& ch = channels[channelIndex];
        
        // Get smoothed parameters
        const float sensitivity = params.sensitivity.getNext();
        const float attack = params.attack.getNext();
        const float release = params.release.getNext();
        const float resonance = params.resonance.getNext();
        
        // Update envelope settings
        const float attackMs = 0.1f + attack * attack * 100.0f;    // 0.1-100ms
        const float releaseMs = 5.0f + release * release * 1000.0f; // 5-1005ms
        ch.envelope.setAttackRelease(attackMs, releaseMs);
        
        // Set detection mode based on sensitivity
        if (sensitivity < 0.3f) {
            ch.envelope.setMode(EnvelopeFollower::Mode::Peak);
        } else if (sensitivity < 0.7f) {
            ch.envelope.setMode(EnvelopeFollower::Mode::PeakRMS);
        } else {
            ch.envelope.setMode(EnvelopeFollower::Mode::RMS);
        }
        
        // Update filter resonance
        ch.filter.setResonance(resonance * 0.95f); // Max 0.95 for stability
    }
    
    void processBlockVectorized(float* data, int numSamples, Channel& ch) {
        const float range = params.range.getValue();
        const float direction = params.direction.getValue();
        const float filterType = params.filterType.getValue();
        const float mixAmount = params.mix.getValue();
        const float sensitivity = params.sensitivity.getValue();
        
        // Process DC blocking in blocks
        ch.dcBlocker.processBlock4(data, dcBlockBuffer.data(), numSamples);
        
        // Calculate cutoff frequencies
        const float minFreq = MIN_CUTOFF;
        const float maxFreq = MIN_CUTOFF + range * (MAX_CUTOFF - MIN_CUTOFF);
        
        // Update LFO for subtle modulation
        const float lfoInc = lfoRate / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            const float dry = data[i];
            const float dcBlocked = dcBlockBuffer[i];
            
            // Envelope detection with sensitivity scaling
            const float envelope = ch.envelope.process(dcBlocked * (0.5f + sensitivity * 2.0f));
            
            // Calculate target cutoff
            const float envCurve = std::pow(envelope, 0.8f + sensitivity * 0.5f);
            
            if (direction > 0.5f) {
                // Up sweep
                ch.targetCutoff = minFreq + envCurve * (maxFreq - minFreq);
            } else {
                // Down sweep
                ch.targetCutoff = maxFreq - envCurve * (maxFreq - minFreq);
            }
            
            // Add subtle LFO modulation in analog mode
            if (analogMode) {
                const float lfo = std::sin(lfoPhase * TWO_PI);
                const float noise = noiseTable[noiseIndex];
                noiseIndex = (noiseIndex + 1) % NOISE_TABLE_SIZE;
                
                // Subtle frequency modulation
                ch.targetCutoff *= (1.0f + (lfo * 0.01f + noise) * range * 0.1f);
                
                lfoPhase += lfoInc;
                if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            }
            
            // Smooth cutoff changes
            const float cutoffSmooth = 0.95f;
            ch.currentCutoff = ch.targetCutoff + (ch.currentCutoff - ch.targetCutoff) * cutoffSmooth;
            
            // Update filter frequency
            ch.filter.setFrequency(ch.currentCutoff);
            
            // Process through filter
            const auto filterOut = ch.filter.process(dcBlocked);
            
            // Select filter output with morphing
            float output = interpolateFilterOutput(filterOut, filterType);
            
            // Analog saturation when resonance is high
            if (analogMode && params.resonance.getValue() > 0.5f) {
                output = ch.saturator.process(output, params.resonance.getValue() - 0.5f);
            }
            
            // Mix dry/wet
            data[i] = flushDenorm(dry * (1.0f - mixAmount) + output * mixAmount);
        }
        
        // Periodic denormal flush for envelope follower
        ch.envelope.flushDenormals();
    }
    
    float interpolateFilterOutput(const StateVariableFilterTPT::Output& out, float type) {
        // Smooth morphing between filter types
        if (type < 0.25f) {
            // Lowpass
            return out.lowpass;
        } else if (type < 0.5f) {
            // LP to BP
            const float blend = (type - 0.25f) * 4.0f;
            return out.lowpass * (1.0f - blend) + out.bandpass * blend;
        } else if (type < 0.75f) {
            // BP to HP
            const float blend = (type - 0.5f) * 4.0f;
            return out.bandpass * (1.0f - blend) + out.highpass * blend;
        } else {
            // HP to Notch
            const float blend = (type - 0.75f) * 4.0f;
            return out.highpass * (1.0f - blend) + out.notch * blend;
        }
    }
    
    void flushModulationStates() noexcept {
        if (++modFlushCounter >= 512) {
            modFlushCounter = 0;
            lfoPhase = flushDenorm(lfoPhase);
            
            // Flush a section of noise table
            const size_t flushStart = noiseIndex & ~31; // Align to 32
            for (size_t i = 0; i < 32 && (flushStart + i) < NOISE_TABLE_SIZE; ++i) {
                noiseTable[flushStart + i] = flushDenorm(noiseTable[flushStart + i]);
            }
        }
    }
};

// ============================================================================
// Public interface
// ============================================================================
EnvelopeFilter::EnvelopeFilter() : pimpl(std::make_unique<Impl>()) {
    // Initialize parameters with default values
    pimpl->params.sensitivity.reset(0.5f);
    pimpl->params.attack.reset(0.3f);
    pimpl->params.release.reset(0.5f);
    pimpl->params.range.reset(0.7f);
    pimpl->params.resonance.reset(0.5f);
    pimpl->params.filterType.reset(0.0f);
    pimpl->params.direction.reset(1.0f);
    pimpl->params.mix.reset(1.0f);
}

EnvelopeFilter::~EnvelopeFilter() = default;

void EnvelopeFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = sampleRate;
    pimpl->blockSize = samplesPerBlock;
    
    // Prepare parameter smoothers with individual times
    pimpl->params.sensitivity.prepare(sampleRate, 20.0f);  // Fast for responsiveness
    pimpl->params.attack.prepare(sampleRate, pimpl->params.attackSmoothMs);
    pimpl->params.release.prepare(sampleRate, pimpl->params.releaseSmoothMs);
    pimpl->params.range.prepare(sampleRate, pimpl->params.filterSmoothMs);
    pimpl->params.resonance.prepare(sampleRate, pimpl->params.resonanceSmoothMs);
    pimpl->params.filterType.prepare(sampleRate, pimpl->params.morphSmoothMs);
    pimpl->params.direction.prepare(sampleRate, 200.0f);   // Slow for smooth direction changes
    pimpl->params.mix.prepare(sampleRate, pimpl->params.mixSmoothMs);
    
    // Pre-generate noise table for analog warmth
    pimpl->noiseTable.resize(pimpl->NOISE_TABLE_SIZE);
    for (auto& sample : pimpl->noiseTable) {
        sample = pimpl->noiseDist(pimpl->rng) * 0.0001f; // Very subtle noise
    }
    pimpl->noiseIndex = 0;
    
    // Prepare channels
    for (auto& ch : pimpl->channels) {
        ch.prepare(sampleRate);
    }
    
    // Allocate work buffers
    pimpl->envelopeBuffer.resize(samplesPerBlock);
    pimpl->cutoffBuffer.resize(samplesPerBlock);
    
    // Setup oversampling if needed
    if (pimpl->oversampleFactor > 1) {
        const int stages = static_cast<int>(std::log2(pimpl->oversampleFactor));
        for (int ch = 0; ch < 2; ++ch) {
            pimpl->oversamplers[ch] = std::make_unique<juce::dsp::Oversampling<float>>(
                1, stages,
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                true, false
            );
            pimpl->oversamplers[ch]->initProcessing(samplesPerBlock);
        }
    }
    
    reset();
}

void EnvelopeFilter::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
    
    pimpl->lfoPhase = 0.0f;
    pimpl->modFlushCounter = 0;
    
    if (pimpl->oversampleFactor > 1) {
        for (auto& os : pimpl->oversamplers) {
            if (os) os->reset();
        }
    }
}

void EnvelopeFilter::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    // Branchless path for oversampleFactor == 1
    if (pimpl->oversampleFactor == 1) {
        // Direct processing - no oversampling overhead
        for (int ch = 0; ch < numChannels; ++ch) {
            pimpl->processChannel(buffer.getWritePointer(ch), numSamples, ch);
        }
    } else if (pimpl->oversamplers[0]) {
        // Oversampled processing - wrap pointers once per block
        for (int ch = 0; ch < numChannels; ++ch) {
            auto channelBlock = juce::dsp::AudioBlock<float>(
                buffer.getArrayOfWritePointers() + ch, 1, numSamples);
            
            auto oversampledBlock = pimpl->oversamplers[ch]->processSamplesUp(channelBlock);
            float* data = oversampledBlock.getChannelPointer(0);
            const int oversampledSamples = static_cast<int>(oversampledBlock.getNumSamples());
            
            pimpl->processChannel(data, oversampledSamples, ch);
            
            pimpl->oversamplers[ch]->processSamplesDown(channelBlock);
        }
    }
    
    scrubBuffer(buffer);
}

void EnvelopeFilter::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        switch (static_cast<ParamID>(id)) {
            case ParamID::Sensitivity:
                pimpl->params.sensitivity.setTarget(value);
                break;
            case ParamID::Attack:
                pimpl->params.attack.setTarget(value);
                break;
            case ParamID::Release:
                pimpl->params.release.setTarget(value);
                break;
            case ParamID::Range:
                pimpl->params.range.setTarget(value);
                break;
            case ParamID::Resonance:
                pimpl->params.resonance.setTarget(value);
                break;
            case ParamID::FilterType:
                pimpl->params.filterType.setTarget(value);
                break;
            case ParamID::Direction:
                pimpl->params.direction.setTarget(value);
                break;
            case ParamID::Mix:
                pimpl->params.mix.setTarget(value);
                break;
        }
    }
}

juce::String EnvelopeFilter::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::Sensitivity: return "Sensitivity";
        case ParamID::Attack:      return "Attack";
        case ParamID::Release:     return "Release";
        case ParamID::Range:       return "Range";
        case ParamID::Resonance:   return "Resonance";
        case ParamID::FilterType:  return "Filter";
        case ParamID::Direction:   return "Direction";
        case ParamID::Mix:         return "Mix";
        default:                   return "";
    }
}

void EnvelopeFilter::setAnalogMode(bool enable) {
    pimpl->analogMode = enable;
}

void EnvelopeFilter::setOversamplingFactor(int factor) {
    pimpl->oversampleFactor = std::clamp(factor, 1, 8);
}
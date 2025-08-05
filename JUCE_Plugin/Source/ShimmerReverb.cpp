// ShimmerReverb.cpp - Final Platinum-spec implementation with all refinements
#include "ShimmerReverb.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <immintrin.h>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <complex>

// Platform-specific optimizations
#ifdef _WIN32
    #define ALWAYS_INLINE __forceinline
#else
    #define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

// ============================================================================
// Constants and helpers
// ============================================================================
namespace {
    constexpr int FFT_SIZE = 4096;
    constexpr int OVERLAP_FACTOR = 4;
    constexpr int HOP_SIZE = FFT_SIZE / OVERLAP_FACTOR;
    constexpr int MAX_CHANNELS = 8;
    constexpr int NUM_DIFFUSERS = 8;
    constexpr int NUM_DELAY_LINES = 16;
    constexpr double TWO_PI_D = 6.283185307179586476925286766559;
    constexpr double PI_D = 3.1415926535897932384626433832795;
    constexpr double DENORM_THR = 1e-25;  // Shared denormal threshold
    
    // Denormal prevention with global FTZ/DAZ
    struct DenormGuard {
        DenormGuard() {
#ifdef __SSE__
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
        }
    } g_denormGuard;
    
    // Unified denormal flusher
    template<typename T>
    ALWAYS_INLINE T flushDenorm(T value) noexcept {
#ifdef __SSE2__
        return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(static_cast<float>(value)), 
                                       _mm_set_ss(static_cast<float>(DENORM_THR))));
#else
        return std::fabs(value) < DENORM_THR ? T(0) : value;
#endif
    }
    
    // Branchless circular increment
    ALWAYS_INLINE size_t wrapIncrement(size_t pos, size_t size) noexcept {
        return (pos + 1 == size) ? 0 : pos + 1;
    }
    
    // SIMD-aligned allocation helper
    template<typename T>
    using AlignedVector = std::vector<T, juce::HeapBlockAllocator<T, 32>>;
}

// ============================================================================
// Parameter smoother with configurable time constants
// ============================================================================
class ParameterSmoother {
public:
    ParameterSmoother() = default;
    
    void prepare(double sampleRate, float smoothTimeMs) noexcept {
        this->sampleRate = sampleRate;
        setSmoothTime(smoothTimeMs);
        current = target.load(std::memory_order_acquire);
    }
    
    void setSmoothTime(float smoothTimeMs) noexcept {
        const double tau = smoothTimeMs * 0.001;
        coeff = static_cast<float>(std::exp(-1.0 / (tau * sampleRate)));
    }
    
    void setTarget(float newTarget) noexcept {
        target.store(newTarget, std::memory_order_release);
    }
    
    ALWAYS_INLINE float process() noexcept {
        const float t = target.load(std::memory_order_acquire);
        current += (1.0f - coeff) * (t - current);
        return flushDenorm(current);
    }
    
    void reset(float value) noexcept {
        current = value;
        target.store(value, std::memory_order_release);
    }
    
private:
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float coeff{0.999f};
    double sampleRate{44100.0};
};

// ============================================================================
// Recursive sine oscillator (no trig calls)
// ============================================================================
class RecursiveOscillator {
public:
    void setFrequency(float freq, double sampleRate) noexcept {
        const double omega = TWO_PI_D * freq / sampleRate;
        const double s = std::sin(omega);
        const double c = std::cos(omega);
        
        // Initialize recursion coefficients
        coeff1 = static_cast<float>(2.0 * c);
        coeff2 = -1.0f;
        
        // Initialize states
        y1 = static_cast<float>(s);
        y2 = 0.0f;
    }
    
    ALWAYS_INLINE float process() noexcept {
        const float y0 = coeff1 * y1 + coeff2 * y2;
        y2 = y1;
        y1 = y0;
        
        // Periodic renormalization to prevent drift
        if (++normCounter >= 1024) {
            normCounter = 0;
            const float mag = std::sqrt(y1 * y1 + y2 * y2);
            if (mag > 0.0f) {
                const float norm = 1.0f / mag;
                y1 *= norm;
                y2 *= norm;
            }
        }
        
        return y1;
    }
    
    void reset() noexcept {
        y1 = 0.0f;
        y2 = 0.0f;
        normCounter = 0;
    }
    
private:
    float coeff1{0.0f}, coeff2{0.0f};
    float y1{0.0f}, y2{0.0f};
    int normCounter{0};
};

// ============================================================================
// All-pass diffuser with modulation (optimized)
// ============================================================================
class ModulatedAllPass {
public:
    void prepare(size_t maxDelay, double sampleRate) noexcept {
        buffer.resize(maxDelay);
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        this->maxDelay = maxDelay;
        this->sampleRate = sampleRate;
    }
    
    void setParameters(float feedback, float modRate, float modDepth) noexcept {
        this->feedback = std::clamp(feedback, -0.99f, 0.99f);
        
        // Clamp mod rate to prevent aliasing
        modRate = std::clamp(modRate, 0.01f, static_cast<float>(sampleRate * 0.1));
        this->modDepth = std::clamp(modDepth, 0.0f, 1.0f);
        
        lfo.setFrequency(modRate, sampleRate);
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        // Get LFO value from recursive oscillator
        const float lfoVal = lfo.process();
        
        // Modulated delay time
        const float delaySamples = (maxDelay * 0.7f) + (maxDelay * 0.3f * modDepth * lfoVal);
        const int delayInt = static_cast<int>(delaySamples);
        const float frac = delaySamples - delayInt;
        
        // Fractional delay read with branchless wrap
        int readPos1 = writePos - delayInt;
        if (readPos1 < 0) readPos1 += maxDelay;
        
        int readPos2 = readPos1 - 1;
        if (readPos2 < 0) readPos2 += maxDelay;
        
        const float delayed = buffer[readPos1] + frac * (buffer[readPos2] - buffer[readPos1]);
        
        // All-pass processing
        const float output = -input + delayed;
        buffer[writePos] = input + feedback * delayed;
        writePos = wrapIncrement(writePos, maxDelay);
        
        return flushDenorm(output);
    }
    
    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        lfo.reset();
    }
    
private:
    AlignedVector<float> buffer;
    RecursiveOscillator lfo;
    size_t writePos{0};
    size_t maxDelay{0};
    float feedback{0.7f};
    float modDepth{0.3f};
    double sampleRate{44100.0};
};

// ============================================================================
// High-quality pitch shifter using phase vocoder (optimized)
// ============================================================================
class PitchShifter {
public:
    void prepare(double sampleRate) noexcept {
        this->sampleRate = sampleRate;
        
        // Initialize FFT
        fftOrder = static_cast<int>(std::log2(FFT_SIZE));
        fft = std::make_unique<juce::dsp::FFT>(fftOrder);
        
        // Allocate aligned buffers
        inputBuffer.resize(FFT_SIZE * 2);
        outputBuffer.resize(FFT_SIZE * 2);
        overlapBuffer.resize(FFT_SIZE);
        fftData.resize(FFT_SIZE * 2);
        window.resize(FFT_SIZE);
        
        // Create Hann window
        for (int i = 0; i < FFT_SIZE; ++i) {
            window[i] = 0.5f * (1.0f - std::cos(TWO_PI_D * i / (FFT_SIZE - 1)));
        }
        
        // Calculate window normalization once
        windowSum = 0.0f;
        for (int i = 0; i < FFT_SIZE; i += HOP_SIZE) {
            windowSum += window[i] * window[i];
        }
        windowNorm = OVERLAP_FACTOR / windowSum;
        
        reset();
    }
    
    void setPitchRatio(float ratio) noexcept {
        pitchRatio = std::clamp(ratio, 0.25f, 4.0f);
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        // Shift input buffer (optimized rotation)
        std::rotate(inputBuffer.begin(), inputBuffer.begin() + 1, inputBuffer.end());
        inputBuffer.back() = input;
        
        float output = outputBuffer.front() + overlapBuffer.front();
        
        // Shift output buffers
        std::rotate(outputBuffer.begin(), outputBuffer.begin() + 1, outputBuffer.end());
        std::rotate(overlapBuffer.begin(), overlapBuffer.begin() + 1, overlapBuffer.end());
        outputBuffer.back() = 0.0f;
        overlapBuffer.back() = 0.0f;
        
        if (++hopCounter >= HOP_SIZE) {
            hopCounter = 0;
            processFrame();
        }
        
        return flushDenorm(output);
    }
    
    void reset() noexcept {
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        std::fill(overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
        std::fill(lastPhase.begin(), lastPhase.end(), 0.0);
        std::fill(phaseAccum.begin(), phaseAccum.end(), 0.0);
        hopCounter = 0;
        denormCounter = 0;
    }
    
private:
    void processFrame() noexcept {
        // Apply window and prepare FFT data
#ifdef __AVX__
        const __m256* pInput = reinterpret_cast<const __m256*>(inputBuffer.data());
        const __m256* pWindow = reinterpret_cast<const __m256*>(window.data());
        __m256* pFFT = reinterpret_cast<__m256*>(fftData.data());
        
        for (int i = 0; i < FFT_SIZE / 8; ++i) {
            pFFT[i] = _mm256_mul_ps(pInput[i], pWindow[i]);
        }
#else
        for (int i = 0; i < FFT_SIZE; ++i) {
            fftData[i] = inputBuffer[i] * window[i];
        }
#endif
        
        // Forward FFT
        fft->performRealOnlyForwardTransform(fftData.data());
        
        // Process spectrum
        processSpectrum();
        
        // Inverse FFT
        fft->performRealOnlyInverseTransform(fftData.data());
        
        // Window and overlap-add with proper scaling
        const float scale = windowNorm / FFT_SIZE;
        
#ifdef __AVX__
        const __m256 vScale = _mm256_set1_ps(scale);
        const __m256* pFFTOut = reinterpret_cast<const __m256*>(fftData.data());
        __m256* pOverlap = reinterpret_cast<__m256*>(overlapBuffer.data());
        
        for (int i = 0; i < FFT_SIZE / 8; ++i) {
            __m256 scaled = _mm256_mul_ps(_mm256_mul_ps(pFFTOut[i], pWindow[i]), vScale);
            pOverlap[i] = _mm256_add_ps(pOverlap[i], scaled);
        }
#else
        for (int i = 0; i < FFT_SIZE; ++i) {
            overlapBuffer[i] += fftData[i] * window[i] * scale;
        }
#endif
        
        // Periodic denormal flush for all accumulator states
        if (++denormCounter >= 256) {
            denormCounter = 0;
            for (auto& p : phaseAccum) p = flushDenorm(p);
            for (auto& p : lastPhase) p = flushDenorm(p);
            for (auto& v : overlapBuffer) v = flushDenorm(v);
            for (auto& v : outputBuffer) v = flushDenorm(v);
        }
    }
    
    void processSpectrum() noexcept {
        const double binFreq = sampleRate / FFT_SIZE;
        const double expectedPhaseInc = TWO_PI_D * HOP_SIZE / FFT_SIZE;
        
        // Clear target spectrum
        std::fill(targetReal.begin(), targetReal.end(), 0.0f);
        std::fill(targetImag.begin(), targetImag.end(), 0.0f);
        
        for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
            const float real = fftData[bin * 2];
            const float imag = fftData[bin * 2 + 1];
            
            // Calculate magnitude and phase
            const float mag = std::sqrt(real * real + imag * imag);
            const double phase = std::atan2(imag, real);
            
            // Phase difference and unwrapping
            double phaseDiff = phase - lastPhase[bin];
            lastPhase[bin] = phase;
            
            // Branchless phase unwrapping
            const double twoPi = TWO_PI_D;
            phaseDiff = phaseDiff - twoPi * std::round(phaseDiff / twoPi);
            
            // True frequency
            const double deviation = phaseDiff - expectedPhaseInc * bin;
            const double trueFreq = binFreq * bin + deviation * sampleRate / (twoPi * HOP_SIZE);
            
            // Pitch shift
            const int targetBin = static_cast<int>(bin * pitchRatio + 0.5);
            if (targetBin > 0 && targetBin <= FFT_SIZE / 2) {
                // Accumulate phase
                phaseAccum[targetBin] += twoPi * trueFreq * pitchRatio * HOP_SIZE / sampleRate;
                
                // Reconstruct with shifted frequency
                targetReal[targetBin] += mag * std::cos(phaseAccum[targetBin]);
                targetImag[targetBin] += mag * std::sin(phaseAccum[targetBin]);
            }
        }
        
        // Copy back to FFT data
        for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
            fftData[bin * 2] = targetReal[bin];
            fftData[bin * 2 + 1] = targetImag[bin];
        }
    }
    
    // Members
    std::unique_ptr<juce::dsp::FFT> fft;
    int fftOrder{0};
    
    AlignedVector<float> inputBuffer;
    AlignedVector<float> outputBuffer;
    AlignedVector<float> overlapBuffer;
    AlignedVector<float> fftData;
    AlignedVector<float> window;
    
    std::array<double, FFT_SIZE/2 + 1> lastPhase{};
    std::array<double, FFT_SIZE/2 + 1> phaseAccum{};
    std::array<float, FFT_SIZE/2 + 1> targetReal{};
    std::array<float, FFT_SIZE/2 + 1> targetImag{};
    
    float pitchRatio{2.0f};
    float windowSum{1.0f};
    float windowNorm{1.0f};
    int hopCounter{0};
    int denormCounter{0};
    double sampleRate{44100.0};
};

// ============================================================================
// Feedback Delay Network with optimized Hadamard mixing
// ============================================================================
class FeedbackDelayNetwork {
public:
    void prepare(double sampleRate) noexcept {
        this->sampleRate = sampleRate;
        
        // Prime number delay lengths for inharmonic response
        const std::array<int, NUM_DELAY_LINES> primes = {
            1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049,
            1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097
        };
        
        // Scale to sample rate
        const float scale = sampleRate / 44100.0f;
        
        for (int i = 0; i < NUM_DELAY_LINES; ++i) {
            int length = static_cast<int>(primes[i] * scale);
            delayLines[i].resize(length);
            std::fill(delayLines[i].begin(), delayLines[i].end(), 0.0f);
            writePos[i] = 0;
            
            // Damping filters
            dampingFilters[i].reset();
            dampingFilters[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
                sampleRate, 8000.0f);
        }
        
        // Initialize Hadamard matrix
        initializeHadamardMatrix();
        
        // Precompute normalization factor
        hadamardNorm = 1.0f / std::sqrt(static_cast<float>(NUM_DELAY_LINES));
    }
    
    void process(const float* input, float* output, float feedback, float damping, float size) noexcept {
        alignas(32) std::array<float, NUM_DELAY_LINES> delayOutputs;
        alignas(32) std::array<float, NUM_DELAY_LINES> mixed;
        
        // Read from delay lines
        for (int i = 0; i < NUM_DELAY_LINES; ++i) {
            delayOutputs[i] = delayLines[i][writePos[i]];
        }
        
        // Apply optimized Hadamard mixing
#ifdef __AVX__
        // Process in blocks of 8 for AVX
        for (int i = 0; i < NUM_DELAY_LINES; i += 8) {
            __m256 sum1 = _mm256_setzero_ps();
            __m256 sum2 = _mm256_setzero_ps();
            
            for (int j = 0; j < NUM_DELAY_LINES; j += 8) {
                __m256 delays1 = _mm256_load_ps(&delayOutputs[j]);
                __m256 matrix1 = _mm256_load_ps(&hadamardMatrix[i][j]);
                sum1 = _mm256_add_ps(sum1, _mm256_mul_ps(delays1, matrix1));
                
                if (j + 8 < NUM_DELAY_LINES) {
                    __m256 delays2 = _mm256_load_ps(&delayOutputs[j + 8]);
                    __m256 matrix2 = _mm256_load_ps(&hadamardMatrix[i][j + 8]);
                    sum2 = _mm256_add_ps(sum2, _mm256_mul_ps(delays2, matrix2));
                }
            }
            
            __m256 total = _mm256_add_ps(sum1, sum2);
            _mm256_store_ps(&mixed[i], total);
        }
#else
        // Scalar fallback with cache-friendly access pattern
        for (int i = 0; i < NUM_DELAY_LINES; ++i) {
            float sum = 0.0f;
            for (int j = 0; j < NUM_DELAY_LINES; ++j) {
                sum += hadamardMatrix[i][j] * delayOutputs[j];
            }
            mixed[i] = sum;
        }
#endif
        
        // Apply normalization and energy compensation
        const float energyComp = 0.85f + 0.15f * size; // Diagonal compensation
        
        *output = 0.0f;
        
        for (int i = 0; i < NUM_DELAY_LINES; ++i) {
            // Normalize Hadamard output
            mixed[i] *= hadamardNorm;
            
            // Apply energy compensation
            mixed[i] = mixed[i] * (1.0f - energyComp) + delayOutputs[i] * energyComp;
            
            // Apply damping
            mixed[i] = dampingFilters[i].processSample(mixed[i]);
            
            // Size-dependent feedback
            float lineFeedback = feedback * (0.9f + 0.1f * size);
            
            // Write to delay line with branchless wrap
            delayLines[i][writePos[i]] = *input * 0.25f + mixed[i] * lineFeedback;
            writePos[i] = wrapIncrement(writePos[i], delayLines[i].size());
            
            // Sum to output
            *output += delayOutputs[i];
        }
        
        *output = flushDenorm(*output * 0.25f);
    }
    
    void reset() noexcept {
        for (int i = 0; i < NUM_DELAY_LINES; ++i) {
            std::fill(delayLines[i].begin(), delayLines[i].end(), 0.0f);
            dampingFilters[i].reset();
        }
        denormFlushCounter = 0;
    }
    
    void flushDenormals() noexcept {
        if (++denormFlushCounter >= 256) {
            denormFlushCounter = 0;
            for (auto& line : delayLines) {
                for (auto& sample : line) {
                    sample = flushDenorm(sample);
                }
            }
        }
    }
    
private:
    void initializeHadamardMatrix() noexcept {
        // Fast Walsh-Hadamard transform pattern
        hadamardMatrix[0][0] = 1.0f;
        
        for (int size = 1; size < NUM_DELAY_LINES; size *= 2) {
            for (int i = 0; i < size && i + size < NUM_DELAY_LINES; ++i) {
                for (int j = 0; j < size && j + size < NUM_DELAY_LINES; ++j) {
                    hadamardMatrix[i + size][j] = hadamardMatrix[i][j];
                    hadamardMatrix[i][j + size] = hadamardMatrix[i][j];
                    hadamardMatrix[i + size][j + size] = -hadamardMatrix[i][j];
                }
            }
        }
    }
    
    std::array<AlignedVector<float>, NUM_DELAY_LINES> delayLines;
    std::array<int, NUM_DELAY_LINES> writePos;
    std::array<juce::dsp::IIR::Filter<float>, NUM_DELAY_LINES> dampingFilters;
    alignas(32) std::array<std::array<float, NUM_DELAY_LINES>, NUM_DELAY_LINES> hadamardMatrix;
    float hadamardNorm{1.0f};
    double sampleRate{44100.0};
    int denormFlushCounter{0};
};

// ============================================================================
// Crossfade helper for smooth freeze transitions
// ============================================================================
class CrossfadeState {
    int counter{0};
    int duration{0};
    
public:
    void trigger(int fadeFrames) noexcept {
        counter = duration = fadeFrames;
    }
    
    ALWAYS_INLINE float getWeight() noexcept {
        if (counter <= 0) return 1.0f;
        const float weight = static_cast<float>(counter) / duration;
        counter--;
        return weight;
    }
    
    ALWAYS_INLINE bool isActive() const noexcept {
        return counter > 0;
    }
    
    void reset() noexcept {
        counter = 0;
    }
};

// ============================================================================
// Main implementation
// ============================================================================
struct ShimmerReverb::Impl {
    // Configurable parameter smoothers
    struct {
        ParameterSmoother size;
        ParameterSmoother shimmer;
        ParameterSmoother pitch;
        ParameterSmoother damping;
        ParameterSmoother diffusion;
        ParameterSmoother modulation;
        ParameterSmoother predelay;
        ParameterSmoother width;
        ParameterSmoother freeze;
        ParameterSmoother mix;
    } params;
    
    // Processing components per channel
    struct ChannelProcessor {
        PitchShifter pitchShifter;
        std::array<ModulatedAllPass, NUM_DIFFUSERS> diffusers;
        FeedbackDelayNetwork fdn;
        
        AlignedVector<float> predelayBuffer;
        int predelayWritePos{0};
        
        juce::dsp::IIR::Filter<float> inputHighpass;
        juce::dsp::IIR::Filter<float> shimmerHighpass;
        
        CrossfadeState freezeCrossfade;
        
        void prepare(double sampleRate) {
            pitchShifter.prepare(sampleRate);
            
            // Initialize diffusers with increasing sizes
            for (int i = 0; i < NUM_DIFFUSERS; ++i) {
                int size = static_cast<int>((50 + i * 30) * sampleRate / 44100.0);
                diffusers[i].prepare(size, sampleRate);
                diffusers[i].setParameters(0.7f, 0.1f + i * 0.05f, 0.3f);
            }
            
            fdn.prepare(sampleRate);
            
            // Pre-allocate predelay up to 500ms
            predelayBuffer.resize(static_cast<int>(0.5 * sampleRate));
            std::fill(predelayBuffer.begin(), predelayBuffer.end(), 0.0f);
            
            // High-pass filters
            inputHighpass.reset();
            inputHighpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(
                sampleRate, 20.0f);
                
            shimmerHighpass.reset();
            shimmerHighpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(
                sampleRate, 200.0f);
        }
        
        void reset() {
            pitchShifter.reset();
            for (auto& d : diffusers) d.reset();
            fdn.reset();
            std::fill(predelayBuffer.begin(), predelayBuffer.end(), 0.0f);
            predelayWritePos = 0;
            inputHighpass.reset();
            shimmerHighpass.reset();
            freezeCrossfade.reset();
        }
    };
    
    std::array<ChannelProcessor, MAX_CHANNELS> channels;
    
    // Freeze state - pre-allocated buffers
    std::atomic<bool> freezeActive{false};
    std::atomic<bool> freezeJustActivated{false};
    std::array<AlignedVector<float>, MAX_CHANNELS> freezeBuffers;
    std::array<int, MAX_CHANNELS> freezeWritePos{};
    std::array<int, MAX_CHANNELS> freezeReadPos{};
    
    // Quality settings
    QualityMode qualityMode{QualityMode::Premium};
    int oversampleFactor{4};
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, MAX_CHANNELS> oversamplers;
    
    double sampleRate{44100.0};
    int numActiveChannels{2};
    
    // Configurable smooth times
    struct SmoothTimes {
        float size{50.0f};
        float shimmer{30.0f};
        float pitch{50.0f};
        float damping{30.0f};
        float diffusion{40.0f};
        float modulation{30.0f};
        float predelay{10.0f};    // Faster for predelay
        float width{30.0f};
        float freeze{5.0f};        // Very fast for freeze
        float mix{20.0f};
    } smoothTimes;
    
    void processChannel(float* data, int numSamples, int channelIndex) {
        auto& ch = channels[channelIndex];
        
        // Get smoothed parameters
        const float sizeVal = params.size.process();
        const float shimmerVal = params.shimmer.process();
        const float pitchVal = params.pitch.process();
        const float dampingVal = params.damping.process();
        const float diffusionVal = params.diffusion.process();
        const float modulationVal = params.modulation.process();
        const float predelayVal = params.predelay.process();
        const float mixVal = params.mix.process();
        const float freezeVal = params.freeze.process();
        
        // Update freeze state with crossfade
        const bool shouldFreeze = freezeVal > 0.5f;
        bool justActivated = false;
        
        if (shouldFreeze && !freezeActive.load()) {
            freezeActive.store(true);
            freezeJustActivated.store(true);
            justActivated = true;
            ch.freezeCrossfade.trigger(HOP_SIZE);
        } else if (!shouldFreeze) {
            if (freezeActive.load()) {
                ch.freezeCrossfade.trigger(HOP_SIZE);
            }
            freezeActive.store(false);
        }
        
        // Convert parameters
        const float pitchRatio = std::pow(2.0f, (pitchVal * 2.0f - 1.0f)); // -1 to +1 octave
        const int predelaySamples = static_cast<int>(predelayVal * 0.5f * sampleRate);
        
        ch.pitchShifter.setPitchRatio(pitchRatio);
        
        // Update diffuser parameters
        for (int i = 0; i < NUM_DIFFUSERS; ++i) {
            ch.diffusers[i].setParameters(
                0.5f + diffusionVal * 0.4f,
                0.1f + modulationVal * 0.5f,
                modulationVal * 0.5f
            );
        }
        
        // Process audio
        for (int i = 0; i < numSamples; ++i) {
            float input = data[i];
            const float dry = input;
            
            // Input high-pass to remove DC
            input = ch.inputHighpass.processSample(input);
            
            // Handle freeze with crossfade
            float freezeSample = 0.0f;
            if (freezeActive.load() || ch.freezeCrossfade.isActive()) {
                if (justActivated) {
                    captureFreezeSample(channelIndex, input);
                }
                freezeSample = getFreezeSample(channelIndex);
                
                if (ch.freezeCrossfade.isActive()) {
                    const float alpha = ch.freezeCrossfade.getWeight();
                    input = shouldFreeze ? 
                        (alpha * input + (1.0f - alpha) * freezeSample) :
                        (alpha * freezeSample + (1.0f - alpha) * input);
                } else if (freezeActive.load()) {
                    input = freezeSample;
                }
            }
            
            // Predelay with branchless wrap
            int readPos = ch.predelayWritePos - predelaySamples;
            if (readPos < 0) readPos += ch.predelayBuffer.size();
            
            const float predelayed = ch.predelayBuffer[readPos];
            ch.predelayBuffer[ch.predelayWritePos] = input;
            ch.predelayWritePos = wrapIncrement(ch.predelayWritePos, ch.predelayBuffer.size());
            
            // Early diffusion
            float diffused = predelayed;
            for (auto& diffuser : ch.diffusers) {
                diffused = diffuser.process(diffused);
            }
            
            // Shimmer path
            float shimmerSignal = 0.0f;
            if (shimmerVal > 0.01f) {
                shimmerSignal = ch.pitchShifter.process(diffused * shimmerVal);
                shimmerSignal = ch.shimmerHighpass.processSample(shimmerSignal);
            }
            
            // Main reverb
            float reverbInput = diffused * (1.0f - shimmerVal * 0.5f) + shimmerSignal;
            float reverbOutput = 0.0f;
            ch.fdn.process(&reverbInput, &reverbOutput, 
                          sizeVal * 0.98f, dampingVal, sizeVal);
            
            // Mix with proper denormal handling
            data[i] = flushDenorm(dry * (1.0f - mixVal) + reverbOutput * mixVal);
        }
        
        // Periodic FDN denormal flush
        ch.fdn.flushDenormals();
        
        // Clear the just-activated flag after processing
        if (justActivated) {
            freezeJustActivated.store(false);
        }
    }
    
    void captureFreezeSample(int channelIndex, float sample) {
        if (channelIndex >= freezeBuffers.size()) return;
        
        auto& buffer = freezeBuffers[channelIndex];
        buffer[freezeWritePos[channelIndex]] = sample;
        freezeWritePos[channelIndex] = wrapIncrement(freezeWritePos[channelIndex], buffer.size());
    }
    
    float getFreezeSample(int channelIndex) {
        if (channelIndex >= freezeBuffers.size()) return 0.0f;
        
        auto& buffer = freezeBuffers[channelIndex];
        float sample = buffer[freezeReadPos[channelIndex]];
        freezeReadPos[channelIndex] = wrapIncrement(freezeReadPos[channelIndex], buffer.size());
        return sample;
    }
};

// ============================================================================
// Public interface
// ============================================================================
ShimmerReverb::ShimmerReverb() : pimpl(std::make_unique<Impl>()) {
    // Set default parameter values
    pimpl->params.size.reset(0.5f);
    pimpl->params.shimmer.reset(0.3f);
    pimpl->params.pitch.reset(0.667f);
    pimpl->params.damping.reset(0.5f);
    pimpl->params.diffusion.reset(0.7f);
    pimpl->params.modulation.reset(0.2f);
    pimpl->params.predelay.reset(0.1f);
    pimpl->params.width.reset(1.0f);
    pimpl->params.freeze.reset(0.0f);
    pimpl->params.mix.reset(0.3f);
}

ShimmerReverb::~ShimmerReverb() = default;

void ShimmerReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = sampleRate;
    
    // Prepare parameter smoothers with configurable times
    pimpl->params.size.prepare(sampleRate, pimpl->smoothTimes.size);
    pimpl->params.shimmer.prepare(sampleRate, pimpl->smoothTimes.shimmer);
    pimpl->params.pitch.prepare(sampleRate, pimpl->smoothTimes.pitch);
    pimpl->params.damping.prepare(sampleRate, pimpl->smoothTimes.damping);
    pimpl->params.diffusion.prepare(sampleRate, pimpl->smoothTimes.diffusion);
    pimpl->params.modulation.prepare(sampleRate, pimpl->smoothTimes.modulation);
    pimpl->params.predelay.prepare(sampleRate, pimpl->smoothTimes.predelay);
    pimpl->params.width.prepare(sampleRate, pimpl->smoothTimes.width);
    pimpl->params.freeze.prepare(sampleRate, pimpl->smoothTimes.freeze);
    pimpl->params.mix.prepare(sampleRate, pimpl->smoothTimes.mix);
    
    // Prepare channels
    for (auto& ch : pimpl->channels) {
        ch.prepare(sampleRate);
    }
    
    // Pre-allocate freeze buffers (never resize in RT thread)
    const int freezeBufferSize = static_cast<int>(sampleRate * 2.0); // 2 seconds
    for (auto& buffer : pimpl->freezeBuffers) {
        buffer.resize(freezeBufferSize);
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
    std::fill(pimpl->freezeWritePos.begin(), pimpl->freezeWritePos.end(), 0);
    std::fill(pimpl->freezeReadPos.begin(), pimpl->freezeReadPos.end(), 0);
    
    // Setup oversampling based on quality mode
    if (pimpl->oversampleFactor > 1) {
        for (int ch = 0; ch < MAX_CHANNELS; ++ch) {
            int stages = static_cast<int>(std::log2(pimpl->oversampleFactor));
            pimpl->oversamplers[ch] = std::make_unique<juce::dsp::Oversampling<float>>(
                1, stages,
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                true, true
            );
            pimpl->oversamplers[ch]->initProcessing(samplesPerBlock);
        }
    }
    
    reset();
}

void ShimmerReverb::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
    
    pimpl->freezeActive.store(false);
    pimpl->freezeJustActivated.store(false);
    
    // Clear freeze buffers
    for (auto& buffer : pimpl->freezeBuffers) {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
    std::fill(pimpl->freezeWritePos.begin(), pimpl->freezeWritePos.end(), 0);
    std::fill(pimpl->freezeReadPos.begin(), pimpl->freezeReadPos.end(), 0);
    
    if (pimpl->oversampleFactor > 1) {
        for (auto& os : pimpl->oversamplers) {
            if (os) os->reset();
        }
    }
}

void ShimmerReverb::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0) return;
    
    pimpl->numActiveChannels = std::min(numChannels, MAX_CHANNELS);
    
    // Get width parameter for stereo processing
    const float width = pimpl->params.width.process();
    
    // Branch-free oversampling path selection
    if (pimpl->oversampleFactor == 1) {
        // Direct processing - no oversampling overhead
        for (int ch = 0; ch < pimpl->numActiveChannels; ++ch) {
            pimpl->processChannel(buffer.getWritePointer(ch), numSamples, ch);
        }
    } else if (pimpl->oversamplers[0]) {
        // Oversampled processing
        for (int ch = 0; ch < pimpl->numActiveChannels; ++ch) {
            auto channelBlock = juce::dsp::AudioBlock<float>(
                buffer.getArrayOfWritePointers() + ch, 1, numSamples);
            
            auto oversampledBlock = pimpl->oversamplers[ch]->processSamplesUp(channelBlock);
            float* data = oversampledBlock.getChannelPointer(0);
            int oversampledSamples = static_cast<int>(oversampledBlock.getNumSamples());
            
            pimpl->processChannel(data, oversampledSamples, ch);
            
            pimpl->oversamplers[ch]->processSamplesDown(channelBlock);
        }
    }
    
    // Apply stereo width processing
    if (pimpl->numActiveChannels >= 2) {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        
        if (width < 0.99f || width > 1.01f) {
            for (int i = 0; i < numSamples; ++i) {
                const float mid = (left[i] + right[i]) * 0.5f;
                const float side = (left[i] - right[i]) * 0.5f * width;
                left[i] = flushDenorm(mid + side);
                right[i] = flushDenorm(mid - side);
            }
        }
    }
}

void ShimmerReverb::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        switch (static_cast<ParamID>(id)) {
            case ParamID::Size:
                pimpl->params.size.setTarget(std::clamp(value, 0.0f, 1.0f));
                break;
            case ParamID::Shimmer:
                pimpl->params.shimmer.setTarget(std::clamp(value, 0.0f, 1.0f));
                break;
            case ParamID::Pitch:
                pimpl->params.pitch.setTarget(std::clamp(value, 0.0f, 1.0f));
                break;
            case ParamID::Damping:
                pimpl->params.damping.setTarget(std::clamp(value, 0.0f, 1.0f));
                break;
            case ParamID::Diffusion:
                pimpl->params.diffusion.setTarget(std::clamp(value, 0.0f, 1.0f));
                break;
            case ParamID::Modulation:
                pimpl->params.modulation.setTarget(std::clamp(value, 0.0f, 1.0f));
                break;
            case ParamID::Predelay:
                pimpl->params.predelay.setTarget(std::clamp(value, 0.0f, 1.0f));
                break;
            case ParamID::Width:
                pimpl->params.width.setTarget(std::clamp(value, 0.0f, 2.0f));
                break;
            case ParamID::Freeze:
                pimpl->params.freeze.setTarget(value > 0.5f ? 1.0f : 0.0f);
                break;
            case ParamID::Mix:
                pimpl->params.mix.setTarget(std::clamp(value, 0.0f, 1.0f));
                break;
        }
    }
}

juce::String ShimmerReverb::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::Size:       return "Size";
        case ParamID::Shimmer:    return "Shimmer";
        case ParamID::Pitch:      return "Pitch";
        case ParamID::Damping:    return "Damping";
        case ParamID::Diffusion:  return "Diffusion";
        case ParamID::Modulation: return "Modulation";
        case ParamID::Predelay:   return "Predelay";
        case ParamID::Width:      return "Width";
        case ParamID::Freeze:     return "Freeze";
        case ParamID::Mix:        return "Mix";
        default:                  return "";
    }
}

void ShimmerReverb::setQualityMode(QualityMode mode) {
    pimpl->qualityMode = mode;
    
    switch (mode) {
        case QualityMode::Draft:
            pimpl->oversampleFactor = 1;
            break;
        case QualityMode::Standard:
            pimpl->oversampleFactor = 2;
            break;
        case QualityMode::Premium:
            pimpl->oversampleFactor = 4;
            break;
        case QualityMode::Platinum:
            pimpl->oversampleFactor = 8;
            break;
    }
}

void ShimmerReverb::setReverbType(int type) {
    // Future expansion - different reverb algorithms
    // This would be implemented off the RT thread with proper synchronization
}
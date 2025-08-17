#include "TransientShaper_Platinum.h"
#include <JuceHeader.h>

// Platform-specific SIMD includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif
#include <atomic>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Feature flags for compile-time optimization
#ifndef TRANSIENT_SHAPER_ENABLE_HILBERT
    #define TRANSIENT_SHAPER_ENABLE_HILBERT 1
#endif

#ifndef TRANSIENT_SHAPER_ENABLE_OVERSAMPLING
    #define TRANSIENT_SHAPER_ENABLE_OVERSAMPLING 1
#endif

#ifndef TRANSIENT_SHAPER_ENABLE_LOOKAHEAD
    #define TRANSIENT_SHAPER_ENABLE_LOOKAHEAD 1
#endif

// Enable FTZ/DAZ globally
static struct DenormGuard {
    DenormGuard() {
#if defined(__SSE__) && HAS_SIMD
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
} s_denormGuard;

// Denormal flush helper
template<typename T>
inline T flushDenorm(T v) noexcept {
    constexpr T tiny = static_cast<T>(1.0e-30);
    return std::fabs(v) < tiny ? static_cast<T>(0) : v;
}

// Custom IIR implementation to avoid JUCE conflicts
class CustomIIRCoefficients {
    public:
        static CustomIIRCoefficients makeHighPass(double sampleRate, double frequency) {
            CustomIIRCoefficients c;
            // Simple high-pass implementation
            double omega = 2.0 * M_PI * frequency / sampleRate;
            double cosOmega = std::cos(omega);
            double sinOmega = std::sin(omega);
            double alpha = sinOmega / (2.0 * 0.707); // Q = 0.707
            
            double b0 = (1.0 + cosOmega) / 2.0;
            double b1 = -(1.0 + cosOmega);
            double b2 = (1.0 + cosOmega) / 2.0;
            double a0 = 1.0 + alpha;
            double a1 = -2.0 * cosOmega;
            double a2 = 1.0 - alpha;
            
            c.coeffs[0] = b0 / a0;
            c.coeffs[1] = b1 / a0;
            c.coeffs[2] = b2 / a0;
            c.coeffs[3] = a1 / a0;
            c.coeffs[4] = a2 / a0;
            return c;
        }
        
        static CustomIIRCoefficients makeLowPass(double sampleRate, double frequency) {
            CustomIIRCoefficients c;
            // Simple low-pass implementation  
            double omega = 2.0 * M_PI * frequency / sampleRate;
            double cosOmega = std::cos(omega);
            double sinOmega = std::sin(omega);
            double alpha = sinOmega / (2.0 * 0.707); // Q = 0.707
            
            double b0 = (1.0 - cosOmega) / 2.0;
            double b1 = 1.0 - cosOmega;
            double b2 = (1.0 - cosOmega) / 2.0;
            double a0 = 1.0 + alpha;
            double a1 = -2.0 * cosOmega;
            double a2 = 1.0 - alpha;
            
            c.coeffs[0] = b0 / a0;
            c.coeffs[1] = b1 / a0;
            c.coeffs[2] = b2 / a0;
            c.coeffs[3] = a1 / a0;
            c.coeffs[4] = a2 / a0;
            return c;
        }
        
        double coeffs[5] = {0};
    };
    
    // Custom IIR filter
    class CustomIIRFilter {
    public:
        void setCoefficients(const CustomIIRCoefficients& newCoeffs) {
            coeffs = newCoeffs;
        }
        
        float processSample(float sample) {
            float output = coeffs.coeffs[0] * sample + 
                          coeffs.coeffs[1] * x1 + 
                          coeffs.coeffs[2] * x2 - 
                          coeffs.coeffs[3] * y1 - 
                          coeffs.coeffs[4] * y2;
            
            x2 = x1;
            x1 = sample;
            y2 = y1;
            y1 = output;
            
            return output;
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0f;
        }
        
    private:
        CustomIIRCoefficients coeffs;
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    };

// Lock-free parameter smoothing
class SmoothParam {
public:
    void setTarget(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
    }
    
    void setImmediate(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
        current = value;
        blockValue = value;
    }
    
    void setSmoothingTime(float milliseconds, double sampleRate) noexcept {
        float samples = static_cast<float>(milliseconds * 0.001 * sampleRate);
        smoothingCoeff = std::exp(-1.0f / samples);
    }
    
    void updateBlock() noexcept {
        float t = target.load(std::memory_order_relaxed);
        current += (t - current) * (1.0f - smoothingCoeff);
        current = flushDenorm(current);
        blockValue = current;
    }
    
    inline float getBlockValue() const noexcept {
        return blockValue;
    }

private:
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float blockValue{0.0f};
    float smoothingCoeff{0.99f};
};

// Professional envelope detector with multiple modes and block-rate optimization
class EnvelopeDetector {
public:
    enum Mode { Peak, RMS, Hilbert, Hybrid };
    
    // Function pointer for mode-specific processing
    using ProcessFunc = float (EnvelopeDetector::*)(float) noexcept;
    
    void prepare(double sampleRate) noexcept {
        fs = static_cast<float>(sampleRate);
        reset();
    }
    
    void setMode(Mode m) noexcept {
        if (mode != m) {
            mode = m;
            modeChanged = true;
        }
    }
    
    void setTimes(float attackMs, float releaseMs) noexcept {
        attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * fs));
        releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * fs));
    }
    
    // Block-rate cache update - call once per block
    void updateBlockCache() noexcept {
        if (modeChanged) {
            modeChanged = false;
            
            // Set function pointer for current mode
            switch (mode) {
                case Peak:
                    processFunc = &EnvelopeDetector::processPeak;
                    break;
                case RMS:
                    processFunc = &EnvelopeDetector::processRMS;
                    std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
                    rmsSum = 0.0f;
                    break;
                case Hilbert:
                    processFunc = &EnvelopeDetector::processHilbert;
                    std::fill(hilbertDelay.begin(), hilbertDelay.end(), 0.0f);
                    break;
                case Hybrid:
                    processFunc = &EnvelopeDetector::processHybrid;
                    break;
            }
        }
    }
    
    inline float process(float input) noexcept {
        // Single indirect call instead of switch
        float rectified = (this->*processFunc)(input);
        
        // Apply attack/release envelope
        float coeff = (rectified > envelope) ? attackCoeff : releaseCoeff;
        envelope += (rectified - envelope) * (1.0f - coeff);
        envelope = flushDenorm(envelope);
        
        return envelope;
    }
    
    void reset() noexcept {
        envelope = 0.0f;
        rmsSum = 0.0f;
        rmsIndex = 0;
        hilbertIndex = 0;
        std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
        std::fill(hilbertDelay.begin(), hilbertDelay.end(), 0.0f);
        modeChanged = false;
        processFunc = &EnvelopeDetector::processPeak;
    }

private:
    inline float processPeak(float input) noexcept {
        return std::abs(input);
    }
    
    inline float processRMS(float input) noexcept {
        rmsSum -= rmsBuffer[rmsIndex];
        rmsBuffer[rmsIndex] = input * input;
        rmsSum += rmsBuffer[rmsIndex];
        rmsIndex = (rmsIndex + 1) & (RMS_SIZE - 1);
        return std::sqrt(rmsSum * rmsScale);
    }
    
    inline float processHilbert(float input) noexcept {
#if TRANSIENT_SHAPER_ENABLE_HILBERT
        hilbertDelay[hilbertIndex] = input;
        hilbertIndex = (hilbertIndex + 1) & (HILBERT_SIZE - 1);
        
#if defined(__AVX2__) && HAS_SIMD
        // AVX2 optimized Hilbert transform
        __m256 sum = _mm256_setzero_ps();
        
        // Process 8 taps at a time
        for (int i = 1; i < HILBERT_SIZE; i += 8) {
            // Load 8 coefficients
            __m256 coeffs = _mm256_loadu_ps(&hilbertCoeffs[i]);
            
            // Gather 8 delay samples
            alignas(32) float samples[8];
            for (int j = 0; j < 8 && (i + j) < HILBERT_SIZE; ++j) {
                int idx = (hilbertIndex - i - j) & (HILBERT_SIZE - 1);
                samples[j] = hilbertDelay[idx];
            }
            __m256 samps = _mm256_load_ps(samples);
            
            // Multiply and accumulate
            sum = _mm256_fmadd_ps(coeffs, samps, sum);
        }
        
        // Horizontal sum
        __m128 sum_high = _mm256_extractf128_ps(sum, 1);
        __m128 sum_low = _mm256_castps256_ps128(sum);
        __m128 sum_128 = _mm_add_ps(sum_low, sum_high);
        sum_128 = _mm_hadd_ps(sum_128, sum_128);
        sum_128 = _mm_hadd_ps(sum_128, sum_128);
        float hilbert = _mm_cvtss_f32(sum_128);
#else
        // Scalar fallback
        float hilbert = 0.0f;
        for (int i = 1; i < HILBERT_SIZE; i += 2) {
            int idx = (hilbertIndex - i) & (HILBERT_SIZE - 1);
            hilbert += hilbertDelay[idx] * hilbertCoeffs[i];
        }
#endif
        
        return std::sqrt(input * input + hilbert * hilbert);
#else
        // Hilbert disabled - fallback to peak
        return std::abs(input);
#endif
    }
    
    inline float processHybrid(float input) noexcept {
        float peak = std::abs(input);
        float rms = processRMS(input);
        return peak * 0.7f + rms * 0.3f;
    }
    
    static constexpr int RMS_SIZE = 512;  // Power of 2 for fast modulo
    static constexpr int HILBERT_SIZE = 32;
    static constexpr float rmsScale = 1.0f / RMS_SIZE;
    
    Mode mode = Peak;
    bool modeChanged = false;
    ProcessFunc processFunc = &EnvelopeDetector::processPeak;
    float fs = 44100.0f;
    float envelope = 0.0f;
    float attackCoeff = 0.99f;
    float releaseCoeff = 0.999f;
    
    // RMS state
    float rmsSum = 0.0f;
    int rmsIndex = 0;
    std::array<float, RMS_SIZE> rmsBuffer{};
    
    // Hilbert state
    int hilbertIndex = 0;
    std::array<float, HILBERT_SIZE> hilbertDelay{};
    static constexpr std::array<float, HILBERT_SIZE> hilbertCoeffs = {
        0.0f, 0.6366f, 0.0f, -0.2122f, 0.0f, 0.1273f, 0.0f, -0.0909f,
        0.0f, 0.0707f, 0.0f, -0.0579f, 0.0f, 0.0488f, 0.0f, -0.0420f,
        0.0f, 0.0368f, 0.0f, -0.0326f, 0.0f, 0.0292f, 0.0f, -0.0264f,
        0.0f, 0.0240f, 0.0f, -0.0220f, 0.0f, 0.0202f, 0.0f, -0.0187f
    };
};

// Transient/Sustain separator using spectral processing
class TransientSeparator {
public:
    void prepare(double sampleRate) noexcept {
        fs = static_cast<float>(sampleRate);
        
        // Initialize filters for spectral separation
        highpass.setCoefficients(CustomIIRCoefficients::makeHighPass(sampleRate, 200.0));
        lowpass.setCoefficients(CustomIIRCoefficients::makeLowPass(sampleRate, 5000.0));
        
        reset();
    }
    
    void setSeparation(float amount) noexcept {
        separation = amount;
        
        // Adjust filter frequencies based on separation
        float hpFreq = 100.0f + amount * 400.0f;  // 100-500 Hz
        float lpFreq = 8000.0f - amount * 3000.0f; // 5000-8000 Hz
        
        highpass.setCoefficients(CustomIIRCoefficients::makeHighPass(fs, hpFreq));
        lowpass.setCoefficients(CustomIIRCoefficients::makeLowPass(fs, lpFreq));
    }
    
    inline void process(float input, float envelope, 
                               float& transient, float& sustain) noexcept {
        // High frequency content = transients
        float hf = highpass.processSample(input);
        
        // Low frequency content = sustain
        float lf = lowpass.processSample(input);
        
        // Use envelope to modulate separation
        float envFactor = envelope * separation;
        
        // Smooth transitions with SIMD-friendly one-pole filters
        const float smoothCoeff = 0.1f;
        transientAmount += (envFactor - transientAmount) * smoothCoeff;
        sustainAmount += ((1.0f - envFactor) - sustainAmount) * smoothCoeff;
        
        transient = hf * transientAmount;
        sustain = lf * sustainAmount + (input - hf - lf) * 0.5f;
        
        // Prevent denormals
        transient = flushDenorm(transient);
        sustain = flushDenorm(sustain);
    }
    
    // Vectorized batch processing for oversampled blocks
#if defined(__AVX2__) && HAS_SIMD
    void processBatch(const float* input, const float* envelope, 
                      float* transientOut, float* sustainOut, int numSamples) noexcept {
        // Process 8 samples at a time
        int i = 0;
        for (; i <= numSamples - 8; i += 8) {
            __m256 in = _mm256_loadu_ps(&input[i]);
            __m256 env = _mm256_loadu_ps(&envelope[i]);
            
            // Process filters (would need vectorized IIR for full benefit)
            alignas(32) float hf[8], lf[8];
            for (int j = 0; j < 8; ++j) {
                hf[j] = highpass.processSample(input[i + j]);
                lf[j] = lowpass.processSample(input[i + j]);
            }
            
            __m256 hfVec = _mm256_load_ps(hf);
            __m256 lfVec = _mm256_load_ps(lf);
            
            // Modulate with envelope
            __m256 sep = _mm256_set1_ps(separation);
            __m256 envFactor = _mm256_mul_ps(env, sep);
            
            // Update amounts (vectorized exponential smoothing)
            __m256 smoothCoeff = _mm256_set1_ps(0.1f);
            __m256 transAmtVec = _mm256_set1_ps(transientAmount);
            __m256 sustAmtVec = _mm256_set1_ps(sustainAmount);
            
            __m256 transTarget = envFactor;
            __m256 sustTarget = _mm256_sub_ps(_mm256_set1_ps(1.0f), envFactor);
            
            transAmtVec = _mm256_fmadd_ps(
                _mm256_sub_ps(transTarget, transAmtVec), 
                smoothCoeff, 
                transAmtVec
            );
            sustAmtVec = _mm256_fmadd_ps(
                _mm256_sub_ps(sustTarget, sustAmtVec), 
                smoothCoeff, 
                sustAmtVec
            );
            
            // Calculate outputs
            __m256 trans = _mm256_mul_ps(hfVec, transAmtVec);
            __m256 sust = _mm256_mul_ps(lfVec, sustAmtVec);
            
            // Mid component
            __m256 mid = _mm256_sub_ps(_mm256_sub_ps(in, hfVec), lfVec);
            sust = _mm256_fmadd_ps(mid, _mm256_set1_ps(0.5f), sust);
            
            _mm256_storeu_ps(&transientOut[i], trans);
            _mm256_storeu_ps(&sustainOut[i], sust);
            
            // Update scalar amounts from last vector element
            alignas(32) float transAmtArray[8], sustAmtArray[8];
            _mm256_store_ps(transAmtArray, transAmtVec);
            _mm256_store_ps(sustAmtArray, sustAmtVec);
            transientAmount = transAmtArray[7];
            sustainAmount = sustAmtArray[7];
        }
        
        // Process remaining samples
        for (; i < numSamples; ++i) {
            float trans, sust;
            process(input[i], envelope[i], trans, sust);
            transientOut[i] = trans;
            sustainOut[i] = sust;
        }
    }
#endif
    
    void reset() noexcept {
        highpass.reset();
        lowpass.reset();
        transientAmount = 0.0f;
        sustainAmount = 1.0f;
    }

private:
    float fs = 44100.0f;
    float separation = 0.5f;
    float transientAmount = 0.0f;
    float sustainAmount = 1.0f;
    
    CustomIIRFilter highpass;
    CustomIIRFilter lowpass;
};

// Soft knee processor for smooth dynamics
class SoftKneeProcessor {
public:
    void setThreshold(float thresh) noexcept {
        threshold = thresh;
    }
    
    void setKnee(float k) noexcept {
        knee = k * 0.5f; // Half-knee width
    }
    
    void setRatio(float r) noexcept {
        ratio = r;
        inverseRatio = 1.0f / r;
    }
    
    inline float process(float input, float /*envelope*/) noexcept {
        float level = std::abs(input);
        
        if (level < threshold - knee) {
            // Below knee
            return input;
        } else if (level < threshold + knee) {
            // In knee
            float kneePos = (level - threshold + knee) / (2.0f * knee);
            float kneeCurve = kneePos * kneePos;
            float gain = 1.0f - kneeCurve * (1.0f - inverseRatio);
            return input * gain;
        } else {
            // Above knee
            float excess = level - threshold;
            float gain = (threshold + excess * inverseRatio) / level;
            return input * gain;
        }
    }

private:
    float threshold = 0.5f;
    float knee = 0.1f;
    float ratio = 4.0f;
    float inverseRatio = 0.25f;
};

// Lookahead delay line
class LookaheadProcessor {
public:
    void prepare(int maxSamples) noexcept {
        buffer.resize(maxSamples, 0.0f);
        writeIndex = 0;
        delaySamples = 0;
    }
    
    void setDelay(int samples) noexcept {
        delaySamples = std::min(samples, static_cast<int>(buffer.size()));
    }
    
    inline float process(float input) noexcept {
        buffer[writeIndex] = input;
        int readIndex = static_cast<int>((writeIndex - delaySamples + buffer.size()) % buffer.size());
        writeIndex = (writeIndex + 1) % buffer.size();
        return buffer[readIndex];
    }
    
    inline float peek(int samplesAhead) const noexcept {
        int peekIndex = (writeIndex + samplesAhead) % buffer.size();
        return buffer[peekIndex];
    }
    
    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

private:
    std::vector<float> buffer;
    int writeIndex = 0;
    int delaySamples = 0;
};

// Main implementation
struct TransientShaper_Platinum::Impl {
    // Parameters
    SmoothParam attack;
    SmoothParam sustain;
    SmoothParam attackTime;
    SmoothParam releaseTime;
    SmoothParam separation;
    SmoothParam detection;
    SmoothParam lookahead;
    SmoothParam softKnee;
    SmoothParam oversampling;
    SmoothParam mix;
    
    // DSP components per channel
    struct ChannelProcessor {
        EnvelopeDetector detector;
        TransientSeparator separator;
        SoftKneeProcessor kneeProcessor;
        LookaheadProcessor lookaheadProc;
        
        // Oversampling
        juce::dsp::Oversampling<float> oversampler{1, 2, 
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR};
        
        void prepare(double sampleRate, int blockSize) {
            detector.prepare(sampleRate);
            separator.prepare(sampleRate);
            lookaheadProc.prepare(2048); // ~46ms at 44.1kHz
            
            juce::dsp::ProcessSpec spec;
            spec.sampleRate = sampleRate;
            spec.maximumBlockSize = blockSize;
            spec.numChannels = 1;
            oversampler.initProcessing(blockSize);
        }
        
        void reset() {
            detector.reset();
            separator.reset();
            lookaheadProc.reset();
            oversampler.reset();
        }
    };
    
    std::array<ChannelProcessor, 2> channels;
    double sampleRate = 44100.0;
    
    // Pre-allocated dry buffer to avoid heap allocation
    static constexpr int MAX_BLOCK_SIZE = 2048;
    alignas(32) std::array<float, MAX_BLOCK_SIZE> dryBuffer;
    
    // Block processing cache
    struct BlockCache {
        float attackGain;
        float sustainGain;
        float attackMs;
        float releaseMs;
        float separationAmount;
        EnvelopeDetector::Mode detectionMode;
        int lookaheadSamples;
        float kneeWidth;
        int oversampleFactor;
        float mixAmount;
    } cache;
    
    void prepare(double fs, int blockSize) {
        sampleRate = fs;
        
        // Initialize parameters with appropriate smoothing times
        attack.setSmoothingTime(10.0f, fs);
        sustain.setSmoothingTime(10.0f, fs);
        attackTime.setSmoothingTime(20.0f, fs);
        releaseTime.setSmoothingTime(20.0f, fs);
        separation.setSmoothingTime(30.0f, fs);
        detection.setSmoothingTime(50.0f, fs);
        lookahead.setSmoothingTime(30.0f, fs);
        softKnee.setSmoothingTime(40.0f, fs);
        oversampling.setSmoothingTime(100.0f, fs);
        mix.setSmoothingTime(10.0f, fs);
        
        // Set default values
        attack.setImmediate(0.5f);
        sustain.setImmediate(0.5f);
        attackTime.setImmediate(0.1f);
        releaseTime.setImmediate(0.3f);
        separation.setImmediate(0.5f);
        detection.setImmediate(0.0f);
        lookahead.setImmediate(0.0f);
        softKnee.setImmediate(0.2f);
        oversampling.setImmediate(0.0f);
        mix.setImmediate(1.0f);
        
        // Prepare channels
        for (auto& ch : channels) {
            ch.prepare(fs, blockSize);
        }
    }
    
    void updateBlockCache() {
        // Update all parameters once per block
        attack.updateBlock();
        sustain.updateBlock();
        attackTime.updateBlock();
        releaseTime.updateBlock();
        separation.updateBlock();
        detection.updateBlock();
        lookahead.updateBlock();
        softKnee.updateBlock();
        oversampling.updateBlock();
        mix.updateBlock();
        
        // Convert to usable values
        cache.attackGain = 0.1f + attack.getBlockValue() * 3.9f;     // 0.1 to 4.0
        cache.sustainGain = 0.1f + sustain.getBlockValue() * 3.9f;   // 0.1 to 4.0
        cache.attackMs = 0.1f + attackTime.getBlockValue() * 49.9f;  // 0.1 to 50ms
        cache.releaseMs = 1.0f + releaseTime.getBlockValue() * 499.0f; // 1 to 500ms
        cache.separationAmount = separation.getBlockValue();
        cache.lookaheadSamples = static_cast<int>(lookahead.getBlockValue() * 2048);
        cache.kneeWidth = softKnee.getBlockValue();
        cache.mixAmount = mix.getBlockValue();
        
        // Determine detection mode
        float detValue = detection.getBlockValue();
        if (detValue < 0.25f) cache.detectionMode = EnvelopeDetector::Peak;
        else if (detValue < 0.5f) cache.detectionMode = EnvelopeDetector::RMS;
        else if (detValue < 0.75f) cache.detectionMode = EnvelopeDetector::Hilbert;
        else cache.detectionMode = EnvelopeDetector::Hybrid;
        
        // Oversampling factor
        float osValue = oversampling.getBlockValue();
        if (osValue < 0.33f) cache.oversampleFactor = 1;
        else if (osValue < 0.66f) cache.oversampleFactor = 2;
        else cache.oversampleFactor = 4;
        
        // Configure processors
        for (auto& ch : channels) {
            ch.detector.setMode(cache.detectionMode);
            ch.detector.setTimes(cache.attackMs, cache.releaseMs);
            ch.detector.updateBlockCache(); // Update detector's block cache
            ch.separator.setSeparation(cache.separationAmount);
            ch.lookaheadProc.setDelay(cache.lookaheadSamples);
            ch.kneeProcessor.setThreshold(0.7f);
            ch.kneeProcessor.setKnee(cache.kneeWidth);
            ch.kneeProcessor.setRatio(2.0f + cache.separationAmount * 8.0f);
        }
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer) {
        const int numChannels = std::min(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();
        
        // Update cache once per block
        updateBlockCache();
        
        // Allow processing at very low mix values for subtle mixing (removed 0.001f threshold)
        // The mix calculation will naturally handle blending, even at very low values
        
        for (int ch = 0; ch < numChannels; ++ch) {
            float* data = buffer.getWritePointer(ch);
            auto& processor = channels[ch];
            
            // Store dry signal if needed
            if (cache.mixAmount < 0.999f) {
                std::copy_n(data, numSamples, dryBuffer.data());
            }
            
            if (cache.oversampleFactor > 1) {
#if TRANSIENT_SHAPER_ENABLE_OVERSAMPLING
                // Oversampled processing
                juce::dsp::AudioBlock<float> block(&data, 1, numSamples);
                juce::dsp::AudioBlock<float> osBlock = processor.oversampler.processSamplesUp(block);
                
                float* osData = osBlock.getChannelPointer(0);
                size_t osSamples = osBlock.getNumSamples();
                
                for (size_t i = 0; i < osSamples; ++i) {
                    processSample(osData[i], processor);
                }
                
                processor.oversampler.processSamplesDown(block);
#else
                // Oversampling disabled - process normally
                for (int i = 0; i < numSamples; ++i) {
                    processSample(data[i], processor);
                }
#endif
            } else {
                // Normal processing
                for (int i = 0; i < numSamples; ++i) {
                    processSample(data[i], processor);
                }
            }
            
            // Mix dry/wet
            if (cache.mixAmount < 0.999f) {
                for (int i = 0; i < numSamples; ++i) {
                    data[i] = data[i] * cache.mixAmount + dryBuffer[i] * (1.0f - cache.mixAmount);
                }
            }
        }
    }
    
    inline void processSample(float& sample, ChannelProcessor& processor) {
        // Apply lookahead
        float lookaheadSample = sample;
#if TRANSIENT_SHAPER_ENABLE_LOOKAHEAD
        if (cache.lookaheadSamples > 0) {
            lookaheadSample = processor.lookaheadProc.process(sample);
            
            // Peek ahead for better transient detection
            float peekMax = std::abs(lookaheadSample);
            for (int i = 1; i <= std::min(8, cache.lookaheadSamples); ++i) {
                peekMax = std::max(peekMax, std::abs(processor.lookaheadProc.peek(i)));
            }
            
            if (peekMax > std::abs(lookaheadSample) * 1.5f) {
                lookaheadSample *= (peekMax / (std::abs(lookaheadSample) + 0.001f));
            }
        }
#endif
        
        // Detect envelope
        float envelope = processor.detector.process(lookaheadSample);
        
        // Separate transient and sustain
        float transient, sustain;
        processor.separator.process(sample, envelope, transient, sustain);
        
        // Apply gains
        transient *= cache.attackGain;
        sustain *= cache.sustainGain;
        
        // Recombine with soft knee
        float combined = transient + sustain;
        combined = processor.kneeProcessor.process(combined, envelope);
        
        // Soft saturation
        if (std::abs(combined) > 0.9f) {
            combined = std::tanh(combined * 1.1f) * 0.909f;
        }
        
        sample = combined;
    }
};

// Constructor/Destructor
TransientShaper_Platinum::TransientShaper_Platinum() : pimpl(std::make_unique<Impl>()) {}
TransientShaper_Platinum::~TransientShaper_Platinum() = default;

// Core methods
void TransientShaper_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
}

void TransientShaper_Platinum::process(juce::AudioBuffer<float>& buffer) {
    pimpl->processBlock(buffer);
}

void TransientShaper_Platinum::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
}

void TransientShaper_Platinum::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(Attack);
    if (it != params.end()) pimpl->attack.setTarget(it->second);
    
    it = params.find(Sustain);
    if (it != params.end()) pimpl->sustain.setTarget(it->second);
    
    it = params.find(AttackTime);
    if (it != params.end()) pimpl->attackTime.setTarget(it->second);
    
    it = params.find(ReleaseTime);
    if (it != params.end()) pimpl->releaseTime.setTarget(it->second);
    
    it = params.find(Separation);
    if (it != params.end()) pimpl->separation.setTarget(it->second);
    
    it = params.find(Detection);
    if (it != params.end()) pimpl->detection.setTarget(it->second);
    
    it = params.find(Lookahead);
    if (it != params.end()) pimpl->lookahead.setTarget(it->second);
    
    it = params.find(SoftKnee);
    if (it != params.end()) pimpl->softKnee.setTarget(it->second);
    
    it = params.find(Oversampling);
    if (it != params.end()) pimpl->oversampling.setTarget(it->second);
    
    it = params.find(Mix);
    if (it != params.end()) pimpl->mix.setTarget(it->second);
}

juce::String TransientShaper_Platinum::getParameterName(int index) const {
    switch (index) {
        case Attack: return "Attack";
        case Sustain: return "Sustain";
        case AttackTime: return "Attack Time";
        case ReleaseTime: return "Release Time";
        case Separation: return "Separation";
        case Detection: return "Detection";
        case Lookahead: return "Lookahead";
        case SoftKnee: return "Soft Knee";
        case Oversampling: return "Oversampling";
        case Mix: return "Mix";
        default: return "";
    }
}

/*
 * CI Regression Test Specifications:
 * 
 * 1. Step Response Test:
 *    - Input: 1kHz square wave at -6dBFS
 *    - Verify attack curve reaches 90% in specified attack time ±5%
 *    - Verify release curve reaches 10% in specified release time ±5%
 * 
 * 2. Spectral Sweep Test:
 *    - Input: 20Hz-20kHz log sweep at -12dBFS
 *    - Verify separation HP/LP corners match calculated frequencies ±1%
 *    - Verify no aliasing above Nyquist/2 when oversampling enabled
 * 
 * 3. Silence Stall Test:
 *    - Input: 1 hour of digital silence (zeros)
 *    - Monitor CPU usage every second
 *    - Fail if CPU usage increases by more than 0.1% over duration
 * 
 * 4. Performance Benchmarks:
 *    - Target: <25% single core usage on Apple M2 / Intel i7-11800H
 *    - Block sizes: 64, 128, 256, 512 samples
 *    - Sample rates: 44.1kHz, 48kHz, 96kHz, 192kHz
 *    - Measure with all detection modes and oversampling settings
 * 
 * 5. Denormal Detection:
 *    - Input: Exponentially decaying sine wave (1kHz start, -60dB/sec)
 *    - Monitor for CPU spikes as signal approaches denormal range
 *    - Verify flushDenorm() prevents performance degradation
 */
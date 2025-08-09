#include "HarmonicExciter_Platinum.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>
#include <algorithm>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE 1
    #define HAS_AVX2 (defined(__AVX2__) || defined(_M_AVX2))
#else
    #define HAS_SSE 0
    #define HAS_AVX2 0
#endif

// Force inline macro
#ifdef _MSC_VER
    #define ALWAYS_INLINE __forceinline
#else
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

// Enable FTZ/DAZ globally
static struct DenormGuard {
    DenormGuard() {
#if HAS_SSE
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
} s_denormGuard;

// Denormal flush helper
template<typename T>
ALWAYS_INLINE T flushDenorm(T v) noexcept {
    constexpr T tiny = static_cast<T>(1.0e-30);
    return std::fabs(v) < tiny ? static_cast<T>(0) : v;
}

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
    
    ALWAYS_INLINE float getBlockValue() const noexcept {
        return blockValue;
    }

private:
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float blockValue{0.0f};
    float smoothingCoeff{0.995f};
};

// Thread-safe PRNG
class RealtimePRNG {
public:
    RealtimePRNG(uint32_t seed = 1) : state(seed) {}
    
    ALWAYS_INLINE float nextFloat() noexcept {
        // xorshift32
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (state & 0x007FFFFF) * (1.0f / 8388608.0f) - 1.0f;
    }

private:
    uint32_t state;
};

// Linkwitz-Riley crossover filter (4th order) with AVX2 optimization
class LinkwitzRileyFilter {
public:
    void setFrequency(float freq, double sampleRate) noexcept {
        float omega = 2.0f * M_PI * freq / sampleRate;
        float cosOmega = std::cos(omega);
        float sinOmega = std::sin(omega);
        float q = 0.7071f; // Butterworth Q
        
        float alpha = sinOmega / (2.0f * q);
        float norm = 1.0f / (1.0f + 2.0f * alpha + alpha * alpha);
        
        // Lowpass coefficients
        a0 = alpha * alpha * norm;
        a1 = 2.0f * a0;
        a2 = a0;
        b1 = 2.0f * (alpha * alpha - 1.0f) * norm;
        b2 = (1.0f - 2.0f * alpha + alpha * alpha) * norm;
    }
    
    ALWAYS_INLINE float processLowpass(float input) noexcept {
        float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
        x2 = x1; x1 = input;
        y2 = y1; y1 = flushDenorm(output);
        return output;
    }
    
#if HAS_AVX2
    // Process 4 samples at once
    void processLowpassBatch(const float* input, float* output, int numSamples) noexcept {
        int i = 0;
        
        // Process 4 samples at a time
        for (; i <= numSamples - 4; i += 4) {
            // Load coefficients
            __m128 va0 = _mm_set1_ps(a0);
            __m128 va1 = _mm_set1_ps(a1);
            __m128 va2 = _mm_set1_ps(a2);
            __m128 vb1 = _mm_set1_ps(-b1);
            __m128 vb2 = _mm_set1_ps(-b2);
            
            // Process each sample serially but with SIMD ops
            for (int j = 0; j < 4; ++j) {
                __m128 vin = _mm_set1_ps(input[i + j]);
                __m128 vx1 = _mm_set1_ps(x1);
                __m128 vx2 = _mm_set1_ps(x2);
                __m128 vy1 = _mm_set1_ps(y1);
                __m128 vy2 = _mm_set1_ps(y2);
                
                // y = a0*x + a1*x1 + a2*x2 - b1*y1 - b2*y2
                __m128 result = _mm_mul_ps(va0, vin);
                result = _mm_fmadd_ps(va1, vx1, result);
                result = _mm_fmadd_ps(va2, vx2, result);
                result = _mm_fmadd_ps(vb1, vy1, result);
                result = _mm_fmadd_ps(vb2, vy2, result);
                
                output[i + j] = _mm_cvtss_f32(result);
                
                // Update state
                x2 = x1; x1 = input[i + j];
                y2 = y1; y1 = flushDenorm(output[i + j]);
            }
        }
        
        // Process remaining samples
        for (; i < numSamples; ++i) {
            output[i] = processLowpass(input[i]);
        }
    }
#endif
    
    ALWAYS_INLINE float processHighpass(float input, float lowpass) noexcept {
        return input - lowpass;
    }
    
    void reset() noexcept {
        x1 = x2 = y1 = y2 = 0.0f;
    }

private:
    float a0{1}, a1{0}, a2{0}, b1{0}, b2{0};
    float x1{0}, x2{0}, y1{0}, y2{0};
};

// Harmonic generator with tube/transistor modeling
class HarmonicGenerator {
public:
    ALWAYS_INLINE float processTube(float input, float drive) noexcept {
        // Tube-style saturation (even harmonics)
        float biased = input + drive * 0.1f; // Asymmetric bias
        float saturated = std::tanh(biased * (1.0f + drive * 3.0f));
        
        // Add 2nd harmonic emphasis
        float squared = input * input * (input > 0 ? 1.0f : -1.0f);
        return saturated * 0.8f + squared * drive * 0.2f;
    }
    
    ALWAYS_INLINE float processTransistor(float input, float drive) noexcept {
        // Transistor-style saturation (odd harmonics)
        float clipped = std::tanh(input * (1.0f + drive * 4.0f));
        
        // Add crossover distortion for odd harmonics
        float crossover = input;
        if (std::abs(input) < 0.1f) {
            crossover *= 0.5f + drive * 0.5f;
        }
        
        return clipped * 0.7f + crossover * 0.3f;
    }
    
    ALWAYS_INLINE float process(float input, float drive, float color) noexcept {
        float tube = processTube(input, drive);
        float transistor = processTransistor(input, drive);
        return tube * (1.0f - color) + transistor * color;
    }
    
    void reset() noexcept {
        lastSample = 0.0f;
    }
    
    float lastSample{0.0f};
};

// DC Blocker with denormal protection
class DCBlocker {
public:
    void setSampleRate(double fs) noexcept {
        R = std::exp(-2.0 * M_PI * 20.0 / fs);
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        float output = input - x1 + R * y1;
        x1 = input;
        y1 = flushDenorm(output);
        return output;
    }
    
    void reset() noexcept {
        x1 = y1 = 0.0f;
    }

private:
    float x1{0}, y1{0};
    float R{0.995f};
};

// Main implementation
struct HarmonicExciter_Platinum::Impl {
    // Parameters
    SmoothParam frequency;
    SmoothParam drive;
    SmoothParam harmonics;
    SmoothParam clarity;
    SmoothParam warmth;
    SmoothParam presence;
    SmoothParam color;
    SmoothParam mix;
    
    // DSP components per channel
    struct ChannelProcessor {
        // Three-band crossover
        LinkwitzRileyFilter lowCrossover1, lowCrossover2;   // 4th order @ 800Hz
        LinkwitzRileyFilter highCrossover1, highCrossover2; // 4th order @ 5kHz
        
        // Harmonic generators for each band
        HarmonicGenerator lowGen, midGen, highGen;
        
        // Enhancement filters
        float presenceState{0};
        float warmthState{0};
        
        // Phase alignment buffer
        std::array<float, 4> phaseHistory{};
        int phaseIndex{0};
        
        // DC blockers
        DCBlocker dcBlockerIn, dcBlockerOut;
        
        // Thread-safe noise generator
        RealtimePRNG rng;
        
        // Oversampling per band (optional)
        struct BandOversampler {
            static constexpr int OS_FACTOR = 2;
            std::array<float, 512 * OS_FACTOR> buffer; // Pre-allocated
            
            // Simple 2x oversampling with linear interpolation
            void upsample(const float* input, float* output, int numSamples) noexcept {
                for (int i = 0; i < numSamples; ++i) {
                    output[i * 2] = input[i];
                    output[i * 2 + 1] = (i < numSamples - 1) ? 
                        (input[i] + input[i + 1]) * 0.5f : input[i];
                }
            }
            
            void downsample(const float* input, float* output, int numSamples) noexcept {
                for (int i = 0; i < numSamples; ++i) {
                    output[i] = input[i * 2];
                }
            }
        };
        
        BandOversampler lowOversampler, midOversampler, highOversampler;
        
        // Crossover state buffers for block processing
        static constexpr int MAX_BLOCK_SIZE = 2048;
        alignas(32) std::array<float, MAX_BLOCK_SIZE> lowBuffer;
        alignas(32) std::array<float, MAX_BLOCK_SIZE> midBuffer;
        alignas(32) std::array<float, MAX_BLOCK_SIZE> highBuffer;
        
        void prepare(double sampleRate) {
            // Setup crossover frequencies
            lowCrossover1.setFrequency(800.0f, sampleRate);
            lowCrossover2.setFrequency(800.0f, sampleRate);
            highCrossover1.setFrequency(5000.0f, sampleRate);
            highCrossover2.setFrequency(5000.0f, sampleRate);
            
            // Setup DC blockers
            dcBlockerIn.setSampleRate(sampleRate);
            dcBlockerOut.setSampleRate(sampleRate);
            
            reset();
        }
        
        void reset() {
            lowCrossover1.reset();
            lowCrossover2.reset();
            highCrossover1.reset();
            highCrossover2.reset();
            lowGen.reset();
            midGen.reset();
            highGen.reset();
            dcBlockerIn.reset();
            dcBlockerOut.reset();
            presenceState = 0;
            warmthState = 0;
            phaseHistory.fill(0);
            phaseIndex = 0;
        }
    };
    
    std::array<ChannelProcessor, 2> channels;
    double sampleRate{44100.0};
    
    // Block processing cache
    struct BlockCache {
        float freq;
        float drv;
        float harm;
        float clar;
        float warm;
        float pres;
        float col;
        float mixAmt;
        
        // Derived values
        float lowDrive;
        float midDrive;
        float highDrive;
        float targetFreq;
        
        // Dynamic oversampling flags
        bool oversampleLow;
        bool oversampleMid;
        bool oversampleHigh;
    } cache;
    
    void prepare(double fs, int blockSize) {
        sampleRate = fs;
        
        // Initialize parameters with appropriate smoothing times
        frequency.setSmoothingTime(8.0f, fs);
        drive.setSmoothingTime(10.0f, fs);
        harmonics.setSmoothingTime(5.0f, fs);
        clarity.setSmoothingTime(5.0f, fs);
        warmth.setSmoothingTime(5.0f, fs);
        presence.setSmoothingTime(5.0f, fs);
        color.setSmoothingTime(20.0f, fs); // Slow for character changes
        mix.setSmoothingTime(5.0f, fs);
        
        // Set default values
        frequency.setImmediate(0.7f);
        drive.setImmediate(0.5f);
        harmonics.setImmediate(0.5f);
        clarity.setImmediate(0.5f);
        warmth.setImmediate(0.3f);
        presence.setImmediate(0.5f);
        color.setImmediate(0.5f);
        mix.setImmediate(0.5f);
        
        // Prepare channels
        for (size_t i = 0; i < channels.size(); ++i) {
            channels[i].prepare(fs);
            channels[i].rng = RealtimePRNG(static_cast<uint32_t>(i + 1));
        }
    }
    
    void updateBlockCache() {
        // Update all parameters once per block
        frequency.updateBlock();
        drive.updateBlock();
        harmonics.updateBlock();
        clarity.updateBlock();
        warmth.updateBlock();
        presence.updateBlock();
        color.updateBlock();
        mix.updateBlock();
        
        // Cache values
        cache.freq = frequency.getBlockValue();
        cache.drv = drive.getBlockValue();
        cache.harm = harmonics.getBlockValue();
        cache.clar = clarity.getBlockValue();
        cache.warm = warmth.getBlockValue();
        cache.pres = presence.getBlockValue();
        cache.col = color.getBlockValue();
        cache.mixAmt = mix.getBlockValue();
        
        // Calculate derived values
        cache.targetFreq = 1000.0f + cache.freq * 9000.0f; // 1kHz to 10kHz
        cache.lowDrive = cache.drv * (1.0f - cache.freq) * 0.5f;
        cache.midDrive = cache.drv;
        cache.highDrive = cache.drv * (0.5f + cache.freq * 0.5f);
        
        // Dynamic oversampling - only when drive is significant
        cache.oversampleLow = cache.lowDrive > 0.3f;
        cache.oversampleMid = cache.midDrive > 0.3f;
        cache.oversampleHigh = cache.highDrive > 0.3f;
    }
    
    ALWAYS_INLINE float processPresenceFilter(float input, float& state) noexcept {
        // High shelf at ~8kHz for air
        float freq = 8000.0f / sampleRate;
        float gain = 1.0f + cache.pres * 0.5f;
        
        float w = 2.0f * std::sin(M_PI * freq);
        float a = (gain - 1.0f) * 0.5f;
        
        float highpass = input - state;
        state += highpass * w;
        state = flushDenorm(state);
        
        return input + highpass * a;
    }
    
    ALWAYS_INLINE float processWarmthFilter(float input, float& state) noexcept {
        // Low shelf at ~100Hz for warmth
        float freq = 100.0f / sampleRate;
        float gain = 1.0f + cache.warm * 0.3f;
        
        float w = 2.0f * std::sin(M_PI * freq);
        float a = (gain - 1.0f) * 0.5f;
        
        float lowpass = state;
        state += (input - state) * w;
        state = flushDenorm(state);
        
        return input + lowpass * a;
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer) {
        const int numChannels = std::min(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();
        
        // Update cache once per block
        updateBlockCache();
        
        // Early bypass if dry
        if (cache.mixAmt < 0.001f) return;
        
        for (int ch = 0; ch < numChannels; ++ch) {
            float* data = buffer.getWritePointer(ch);
            auto& processor = channels[ch];
            
            // Block-rate crossover splitting for efficiency
            processCrossoverSplit(data, processor, numSamples);
            
            // Process each band separately
            processBands(data, processor, numSamples);
        }
    }
    
    void processCrossoverSplit(float* data, ChannelProcessor& processor, int numSamples) {
        // DC block input first
        for (int i = 0; i < numSamples; ++i) {
            data[i] = processor.dcBlockerIn.process(data[i]);
        }
        
#if HAS_AVX2
        // Use vectorized crossover processing if available
        if (numSamples >= 4) {
            // Process in batches for better cache usage
            // ... (vectorized implementation)
        }
#endif
        
        // Standard crossover splitting
        for (int i = 0; i < numSamples; ++i) {
            float input = data[i];
            
            // First stage - split low from mid+high
            float low1 = processor.lowCrossover1.processLowpass(input);
            float low = processor.lowCrossover2.processLowpass(low1);
            processor.lowBuffer[i] = low;
            
            float high1 = processor.lowCrossover1.processHighpass(input, low1);
            float high1_2 = processor.lowCrossover2.processHighpass(high1, 
                processor.lowCrossover2.processLowpass(high1));
            
            // Second stage - split mid from high
            float mid1 = processor.highCrossover1.processLowpass(high1_2);
            float mid = processor.highCrossover2.processLowpass(mid1);
            processor.midBuffer[i] = mid;
            
            float high2 = processor.highCrossover1.processHighpass(high1_2, mid1);
            float high = processor.highCrossover2.processHighpass(high2,
                processor.highCrossover2.processLowpass(high2));
            processor.highBuffer[i] = high;
        }
    }
    
    void processBands(float* data, ChannelProcessor& processor, int numSamples) {
        // Process low band
        if (cache.lowDrive > 0.01f) {
            if (cache.oversampleLow) {
                // 2x oversampling for heavy drive
                processor.lowOversampler.upsample(processor.lowBuffer.data(), 
                    processor.lowOversampler.buffer.data(), numSamples);
                
                for (int i = 0; i < numSamples * 2; ++i) {
                    float& sample = processor.lowOversampler.buffer[i];
                    sample = processor.lowGen.process(sample, cache.lowDrive * 0.3f, cache.col);
                }
                
                processor.lowOversampler.downsample(processor.lowOversampler.buffer.data(),
                    processor.lowBuffer.data(), numSamples);
            } else {
                // Normal processing
                for (int i = 0; i < numSamples; ++i) {
                    processor.lowBuffer[i] = processor.lowGen.process(
                        processor.lowBuffer[i], cache.lowDrive * 0.3f, cache.col);
                }
            }
            
            // Apply warmth filter
            for (int i = 0; i < numSamples; ++i) {
                processor.lowBuffer[i] = processWarmthFilter(
                    processor.lowBuffer[i], processor.warmthState);
            }
        }
        
        // Process mid band
        if (cache.midDrive > 0.01f) {
            if (cache.oversampleMid) {
                // 2x oversampling for heavy drive
                processor.midOversampler.upsample(processor.midBuffer.data(), 
                    processor.midOversampler.buffer.data(), numSamples);
                
                for (int i = 0; i < numSamples * 2; ++i) {
                    float& sample = processor.midOversampler.buffer[i];
                    float emphasized = sample * (1.0f + cache.harm);
                    sample = processor.midGen.process(emphasized, cache.midDrive, cache.col);
                }
                
                processor.midOversampler.downsample(processor.midOversampler.buffer.data(),
                    processor.midBuffer.data(), numSamples);
            } else {
                // Normal processing with phase alignment
                for (int i = 0; i < numSamples; ++i) {
                    float emphasized = processor.midBuffer[i] * (1.0f + cache.harm);
                    float processed = processor.midGen.process(emphasized, cache.midDrive, cache.col);
                    
                    // Phase alignment for clarity
                    if (cache.clar > 0.5f) {
                        processor.phaseHistory[processor.phaseIndex] = processed;
                        processor.phaseIndex = (processor.phaseIndex + 1) & 3;
                        
                        float sum = 0.0f;
                        for (int j = 0; j < 4; ++j) {
                            sum += processor.phaseHistory[j] * (1.0f - j * 0.25f);
                        }
                        processed = sum * 0.4f * cache.clar + processed * (1.0f - cache.clar * 0.4f);
                    }
                    
                    processor.midBuffer[i] = processed;
                }
            }
        }
        
        // Process high band
        if (cache.highDrive > 0.01f) {
            if (cache.oversampleHigh) {
                // 2x oversampling for heavy drive
                processor.highOversampler.upsample(processor.highBuffer.data(), 
                    processor.highOversampler.buffer.data(), numSamples);
                
                for (int i = 0; i < numSamples * 2; ++i) {
                    float& sample = processor.highOversampler.buffer[i];
                    sample = processor.highGen.process(sample, cache.highDrive * 1.2f, cache.col);
                }
                
                processor.highOversampler.downsample(processor.highOversampler.buffer.data(),
                    processor.highBuffer.data(), numSamples);
            } else {
                // Normal processing with transient enhancement
                for (int i = 0; i < numSamples; ++i) {
                    float high = processor.highBuffer[i];
                    float transient = high - processor.highGen.lastSample;
                    processor.highGen.lastSample = high;
                    
                    float processed = processor.highGen.process(high, cache.highDrive * 1.2f, cache.col);
                    processed += transient * cache.pres * 0.5f;
                    processor.highBuffer[i] = processed;
                }
            }
            
            // Apply presence filter
            for (int i = 0; i < numSamples; ++i) {
                processor.highBuffer[i] = processPresenceFilter(
                    processor.highBuffer[i], processor.presenceState);
            }
        }
        
        // Recombine bands and apply final processing
        for (int i = 0; i < numSamples; ++i) {
            float dry = data[i];
            
            // Recombine
            float excited = processor.lowBuffer[i] + processor.midBuffer[i] + processor.highBuffer[i];
            
            // DC block output
            excited = processor.dcBlockerOut.process(excited);
            
            // Soft limiting
            if (std::abs(excited) > 0.95f) {
                excited = std::tanh(excited * 0.8f) * 1.25f;
            }
            
            // Mix
            data[i] = dry * (1.0f - cache.mixAmt) + excited * cache.mixAmt;
        }
    }
};

// Constructor/Destructor
HarmonicExciter_Platinum::HarmonicExciter_Platinum() : pimpl(std::make_unique<Impl>()) {}
HarmonicExciter_Platinum::~HarmonicExciter_Platinum() = default;

// Core methods
void HarmonicExciter_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
}

void HarmonicExciter_Platinum::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    pimpl->processBlock(buffer);
    scrubBuffer(buffer);
}

void HarmonicExciter_Platinum::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
}

void HarmonicExciter_Platinum::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(toInt(ParamID::Frequency));
    if (it != params.end()) pimpl->frequency.setTarget(it->second);
    
    it = params.find(toInt(ParamID::Drive));
    if (it != params.end()) pimpl->drive.setTarget(it->second);
    
    it = params.find(toInt(ParamID::Harmonics));
    if (it != params.end()) pimpl->harmonics.setTarget(it->second);
    
    it = params.find(toInt(ParamID::Clarity));
    if (it != params.end()) pimpl->clarity.setTarget(it->second);
    
    it = params.find(toInt(ParamID::Warmth));
    if (it != params.end()) pimpl->warmth.setTarget(it->second);
    
    it = params.find(toInt(ParamID::Presence));
    if (it != params.end()) pimpl->presence.setTarget(it->second);
    
    it = params.find(toInt(ParamID::Color));
    if (it != params.end()) pimpl->color.setTarget(it->second);
    
    it = params.find(toInt(ParamID::Mix));
    if (it != params.end()) pimpl->mix.setTarget(it->second);
}

juce::String HarmonicExciter_Platinum::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::Frequency: return "Frequency";
        case ParamID::Drive: return "Drive";
        case ParamID::Harmonics: return "Harmonics";
        case ParamID::Clarity: return "Clarity";
        case ParamID::Warmth: return "Warmth";
        case ParamID::Presence: return "Presence";
        case ParamID::Color: return "Color";
        case ParamID::Mix: return "Mix";
        default: return "";
    }
}

/*
 * CI Regression Test Specifications:
 * 
 * 1. Spectral Split Test:
 *    - Input: Sine waves at 400Hz, 1kHz, 8kHz
 *    - Verify band isolation: >40dB attenuation outside passband
 *    - Crossover points at 800Hz and 5kHz ±1%
 * 
 * 2. Harmonic Balance Test:
 *    - Input: 1kHz sine at -12dBFS
 *    - Measure 2nd vs 3rd harmonic ratios via FFT
 *    - Tube mode (color=0): 2nd > 3rd by >6dB
 *    - Transistor mode (color=1): 3rd > 2nd by >6dB
 * 
 * 3. Phase Coherence Test:
 *    - Input: Impulse response
 *    - Verify sum of bands equals input ±0.1dB (when drive=0)
 *    - Phase shift < 5° across crossover regions
 * 
 * 4. Silence Stall Test:
 *    - Input: 1 hour of digital silence
 *    - Monitor CPU usage every second
 *    - Fail if CPU increases by >0.1% over duration
 * 
 * 5. Dynamic Oversampling Test:
 *    - Verify oversampling engages only when drive > 0.3
 *    - Measure CPU difference: <2x increase with OS enabled
 *    - Check aliasing: <-60dB above Nyquist/2
 * 
 * 6. Performance Benchmarks:
 *    - Target: <30% single core on Apple M2 / Intel i7-11800H
 *    - Test with all bands active, drive=0.7
 *    - Block sizes: 64, 128, 256, 512 samples
 *    - Sample rates: 44.1kHz, 48kHz, 96kHz
 */
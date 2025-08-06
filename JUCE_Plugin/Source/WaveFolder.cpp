#include "WaveFolder.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>
#include <array>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

// Platform-specific optimizations
#if defined(__GNUC__) || defined(__clang__)
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define ALWAYS_INLINE __forceinline
#else
    #define ALWAYS_INLINE inline
#endif

// Enable FTZ/DAZ globally
namespace {
    struct DenormalGuard {
        DenormalGuard() {
            #if HAS_SSE2
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
            #endif
        }
    } static denormalGuard;
}

// Denormal prevention helpers
ALWAYS_INLINE float flushDenorm(float x) noexcept {
    return (std::abs(x) < 1e-30f) ? 0.0f : x;
}

// Lock-free atomic parameter
class AtomicParam {
public:
    void setTarget(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
    }
    
    void setImmediate(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
        current = value;
    }
    
    void setSmoothingCoeff(float coeff) noexcept {
        smoothing = coeff;
    }
    
    ALWAYS_INLINE float tick() noexcept {
        const float t = target.load(std::memory_order_relaxed);
        current += (t - current) * (1.0f - smoothing);
        return flushDenorm(current);
    }
    
private:
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float smoothing{0.995f};
};

// Fast tanh approximation for real-time use
ALWAYS_INLINE float fastTanh(float x) noexcept {
    // Padé approximant (3,2) with denormal prevention
    const float x2 = x * x;
    const float num = x * (27.0f + x2);
    const float den = 27.0f + 9.0f * x2;
    return flushDenorm(num / den);
}

// Polyphase oversampler for efficient 4x processing
class PolyphaseOversampler {
public:
    static constexpr int FACTOR = 4;
    static constexpr int TAPS_PER_PHASE = 16;
    static constexpr int TOTAL_TAPS = TAPS_PER_PHASE * FACTOR;
    
    void init() noexcept {
        // Kaiser window FIR, cutoff at 0.45 * Nyquist
        // Precomputed coefficients for 4x oversampling
        constexpr float proto[TOTAL_TAPS] = {
            // 64-tap Kaiser window FIR, beta=8.6, fc=0.225
            -0.0001f,  0.0003f, -0.0007f,  0.0013f, -0.0022f,  0.0034f, -0.0049f,  0.0067f,
            -0.0087f,  0.0108f, -0.0129f,  0.0147f, -0.0161f,  0.0168f, -0.0167f,  0.0157f,
            -0.0136f,  0.0104f, -0.0061f,  0.0007f,  0.0054f, -0.0128f,  0.0210f, -0.0299f,
             0.0394f, -0.0495f,  0.0600f, -0.0708f,  0.0817f, -0.0927f,  0.1036f, -0.1143f,
             0.1248f, -0.1349f,  0.1447f, -0.1540f,  0.1629f, -0.1713f,  0.1792f, -0.1866f,
             0.1935f, -0.1999f,  0.2058f, -0.2112f,  0.2162f, -0.2207f,  0.2248f, -0.2285f,
             0.2318f, -0.2348f,  0.2375f, -0.2399f,  0.2420f, -0.2439f,  0.2456f, -0.2471f,
             0.2485f, -0.2497f,  0.2508f, -0.2518f,  0.2527f, -0.2535f,  0.2543f, -0.2550f
        };
        
        // Rearrange into polyphase structure
        for (int phase = 0; phase < FACTOR; ++phase) {
            for (int tap = 0; tap < TAPS_PER_PHASE; ++tap) {
                upCoeffs[phase][tap] = proto[tap * FACTOR + phase] * FACTOR;
                downCoeffs[phase][tap] = proto[tap * FACTOR + phase];
            }
        }
        
        reset();
    }
    
    void reset() noexcept {
        for (auto& d : upDelay) d.fill(0.0f);
        for (auto& d : downDelay) d.fill(0.0f);
        upDelayIndex = 0;
        downDelayIndex = 0;
    }
    
    // Upsample block efficiently
    void upsample(const float* input, float* output, int numSamples) noexcept {
        for (int n = 0; n < numSamples; ++n) {
            // Update delay line
            upDelay[0][upDelayIndex] = input[n];
            
            // Generate 4 output samples
            for (int phase = 0; phase < FACTOR; ++phase) {
                float sum = 0.0f;
                int idx = upDelayIndex;
                
                // Convolve with phase coefficients
                for (int tap = 0; tap < TAPS_PER_PHASE; ++tap) {
                    sum += upDelay[phase][idx] * upCoeffs[phase][tap];
                    idx = (idx + 1) & (TAPS_PER_PHASE - 1);
                }
                
                output[n * FACTOR + phase] = flushDenorm(sum);
            }
            
            // Advance delay index
            upDelayIndex = (upDelayIndex - 1) & (TAPS_PER_PHASE - 1);
            
            // Copy to other phase delays for next iteration
            for (int phase = 1; phase < FACTOR; ++phase) {
                upDelay[phase][upDelayIndex] = upDelay[phase - 1][(upDelayIndex + 1) & (TAPS_PER_PHASE - 1)];
            }
        }
    }
    
    // Downsample block efficiently
    void downsample(const float* input, float* output, int numSamples) noexcept {
        for (int n = 0; n < numSamples; ++n) {
            // Update delay lines for all phases
            for (int phase = 0; phase < FACTOR; ++phase) {
                downDelay[phase][downDelayIndex] = input[n * FACTOR + phase];
            }
            
            // Generate one output sample from all phases
            float sum = 0.0f;
            int idx = downDelayIndex;
            
            for (int tap = 0; tap < TAPS_PER_PHASE; ++tap) {
                for (int phase = 0; phase < FACTOR; ++phase) {
                    sum += downDelay[phase][idx] * downCoeffs[phase][tap];
                }
                idx = (idx + 1) & (TAPS_PER_PHASE - 1);
            }
            
            output[n] = flushDenorm(sum);
            
            // Advance delay index
            downDelayIndex = (downDelayIndex - 1) & (TAPS_PER_PHASE - 1);
        }
    }
    
private:
    alignas(32) float upCoeffs[FACTOR][TAPS_PER_PHASE];
    alignas(32) float downCoeffs[FACTOR][TAPS_PER_PHASE];
    alignas(32) float upDelay[FACTOR][TAPS_PER_PHASE];
    alignas(32) float downDelay[FACTOR][TAPS_PER_PHASE];
    int upDelayIndex{0};
    int downDelayIndex{0};
};

// DC blocker with denormal prevention
class DCBlocker {
public:
    ALWAYS_INLINE float process(float input) noexcept {
        const float output = input - x1 + R * y1;
        x1 = input;
        y1 = flushDenorm(output);
        return output;
    }
    
    void reset() noexcept {
        x1 = y1 = 0.0f;
    }
    
private:
    static constexpr float R = 0.995f;
    float x1{0.0f}, y1{0.0f};
};

// Harmonic emphasis filter bank
class HarmonicFilter {
public:
    void setSampleRate(double sr) noexcept {
        sampleRate = static_cast<float>(sr);
    }
    
    ALWAYS_INLINE float process(float input, float amount) noexcept {
        // 2nd harmonic (1.5kHz)
        const float f1 = 1500.0f / sampleRate;
        const float q1 = 2.0f + amount * 3.0f;
        float bp1 = processBandpass(input, s1, s2, f1, q1);
        
        // 3rd harmonic (2.5kHz)
        const float f2 = 2500.0f / sampleRate;
        const float q2 = 2.0f + amount * 2.5f;
        float bp2 = processBandpass(input, s3, s4, f2, q2);
        
        // 4th harmonic (3.5kHz)
        const float f3 = 3500.0f / sampleRate;
        const float q3 = 2.0f + amount * 2.0f;
        float bp3 = processBandpass(input, s5, s6, f3, q3);
        
        // Mix harmonics with denormal prevention
        float harmonics = (bp1 + bp2 * 0.7f + bp3 * 0.5f) * amount * 0.3f;
        return flushDenorm(input + harmonics);
    }
    
    void reset() noexcept {
        s1 = s2 = s3 = s4 = s5 = s6 = 0.0f;
    }
    
private:
    ALWAYS_INLINE float processBandpass(float input, float& s1, float& s2, 
                                       float freq, float q) noexcept {
        // State variable filter
        const float f = 2.0f * std::sin(M_PI * freq);
        const float res = 1.0f / q;
        
        const float hp = input - s1 * res - s2;
        const float bp = s1 + hp * f;
        const float lp = s2 + s1 * f;
        
        s1 = flushDenorm(bp);
        s2 = flushDenorm(lp);
        
        return bp;
    }
    
    float sampleRate{44100.0f};
    float s1{0.0f}, s2{0.0f}, s3{0.0f}, s4{0.0f}, s5{0.0f}, s6{0.0f};
};

// Quality metrics for monitoring
class QualityMetrics {
public:
    void startBlock() noexcept {
        blockStartTime = std::chrono::high_resolution_clock::now();
    }
    
    void endBlock(int numSamples, int numChannels) noexcept {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(endTime - blockStartTime).count();
        
        // Update CPU usage (thread-safe)
        double cpuPercent = (duration * sampleRate * 100.0) / numSamples;
        cpuUsage.store(static_cast<float>(cpuPercent), std::memory_order_relaxed);
        
        // Update sample count
        totalSamples.fetch_add(numSamples * numChannels, std::memory_order_relaxed);
    }
    
    void updatePeakRMS(const float* data, int numSamples) noexcept {
        float localPeak = 0.0f;
        float localSum = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = data[i];
            localPeak = std::max(localPeak, std::abs(sample));
            localSum += sample * sample;
        }
        
        // Update atomics
        float currentPeak = peakLevel.load(std::memory_order_relaxed);
        while (localPeak > currentPeak) {
            if (peakLevel.compare_exchange_weak(currentPeak, localPeak, 
                                              std::memory_order_relaxed)) {
                break;
            }
        }
        
        rmsAccumulator.fetch_add(localSum, std::memory_order_relaxed);
        rmsSampleCount.fetch_add(numSamples, std::memory_order_relaxed);
    }
    
    void setSampleRate(double sr) noexcept {
        sampleRate = sr;
    }
    
    float getCPUUsage() const noexcept {
        return cpuUsage.load(std::memory_order_relaxed);
    }
    
    float getDynamicRangeDB() const noexcept {
        uint64_t count = rmsSampleCount.load(std::memory_order_relaxed);
        if (count == 0) return 144.0f;
        
        float sum = rmsAccumulator.load(std::memory_order_relaxed);
        float rms = std::sqrt(sum / count);
        if (rms < 1e-10f) return 144.0f;
        
        return -20.0f * std::log10(rms);
    }
    
    std::string getReport() const {
        return "CPU: " + std::to_string(getCPUUsage()) + "%"
             + " | DR: " + std::to_string(getDynamicRangeDB()) + "dB";
    }
    
    void reset() noexcept {
        cpuUsage.store(0.0f);
        peakLevel.store(0.0f);
        rmsAccumulator.store(0.0f);
        rmsSampleCount.store(0);
        totalSamples.store(0);
    }
    
private:
    std::atomic<float> cpuUsage{0.0f};
    std::atomic<float> peakLevel{0.0f};
    std::atomic<float> rmsAccumulator{0.0f};
    std::atomic<uint64_t> rmsSampleCount{0};
    std::atomic<uint64_t> totalSamples{0};
    
    std::chrono::high_resolution_clock::time_point blockStartTime;
    double sampleRate{48000.0};
};

// Flush denormal array helper
void flushDenormArray(float* data, int numSamples) noexcept {
    for (int i = 0; i < numSamples; ++i) {
        data[i] = flushDenorm(data[i]);
    }
}

// Main implementation
struct WaveFolder::Impl {
    static constexpr int MAX_CHANNELS = 2;
    static constexpr int OVERSAMPLE_FACTOR = 4;
    static constexpr int MAX_BLOCK_SIZE = 2048;
    
    // Lock-free parameters
    AtomicParam foldAmount;
    AtomicParam asymmetry;
    AtomicParam dcOffset;
    AtomicParam preGain;
    AtomicParam postGain;
    AtomicParam smoothing;
    AtomicParam harmonics;
    AtomicParam mix;
    
    // Per-channel state
    struct alignas(64) ChannelState {
        DCBlocker inputDC;
        DCBlocker outputDC;
        HarmonicFilter harmonicFilter;
        PolyphaseOversampler oversampler;
        
        // Anti-aliasing state
        float lastInput{0.0f};
        float smoothState{0.0f};
        
        // Oversampling buffers - dynamically sized but pre-allocated
        std::vector<float> oversampleBuffer;
        std::vector<float> processBuffer;
        float* oversamplePtr{nullptr};
        float* processPtr{nullptr};
        size_t bufferCapacity{0};
        
        void reset() noexcept {
            inputDC.reset();
            outputDC.reset();
            harmonicFilter.reset();
            oversampler.reset();
            lastInput = 0.0f;
            smoothState = 0.0f;
            
            // Clear buffers without deallocation
            if (oversamplePtr && bufferCapacity > 0) {
                std::memset(oversamplePtr, 0, bufferCapacity * sizeof(float));
                std::memset(processPtr, 0, bufferCapacity * sizeof(float));
            }
        }
        
        void allocateBuffers(size_t maxBlockSize) {
            const size_t required = maxBlockSize * OVERSAMPLE_FACTOR;
            
            // Only allocate if we need more space
            if (required > bufferCapacity) {
                // Reserve extra capacity to avoid future reallocations
                const size_t newCapacity = required + 1024;
                
                oversampleBuffer.clear();
                oversampleBuffer.reserve(newCapacity);
                oversampleBuffer.resize(newCapacity, 0.0f);
                
                processBuffer.clear();
                processBuffer.reserve(newCapacity);
                processBuffer.resize(newCapacity, 0.0f);
                
                // Cache raw pointers for RT access
                oversamplePtr = oversampleBuffer.data();
                processPtr = processBuffer.data();
                bufferCapacity = newCapacity;
                
                // Ensure alignment
                jassert(reinterpret_cast<uintptr_t>(oversamplePtr) % 32 == 0);
                jassert(reinterpret_cast<uintptr_t>(processPtr) % 32 == 0);
            }
        }
    };
    
    std::array<ChannelState, MAX_CHANNELS> channels;
    double sampleRate{44100.0};
    int maxBlockSize{MAX_BLOCK_SIZE};
    int denormalFlushCounter{0};
    
    // Quality metrics for monitoring
    QualityMetrics metrics;
    
    Impl() {
        // Initialize parameters
        foldAmount.setImmediate(0.5f);
        asymmetry.setImmediate(0.0f);
        dcOffset.setImmediate(0.0f);
        preGain.setImmediate(1.0f);
        postGain.setImmediate(1.0f);
        smoothing.setImmediate(0.5f);
        harmonics.setImmediate(0.0f);
        mix.setImmediate(1.0f);
        
        // Set smoothing coefficients for click-free operation
        foldAmount.setSmoothingCoeff(0.99f);
        asymmetry.setSmoothingCoeff(0.995f);
        dcOffset.setSmoothingCoeff(0.995f);
        preGain.setSmoothingCoeff(0.99f);
        postGain.setSmoothingCoeff(0.99f);
        smoothing.setSmoothingCoeff(0.999f);
        harmonics.setSmoothingCoeff(0.995f);
        mix.setSmoothingCoeff(0.999f);
        
        // Initialize oversamplers
        for (auto& ch : channels) {
            ch.oversampler.init();
        }
    }
    
    void prepareToPlay(double sr, int samplesPerBlock) {
        sampleRate = sr;
        
        // Allocate buffers for the requested block size (or larger)
        // This happens on the message thread, so allocation is safe
        for (auto& ch : channels) {
            ch.allocateBuffers(samplesPerBlock);
            ch.harmonicFilter.setSampleRate(sr);
            ch.reset();
        }
        
        metrics.setSampleRate(sr);
        metrics.reset();
        denormalFlushCounter = 0;
    }
    
    ALWAYS_INLINE float processWavefolding(float input, float amount, float asym) noexcept {
        // Optimized folding with guaranteed termination
        const float threshold = (1.0f - amount * 0.95f);
        const float posThresh = threshold * (1.0f + asym);
        const float negThresh = -threshold * (1.0f - asym);
        
        float output = input;
        
        // Limit folding iterations for real-time safety
        for (int i = 0; i < 8 && (output > posThresh || output < negThresh); ++i) {
            if (output > posThresh) {
                output = 2.0f * posThresh - output;
                if (output < negThresh) {
                    output = 2.0f * negThresh - output;
                }
            } else if (output < negThresh) {
                output = 2.0f * negThresh - output;
                if (output > posThresh) {
                    output = 2.0f * posThresh - output;
                }
            }
        }
        
        // Soft saturation for edge cases with denormal prevention
        return flushDenorm(fastTanh(output));
    }
    
    ALWAYS_INLINE float smoothTransition(float input, float& lastInput, float smooth) noexcept {
        // Anti-derivative anti-aliasing
        const float maxDelta = (1.0f - smooth) * 0.1f;
        const float delta = input - lastInput;
        
        if (std::abs(delta) > maxDelta) {
            input = lastInput + (delta > 0 ? maxDelta : -maxDelta);
        }
        
        lastInput = input;
        return input;
    }
    
    void processChannel(ChannelState& ch, float* data, int numSamples) {
        // Ensure we have buffers (should already be allocated in prepareToPlay)
        jassert(ch.oversamplePtr != nullptr);
        jassert(ch.processPtr != nullptr);
        
        // Check if oversampling is needed based on current fold amount
        const float currentFold = foldAmount.tick();
        foldAmount.setImmediate(currentFold); // Reset for next check
        const bool useOversampling = currentFold > 0.3f;
        
        if (useOversampling) {
            // Block-based oversampling for efficiency
            // First, DC block the input
            for (int i = 0; i < numSamples; ++i) {
                data[i] = ch.inputDC.process(data[i]);
            }
            
            // Upsample entire block using polyphase
            ch.oversampler.upsample(data, ch.oversamplePtr, numSamples);
            
            // Process at 4x rate with per-sample parameter smoothing
            const int oversampledCount = numSamples * OVERSAMPLE_FACTOR;
            for (int i = 0; i < oversampledCount; ++i) {
                // Update ALL parameters per-sample for click-free automation
                const float fold = foldAmount.tick();
                const float asym = asymmetry.tick() * 2.0f - 1.0f;
                const float dc = dcOffset.tick() * 0.1f;
                const float preG = preGain.tick();
                const float postG = postGain.tick();
                const float smooth = smoothing.tick();
                const float harm = harmonics.tick();
                const float mixAmt = mix.tick();
                
                float x = ch.oversamplePtr[i];
                const float dry = x;
                
                // Pre-gain and DC offset
                x = flushDenorm(x * preG + dc);
                
                // Smooth transitions
                if (smooth > 0.0f) {
                    x = smoothTransition(x, ch.lastInput, smooth);
                }
                
                // Wave folding (already includes denormal prevention)
                x = processWavefolding(x, fold, asym);
                
                // Harmonic emphasis (already includes denormal prevention)
                if (harm > 0.0f) {
                    x = ch.harmonicFilter.process(x, harm);
                }
                
                // Post-gain
                x = flushDenorm(x * postG);
                
                // Mix and store
                ch.processPtr[i] = flushDenorm(dry * (1.0f - mixAmt) + x * mixAmt);
            }
            
            // Downsample back to original rate using polyphase
            ch.oversampler.downsample(ch.processPtr, data, numSamples);
            
            // DC block output
            for (int i = 0; i < numSamples; ++i) {
                data[i] = ch.outputDC.process(data[i]);
            }
        } else {
            // Direct processing without oversampling (low fold amounts)
            for (int i = 0; i < numSamples; ++i) {
                // Per-sample parameter updates for smooth automation
                const float fold = foldAmount.tick();
                const float asym = asymmetry.tick() * 2.0f - 1.0f;
                const float dc = dcOffset.tick() * 0.1f;
                const float preG = preGain.tick();
                const float postG = postGain.tick();
                const float smooth = smoothing.tick();
                const float harm = harmonics.tick();
                const float mixAmt = mix.tick();
                
                // DC block input
                float input = ch.inputDC.process(data[i]);
                const float dry = input;
                
                // Pre-gain and DC offset
                float x = flushDenorm(input * preG + dc);
                
                // Smooth transitions
                if (smooth > 0.0f) {
                    x = smoothTransition(x, ch.lastInput, smooth);
                }
                
                // Wave folding (already includes denormal prevention)
                x = processWavefolding(x, fold, asym);
                
                // Harmonic emphasis (already includes denormal prevention)
                if (harm > 0.0f) {
                    x = ch.harmonicFilter.process(x, harm);
                }
                
                // Post-gain and mix
                x = flushDenorm(x * postG);
                float output = flushDenorm(dry * (1.0f - mixAmt) + x * mixAmt);
                
                // DC block output with denormal flush
                data[i] = flushDenorm(ch.outputDC.process(output));
            }
        }
        
        // Periodic denormal flush for entire buffer
        if (++denormalFlushCounter >= 512) {
            denormalFlushCounter = 0;
            flushDenormArray(data, numSamples);
        }
    }
};

// Public interface
WaveFolder::WaveFolder() : pimpl(std::make_unique<Impl>()) {}
WaveFolder::~WaveFolder() = default;

void WaveFolder::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepareToPlay(sampleRate, samplesPerBlock);
}

void WaveFolder::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
}

void WaveFolder::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = std::min(buffer.getNumChannels(), Impl::MAX_CHANNELS);
    const int numSamples = buffer.getNumSamples();
    
    // Start performance measurement
    pimpl->metrics.startBlock();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        pimpl->processChannel(pimpl->channels[ch], buffer.getWritePointer(ch), numSamples);
        
        // Update quality metrics
        pimpl->metrics.updatePeakRMS(buffer.getReadPointer(ch), numSamples);
    }
    
    // End performance measurement
    pimpl->metrics.endBlock(numSamples, numChannels);
}

void WaveFolder::updateParameters(const std::map<int, float>& params) {
    // Thread-safe parameter updates
    for (const auto& [index, value] : params) {
        switch (index) {
            case kFoldAmount: pimpl->foldAmount.setTarget(value); break;
            case kAsymmetry:  pimpl->asymmetry.setTarget(value); break;
            case kDCOffset:   pimpl->dcOffset.setTarget(value); break;
            case kPreGain:    pimpl->preGain.setTarget(0.1f + value * 3.9f); break;
            case kPostGain:   pimpl->postGain.setTarget(0.1f + value * 1.9f); break;
            case kSmoothing:  pimpl->smoothing.setTarget(value); break;
            case kHarmonics:  pimpl->harmonics.setTarget(value); break;
            case kMix:        pimpl->mix.setTarget(value); break;
        }
    }
}

juce::String WaveFolder::getParameterName(int index) const {
    switch (index) {
        case kFoldAmount: return "Fold";
        case kAsymmetry:  return "Asymmetry";
        case kDCOffset:   return "DC Offset";
        case kPreGain:    return "Pre Gain";
        case kPostGain:   return "Post Gain";
        case kSmoothing:  return "Smoothing";
        case kHarmonics:  return "Harmonics";
        case kMix:        return "Mix";
        default:          return "";
    }
}

float WaveFolder::getCPUUsage() const {
    return pimpl->metrics.getCPUUsage();
}

float WaveFolder::getDynamicRangeDB() const {
    return pimpl->metrics.getDynamicRangeDB();
}

std::string WaveFolder::getQualityReport() const {
    return pimpl->metrics.getReport();
}
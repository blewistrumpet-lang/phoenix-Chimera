#include "ParametricEQ_Platinum.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <memory>
#include <array>
#include <atomic>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE 1
#else
    #define HAS_SSE 0
#endif

// Anonymous namespace for internal implementation
namespace {

// Aligned memory allocator for SIMD
template<typename T>
T* alignedAlloc(size_t count, size_t alignment = 32) {
    void* ptr = nullptr;
    #ifdef _WIN32
    ptr = _aligned_malloc(count * sizeof(T), alignment);
    #else
    if (posix_memalign(&ptr, alignment, count * sizeof(T)) != 0) {
        ptr = nullptr;
    }
    #endif
    return static_cast<T*>(ptr);
}

template<typename T>
void alignedFree(T* ptr) {
    #ifdef _WIN32
    _aligned_free(ptr);
    #else
    free(ptr);
    #endif
}


// CPU denormal mode - set once at startup
struct DenormalDisabler {
    DenormalDisabler() noexcept {
        #if HAS_SSE
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
        #endif
    }
} g_denormalDisabler;

// One-pole smoother with FULL denormal protection
class EQOnePoleFilter {
public:
    void setSampleRate(double sr) noexcept { 
        sampleRate = sr; 
    }
    
    // Pre-compute coefficient, never call in RT thread
    void setTimeMs(double ms) noexcept {
        if (ms <= 0.0) {
            a = 1.0;  // Instant
        } else {
            // Proper one-pole coefficient
            a = 1.0 - std::exp(-1000.0 / (ms * sampleRate));
        }
    }
    
    void reset(double value) noexcept {
        z1 = target = value;
    }
    
    void setTarget(double t) noexcept { 
        target = t; 
    }
    
    inline double process() noexcept {
        z1 += a * (target - z1);
        z1 = DSPUtils::flushDenorm(z1);  // CRITICAL: Flush here!
        return z1;
    }
    
    double getCurrentValue() const noexcept { return z1; }
    
    bool isSmoothing() const noexcept { 
        return std::abs(target - z1) > 1e-6; 
    }
    
private:
    double z1 = 0.0;
    double target = 0.0;
    double a = 0.0;
    double sampleRate = 44100.0;
};

// Platinum Biquad with configurable coefficient smoothing
class PlatinumBiquad {
public:
    enum class FilterType {
        LOW_SHELF,
        HIGH_SHELF,
        PEAK
    };
    
    PlatinumBiquad(FilterType t = FilterType::PEAK) 
        : type(t) {
        reset();
    }
    
    void reset() noexcept {
        s1 = s2 = 0.0;
        b0_cur = b0_tgt = 1.0;
        b1_cur = b1_tgt = 0.0;
        b2_cur = b2_tgt = 0.0;
        a1_cur = a1_tgt = 0.0;
        a2_cur = a2_tgt = 0.0;
    }
    
    // Set coefficient smoothing time (call in prepareToPlay)
    void setCoeffSmoothingMs(double ms, double sampleRate) noexcept {
        if (ms <= 0.0) {
            coeffRamp = 1.0;  // Instant
        } else {
            coeffRamp = 1.0 - std::exp(-1000.0 / (ms * sampleRate));
        }
    }
    
    // Process with zipper-free coefficient interpolation
    inline double process(double input) noexcept {
        // Smooth coefficients
        b0_cur += (b0_tgt - b0_cur) * coeffRamp;
        b1_cur += (b1_tgt - b1_cur) * coeffRamp;
        b2_cur += (b2_tgt - b2_cur) * coeffRamp;
        a1_cur += (a1_tgt - a1_cur) * coeffRamp;
        a2_cur += (a2_tgt - a2_cur) * coeffRamp;
        
        // DF-II Transposed
        double output = b0_cur * input + s1;
        s1 = b1_cur * input - a1_cur * output + s2;
        s2 = b2_cur * input - a2_cur * output;
        
        // CRITICAL: Flush state variables
        s1 = DSPUtils::flushDenorm(s1);
        s2 = DSPUtils::flushDenorm(s2);
        
        // Safety check
        if (!std::isfinite(output)) {
            reset();
            return 0.0;
        }
        
        return output;
    }
    
    // SIMD-friendly block processing (4 samples at once)
    void processBlock4(const float* input, float* output) noexcept {
        #if HAS_SSE
        __m128d b0v = _mm_set1_pd(b0_cur);
        __m128d b1v = _mm_set1_pd(b1_cur);
        __m128d b2v = _mm_set1_pd(b2_cur);
        __m128d a1v = _mm_set1_pd(a1_cur);
        __m128d a2v = _mm_set1_pd(a2_cur);
        
        // Process 4 samples (2 at a time in double precision)
        for (int i = 0; i < 4; i += 2) {
            __m128d in = _mm_cvtps_pd(_mm_load_ps(input + i));
            __m128d s1v = _mm_set1_pd(s1);
            __m128d s2v = _mm_set1_pd(s2);
            
            __m128d out = _mm_add_pd(_mm_mul_pd(b0v, in), s1v);
            
            // Update states (simplified for clarity)
            s1 = _mm_cvtsd_f64(_mm_add_pd(
                _mm_sub_pd(_mm_mul_pd(b1v, in), _mm_mul_pd(a1v, out)),
                s2v
            ));
            s2 = _mm_cvtsd_f64(_mm_sub_pd(
                _mm_mul_pd(b2v, in),
                _mm_mul_pd(a2v, out)
            ));
            
            _mm_store_ps(output + i, _mm_cvtpd_ps(out));
        }
        
        // Flush denormals
        s1 = DSPUtils::flushDenorm(s1);
        s2 = DSPUtils::flushDenorm(s2);
        #else
        // Fallback to scalar processing
        for (int i = 0; i < 4; ++i) {
            output[i] = static_cast<float>(process(input[i]));
        }
        #endif
    }
    
    void setCoefficients(double freq, double gainDb, double Q, double sampleRate) noexcept {
        double K = std::tan(M_PI * freq / sampleRate);
        double K2 = K * K;
        double V = std::pow(10.0, std::abs(gainDb) / 20.0);
        
        if (type == FilterType::LOW_SHELF) {
            double sqrt2V = std::sqrt(2.0 * V);
            
            if (gainDb >= 0) {
                double norm = 1.0 / (1.0 + std::sqrt(2.0) * K + K2);
                b0_tgt = (1.0 + sqrt2V * K + V * K2) * norm;
                b1_tgt = 2.0 * (V * K2 - 1.0) * norm;
                b2_tgt = (1.0 - sqrt2V * K + V * K2) * norm;
                a1_tgt = 2.0 * (K2 - 1.0) * norm;
                a2_tgt = (1.0 - std::sqrt(2.0) * K + K2) * norm;
            } else {
                double norm = 1.0 / (V + sqrt2V * K + K2);
                b0_tgt = V * (1.0 + std::sqrt(2.0) * K + K2) * norm;
                b1_tgt = 2.0 * V * (K2 - 1.0) * norm;
                b2_tgt = V * (1.0 - std::sqrt(2.0) * K + K2) * norm;
                a1_tgt = 2.0 * (K2 - V) * norm;
                a2_tgt = (V - sqrt2V * K + K2) * norm;
            }
        }
        else if (type == FilterType::HIGH_SHELF) {
            double sqrt2V = std::sqrt(2.0 * V);
            
            if (gainDb >= 0) {
                double norm = 1.0 / (1.0 + std::sqrt(2.0) * K + K2);
                b0_tgt = (V + sqrt2V * K + K2) * norm;
                b1_tgt = 2.0 * (K2 - V) * norm;
                b2_tgt = (V - sqrt2V * K + K2) * norm;
                a1_tgt = 2.0 * (K2 - 1.0) * norm;
                a2_tgt = (1.0 - std::sqrt(2.0) * K + K2) * norm;
            } else {
                double norm = 1.0 / (1.0 + sqrt2V * K + V * K2);
                b0_tgt = V * (1.0 + std::sqrt(2.0) * K + K2) * norm;
                b1_tgt = 2.0 * V * (K2 - 1.0) * norm;
                b2_tgt = V * (1.0 - std::sqrt(2.0) * K + K2) * norm;
                a1_tgt = 2.0 * (V * K2 - 1.0) * norm;
                a2_tgt = (1.0 - sqrt2V * K + V * K2) * norm;
            }
        }
        else { // PEAK
            if (gainDb >= 0) {
                double norm = 1.0 / (1.0 + K/Q + K2);
                b0_tgt = (1.0 + V*K/Q + K2) * norm;
                b1_tgt = 2.0 * (K2 - 1.0) * norm;
                b2_tgt = (1.0 - V*K/Q + K2) * norm;
                a1_tgt = b1_tgt;
                a2_tgt = (1.0 - K/Q + K2) * norm;
            } else {
                double norm = 1.0 / (1.0 + V*K/Q + K2);
                b0_tgt = (1.0 + K/Q + K2) * norm;
                b1_tgt = 2.0 * (K2 - 1.0) * norm;
                b2_tgt = (1.0 - K/Q + K2) * norm;
                a1_tgt = b1_tgt;
                a2_tgt = (1.0 - V*K/Q + K2) * norm;
            }
        }
    }
    
private:
    FilterType type;
    double s1 = 0.0, s2 = 0.0;
    double b0_cur, b1_cur, b2_cur, a1_cur, a2_cur;
    double b0_tgt, b1_tgt, b2_tgt, a1_tgt, a2_tgt;
    double coeffRamp = 0.001;
};

// Fast tanh with denormal protection
inline float fastTanhSafe(float x) noexcept {
    float x2 = x * x;
    float num = x * (27.0f + x2);
    float den = 27.0f + 9.0f * x2;
    return DSPUtils::flushDenorm(num / den);
}

} // anonymous namespace

// PIMPL implementation with aligned buffers
struct ParametricEQ_Platinum::Impl {
    // Configuration
    double sampleRate = 44100.0;
    int blockSize = 512;
    static constexpr int maxChannels = 8;  // Support up to 8 channels
    
    // User-configurable smoothing times
    double paramSmoothingMs = 2.0;
    double coeffSmoothingMs = 1.0;
    
    // Smoothed parameters (with denormal protection)
    EQOnePoleFilter lowGain, lowFreq;
    EQOnePoleFilter midGain, midFreq, midQ;
    EQOnePoleFilter highGain, highFreq;
    EQOnePoleFilter outputGain, mix;
    
    // Filter banks (support up to 8 channels)
    std::array<PlatinumBiquad, maxChannels> lowShelf;
    std::array<PlatinumBiquad, maxChannels> midBand;
    std::array<PlatinumBiquad, maxChannels> highShelf;
    
    // ALIGNED buffers for SIMD (no allocations in RT!)
    float* dryBuffer = nullptr;
    float* tempBuffer = nullptr;
    size_t bufferCapacity = 0;
    
    // State tracking
    double lastLowFreq = -1.0;
    double lastLowGain = -1.0;
    double lastMidFreq = -1.0;
    double lastMidGain = -1.0;
    double lastMidQ = -1.0;
    double lastHighFreq = -1.0;
    double lastHighGain = -1.0;
    
    // Atomic parameters for thread-safe updates
    std::array<std::atomic<float>, 9> params;
    
    // Constructor
    Impl() {
        // Initialize filter types
        for (int ch = 0; ch < maxChannels; ++ch) {
            lowShelf[ch] = PlatinumBiquad(PlatinumBiquad::FilterType::LOW_SHELF);
            midBand[ch] = PlatinumBiquad(PlatinumBiquad::FilterType::PEAK);
            highShelf[ch] = PlatinumBiquad(PlatinumBiquad::FilterType::HIGH_SHELF);
        }
        
        // Initialize atomic parameters
        params[kLowGain] = 0.5f;
        params[kLowFreq] = 0.15f;
        params[kMidGain] = 0.5f;
        params[kMidFreq] = 0.5f;
        params[kMidQ] = 0.5f;
        params[kHighGain] = 0.5f;
        params[kHighFreq] = 0.8f;
        params[kOutputGain] = 0.5f;
        params[kMix] = 1.0f;
    }
    
    // Destructor - clean up aligned memory
    ~Impl() {
        if (dryBuffer) alignedFree(dryBuffer);
        if (tempBuffer) alignedFree(tempBuffer);
    }
    
    // Allocate aligned buffers (call only in prepareToPlay)
    void allocateBuffers(int samplesPerBlock) {
        size_t requiredSize = maxChannels * samplesPerBlock;
        
        // Reserve extra space to prevent reallocation
        size_t newCapacity = requiredSize * 2;
        
        if (newCapacity > bufferCapacity) {
            // Free old buffers
            if (dryBuffer) alignedFree(dryBuffer);
            if (tempBuffer) alignedFree(tempBuffer);
            
            // Allocate new aligned buffers
            dryBuffer = alignedAlloc<float>(newCapacity, 32);
            tempBuffer = alignedAlloc<float>(newCapacity, 32);
            
            // Zero initialize
            std::memset(dryBuffer, 0, newCapacity * sizeof(float));
            std::memset(tempBuffer, 0, newCapacity * sizeof(float));
            
            bufferCapacity = newCapacity;
        }
    }
    
    void updateCoefficientsIfNeeded() {
        double currentLowFreq = lowFreq.getCurrentValue();
        double currentLowGain = lowGain.getCurrentValue();
        double currentMidFreq = midFreq.getCurrentValue();
        double currentMidGain = midGain.getCurrentValue();
        double currentMidQ = midQ.getCurrentValue();
        double currentHighFreq = highFreq.getCurrentValue();
        double currentHighGain = highGain.getCurrentValue();
        
        constexpr double threshold = 0.0001;
        
        bool needsUpdate = 
            std::abs(currentLowFreq - lastLowFreq) > threshold ||
            std::abs(currentLowGain - lastLowGain) > threshold ||
            std::abs(currentMidFreq - lastMidFreq) > threshold ||
            std::abs(currentMidGain - lastMidGain) > threshold ||
            std::abs(currentMidQ - lastMidQ) > threshold ||
            std::abs(currentHighFreq - lastHighFreq) > threshold ||
            std::abs(currentHighGain - lastHighGain) > threshold;
        
        if (needsUpdate) {
            double lowFreqHz = 20.0 + currentLowFreq * currentLowFreq * 480.0;
            double midFreqHz = 200.0 + currentMidFreq * currentMidFreq * 4800.0;
            double highFreqHz = 1000.0 + currentHighFreq * currentHighFreq * 14000.0;
            
            double lowGainDb = (currentLowGain - 0.5) * 24.0;
            double midGainDb = (currentMidGain - 0.5) * 24.0;
            double highGainDb = (currentHighGain - 0.5) * 24.0;
            
            double midQValue = 0.3 + currentMidQ * 4.7;
            
            // Update all channel filters
            for (int ch = 0; ch < maxChannels; ++ch) {
                lowShelf[ch].setCoefficients(lowFreqHz, lowGainDb, 0.707, sampleRate);
                midBand[ch].setCoefficients(midFreqHz, midGainDb, midQValue, sampleRate);
                highShelf[ch].setCoefficients(highFreqHz, highGainDb, 0.707, sampleRate);
            }
            
            lastLowFreq = currentLowFreq;
            lastLowGain = currentLowGain;
            lastMidFreq = currentMidFreq;
            lastMidGain = currentMidGain;
            lastMidQ = currentMidQ;
            lastHighFreq = currentHighFreq;
            lastHighGain = currentHighGain;
        }
    }
};

// Public interface implementation
ParametricEQ_Platinum::ParametricEQ_Platinum() 
    : pimpl(std::make_unique<Impl>()) {
}

ParametricEQ_Platinum::~ParametricEQ_Platinum() = default;

void ParametricEQ_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = sampleRate;
    pimpl->blockSize = samplesPerBlock;
    
    // CRITICAL: Pre-allocate aligned buffers with reserve
    pimpl->allocateBuffers(samplesPerBlock);
    
    // Pre-compute all smoothing coefficients (never in RT!)
    pimpl->lowGain.setSampleRate(sampleRate);
    pimpl->lowGain.setTimeMs(pimpl->paramSmoothingMs);
    pimpl->lowGain.reset(pimpl->params[kLowGain]);
    
    pimpl->lowFreq.setSampleRate(sampleRate);
    pimpl->lowFreq.setTimeMs(pimpl->paramSmoothingMs);
    pimpl->lowFreq.reset(pimpl->params[kLowFreq]);
    
    pimpl->midGain.setSampleRate(sampleRate);
    pimpl->midGain.setTimeMs(pimpl->paramSmoothingMs);
    pimpl->midGain.reset(pimpl->params[kMidGain]);
    
    pimpl->midFreq.setSampleRate(sampleRate);
    pimpl->midFreq.setTimeMs(pimpl->paramSmoothingMs);
    pimpl->midFreq.reset(pimpl->params[kMidFreq]);
    
    pimpl->midQ.setSampleRate(sampleRate);
    pimpl->midQ.setTimeMs(pimpl->paramSmoothingMs);
    pimpl->midQ.reset(pimpl->params[kMidQ]);
    
    pimpl->highGain.setSampleRate(sampleRate);
    pimpl->highGain.setTimeMs(pimpl->paramSmoothingMs);
    pimpl->highGain.reset(pimpl->params[kHighGain]);
    
    pimpl->highFreq.setSampleRate(sampleRate);
    pimpl->highFreq.setTimeMs(pimpl->paramSmoothingMs);
    pimpl->highFreq.reset(pimpl->params[kHighFreq]);
    
    pimpl->outputGain.setSampleRate(sampleRate);
    pimpl->outputGain.setTimeMs(pimpl->paramSmoothingMs);
    pimpl->outputGain.reset(pimpl->params[kOutputGain]);
    
    pimpl->mix.setSampleRate(sampleRate);
    pimpl->mix.setTimeMs(pimpl->paramSmoothingMs);
    pimpl->mix.reset(pimpl->params[kMix]);
    
    // Set coefficient smoothing for all filters
    for (int ch = 0; ch < pimpl->maxChannels; ++ch) {
        pimpl->lowShelf[ch].setCoeffSmoothingMs(pimpl->coeffSmoothingMs, sampleRate);
        pimpl->midBand[ch].setCoeffSmoothingMs(pimpl->coeffSmoothingMs, sampleRate);
        pimpl->highShelf[ch].setCoeffSmoothingMs(pimpl->coeffSmoothingMs, sampleRate);
    }
    
    reset();
    
    // Force initial coefficient update
    pimpl->lastLowFreq = -1.0;
    pimpl->updateCoefficientsIfNeeded();
}

void ParametricEQ_Platinum::reset() {
    for (int ch = 0; ch < pimpl->maxChannels; ++ch) {
        pimpl->lowShelf[ch].reset();
        pimpl->midBand[ch].reset();
        pimpl->highShelf[ch].reset();
    }
}

void ParametricEQ_Platinum::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    const int numChannels = std::min(buffer.getNumChannels(), pimpl->maxChannels);
    const int numSamples = buffer.getNumSamples();
    
    // Update smoother targets from atomics
    pimpl->lowGain.setTarget(pimpl->params[kLowGain].load(std::memory_order_relaxed));
    pimpl->lowFreq.setTarget(pimpl->params[kLowFreq].load(std::memory_order_relaxed));
    pimpl->midGain.setTarget(pimpl->params[kMidGain].load(std::memory_order_relaxed));
    pimpl->midFreq.setTarget(pimpl->params[kMidFreq].load(std::memory_order_relaxed));
    pimpl->midQ.setTarget(pimpl->params[kMidQ].load(std::memory_order_relaxed));
    pimpl->highGain.setTarget(pimpl->params[kHighGain].load(std::memory_order_relaxed));
    pimpl->highFreq.setTarget(pimpl->params[kHighFreq].load(std::memory_order_relaxed));
    pimpl->outputGain.setTarget(pimpl->params[kOutputGain].load(std::memory_order_relaxed));
    pimpl->mix.setTarget(pimpl->params[kMix].load(std::memory_order_relaxed));
    
    // Block-process parameter smoothing
    for (int i = 0; i < numSamples; ++i) {
        pimpl->lowGain.process();
        pimpl->lowFreq.process();
        pimpl->midGain.process();
        pimpl->midFreq.process();
        pimpl->midQ.process();
        pimpl->highGain.process();
        pimpl->highFreq.process();
        pimpl->outputGain.process();
        pimpl->mix.process();
    }
    
    // Update coefficients if needed
    pimpl->updateCoefficientsIfNeeded();
    
    // Get current mix/output values
    const float mixAmount = static_cast<float>(pimpl->mix.getCurrentValue());
    const float outGain = static_cast<float>(0.25 + pimpl->outputGain.getCurrentValue() * 1.5);
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        
        // Use aligned buffers
        float* dryPtr = pimpl->dryBuffer + channel * numSamples;
        
        // Copy dry signal (aligned)
        std::memcpy(dryPtr, channelData, numSamples * sizeof(float));
        
        // Process samples (consider SIMD for blocks of 4)
        for (int sample = 0; sample < numSamples; ++sample) {
            double input = static_cast<double>(channelData[sample]);
            
            // Three-band EQ chain
            double processed = pimpl->lowShelf[channel].process(input);
            processed = pimpl->midBand[channel].process(processed);
            processed = pimpl->highShelf[channel].process(processed);
            
            // Apply output gain
            processed *= outGain;
            
            // Mix dry/wet with denormal protection
            double mixed = dryPtr[sample] * (1.0 - mixAmount) + 
                          processed * mixAmount;
            mixed = DSPUtils::flushDenorm(mixed);
            
            // Soft limiting with denormal-safe tanh
            if (std::abs(mixed) > 0.95) {
                mixed = 0.95 * fastTanhSafe(static_cast<float>(mixed / 0.95));
            }
            
            // Final denormal flush before output
            channelData[sample] = static_cast<float>(DSPUtils::flushDenorm(mixed));
        }
    }
    
    scrubBuffer(buffer);
}

void ParametricEQ_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index >= 0 && index < 9) {
            pimpl->params[index].store(value, std::memory_order_relaxed);
        }
    }
}

juce::String ParametricEQ_Platinum::getParameterName(int index) const {
    switch (index) {
        case kLowGain:    return "Low Gain";
        case kLowFreq:    return "Low Freq";
        case kMidGain:    return "Mid Gain";
        case kMidFreq:    return "Mid Freq";
        case kMidQ:       return "Mid Q";
        case kHighGain:   return "High Gain";
        case kHighFreq:   return "High Freq";
        case kOutputGain: return "Output";
        case kMix:        return "Mix";
        default:          return "";
    }
}
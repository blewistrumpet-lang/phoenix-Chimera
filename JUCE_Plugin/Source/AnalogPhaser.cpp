#include "AnalogPhaser.h"
#include "Denorm.hpp"
#include "QualityMetrics.hpp"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>
#include <array>
#include <random>

// Platform-specific SIMD includes
#if HAS_SSE
    #include <immintrin.h>
#endif

// Enable FTZ/DAZ globally
namespace {
    struct DenormalGuard {
        DenormalGuard() {
            #if HAS_SSE
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
            #endif
        }
    } static denormalGuard;
}

// Thread-safe random number generation
class RTRandom {
public:
    RTRandom() : state(std::random_device{}()) {}
    
    // Linear congruential generator - fast and RT-safe
    ALWAYS_INLINE float nextFloat() noexcept {
        state = state * 1664525u + 1013904223u;
        return (state >> 9) * (1.0f / 8388608.0f) - 1.0f;
    }
    
private:
    uint32_t state;
};

// Lock-free parameter with smoothing
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
    
    float getValue() const noexcept { return current; }
    
private:
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float smoothing{0.995f};
};

// All-pass filter with denormal prevention
class AllPassFilter {
public:
    void reset() noexcept {
        state = 0.0f;
        coefficient = 0.0f;
        lastCoeff = 0.0f;
        smoothingRate = 0.001f;
    }
    
    void setCoefficient(float coeff) noexcept {
        coefficient = coeff;
    }
    
    void setSmoothingRate(float rate) noexcept {
        smoothingRate = rate;
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        // Interpolate coefficient for click-free modulation
        lastCoeff += (coefficient - lastCoeff) * smoothingRate;
        lastCoeff = flushDenorm(lastCoeff);
        
        // All-pass difference equation with denormal prevention
        const float output = flushDenorm(-input + state);
        state = flushDenorm(input + lastCoeff * output);
        
        return output;
    }
    
private:
    float state{0.0f};
    float coefficient{0.0f};
    float lastCoeff{0.0f};
    float smoothingRate{0.001f};
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

// Main implementation
struct AnalogPhaser::Impl {
    static constexpr int MAX_STAGES = 8;
    static constexpr int MAX_CHANNELS = 2;
    static constexpr int COEFFICIENT_TABLE_SIZE = 4096;
    static constexpr int EXP2_TABLE_SIZE = 256;
    
    // Lock-free parameters
    AtomicParam rate;
    AtomicParam depth;
    AtomicParam feedback;
    AtomicParam stages;
    AtomicParam stereoSpread;
    AtomicParam centerFreq;
    AtomicParam resonance;
    AtomicParam mix;
    
    // Pre-computed lookup tables
    alignas(64) std::array<float, COEFFICIENT_TABLE_SIZE> coeffTable;
    alignas(64) std::array<float, EXP2_TABLE_SIZE> exp2Table;
    
    // Per-channel state
    struct alignas(64) ChannelState {
        std::array<AllPassFilter, MAX_STAGES> allpassFilters;
        DCBlocker inputDC;
        DCBlocker outputDC;
        RTRandom rng;
        
        double lfoPhase{0.0};
        float feedbackSample{0.0f};
        float thermalDrift{0.0f};
        
        void reset() noexcept {
            for (auto& filter : allpassFilters) {
                filter.reset();
            }
            inputDC.reset();
            outputDC.reset();
            lfoPhase = 0.0;
            feedbackSample = 0.0f;
            thermalDrift = 0.0f;
        }
    };
    
    std::array<ChannelState, MAX_CHANNELS> channels;
    double sampleRate{44100.0};
    float invSampleRate{1.0f / 44100.0f};
    int denormalFlushCounter{0};
    
    // Quality metrics for monitoring
    QualityMetrics metrics;
    
    // Parameter buffer for lock-free access
    alignas(64) std::array<float, 8> paramBuffer{};
    
    Impl() {
        // Initialize parameters
        rate.setImmediate(0.5f);
        depth.setImmediate(0.5f);
        feedback.setImmediate(0.3f);
        stages.setImmediate(0.5f);
        stereoSpread.setImmediate(0.5f);
        centerFreq.setImmediate(0.5f);
        resonance.setImmediate(0.3f);
        mix.setImmediate(0.5f);
        
        // Set smoothing coefficients
        rate.setSmoothingCoeff(0.995f);
        depth.setSmoothingCoeff(0.990f);
        feedback.setSmoothingCoeff(0.995f);
        stages.setSmoothingCoeff(0.999f);
        stereoSpread.setSmoothingCoeff(0.995f);
        centerFreq.setSmoothingCoeff(0.992f);
        resonance.setSmoothingCoeff(0.995f);
        mix.setSmoothingCoeff(0.995f);
        
        // Pre-compute coefficient table
        initCoefficientTable();
        
        // Pre-compute exp2 table
        initExp2Table();
    }
    
    void initCoefficientTable() {
        // Pre-compute all-pass coefficients for efficiency
        for (int i = 0; i < COEFFICIENT_TABLE_SIZE; ++i) {
            float normalizedFreq = static_cast<float>(i) / COEFFICIENT_TABLE_SIZE;
            float freq = 20.0f * std::pow(1000.0f, normalizedFreq); // 20Hz to 20kHz
            
            // Bilinear transform coefficient
            // Using fast approximation for tan
            float w = freq * 2.0f * M_PI / 48000.0f; // Normalized for 48kHz
            float tanw = fastTan(w * 0.5f);
            coeffTable[i] = (tanw - 1.0f) / (tanw + 1.0f);
        }
    }
    
    void initExp2Table() {
        // Pre-compute 2^x for x in [-1, 1]
        for (int i = 0; i < EXP2_TABLE_SIZE; ++i) {
            float x = -1.0f + 2.0f * i / (EXP2_TABLE_SIZE - 1);
            exp2Table[i] = flushDenorm(std::pow(2.0f, x));
        }
    }
    
    // Fast tan approximation for real-time use
    static ALWAYS_INLINE float fastTan(float x) noexcept {
        // Padé approximant for tan(x)
        const float x2 = x * x;
        const float num = x * (1.0f + x2 * (0.3333333f + x2 * 0.1333333f));
        const float den = 1.0f + x2 * (0.3333333f + x2 * 0.0666666f);
        return flushDenorm(num / den);
    }
    
    // Fast exp2 using lookup table
    ALWAYS_INLINE float fastExp2(float x) noexcept {
        // Clamp to table range [-1, 1]
        x = std::max(-1.0f, std::min(1.0f, x));
        
        // Convert to table index
        float fidx = (x + 1.0f) * 0.5f * (EXP2_TABLE_SIZE - 1);
        int idx = static_cast<int>(fidx);
        float frac = fidx - idx;
        
        // Bounds check
        if (idx >= EXP2_TABLE_SIZE - 1) {
            return exp2Table[EXP2_TABLE_SIZE - 1];
        }
        
        // Linear interpolation with denormal prevention
        float result = exp2Table[idx] + frac * (exp2Table[idx + 1] - exp2Table[idx]);
        return flushDenorm(result);
    }
    
    void prepareToPlay(double sr, int /*samplesPerBlock*/) {
        sampleRate = sr;
        invSampleRate = 1.0f / sr;
        
        // Rebuild coefficient table for actual sample rate
        const float minFreq = 20.0f;
        const float maxFreq = 20000.0f;
        
        for (int i = 0; i < COEFFICIENT_TABLE_SIZE; ++i) {
            float normalizedFreq = static_cast<float>(i) / (COEFFICIENT_TABLE_SIZE - 1);
            float freq = minFreq * std::pow(maxFreq / minFreq, normalizedFreq);
            float w = freq * 2.0f * static_cast<float>(M_PI) * invSampleRate;
            float tanw = fastTan(w * 0.5f);
            coeffTable[i] = flushDenorm((tanw - 1.0f) / (tanw + 1.0f));
        }
        
        // Reset channels
        for (auto& ch : channels) {
            ch.reset();
            // Set smoothing rate for all-pass filters based on sample rate
            float smoothingMs = 1.0f; // 1ms smoothing time
            float smoothingRate = 1.0f - std::exp(-1000.0f / (smoothingMs * sr));
            for (auto& filter : ch.allpassFilters) {
                filter.setSmoothingRate(smoothingRate);
            }
        }
        
        // Set stereo phase offset
        channels[1].lfoPhase = M_PI;
        
        // Set initial all-pass smoothing based on rate
        updateAllPassSmoothing();
        
        // Initialize metrics
        metrics.setSampleRate(sr);
        metrics.reset();
        
        // Initialize exp2 table
        initExp2Table();
        
        denormalFlushCounter = 0;
    }
    
    void updateAllPassSmoothing() {
        // Dynamic smoothing based on LFO rate and resonance
        float rateValue = rate.getValue();
        float resValue = resonance.getValue();
        
        // Faster rates need tighter tracking
        float baseSmoothing = 1.0f - std::exp(-(rateValue * 10.0f + 0.1f));
        
        // Higher resonance needs smoother changes to avoid instability
        float smoothingFactor = baseSmoothing * (1.0f - resValue * 0.5f);
        
        for (auto& ch : channels) {
            for (auto& filter : ch.allpassFilters) {
                filter.setSmoothingRate(flushDenorm(smoothingFactor));
            }
        }
    }
    
    ALWAYS_INLINE float generateLFO(double phase) noexcept {
        // Triangle wave with sine shaping
        float triangle = 1.0f - 4.0f * std::abs(0.5f - std::fmod(phase / (2.0 * M_PI), 1.0f));
        
        // Add sine component for smoother modulation
        float sine = std::sin(phase);
        
        // Blend with denormal prevention
        float shaped = triangle * 0.7f + sine * 0.3f;
        return flushDenorm(shaped);
    }
    
    ALWAYS_INLINE int getActiveStages(float stageParam) noexcept {
        if (stageParam < 0.25f) return 2;
        else if (stageParam < 0.5f) return 4;
        else if (stageParam < 0.75f) return 6;
        else return 8;
    }
    
    ALWAYS_INLINE float lookupCoefficient(float freq) noexcept {
        // Convert frequency to table index
        float normalizedFreq = (std::log(freq / 20.0f) / std::log(1000.0f));
        normalizedFreq = std::max(0.0f, std::min(1.0f, normalizedFreq));
        
        // Linear interpolation in table
        float fidx = normalizedFreq * (COEFFICIENT_TABLE_SIZE - 1);
        int idx = static_cast<int>(fidx);
        float frac = fidx - idx;
        
        if (idx >= COEFFICIENT_TABLE_SIZE - 1) {
            return coeffTable[COEFFICIENT_TABLE_SIZE - 1];
        }
        
        float interp = coeffTable[idx] + frac * (coeffTable[idx + 1] - coeffTable[idx]);
        return flushDenorm(interp);
    }
    
    void processChannel(ChannelState& ch, float* data, int numSamples, int channelIndex) {
        for (int i = 0; i < numSamples; ++i) {
            // Per-sample parameter updates for smooth automation
            const float rateHz = rate.tick() * 10.0f;
            const float depthAmt = depth.tick();
            const float fbAmt = feedback.tick() * 0.95f;
            const float stageParam = stages.tick();
            const float spread = stereoSpread.tick();
            const float center = centerFreq.tick();
            const float res = resonance.tick();
            const float mixAmt = mix.tick();
            
            // DC block input
            float input = ch.inputDC.process(data[i]);
            const float dry = input;
            
            // Update LFO phase with proper modulo wrap and denormal prevention
            float phaseDelta = rateHz * 2.0 * M_PI * invSampleRate;
            ch.lfoPhase = flushDenorm(std::fmod(ch.lfoPhase + phaseDelta, 2.0 * M_PI));
            
            // Generate LFO with stereo offset
            float lfoValue = generateLFO(ch.lfoPhase + channelIndex * spread * M_PI);
            
            // Add subtle thermal drift with denormal prevention
            float noise = ch.rng.nextFloat();
            noise = flushDenorm(noise * 0.00001f);
            ch.thermalDrift = flushDenorm(ch.thermalDrift + noise);
            ch.thermalDrift = std::max(-0.01f, std::min(0.01f, ch.thermalDrift));
            lfoValue = flushDenorm(lfoValue + ch.thermalDrift);
            
            // Calculate modulated frequency using fast exp2
            float centerFreqHz = 200.0f + center * 1800.0f; // 200Hz to 2kHz
            float modDepth = depthAmt * 0.9f;
            float exponent = lfoValue * modDepth;
            float modFactor = fastExp2(exponent);
            float modulatedFreq = flushDenorm(centerFreqHz * modFactor);
            
            // Apply feedback with soft clipping and denormal prevention
            float resonanceBoost = 1.0f + res * 2.0f;
            float fbSample = flushDenorm(ch.feedbackSample * fbAmt * resonanceBoost);
            input += flushDenorm(std::tanh(fbSample));
            
            // Get active stages
            int activeStages = getActiveStages(stageParam);
            
            // Process through all-pass stages
            float output = input;
            for (int stage = 0; stage < activeStages; ++stage) {
                // Calculate stage frequency with slight detuning
                float stageDetune = 1.0f + 0.08f * stage; // Avoid pow()
                float stageFreq = flushDenorm(modulatedFreq * stageDetune);
                
                // Lookup coefficient from table
                float coeff = lookupCoefficient(stageFreq);
                ch.allpassFilters[stage].setCoefficient(coeff);
                
                // Process with denormal prevention
                output = ch.allpassFilters[stage].process(output);
            }
            
            // Store feedback with denormal prevention
            ch.feedbackSample = flushDenorm(output);
            
            // Soft saturation
            output = flushDenorm(std::tanh(output * 0.7f) * 1.4f);
            
            // DC block output
            output = ch.outputDC.process(output);
            
            // Mix with dry - flush final result
            data[i] = flushDenorm(dry * (1.0f - mixAmt) + output * mixAmt);
        }
        
        // Periodic denormal flush and smoothing update
        if (++denormalFlushCounter >= 512) {
            denormalFlushCounter = 0;
            ch.feedbackSample = flushDenorm(ch.feedbackSample);
            ch.thermalDrift = flushDenorm(ch.thermalDrift);
            
            // Update all-pass smoothing periodically
            updateAllPassSmoothing();
            
            for (auto& filter : ch.allpassFilters) {
                // Force state flush through process
                filter.process(0.0f);
            }
        }
    }
};

// Public interface
AnalogPhaser::AnalogPhaser() : pimpl(std::make_unique<Impl>()) {}
AnalogPhaser::~AnalogPhaser() = default;

void AnalogPhaser::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepareToPlay(sampleRate, samplesPerBlock);
}

void AnalogPhaser::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
    pimpl->channels[1].lfoPhase = M_PI; // Restore stereo offset
}

void AnalogPhaser::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = std::min(buffer.getNumChannels(), Impl::MAX_CHANNELS);
    const int numSamples = buffer.getNumSamples();
    
    // Start performance measurement
    pimpl->metrics.startBlock();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        pimpl->processChannel(pimpl->channels[ch], buffer.getWritePointer(ch), 
                            numSamples, ch);
        
        // Update quality metrics
        pimpl->metrics.updatePeakRMS(buffer.getReadPointer(ch), numSamples);
        
        #ifdef DEBUG
        // Check for denormals in debug builds
        pimpl->metrics.checkDenormals(buffer.getReadPointer(ch), numSamples);
        #endif
    }
    
    // End performance measurement
    pimpl->metrics.endBlock(numSamples, numChannels);
}

void AnalogPhaser::updateParameters(const std::map<int, float>& params) {
    // Thread-safe parameter updates - this runs on the message thread
    // First, update the parameter buffer (lock-free write)
    for (const auto& [index, value] : params) {
        if (index >= 0 && index < 8) {
            pimpl->paramBuffer[index] = value;
        }
    }
    
    // Then update atomic parameters from the buffer
    // This ensures the RT thread never touches the std::map
    if (params.count(kRate))         pimpl->rate.setTarget(pimpl->paramBuffer[kRate]);
    if (params.count(kDepth))        pimpl->depth.setTarget(pimpl->paramBuffer[kDepth]);
    if (params.count(kFeedback))     pimpl->feedback.setTarget(pimpl->paramBuffer[kFeedback]);
    if (params.count(kStages))       pimpl->stages.setTarget(pimpl->paramBuffer[kStages]);
    if (params.count(kStereoSpread)) pimpl->stereoSpread.setTarget(pimpl->paramBuffer[kStereoSpread]);
    if (params.count(kCenterFreq))   pimpl->centerFreq.setTarget(pimpl->paramBuffer[kCenterFreq]);
    if (params.count(kResonance))    pimpl->resonance.setTarget(pimpl->paramBuffer[kResonance]);
    if (params.count(kMix))          pimpl->mix.setTarget(pimpl->paramBuffer[kMix]);
}

juce::String AnalogPhaser::getParameterName(int index) const {
    switch (index) {
        case kRate:         return "Rate";
        case kDepth:        return "Depth";
        case kFeedback:     return "Feedback";
        case kStages:       return "Stages";
        case kStereoSpread: return "Spread";
        case kCenterFreq:   return "Center";
        case kResonance:    return "Resonance";
        case kMix:          return "Mix";
        default:            return "";
    }
}

float AnalogPhaser::getCPUUsage() const {
    return pimpl->metrics.getCPUUsage();
}

float AnalogPhaser::getDynamicRangeDB() const {
    return pimpl->metrics.getDynamicRangeDB();
}

std::string AnalogPhaser::getQualityReport() const {
    return pimpl->metrics.getReport();
}
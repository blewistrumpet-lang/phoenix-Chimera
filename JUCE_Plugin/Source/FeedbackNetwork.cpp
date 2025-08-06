#include "FeedbackNetwork.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <array>
#include <cstring>

// Platform-specific includes for denormal protection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
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
#if HAS_SSE2
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

// Thread-safe PRNG for modulation
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

// Modulated delay line with denormal protection
class ModulatedDelay {
public:
    static constexpr size_t MAX_DELAY_SAMPLES = 2 * 48000; // 2 seconds @ 48kHz
    
    void prepare(double sampleRate) {
        this->sampleRate = static_cast<float>(sampleRate);
        reset();
    }
    
    void reset() {
        std::memset(buffer.data(), 0, buffer.size() * sizeof(float));
        writePos = 0;
        lfoPhase = 0.0f;
    }
    
    ALWAYS_INLINE void write(float input) noexcept {
        buffer[writePos] = flushDenorm(input);
        writePos = (writePos + 1) & (buffer.size() - 1);
    }
    
    ALWAYS_INLINE float read(float delaySamples) noexcept {
        float readIdx = static_cast<float>(writePos) - delaySamples - modulation;
        
        // Wrap around
        while (readIdx < 0.0f) readIdx += buffer.size();
        while (readIdx >= buffer.size()) readIdx -= buffer.size();
        
        // Linear interpolation
        int idx0 = static_cast<int>(readIdx);
        int idx1 = (idx0 + 1) & (buffer.size() - 1);
        float frac = readIdx - static_cast<float>(idx0);
        
        return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
    }
    
    void updateModulation(float rate, float depth) noexcept {
        lfoPhase += rate / sampleRate;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        
        // Sine LFO
        modulation = std::sin(2.0f * M_PI * lfoPhase) * depth;
    }

private:
    std::array<float, MAX_DELAY_SAMPLES> buffer{};
    size_t writePos{0};
    float sampleRate{44100.0f};
    float lfoPhase{0.0f};
    float modulation{0.0f};
};

// Allpass diffuser for smooth reverb decay
class AllpassDiffuser {
public:
    void setDelay(float samples) noexcept {
        delaySamples = samples;
    }
    
    void setFeedback(float fb) noexcept {
        feedback = fb;
    }
    
    void reset() noexcept {
        std::memset(buffer.data(), 0, buffer.size() * sizeof(float));
        writePos = 0;
    }
    
    ALWAYS_INLINE float process(float input) noexcept {
        int readPos = (writePos - static_cast<int>(delaySamples)) & (buffer.size() - 1);
        float delayed = buffer[readPos];
        float output = -input + delayed;
        buffer[writePos] = flushDenorm(input + delayed * feedback);
        writePos = (writePos + 1) & (buffer.size() - 1);
        return output;
    }

private:
    static constexpr size_t BUFFER_SIZE = 4096;
    std::array<float, BUFFER_SIZE> buffer{};
    size_t writePos{0};
    float delaySamples{100.0f};
    float feedback{0.5f};
};

// Pitch shifter for shimmer effect
class ShimmerPitchShifter {
public:
    void prepare(double sampleRate) {
        reset();
    }
    
    void reset() {
        std::memset(buffer.data(), 0, buffer.size() * sizeof(float));
        writePos = 0;
        readPos1 = 0.0f;
        readPos2 = BUFFER_SIZE * 0.5f;
        crossfade = 0.0f;
    }
    
    ALWAYS_INLINE float process(float input, float pitchRatio) noexcept {
        // Write to circular buffer
        buffer[writePos] = input;
        writePos = (writePos + 1) & (BUFFER_SIZE - 1);
        
        // Read with pitch shift
        float out1 = readInterpolated(readPos1);
        float out2 = readInterpolated(readPos2);
        
        // Crossfade between two read heads
        float output = out1 * (1.0f - crossfade) + out2 * crossfade;
        
        // Update read positions
        readPos1 += pitchRatio;
        readPos2 += pitchRatio;
        
        // Wrap and crossfade
        if (readPos1 >= BUFFER_SIZE) {
            readPos1 -= BUFFER_SIZE;
            crossfade = 1.0f;
        }
        if (readPos2 >= BUFFER_SIZE) {
            readPos2 -= BUFFER_SIZE;
            crossfade = 0.0f;
        }
        
        // Smooth crossfade
        float target = (readPos1 < readPos2) ? 0.0f : 1.0f;
        crossfade += (target - crossfade) * 0.01f;
        
        return output;
    }

private:
    static constexpr size_t BUFFER_SIZE = 4096;
    std::array<float, BUFFER_SIZE> buffer{};
    size_t writePos{0};
    float readPos1{0.0f};
    float readPos2{0.0f};
    float crossfade{0.0f};
    
    ALWAYS_INLINE float readInterpolated(float pos) noexcept {
        int idx0 = static_cast<int>(pos);
        int idx1 = (idx0 + 1) & (BUFFER_SIZE - 1);
        float frac = pos - static_cast<float>(idx0);
        return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
    }
};

// DC Blocker
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
struct FeedbackNetwork::Impl {
    static constexpr int NUM_DELAYS = 4;
    static constexpr int NUM_DIFFUSERS = 2;
    
    // Hadamard matrix for decorrelation
    static constexpr float HADAMARD[4][4] = {
        { 0.5f,  0.5f,  0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f,  0.5f}
    };
    
    // Parameters
    SmoothParam delayTime;
    SmoothParam feedback;
    SmoothParam crossFeed;
    SmoothParam diffusion;
    SmoothParam modulation;
    SmoothParam freeze;
    SmoothParam shimmer;
    SmoothParam mix;
    
    // DSP components per channel
    struct ChannelProcessor {
        std::array<ModulatedDelay, NUM_DELAYS> delays;
        std::array<std::array<AllpassDiffuser, NUM_DIFFUSERS>, NUM_DELAYS> diffusers;
        std::array<ShimmerPitchShifter, NUM_DELAYS> shimmers;
        std::array<float, NUM_DELAYS> lowpassStates{};
        std::array<float, NUM_DELAYS> highpassStates{};
        DCBlocker dcBlocker;
        RealtimePRNG rng;
        
        void prepare(double sampleRate) {
            for (int i = 0; i < NUM_DELAYS; ++i) {
                delays[i].prepare(sampleRate);
                shimmers[i].prepare(sampleRate);
                
                // Setup diffusers with different delay times
                for (int j = 0; j < NUM_DIFFUSERS; ++j) {
                    float delaySamples = 100.0f + i * 50.0f + j * 30.0f;
                    diffusers[i][j].setDelay(delaySamples);
                    diffusers[i][j].setFeedback(0.5f);
                }
            }
            
            dcBlocker.setSampleRate(sampleRate);
            reset();
        }
        
        void reset() {
            for (int i = 0; i < NUM_DELAYS; ++i) {
                delays[i].reset();
                shimmers[i].reset();
                lowpassStates[i] = 0.0f;
                highpassStates[i] = 0.0f;
                
                for (int j = 0; j < NUM_DIFFUSERS; ++j) {
                    diffusers[i][j].reset();
                }
            }
            dcBlocker.reset();
        }
    };
    
    std::array<ChannelProcessor, 2> channels;
    double sampleRate{44100.0};
    int totalLatency{0};
    
    // Block processing cache
    struct BlockCache {
        float delayTimeVal;
        float feedbackVal;
        float crossFeedVal;
        float diffusionVal;
        float modulationVal;
        float freezeVal;
        float shimmerVal;
        float mixVal;
        
        // Derived values
        std::array<float, NUM_DELAYS> delayTimes;
        std::array<float, NUM_DELAYS> modRates;
        std::array<float, NUM_DELAYS> modDepths;
        float inputGain;
        bool isFrozen;
    } cache;
    
    void prepare(double fs, int blockSize) {
        sampleRate = fs;
        totalLatency = static_cast<int>(fs * 0.001); // 1ms base latency
        
        // Initialize parameters with appropriate smoothing times
        delayTime.setSmoothingTime(20.0f, fs);
        feedback.setSmoothingTime(10.0f, fs);
        crossFeed.setSmoothingTime(10.0f, fs);
        diffusion.setSmoothingTime(10.0f, fs);
        modulation.setSmoothingTime(10.0f, fs);
        freeze.setSmoothingTime(5.0f, fs);
        shimmer.setSmoothingTime(10.0f, fs);
        mix.setSmoothingTime(5.0f, fs);
        
        // Set default values
        delayTime.setImmediate(0.5f);
        feedback.setImmediate(0.6f);
        crossFeed.setImmediate(0.3f);
        diffusion.setImmediate(0.5f);
        modulation.setImmediate(0.2f);
        freeze.setImmediate(0.0f);
        shimmer.setImmediate(0.0f);
        mix.setImmediate(0.5f);
        
        // Prepare channels
        for (size_t i = 0; i < channels.size(); ++i) {
            channels[i].prepare(fs);
            channels[i].rng = RealtimePRNG(static_cast<uint32_t>(i + 1));
        }
    }
    
    void updateBlockCache() {
        // Update all parameters once per block
        delayTime.updateBlock();
        feedback.updateBlock();
        crossFeed.updateBlock();
        diffusion.updateBlock();
        modulation.updateBlock();
        freeze.updateBlock();
        shimmer.updateBlock();
        mix.updateBlock();
        
        // Cache values
        cache.delayTimeVal = delayTime.getBlockValue();
        cache.feedbackVal = feedback.getBlockValue();
        cache.crossFeedVal = crossFeed.getBlockValue();
        cache.diffusionVal = diffusion.getBlockValue();
        cache.modulationVal = modulation.getBlockValue();
        cache.freezeVal = freeze.getBlockValue();
        cache.shimmerVal = shimmer.getBlockValue();
        cache.mixVal = mix.getBlockValue();
        
        // Calculate delay times based on golden ratio
        const float baseDelays[NUM_DELAYS] = {0.11f, 0.17f, 0.29f, 0.47f};
        for (int i = 0; i < NUM_DELAYS; ++i) {
            cache.delayTimes[i] = (baseDelays[i] + cache.delayTimeVal * baseDelays[i] * 3.0f) * sampleRate;
            cache.modRates[i] = 0.1f + i * 0.13f; // 0.1Hz to 0.49Hz
            cache.modDepths[i] = 5.0f + cache.modulationVal * 20.0f; // 5-25 samples
        }
        
        cache.inputGain = 1.0f / std::sqrt(static_cast<float>(NUM_DELAYS));
        cache.isFrozen = cache.freezeVal > 0.5f;
    }
    
    ALWAYS_INLINE float softClip(float input) noexcept {
        if (std::abs(input) < 0.7f) return input;
        float sign = input > 0.0f ? 1.0f : -1.0f;
        return sign * (0.7f + 0.3f * std::tanh((std::abs(input) - 0.7f) * 3.0f));
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer) {
        const int numChannels = std::min(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();
        
        // Update cache once per block
        updateBlockCache();
        
        // Early bypass if dry
        if (cache.mixVal < 0.001f) return;
        
        for (int ch = 0; ch < numChannels; ++ch) {
            float* data = buffer.getWritePointer(ch);
            auto& processor = channels[ch];
            
            for (int sample = 0; sample < numSamples; ++sample) {
                float input = data[sample];
                float drySignal = input;
                
                // DC block input
                input = processor.dcBlocker.process(input);
                
                // Read from all delays
                std::array<float, NUM_DELAYS> delayOutputs;
                for (int i = 0; i < NUM_DELAYS; ++i) {
                    processor.delays[i].updateModulation(cache.modRates[i], cache.modDepths[i]);
                    delayOutputs[i] = processor.delays[i].read(cache.delayTimes[i]);
                }
                
                // Apply Hadamard matrix for decorrelation
                std::array<float, NUM_DELAYS> mixed{};
                for (int i = 0; i < NUM_DELAYS; ++i) {
                    for (int j = 0; j < NUM_DELAYS; ++j) {
                        mixed[i] += delayOutputs[j] * HADAMARD[i][j];
                    }
                }
                
                // Process each delay line
                for (int i = 0; i < NUM_DELAYS; ++i) {
                    float signal = mixed[i];
                    
                    // Add input to first two delays for stereo spread
                    if (i < 2) {
                        signal += input * cache.inputGain * (ch == i ? 1.0f : 0.5f);
                    }
                    
                    // Apply diffusion
                    if (cache.diffusionVal > 0.0f) {
                        float diffused = signal;
                        for (int j = 0; j < NUM_DIFFUSERS; ++j) {
                            diffused = processor.diffusers[i][j].process(diffused);
                        }
                        signal = signal * (1.0f - cache.diffusionVal) + diffused * cache.diffusionVal;
                    }
                    
                    // Apply shimmer (octave up)
                    if (cache.shimmerVal > 0.0f) {
                        float shimmerSignal = processor.shimmers[i].process(signal, 2.0f);
                        
                        // High-pass filter the shimmer
                        float highpassed = shimmerSignal - processor.highpassStates[i];
                        processor.highpassStates[i] += highpassed * 0.99f;
                        processor.highpassStates[i] = flushDenorm(processor.highpassStates[i]);
                        
                        signal = signal * (1.0f - cache.shimmerVal * 0.5f) + 
                                highpassed * cache.shimmerVal * 0.5f;
                    }
                    
                    // Apply feedback and freeze
                    float feedbackAmount = cache.feedbackVal;
                    if (cache.isFrozen) {
                        feedbackAmount = 0.99f;
                        signal *= 0.1f; // Reduce input when frozen
                    }
                    
                    // Damping (gentle lowpass)
                    float dampingCutoff = 0.3f + (1.0f - feedbackAmount) * 0.5f;
                    processor.lowpassStates[i] += (signal - processor.lowpassStates[i]) * dampingCutoff;
                    processor.lowpassStates[i] = flushDenorm(processor.lowpassStates[i]);
                    signal = processor.lowpassStates[i];
                    
                    // Apply feedback
                    signal = softClip(signal * feedbackAmount);
                    
                    // Cross-feed to other delays
                    if (cache.crossFeedVal > 0.0f) {
                        for (int j = 0; j < NUM_DELAYS; ++j) {
                            if (i != j) {
                                float crossSignal = signal * cache.crossFeedVal * 0.5f * HADAMARD[i][j];
                                processor.delays[j].write(processor.delays[j].read(0) + crossSignal);
                            }
                        }
                    }
                    
                    // Write back to delay
                    processor.delays[i].write(signal);
                }
                
                // Sum all delay outputs
                float output = 0.0f;
                for (int i = 0; i < NUM_DELAYS; ++i) {
                    output += delayOutputs[i] * 0.5f;
                }
                
                // Final soft clipping
                output = softClip(output);
                
                // Mix with dry signal
                data[sample] = drySignal * (1.0f - cache.mixVal) + output * cache.mixVal;
            }
        }
    }
};

// Constructor/Destructor
FeedbackNetwork::FeedbackNetwork() : pimpl(std::make_unique<Impl>()) {}
FeedbackNetwork::~FeedbackNetwork() = default;

// Core methods
void FeedbackNetwork::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
}

void FeedbackNetwork::process(juce::AudioBuffer<float>& buffer) {
    pimpl->processBlock(buffer);
}

void FeedbackNetwork::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
}

void FeedbackNetwork::updateParameters(const std::map<int, float>& params) {
    auto it = params.find(kDelayTime);
    if (it != params.end()) pimpl->delayTime.setTarget(it->second);
    
    it = params.find(kFeedback);
    if (it != params.end()) pimpl->feedback.setTarget(it->second * 0.98f); // Max 98% to prevent runaway
    
    it = params.find(kCrossFeed);
    if (it != params.end()) pimpl->crossFeed.setTarget(it->second);
    
    it = params.find(kDiffusion);
    if (it != params.end()) pimpl->diffusion.setTarget(it->second);
    
    it = params.find(kModulation);
    if (it != params.end()) pimpl->modulation.setTarget(it->second);
    
    it = params.find(kFreeze);
    if (it != params.end()) pimpl->freeze.setTarget(it->second);
    
    it = params.find(kShimmer);
    if (it != params.end()) pimpl->shimmer.setTarget(it->second);
    
    it = params.find(kMix);
    if (it != params.end()) pimpl->mix.setTarget(it->second);
}

juce::String FeedbackNetwork::getParameterName(int index) const {
    switch (index) {
        case kDelayTime: return "Delay Time";
        case kFeedback: return "Feedback";
        case kCrossFeed: return "Cross Feed";
        case kDiffusion: return "Diffusion";
        case kModulation: return "Modulation";
        case kFreeze: return "Freeze";
        case kShimmer: return "Shimmer";
        case kMix: return "Mix";
        default: return "";
    }
}

int FeedbackNetwork::getLatencySamples() const {
    return pimpl->totalLatency;
}
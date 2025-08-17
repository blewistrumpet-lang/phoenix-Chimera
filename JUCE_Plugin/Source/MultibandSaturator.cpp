#include "MultibandSaturator.h"
#include <cmath>
#include <algorithm>
#include <vector>

// Platform-specific SSE for denormal handling
#if defined(__SSE2__)
    #include <immintrin.h>
#endif

namespace {
    // Constants
    constexpr float kTinyF = 1e-30f;
    constexpr float kMaxDrive = 10.0f;
    constexpr float kLowFreq = 200.0f;
    constexpr float kHighFreq = 2000.0f;
    
    // Global denormal protection
    struct DenormalGuard {
        DenormalGuard() {
#if defined(__SSE2__)
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
        }
    } static g_denormGuard;
    
    // Safe utilities
    inline float flushDenorm(float x) noexcept {
        return (std::fabs(x) < kTinyF) ? 0.0f : x;
    }
    
    inline float clamp01(float x) noexcept {
        return std::max(0.0f, std::min(1.0f, x));
    }
    
    inline float clamp(float x, float lo, float hi) noexcept {
        return std::max(lo, std::min(hi, x));
    }
    
    // Safe saturation functions
    inline float softClip(float x, float drive) noexcept {
        x *= drive;
        return std::tanh(x) / drive;
    }
    
    inline float tubeSat(float x, float drive) noexcept {
        x *= drive;
        const float ax = std::fabs(x);
        if (ax < 1.0f) {
            return x * (1.0f - 0.3333f * ax * ax);
        }
        return x / (ax + 1.0f);
    }
    
    inline float tapeSat(float x, float drive) noexcept {
        x *= drive;
        const float x2 = x * x;
        return x / (1.0f + 0.5f * x2);
    }
    
    // Simple Butterworth filter
    class ButterworthFilter {
    public:
        void setLowpass(float freq, float sampleRate) {
            const float w = 2.0f * std::tan(M_PI * freq / sampleRate);
            const float k = 1.0f / (1.0f + w + w * w);
            
            a0 = k * w * w;
            a1 = 2.0f * a0;
            a2 = a0;
            b1 = 2.0f * k * (w * w - 1.0f);
            b2 = k * (1.0f - w + w * w);
        }
        
        void setHighpass(float freq, float sampleRate) {
            const float w = 2.0f * std::tan(M_PI * freq / sampleRate);
            const float k = 1.0f / (1.0f + w + w * w);
            
            a0 = k;
            a1 = -2.0f * k;
            a2 = k;
            b1 = 2.0f * k * (w * w - 1.0f);
            b2 = k * (1.0f - w + w * w);
        }
        
        float process(float input) noexcept {
            const float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
            
            x2 = x1;
            x1 = input;
            y2 = y1;
            y1 = flushDenorm(output);
            
            return output;
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0f;
        }
        
    private:
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
        float b1 = 0.0f, b2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
    };
    
    // Simple 3-band crossover
    class Crossover {
    public:
        struct Bands {
            float low = 0.0f;
            float mid = 0.0f;
            float high = 0.0f;
        };
        
        void prepare(float sampleRate) {
            // Set up filters
            lowLP.setLowpass(kLowFreq, sampleRate);
            lowHP.setHighpass(kLowFreq, sampleRate);
            highLP.setLowpass(kHighFreq, sampleRate);
            highHP.setHighpass(kHighFreq, sampleRate);
            reset();
        }
        
        void reset() {
            lowLP.reset();
            lowHP.reset();
            highLP.reset();
            highHP.reset();
        }
        
        Bands process(float input) noexcept {
            Bands bands;
            
            // Low band: lowpass
            bands.low = lowLP.process(input);
            
            // High frequency content
            float highContent = lowHP.process(input);
            
            // Mid band: bandpass
            bands.mid = highLP.process(highContent);
            
            // High band: highpass
            bands.high = highHP.process(highContent);
            
            return bands;
        }
        
    private:
        ButterworthFilter lowLP, lowHP;
        ButterworthFilter highLP, highHP;
    };
    
    // Parameter smoother
    class ParamSmoother {
    public:
        void setTimeMs(float ms, float sampleRate) {
            const float tau = ms * 0.001f;
            coeff = std::exp(-1.0f / (tau * sampleRate));
        }
        
        void setTarget(float target) {
            this->target = target;
        }
        
        void snap(float value) {
            current = target = value;
        }
        
        float tick() noexcept {
            current += (1.0f - coeff) * (target - current);
            return flushDenorm(current);
        }
        
    private:
        float current = 0.0f;
        float target = 0.0f;
        float coeff = 0.99f;
    };
}

// ===================== PIMPL Implementation =====================
struct MultibandSaturator::Impl {
    // Core
    float sampleRate = 0.0f;
    int blockSize = 512;
    
    // Per-channel processing
    static constexpr int kMaxChannels = 8;
    std::array<Crossover, kMaxChannels> crossovers;
    
    // Parameters
    ParamSmoother lowDrive;
    ParamSmoother midDrive;
    ParamSmoother highDrive;
    ParamSmoother satType;
    ParamSmoother harmonics;
    ParamSmoother outputGain;
    ParamSmoother mix;
    
    void prepare(double sr, int bs) {
        sampleRate = static_cast<float>(std::max(8000.0, sr));
        blockSize = std::max(1, bs);
        
        // Setup crossovers
        for (auto& xover : crossovers) {
            xover.prepare(sampleRate);
        }
        
        // Setup smoothers
        const float smoothMs = 5.0f;
        lowDrive.setTimeMs(smoothMs, sampleRate);
        midDrive.setTimeMs(smoothMs, sampleRate);
        highDrive.setTimeMs(smoothMs, sampleRate);
        satType.setTimeMs(smoothMs, sampleRate);
        harmonics.setTimeMs(smoothMs, sampleRate);
        outputGain.setTimeMs(smoothMs, sampleRate);
        mix.setTimeMs(smoothMs, sampleRate);
        
        reset();
    }
    
    void reset() {
        for (auto& xover : crossovers) {
            xover.reset();
        }
        
        // Default values
        lowDrive.snap(1.0f);
        midDrive.snap(1.0f);
        highDrive.snap(1.0f);
        satType.snap(0.0f);
        harmonics.snap(0.5f);
        outputGain.snap(1.0f);
        mix.snap(1.0f);
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer) {
        const int numChannels = std::min(buffer.getNumChannels(), kMaxChannels);
        const int numSamples = buffer.getNumSamples();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        // Process each sample
        for (int i = 0; i < numSamples; ++i) {
            // Update parameters
            const float lowDr = lowDrive.tick();
            const float midDr = midDrive.tick();
            const float highDr = highDrive.tick();
            const float type = satType.tick();
            const float harm = harmonics.tick();
            const float gain = outputGain.tick();
            const float mixAmt = mix.tick();
            
            // Determine saturation type (0-1 mapped to 4 types)
            const int satTypeIdx = clamp(static_cast<int>(type * 3.99f), 0, 3);
            
            for (int ch = 0; ch < numChannels; ++ch) {
                float* channelData = buffer.getWritePointer(ch);
                const float dry = channelData[i];
                
                // Split into bands
                auto bands = crossovers[ch].process(dry);
                
                // Saturate each band
                float lowOut = 0.0f, midOut = 0.0f, highOut = 0.0f;
                
                switch (satTypeIdx) {
                    case 0: // Tube
                        lowOut = tubeSat(bands.low, 1.0f + lowDr * kMaxDrive);
                        midOut = tubeSat(bands.mid, 1.0f + midDr * kMaxDrive);
                        highOut = tubeSat(bands.high, 1.0f + highDr * kMaxDrive);
                        break;
                        
                    case 1: // Tape
                        lowOut = tapeSat(bands.low, 1.0f + lowDr * kMaxDrive);
                        midOut = tapeSat(bands.mid, 1.0f + midDr * kMaxDrive);
                        highOut = tapeSat(bands.high, 1.0f + highDr * kMaxDrive);
                        break;
                        
                    case 2: // Transistor
                        lowOut = softClip(bands.low, 1.0f + lowDr * kMaxDrive * 1.5f);
                        midOut = softClip(bands.mid, 1.0f + midDr * kMaxDrive * 1.5f);
                        highOut = softClip(bands.high, 1.0f + highDr * kMaxDrive * 1.5f);
                        break;
                        
                    case 3: // Diode
                        lowOut = softClip(bands.low, 1.0f + lowDr * kMaxDrive * 2.0f);
                        midOut = softClip(bands.mid, 1.0f + midDr * kMaxDrive * 2.0f);
                        highOut = softClip(bands.high, 1.0f + highDr * kMaxDrive * 2.0f);
                        break;
                }
                
                // Add harmonics (simple even/odd mix)
                if (harm > 0.01f) {
                    const float h2 = harm * 0.1f;
                    lowOut += h2 * lowOut * lowOut * ((lowOut > 0) ? 1.0f : -1.0f);
                    midOut += h2 * midOut * midOut * ((midOut > 0) ? 1.0f : -1.0f);
                    highOut += h2 * highOut * highOut * ((highOut > 0) ? 1.0f : -1.0f);
                }
                
                // Recombine bands
                float wet = (lowOut + midOut + highOut) * gain;
                
                // Safety limiting
                wet = clamp(wet, -2.0f, 2.0f);
                
                // Mix dry/wet
                channelData[i] = dry * (1.0f - mixAmt) + wet * mixAmt;
                
                // Final safety and denormal flush
                if (!std::isfinite(channelData[i])) {
                    channelData[i] = 0.0f;
                }
                channelData[i] = flushDenorm(channelData[i]);
            }
        }
    }
};

// ===================== Public Implementation =====================
MultibandSaturator::MultibandSaturator()
    : pImpl(std::make_unique<Impl>()) {}

MultibandSaturator::~MultibandSaturator() = default;

void MultibandSaturator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepare(sampleRate, samplesPerBlock);
}

void MultibandSaturator::reset() {
    pImpl->reset();
}

void MultibandSaturator::process(juce::AudioBuffer<float>& buffer) {
    pImpl->processBlock(buffer);
}

void MultibandSaturator::updateParameters(const std::map<int, float>& params) {
    auto get = [&](int id, float defaultVal) {
        auto it = params.find(id);
        return it != params.end() ? it->second : defaultVal;
    };
    
    // All parameters normalized 0-1
    pImpl->lowDrive.setTarget(clamp01(get(kLowDrive, 0.5f)));
    pImpl->midDrive.setTarget(clamp01(get(kMidDrive, 0.5f)));
    pImpl->highDrive.setTarget(clamp01(get(kHighDrive, 0.5f)));
    pImpl->satType.setTarget(clamp01(get(kSaturationType, 0.0f)));
    pImpl->harmonics.setTarget(clamp01(get(kHarmonicCharacter, 0.5f)));
    pImpl->outputGain.setTarget(0.1f + 1.9f * clamp01(get(kOutputGain, 0.5f))); // 0.1 to 2.0
    pImpl->mix.setTarget(clamp01(get(kMix, 1.0f)));
}

juce::String MultibandSaturator::getParameterName(int index) const {
    switch (index) {
        case kLowDrive:          return "Low Drive";
        case kMidDrive:          return "Mid Drive";
        case kHighDrive:         return "High Drive";
        case kSaturationType:    return "Saturation Type";
        case kHarmonicCharacter: return "Harmonic Character";
        case kOutputGain:        return "Output Gain";
        case kMix:               return "Mix";
        default:                 return "";
    }
}
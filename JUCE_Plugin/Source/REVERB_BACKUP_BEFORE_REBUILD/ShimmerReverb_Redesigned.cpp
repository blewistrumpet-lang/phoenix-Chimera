// ==================== ShimmerReverb - Complete Redesign ====================
// Professional shimmer reverb with proper pitch shifting and harmonic generation
#include "ShimmerReverb.h"
#include "signalsmith-stretch.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <array>

namespace {
struct FTZGuard {
    FTZGuard() {
       #if defined(__SSE__)
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
       #endif
    }
} s_ftzGuard;
}

// Professional pitch shifter using block processing
class BlockPitchShifter {
public:
    BlockPitchShifter() : stretcher(std::make_unique<signalsmith::stretch::SignalsmithStretch<float>>()) {}
    
    void prepare(double sampleRate, int maxBlockSize) {
        sr = sampleRate;
        blockSize = maxBlockSize;
        
        // Configure for quality shimmer processing
        stretcher->presetDefault(1, (float)sampleRate);
        stretcher->reset();
        
        // Allocate buffers
        inputBuffer.resize(blockSize);
        outputBuffer.resize(blockSize);
        accumulator.resize(blockSize * 4); // Overlap buffer
        accumulatorWritePos = 0;
        accumulatorReadPos = 0;
    }
    
    void setShiftRatio(float ratio) {
        if (std::abs(ratio - currentRatio) > 0.001f) {
            stretcher->setTransposeFactor(ratio);
            currentRatio = ratio;
        }
    }
    
    void processBlock(const float* input, float* output, int numSamples) {
        // Accumulate input samples
        for (int i = 0; i < numSamples; ++i) {
            accumulator[accumulatorWritePos] = input[i];
            accumulatorWritePos = (accumulatorWritePos + 1) % accumulator.size();
        }
        
        // Process in chunks for efficiency
        const int chunkSize = 64;
        int samplesProcessed = 0;
        
        while (samplesProcessed < numSamples) {
            int toProcess = std::min(chunkSize, numSamples - samplesProcessed);
            
            // Prepare input pointer
            const float* inputPtr = &input[samplesProcessed];
            float* outputPtr = &output[samplesProcessed];
            
            // Process through stretcher
            stretcher->process(&inputPtr, toProcess, &outputPtr, toProcess);
            
            samplesProcessed += toProcess;
        }
    }
    
    void reset() {
        stretcher->reset();
        std::fill(accumulator.begin(), accumulator.end(), 0.0f);
        accumulatorWritePos = 0;
        accumulatorReadPos = 0;
    }
    
private:
    std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretcher;
    double sr = 48000.0;
    int blockSize = 512;
    float currentRatio = 1.0f;
    
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::vector<float> accumulator;
    int accumulatorWritePos = 0;
    int accumulatorReadPos = 0;
};

// Multi-voice shimmer processor with proper harmonic generation
class MultiVoiceShimmer {
public:
    static constexpr int NUM_VOICES = 3;
    
    MultiVoiceShimmer() {
        for (auto& shifter : pitchShifters) {
            shifter = std::make_unique<BlockPitchShifter>();
        }
    }
    
    void prepare(double sampleRate, int maxBlockSize) {
        sr = sampleRate;
        blockSize = maxBlockSize;
        
        // Prepare all pitch shifters
        for (auto& shifter : pitchShifters) {
            shifter->prepare(sampleRate, maxBlockSize);
        }
        
        // Allocate working buffers
        workBuffer.resize(blockSize);
        voiceBuffers.fill(std::vector<float>(blockSize));
        
        // Initialize filters for each voice
        for (int v = 0; v < NUM_VOICES; ++v) {
            // High-pass filters to remove DC and low frequency artifacts
            highpassFilters[v].setCoefficients(juce::IIRCoefficients::makeHighPass(sampleRate, 100.0));
            
            // Low-pass filters for anti-aliasing
            lowpassFilters[v].setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, 
                std::min(20000.0, sampleRate * 0.45)));
        }
    }
    
    void processBlock(const float* reverbSignal, float* outputL, float* outputR, 
                     int numSamples, float shimmerAmount, float pitchParam, float widthParam) {
        
        if (shimmerAmount < 0.01f) {
            // No shimmer - bypass processing
            return;
        }
        
        // Configure pitch ratios based on parameters
        configurePitchRatios(pitchParam);
        
        // Process each voice
        for (int voice = 0; voice < NUM_VOICES; ++voice) {
            // Check if this voice should be active based on shimmer amount
            float voiceLevel = getVoiceLevel(voice, shimmerAmount);
            if (voiceLevel < 0.01f) continue;
            
            // Copy input to voice buffer
            std::copy(reverbSignal, reverbSignal + numSamples, voiceBuffers[voice].data());
            
            // Apply high-pass filter to remove DC
            highpassFilters[voice].processSamples(voiceBuffers[voice].data(), numSamples);
            
            // Pitch shift this voice
            pitchShifters[voice]->processBlock(voiceBuffers[voice].data(), 
                                              voiceBuffers[voice].data(), 
                                              numSamples);
            
            // Apply low-pass filter for anti-aliasing
            lowpassFilters[voice].processSamples(voiceBuffers[voice].data(), numSamples);
            
            // Mix into output with appropriate gain and stereo positioning
            mixVoiceToOutput(voice, outputL, outputR, numSamples, voiceLevel, widthParam);
        }
    }
    
    void reset() {
        for (auto& shifter : pitchShifters) {
            shifter->reset();
        }
        for (auto& filter : highpassFilters) {
            filter.reset();
        }
        for (auto& filter : lowpassFilters) {
            filter.reset();
        }
    }
    
private:
    void configurePitchRatios(float pitchParam) {
        // Voice 1: Main octave (0-12 semitones based on pitch parameter)
        float baseSemitones = pitchParam * 12.0f;
        pitchShifters[0]->setShiftRatio(std::pow(2.0f, (baseSemitones + 12.0f) / 12.0f)); // +12 semitones base
        
        // Voice 2: Perfect fifth above voice 1
        if (pitchShifters.size() > 1) {
            pitchShifters[1]->setShiftRatio(std::pow(2.0f, (baseSemitones + 19.0f) / 12.0f)); // +19 semitones
        }
        
        // Voice 3: Second octave
        if (pitchShifters.size() > 2) {
            pitchShifters[2]->setShiftRatio(std::pow(2.0f, (baseSemitones + 24.0f) / 12.0f)); // +24 semitones
        }
    }
    
    float getVoiceLevel(int voice, float shimmerAmount) {
        switch (voice) {
            case 0: // Main octave - always active when shimmer > 0
                return shimmerAmount;
                
            case 1: // Fifth - fades in from 50-100%
                return shimmerAmount > 0.5f ? (shimmerAmount - 0.5f) * 2.0f : 0.0f;
                
            case 2: // Second octave - fades in from 75-100%
                return shimmerAmount > 0.75f ? (shimmerAmount - 0.75f) * 4.0f : 0.0f;
                
            default:
                return 0.0f;
        }
    }
    
    void mixVoiceToOutput(int voice, float* outputL, float* outputR, 
                         int numSamples, float voiceLevel, float widthParam) {
        // Voice gains and stereo positioning
        const float voiceGains[NUM_VOICES] = {0.7f, 0.4f, 0.3f};
        const float stereoPan[NUM_VOICES] = {0.0f, -0.3f, 0.3f}; // Center, left, right
        
        float gain = voiceGains[voice] * voiceLevel * voiceLevel; // Squared for smoother curve
        float panL = 1.0f - std::max(0.0f, stereoPan[voice] * widthParam);
        float panR = 1.0f + std::min(0.0f, stereoPan[voice] * widthParam);
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = voiceBuffers[voice][i] * gain;
            outputL[i] += sample * panL;
            outputR[i] += sample * panR;
        }
    }
    
    double sr = 48000.0;
    int blockSize = 512;
    
    std::array<std::unique_ptr<BlockPitchShifter>, NUM_VOICES> pitchShifters;
    std::array<juce::IIRFilter, NUM_VOICES> highpassFilters;
    std::array<juce::IIRFilter, NUM_VOICES> lowpassFilters;
    
    std::vector<float> workBuffer;
    std::array<std::vector<float>, NUM_VOICES> voiceBuffers;
};

// Global shimmer processor instance
static MultiVoiceShimmer g_shimmerProcessor;

// -------------------------------------------------------
ShimmerReverb::ShimmerReverb() {
    // musical defaults
    pSize.snap(0.5f);
    pShimmer.snap(0.0f);
    pPitch.snap(1.0f);
    pDamp.snap(0.5f);
    pDiff.snap(0.6f);
    pMod.snap(0.4f);
    pPredelay.snap(0.0f);
    pWidth.snap(0.8f);
    pFreeze.snap(0.0f);
    pMix.snap(0.3f);
}

// -------------------------------------------------------
void ShimmerReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sr_ = sampleRate;
    const auto sr = sr_;

    // Set up parameter smoothing
    pSize.setTimeMs(60.0f, sr);
    pShimmer.setTimeMs(80.0f, sr);
    pPitch.setTimeMs(80.0f, sr);
    pDamp.setTimeMs(30.0f, sr);
    pDiff.setTimeMs(30.0f, sr);
    pMod.setTimeMs(30.0f, sr);
    pPredelay.setTimeMs(10.0f, sr);
    pWidth.setTimeMs(40.0f, sr);
    pFreeze.setTimeMs(10.0f, sr);
    pMix.setTimeMs(15.0f, sr);

    // Setup FDN delay lines
    L_[0].delay.prepare(static_cast<int>(0.030 * sr));
    L_[1].delay.prepare(static_cast<int>(0.034 * sr));
    L_[2].delay.prepare(static_cast<int>(0.039 * sr));
    L_[3].delay.prepare(static_cast<int>(0.041 * sr));
    
    // Initialize allpass modulators
    for (auto& l : L_) {
        l.ap1.prepare(128, sr);
        l.ap2.prepare(128, sr);
    }

    // Pre-delay
    preDelay_.prepare(static_cast<int>(0.250 * sr));
    
    // Initialize the redesigned shimmer processor
    g_shimmerProcessor.prepare(sr, samplesPerBlock);
    
    // Keep old shimmer for fallback
    shimmer_.prepare(static_cast<int>(0.250 * sr), sr);
    
    // Allocate shimmer processing buffer
    shimmerBuffer.setSize(1, samplesPerBlock);

    reset();
}

// -------------------------------------------------------
void ShimmerReverb::reset() {
    for (auto& l : L_) {
        l.delay.reset();
        l.ap1.reset();
        l.ap2.reset();
        l.damp.reset();
        l.state = 0.0f;
    }
    preDelay_.reset();
    shimmer_.reset();
    g_shimmerProcessor.reset();
    shimmerBuffer.clear();
}

// -------------------------------------------------------
void ShimmerReverb::updateParameters(const std::map<int, float>& params) {
    auto get = [&](int idx, float def){ 
        auto it = params.find(idx); 
        return it != params.end() ? it->second : def; 
    };

    // Special handling for mix parameter
    auto mixIt = params.find((int)ParamID::Mix);
    if (mixIt != params.end()) {
        float mixValue = clamp01(mixIt->second);
        if (mixValue < 0.01f) {
            pMix.snap(0.0f);
        } else {
            pMix.target.store(mixValue, std::memory_order_relaxed);
        }
    }

    // Update other parameters
    pSize.target.store(clamp01(get((int)ParamID::Size, 0.5f)), std::memory_order_relaxed);
    pShimmer.target.store(clamp01(get((int)ParamID::Shimmer, 0.0f)), std::memory_order_relaxed);
    pPitch.target.store(clamp01(get((int)ParamID::Pitch, 1.0f)), std::memory_order_relaxed);
    pDamp.target.store(clamp01(get((int)ParamID::Damping, 0.5f)), std::memory_order_relaxed);
    pDiff.target.store(clamp01(get((int)ParamID::Diffusion, 0.6f)), std::memory_order_relaxed);
    pMod.target.store(clamp01(get((int)ParamID::Modulation, 0.4f)), std::memory_order_relaxed);
    pPredelay.target.store(clamp01(get((int)ParamID::Predelay, 0.0f)), std::memory_order_relaxed);
    pWidth.target.store(clamp01(get((int)ParamID::Width, 0.8f)), std::memory_order_relaxed);
    pFreeze.target.store(clamp01(get((int)ParamID::Freeze, 0.0f)), std::memory_order_relaxed);
}

// -------------------------------------------------------
void ShimmerReverb::process(juce::AudioBuffer<float>& buffer) {
    const int numCh = std::min(buffer.getNumChannels(), 2);
    const int N = buffer.getNumSamples();
    if (N <= 0) return;

    // Pull smoothed parameters
    const float size01   = pSize.tick();
    const float shAmt    = pShimmer.tick();
    const float pitch01  = pPitch.tick();
    const float damp01   = pDamp.tick();
    const float diff01   = pDiff.tick();
    const float mod01    = pMod.tick();
    const float preMs    = pPredelay.tick() * 250.0f;
    const float width01  = pWidth.tick();
    const float freeze01 = pFreeze.tick();
    const float mix01    = pMix.tick();

    // Calculate feedback
    const float baseFeedback = 0.4f + 0.15f * size01;
    const float freezeBoost = freeze01 * 0.3f;
    const float fbBoost = baseFeedback + freezeBoost;
    
    // Setup damping
    const float dampHz = 500.0f + 8000.0f * (1.0f - damp01);
    for (auto& l : L_) l.damp.setCutoff(dampHz, sr_);

    // Setup diffusion
    const float apG = -0.7f + 0.5f * diff01;
    const float apRate = 0.05f + 4.0f * mod01;
    const float apDepth = 2.0f + 20.0f * mod01;
    for (auto& l : L_) {
        l.ap1.set(apG, apRate * 0.7f, apDepth);
        l.ap2.set(-apG, apRate * 1.1f, apDepth * 0.7f);
    }

    // Pre-delay samples
    const int preSamp = (int) std::round(std::min(0.25f, preMs * 0.001f) * sr_);

    float* Lp = buffer.getWritePointer(0);
    float* Rp = (numCh > 1 ? buffer.getWritePointer(1) : nullptr);
    
    // Prepare shimmer buffer if needed
    if (shAmt > 0.01f) {
        shimmerBuffer.clear();
    }
    
    // Allocate output buffers for shimmer
    std::vector<float> shimmerOutL(N, 0.0f);
    std::vector<float> shimmerOutR(N, 0.0f);

    for (int n = 0; n < N; ++n) {
        const float inL = Lp[n];
        const float inR = (Rp ? Rp[n] : inL);
        
        // Store dry
        const float dryL = inL;
        const float dryR = inR;
        
        // Mono input for reverb
        float inMono = 0.5f * (inL + inR);

        // Handle freeze
        const float freezeAmount = clamp01(freeze01 * 2.0f);
        float reverbInput = inMono * (1.0f - freezeAmount);

        // Pre-delay
        preDelay_.write(reverbInput);
        const float x = preSamp > 0 ? preDelay_.read(preSamp) : reverbInput;

        // Size-scaled delays
        const float sizeScale = 0.3f + 1.4f * size01;
        const int delayA = std::max(64, (int)((float)L_[0].delay.buf.size() * sizeScale * 0.4f));
        const int delayB = std::max(64, (int)((float)L_[1].delay.buf.size() * sizeScale * 0.5f));
        const int delayC = std::max(64, (int)((float)L_[2].delay.buf.size() * sizeScale * 0.6f));
        const int delayD = std::max(64, (int)((float)L_[3].delay.buf.size() * sizeScale * 0.7f));

        // FDN processing
        float a = L_[0].ap1.process(x + 0.15f * L_[3].state);
        float b = L_[1].ap1.process(x + 0.15f * L_[0].state);
        float c = L_[2].ap1.process(x + 0.15f * L_[1].state);
        float d = L_[3].ap1.process(x + 0.15f * L_[2].state);

        float delayedA = L_[0].damp.process(L_[0].delay.read(delayA));
        float delayedB = L_[1].damp.process(L_[1].delay.read(delayB));
        float delayedC = L_[2].damp.process(L_[2].delay.read(delayC));
        float delayedD = L_[3].damp.process(L_[3].delay.read(delayD));
        
        a = delayedA * fbBoost + a * 0.05f;
        b = delayedB * fbBoost + b * 0.05f;
        c = delayedC * fbBoost + c * 0.05f;
        d = delayedD * fbBoost + d * 0.05f;

        L_[0].delay.write(L_[0].ap2.process(a));
        L_[1].delay.write(L_[1].ap2.process(b));
        L_[2].delay.write(L_[2].ap2.process(c));
        L_[3].delay.write(L_[3].ap2.process(d));

        L_[0].state = a; L_[1].state = b; L_[2].state = c; L_[3].state = d;

        // Create reverb output
        float outL = 0.5f * a - 0.35f * b + 0.25f * c + 0.1f * d;
        float outR = -0.35f * a + 0.5f * b + 0.1f * c + 0.25f * d;
        
        // Conservative reverb level
        const float reverbLevel = 1.2f;
        outL *= reverbLevel;
        outR *= reverbLevel;
        
        // Collect shimmer input signal
        if (shAmt > 0.01f) {
            shimmerBuffer.setSample(0, n, 0.25f * (a + b + c + d));
        }

        // Width processing on reverb
        const float widthScaled = width01 * width01;
        stereoWidth(outL, outR, widthScaled);

        // Mix dry/wet WITHOUT shimmer first
        float yL, yR;
        if (mix01 < 0.001f) {
            yL = dryL;
            yR = dryR;
        } else {
            yL = dryL * (1.0f - mix01) + outL * mix01;
            yR = dryR * (1.0f - mix01) + outR * mix01;
        }

        // Store for shimmer processing
        shimmerOutL[n] = yL;
        shimmerOutR[n] = yR;
    }
    
    // Process shimmer as a block (much more efficient)
    if (shAmt > 0.01f) {
        g_shimmerProcessor.processBlock(shimmerBuffer.getReadPointer(0), 
                                       shimmerOutL.data(), 
                                       shimmerOutR.data(), 
                                       N, shAmt, pitch01, width01);
    }
    
    // Final output with safety processing
    for (int n = 0; n < N; ++n) {
        float yL = shimmerOutL[n];
        float yR = shimmerOutR[n];
        
        // Safety checks
        if (!std::isfinite(yL)) yL = 0.0f;
        if (!std::isfinite(yR)) yR = 0.0f;
        
        // Soft clipping
        const float clipThreshold = 0.95f;
        if (std::abs(yL) > clipThreshold) {
            yL = clipThreshold * std::tanh(yL / clipThreshold);
        }
        if (std::abs(yR) > clipThreshold) {
            yR = clipThreshold * std::tanh(yR / clipThreshold);
        }

        Lp[n] = flushDenorm(yL);
        if (Rp) Rp[n] = flushDenorm(yR);
    }
}

// -------------------------------------------------------
juce::String ShimmerReverb::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::Size:      return "Size";
        case ParamID::Shimmer:   return "Shimmer";
        case ParamID::Pitch:     return "Pitch";
        case ParamID::Damping:   return "Damping";
        case ParamID::Diffusion: return "Diffusion";
        case ParamID::Modulation:return "Modulation";
        case ParamID::Predelay:  return "PreDelay";
        case ParamID::Width:     return "Width";
        case ParamID::Freeze:    return "Freeze";
        case ParamID::Mix:       return "Mix";
        default:                 return {};
    }
}
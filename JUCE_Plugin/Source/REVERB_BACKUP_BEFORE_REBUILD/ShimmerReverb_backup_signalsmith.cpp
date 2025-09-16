// ==================== ShimmerReverb with Proper Signalsmith Integration ====================
#include "ShimmerReverb.h"
#include "signalsmith-stretch.h"
#include <algorithm>
#include <cmath>
#include <memory>

namespace {
struct FTZGuard {
    FTZGuard() {
       #if defined(__SSE__)
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
       #endif
    }
} s_ftzGuard;

// Simple pitch shifter for shimmer (beta quality, zero latency)
class SimpleShimmerPitch {
public:
    SimpleShimmerPitch() {
        // Use SimplePitchShift for beta - zero latency!
    }
    
    void prepare(double sampleRate, int maxBlockSize) {
        sr = sampleRate;
        blockSize = maxBlockSize;
        
        // Configure for real-time pitch shifting with low latency
        stretcher->presetCheaper(1, (float)sampleRate);
        stretcher->reset();
        stretcher2->presetCheaper(1, (float)sampleRate);
        stretcher2->reset();
        stretcher3->presetCheaper(1, (float)sampleRate);
        stretcher3->reset();
        
        // Initial transpose factors
        stretcher->setTransposeFactor(2.0f); // Default octave up (12 semitones)
        stretcher2->setTransposeFactor(3.0f); // Octave + fifth (19 semitones)
        stretcher3->setTransposeFactor(4.0f); // 2 octaves up (24 semitones)
        
        // We need buffering for the stretchers
        inputBuffer.resize(blockSize * 4);
        outputBuffer.resize(blockSize * 4);
        boostedInput.resize(blockSize);
        harmonicBuffer.resize(blockSize);
        fifthBuffer.resize(blockSize);
        
        reset();
    }
    
    void setPitchShift(float pitchParam) {
        // Shimmer pitch: 12 semitones (octave) + pitch adjustment
        // pitchParam 0.0 = octave (12 semitones)
        // pitchParam 1.0 = octave + fifth (19 semitones)
        float semitones = 12.0f + pitchParam * 7.0f; // 12-19 semitones range
        stretcher->setTransposeSemitones(semitones);
        currentPitchParam = pitchParam;
    }
    
    void processBlock(const float* input, float* output, int numSamples, float shimmerAmount) {
        if (shimmerAmount < 0.01f) {
            // No shimmer - clear output
            std::fill(output, output + numSamples, 0.0f);
            return;
        }
        
        // CRITICAL FIX: Signalsmith needs higher amplitude input for proper output
        // At 0.5 amplitude, it outputs ~64% of input
        // At 1.0 amplitude, it outputs ~127% of input  
        // We'll boost input to get better response
        if (boostedInput.size() < numSamples) {
            boostedInput.resize(numSamples);
        }
        
        if (harmonicBuffer.size() < numSamples) {
            harmonicBuffer.resize(numSamples);
        }
        
        if (fifthBuffer.size() < numSamples) {
            fifthBuffer.resize(numSamples);
        }
        
        for (int i = 0; i < numSamples; ++i) {
            // Boost input to 0.8 amplitude for optimal Signalsmith response
            boostedInput[i] = input[i] * 1.6f;
        }
        
        // Process octave (main shimmer) - 12 semitones
        const float* inputChannels[1] = { boostedInput.data() };
        float* outputChannels[1] = { output };
        stretcher->process(inputChannels, numSamples, outputChannels, numSamples);
        
        // BOOST THE OCTAVE - Signalsmith outputs weak signal, we need massive gain
        float octaveGain = 5.0f; // 5x gain to bring octave to proper level
        
        // Process octave + fifth (19 semitones) for richness
        float* fifthChannels[1] = { fifthBuffer.data() };
        stretcher2->setTransposeSemitones(19.0f); // Octave + perfect fifth
        stretcher2->process(inputChannels, numSamples, fifthChannels, numSamples);
        
        // Process 2nd octave (24 semitones) for sparkle if pitch param > 0.5
        if (currentPitchParam > 0.5f) {
            float* harmonicChannels[1] = { harmonicBuffer.data() };
            stretcher3->setTransposeSemitones(24.0f); // 2 octaves up
            stretcher3->process(inputChannels, numSamples, harmonicChannels, numSamples);
            
            // Mix all three harmonics
            float fifthMix = 0.7f; // Fifth is always present but slightly quieter than octave
            float harmonicMix = (currentPitchParam - 0.5f) * 2.0f; // 0-1 range for 2nd octave
            
            for (int i = 0; i < numSamples; ++i) {
                output[i] = (output[i] * octaveGain) +                    // Octave (loudest)
                           (fifthBuffer[i] * octaveGain * fifthMix) +     // Fifth (moderate)
                           (harmonicBuffer[i] * octaveGain * harmonicMix * 0.5f); // 2nd octave (subtle)
                output[i] *= shimmerAmount;
            }
        } else {
            // Octave + fifth, no 2nd octave
            float fifthMix = 0.7f * (0.5f + currentPitchParam); // Scale fifth with pitch param
            
            for (int i = 0; i < numSamples; ++i) {
                output[i] = (output[i] * octaveGain) +                    // Octave
                           (fifthBuffer[i] * octaveGain * fifthMix);      // Fifth
                output[i] *= shimmerAmount;
            }
        }
    }
    
    void reset() {
        stretcher->reset();
        stretcher2->reset();
        stretcher3->reset();
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        std::fill(harmonicBuffer.begin(), harmonicBuffer.end(), 0.0f);
        std::fill(fifthBuffer.begin(), fifthBuffer.end(), 0.0f);
    }
    
private:
    std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretcher;  // Octave (12 semitones)
    std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretcher2; // Fifth above octave (19 semitones)
    std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretcher3; // 2nd octave (24 semitones)
    double sr = 48000.0;
    int blockSize = 512;
    float currentPitchParam = 1.0f;
    
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::vector<float> boostedInput;
    std::vector<float> harmonicBuffer; // For 2nd octave
    std::vector<float> fifthBuffer;    // For octave + fifth
};

// Global instance
static SignalsmithShimmer g_signalsmithShimmer;

} // namespace

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
    
    // Initialize Signalsmith shimmer processor
    g_signalsmithShimmer.prepare(sr, samplesPerBlock);
    
    // Also prepare the simple shimmer as fallback
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
    g_signalsmithShimmer.reset();
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

    // Calculate feedback - increased range for longer reverb tails
    const float baseFeedback = 0.5f + 0.45f * size01;  // 0.5 to 0.95 range
    const float freezeBoost = freeze01 * 0.04f;  // Small boost when frozen (up to 0.99)
    const float fbBoost = std::min(0.99f, baseFeedback + freezeBoost);
    
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
    
    // Prepare shimmer buffer
    shimmerBuffer.clear();
    
    // Allocate temporary buffer for shimmer output
    std::vector<float> shimmerOutput(N, 0.0f);

    // First, process shimmer on DRY signal if enabled (much cleaner pitch shifting)
    if (shAmt > 0.01f) {
        // Collect dry input for shimmer - use full amplitude for better Signalsmith response
        for (int n = 0; n < N; ++n) {
            // Use stronger input signal (not halved)
            shimmerBuffer.setSample(0, n, Lp[n] + (Rp ? Rp[n] : Lp[n]));
        }
        
        // Process dry signal through pitch shifter
        g_signalsmithShimmer.setPitchShift(pitch01);
        g_signalsmithShimmer.processBlock(
            shimmerBuffer.getReadPointer(0),
            shimmerOutput.data(),
            N,
            1.0f  // Full strength for processing
        );
    }
    
    for (int n = 0; n < N; ++n) {
        const float inL = Lp[n];
        const float inR = (Rp ? Rp[n] : inL);
        
        // Store dry
        const float dryL = inL;
        const float dryR = inR;
        
        // Mono input for reverb - now includes shimmer if enabled
        float inMono = 0.5f * (inL + inR);
        
        // Add pitched signal to reverb input
        if (shAmt > 0.01f) {
            // Shimmer output is now properly gained in processBlock
            float shimmerGain = 0.5f * shAmt; // Reduced since we boosted in processBlock
            inMono = inMono * (1.0f - shAmt * 0.3f) + shimmerOutput[n] * shimmerGain;
        }

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
        
        // Moderate reverb level
        const float reverbLevel = 1.2f;
        outL *= reverbLevel;
        outR *= reverbLevel;
        
        // No longer collecting reverb output for shimmer - we process dry signal instead

        // Width processing on reverb
        const float widthScaled = width01 * width01;
        stereoWidth(outL, outR, widthScaled);

        // Mix dry/wet
        float yL, yR;
        if (mix01 < 0.001f) {
            yL = dryL;
            yR = dryR;
        } else {
            yL = dryL * (1.0f - mix01) + outL * mix01;
            yR = dryR * (1.0f - mix01) + outR * mix01;
        }
        
        // Add shimmer directly to output as well for more presence
        if (shAmt > 0.01f) {
            float directShimmer = shimmerOutput[n] * shAmt * 0.3f; // Reduced since we boosted
            yL += directShimmer;
            yR += directShimmer;
        }

        // Store temporarily
        Lp[n] = yL;
        if (Rp) Rp[n] = yR;
    }
    
    // Shimmer is now processed before reverb and fed into the reverb input
    // This creates a much cleaner shimmer effect with the pitched signal being reverberated
    // rather than trying to pitch shift the reverb output
    
    // Final safety processing
    for (int n = 0; n < N; ++n) {
        // Safety checks
        if (!std::isfinite(Lp[n])) Lp[n] = 0.0f;
        if (Rp && !std::isfinite(Rp[n])) Rp[n] = 0.0f;
        
        // Soft clipping
        const float clipThreshold = 0.95f;
        if (std::abs(Lp[n]) > clipThreshold) {
            Lp[n] = clipThreshold * std::tanh(Lp[n] / clipThreshold);
        }
        if (Rp && std::abs(Rp[n]) > clipThreshold) {
            Rp[n] = clipThreshold * std::tanh(Rp[n] / clipThreshold);
        }
        
        Lp[n] = flushDenorm(Lp[n]);
        if (Rp) Rp[n] = flushDenorm(Rp[n]);
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
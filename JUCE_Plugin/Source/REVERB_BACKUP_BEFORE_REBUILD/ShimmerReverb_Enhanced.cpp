// ==================== ShimmerReverb_Enhanced.cpp ====================
// Enhanced version with Signalsmith pitch shifting and multiple harmonics
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
}

// Enhanced shimmer processor with multiple harmonics
class EnhancedShimmerProcessor {
public:
    EnhancedShimmerProcessor() {
        // Initialize 3 pitch shifters for different harmonics
        for (auto& shifter : shifters) {
            shifter = std::make_unique<signalsmith::stretch::SignalsmithStretch<float>>();
        }
    }
    
    void prepare(double sampleRate, int blockSize) {
        sr = sampleRate;
        maxBlockSize = blockSize;
        
        // Configure each shifter for different harmonics
        for (auto& shifter : shifters) {
            shifter->presetCheaper(1, (float)sampleRate);
            shifter->reset();
        }
        
        // Set initial pitch ratios
        // Voice 1: Octave up (12 semitones)
        shifters[0]->setTransposeFactor(2.0f);
        
        // Voice 2: Octave + fifth (19 semitones)
        shifters[1]->setTransposeFactor(std::pow(2.0f, 19.0f/12.0f));
        
        // Voice 3: Two octaves (24 semitones)
        shifters[2]->setTransposeFactor(4.0f);
        
        // Allocate processing buffers
        inputBuffer.resize(maxBlockSize);
        for (auto& buf : outputBuffers) {
            buf.resize(maxBlockSize);
        }
    }
    
    void setSemitones(float semitones) {
        // semitones controls the base pitch shift (0-12)
        // We create harmonics relative to this
        float baseRatio = std::pow(2.0f, semitones / 12.0f);
        
        // Voice 1: Base octave shift
        shifters[0]->setTransposeFactor(baseRatio);
        
        // Voice 2: Add a perfect fifth above voice 1 (7 semitones)
        shifters[1]->setTransposeFactor(baseRatio * std::pow(2.0f, 7.0f/12.0f));
        
        // Voice 3: Add another octave (12 semitones above voice 1)
        shifters[2]->setTransposeFactor(baseRatio * 2.0f);
    }
    
    float process(float input, float shimmerAmount, float pitch01) {
        if (shimmerAmount < 0.01f) return 0.0f;
        
        // Update pitch based on parameter
        setSemitones(pitch01 * 12.0f);
        
        // Process through each harmonic shifter
        float output = 0.0f;
        
        // Voice mixing ratios (can be made adjustable)
        const float voice1Gain = 0.5f;   // Main octave
        const float voice2Gain = 0.25f;  // Fifth harmonic
        const float voice3Gain = 0.15f;  // Second octave
        
        // Process each voice
        const float* inputPtr = &input;
        
        // Voice 1: Main octave up
        float* output1 = outputBuffers[0].data();
        shifters[0]->process(&inputPtr, 1, &output1, 1);
        output += output1[0] * voice1Gain;
        
        // Voice 2: Fifth harmonic (only if shimmer > 0.5)
        if (shimmerAmount > 0.5f) {
            float* output2 = outputBuffers[1].data();
            shifters[1]->process(&inputPtr, 1, &output2, 1);
            output += output2[0] * voice2Gain * (shimmerAmount - 0.5f) * 2.0f;
        }
        
        // Voice 3: Second octave (only if shimmer > 0.75)
        if (shimmerAmount > 0.75f) {
            float* output3 = outputBuffers[2].data();
            shifters[2]->process(&inputPtr, 1, &output3, 1);
            output += output3[0] * voice3Gain * (shimmerAmount - 0.75f) * 4.0f;
        }
        
        // Apply shimmer amount and return
        return output * shimmerAmount * shimmerAmount; // Squared for better curve
    }
    
    void reset() {
        for (auto& shifter : shifters) {
            shifter->reset();
        }
    }
    
private:
    double sr = 48000.0;
    int maxBlockSize = 512;
    
    // Three pitch shifters for different harmonics
    std::array<std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>>, 3> shifters;
    
    // Processing buffers
    std::vector<float> inputBuffer;
    std::array<std::vector<float>, 3> outputBuffers;
};

// -------------------------------------------------------
ShimmerReverb::ShimmerReverb() {
    // musical-ish defaults
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

    // Set up parameter smoothing times
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

    // setup 4-delay FDN
    L_[0].delay.prepare(static_cast<int>(0.030 * sr));
    L_[1].delay.prepare(static_cast<int>(0.034 * sr));
    L_[2].delay.prepare(static_cast<int>(0.039 * sr));
    L_[3].delay.prepare(static_cast<int>(0.041 * sr));
    
    // Initialize allpass modulators for each line
    for (auto& l : L_) {
        l.ap1.prepare(128, sr);
        l.ap2.prepare(128, sr);
    }

    // pre-delay
    preDelay_.prepare(static_cast<int>(0.250 * sr));
    
    // Replace simple shimmer with enhanced processor
    // Note: This is a conceptual replacement - we need to modify the header too
    // For now, we'll use the enhanced processor internally
    static EnhancedShimmerProcessor enhancedShimmer;
    enhancedShimmer.prepare(sr, samplesPerBlock);

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
    
    // Reset enhanced shimmer
    static EnhancedShimmerProcessor enhancedShimmer;
    enhancedShimmer.reset();
}

// -------------------------------------------------------
void ShimmerReverb::updateParameters(const std::map<int, float>& params) {
    auto get = [&](int idx, float def){ auto it=params.find(idx); return it!=params.end()? it->second : def; };

    // Special handling for mix parameter to ensure proper dry passthrough
    auto mixIt = params.find((int)ParamID::Mix);
    if (mixIt != params.end()) {
        float mixValue = clamp01(mixIt->second);
        // For very low mix values, snap immediately for true dry signal
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

    // Pull smoothed params (block-rate)
    const float size01   = pSize.tick();
    const float shAmt    = pShimmer.tick();
    const float pitch01  = pPitch.tick();
    const float damp01   = pDamp.tick();
    const float diff01   = pDiff.tick();
    const float mod01    = pMod.tick();
    const float preMs    = pPredelay.tick() * 250.0f; // 0..250ms
    const float width01  = pWidth.tick();
    const float freeze01 = pFreeze.tick();
    const float mix01    = pMix.tick();

    // === ARCHITECTURAL FIX 1: Proper feedback calculation ===
    // Base feedback should be stable without mix dependency
    const float baseFeedback = 0.4f + 0.15f * size01; // 0.4 to 0.55
    const float freezeBoost = freeze01 * 0.3f; // Add up to 0.3 when frozen
    const float fbBoost = baseFeedback + freezeBoost;
    
    // Damping setup
    const float dampHz = 500.0f + 8000.0f * (1.0f - damp01); // Narrower range for stability
    for (auto& l : L_) l.damp.setCutoff(dampHz, sr_);

    // Diffusion allpass params
    const float apG = -0.7f + 0.5f * diff01; // -0.7 to -0.2
    const float apRate = 0.05f + 4.0f * mod01; // 0.05 to 4 Hz
    const float apDepth = 2.0f + 20.0f * mod01; // 2 to 22 samples
    for (auto& l : L_) {
        l.ap1.set(apG, apRate * 0.7f, apDepth);
        l.ap2.set(-apG, apRate * 1.1f, apDepth * 0.7f);
    }

    // Pre-delay setup
    const int preSamp = (int) std::round(std::min(0.25f, preMs * 0.001f) * sr_);

    // Get enhanced shimmer processor
    static EnhancedShimmerProcessor enhancedShimmer;

    float* Lp = buffer.getWritePointer(0);
    float* Rp = (numCh > 1 ? buffer.getWritePointer(1) : nullptr);

    for (int n=0; n<N; ++n) {
        const float inL = Lp[n];
        const float inR = (Rp ? Rp[n] : inL);
        
        // === ARCHITECTURAL FIX 2: Store dry signal immediately ===
        const float dryL = inL;
        const float dryR = inR;
        
        // Create mono input for reverb
        float inMono = 0.5f * (inL + inR);

        // === ARCHITECTURAL FIX 3: Proper input handling ===
        float reverbInput = inMono;
        
        // Apply freeze effect (gradually blocks input)
        const float freezeAmount = clamp01(freeze01 * 2.0f);
        reverbInput *= (1.0f - freezeAmount);

        // Pre-delay processing
        preDelay_.write(reverbInput);
        const float x = preSamp > 0 ? preDelay_.read(preSamp) : reverbInput;

        // Size-scaled delay lengths
        const float sizeScale = 0.3f + 1.4f * size01; // Narrower range: 0.3x to 1.7x
        const int delayA = std::max(64, (int)((float)L_[0].delay.buf.size() * sizeScale * 0.4f));
        const int delayB = std::max(64, (int)((float)L_[1].delay.buf.size() * sizeScale * 0.5f));
        const int delayC = std::max(64, (int)((float)L_[2].delay.buf.size() * sizeScale * 0.6f));
        const int delayD = std::max(64, (int)((float)L_[3].delay.buf.size() * sizeScale * 0.7f));

        // === FDN Network (always runs for tails) ===
        // Input injection with cross-coupling
        float a = L_[0].ap1.process(x + 0.15f * L_[3].state); // Reduced cross-coupling
        float b = L_[1].ap1.process(x + 0.15f * L_[0].state);
        float c = L_[2].ap1.process(x + 0.15f * L_[1].state);
        float d = L_[3].ap1.process(x + 0.15f * L_[2].state);

        // === ARCHITECTURAL FIX 5: Proper gain staging in feedback ===
        // Read delayed signals and apply feedback
        float delayedA = L_[0].damp.process(L_[0].delay.read(delayA));
        float delayedB = L_[1].damp.process(L_[1].delay.read(delayB));
        float delayedC = L_[2].damp.process(L_[2].delay.read(delayC));
        float delayedD = L_[3].damp.process(L_[3].delay.read(delayD));
        
        // Mix delayed signal with new input (reduced input contribution)
        a = delayedA * fbBoost + a * 0.05f; // Much less input contribution
        b = delayedB * fbBoost + b * 0.05f;
        c = delayedC * fbBoost + c * 0.05f;
        d = delayedD * fbBoost + d * 0.05f;

        // Write back with second diffuser
        L_[0].delay.write(L_[0].ap2.process(a));
        L_[1].delay.write(L_[1].ap2.process(b));
        L_[2].delay.write(L_[2].ap2.process(c));
        L_[3].delay.write(L_[3].ap2.process(d));

        // Remember states for next sample
        L_[0].state = a; L_[1].state = b; L_[2].state = c; L_[3].state = d;

        // === ARCHITECTURAL FIX 6: Normalized output matrix ===
        // Create stereo output from FDN taps with proper normalization
        float outL = 0.5f * a - 0.35f * b + 0.25f * c + 0.1f * d;
        float outR = -0.35f * a + 0.5f * b + 0.1f * c + 0.25f * d;
        
        // Moderate reverb boost for presence
        const float reverbBoost = 1.2f;
        outL *= reverbBoost;
        outR *= reverbBoost;

        // === ENHANCED Shimmer processing with multiple harmonics ===
        if (shAmt > 0.01f) {
            const float fdnSum = 0.25f * (a + b + c + d);
            
            // Process through enhanced shimmer with multiple harmonics
            float shimmerOut = enhancedShimmer.process(fdnSum, shAmt, pitch01);
            
            // Mix shimmer into stereo output with some width
            const float shimmerGain = 0.5f; // Adjust for taste
            outL += shimmerOut * shimmerGain * (0.9f + 0.1f * width01);
            outR += shimmerOut * shimmerGain * (0.9f - 0.1f * width01);
        }

        // Width processing
        const float widthScaled = width01 * width01;
        stereoWidth(outL, outR, widthScaled);

        // === ARCHITECTURAL FIX 7: Clean dry/wet mixing ===
        // Simple linear crossfade
        float yL, yR;
        
        // At mix=0, output pure dry signal
        if (mix01 < 0.001f) {
            yL = dryL;
            yR = dryR;
        } else {
            // Linear mix between dry and wet
            yL = dryL * (1.0f - mix01) + outL * mix01;
            yR = dryR * (1.0f - mix01) + outR * mix01;
        }

        // === Final safety processing ===
        // Denormal and NaN protection
        if (!std::isfinite(yL)) yL = 0.0f;
        if (!std::isfinite(yR)) yR = 0.0f;
        
        // Soft clipping at 0.95 to prevent digital overs
        const float clipThreshold = 0.95f;
        if (std::abs(yL) > clipThreshold) {
            yL = clipThreshold * std::tanh(yL / clipThreshold);
        }
        if (std::abs(yR) > clipThreshold) {
            yR = clipThreshold * std::tanh(yR / clipThreshold);
        }

        // Write output
        Lp[n] = flushDenorm(yL);
        if (Rp) Rp[n] = flushDenorm(yR);
    }
}

// -------------------------------------------------------
juce::String ShimmerReverb::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::Size:     return "Size";
        case ParamID::Shimmer:  return "Shimmer";
        case ParamID::Pitch:    return "Pitch";
        case ParamID::Damping:  return "Damping";
        case ParamID::Diffusion:return "Diffusion";
        case ParamID::Modulation:return "Modulation";
        case ParamID::Predelay: return "PreDelay";
        case ParamID::Width:    return "Width";
        case ParamID::Freeze:   return "Freeze";
        case ParamID::Mix:      return "Mix";
        default:                return {};
    }
}
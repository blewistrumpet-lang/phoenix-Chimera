// ==================== ShimmerReverb with Enhanced Harmonics ====================
// Uses simple pitch shifting but adds multiple harmonic layers
#include "ShimmerReverb.h"
#include <algorithm>
#include <cmath>

namespace {
struct FTZGuard {
    FTZGuard() {
       #if defined(__SSE__)
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
       #endif
    }
} s_ftzGuard;

// Enhanced octave shifter with harmonic generation
class MultiHarmonicShifter {
public:
    void prepare(int maxSamples, double sampleRate) {
        sr = sampleRate;
        
        // Main octave buffer
        octaveBuf.assign(std::max(2048, maxSamples), 0.0f);
        octaveWritePos = 0;
        octaveReadPosA = 0.0;
        octaveReadPosB = octaveBuf.size() * 0.5;
        
        // Fifth harmonic buffer (smaller for higher pitch)
        fifthBuf.assign(std::max(1024, maxSamples/2), 0.0f);
        fifthWritePos = 0;
        fifthReadPos = 0.0;
        
        // Second octave buffer
        secondOctBuf.assign(std::max(1024, maxSamples/2), 0.0f);
        secondOctWritePos = 0;
        secondOctReadPos = 0.0;
        
        xfade = 0.0f;
        xfadeStep = 1.0f / 256.0f;
    }
    
    void push(float sample) {
        // Write to all buffers
        octaveBuf[octaveWritePos] = sample;
        if (++octaveWritePos >= octaveBuf.size()) octaveWritePos = 0;
        
        fifthBuf[fifthWritePos] = sample;
        if (++fifthWritePos >= fifthBuf.size()) fifthWritePos = 0;
        
        secondOctBuf[secondOctWritePos] = sample;
        if (++secondOctWritePos >= secondOctBuf.size()) secondOctWritePos = 0;
    }
    
    float process(float shimmerAmount, float pitch01) {
        if (shimmerAmount < 0.01f) return 0.0f;
        
        // Calculate pitch ratios
        // For shimmer reverb: always shift up by at least one octave
        // pitch01=0: one octave up (ratio=2.0)
        // pitch01=1: two octaves up (ratio=4.0)
        float octaveRatio = 2.0f + 2.0f * pitch01; // 2.0 to 4.0
        float fifthRatio = octaveRatio * std::pow(2.0f, 7.0f/12.0f); // Perfect fifth above
        float secondOctRatio = octaveRatio * 2.0f; // Another octave up
        
        // === Main octave processing (dual-head with crossfade) ===
        octaveReadPosA += octaveRatio;
        octaveReadPosB += octaveRatio;
        
        const int octSize = octaveBuf.size();
        if (octaveReadPosA >= octSize) {
            octaveReadPosA -= octSize;
            xfade = 0.0f;
        }
        if (octaveReadPosB >= octSize) {
            octaveReadPosB -= octSize;
            xfade = 0.0f;
        }
        
        float octaveOut = tap(octaveBuf, octaveReadPosA) * (1.0f - xfade) +
                         tap(octaveBuf, octaveReadPosB) * xfade;
        xfade = std::min(1.0f, xfade + xfadeStep);
        
        // === Fifth harmonic (single head, simpler) ===
        float fifthOut = 0.0f;
        if (shimmerAmount > 0.5f) {
            fifthReadPos += fifthRatio;
            if (fifthReadPos >= fifthBuf.size()) {
                fifthReadPos -= fifthBuf.size();
            }
            fifthOut = tap(fifthBuf, fifthReadPos);
            fifthOut *= (shimmerAmount - 0.5f) * 2.0f; // Fade in from 50-100%
        }
        
        // === Second octave ===
        float secondOctOut = 0.0f;
        if (shimmerAmount > 0.75f) {
            secondOctReadPos += secondOctRatio;
            if (secondOctReadPos >= secondOctBuf.size()) {
                secondOctReadPos -= secondOctBuf.size();
            }
            secondOctOut = tap(secondOctBuf, secondOctReadPos);
            secondOctOut *= (shimmerAmount - 0.75f) * 4.0f; // Fade in from 75-100%
        }
        
        // Mix harmonics with stronger gains for more prominent shimmer
        float output = octaveOut * 1.2f +      // Main octave (doubled gain)
                      fifthOut * 0.5f +         // Fifth harmonic (doubled)
                      secondOctOut * 0.3f;      // Second octave (doubled)
        
        // Apply shimmer amount curve with less aggressive reduction
        return output * shimmerAmount * 1.5f;  // More linear response, higher gain
    }
    
    void reset() {
        std::fill(octaveBuf.begin(), octaveBuf.end(), 0.0f);
        std::fill(fifthBuf.begin(), fifthBuf.end(), 0.0f);
        std::fill(secondOctBuf.begin(), secondOctBuf.end(), 0.0f);
        
        octaveWritePos = 0;
        octaveReadPosA = 0.0;
        octaveReadPosB = octaveBuf.size() * 0.5;
        
        fifthWritePos = 0;
        fifthReadPos = 0.0;
        
        secondOctWritePos = 0;
        secondOctReadPos = 0.0;
        
        xfade = 0.0f;
    }
    
private:
    // Linear interpolation tap
    float tap(const std::vector<float>& buf, double pos) const {
        const int size = buf.size();
        while (pos < 0.0) pos += size;
        while (pos >= size) pos -= size;
        
        int i0 = (int)pos;
        int i1 = (i0 + 1) % size;
        float frac = (float)(pos - i0);
        
        return buf[i0] * (1.0f - frac) + buf[i1] * frac;
    }
    
    double sr = 48000.0;
    
    // Main octave buffer (dual-head)
    std::vector<float> octaveBuf;
    int octaveWritePos;
    double octaveReadPosA, octaveReadPosB;
    float xfade, xfadeStep;
    
    // Fifth harmonic buffer
    std::vector<float> fifthBuf;
    int fifthWritePos;
    double fifthReadPos;
    
    // Second octave buffer
    std::vector<float> secondOctBuf;
    int secondOctWritePos;
    double secondOctReadPos;
};

// Global instance for shimmer processor
static MultiHarmonicShifter s_harmonicShimmer;

} // namespace

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
    
    // Initialize enhanced harmonic shimmer
    s_harmonicShimmer.prepare(static_cast<int>(0.250 * sr), sr);
    
    // Also prepare the simple shimmer for compatibility
    shimmer_.prepare(static_cast<int>(0.250 * sr), sr);

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
    s_harmonicShimmer.reset();
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

    // Proper feedback calculation
    const float baseFeedback = 0.4f + 0.15f * size01; // 0.4 to 0.55
    const float freezeBoost = freeze01 * 0.3f; // Add up to 0.3 when frozen
    const float fbBoost = baseFeedback + freezeBoost;
    
    // Damping setup
    const float dampHz = 500.0f + 8000.0f * (1.0f - damp01);
    for (auto& l : L_) l.damp.setCutoff(dampHz, sr_);

    // Diffusion allpass params
    const float apG = -0.7f + 0.5f * diff01;
    const float apRate = 0.05f + 4.0f * mod01;
    const float apDepth = 2.0f + 20.0f * mod01;
    for (auto& l : L_) {
        l.ap1.set(apG, apRate * 0.7f, apDepth);
        l.ap2.set(-apG, apRate * 1.1f, apDepth * 0.7f);
    }

    // Pre-delay setup
    const int preSamp = (int) std::round(std::min(0.25f, preMs * 0.001f) * sr_);

    float* Lp = buffer.getWritePointer(0);
    float* Rp = (numCh > 1 ? buffer.getWritePointer(1) : nullptr);

    for (int n=0; n<N; ++n) {
        const float inL = Lp[n];
        const float inR = (Rp ? Rp[n] : inL);
        
        // Store dry signal
        const float dryL = inL;
        const float dryR = inR;
        
        // Create mono input for reverb
        float inMono = 0.5f * (inL + inR);

        // Proper input handling
        float reverbInput = inMono;
        
        // Apply freeze effect
        const float freezeAmount = clamp01(freeze01 * 2.0f);
        reverbInput *= (1.0f - freezeAmount);

        // Pre-delay processing
        preDelay_.write(reverbInput);
        const float x = preSamp > 0 ? preDelay_.read(preSamp) : reverbInput;

        // Size-scaled delay lengths
        const float sizeScale = 0.3f + 1.4f * size01;
        const int delayA = std::max(64, (int)((float)L_[0].delay.buf.size() * sizeScale * 0.4f));
        const int delayB = std::max(64, (int)((float)L_[1].delay.buf.size() * sizeScale * 0.5f));
        const int delayC = std::max(64, (int)((float)L_[2].delay.buf.size() * sizeScale * 0.6f));
        const int delayD = std::max(64, (int)((float)L_[3].delay.buf.size() * sizeScale * 0.7f));

        // FDN Network
        float a = L_[0].ap1.process(x + 0.15f * L_[3].state);
        float b = L_[1].ap1.process(x + 0.15f * L_[0].state);
        float c = L_[2].ap1.process(x + 0.15f * L_[1].state);
        float d = L_[3].ap1.process(x + 0.15f * L_[2].state);

        // Read delayed signals and apply feedback
        float delayedA = L_[0].damp.process(L_[0].delay.read(delayA));
        float delayedB = L_[1].damp.process(L_[1].delay.read(delayB));
        float delayedC = L_[2].damp.process(L_[2].delay.read(delayC));
        float delayedD = L_[3].damp.process(L_[3].delay.read(delayD));
        
        // Mix delayed signal with new input
        a = delayedA * fbBoost + a * 0.05f;
        b = delayedB * fbBoost + b * 0.05f;
        c = delayedC * fbBoost + c * 0.05f;
        d = delayedD * fbBoost + d * 0.05f;

        // Write back with second diffuser
        L_[0].delay.write(L_[0].ap2.process(a));
        L_[1].delay.write(L_[1].ap2.process(b));
        L_[2].delay.write(L_[2].ap2.process(c));
        L_[3].delay.write(L_[3].ap2.process(d));

        // Remember states
        L_[0].state = a; L_[1].state = b; L_[2].state = c; L_[3].state = d;

        // Create stereo output from FDN taps
        float outL = 0.5f * a - 0.35f * b + 0.25f * c + 0.1f * d;
        float outR = -0.35f * a + 0.5f * b + 0.1f * c + 0.25f * d;
        
        // Moderate reverb boost for presence
        const float reverbBoost = 1.2f;
        outL *= reverbBoost;
        outR *= reverbBoost;

        // === Shimmer processing using the simple octave shifter ===
        if (shAmt > 0.01f) {
            // Take signal from the FDN for shimmer processing
            const float fdnSum = 0.25f * (a + b + c + d);
            
            // Use the simple octave shifter from the header
            shimmer_.push(fdnSum);
            // Set to max (12 semitones = octave up) when pitch is at max
            shimmer_.setSemitones(12.0f); // Fixed octave up for true shimmer
            float shimmerOut = shimmer_.process() * shAmt;
            
            // Mix shimmer with good presence
            const float shimmerGain = 3.0f;
            outL += shimmerOut * shimmerGain;
            outR += shimmerOut * shimmerGain;
        }

        // Width processing
        const float widthScaled = width01 * width01;
        stereoWidth(outL, outR, widthScaled);

        // Clean dry/wet mixing
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

        // Final safety processing
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
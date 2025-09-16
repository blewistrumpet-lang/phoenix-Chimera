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
}

// -------------------------------------------------------
ShimmerReverb::ShimmerReverb() {
    // musical-ish defaults
    pSize.snap(0.5f);
    pShimmer.snap(0.0f);
    pPitch.snap(1.0f);     // => +12 semitones default target
    pDamp.snap(0.5f);
    pDiff.snap(0.6f);
    pMod.snap(0.4f);
    pPredelay.snap(0.0f);
    pWidth.snap(0.8f);
    pFreeze.snap(0.0f);
    pMix.snap(0.3f);
}

void ShimmerReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sr_ = std::max(8000.0, sampleRate);
    maxBlock_ = std::max(16, samplesPerBlock);

    // smoothing times
    pSize.setTimeMs(60.f, sr_);
    pShimmer.setTimeMs(80.f, sr_);
    pPitch.setTimeMs(80.f, sr_);
    pDamp.setTimeMs(30.f, sr_);
    pDiff.setTimeMs(30.f, sr_);
    pMod.setTimeMs(30.f, sr_);
    pPredelay.setTimeMs(10.f, sr_);
    pWidth.setTimeMs(40.f, sr_);
    pFreeze.setTimeMs(10.f, sr_);
    pMix.setTimeMs(15.f, sr_);

    // predelay up to 250ms
    preDelay_.prepare((int) std::ceil(0.25 * sr_));

    // lines
    for (int i=0;i<kLines;++i) {
        const double scale = 48'000.0 / sr_;
        const int len = std::max(128, (int) std::round(baseLen48_[i] / scale));
        L_[i].delay.prepare(len + 128); // margin for modulation
        L_[i].ap1.prepare(128, sr_);
        L_[i].ap2.prepare(128, sr_);
        L_[i].damp.setCutoff(8000.0f, sr_);
        L_[i].reset();
    }

    // shimmer buffer a bit larger than longest line
    int maxDelay = 0;
    for (int i=0;i<kLines;++i)
        maxDelay = std::max(maxDelay, (int)L_[i].delay.buf.size());
    shimmer_.prepare(std::max(maxDelay, (int)(0.2 * sr_)), sr_);

    reset();
}

void ShimmerReverb::reset() {
    preDelay_.reset();
    for (auto& l : L_) l.reset();
    shimmer_.reset();
}

// -------------------------------------------------------
void ShimmerReverb::updateParameters(const std::map<int, float>& params) {
    auto get = [&](int idx, float def){ auto it=params.find(idx); return it!=params.end()? it->second : def; };

    // keep APVTS mapping
    pSize.target.store(clamp01(get((int)ParamID::Size, 0.5f)), std::memory_order_relaxed);
    pShimmer.target.store(clamp01(get((int)ParamID::Shimmer, 0.0f)), std::memory_order_relaxed);
    pPitch.target.store(clamp01(get((int)ParamID::Pitch, 1.0f)), std::memory_order_relaxed);
    pDamp.target.store(clamp01(get((int)ParamID::Damping, 0.5f)), std::memory_order_relaxed);
    pDiff.target.store(clamp01(get((int)ParamID::Diffusion, 0.6f)), std::memory_order_relaxed);
    pMod.target.store(clamp01(get((int)ParamID::Modulation, 0.4f)), std::memory_order_relaxed);
    pPredelay.target.store(clamp01(get((int)ParamID::Predelay, 0.0f)), std::memory_order_relaxed);
    pWidth.target.store(clamp01(get((int)ParamID::Width, 0.8f)), std::memory_order_relaxed);
    pFreeze.target.store(clamp01(get((int)ParamID::Freeze, 0.0f)), std::memory_order_relaxed);
    pMix.target.store(clamp01(get((int)ParamID::Mix, 0.3f)), std::memory_order_relaxed);
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

    // derive internals - ENHANCED FEEDBACK SCALING FOR SIZE AND FREEZE
    const float fbBoost  = freeze01 > 0.01f ? (0.50f + 0.10f * size01 + 0.15f * freeze01) : (0.50f + 0.10f * size01); // Further reduced feedback
    const float dampHz   = 500.0f + 12000.0f * (1.0f - damp01); // WIDER damping range
    for (auto& l : L_) l.damp.setCutoff(dampHz, sr_);

    // diffusion allpass params - ENHANCED RANGES
    const float apG   = -0.7f + 0.65f * diff01; // -0.7...-0.05 (wider diffusion range)
    const float apRate= 0.05f + 8.0f * mod01;    // 0.05..8 Hz (more modulation)
    const float apDepth = 2.0f + 50.0f * mod01; // 2..52 samples (stronger modulation)
    for (auto& l : L_) {
        l.ap1.set(apG, apRate * 0.7f, apDepth);
        l.ap2.set(-apG, apRate * 1.1f, apDepth * 0.7f);
    }

    // predelay samples
    const int preSamp = (int) std::round(std::min(0.25f, preMs * 0.001f) * sr_);

    // shimmer semitones target (0..12)
    const float semis = 12.0f * pitch01;
    shimmer_.setSemitones(semis);

    float* Lp = buffer.getWritePointer(0);
    float* Rp = (numCh > 1 ? buffer.getWritePointer(1) : nullptr);

    for (int n=0; n<N; ++n) {
        const float inL = Lp[n];
        const float inR = (Rp ? Rp[n] : inL);
        float inMono = 0.5f * (inL + inR);

        // FREEZE: gradually block input & boost feedback
        const float freezeAmount = clamp01(freeze01 * 2.0f); // 0..1 over first half
        inMono *= (1.0f - freezeAmount); // gradually reduce input

        // predelay write
        preDelay_.write(inMono);
        const float x = preSamp > 0 ? preDelay_.read(preSamp) : inMono;

        // SIZE-SCALED delay lengths (0.2x to 2.0x of base length)
        const float sizeScale = 0.2f + 1.8f * size01; // 0.2..2.0x
        const int delayA = std::max(64, (int)((float)L_[0].delay.buf.size() * sizeScale * 0.4f));
        const int delayB = std::max(64, (int)((float)L_[1].delay.buf.size() * sizeScale * 0.5f));
        const int delayC = std::max(64, (int)((float)L_[2].delay.buf.size() * sizeScale * 0.6f));
        const int delayD = std::max(64, (int)((float)L_[3].delay.buf.size() * sizeScale * 0.7f));

        // FDN-ish network
        // inject small decorrelated taps
        float a = L_[0].ap1.process(x + 0.3f * L_[3].state);
        float b = L_[1].ap1.process(x + 0.3f * L_[0].state);
        float c = L_[2].ap1.process(x + 0.3f * L_[1].state);
        float d = L_[3].ap1.process(x + 0.3f * L_[2].state);

        // delays & damping in feedback - NOW USING SIZE-SCALED DELAYS
        a = L_[0].damp.process(L_[0].delay.read(delayA)) * fbBoost + a * 0.1f;
        b = L_[1].damp.process(L_[1].delay.read(delayB)) * fbBoost + b * 0.1f;
        c = L_[2].damp.process(L_[2].delay.read(delayC)) * fbBoost + c * 0.1f;
        d = L_[3].damp.process(L_[3].delay.read(delayD)) * fbBoost + d * 0.1f;

        // write back with second diffuser
        L_[0].delay.write( L_[0].ap2.process(a) );
        L_[1].delay.write( L_[1].ap2.process(b) );
        L_[2].delay.write( L_[2].ap2.process(c) );
        L_[3].delay.write( L_[3].ap2.process(d) );

        // remember states for injection next time
        L_[0].state = a; L_[1].state = b; L_[2].state = c; L_[3].state = d;

        // output mix from taps (reduced gains to prevent excessive output)
        float outL =  0.3f*a - 0.2f*b + 0.15f*c + 0.05f*d;
        float outR = -0.2f*a + 0.3f*b + 0.05f*c + 0.15f*d;

        // shimmer path: mono capture from (a+b+c+d) - Controlled shimmer
        const float net = 0.25f * (a + b + c + d);
        shimmer_.push(net);
        const float shimSample = shimmer_.process();  // octave-up
        const float shimMix = shAmt * shAmt;          // SQUARED for better response curve
        outL += shimSample * (0.6f * shimMix);        // Reduced shimmer gain
        outR += shimSample * (0.6f * shimMix);

        // width - ENHANCED STEREO WIDTH EFFECT
        const float widthScaled = width01 * width01; // squared for better curve
        stereoWidth(outL, outR, widthScaled);

        // wet/dry - Simple linear mix with lower wet gain
        const float dryL = inL;
        const float dryR = inR;
        const float wet = clamp01(mix01); // Linear response for predictable gain
        float yL = dryL * (1.0f - wet) + outL * wet * 0.3f; // Much lower wet gain to prevent excessive output
        float yR = dryR * (1.0f - wet) + outR * wet * 0.3f;

        // clip guard + denormal flush
        if (!std::isfinite(yL)) yL = 0.0f;
        if (!std::isfinite(yR)) yR = 0.0f;
        // More aggressive clipping at 0.95 to prevent exceeding 1.0
        if (std::abs(yL) > 0.95f) yL = 0.95f * std::tanh(yL / 0.95f);
        if (std::abs(yR) > 0.95f) yR = 0.95f * std::tanh(yR / 0.95f);

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
#include "AnalogPhaser.h"
#include "DspEngineUtilities.h"
#include "QualityMetrics.hpp" // keep if you have it; otherwise stub out
#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <cmath>

#if defined(__SSE2__)
 #include <immintrin.h>
#endif

namespace {
inline float flushD(float x) noexcept {
  #if defined(__SSE2__)
    // quick denorm flush
    if ((reinterpret_cast<const uint32_t&>(x) & 0x7f800000u) == 0u) return 0.0f;
  #endif
    return x;
}

struct Smoothed {
    std::atomic<float> target{0.f};
    float current = 0.f;
    float coeff   = 0.f;
    void setTime(float seconds, float fs) {
        seconds = std::max(1.0e-4f, seconds);
        coeff = std::exp(-1.0f / (seconds * fs));
    }
    float next() noexcept {
        const float t = target.load(std::memory_order_relaxed);
        current = t + (current - t) * coeff;
        return current;
    }
    void snap() noexcept { current = target.load(std::memory_order_relaxed); }
};

// 1st-order TPT all-pass (Zavalishin)
struct AllpassTPT {
    float g = 0.f; // tan(pi*fc/fs)
    float z = 0.f; // state
    // "a" form for convenience (a = (1-g)/(1+g)), but we keep g for stability
    float a = 0.f; // cached/clamped (-0.98..0.98)

    void setCutoffHz(float fc, float fs) {
        fc = juce::jlimit(10.0f, 0.45f*fs, fc);
        g  = std::tan(juce::MathConstants<float>::pi * (fc / fs));
        // convert to coefficient, clamp for safety
        float aa = (1.0f - g) / (1.0f + g);
        a = juce::jlimit(-0.98f, 0.98f, aa);
    }
    // If you want to modulate by directly setting a:
    void setA(float newA) {
        a = juce::jlimit(-0.98f, 0.98f, newA);
    }
    float process(float x) noexcept {
        // canonical 1st-order AP:
        // y = -x + z;  z = x + a*y;
        const float y = flushD(-x + z);
        z = flushD(x + a * y);
        if (!std::isfinite(z)) z = 0.0f;
        return y;
    }
    void reset() noexcept { z = 0.f; }
};

// Local DCBlocker removed - using DCBlocker from DspEngineUtilities

inline float softClip(float x) noexcept { return std::tanh(x); }
} // namespace

//==============================================================================
struct AnalogPhaser::Impl {
    static constexpr int kMaxStages = 8;
    static constexpr int kChannels  = 2;

    // Parameters (smoothed)
    Smoothed rate, depth, feedback, stages, spread, center, resonance, mix;

    // Runtime
    double fs = 44100.0;
    int    maxBlock = 512;

    // LFO state
    float lfoPhase[kChannels] { 0.f, 0.f };
    float lfoInc = 0.f;

    // Coefficient retune throttle
    int retuneCountdown[kChannels] { 0, 0 };

    // All-pass ladders
    std::array<std::array<AllpassTPT, kMaxStages>, kChannels> ap;
    int stageCount = 4;

    // IO helpers
    DCBlocker inDC[kChannels], outDC[kChannels];

    // Feedback state
    float fbState[kChannels] { 0.f, 0.f };

    // Optional metrics (assumed lock-free)
    QualityMetrics metrics;

    // ---- helpers ------------------------------------------------------------
    void defaults() {
        rate.target.store(0.3f);
        depth.target.store(0.8f);
        feedback.target.store(0.2f);
        stages.target.store(0.75f);
        spread.target.store(0.5f);
        center.target.store(0.4f);
        resonance.target.store(0.5f);
        mix.target.store(0.5f);

        rate.snap(); depth.snap(); feedback.snap(); stages.snap();
        spread.snap(); center.snap(); resonance.snap(); mix.snap();
    }

    inline int mapStages(float v) const noexcept {
        // 2,4,6,8 mapping
        int idx = (int) std::round(juce::jmap(v, 0.f, 1.f, 1.f, 4.f));
        return juce::jlimit(1, 4, idx) * 2;
    }

    inline float mapCenterHz(float v) const noexcept {
        return juce::jmap(v, 0.f, 1.f, 80.f, 2500.f);
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
       #if defined(__SSE2__)
        _mm_setcsr(_mm_getcsr() | 0x8040); // FTZ | DAZ
       #endif

        fs = std::max(8000.0, sampleRate);
        maxBlock = std::max(1, samplesPerBlock);

        // smoothing times
        const float ffs = (float) fs;
        rate.setTime(0.10f, ffs);
        depth.setTime(0.05f, ffs);
        feedback.setTime(0.05f, ffs);
        stages.setTime(0.10f, ffs);
        spread.setTime(0.05f, ffs);
        center.setTime(0.10f, ffs);
        resonance.setTime(0.10f, ffs);
        mix.setTime(0.02f, ffs);

        // reset states
        for (int ch=0; ch<kChannels; ++ch) {
            for (auto& s : ap[ch]) s.reset();
            inDC[ch].prepare(sampleRate);
            outDC[ch].prepare(sampleRate);
            inDC[ch].reset();
            outDC[ch].reset();
            fbState[ch] = 0.f;
            lfoPhase[ch] = (ch == 0) ? 0.f : juce::MathConstants<float>::pi; // stereo spread base
            retuneCountdown[ch] = 0;
        }

        metrics.setSampleRate(fs);
        metrics.reset();
    }

    void updateBlockParams()
    {
        // Compute once per block
        const float rateHz = juce::jmap(rate.current, 0.f, 1.f, 0.02f, 2.5f);
        lfoInc = 2.0f * juce::MathConstants<float>::pi * (rateHz / (float)fs);
        stageCount = mapStages(stages.current);
    }

    inline float lfo(float phase) const noexcept {
        // simple sine is smoother for coefficient updates
        return 0.5f * (1.0f + std::sin(phase)); // [0..1]
    }

    inline void retuneChannel(int ch, float modulatedFc, float Q)
    {
        modulatedFc = juce::jlimit(10.0f, 0.45f*(float)fs, modulatedFc);
        // spread stages slightly around center; scaled by Q
        for (int s=0; s<stageCount; ++s) {
            const float k = (float)s - (stageCount - 1) * 0.5f;
            const float skew = 1.0f + (k * 0.12f) / juce::jmax(0.6f, Q);
            const float fcs = juce::jlimit(10.0f, 0.45f*(float)fs, modulatedFc * skew);
            ap[ch][s].setCutoffHz(fcs, (float)fs); // internally clamps a ∈ (-0.98,0.98)
        }
    }

    float processSample(float in, int ch, float Q, float fbAmt) noexcept
    {
        in = inDC[ch].process(juce::jlimit(-2.0f, 2.0f, in));

        // ladder
        float y = in;
        for (int s=0; s<stageCount; ++s)
            y = ap[ch][s].process(y);

        // feedback path (bounded)
        const float fb = juce::jlimit(0.0f, 0.90f, fbAmt);
        const float fbin = 0.5f * (y + fbState[ch]);  // small leak averaging
        fbState[ch] = 0.98f * softClip(fb * fbin);

        float wet = y + fbState[ch];
        wet = outDC[ch].process(wet);
        if (!std::isfinite(wet)) wet = 0.0f;
        return wet;
    }
};

//==============================================================================
AnalogPhaser::AnalogPhaser() : pimpl(std::make_unique<Impl>()) {
    pimpl->defaults();
}

AnalogPhaser::~AnalogPhaser() = default;

void AnalogPhaser::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
}

void AnalogPhaser::reset() {
    pimpl = std::make_unique<Impl>();
    pimpl->defaults();
    pimpl->prepare(pimpl->fs, pimpl->maxBlock);
}

void AnalogPhaser::updateParameters(const std::map<int, float>& params) {
    auto set = [&](int idx, std::atomic<float>& t, float def) {
        auto it = params.find(idx);
        t.store(it != params.end() ? juce::jlimit(0.f, 1.f, it->second) : def,
                std::memory_order_relaxed);
    };
    set(kRate,         pimpl->rate.target,      0.3f);
    set(kDepth,        pimpl->depth.target,     0.8f);
    set(kFeedback,     pimpl->feedback.target,  0.2f);
    set(kStages,       pimpl->stages.target,    0.75f);
    set(kStereoSpread, pimpl->spread.target,    0.5f);
    set(kCenterFreq,   pimpl->center.target,    0.4f);
    set(kResonance,    pimpl->resonance.target, 0.5f);
    set(kMix,          pimpl->mix.target,       0.5f);
}

juce::String AnalogPhaser::getParameterName(int i) const {
    switch (i) {
        case kRate:          return "Rate";
        case kDepth:         return "Depth";
        case kFeedback:      return "Feedback";
        case kStages:        return "Stages";
        case kStereoSpread:  return "Stereo Spread";
        case kCenterFreq:    return "Center Freq";
        case kResonance:     return "Resonance";
        case kMix:           return "Mix";
        default:             return {};
    }
}

void AnalogPhaser::process(juce::AudioBuffer<float>& buffer)
{
    DenormalGuard guard;
    
    const int nCh = std::min(buffer.getNumChannels(), 2);
    const int nSm = buffer.getNumSamples();
    if (nCh <= 0 || nSm <= 0) return;

    // Smooth pulls once per block
    const float mix   = pimpl->mix.next();
    const float depth = pimpl->depth.next();
    const float spread= pimpl->spread.next();
    const float ctr   = pimpl->center.next();
    const float Q     = juce::jmap(pimpl->resonance.next(), 0.f, 1.f, 0.6f, 2.5f);
    const float fbAmt = pimpl->feedback.next();

    // Stage count & LFO rate derived once
    pimpl->stages.next(); // updates internal current
    pimpl->updateBlockParams();

    // Early bypass check for mix parameter
    if (mix < 0.001f) {
        // Completely dry - no processing needed, parameters already updated
        return;
    }

    auto* Lr = buffer.getReadPointer(0);
    auto* Rr = (nCh > 1 ? buffer.getReadPointer(1) : Lr);
    auto* Lw = buffer.getWritePointer(0);
    auto* Rw = (nCh > 1 ? buffer.getWritePointer(1) : nullptr);

    pimpl->metrics.startBlock();

    for (int i=0; i<nSm; ++i) {
        // advance per-sample LFO
        for (int ch=0; ch<Impl::kChannels; ++ch) {
            pimpl->lfoPhase[ch] += pimpl->lfoInc;
            if (pimpl->lfoPhase[ch] > juce::MathConstants<float>::twoPi)
                pimpl->lfoPhase[ch] -= juce::MathConstants<float>::twoPi;
        }

        // compute modulated center for each channel
        float fc0 = pimpl->mapCenterHz(ctr);
        float lL  = pimpl->lfo(pimpl->lfoPhase[0]);
        float lR  = pimpl->lfo(pimpl->lfoPhase[1] + spread * juce::MathConstants<float>::halfPi);

        // depth factor ~0.6..1.8x of center
        float dMul = juce::jmap(depth, 0.f, 1.f, 0.6f, 1.8f);
        float fcL  = juce::jlimit(10.0f, 0.45f*(float)pimpl->fs, fc0 * (0.97f + 0.06f * lL) * dMul);
        float fcR  = juce::jlimit(10.0f, 0.45f*(float)pimpl->fs, fc0 * (0.97f + 0.06f * lR) * dMul);

        // Retune ladders at a modest rate (every 64 samples) to avoid thrash
        if (--pimpl->retuneCountdown[0] <= 0) { pimpl->retuneCountdown[0] = 64; pimpl->retuneChannel(0, fcL, Q); }
        if (--pimpl->retuneCountdown[1] <= 0) { pimpl->retuneCountdown[1] = 64; pimpl->retuneChannel(1, fcR, Q); }

        float inL = Lr[i], inR = Rr[i];
        float wetL = pimpl->processSample(inL, 0, Q, fbAmt);
        float wetR = pimpl->processSample(inR, 1, Q, fbAmt);

        float outL = (1.0f - mix) * inL + mix * wetL;
        float outR = (1.0f - mix) * inR + mix * wetR;

        if (!std::isfinite(outL)) outL = 0.0f;
        if (!std::isfinite(outR)) outR = 0.0f;

        Lw[i] = outL;
        if (Rw) Rw[i] = outR;
    }

    pimpl->metrics.updatePeakRMS(buffer.getReadPointer(0), nSm);
    if (nCh > 1) pimpl->metrics.updatePeakRMS(buffer.getReadPointer(1), nSm);
    pimpl->metrics.endBlock(nSm, nCh);
    
    scrubBuffer(buffer);
}

float AnalogPhaser::getCPUUsage() const          { return pimpl->metrics.getCPUUsage(); }
float AnalogPhaser::getDynamicRangeDB() const    { return pimpl->metrics.getDynamicRangeDB(); }
std::string AnalogPhaser::getQualityReport() const { return pimpl->metrics.getReport(); }
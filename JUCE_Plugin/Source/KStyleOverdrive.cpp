#include "KStyleOverdrive.h"
#if defined(__SSE2__)
 #include <immintrin.h>
#endif

KStyleOverdrive::KStyleOverdrive() {
    // musical defaults
    pDrive_.target.store(0.35f); // light grit
    pTone_.target.store(0.5f);   // neutral tilt
    pLevel_.target.store(0.5f);  // 0 dB
    pMix_.target.store(1.0f);    // fully wet

    pDrive_.snap(); pTone_.snap(); pLevel_.snap(); pMix_.snap();

   #if defined(__SSE2__)
    _mm_setcsr(_mm_getcsr() | 0x8040); // FTZ/DAZ
   #endif
}

void KStyleOverdrive::prepareToPlay(double fs, int /*samplesPerBlock*/) {
    sampleRate_ = std::max(8000.0, fs);
    const float ffs = (float) sampleRate_;

    // gentle UI smoothing
    pDrive_.setTimeConst(0.03f, ffs);
    pTone_.setTimeConst(0.03f, ffs);
    pLevel_.setTimeConst(0.02f, ffs);
    pMix_.setTimeConst(0.02f, ffs);

    for (int ch = 0; ch < 2; ++ch) {
        tone_[ch].prepare(sampleRate_);
        tone_[ch].reset();
    }
}

void KStyleOverdrive::reset() {
    for (int ch = 0; ch < 2; ++ch) tone_[ch].reset();
}

void KStyleOverdrive::updateParameters(const std::map<int, float>& params) {
    auto set = [&](int idx, Smoothed& P, float def) {
        auto it = params.find(idx);
        P.target.store(it != params.end() ? clamp01(it->second) : def, std::memory_order_relaxed);
    };
    set(0, pDrive_, 0.35f);
    set(1, pTone_,  0.5f);
    set(2, pLevel_, 0.5f);
    set(3, pMix_,   1.0f);
}

juce::String KStyleOverdrive::getParameterName(int index) const {
    switch (index) {
        case 0: return "Drive";
        case 1: return "Tone";
        case 2: return "Level";
        case 3: return "Mix";
        default:return {};
    }
}

void KStyleOverdrive::process(juce::AudioBuffer<float>& buffer) {
    // DenormalGuard guard; // TODO: Add denormal protection
    
    const int nCh = std::min(buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    // block-smooth params
    const float drive = pDrive_.next();
    const float tone  = pTone_.next();
    const float level = fromdB(juce::jmap(pLevel_.next(), 0.0f, 1.0f, -12.0f, +12.0f));
    const float mix   = pMix_.next();

    for (int ch = 0; ch < nCh; ++ch)
        tone_[ch].setMix(tone);

    auto* Lr = buffer.getReadPointer(0);
    auto* Rr = (nCh > 1) ? buffer.getReadPointer(1) : Lr;
    auto* Lw = buffer.getWritePointer(0);
    auto* Rw = (nCh > 1) ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < n; ++i) {
        float inL = Lr[i];
        float inR = Rr[i];

        // Pre-EQ (slight HP to reduce mud before drive; stable TPT via TiltTone hp/lp choice)
        float preL = tone_[0].hp.processHP(inL); // use the HP core at ~1k cutoff
        float preR = tone_[1].hp.processHP(inR);

        // Nonlinearity
        float odL = waveshaper(preL, drive);
        float odR = waveshaper(preR, drive);

        // Post "tone" tilt (musical single knob)
        float postL = tone_[0].process(odL);
        float postR = tone_[1].process(odR);

        // Output level & mix
        float wetL = postL * level;
        float wetR = postR * level;

        float outL = (1.0f - mix) * inL + mix * wetL;
        float outR = (1.0f - mix) * inR + mix * wetR;

        // comprehensive NaN/Inf protection
        if (!std::isfinite(outL) || std::isnan(outL)) outL = 0.0f;
        if (!std::isfinite(outR) || std::isnan(outR)) outR = 0.0f;

        Lw[i] = outL;
        if (Rw) Rw[i] = outR;
    }
    
    // scrubBuffer(buffer); // TODO: Add buffer scrubbing
}
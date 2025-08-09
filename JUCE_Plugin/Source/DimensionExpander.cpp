#include "DimensionExpander.h"
#if defined(__SSE2__)
 #include <immintrin.h>
#endif

DimensionExpander::DimensionExpander() {
    // pleasant defaults
    pWidth_.target.store(0.7f);
    pDepth_.target.store(0.4f);
    pCross_.target.store(0.15f);
    pBassKeep_.target.store(0.5f);
    pAmb_.target.store(0.35f);
    pMove_.target.store(0.1f);
    pClar_.target.store(0.4f);
    pMix_.target.store(0.7f);

    pWidth_.snap(); pDepth_.snap(); pCross_.snap(); pBassKeep_.snap();
    pAmb_.snap();   pMove_.snap();  pClar_.snap();  pMix_.snap();

   #if defined(__SSE2__)
    _mm_setcsr(_mm_getcsr() | 0x8040); // FTZ/DAZ
   #endif
}

void DimensionExpander::prepareToPlay(double fs, int samplesPerBlock) {
    sampleRate_ = std::max(8000.0, fs);
    maxBlock_   = samplesPerBlock;

    const float ffs = (float) sampleRate_;
    // gentle UI smoothing
    pWidth_.setTau(0.05f, ffs);
    pDepth_.setTau(0.05f, ffs);
    pCross_.setTau(0.05f, ffs);
    pBassKeep_.setTau(0.05f, ffs);
    pAmb_.setTau(0.1f,  ffs);
    pMove_.setTau(0.1f, ffs);
    pClar_.setTau(0.05f,ffs);
    pMix_.setTau(0.02f, ffs);

    // micro delays up to ~20 ms
    dL_.prepare((int) std::ceil(0.020 * sampleRate_));
    dR_.prepare((int) std::ceil(0.020 * sampleRate_));

    // filters
    updateFilters();

    // APFs and LFO
    apL1_.setCoefficient(0.45f);
    apL2_.setCoefficient(0.55f);
    apR1_.setCoefficient(0.48f);
    apR2_.setCoefficient(0.52f);

    lfoPhase_ = 0.0f;
    lfoInc_   = 2.0f * juce::MathConstants<float>::pi * (0.12f / (float)sampleRate_);
}

void DimensionExpander::reset() {
    lowKeeperL_.reset(); lowKeeperR_.reset();
    clarityLP_.reset();  clarityHP_.reset();
    apL1_.reset(); apL2_.reset(); apR1_.reset(); apR2_.reset();
    dL_.reset(); dR_.reset();
    lfoPhase_ = 0.0f;
}

void DimensionExpander::updateParameters(const std::map<int, float>& params) {
    auto set = [&](int idx, Smoothed& P, float def){
        auto it = params.find(idx);
        P.target.store(it != params.end() ? clamp01(it->second) : def, std::memory_order_relaxed);
    };
    set(kWidth,         pWidth_,    0.7f);
    set(kDepth,         pDepth_,    0.4f);
    set(kCrossfeed,     pCross_,    0.15f);
    set(kBassRetention, pBassKeep_, 0.5f);
    set(kAmbience,      pAmb_,      0.35f);
    set(kMovement,      pMove_,     0.1f);
    set(kClarity,       pClar_,     0.4f);
    set(kMix,           pMix_,      0.7f);

    updateFilters();
}

juce::String DimensionExpander::getParameterName(int index) const {
    switch (index) {
        case kWidth:         return "Width";
        case kDepth:         return "Depth";
        case kCrossfeed:     return "Crossfeed";
        case kBassRetention: return "Bass Retention";
        case kAmbience:      return "Ambience";
        case kMovement:      return "Movement";
        case kClarity:       return "Clarity";
        case kMix:           return "Mix";
        default:             return "";
    }
}

void DimensionExpander::updateFilters() {
    const float ffs = (float) sampleRate_;

    // Bass retention: keep low-band in M (reduce widening for lows)
    // map control -> lowpass cutoff ~ [100..300 Hz] (more keep => higher cutoff)
    const float keepHz = juce::jmap(pBassKeep_.current, 0.f, 1.f, 100.f, 300.f);
    lowKeeperL_.setLowpass(keepHz, ffs);
    lowKeeperR_.setLowpass(keepHz, ffs);

    // Clarity: shape highs a bit; construct a "tilt" using LP+HP around ~2–4 kHz
    const float clarHz = juce::jmap(pClar_.current, 0.f, 1.f, 2000.f, 4000.f);
    clarityLP_.setLowpass(clarHz, ffs);
    clarityHP_.setLowpass(clarHz, ffs); // use HP topology at process time
}

void DimensionExpander::process(juce::AudioBuffer<float>& buffer) { DenormalGuard guard;
    const int nCh = std::min(buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    const float width   = pWidth_.next();     // 0..1
    const float depth   = pDepth_.next();     // 0..1
    const float cross   = pCross_.next();     // 0..1 (0=none, 1=heavy)
    const float keep    = pBassKeep_.next();  // controls LP split (already mapped)
    const float amb     = pAmb_.next();       // allpass mix
    const float move    = pMove_.next();      // LFO intensity for MS rotation
    const float clar    = pClar_.next();      // tilt intensity
    const float mix     = pMix_.next();

    // micro-delay amounts (samples): Depth blends between subtle haas and almost none
    const int   maxDelaySamp = dL_.size - 2; // safe
    const float haasMs       = juce::jmap(depth, 0.0f, 1.0f, 0.8f, 8.0f);
    const int   haasSamp     = juce::jlimit(1, std::max(1, (int)std::round(haasMs * 0.001 * sampleRate_)), maxDelaySamp);

    auto* Lr = buffer.getReadPointer(0);
    auto* Rr = (nCh > 1) ? buffer.getReadPointer(1) : Lr;
    auto* Lw = buffer.getWritePointer(0);
    auto* Rw = (nCh > 1) ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < n; ++i) {
        // LFO for movement (slow)
        lfoPhase_ += lfoInc_;
        if (lfoPhase_ > 2.0f * juce::MathConstants<float>::pi) lfoPhase_ -= 2.0f * juce::MathConstants<float>::pi;
        const float rot = move * 0.25f * std::sin(lfoPhase_); // +/- small radians

        float inL = Lr[i];
        float inR = Rr[i];

        // Crossfeed (pre-width), simple safe mix
        // cross=0 -> none, cross=1 -> 50/50
        const float cf = juce::jlimit(0.0f, 1.0f, cross);
        const float cfA = 1.0f - 0.5f * cf;
        const float cfB = 0.5f * cf;
        float xfL = cfA * inL + cfB * inR;
        float xfR = cfA * inR + cfB * inL;

        // Clarity tilt (LP+HP blend)
        float lpL = clarityLP_.processLP(xfL);
        float lpR = clarityLP_.processLP(xfR);
        // cheap HP via input - LP (TPT one-pole complement is stable enough here)
        float hpL = xfL - lpL;
        float hpR = xfR - lpR;
        const float tiltA = std::cos(juce::MathConstants<float>::halfPi * clar);
        const float tiltB = std::sin(juce::MathConstants<float>::halfPi * clar);
        float ctL = tiltA * lpL + tiltB * hpL;
        float ctR = tiltA * lpR + tiltB * hpR;

        // Micro-delay for depth (Haas)
        dL_.push(ctL);
        dR_.push(ctR);
        float mdL = dL_.readInt(haasSamp);
        float mdR = dR_.readInt(haasSamp);

        // Ambience allpass sprinkle
        float apMix = juce::jlimit(0.0f, 1.0f, amb);
        float apL = apL2_.process(apL1_.process(mdL));
        float apR = apR2_.process(apR1_.process(mdR));
        float ambL = (1.0f - apMix) * mdL + apMix * apL;
        float ambR = (1.0f - apMix) * mdR + apMix * apR;

        // Convert to M/S
        float M = 0.5f * (ambL + ambR);
        float S = 0.5f * (ambL - ambR);

        // Bass retention: reduce widening in lows by mixing in lowpassed M to mid
        float keepL = lowKeeperL_.processLP(M);
        float keepR = lowKeeperR_.processLP(M);
        M = juce::jmap(keep, 0.0f, 1.0f, M, keepL); // more keep => more lowpassed mid

        // Width: scale S only; no sqrt or divides
        S *= juce::jlimit(0.0f, 1.0f, width);

        // Movement: rotate (M,S) by small angle rot
        const float c = std::cos(rot), s = std::sin(rot);
        const float Mr =  c*M - s*S;
        const float Sr =  s*M + c*S;

        // Back to LR
        float wetL = Mr + Sr;
        float wetR = Mr - Sr;

        // Mix
        float outL = (1.0f - mix) * inL + mix * wetL;
        float outR = (1.0f - mix) * inR + mix * wetR;

        if (!finitef(outL)) outL = 0.0f;
        if (!finitef(outR)) outR = 0.0f;

        Lw[i] = outL;
        if (Rw) Rw[i] = outR;
    }
    
    scrubBuffer(buffer);
}
#include "SpringReverb_Platinum.h"
#if defined(__SSE2__)
 #include <immintrin.h>
#endif
#include <chrono>

SpringReverb_Platinum::SpringReverb_Platinum() {
    // Musical defaults
    pTension_.target.store(0.45f);
    pDamp_.target.store(0.35f);
    pDecay_.target.store(0.55f);
    pMod_.target.store(0.25f);
    pChirp_.target.store(0.15f);
    pDrive_.target.store(0.2f);
    pWidth_.target.store(0.75f);
    pMix_.target.store(0.35f);

    pTension_.snap(); pDamp_.snap(); pDecay_.snap(); pMod_.snap();
    pChirp_.snap();   pDrive_.snap(); pWidth_.snap(); pMix_.snap();

   #if defined(__SSE2__)
    _mm_setcsr(_mm_getcsr() | 0x8040); // FTZ/DAZ
   #endif
}

void SpringReverb_Platinum::prepareToPlay(double fs, int samplesPerBlock) {
    sampleRate_ = std::max(8000.0, fs);
    maxBlock_   = samplesPerBlock;

    // Smoothing
    const float ffs = (float) sampleRate_;
    pTension_.setTau(0.05f, ffs);
    pDamp_.setTau(0.05f, ffs);
    pDecay_.setTau(0.05f, ffs);
    pMod_.setTau(0.1f, ffs);
    pChirp_.setTau(0.2f, ffs);
    pDrive_.setTau(0.05f, ffs);
    pWidth_.setTau(0.05f, ffs);
    pMix_.setTau(0.02f, ffs);

    // Prepare delays (max 300 ms per line)
    const float maxMs = 300.0f;
    for (auto& l : L_) { l.delay.prepare(sampleRate_, maxMs); l.reset(); }
    for (auto& r : R_) { r.delay.prepare(sampleRate_, maxMs); r.reset(); }

    // Base APF setup
    for (auto* bank : { &L_, &R_ }) {
        for (auto& line : *bank) {
            line.apf1.set(0.5f);
            line.apf2.set(0.8f);
        }
    }

    // LFO for gentle dispersion modulation
    lfoPhase_ = 0.0f;
    lfoIncr_  = 2.0f * juce::MathConstants<float>::pi * (0.32f / (float)sampleRate_);

    // chirp
    chirpPhase_ = 0.0f;
    chirpGain_  = 0.0f;

    updateTankCoeffs();
}

void SpringReverb_Platinum::reset() {
    for (auto& l : L_) l.reset();
    for (auto& r : R_) r.reset();
    lfoPhase_ = 0.0f;
    chirpPhase_ = 0.0f;
    chirpGain_ = 0.0f;
}

void SpringReverb_Platinum::updateParameters(const std::map<int, float>& params) {
    auto set = [&](int idx, Smoothed& p, float def){
        auto it = params.find(idx);
        p.target.store(it != params.end() ? clamp01(it->second) : def, std::memory_order_relaxed);
    };
    set(kTension, pTension_, 0.45f);
    set(kDamping, pDamp_,    0.35f);
    set(kDecay,   pDecay_,   0.55f);
    set(kMod,     pMod_,     0.25f);
    set(kChirp,   pChirp_,   0.15f);
    set(kDrive,   pDrive_,   0.2f);
    set(kWidth,   pWidth_,   0.75f);
    set(kMix,     pMix_,     0.35f);
}

juce::String SpringReverb_Platinum::getParameterName(int index) const {
    switch (index) {
        case 0: return "Tension";
        case 1: return "Damping";
        case 2: return "Decay";
        case 3: return "Modulation";
        case 4: return "Chirp";
        case 5: return "Drive";
        case 6: return "Width";
        case 7: return "Mix";
        default: return "";
    }
}

void SpringReverb_Platinum::updateTankCoeffs() {
    const float ffs = (float) sampleRate_;

    // Map damping [0..1] to LP cutoff ~ [2k .. 14k]
    const float dampHz = juce::jmap(pDamp_.current, 0.0f, 1.0f, 2000.0f, 14000.0f);

    for (auto* bank : { &L_, &R_ }) {
        for (auto& line : *bank) {
            line.dampLP.setLowpass(dampHz, ffs);
        }
    }
}

float SpringReverb_Platinum::lineProcess(TankLine& line, float in, float baseDelaySamp, float modDepthSamp, float tensionDisp) {
    // APF scattering first
    float s = line.apf1.process(in);
    s = line.apf2.process(s);

    // Damping (HF loss)
    s = line.dampLP.processLP(s);

    // Delay with modulation: base +/- modDepth * sin
    // small dispersion "tension": add tiny phase-advance by nudging delay
    const float disp = tensionDisp; // already small
    double delayNow = juce::jlimit(1.0, (double)line.delay.capacity()-4.0,
                                   (double)baseDelaySamp + modDepthSamp * std::sin(lfoPhase_ + disp));
    float d = line.delay.readInterp(delayNow);

    // Push current damped sample (pre-feedback) into delay line
    line.delay.push(s);

    line.lastOut = d;
    return d;
}

void SpringReverb_Platinum::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    const int nCh = std::min(buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    // read smoothed params once/block
    const float tension   = pTension_.next(); // affects dispersion hint + delay set
    const float damping   = pDamp_.next();
    const float decay     = pDecay_.next();
    const float modAmt    = pMod_.next();
    const float chirpAmt  = pChirp_.next();
    const float drive     = pDrive_.next();
    const float width     = pWidth_.next();
    const float mix       = pMix_.next();

    // Update damping LP coeffs on change
    updateTankCoeffs();

    // Loop gain mapping: keep < 1.0 always
    const float loopGain = juce::jlimit(0.0f, 0.98f, juce::jmap(decay, 0.0f, 1.0f, 0.55f, 0.98f));

    // Base delays per line (ms) – staggered for density; tension shifts slightly
    const float baseMsL[kLines] = { 42.0f, 63.0f, 85.0f };
    const float baseMsR[kLines] = { 47.0f, 70.0f, 92.0f };
    const float tensShift = juce::jmap(tension, 0.0f, 1.0f, -3.0f, +3.0f);

    // Convert to samples
    float baseSampL[kLines], baseSampR[kLines];
    for (int i=0;i<kLines;++i) {
        baseSampL[i] = (float)((baseMsL[i] + tensShift) * 0.001 * sampleRate_);
        baseSampR[i] = (float)((baseMsR[i] - tensShift) * 0.001 * sampleRate_);
    }

    // Mod depth (samples)
    const float modDepth = juce::jmap(modAmt, 0.0f, 1.0f, 0.05f, 1.5f); // small, safe

    // Tension → small dispersion offset per line
    const float disp = juce::jmap(tension, 0.0f, 1.0f, 0.0f, 0.4f);

    // Drive and chirp
    const float preDrive = fromdB(juce::jmap(drive, 0.0f, 1.0f, 0.0f, 12.0f));
    const float chirpInc = 2.0f * juce::MathConstants<float>::pi * juce::jmap(chirpAmt, 0.0f, 1.0f, 0.0f, 3.0f) / (float)sampleRate_;
    chirpGain_ = juce::jlimit(0.0f, 1.0f, chirpGain_ * 0.995f + chirpAmt * 0.001f);

    auto* Lr = buffer.getReadPointer(0);
    auto* Rr = (nCh > 1) ? buffer.getReadPointer(1) : Lr;
    auto* Lw = buffer.getWritePointer(0);
    auto* Rw = (nCh > 1) ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < n; ++i) {
        // LFO advance
        lfoPhase_ += lfoIncr_;
        if (lfoPhase_ > 2.0f * juce::MathConstants<float>::pi) lfoPhase_ -= 2.0f * juce::MathConstants<float>::pi;

        // Input with soft drive and tiny chirp burst
        float chirp = chirpGain_ * std::sin(chirpPhase_);
        chirpPhase_ += chirpInc;
        if (chirpPhase_ > 2.0f * juce::MathConstants<float>::pi) chirpPhase_ -= 2.0f * juce::MathConstants<float>::pi;

        float inL = sat((Lr[i] + chirp) * preDrive);
        float inR = sat((Rr[i] + chirp) * preDrive);

        // Feed tank lines with cross feedback for diffusion
        float accL = 0.0f, accR = 0.0f;
        for (int k=0;k<kLines;++k) {
            float fbinL = inL + loopGain * (0.6f * L_[(k+0)%kLines].lastOut + 0.4f * R_[(k+1)%kLines].lastOut);
            float fbinR = inR + loopGain * (0.6f * R_[(k+0)%kLines].lastOut + 0.4f * L_[(k+1)%kLines].lastOut);

            accL += lineProcess(L_[k], fbinL, baseSampL[k], modDepth, disp * (k+1));
            accR += lineProcess(R_[k], fbinR, baseSampR[k], modDepth, disp * (k+1));
        }

        // Average lines
        float wetL = (accL / (float)kLines);
        float wetR = (accR / (float)kLines);

        // Gentle internal limiter to keep loop sane
        wetL = 0.98f * sat(wetL * 1.2f);
        wetR = 0.98f * sat(wetR * 1.2f);

        // Stereo width via safe M/S (no divides)
        float M = 0.5f * (wetL + wetR);
        float S = 0.5f * (wetL - wetR);
        S *= juce::jlimit(0.0f, 1.0f, width);
        wetL = M + S;
        wetR = M - S;

        // Mix
        float outL = (1.0f - mix) * Lr[i] + mix * wetL;
        float outR = (1.0f - mix) * Rr[i] + mix * wetR;

        // Final sanity
        if (!finitef(outL)) outL = 0.0f;
        if (!finitef(outR)) outR = 0.0f;

        Lw[i] = outL;
        if (Rw) Rw[i] = outR;
    }
    
    scrubBuffer(buffer);
}
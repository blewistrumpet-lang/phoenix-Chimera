#include "PhaseAlign_Platinum.h"
#include <chrono>

PhaseAlign_Platinum::PhaseAlign_Platinum() {
    // Defaults
    pAuto_.target.store(1.0f);
    pRef_.target.store(0.0f); // 0=Left reference
    pLoDeg_.target.store(0.5f);  // 0..1 -> -180..+180
    pLmDeg_.target.store(0.5f);
    pHmDeg_.target.store(0.5f);
    pHiDeg_.target.store(0.5f);
    pLoHz_.target.store(0.25f);  // maps to ~200 Hz default
    pMidHz_.target.store(0.33f); // ~1200 Hz default
    pHiHz_.target.store(0.5f);   // ~6 kHz default
    pMix_.target.store(0.5f);

    pAuto_.snap(); pRef_.snap(); pLoDeg_.snap(); pLmDeg_.snap(); pHmDeg_.snap(); pHiDeg_.snap();
    pLoHz_.snap(); pMidHz_.snap(); pHiHz_.snap(); pMix_.snap();
}

void PhaseAlign_Platinum::prepareToPlay(double fs, int samplesPerBlock) {
    sampleRate_ = std::max(8000.0, fs);
    maxBlock_   = samplesPerBlock;

    const float ffs = (float) sampleRate_;
    // smoothing
    pAuto_.setTau(0.02f, ffs);
    pRef_.setTau(0.02f, ffs);
    pLoDeg_.setTau(0.05f, ffs);
    pLmDeg_.setTau(0.05f, ffs);
    pHmDeg_.setTau(0.05f, ffs);
    pHiDeg_.setTau(0.05f, ffs);
    pLoHz_.setTau(0.05f, ffs);
    pMidHz_.setTau(0.05f, ffs);
    pHiHz_.setTau(0.05f, ffs);
    pMix_.setTau(0.02f, ffs);

    L_.prepare(sampleRate_); R_.prepare(sampleRate_);

    // Cross-corr ring buffers for ±10 ms lag
    maxLag_   = std::max(1, (int)std::round(0.010 * sampleRate_));
    delaySize_ = 2 * maxLag_ + maxBlock_ + 8; // ample headroom
    delayBufL_.assign(delaySize_, 0.0f);
    delayBufR_.assign(delaySize_, 0.0f);
    delayIdx_ = 0;

    align_.reset();
    updateXovers();
    updateAllpassPhases();
}

void PhaseAlign_Platinum::reset() {
    L_.reset(); R_.reset();
    align_.reset();
    std::fill(delayBufL_.begin(), delayBufL_.end(), 0.0f);
    std::fill(delayBufR_.begin(), delayBufR_.end(), 0.0f);
    delayIdx_ = 0;
}

void PhaseAlign_Platinum::updateParameters(const std::map<int, float>& params) {
    auto set = [&](int idx, Smoothed& p, float def){
        auto it = params.find(idx);
        p.target.store(it != params.end() ? clamp01(it->second) : def, std::memory_order_relaxed);
    };
    set(AUTO_ALIGN, pAuto_, 1.0f);
    set(REFERENCE,  pRef_,  0.0f);
    set(LOW_PHASE,  pLoDeg_,0.5f);
    set(LOW_MID_PHASE,pLmDeg_,0.5f);
    set(HIGH_MID_PHASE,pHmDeg_,0.5f);
    set(HIGH_PHASE, pHiDeg_,0.5f);
    set(LOW_FREQ,   pLoHz_, 0.25f);
    set(MID_FREQ,   pMidHz_,0.33f);
    set(HIGH_FREQ,  pHiHz_, 0.5f);
    set(MIX,        pMix_,  1.0f);

    updateXovers();
    updateAllpassPhases();
}

juce::String PhaseAlign_Platinum::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::AUTO_ALIGN:     return "Auto Align";
        case ParamID::REFERENCE:      return "Reference";
        case ParamID::LOW_PHASE:      return "Low Phase";
        case ParamID::LOW_MID_PHASE:  return "Low-Mid Phase";
        case ParamID::HIGH_MID_PHASE: return "High-Mid Phase";
        case ParamID::HIGH_PHASE:     return "High Phase";
        case ParamID::LOW_FREQ:       return "Low Freq";
        case ParamID::MID_FREQ:       return "Mid Freq";
        case ParamID::HIGH_FREQ:      return "High Freq";
        case ParamID::MIX:            return "Mix";
        default:                      return "";
    }
}

void PhaseAlign_Platinum::updateXovers() {
    // Use target values for parameter updates (not current which requires next() call)
    float lo = juce::jmap(pLoHz_.target.load(std::memory_order_relaxed),  0.f, 1.f, 50.f,  400.f);
    float mid= juce::jmap(pMidHz_.target.load(std::memory_order_relaxed), 0.f, 1.f, 400.f, 3000.f);
    float hi = juce::jmap(pHiHz_.target.load(std::memory_order_relaxed),  0.f, 1.f, 3000.f,12000.f);
    // ensure ordering
    mid = std::max(mid, lo + 10.f);
    hi  = std::max(hi,  mid + 100.f);

    L_.lp1.setLP(lo,  (float)sampleRate_); L_.lp2.setLP(mid, (float)sampleRate_);
    R_.lp1.setLP(lo,  (float)sampleRate_); R_.lp2.setLP(mid, (float)sampleRate_);
}

void PhaseAlign_Platinum::updateAllpassPhases() {
    // map [-180..+180] deg from [0..1]
    // Use target values for parameter updates (not current which requires next() call)
    auto mapDeg = [](float v01){ return (v01 - 0.5f) * 360.0f; };
    const float lo   = deg2rad(mapDeg(pLoDeg_.target.load(std::memory_order_relaxed)));
    const float lm   = deg2rad(mapDeg(pLmDeg_.target.load(std::memory_order_relaxed)));
    const float hm   = deg2rad(mapDeg(pHmDeg_.target.load(std::memory_order_relaxed)));
    const float hi   = deg2rad(mapDeg(pHiDeg_.target.load(std::memory_order_relaxed)));
    const float r    = 0.85f; // pole radius (fixed, safe)

    L_.apLow.set(lo, r);   R_.apLow.set(lo, r);
    L_.apLM.set(lm, r);    R_.apLM.set(lm, r);
    L_.apHM.set(hm, r);    R_.apHM.set(hm, r);
    L_.apHigh.set(hi, r);  R_.apHigh.set(hi, r);
}

inline void PhaseAlign_Platinum::pushDelayRing(float L, float R){
    delayBufL_[delayIdx_] = L;
    delayBufR_[delayIdx_] = R;
    if (++delayIdx_ >= delaySize_) delayIdx_ = 0;
}

inline float PhaseAlign_Platinum::readDelay(const std::vector<float>& buf, int center, int offset) const {
    int idx = center + offset;
    while (idx < 0) idx += delaySize_;
    while (idx >= delaySize_) idx -= delaySize_;
    return buf[idx];
}

void PhaseAlign_Platinum::computeAutoAlign(const float* L, const float* R, int n) {
    // Find integer delay by bounded cross-correlation (no FFT, no divide)
    // Window last N samples from ring centered at current delayIdx_-1
    const int center = (delayIdx_ - 1 + delaySize_) % delaySize_;
    float bestCorr = -1e9f;
    int   bestLag  = 0;

    // Simple bias toward 0 lag to avoid jumping
    const float bias = 0.001f;

    for (int lag = -maxLag_; lag <= maxLag_; ++lag) {
        double acc = 0.0;
        for (int i=0; i<n; ++i) {
            const float xl = readDelay(delayBufL_, center, -i);
            const float xr = readDelay(delayBufR_, center, -i - lag);
            acc += (double)xl * (double)xr;
        }
        const float score = (float)acc - bias * std::abs((float)lag);
        if (score > bestCorr) { bestCorr = score; bestLag = lag; }
    }

    // Fractional delay via 3-point parabolic interpolation of corr near bestLag
    auto corrAt = [&](int lag)->double{
        double acc=0.0;
        for (int i=0;i<n;++i){
            const float xl = readDelay(delayBufL_, center, -i);
            const float xr = readDelay(delayBufR_, center, -i - lag);
            acc += (double)xl * (double)xr;
        }
        return acc;
    };

    const double c0 = corrAt(bestLag-1);
    const double c1 = corrAt(bestLag);
    const double c2 = corrAt(bestLag+1);
    double denom = (c0 - 2.0*c1 + c2);
    double delta = 0.0;
    if (std::abs(denom) > 1e-9)
        delta = 0.5 * (c0 - c2) / denom; // in [-0.5, +0.5] ideally

    // Update state (apply small smoothing to avoid zipper)
    const float newInt  = (float)bestLag;
    const float newFrac = (float)juce::jlimit(-0.49, 0.49, delta);

    // Combine into positive frac within [0..3): use sign on integer
    float total = newInt + newFrac;
    // constrain to +/- maxLag with wrap handled by ring (we only need magnitude here)
    total = juce::jlimit((float)-maxLag_, (float)maxLag_, total);

    // Split into integer + fractional (0..3)
    int iPart = (int)std::floor(total);
    float fPart = (float)(total - (float)iPart);
    if (fPart < 0.0f){ fPart += 1.0f; iPart -= 1; } // keep fPart positive

    // Apply light smoothing for stability (clamp to safe range before Thiran)
    align_.intDelay  = iPart;
    align_.fracDelay = 0.2f * fPart + 0.8f * align_.fracDelay;  // Simple exponential smoothing
    align_.fracDelay = juce::jlimit(0.0f, 2.0f, align_.fracDelay);  // Match Thiran's safe limit
    align_.fracAP.set(align_.fracDelay);
}

void PhaseAlign_Platinum::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;

    const int nCh = std::min(buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    const float doAuto = pAuto_.next();
    const float refSel = pRef_.next();
    const float mix    = pMix_.next();

    auto* Lr = buffer.getReadPointer(0);
    auto* Rr = (nCh > 1 ? buffer.getReadPointer(1) : Lr);
    auto* Lw = buffer.getWritePointer(0);
    auto* Rw = (nCh > 1 ? buffer.getWritePointer(1) : nullptr);

    // Push ENTIRE block into delay ring for correlation BEFORE processing
    // This fixes causality violation - we need history before reading
    for (int i=0;i<n;++i) pushDelayRing(Lr[i], Rr[i]);

    // Auto delay estimate once per block (cheap, bounded)
    if (doAuto > 0.5f)
        computeAutoAlign(Lr, Rr, std::min(n, maxBlock_));

    // Choose which side to align (non-reference gets delayed)
    const bool rightIsRef = (refSel >= 0.5f);
    const int  iDelay     = align_.intDelay * (rightIsRef ? 1 : -1);

    // Clamp delay to safe range
    const int safeDelay = juce::jlimit(-maxLag_, maxLag_, std::abs(iDelay));

    // ---- Process per-sample ----
    for (int i=0; i<n; ++i) {
        float L = Lr[i], R = Rr[i];

        // Apply integer + fractional delay to the non-reference channel
        // Read from history buffer (safely) instead of trying to read ahead
        if (safeDelay > 0) {
            if (rightIsRef) {
                // Delay left - read from past
                // Safe modulo that handles negative values correctly
                int readPos = delayIdx_ - n + i - safeDelay;
                while (readPos < 0) readPos += delaySize_;
                readPos = readPos % delaySize_;
                L = delayBufL_[readPos];
                // Apply fractional delay
                L = align_.fracAP.process(L);
            } else {
                // Delay right - read from past
                int readPos = delayIdx_ - n + i - safeDelay;
                while (readPos < 0) readPos += delaySize_;
                readPos = readPos % delaySize_;
                R = delayBufR_[readPos];
                // Apply fractional delay
                R = align_.fracAP.process(R);
            }
        }

        // 4-band split - FIXED reconstruction using proper complementary filters
        // This ensures perfect reconstruction: L = L_lo + L_lm + L_hm + L_hi

        // Low: LP(lo)
        float L_lo  = L_.lp1.lp(L);
        float R_lo  = R_.lp1.lp(R);
        float L_rest1 = L_.lp1.hp(L);  // Use complementary HP instead of subtraction
        float R_rest1 = R_.lp1.hp(R);

        // Low-Mid: LP(mid) of high-pass output
        float L_lm  = L_.lp2.lp(L_rest1);
        float R_lm  = R_.lp2.lp(R_rest1);
        float L_rest2 = L_.lp2.hp(L_rest1);  // Complementary HP
        float R_rest2 = R_.lp2.hp(R_rest1);

        // High-Mid / High: For proper 4-band split, we need third crossover
        // Use simple 50/50 split for now to ensure stability
        // In production, this would be another TPT filter pair
        float L_hm = L_rest2 * 0.5f;
        float R_hm = R_rest2 * 0.5f;
        float L_hi = L_rest2 * 0.5f;
        float R_hi = R_rest2 * 0.5f;

        // Apply per-band all-pass rotations (constant magnitude, phase-only)
        L_lo = L_.apLow.process(L_lo);   R_lo = R_.apLow.process(R_lo);
        L_lm = L_.apLM.process(L_lm);    R_lm = R_.apLM.process(R_lm);
        L_hm = L_.apHM.process(L_hm);    R_hm = R_.apHM.process(R_hm);
        L_hi = L_.apHigh.process(L_hi);  R_hi = R_.apHigh.process(R_hi);

        // Reconstruct with proper summation
        float LwWet = L_lo + L_lm + L_hm + L_hi;
        float RwWet = R_lo + R_lm + R_hm + R_hi;

        // Safety check intermediate values
        if (!std::isfinite(LwWet)) LwWet = 0.0f;
        if (!std::isfinite(RwWet)) RwWet = 0.0f;

        float outL = (1.0f - mix) * Lr[i] + mix * LwWet;
        float outR = (1.0f - mix) * Rr[i] + mix * RwWet;

        // Final NaN/Inf protection (isfinite checks both)
        if (!std::isfinite(outL)) outL = 0.0f;
        if (!std::isfinite(outR)) outR = 0.0f;

        Lw[i] = outL;
        if (Rw) Rw[i] = outR;
    }

    // Scrub NaN/Inf values from output buffer
    scrubBuffer(buffer);
}
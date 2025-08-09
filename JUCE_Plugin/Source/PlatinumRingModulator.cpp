#include "PlatinumRingModulator.h"
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>

// Ensure FTZ/DAZ on x86
namespace {
struct DenormGuard {
    DenormGuard() {
       #if defined(__SSE__)
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
       #endif
    }
} s_denormGuard;
}

// ------------------------------------------------------
// ctor / prepare / reset
// ------------------------------------------------------
PlatinumRingModulator::PlatinumRingModulator() {
    // Musical defaults (same intent as original)
    p_carrierHz.snap(440.0f);
    p_ringAmt.snap(1.0f);
    p_freqShiftNorm.snap(0.0f);
    p_feedback.snap(0.0f);
    p_pulseWidth.snap(0.5f);
    p_phaseMod.snap(0.0f);
    p_stretch.snap(1.0f);
    p_tilt.snap(0.0f);
    p_resonance.snap(0.0f);
    p_shimmer.snap(0.0f);
    p_thermal.snap(0.0f);
    p_pitchTrack.snap(0.0f);
}

void PlatinumRingModulator::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sr_ = std::max(8000.0, sampleRate);
    maxBlock_ = std::max(16, samplesPerBlock);

    // smoothing times (ms)
    p_carrierHz.setTimeMs(10.f, sr_);
    p_ringAmt.setTimeMs(15.f, sr_);
    p_freqShiftNorm.setTimeMs(15.f, sr_);
    p_feedback.setTimeMs(40.f, sr_);
    p_pulseWidth.setTimeMs(20.f, sr_);
    p_phaseMod.setTimeMs(20.f, sr_);
    p_stretch.setTimeMs(40.f, sr_);
    p_tilt.setTimeMs(25.f, sr_);
    p_resonance.setTimeMs(25.f, sr_);
    p_shimmer.setTimeMs(45.f, sr_);
    p_thermal.setTimeMs(200.f, sr_);
    p_pitchTrack.setTimeMs(100.f, sr_);

    carrier_.reset();
    carrier_.setFreq(440.0f, sr_);

    for (auto& c: ch_) c.prepare(sr_);
}

void PlatinumRingModulator::reset() {
    carrier_.reset();
    for (auto& c: ch_) c.reset();
}

// ------------------------------------------------------
// parameters — NOTE: APVTS stays unchanged upstream.
// We only map [0..1] to engine ranges here.
// ------------------------------------------------------
void PlatinumRingModulator::updateParameters(const std::map<int, float>& params) {
    auto get = [&](int idx, float def){ auto it=params.find(idx); return it!=params.end()? it->second : def; };

    // idx 0: Carrier Frequency (20..5k)
    {
        const float norm = PlatinumRingModulator::clampFinite(get(0, 0.5f), 0.f, 1.f);
        // perceptual map
        const float hz = 20.0f * std::pow(250.0f, norm) + 20.0f; // ~20..~5k
        p_carrierHz.target.store(hz, std::memory_order_relaxed);
    }
    // idx 1: Ring Amount [0..1]
    p_ringAmt.target.store(PlatinumRingModulator::clampFinite(get(1, 1.0f),0.f,1.f), std::memory_order_relaxed);
    // idx 2: Frequency Shift (norm -1..+1)
    p_freqShiftNorm.target.store(PlatinumRingModulator::clampFinite(get(2,0.5f)*2.f-1.f,-1.f,1.f), std::memory_order_relaxed);
    // idx 3: Feedback [0..1] (internally < 0.9)
    p_feedback.target.store(PlatinumRingModulator::clampFinite(get(3,0.0f),0.f,1.f), std::memory_order_relaxed);
    // idx 4: Pulse Width [0.1..0.9]
    p_pulseWidth.target.store(PlatinumRingModulator::clampFinite(get(4,0.5f),0.f,1.f), std::memory_order_relaxed);
    // idx 5: Phase Mod depth [0..1] (kept for compatibility)
    p_phaseMod.target.store(PlatinumRingModulator::clampFinite(get(5,0.0f),0.f,1.f), std::memory_order_relaxed);
    // idx 6: Harmonic stretch [0.5..2.0]
    p_stretch.target.store(PlatinumRingModulator::clampFinite(get(6,0.5f),0.f,1.f), std::memory_order_relaxed);
    // idx 7: Spectral tilt [-1..+1]
    p_tilt.target.store(PlatinumRingModulator::clampFinite(get(7,0.5f)*2.f-1.f,-1.f,1.f), std::memory_order_relaxed);
    // idx 8: Resonance [0..1]
    p_resonance.target.store(PlatinumRingModulator::clampFinite(get(8,0.0f),0.f,1.f), std::memory_order_relaxed);
    // idx 9: Shimmer [0..1]
    p_shimmer.target.store(PlatinumRingModulator::clampFinite(get(9,0.0f),0.f,1.f), std::memory_order_relaxed);
    // idx 10: Thermal drift [0..1]
    p_thermal.target.store(PlatinumRingModulator::clampFinite(get(10,0.0f),0.f,1.f), std::memory_order_relaxed);
    // idx 11: Pitch tracking [0..1]
    p_pitchTrack.target.store(PlatinumRingModulator::clampFinite(get(11,0.0f),0.f,1.f), std::memory_order_relaxed);

    // NB: No allocation here, safe for RT usage model (map lives on message thread).
}

// ------------------------------------------------------
// process (hardened)
// ------------------------------------------------------
void PlatinumRingModulator::process(juce::AudioBuffer<float>& buffer) {
    const int numCh = std::min(buffer.getNumChannels(), 2);
    const int N = buffer.getNumSamples();
    if (N <= 0) return;

    // Update smoothed params (once per block is fine)
    const float carrierHz   = p_carrierHz.tick();
    const float ringAmt     = p_ringAmt.tick();
    const float shiftNorm   = p_freqShiftNorm.tick();
    const float fbAmt       = std::min(0.9f, p_feedback.tick() * 0.9f);
    const float pw          = 0.1f + 0.8f * p_pulseWidth.tick();
    const float phaseMod    = p_phaseMod.tick(); (void)phaseMod; // kept for compat
    const float stretch     = 0.5f + 1.5f * p_stretch.tick();
    const float tilt        = p_tilt.tick(); // -1..+1
    const float resAmt      = p_resonance.tick();
    const float shimAmt     = p_shimmer.tick();
    const float thermal     = p_thermal.tick();
    const float trackMix    = p_pitchTrack.tick();

    // Apply thermal drift subtly
    const float driftFactor = 1.0f + (thermal * 0.002f); // ±0.2%
    carrier_.setFreq(carrierHz * driftFactor, sr_);
    carrier_.pulseWidth = pw;
    carrier_.stretch = stretch;
    carrier_.subMix = std::clamp(0.25f * (tilt + 1.0f) * 0.5f, 0.0f, 0.3f); // gentle LF tilt to sub

    for (int ch = 0; ch < numCh; ++ch) {
        auto* d = buffer.getWritePointer(ch);
        auto& C = ch_[ch];

        for (int i=0;i<N;++i) {
            float x = d[i];
            // Optional pitch-tracking: mix carrier to target detected frequency
            float hz = carrierHz;
            if (usePitchTrack_ && trackMix > 1e-4f) {
                const float detected = C.yin.detectPush(x, sr_, C.yinDecim++);
                hz = juce::jmap(trackMix, 0.0f, 1.0f, carrierHz, detected);
                hz = clampFinite(hz, 20.0f, float(sr_*0.45));
            }
            carrier_.setFreq(hz * driftFactor, sr_);
            const float c = carrier_.tick();

            // Classic ring mod
            float y = processRing(x, c, ringAmt);

            // Frequency shifting via Hilbert
            y = processFreqShift(y, shiftNorm, C);

            // Feedback (bounded)
            processFeedback(y, fbAmt, C);

            // Resonance "color" (program-dependent)
            processResonance(y, resAmt, hz, C);

            // Shimmer (light pitch-shifted echo substitute, safe)
            processShimmer(y, shimAmt, C);

            // Output DC block + clip guard
            y = C.dcBlock(y);

            // Final hardening: finite + soft limiter
            if (!std::isfinite(y)) y = 0.0f;
            if (std::abs(y) > 1.2f) y = 1.2f * std::tanh(y / 1.2f);

            d[i] = y;
        }
    }
}

// ------------------------------------------------------
// helpers
// ------------------------------------------------------
float PlatinumRingModulator::processRing(float in, float carrier, float amt) noexcept {
    amt = clampFinite(amt, 0.0f, 1.0f);
    const float ring = in * carrier;
    const float out = in*(1.0f - amt) + ring*amt;
    return flushDenorm(out);
}

float PlatinumRingModulator::processFreqShift(float in, float norm, Channel& c) noexcept {
    // norm in [-1..1] maps to ±500 Hz
    if (std::abs(norm) < 1e-4f) return in;
    const float shiftHz = 500.0f * clampFinite(norm, -1.0f, 1.0f);

    // analytic signal
    const auto z = c.hilb.process(in);
    // NCO (complex) using a small phase increment, integrate safely
    static thread_local float ph = 0.0f;
    ph += 2.0f * float(M_PI) * (shiftHz / float(sr_));
    if (ph >  2.0f*float(M_PI)) ph -= 2.0f*float(M_PI);
    if (ph < -2.0f*float(M_PI)) ph += 2.0f*float(M_PI);

    const float cs = std::cos(ph);
    const float sn = std::sin(ph);
    // analytic * e^{j ph}
    const float re = z.real()*cs - z.imag()*sn;
    // imag discarded for real output
    return flushDenorm(re);
}

void PlatinumRingModulator::processFeedback(float& x, float fbAmt, Channel& c) noexcept {
    if (fbAmt <= 1e-4f) return;
    // Safe margin
    const float g = std::clamp(fbAmt, 0.0f, 0.9f);
    const int D = (int)c.fbDelay.size();
    const int delaySamp = std::clamp((int)(0.010 * sr_), 1, D-2); // ~10ms
    int rp = c.fbW - delaySamp; if (rp < 0) rp += D;
    const float fb = c.fbDelay[rp];
    // inject with soft clip
    x = flushDenorm(x + softClip(fb * (g * 0.7f)));
    // write
    c.fbDelay[c.fbW] = x;
    if (++c.fbW == D) c.fbW = 0;
}

void PlatinumRingModulator::processResonance(float& x, float resAmt, float baseHz, Channel& c) noexcept {
    if (resAmt <= 1e-4f) return;
    // map resAmt to Q and amount
    const float q = 0.5f + 9.5f * std::clamp(resAmt, 0.0f, 1.0f); // 0.5..10
    const float freq = std::clamp(baseHz * 2.0f, 30.0f, float(sr_*0.45));
    c.svf.set(freq, q, sr_);
    const float bp = c.svf.bp(x);
    x = flushDenorm(x*(1.0f - 0.4f*resAmt) + bp*(0.4f*resAmt));
}

void PlatinumRingModulator::processShimmer(float& x, float shimAmt, Channel& c) noexcept {
    if (shimAmt <= 1e-4f) return;
    // super simple, stable "shimmer": short bright echo
    const int D = (int)c.shim.size();
    const int dSamp = std::clamp((int)(0.050 * sr_), 1, D-2); // ~50ms
    int rp = c.shW - dSamp; if (rp < 0) rp += D;
    const float y = c.shim[rp];
    // write current with slight HF tilt
    const float write = x + 0.1f*(x - c.dcX); // tiny pre-emphasis
    c.shim[c.shW] = write;
    if (++c.shW == D) c.shW = 0;

    x = flushDenorm(x + y * (0.25f * std::clamp(shimAmt, 0.0f, 1.0f)));
}

// ------------------------------------------------------
// parameter names (indices preserved to match APVTS)
// ------------------------------------------------------
juce::String PlatinumRingModulator::getParameterName(int index) const {
    switch (index) {
        case 0:  return "Carrier Frequency";
        case 1:  return "Ring Amount";
        case 2:  return "Frequency Shift";
        case 3:  return "Feedback";
        case 4:  return "Pulse Width";
        case 5:  return "Phase Modulation";
        case 6:  return "Harmonic Stretch";
        case 7:  return "Spectral Tilt";
        case 8:  return "Resonance";
        case 9:  return "Shimmer";
        case 10: return "Thermal Drift";
        case 11: return "Pitch Tracking";
        default: return {};
    }
}
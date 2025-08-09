#include "VintageOptoCompressor_Platinum.h"
#include <chrono>
#if defined(__SSE2__)
 #include <immintrin.h>
#endif

// Keep your original names
const std::array<juce::String, 8>
VintageOptoCompressor_Platinum::kParameterNames = {
    "Gain",           // 0
    "Peak Reduction", // 1
    "HF Emphasis",    // 2
    "Output",         // 3
    "Mix",            // 4
    "Knee",           // 5
    "Harmonics",      // 6
    "Stereo Link"     // 7
};

VintageOptoCompressor_Platinum::VintageOptoCompressor_Platinum() {
    // musical defaults
    pGain_.target.store(0.5f);          // -12..+12dB -> 0dB
    pPeakReduction_.target.store(0.5f); // threshold-ish middle
    pEmph_.target.store(0.3f);          // slight HF emphasis
    pOut_.target.store(0.5f);           // 0 dB
    pMix_.target.store(0.5f);           // 50/50
    pKnee_.target.store(0.5f);          // ~6 dB knee
    pHarm_.target.store(0.15f);         // subtle harmonics
    pLink_.target.store(1.0f);          // fully linked by default

    pGain_.snap(); pPeakReduction_.snap(); pEmph_.snap(); pOut_.snap();
    pMix_.snap();  pKnee_.snap();        pHarm_.snap();   pLink_.snap();

   #if defined(__SSE2__)
    _mm_setcsr(_mm_getcsr() | 0x8040); // FTZ/DAZ
   #endif
}

void VintageOptoCompressor_Platinum::prepareToPlay(double fs, int /*samplesPerBlock*/) {
    sampleRate_ = std::max(8000.0, fs);
    const float ffs = (float) sampleRate_;

    // UI smoothers
    pGain_.setTau(0.02f, ffs);
    pPeakReduction_.setTau(0.02f, ffs);
    pEmph_.setTau(0.05f, ffs);
    pOut_.setTau(0.02f, ffs);
    pMix_.setTau(0.02f, ffs);
    pKnee_.setTau(0.05f, ffs);
    pHarm_.setTau(0.05f, ffs);
    pLink_.setTau(0.05f, ffs);

    // sidechain: HP + LP (we'll blend for tilt)
    scHP_.set(120.0f, 0.707f, ffs);
    scLP_.set(6000.0f, 0.707f, ffs);

    // detector timing (will be re-mapped each block from params)
    envAtk_.setTau(0.005f, ffs);
    envRel_.setTau(0.200f, ffs);
    env_ = 0.0f;

    // GR smoothing in dB (~10ms)
    grSmooth_.setTau(0.010f, ffs);
    grSmooth_.reset();

    metrics_.cpu.store(0.0f);
    metrics_.peak.store(0.0f);
}

void VintageOptoCompressor_Platinum::reset() {
    scHP_.reset(); scLP_.reset();
    env_ = 0.0f;
    envAtk_.reset(); envRel_.reset();
    grSmooth_.reset();
}

void VintageOptoCompressor_Platinum::updateParameters(const std::map<int, float>& params) {
    auto set = [&](int idx, Smoothed& p, float def){
        auto it = params.find(idx);
        p.target.store(it == params.end() ? def : clamp01(it->second), std::memory_order_relaxed);
    };
    set(kParamGain,         pGain_,         0.5f);
    set(kParamPeakReduction,pPeakReduction_,0.5f);
    set(kParamEmphasis,     pEmph_,         0.3f);
    set(kParamOutput,       pOut_,          0.5f);
    set(kParamMix,          pMix_,          0.5f);
    set(kParamKnee,         pKnee_,         0.5f);
    set(kParamHarmonics,    pHarm_,         0.15f);
    set(kParamStereoLink,   pLink_,         1.0f);
}

juce::String VintageOptoCompressor_Platinum::getParameterName(int index) const {
    if (index >= 0 && index < (int)kParameterNames.size()) return kParameterNames[(size_t)index];
    return {};
}

inline float VintageOptoCompressor_Platinum::detectMono(float L, float R) noexcept {
    // pre-filter (TPT SVF), then tilt
    float m = 0.5f * (L + R);
    float hp = scHP_.processHP(m);
    float lp = scLP_.processLP(m);
    float sc = m + (lp - hp) * 0.5f * scTilt_; // tilt in [-1..+1]
    return std::abs(sc);
}

inline float VintageOptoCompressor_Platinum::gainReductionDB(float envLin, float peakRed, float ratio, float kneeDB) noexcept {
    // Map params to curve
    const float thr = juce::jmap(peakRed, 0.0f, 1.0f, 0.0f, -36.0f);         // threshold dB
    const float r   = juce::jmap(ratio,   0.0f, 1.0f, 2.0f,  8.0f);          // 2:1 .. 8:1
    const float xDB = toDB(envLin);
    const float tDB = thr;
    const float k   = juce::jlimit(0.0f, 18.0f, kneeDB);

    const float over = xDB - tDB;
    float grDB = 0.0f;

    if (over <= -0.5f * k) {
        grDB = 0.0f; // below knee
    } else if (over >= 0.5f * k) {
        grDB = -(1.0f - 1.0f/r) * over; // above knee
    } else {
        // knee region (quadratic crossfade)
        const float x = (over + 0.5f*k) / std::max(1.0e-6f, k); // 0..1
        const float full = (1.0f - 1.0f/r) * (xDB - (tDB - 0.5f*k));
        grDB = -full * x * x;
    }

    return juce::jlimit(-48.0f, 0.0f, grDB);
}

void VintageOptoCompressor_Platinum::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    const auto t0 = std::chrono::high_resolution_clock::now();

    const int nCh = std::min(buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    // Read smoothed params once per block
    const float inGain  = fromDB(juce::jmap(pGain_.next(), 0.f, 1.f, -12.f, +12.f));
    const float outGain = fromDB(juce::jmap(pOut_.next(),  0.f, 1.f, -12.f, +12.f));
    const float mix     = pMix_.next();
    const float peakRed = pPeakReduction_.next();
    const float kneeDB  = juce::jmap(pKnee_.next(), 0.f, 1.f, 0.f, 12.f);
    const float harmon  = pHarm_.next();
    const float link    = pLink_.next();

    scTilt_ = juce::jmap(pEmph_.next(), 0.f, 1.f, -1.f, +1.f);

    // attack/release mapping (musical)
    const float atkMs = juce::jmap(peakRed, 0.f, 1.f, 5.f,  30.f);   // slightly slower with more reduction
    const float relMs = juce::jmap(peakRed, 0.f, 1.f, 120.f, 600.f); // slower release for deeper GR
    envAtk_.setTau(atkMs * 0.001f, (float)sampleRate_);
    envRel_.setTau(relMs * 0.001f, (float)sampleRate_);

    auto* Lw = buffer.getWritePointer(0);
    auto* Rw = (nCh > 1) ? buffer.getWritePointer(1) : nullptr;
    auto* Lr = buffer.getReadPointer(0);
    auto* Rr = (nCh > 1) ? buffer.getReadPointer(1) : Lr;

    for (int i = 0; i < n; ++i) {
        float xL = Lr[i] * inGain;
        float xR = Rr[i] * inGain;

        // Per-channel detect, then stereo link (link=1 → max/mono; link=0 → per-channel)
        const float dL = detectMono(xL, xL);
        const float dR = detectMono(xR, xR);
        const float dM = std::max(dL, dR);
        const float detL = link * dM + (1.0f - link) * dL;
        const float detR = link * dM + (1.0f - link) * dR;

        // Update single envelope using the larger (classic linked opto feel)
        const float det = std::max(detL, detR);
        const float a = (det > env_) ? envAtk_.a : envRel_.a;
        env_ = a * env_ + (1.0f - a) * det;

        // Gain reduction (dB), smooth in dB, then convert to linear
        const float grDB = gainReductionDB(env_, peakRed, /*ratio*/ juce::jmap(peakRed,0.f,1.f,2.f,6.f), kneeDB);
        const float grLin = fromDB( grSmooth_.process(grDB) );

        float yL = xL * grLin;
        float yR = xR * grLin;

        // subtle post nonlinearity (Harmonics)
        if (harmon > 0.001f) {
            const float k = juce::jmap(harmon, 0.f, 1.f, 0.f, 1.5f);
            yL = std::tanh(yL * (1.0f + k)) / std::max(1.0f, (1.0f + 0.5f*k));
            yR = std::tanh(yR * (1.0f + k)) / std::max(1.0f, (1.0f + 0.5f*k));
        }

        // output gain + mix (keep dry path pre-input gain to avoid "double gain" feel)
        const float dryL = Lr[i];
        const float dryR = Rr[i];
        const float wetL = yL * outGain;
        const float wetR = yR * outGain;

        float outL = (1.0f - mix) * dryL + mix * wetL;
        float outR = (1.0f - mix) * dryR + mix * wetR;

        // final sanity (shouldn't ever trip with this math)
        if (!finitef(outL)) outL = 0.0f;
        if (!finitef(outR)) outR = 0.0f;

        Lw[i] = outL;
        if (Rw) Rw[i] = outR;
    }

    // meters
    const auto t1 = std::chrono::high_resolution_clock::now();
    const double dt = std::chrono::duration<double>(t1 - t0).count();
    const double block = (double)n / sampleRate_;
    const float cpu = (float) juce::jlimit(0.0, 100.0, 100.0 * (dt / std::max(1e-9, block)));
    metrics_.cpu.store(cpu);
    float pk = metrics_.peak.load();
    if (cpu > pk) metrics_.peak.store(cpu);
    
    scrubBuffer(buffer);
}
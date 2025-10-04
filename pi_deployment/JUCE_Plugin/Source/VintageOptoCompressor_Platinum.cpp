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
    // DEBUG: Track instance creation
    FILE* f = fopen("/tmp/opto_lifecycle.txt", "a");
    if (f) {
        fprintf(f, "VintageOptoCompressor CREATED: instance=%p\n", (void*)this);
        fclose(f);
    }
    
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

VintageOptoCompressor_Platinum::~VintageOptoCompressor_Platinum() {
    // DEBUG: Track instance destruction
    FILE* f = fopen("/tmp/opto_lifecycle.txt", "a");
    if (f) {
        fprintf(f, "VintageOptoCompressor DESTROYED: instance=%p\n", (void*)this);
        fclose(f);
    }
}

void VintageOptoCompressor_Platinum::prepareToPlay(double fs, int /*samplesPerBlock*/) {
    DBG("VintageOpto prepareToPlay called with fs=" + juce::String(fs));
    
    // Debug: Log to file to confirm prepareToPlay is called
    FILE* f = fopen("/tmp/opto_debug.txt", "a");
    if (f) {
        fprintf(f, "prepareToPlay called! fs=%.1f\n", fs);
        fclose(f);
    }
    
    sampleRate_ = std::max(8000.0, fs);
    const float ffs = (float) sampleRate_;

    // UI smoothers - DISABLED for instant response
    // Setting tau to 0.0001 gives nearly instant response
    pGain_.setTau(0.0001f, ffs);
    pPeakReduction_.setTau(0.0001f, ffs);
    pEmph_.setTau(0.0001f, ffs);
    pOut_.setTau(0.0001f, ffs);
    pMix_.setTau(0.0001f, ffs);
    pKnee_.setTau(0.0001f, ffs);
    pHarm_.setTau(0.0001f, ffs);
    pLink_.setTau(0.0001f, ffs);

    // sidechain: HP + LP (we'll blend for tilt)
    scHP_.set(120.0f, 0.707f, ffs);
    scLP_.set(6000.0f, 0.707f, ffs);

    // detector timing (will be re-mapped each block from params)
    envAtk_.setTau(0.005f, ffs);
    envRel_.setTau(0.200f, ffs);
    env_ = 0.0f;
    
    // Debug: Log envelope coefficients
    FILE* f2 = fopen("/tmp/opto_debug.txt", "a");
    if (f2) {
        fprintf(f2, "Envelope setup: atk.a=%.6f rel.a=%.6f\n", envAtk_.a, envRel_.a);
        fclose(f2);
    }

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
    // Debug: Write to file every time parameters change with STACK TRACE
    static int updateCounter = 0;
    static std::map<int, float> lastParams;
    
    bool changed = false;
    for (const auto& p : params) {
        if (lastParams[p.first] != p.second) {
            changed = true;
            lastParams[p.first] = p.second;
        }
    }
    
    if (changed || ++updateCounter % 100 == 0) {
        FILE* f = fopen("/tmp/opto_debug.txt", "a");
        if (f) {
            // Add instance address to identify WHICH VintageOpto this is
            fprintf(f, "VintageOpto [%p] params: ", (void*)this);
            for (const auto& p : params) {
                fprintf(f, "[%d]=%.3f ", p.first, p.second);
            }
            fprintf(f, "\n");
            fclose(f);
        }
    }
    
    auto set = [&](int idx, Smoothed& p, float def){
        auto it = params.find(idx);
        float newVal = it == params.end() ? def : clamp01(it->second);
        p.target.store(newVal, std::memory_order_relaxed);
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
    const float r   = juce::jmap(peakRed, 0.0f, 1.0f, 2.0f,  8.0f);          // 2:1 .. 8:1 (controlled by peak reduction)
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
    
    // Debug: Log GR calculation details occasionally
    static int grCalcCounter = 0;
    if (++grCalcCounter % 1000 == 0 && envLin > 0.001f) {
        FILE* f = fopen("/tmp/opto_dsp_debug.txt", "a");
        if (f) {
            fprintf(f, "GR calc: envLin=%.4f xDB=%.1f thr=%.1f over=%.1f ratio=%.1f grDB=%.1f\n",
                envLin, xDB, thr, over, r, grDB);
            fclose(f);
        }
    }

    return juce::jlimit(-48.0f, 0.0f, grDB);
}

void VintageOptoCompressor_Platinum::process(juce::AudioBuffer<float>& buffer) {
    // DEBUG: Log EVERY process call
    static int processCallCount = 0;
    if (++processCallCount % 100 == 0) {
        FILE* f = fopen("/tmp/opto_process_calls.txt", "a");
        if (f) {
            fprintf(f, "process() called #%d: channels=%d samples=%d instance=%p\n", 
                processCallCount, buffer.getNumChannels(), buffer.getNumSamples(), (void*)this);
            fclose(f);
        }
    }
    
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
    
    // Debug output every 100 blocks
    static int blockCounter = 0;
    if (++blockCounter % 100 == 0) {
        DBG("VintageOpto - inGain: " + juce::String(inGain) + 
            ", peakRed: " + juce::String(peakRed) + 
            ", mix: " + juce::String(mix));
        
        // Debug gains to file
        FILE* f = fopen("/tmp/opto_gains.txt", "a");
        if (f) {
            fprintf(f, "Gains: pGain=%.3f inGain=%.3f (%.1fdB) pOut=%.3f outGain=%.3f (%.1fdB)\n",
                pGain_.current, inGain, 20.0f*std::log10(inGain),
                pOut_.current, outGain, 20.0f*std::log10(outGain));
            fclose(f);
        }
    }

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

    // Debug: Track if we're actually processing audio
    static int processCounter = 0;
    static bool hasSignal = false;
    
    for (int i = 0; i < n; ++i) {
        float xL = Lr[i] * inGain;
        float xR = Rr[i] * inGain;
        
        // Check if we have signal
        if (std::abs(xL) > 0.001f || std::abs(xR) > 0.001f) {
            hasSignal = true;
        }

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
        // Pass peakRed directly - gainReductionDB will map it to ratio internally
        const float grDB = gainReductionDB(env_, peakRed, peakRed, kneeDB);
        const float smoothedGRDB = grSmooth_.process(grDB);
        const float grLin = fromDB(smoothedGRDB);
        
        // Debug: Log compression values every 100 samples if we have signal
        if (++processCounter % 100 == 0 && hasSignal) {
            FILE* f = fopen("/tmp/opto_dsp_debug.txt", "a");
            if (f) {
                fprintf(f, "DSP: xL=%.3f xR=%.3f det=%.3f env=%.3f grDB=%.3f grLin=%.3f a=%.5f\n",
                    xL, xR, det, env_, grDB, grLin, a);
                fclose(f);
            }
            hasSignal = false;
        }

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

        // final sanity - comprehensive NaN/Inf protection
        if (!std::isfinite(outL) || std::isnan(outL)) outL = 0.0f;
        if (!std::isfinite(outR) || std::isnan(outR)) outR = 0.0f;

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
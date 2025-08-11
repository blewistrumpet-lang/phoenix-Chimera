#include "ParametricEQ_Studio.h"
#include <cstring>

// ===================== Analyzer Ring =====================
size_t ParametricEQ_Studio::AnalyzerRing::snapshot(float* out, size_t cap) noexcept {
    if (!out || cap==0) return 0;
    const uint32_t wi = w.load(std::memory_order_acquire);
    size_t n = std::min(cap,(size_t)kAnalyzerSize);
    for (size_t i=0;i<n;++i) {
        const size_t idx = (wi + i) % kAnalyzerSize;
        out[i] = buf[idx];
    }
    return n;
}

// ===================== Coeff Calc & Mag ===================
void ParametricEQ_Studio::calcPeakingBiquad(BiquadCoeffs& out,double fs,double f,double Q,double gainDB){
    f = std::clamp(f,20.0,20000.0); Q = std::clamp(Q,0.1,20.0);
    const double A=std::pow(10.0,gainDB/40.0);
    const double w0=2.0*M_PI*(f/fs);
    const double c=std::cos(w0), s=std::sin(w0);
    const double alpha = std::max(1e-8, s/(2.0*Q));
    const double b0=1.0 + alpha*A, b1=-2.0*c, b2=1.0 - alpha*A;
    const double a0=1.0 + alpha/A, a1=-2.0*c, a2=1.0 - alpha/A;
    const double invA0 = 1.0/a0;
    out.b0=(float)(b0*invA0); out.b1=(float)(b1*invA0); out.b2=(float)(b2*invA0);
    out.a1=(float)(a1*invA0); out.a2=(float)(a2*invA0);
}

// |H(e^{jw})| for normalized direct-form with a0=1
float ParametricEQ_Studio::biquadMagAtW(const BiquadCoeffs& c, float w) {
    const float cw = std::cos(w), sw = std::sin(w);
    // z^{-1} = e^{-jw}
    // H(z) = (b0 + b1 z^{-1} + b2 z^{-2}) / (1 + a1 z^{-1} + a2 z^{-2})
    // Evaluate real/imag parts using cos/sin identities
    const float zr1r =  cw, zr1i = -sw;
    const float zr2r =  2*cw*cw - 1.0f;  // cos(2w)
    const float zr2i = -2*cw*sw;         // -sin(2w)
    auto numr = c.b0 + c.b1*zr1r + c.b2*zr2r;
    auto numi =        c.b1*zr1i + c.b2*zr2i;
    auto denr = 1.0f + c.a1*zr1r + c.a2*zr2r;
    auto deni =        c.a1*zr1i + c.a2*zr2i;
    const float num2 = numr*numr + numi*numi;
    const float den2 = denr*denr + deni*deni;
    const float mag  = std::sqrt(std::max(1e-20f, num2/den2));
    return mag;
}

// ===================== Lifecycle =========================
ParametricEQ_Studio::ParametricEQ_Studio(){}
ParametricEQ_Studio::~ParametricEQ_Studio(){}

void ParametricEQ_Studio::prepareToPlay(double sampleRate,int samplesPerBlock){
    fs_ = std::max(1.0, sampleRate);
    blockSize_ = std::max(1, samplesPerBlock);

    for (int ch=0; ch<kMaxChannels; ++ch) {
        for (int b=0; b<kMaxBands; ++b) paths_[ch][b].reset();
        dc_[ch].prepare(sampleRate);  // Initialize DC blocker
        activeCount_[ch] = 0;
    }
    preRing_.reset(); postRing_.reset(); hb_.reset();
    analyzerDecimCtr_ = 0; ctrlPhase_ = 0;

    const double initF[kMaxBands] = { 90.0, 250.0, 750.0, 2000.0, 6000.0, 12000.0 };
    for (int b=0; b<kMaxBands; ++b) {
        auto& B = bands_[b];
        B.tEnabled = 0.f; B.tFreq = (float)initF[b]; B.tGainDB = 0.f; B.tQ = 1.f;
        B.en.reset(B.tEnabled); B.f0.reset(B.tFreq); B.gdB.reset(B.tGainDB); B.q.reset(B.tQ);
        B.haveLast = false;
    }
}

void ParametricEQ_Studio::reset(){
    for (int ch=0; ch<kMaxChannels; ++ch) {
        for (int b=0; b<kMaxBands; ++b) paths_[ch][b].reset();
        dc_[ch].reset();
        activeCount_[ch] = 0;
    }
    preRing_.reset(); postRing_.reset(); hb_.reset();
}

// ===================== Params & Band MS ==================
void ParametricEQ_Studio::updateParameters(const std::map<int,float>& p){
    // Use helper function to safely get parameters
    auto getParam = [&p](int index, float defaultValue) {
        auto it = p.find(index);
        return it != p.end() ? it->second : defaultValue;
    };
    
    bypass_     = getParam(kGlobalBypass, 0.f) >= 0.5f;
    trim_       = std::clamp(getParam(kOutputTrim_dB, 0.f), -24.f, 24.f);
    wetDry_     = std::clamp(getParam(kWetDry, 1.f), 0.f, 1.f);
    vintageOn_  = getParam(kVintageOn, 0.f)  >= 0.5f;
    midSideOn_  = getParam(kMidSideOn, 0.f)  >= 0.5f;
    analyzerOn_ = getParam(kAnalyzerOn, 1.f) >= 0.5f;
    
    for (int b=0; b<kMaxBands; ++b) {
        const int base = kBandBase + b*4;
        auto& B = bands_[b];
        B.tEnabled = std::clamp(getParam(base+0, B.tEnabled), 0.f, 1.f);
        B.tFreq    = std::clamp(getParam(base+1, B.tFreq),   20.f, 20000.f);
        B.tGainDB  = std::clamp(getParam(base+2, B.tGainDB), -18.f, 18.f);
        B.tQ       = std::clamp(getParam(base+3, B.tQ),       0.1f, 20.f);
    }
}

juce::String ParametricEQ_Studio::getParameterName(int index) const {
    if (index < 6) {
        switch(index) {
            case 0: return "Bypass";
            case 1: return "Output Trim";
            case 2: return "Mix";
            case 3: return "Vintage";
            case 4: return "M/S Mode";
            case 5: return "Analyzer";
        }
    }
    
    // Band parameters
    if (index >= 6 && index < 30) {
        int paramIdx = index - 6;
        int band = paramIdx / 4;
        int param = paramIdx % 4;
        
        juce::String bandName = "Band " + juce::String(band + 1) + " ";
        switch(param) {
            case 0: return bandName + "Enable";
            case 1: return bandName + "Freq";
            case 2: return bandName + "Gain";
            case 3: return bandName + "Q";
        }
    }
    
    return "";
}

void ParametricEQ_Studio::setBandMSMode(int band,int mode){
    if (band<0 || band>=kMaxBands) return;
    mode = std::clamp(mode,0,2);
    for (int ch=0; ch<kMaxChannels; ++ch) paths_[ch][band].msMode = mode;
}

// ===================== Control Tick & Recalc ============
void ParametricEQ_Studio::controlTickAll() {
    const float dt = (float)kCtrlInterval / (float)fs_;
    // Smooth, compute coeffs, possibly arm crossfades, and rebuild active lists
    for (int b=0; b<kMaxBands; ++b) {
        auto& B = bands_[b];
        B.en.processToward(B.tEnabled, dt);
        B.f0.processToward(B.tFreq,    dt);
        B.gdB.processToward(B.tGainDB, dt);
        B.q.processToward(B.tQ,        dt);
    }
    for (int ch=0; ch<kMaxChannels; ++ch) {
        activeCount_[ch] = 0;
        for (int b=0; b<kMaxBands; ++b) recalcIfNeeded(ch,b);
        // rebuild compact active list
        for (int b=0; b<kMaxBands; ++b) if (paths_[ch][b].enabled) {
            activeIdx_[ch][activeCount_[ch]++] = b;
        }
    }
}

void ParametricEQ_Studio::recalcIfNeeded(int ch, int b) {
    auto& B = bands_[b];
    auto& path = paths_[ch][b];

    const bool en = (B.en.y >= 0.5f);
    path.enabled = en;

    // Compute fresh coeffs for smoothed values
    BiquadCoeffs C{};
    calcPeakingBiquad(C, fs_, (double)B.f0.y, (double)B.q.y, (double)B.gdB.y);

    // First time: set both sides
    if (!B.haveLast) {
        path.A.c = C; path.B.c = C;
        B.last   = C; B.haveLast = true;
        return;
    }

    auto delta = [](float a,float d){ return std::abs(a-d); };
    const bool big =
        delta(C.b0,B.last.b0)>1e-4f || delta(C.b1,B.last.b1)>1e-4f ||
        delta(C.b2,B.last.b2)>1e-4f || delta(C.a1,B.last.a1)>1e-4f ||
        delta(C.a2,B.last.a2)>1e-4f;

    if (big) {
        // Analytical power-comp: magnitude at band center
        const float w = (float)(2.0 * M_PI * (B.f0.y / (float)fs_));
        const float magNew = biquadMagAtW(C, w);
        const float magOld = biquadMagAtW(path.useA ? path.A.c : path.B.c, w);
        if (path.useA) { path.B.c = C; } else { path.A.c = C; }
        path.g.setFromMag(magOld, magNew);
        path.xfadeCtr = kXfadeSamples;
        path.useA     = !path.useA;
        B.last        = C;
    }
}

// ===================== Process ==========================
void ParametricEQ_Studio::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;

    const int nCh = std::min(buffer.getNumChannels(), kMaxChannels);
    const int N   = buffer.getNumSamples();
    if (N <= 0 || nCh <= 0) return;

    if (bypass_) {
        if (analyzerOn_) {
            const float* L = buffer.getReadPointer(0);
            for (int n=0;n<N;++n) if ((analyzerDecimCtr_++ % kAnalyzerDecim)==0) {
                preRing_.push(L[n]); postRing_.push(L[n]);
            }
        }
        return;
    }

    const float wet = wetDry_, dry = 1.f - wet, trim = dbToLin(trim_);

    // Optional MS encode (process entirely in MS if enabled)
    if (midSideOn_ && nCh >= 2) {
        float* L = buffer.getWritePointer(0);
        float* R = buffer.getWritePointer(1);
        for (int n=0;n<N;++n) { const float l=L[n], r=R[n]; L[n]=(l+r)*0.70710678f; R[n]=(l-r)*0.70710678f; }
    }

    // Analyzer pre (tap ch0/M)
    if (analyzerOn_) {
        const float* pre = buffer.getReadPointer(0);
        for (int n=0;n<N;++n) if ((analyzerDecimCtr_++ % kAnalyzerDecim)==0) preRing_.push(pre[n]);
    }

    // Process in chunks delimited by control ticks (no per-sample modulo)
    int pos = 0;
    while (pos < N) {
        int untilTick = (ctrlPhase_ > 0 ? ctrlPhase_ : kCtrlInterval);
        int run = std::min(untilTick, N - pos);
        ctrlPhase_ = untilTick - run;
        if (ctrlPhase_ == 0) { controlTickAll(); ctrlPhase_ = kCtrlInterval; }

        const bool needOS = vintageOn_ && fs_ < 96000.0;

        if (!needOS) {
            // Base-rate processing
            for (int ch=0; ch<nCh; ++ch) {
                float* x = buffer.getWritePointer(ch) + pos;
                const int nb = activeCount_[ch];
                for (int n=0; n<run; ++n) {
                    const float in = x[n]; float y = in;
                    for (int k=0; k<nb; ++k) {
                        const int b = activeIdx_[ch][k];
                        auto& P = paths_[ch][b];
                        if (midSideOn_ && nCh>=2) {
                            // Per-band MS routing respected in MS domain
                            if (P.msMode==1 && ch!=0) continue; // M-only
                            if (P.msMode==2 && ch!=1) continue; // S-only
                        }
                        y = P.process(y);
                    }
                    if (vintageOn_) y = vintageSaturate(y);
                    float out = dry*in + wet*y;
                    out = dc_[ch].process(out) * trim;
                    x[n] = out;
                }
            }
        } else {
            // 2× OS around the vintage saturator; EQ remains base-rate
            float* M = buffer.getWritePointer(0) + pos;
            float* S = (nCh>=2 ? buffer.getWritePointer(1) + pos : nullptr);

            for (int n=0; n<run; ++n) {
                float in0 = M[n];
                float in1 = (S ? S[n] : in0);

                // Apply EQ per channel (MS-aware band routing)
                float y0 = in0, y1 = in1;
                {
                    const int nb0 = activeCount_[0];
                    for (int k=0; k<nb0; ++k) {
                        const int b = activeIdx_[0][k];
                        auto& P = paths_[0][b];
                        if (midSideOn_ && nCh>=2) { // in MS domain: ch0=M
                            if (P.msMode==2) continue; // skip S-only on M
                        }
                        y0 = P.process(y0);
                    }
                    const int nb1 = (nCh>=2 ? activeCount_[1] : 0);
                    for (int k=0; k<nb1; ++k) {
                        const int b = activeIdx_[1][k];
                        auto& P = paths_[1][b];
                        if (midSideOn_ && nCh>=2) { // ch1=S
                            if (P.msMode==1) continue; // skip M-only on S
                        }
                        y1 = P.process(y1);
                    }
                }

                // Up -> saturate -> down (per sample)
                float eL,oL,eR,oR; hb_.upsample(y0, y1, eL,oL, eR,oR);
                eL = vintageSaturate(eL); oL = vintageSaturate(oL);
                eR = vintageSaturate(eR); oR = vintageSaturate(oR);
                float dL,dR; hb_.downsample(eL,oL, eR,oR, dL,dR);

                float out0 = dc_[0].process(dry*in0 + wet*dL) * trim;
                float out1 = dc_[1].process(dry*in1 + wet*dR) * trim;

                M[n] = out0;
                if (S) S[n] = out1; else M[n] = out0;
            }
        }

        pos += run;
    }

    // MS decode back to L/R if we encoded
    if (midSideOn_ && nCh >= 2) {
        float* M = buffer.getWritePointer(0);
        float* S = buffer.getWritePointer(1);
        for (int n=0;n<N;++n) {
            const float m=M[n], s=S[n];
            M[n]=(m+s)*0.70710678f; S[n]=(m-s)*0.70710678f;
        }
    }

    if (analyzerOn_) {
        const float* post = buffer.getReadPointer(0);
        for (int n=0;n<N;++n) if ((analyzerDecimCtr_++ % kAnalyzerDecim)==0) postRing_.push(post[n]);
    }

    scrubBuffer(buffer);
}

size_t ParametricEQ_Studio::getAnalyzerSnapshot(bool post,float* out,size_t cap){
    return (post?postRing_:preRing_).snapshot(out,cap);
}
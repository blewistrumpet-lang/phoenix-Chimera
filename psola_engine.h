#pragma once
#include <vector>
#include <deque>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <cstdio>

struct PsolaEpoch {
    int64_t nAbs;   // absolute sample index of GCI/pitch mark
    float   T0;     // local period in samples
    float   rms;    // local RMS near epoch
    bool    voiced;
};

class PsolaEngine {
public:
    void prepare(double fs, double histSeconds = 0.6) {
        fs_ = fs;
        const int want = nextPow2((int)std::ceil(histSeconds*fs_) + 8192);
        histSize_ = std::max(1<<16, want);
        histMask_ = histSize_ - 1;
        hist_.assign(histSize_, 0.0f);
        writeAbs_ = 0;
        epochs_.clear();
        synTimeAbs_ = 0.0;
        anaIdxF_    = 0.0f;
        lastT0_     = (float)(fs_/200.0);
        hannW_.clear(); w2sum_ = 1.0f; rmsEnv_ = 0.0f;

        refT0_ = 0.f; epochsVersion_ = 0; cachedVersion_ = -1;

        // phase-lock state
        prevWin_.clear(); prevE2_ = 0.f; havePrev_ = false;
    }

    // reset synthesis timeline (for offline or voice reset)
    void resetSynthesis(int64_t synStartAbs = 0) {
        synTimeAbs_ = (double)synStartAbs;
        anaIdxF_    = 0.0f;
        havePrev_   = false;
        prevE2_     = 0.f;
    }

    // write input into history ring
    void pushBlock(const float* x, int N) {
        for (int i=0;i<N;++i) hist_[wrap(writeAbs_ + i)] = x[i];
        writeAbs_ += N;
    }

    // map locally detected epochs to absolute time; computes per-epoch RMS and prunes old
    void appendEpochs(const std::vector<int>& local, int64_t localStartAbs, float T0, bool voiced) {
        const int64_t oldest = writeAbs_ - histSize_;
        const int rmsHalf = std::max(1, (int)std::round(0.5f*std::max(16.f, T0)));
        for (int e : local) {
            int64_t nAbs = localStartAbs + e;
            if (nAbs <= oldest) continue;
            if (!epochs_.empty() && std::llabs(epochs_.back().nAbs - nAbs) < (int)(0.3f*T0))
                continue;

            double e2=0.0; int cnt=0;
            for (int i=-rmsHalf;i<=rmsHalf;++i) {
                int64_t idx = nAbs + i;
                if (idx<oldest || idx>=writeAbs_) continue;
                float s = hist_[wrap(idx)];
                e2 += (double)s*s; ++cnt;
            }
            float rms=(cnt>0)? std::sqrt((float)(e2/cnt)) : 0.f;
            epochs_.push_back({nAbs, std::max(16.f,T0), rms, voiced});
        }
        const int64_t keepFrom = writeAbs_ - histSize_ + 8192;
        while(!epochs_.empty() && epochs_.front().nAbs < keepFrom) epochs_.pop_front();

        // change version so we refresh refT0 on next render
        epochsVersion_++;
    }

    // alpha = f0_target / f0_source ; >1 => pitch up
    // outStartAbs: absolute start of THIS output block (lets you render from 0…N offline)
    void renderBlock(float alpha, float* out, int outN, int64_t outStartAbs = -1) {
        std::fill(out, out+outN, 0.f);
        if (epochs_.size() < 4 || !(alpha>0.f) || !std::isfinite(alpha)) return;

        if (outStartAbs < 0) outStartAbs = writeAbs_ - outN;
        if (synTimeAbs_ < (double)outStartAbs) synTimeAbs_ = (double)outStartAbs;

        // --- stable ref period (median of diffs) ---
        if (cachedVersion_ != epochsVersion_ || refT0_ <= 0.f) {
            refT0_ = computeRefT0_();
            cachedVersion_ = epochsVersion_;
            if (refT0_ <= 0.f) refT0_ = lastT0_;
        }

        // synthesis hop strictly follows alpha using refT0
        const float synHop = refT0_ / std::max(1e-6f, alpha);
        const int   Lref   = std::max(32, makeOdd((int)std::round(2.5f * refT0_)));
        ensureHann(Lref);
        const int   half   = Lref/2;

        // micro-WSOLA search window (± ~0.05·T0 for fine alignment only)
        const int searchHalf = std::max(1, std::min(half-2, (int)std::round(0.05f * refT0_)));

        // buffers for phase-locking
        if ((int)prevWin_.size() != Lref) { prevWin_.assign(Lref, 0.f); havePrev_ = false; prevE2_ = 0.f; }
        std::vector<float> curWin(Lref, 0.f);

        const double blockEndAbs = (double)outStartAbs + outN;

        while (synTimeAbs_ < blockEndAbs + 0.5*refT0_) {
            // 1) Analysis index grows by 1/α (repeat/skip epochs as needed)
            float kF = anaIdxF_;

            // 2) **Nearest epoch** (NO time interpolation of epoch positions)
            int kNear = (int)std::lround(kF);
            if (kNear < 0) kNear = 0;
            if (kNear > (int)epochs_.size()-1) kNear = (int)epochs_.size()-1;
            int64_t centerAbs = epochs_[kNear].nAbs;
            
            // Debug epoch selection for problematic ratios
            static int debugGrain = 0;
            if (alpha > 0.7 && alpha < 0.8 && (debugGrain++ % 10) == 0) {
                std::fprintf(stderr, "α=%.3f: kF=%.2f → kNear=%d, epochPos=%lld, synTime=%.1f\n",
                            alpha, kF, kNear, centerAbs, synTimeAbs_);
            }

            // 3) micro-WSOLA: **tiny** ±0.05·T0 search to maximize correlation with previous grain
            int bestShift = 0;
            if (havePrev_ && prevE2_ > 1e-8f) {
                float bestScore = -1e30f;
                for (int d = -searchHalf; d <= searchHalf; ++d) {
                    double dot = 0.0, e2 = 0.0;
                    for (int i=0;i<Lref;++i) {
                        int64_t idx = centerAbs + d + i - half;
                        if (idx < writeAbs_ - histSize_ || idx >= writeAbs_) continue;
                        float s = hannW_[i] * hist_[wrap(idx)];
                        dot += (double)s * (double)prevWin_[i];
                        e2  += (double)s * (double)s;
                    }
                    float norm = (float)std::sqrt(std::max(1e-12, e2 * (double)prevE2_));
                    float score = (norm > 1e-9f) ? (float)(dot / norm) : 0.f;
                    if (score > bestScore) { bestScore = score; bestShift = d; }
                }
            }
            const int64_t alignedCenterAbs = centerAbs + bestShift;

            // build current windowed grain, compute energy and polarity vs previous
            double e2 = 0.0, dotPrev = 0.0;
            for (int i=0;i<Lref;++i) {
                int64_t idx = alignedCenterAbs + i - half;
                float s = (idx < writeAbs_ - histSize_ || idx >= writeAbs_) ? 0.f : hist_[wrap(idx)];
                float v = hannW_[i] * s;
                curWin[i] = v;
                e2 += (double)v*v;
                if (havePrev_) dotPrev += (double)v * (double)prevWin_[i];
            }
            const float curE2 = (float)e2 + 1e-12f;

            // 5) Polarity guard to avoid F0/2 and F0/3 traps
            float sgn = 1.f;
            if (havePrev_ && dotPrev < 0.0) sgn = -1.f;

            // energy equalization and density compensation
            const float curRms = std::sqrt(curE2 / w2sum_);
            rmsEnv_ = 0.995f*rmsEnv_ + 0.005f*curRms;

            const float overlap = (float)Lref / std::max(1e-6f, synHop);
            const float g_ola   = std::sqrt(std::max(1e-6f, 1.0f / overlap));
            const float g_eq    = (curRms>1e-9f) ? (rmsEnv_ / curRms) : 1.f;
            const float g       = g_eq * g_ola * sgn;

            // OLA write
            const int synC = (int)std::llround(synTimeAbs_);
            for (int i=0;i<Lref;++i) {
                const int nsAbs = synC + i - half;
                const int rel   = (int)(nsAbs - outStartAbs);
                if (rel<0 || rel>=outN) continue;
                out[rel] += g * curWin[i];
            }

            // update phase-lock reference
            prevWin_ = curWin;
            prevE2_  = curE2;
            havePrev_ = true;

            // advance synthesis & analysis
            synTimeAbs_ += synHop;
            anaIdxF_    += 1.0f / std::max(1e-6f, alpha);
            if (anaIdxF_ > (float)epochs_.size()-1.5f) anaIdxF_ = (float)epochs_.size()-1.5f;

            lastT0_ = refT0_;
        }
    }

    // utilities
    int64_t writeCursorAbs() const { return writeAbs_; }
    const std::deque<PsolaEpoch>& epochs() const { return epochs_; }

private:
    // ring buffer
    double fs_ = 48000.0;
    int histSize_ = 0, histMask_ = 0;
    std::vector<float> hist_;
    int64_t writeAbs_ = 0;

    // epochs
    std::deque<PsolaEpoch> epochs_;
    int32_t epochsVersion_ = 0;
    int32_t cachedVersion_ = -1;

    // synthesis state
    double synTimeAbs_ = 0.0;
    float  anaIdxF_ = 0.0f;
    float  lastT0_  = 120.f;
    float  refT0_   = 0.f;

    // window + EQ
    std::vector<float> hannW_; float w2sum_ = 1.f; float rmsEnv_ = 0.f;

    // phase-lock
    std::vector<float> prevWin_;
    float prevE2_ = 0.f;
    bool  havePrev_ = false;

    // helpers
    static int nextPow2(int x){ int p=1; while(p<x) p<<=1; return p; }
    inline int wrap(int64_t abs) const { return (int)(abs & histMask_); }
    static int makeOdd(int L){ return (L&1)? L : (L+1); }
    void ensureHann(int L){
        if ((int)hannW_.size()==L) return;
        hannW_.assign(L,0.f);
        for (int i=0;i<L;++i) hannW_[i]=0.5f*(1.f-std::cos(2.f*float(M_PI)*i/(L-1)));
        w2sum_=0.f; for(float w: hannW_) w2sum_+=w*w; if (w2sum_<1e-9f) w2sum_=1.f;
    }
    int clampIndex(int k) const {
        if (epochs_.empty()) return -1;
        if (k<0) return 0;
        if (k>=(int)epochs_.size()-1) return (int)epochs_.size()-1;
        return k;
    }
    float computeRefT0_() const {
        if (epochs_.size()<3) return lastT0_;
        std::vector<float> diffs; diffs.reserve(epochs_.size());
        for (size_t i=1;i<epochs_.size();++i){
            int64_t d = epochs_[i].nAbs - epochs_[i-1].nAbs;
            if (d>16 && d< (int64_t)(0.03*fs_)) diffs.push_back((float)d);
        }
        if (diffs.empty()) return lastT0_;
        std::nth_element(diffs.begin(), diffs.begin()+diffs.size()/2, diffs.end());
        return diffs[diffs.size()/2];
    }
};
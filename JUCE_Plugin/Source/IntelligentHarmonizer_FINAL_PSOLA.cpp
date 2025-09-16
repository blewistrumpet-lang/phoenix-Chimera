// IntelligentHarmonizer_FINAL_PSOLA.cpp
// Complete TD-PSOLA implementation with surgical fixes for irrational ratios
// Includes integer schedule, variable windows, and core WSOLA

#include "IntelligentHarmonizer.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <deque>
#include <atomic>

namespace {

// Utilities
template<typename T>
inline T flushDenorm(T v) noexcept {
    constexpr T tiny = static_cast<T>(1.0e-38);
    return std::fabs(v) < tiny ? static_cast<T>(0) : v;
}

inline int nextPow2(int x) {
    int p = 1;
    while (p < x) p <<= 1;
    return p;
}

// Parameter smoothing
class SmoothedParam {
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float coeff{0.9995f};
    
public:
    void setSmoothingTime(float timeMs, double sampleRate) noexcept {
        float samples = timeMs * 0.001f * sampleRate;
        coeff = std::exp(-1.0f / samples);
    }
    
    void set(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
    }
    
    void snap(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
        current = value;
    }
    
    float tick() noexcept {
        float t = target.load(std::memory_order_relaxed);
        current = t + coeff * (current - t);
        return current;
    }
    
    float get() const noexcept { return current; }
};

// Epoch structure
struct PsolaEpoch {
    int64_t nAbs;   // absolute sample index
    float   T0;     // local period in samples
    float   rms;    // local RMS near epoch
    bool    voiced;
};

// Complete PSOLA Engine with surgical fixes
class PsolaEngineFinal {
public:
    void prepare(double fs, double histSeconds = 0.6) {
        fs_ = fs;
        const int want = nextPow2((int)std::ceil(histSeconds*fs) + 8192);
        histSize_ = std::max(1<<16, want);
        histMask_ = histSize_ - 1;
        hist_.assign(histSize_, 0.0f);
        writeAbs_ = 0;
        epochs_.clear();
        synTimeAbs_ = 0.0;
        
        // Integer schedule state
        kInt_ = 0;
        acc_ = 0.0f;
        
        lastT0_ = (float)(fs/200.0);
        prevWin_.clear(); 
        prevE2_ = 0.0f; 
        havePrev_ = false;
        rmsEnv_ = 0.0f;
        
        refT0_ = 0.f; 
        epochsVersion_ = 0; 
        cachedVersion_ = -1;
    }

    void resetSynthesis(int64_t synStartAbs = 0) {
        synTimeAbs_ = (double)synStartAbs;
        kInt_ = 0;
        acc_ = 0.0f;
        havePrev_ = false;
        prevE2_ = 0.f;
    }

    void pushBlock(const float* x, int N) {
        for (int i=0;i<N;++i) hist_[wrap(writeAbs_ + i)] = x[i];
        writeAbs_ += N;
    }

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

        epochsVersion_++;
    }

    void renderBlock(float alpha, float* out, int outN, int64_t outStartAbs = -1) {
        std::fill(out, out+outN, 0.f);
        if (epochs_.size() < 4 || !(alpha>0.f) || !std::isfinite(alpha)) return;

        if (outStartAbs < 0) outStartAbs = writeAbs_ - outN;
        if (synTimeAbs_ < (double)outStartAbs) synTimeAbs_ = (double)outStartAbs;

        // Stable reference period
        if (cachedVersion_ != epochsVersion_ || refT0_ <= 0.f) {
            refT0_ = computeRefT0_();
            cachedVersion_ = epochsVersion_;
            if (refT0_ <= 0.f) refT0_ = lastT0_;
        }

        const float synHop = refT0_ / std::max(1e-6f, alpha);
        const double blockEndAbs = (double)outStartAbs + outN;

        // Core size for alignment (60% of period)
        const int core = std::max(16, (int)std::round(0.60f * refT0_));
        const int coreHalf = core / 2;
        
        // Search window for micro-WSOLA (±10% of period)
        const int searchHalf = std::max(1, (int)std::round(0.10f * refT0_));

        while (synTimeAbs_ < blockEndAbs + 0.5*refT0_) {
            // FIX 1: Integer epoch schedule (Bresenham-style)
            const float invA = 1.0f / std::max(1e-6f, alpha);
            acc_ += invA;
            int step = (int)acc_;
            acc_ -= step;
            step = std::max(1, step);
            kInt_ += step;
            
            kInt_ = clampIndex(kInt_);
            const int kNear = kInt_;
            
            if (kNear < 0 || kNear >= (int)epochs_.size()) {
                synTimeAbs_ += synHop;
                continue;
            }
            
            const int64_t centerAbs = epochs_[kNear].nAbs;
            
            // FIX 2: Pitch-synchronous variable windows (midpoint-to-midpoint)
            auto midpoint = [](int64_t a, int64_t b) { 
                return (int64_t)std::llround(0.5*(double(a)+double(b))); 
            };
            
            int km1 = clampIndex(kNear - 1);
            int kp1 = clampIndex(kNear + 1);
            
            int64_t Lb = (km1 >= 0) ? midpoint(epochs_[km1].nAbs, epochs_[kNear].nAbs) 
                                     : epochs_[kNear].nAbs - (int64_t)refT0_;
            int64_t Rb = (kp1 < (int)epochs_.size()) ? midpoint(epochs_[kNear].nAbs, epochs_[kp1].nAbs)
                                                      : epochs_[kNear].nAbs + (int64_t)refT0_;
            
            int Lk = (int)(Rb - Lb);
            if (Lk < 32) Lk = 32;
            if ((Lk & 1) == 0) ++Lk;  // make odd
            const int half = Lk / 2;
            
            // Build Hann window for this grain
            std::vector<float> w(Lk);
            double w2sum = 0.0;
            for (int i = 0; i < Lk; ++i) {
                float s = 0.5f * (1.f - std::cos(2.f * float(M_PI) * i / (Lk - 1)));
                w[i] = s;
                w2sum += s * s;
            }
            if (w2sum < 1e-9) w2sum = 1.0;
            
            // Resize prevWin_ if needed
            if ((int)prevWin_.size() != Lk) {
                prevWin_.resize(Lk, 0.0f);
                if (!havePrev_) prevE2_ = 0.0f;
            }
            
            // FIX 3: Core-focused micro-WSOLA with penalty
            int bestShift = 0;
            float bestScore = -1e30f;
            
            if (havePrev_ && prevE2_ > 1e-8f) {
                for (int d = -searchHalf; d <= searchHalf; ++d) {
                    double dot = 0.0, e2 = 0.0;
                    
                    // Only correlate the core region
                    for (int i = -coreHalf; i <= coreHalf; ++i) {
                        int wi = half + i;
                        if (wi < 0 || wi >= Lk) continue;
                        
                        int64_t idx = centerAbs + d + i;
                        if (idx < writeAbs_ - histSize_ || idx >= writeAbs_) continue;
                        
                        float v = w[wi] * hist_[wrap(idx)];
                        dot += (double)v * (double)prevWin_[wi];
                        e2 += (double)v * (double)v;
                    }
                    
                    float norm = (float)std::sqrt(std::max(1e-12, e2 * (double)prevE2_));
                    // Penalty discourages large shifts
                    float score = ((norm > 1e-9f) ? (float)(dot/norm) : 0.f) - 0.002f * std::abs((float)d);
                    
                    if (score > bestScore) {
                        bestScore = score;
                        bestShift = d;
                    }
                }
            }
            
            const int64_t alignedCenterAbs = centerAbs + bestShift;
            
            // Build current grain with full window
            double e2 = 0.0, coreDot = 0.0;
            std::vector<float> cur(Lk);
            for (int i = 0; i < Lk; ++i) {
                int64_t idx = alignedCenterAbs + i - half;
                float s = (idx < writeAbs_ - histSize_ || idx >= writeAbs_) ? 0.f : hist_[wrap(idx)];
                float v = w[i] * s;
                cur[i] = v;
                e2 += (double)v * (double)v;
            }
            
            // Polarity check on core only
            if (havePrev_) {
                for (int i = -coreHalf; i <= coreHalf; ++i) {
                    int wi = half + i;
                    if (wi < 0 || wi >= Lk) continue;
                    coreDot += (double)cur[wi] * (double)prevWin_[wi];
                }
            }
            float sgn = (havePrev_ && coreDot < 0.0) ? -1.f : 1.f;
            
            // Energy equalization and density compensation
            float curE2 = (float)e2 + 1e-12f;
            float curRms = std::sqrt(curE2 / (float)w2sum);
            rmsEnv_ = 0.995f * rmsEnv_ + 0.005f * curRms;
            
            const float overlap = (float)Lk / std::max(1e-6f, synHop);
            const float g = sgn
                          * ((curRms > 1e-9f) ? (rmsEnv_ / curRms) : 1.f)
                          * std::sqrt(std::max(1e-6f, 1.0f / overlap));
            
            // OLA write
            const int synC = (int)std::llround(synTimeAbs_);
            for (int i = 0; i < Lk; ++i) {
                const int nsAbs = synC + i - half;
                const int rel = (int)(nsAbs - outStartAbs);
                if (rel < 0 || rel >= outN) continue;
                out[rel] += g * cur[i];
            }
            
            // Update phase-lock reference
            prevWin_ = cur;
            prevE2_ = curE2;
            havePrev_ = true;
            
            // Advance synthesis time
            synTimeAbs_ += synHop;
            lastT0_ = refT0_;
        }
    }

    int64_t writeCursorAbs() const { return writeAbs_; }

private:
    double fs_ = 48000.0;
    int histSize_ = 0, histMask_ = 0;
    std::vector<float> hist_;
    int64_t writeAbs_ = 0;

    std::deque<PsolaEpoch> epochs_;
    int32_t epochsVersion_ = 0;
    int32_t cachedVersion_ = -1;

    double synTimeAbs_ = 0.0;
    
    // Integer schedule state
    int kInt_ = 0;
    float acc_ = 0.0f;
    
    float lastT0_ = 120.f;
    float refT0_ = 0.f;

    // Phase-lock state
    std::vector<float> prevWin_;
    float prevE2_ = 0.f;
    bool havePrev_ = false;
    
    float rmsEnv_ = 0.f;

    static int nextPow2(int x){ int p=1; while(p<x) p<<=1; return p; }
    inline int wrap(int64_t abs) const { return (int)(abs & histMask_); }
    
    int clampIndex(int k) const {
        if (epochs_.empty()) return -1;
        if (k < 0) return 0;
        if (k >= (int)epochs_.size()) return (int)epochs_.size() - 1;
        return k;
    }
    
    float computeRefT0_() const {
        if (epochs_.size() < 3) return lastT0_;
        std::vector<float> diffs;
        diffs.reserve(epochs_.size());
        for (size_t i = 1; i < epochs_.size(); ++i) {
            int64_t d = epochs_[i].nAbs - epochs_[i-1].nAbs;
            if (d > 16 && d < (int64_t)(0.03*fs_)) diffs.push_back((float)d);
        }
        if (diffs.empty()) return lastT0_;
        std::nth_element(diffs.begin(), diffs.begin() + diffs.size()/2, diffs.end());
        return diffs[diffs.size()/2];
    }
};

// Simple pitch detector for epoch marking
class SimplePitchDetector {
    float lastPeriod_ = 100.0f;
    
public:
    std::vector<int> findEpochs(const float* input, int numSamples, float sampleRate) {
        std::vector<int> marks;
        
        // Simple peak detection
        int minDist = (int)(sampleRate / 800);  // Max 800Hz
        int lastMark = -minDist;
        
        for (int i = 1; i < numSamples - 1; ++i) {
            if (i - lastMark < minDist) continue;
            
            // Local maximum
            if (input[i] > input[i-1] && input[i] > input[i+1] && input[i] > 0.01f) {
                marks.push_back(i);
                lastMark = i;
            }
        }
        
        // Update period estimate
        if (marks.size() >= 2) {
            float avgPeriod = 0;
            for (size_t i = 1; i < marks.size(); ++i) {
                avgPeriod += marks[i] - marks[i-1];
            }
            avgPeriod /= (marks.size() - 1);
            lastPeriod_ = avgPeriod;
        }
        
        return marks;
    }
    
    float getLastPeriod() const { return lastPeriod_; }
};

} // namespace

// ==================== Main IntelligentHarmonizer Class ====================
class IntelligentHarmonizer::Impl {
public:
    PsolaEngineFinal psolaEngine_;
    SimplePitchDetector pitchDetector_;
    
    // Parameters
    SmoothedParam pitchRatio_;
    SmoothedParam mix_;
    SmoothedParam formantShift_;
    
    int scaleIndex_ = 9; // Chromatic by default
    double sampleRate_ = 48000.0;
    int64_t processedSamples_ = 0;
    
    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        psolaEngine_.prepare(sampleRate, 0.6);
        
        pitchRatio_.setSmoothingTime(10.0f, sampleRate);
        mix_.setSmoothingTime(10.0f, sampleRate);
        formantShift_.setSmoothingTime(10.0f, sampleRate);
        
        pitchRatio_.snap(1.0f);
        mix_.snap(1.0f);
        formantShift_.snap(0.0f);
        
        processedSamples_ = 0;
    }
    
    void processBlock(const float* input, float* output, int numSamples) {
        // Push input to PSOLA engine
        psolaEngine_.pushBlock(input, numSamples);
        
        // Detect epochs
        auto marks = pitchDetector_.findEpochs(input, numSamples, sampleRate_);
        if (!marks.empty()) {
            float period = pitchDetector_.getLastPeriod();
            psolaEngine_.appendEpochs(marks, processedSamples_, period, true);
        }
        
        // Get current pitch ratio
        float ratio = pitchRatio_.tick();
        
        // Render PSOLA output
        psolaEngine_.renderBlock(ratio, output, numSamples, processedSamples_);
        
        // Apply mix
        float mixVal = mix_.tick();
        if (mixVal < 0.999f) {
            for (int i = 0; i < numSamples; ++i) {
                output[i] = input[i] * (1.0f - mixVal) + output[i] * mixVal;
            }
        }
        
        // Apply output limiting
        for (int i = 0; i < numSamples; ++i) {
            output[i] = std::max(-1.0f, std::min(1.0f, output[i]));
            output[i] = flushDenorm(output[i]);
        }
        
        processedSamples_ += numSamples;
    }
    
    void setPitchRatio(float ratio) { pitchRatio_.set(ratio); }
    void setMix(float mix) { mix_.set(mix); }
    void setFormantShift(float shift) { formantShift_.set(shift); }
    void setScaleIndex(int index) { scaleIndex_ = index; }
    
    void snapParameters(float ratio, float mix) {
        pitchRatio_.snap(ratio);
        mix_.snap(mix);
    }
};

// ==================== Public Interface ====================
IntelligentHarmonizer::IntelligentHarmonizer() : pImpl(std::make_unique<Impl>()) {}
IntelligentHarmonizer::~IntelligentHarmonizer() = default;

void IntelligentHarmonizer::prepare(double sampleRate, int samplesPerBlock) {
    pImpl->prepare(sampleRate);
}

void IntelligentHarmonizer::processBlock(const float* input, float* output, int numSamples) {
    pImpl->processBlock(input, output, numSamples);
}

void IntelligentHarmonizer::setPitchRatio(float ratio) {
    pImpl->setPitchRatio(ratio);
}

void IntelligentHarmonizer::setMix(float mix) {
    pImpl->setMix(mix);
}

void IntelligentHarmonizer::setFormantShift(float shift) {
    pImpl->setFormantShift(shift);
}

void IntelligentHarmonizer::setScaleIndex(int index) {
    pImpl->setScaleIndex(index);
}

void IntelligentHarmonizer::snapParameters(float ratio, float mix) {
    pImpl->snapParameters(ratio, mix);
}

float IntelligentHarmonizer::getPitchRatio() const {
    return pImpl->pitchRatio_.get();
}

float IntelligentHarmonizer::getMix() const {
    return pImpl->mix_.get();
}
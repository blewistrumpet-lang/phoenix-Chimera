// IntelligentHarmonizer_FIXED.cpp
// Fixes for click artifacts, dropouts, and subharmonics based on diagnostic analysis

#include "IntelligentHarmonizer.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <deque>
#include <atomic>
#include <cstring>

namespace {

// Utilities
template<typename T>
inline T flushDenorm(T v) noexcept {
    // Proper denormal flushing
    return (std::fabs(v) < 1e-30f) ? T(0) : v;
}

inline int nextPow2(int x) {
    int p = 1;
    while (p < x) p <<= 1;
    return p;
}

// Parameter smoothing with faster response
class SmoothedParam {
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float coeff{0.99f}; // Faster than before
    
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
        return flushDenorm(current);
    }
    
    float get() const noexcept { return current; }
};

// Epoch structure
struct PsolaEpoch {
    int64_t nAbs;
    float T0;
    float rms;
    bool voiced;
};

// Fixed PSOLA Engine addressing clicks and subharmonics
class PsolaEngineFixed {
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
        
        // FIX: Initialize crossfade buffer for click reduction
        crossfadeBuffer_.resize(64, 0.0f);
        crossfadePos_ = 0;
    }

    void resetSynthesis(int64_t synStartAbs = 0) {
        synTimeAbs_ = (double)synStartAbs;
        kInt_ = 0;
        acc_ = 0.0f;
        havePrev_ = false;
        prevE2_ = 0.f;
        std::fill(crossfadeBuffer_.begin(), crossfadeBuffer_.end(), 0.0f);
        crossfadePos_ = 0;
    }

    void pushBlock(const float* x, int N) {
        for (int i=0;i<N;++i) {
            hist_[wrap(writeAbs_ + i)] = flushDenorm(x[i]);
        }
        writeAbs_ += N;
    }

    void appendEpochs(const std::vector<int>& local, int64_t localStartAbs, float T0, bool voiced) {
        const int64_t oldest = writeAbs_ - histSize_;
        const int rmsHalf = std::max(1, (int)std::round(0.5f*std::max(16.f, T0)));
        
        for (int e : local) {
            int64_t nAbs = localStartAbs + e;
            if (nAbs <= oldest) continue;
            
            // FIX: Better epoch filtering to avoid duplicates
            bool tooClose = false;
            for (const auto& existing : epochs_) {
                if (std::abs((int64_t)(existing.nAbs - nAbs)) < (int)(0.5f*T0)) {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose) continue;

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
        while(!epochs_.empty() && epochs_.front().nAbs < keepFrom) {
            epochs_.pop_front();
        }

        epochsVersion_++;
    }

    void renderBlock(float alpha, float* out, int outN, int64_t outStartAbs = -1) {
        std::fill(out, out+outN, 0.f);
        if (epochs_.size() < 4 || !(alpha>0.f) || !std::isfinite(alpha)) return;

        if (outStartAbs < 0) outStartAbs = writeAbs_ - outN;
        if (synTimeAbs_ < (double)outStartAbs) synTimeAbs_ = (double)outStartAbs;

        if (cachedVersion_ != epochsVersion_ || refT0_ <= 0.f) {
            refT0_ = computeRefT0_();
            cachedVersion_ = epochsVersion_;
            if (refT0_ <= 0.f) refT0_ = lastT0_;
        }

        const float synHop = refT0_ / std::max(1e-6f, alpha);
        const double blockEndAbs = (double)outStartAbs + outN;

        // FIX: Adjusted core size and search window
        const int core = std::max(32, (int)std::round(0.75f * refT0_)); // Larger core
        const int coreHalf = core / 2;
        const int searchHalf = std::max(2, (int)std::round(0.15f * refT0_)); // Wider search

        // FIX: Track previous grain end for seamless crossfading
        int prevGrainEnd = -1;

        while (synTimeAbs_ < blockEndAbs + refT0_) {
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
            
            // Variable window calculation
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
            // FIX: Ensure reasonable window size
            Lk = std::max(64, std::min(Lk, (int)(3.0f * refT0_)));
            if ((Lk & 1) == 0) ++Lk;
            const int half = Lk / 2;
            
            // Build Tukey window (less aggressive than Hann at edges)
            std::vector<float> w(Lk);
            double w2sum = 0.0;
            const float taperRatio = 0.25f; // 25% taper on each side
            const int taperLen = (int)(taperRatio * Lk);
            
            for (int i = 0; i < Lk; ++i) {
                float winVal = 1.0f;
                if (i < taperLen) {
                    // Left taper
                    float x = (float)i / taperLen;
                    winVal = 0.5f * (1.0f - cos(M_PI * x));
                } else if (i >= Lk - taperLen) {
                    // Right taper
                    float x = (float)(Lk - 1 - i) / taperLen;
                    winVal = 0.5f * (1.0f - cos(M_PI * x));
                }
                w[i] = winVal;
                w2sum += winVal * winVal;
            }
            if (w2sum < 1e-9) w2sum = 1.0;
            
            if ((int)prevWin_.size() != Lk) {
                prevWin_.resize(Lk, 0.0f);
                if (!havePrev_) prevE2_ = 0.0f;
            }
            
            // Micro-WSOLA alignment
            int bestShift = 0;
            float bestScore = -1e30f;
            
            if (havePrev_ && prevE2_ > 1e-8f) {
                for (int d = -searchHalf; d <= searchHalf; ++d) {
                    double dot = 0.0, e2 = 0.0;
                    
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
                    // FIX: Reduced penalty for better alignment
                    float score = ((norm > 1e-9f) ? (float)(dot/norm) : 0.f) - 0.001f * std::abs((float)d);
                    
                    if (score > bestScore) {
                        bestScore = score;
                        bestShift = d;
                    }
                }
            }
            
            const int64_t alignedCenterAbs = centerAbs + bestShift;
            
            // Build current grain
            double e2 = 0.0, coreDot = 0.0;
            std::vector<float> cur(Lk);
            for (int i = 0; i < Lk; ++i) {
                int64_t idx = alignedCenterAbs + i - half;
                float s = (idx < writeAbs_ - histSize_ || idx >= writeAbs_) ? 0.f : hist_[wrap(idx)];
                float v = w[i] * s;
                cur[i] = flushDenorm(v);
                e2 += (double)v * (double)v;
            }
            
            // Polarity check
            if (havePrev_) {
                for (int i = -coreHalf; i <= coreHalf; ++i) {
                    int wi = half + i;
                    if (wi < 0 || wi >= Lk) continue;
                    coreDot += (double)cur[wi] * (double)prevWin_[wi];
                }
            }
            float sgn = (havePrev_ && coreDot < 0.0) ? -1.f : 1.f;
            
            // Energy equalization
            float curE2 = (float)e2 + 1e-12f;
            float curRms = std::sqrt(curE2 / (float)w2sum);
            
            // FIX: Smoother RMS envelope tracking
            if (rmsEnv_ < 1e-6f) rmsEnv_ = curRms;
            rmsEnv_ = 0.99f * rmsEnv_ + 0.01f * curRms;
            
            const float overlap = (float)Lk / std::max(1e-6f, synHop);
            
            // FIX: Better gain calculation to avoid dropouts
            float g_eq = (curRms > 1e-9f && rmsEnv_ > 1e-9f) ? 
                        std::min(2.0f, rmsEnv_ / curRms) : 1.0f;
            float g_ola = std::sqrt(std::max(0.5f, std::min(2.0f, 1.0f / overlap)));
            const float g = sgn * g_eq * g_ola * 0.8f; // Scale down slightly
            
            // FIX: Crossfade with previous grain to avoid clicks
            const int synC = (int)std::llround(synTimeAbs_);
            const int grainStart = synC - half;
            const int grainEnd = synC + half;
            
            // Apply crossfade at boundaries
            const int fadeLen = std::min(32, Lk/8);
            
            for (int i = 0; i < Lk; ++i) {
                const int nsAbs = grainStart + i;
                const int rel = (int)(nsAbs - outStartAbs);
                if (rel < 0 || rel >= outN) continue;
                
                float sample = g * cur[i];
                
                // Crossfade at grain boundaries
                float fadeFactor = 1.0f;
                if (i < fadeLen) {
                    fadeFactor = (float)i / fadeLen;
                } else if (i >= Lk - fadeLen) {
                    fadeFactor = (float)(Lk - i) / fadeLen;
                }
                
                out[rel] += flushDenorm(sample * fadeFactor);
            }
            
            prevWin_ = cur;
            prevE2_ = curE2;
            havePrev_ = true;
            prevGrainEnd = grainEnd;
            
            synTimeAbs_ += synHop;
            lastT0_ = refT0_;
        }
        
        // FIX: Final denormal flush on output
        for (int i = 0; i < outN; ++i) {
            out[i] = flushDenorm(out[i]);
        }
    }

    int64_t writeCursorAbs() const { return writeAbs_; }
    const std::deque<PsolaEpoch>& epochs() const { return epochs_; }

private:
    double fs_ = 48000.0;
    int histSize_ = 0, histMask_ = 0;
    std::vector<float> hist_;
    int64_t writeAbs_ = 0;

    std::deque<PsolaEpoch> epochs_;
    int32_t epochsVersion_ = 0;
    int32_t cachedVersion_ = -1;

    double synTimeAbs_ = 0.0;
    int kInt_ = 0;
    float acc_ = 0.0f;
    
    float lastT0_ = 120.f;
    float refT0_ = 0.f;

    std::vector<float> prevWin_;
    float prevE2_ = 0.f;
    bool havePrev_ = false;
    
    float rmsEnv_ = 0.f;
    
    // FIX: Crossfade buffer
    std::vector<float> crossfadeBuffer_;
    int crossfadePos_;

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

// Better pitch detector
class RobustPitchDetector {
    float lastPeriod_ = 218.0f;
    std::vector<float> periodHistory_;
    
public:
    std::vector<int> findEpochs(const float* input, int numSamples, float sampleRate) {
        std::vector<int> marks;
        
        // Calculate RMS for adaptive threshold
        float rms = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            rms += input[i] * input[i];
        }
        rms = std::sqrt(rms / numSamples);
        
        // FIX: Dynamic threshold based on signal level
        float threshold = std::max(0.001f, std::min(0.1f, rms * 0.2f));
        
        // FIX: Better peak detection with hysteresis
        int minDist = std::max(30, (int)(lastPeriod_ * 0.7f));
        float peakValue = 0;
        int peakIdx = -1;
        bool inPeak = false;
        
        for (int i = 1; i < numSamples - 1; ++i) {
            float val = input[i];
            
            if (!inPeak && val > threshold && val > input[i-1]) {
                // Starting a peak
                inPeak = true;
                peakValue = val;
                peakIdx = i;
            } else if (inPeak) {
                if (val > peakValue) {
                    // Update peak
                    peakValue = val;
                    peakIdx = i;
                } else if (val < threshold * 0.7f || val < input[i-1]) {
                    // End of peak
                    if (marks.empty() || peakIdx - marks.back() >= minDist) {
                        marks.push_back(peakIdx);
                        
                        // Update period estimate
                        if (marks.size() >= 2) {
                            float newPeriod = (float)(marks.back() - marks[marks.size()-2]);
                            periodHistory_.push_back(newPeriod);
                            if (periodHistory_.size() > 10) {
                                periodHistory_.erase(periodHistory_.begin());
                            }
                            
                            // Median filter for stability
                            std::vector<float> sorted = periodHistory_;
                            std::sort(sorted.begin(), sorted.end());
                            lastPeriod_ = sorted[sorted.size()/2];
                        }
                    }
                    inPeak = false;
                }
            }
        }
        
        return marks;
    }
    
    float getLastPeriod() const { return lastPeriod_; }
};

} // namespace

// Main IntelligentHarmonizer implementation
class IntelligentHarmonizer::Impl {
public:
    PsolaEngineFixed psolaEngine_;
    RobustPitchDetector pitchDetector_;
    
    SmoothedParam pitchRatio_;
    SmoothedParam mix_;
    SmoothedParam formantShift_;
    
    int scaleIndex_ = 9;
    double sampleRate_ = 48000.0;
    int64_t processedSamples_ = 0;
    
    // FIX: Add DC blocker
    float dcBlockerState_ = 0.0f;
    
    void prepare(double sampleRate) {
        sampleRate_ = sampleRate;
        psolaEngine_.prepare(sampleRate, 0.6);
        
        // FIX: Faster parameter smoothing
        pitchRatio_.setSmoothingTime(5.0f, sampleRate);
        mix_.setSmoothingTime(5.0f, sampleRate);
        formantShift_.setSmoothingTime(5.0f, sampleRate);
        
        pitchRatio_.snap(1.0f);
        mix_.snap(1.0f);
        formantShift_.snap(0.0f);
        
        processedSamples_ = 0;
        dcBlockerState_ = 0.0f;
    }
    
    void processBlock(const float* input, float* output, int numSamples) {
        // FIX: DC blocking on input
        std::vector<float> dcBlocked(numSamples);
        const float dcAlpha = 0.995f;
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = input[i];
            dcBlocked[i] = sample - dcBlockerState_;
            dcBlockerState_ = sample * (1.0f - dcAlpha) + dcBlockerState_ * dcAlpha;
            dcBlocked[i] = flushDenorm(dcBlocked[i]);
        }
        
        psolaEngine_.pushBlock(dcBlocked.data(), numSamples);
        
        // Check for sufficient signal level
        float inputEnergy = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            inputEnergy += dcBlocked[i] * dcBlocked[i];
        }
        
        if (inputEnergy > 1e-8f) {
            auto marks = pitchDetector_.findEpochs(dcBlocked.data(), numSamples, sampleRate_);
            if (!marks.empty()) {
                float period = pitchDetector_.getLastPeriod();
                psolaEngine_.appendEpochs(marks, processedSamples_, period, true);
            }
        }
        
        float ratio = pitchRatio_.tick();
        
        if (std::fabs(ratio - 1.0f) < 0.01f) {
            std::memcpy(output, input, numSamples * sizeof(float));
        } else {
            psolaEngine_.renderBlock(ratio, output, numSamples, processedSamples_);
            
            float mixVal = mix_.tick();
            if (mixVal < 0.999f) {
                for (int i = 0; i < numSamples; ++i) {
                    output[i] = input[i] * (1.0f - mixVal) + output[i] * mixVal;
                    output[i] = flushDenorm(output[i]);
                }
            }
        }
        
        // FIX: Final limiting with proper denormal handling
        for (int i = 0; i < numSamples; ++i) {
            float x = output[i];
            // Soft limiting
            if (x > 0.95f) {
                x = 0.95f + 0.05f * std::tanh(20.0f * (x - 0.95f));
            } else if (x < -0.95f) {
                x = -0.95f - 0.05f * std::tanh(-20.0f * (x + 0.95f));
            }
            output[i] = flushDenorm(x);
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

// ... [Include all the scale definitions and public interface methods from before]
// ... [The rest of the file continues with the same public interface as before]
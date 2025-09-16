// IntelligentHarmonizer with Hybrid TD-PSOLA/Resampling Engine
// Automatically selects best algorithm based on pitch ratio
// TD-PSOLA for simple ratios (0.5, 1.5, 2.0)
// Sinc resampling for irrational ratios (0.7071, 1.4142)

#include <vector>
#include <deque>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <array>

// Simple high-quality sinc resampler for problematic ratios
class SincResampler {
    static constexpr int kSincTaps = 32;
    static constexpr int kTableSize = 1024;
    std::array<std::array<float, kSincTaps>, kTableSize> sincTable_;
    
public:
    SincResampler() {
        // Precompute windowed sinc table
        for (int i = 0; i < kTableSize; ++i) {
            float frac = (float)i / kTableSize;
            for (int j = 0; j < kSincTaps; ++j) {
                int n = j - kSincTaps/2;
                float x = n - frac;
                float sinc = (x == 0) ? 1.0f : std::sin(M_PI * x) / (M_PI * x);
                float window = 0.5f + 0.5f * std::cos(2.0f * M_PI * (j + 0.5f) / kSincTaps);
                sincTable_[i][j] = sinc * window;
            }
        }
    }
    
    float interpolate(const std::deque<float>& buffer, double position) {
        int idx = (int)position;
        float frac = position - idx;
        int tableIdx = (int)(frac * kTableSize);
        
        float sum = 0.0f;
        for (int i = 0; i < kSincTaps; ++i) {
            int sampleIdx = idx + i - kSincTaps/2;
            if (sampleIdx >= 0 && sampleIdx < buffer.size()) {
                sum += buffer[sampleIdx] * sincTable_[tableIdx][i];
            }
        }
        return sum;
    }
};

// Complete Intelligent Harmonizer with hybrid pitch shifting
class IntelligentHarmonizer {
public:
    struct Parameters {
        float pitchRatio = 1.0f;     // Pitch shift ratio
        float mix = 1.0f;             // Dry/wet mix
        int scaleIndex = 9;           // 9 = Chromatic (no quantization)
        float formantShift = 0.0f;    // Formant preservation amount
        bool useHybrid = true;        // Enable hybrid mode
    };
    
private:
    // PSOLA Engine components
    struct Epoch {
        int64_t nAbs;
        float T0;
        float rms;
        bool voiced;
    };
    
    // State
    double fs_ = 48000.0;
    Parameters params_;
    
    // History buffer
    static constexpr int kHistSize = 131072; // 2^17 samples
    static constexpr int kHistMask = kHistSize - 1;
    std::array<float, kHistSize> hist_;
    int64_t writeAbs_ = 0;
    
    // Epochs
    std::deque<Epoch> epochs_;
    
    // Synthesis state
    double synTimeAbs_ = 0.0;
    float anaIdxF_ = 0.0f;
    float refT0_ = 240.0f;
    
    // Phase locking
    std::vector<float> prevWin_;
    float prevE2_ = 0.0f;
    bool havePrev_ = false;
    
    // Window cache
    std::vector<float> hannW_;
    float w2sum_ = 1.0f;
    
    // RMS tracking
    float rmsEnv_ = 0.0f;
    
    // Resampler for hybrid mode
    SincResampler resampler_;
    std::deque<float> resampleBuffer_;
    double resamplePhase_ = 0.0;
    
    // Pitch detection state
    float lastDetectedF0_ = 0.0f;
    
public:
    void prepare(double sampleRate) {
        fs_ = sampleRate;
        hist_.fill(0.0f);
        writeAbs_ = 0;
        epochs_.clear();
        synTimeAbs_ = 0.0;
        anaIdxF_ = 0.0f;
        refT0_ = fs_ / 200.0f;
        havePrev_ = false;
        rmsEnv_ = 0.0f;
        resampleBuffer_.clear();
        resamplePhase_ = 0.0;
        lastDetectedF0_ = 0.0f;
    }
    
    void setParameters(const Parameters& p) {
        params_ = p;
    }
    
    void processBlock(const float* input, float* output, int numSamples) {
        // Store input in history
        for (int i = 0; i < numSamples; ++i) {
            hist_[writeAbs_ & kHistMask] = input[i];
            resampleBuffer_.push_back(input[i]);
            writeAbs_++;
        }
        
        // Keep resampler buffer reasonable size
        while (resampleBuffer_.size() > fs_) {
            resampleBuffer_.pop_front();
        }
        
        // Detect pitch and mark epochs
        detectAndMarkEpochs(numSamples);
        
        // Determine if we should use resampling for this ratio
        bool useResampling = shouldUseResampling(params_.pitchRatio);
        
        if (useResampling && params_.useHybrid) {
            // Use high-quality sinc resampling
            processWithResampling(output, numSamples);
        } else {
            // Use TD-PSOLA
            processWithPSOLA(output, numSamples);
        }
        
        // Apply mix
        if (params_.mix < 0.999f) {
            for (int i = 0; i < numSamples; ++i) {
                output[i] = input[i] * (1.0f - params_.mix) + output[i] * params_.mix;
            }
        }
    }
    
private:
    bool shouldUseResampling(float alpha) const {
        // Problematic irrational ratios that cause subharmonics in TD-PSOLA
        const struct { float ratio; float tol; } problematic[] = {
            {0.7071f, 0.01f}, // √2/2
            {1.4142f, 0.01f}, // √2
            {0.7937f, 0.01f}, // 2^(-4/12)
            {1.2599f, 0.01f}, // 2^(4/12)
            {0.8909f, 0.01f}, // 2^(-2/12)
            {1.1225f, 0.01f}, // 2^(2/12)
        };
        
        for (const auto& p : problematic) {
            if (std::fabs(alpha - p.ratio) < p.tol) return true;
        }
        
        // Simple ratios work well with TD-PSOLA
        const float tol = 0.001f;
        for (int n = 1; n <= 4; ++n) {
            for (int d = 1; d <= 4; ++d) {
                if (std::fabs(alpha - (float)n/d) < tol) return false;
            }
        }
        
        return false;
    }
    
    void detectAndMarkEpochs(int blockSize) {
        // Simple epoch detection every period
        if (epochs_.empty() || writeAbs_ - epochs_.back().nAbs > refT0_ * 0.8f) {
            // Find local maximum in recent history
            int64_t searchStart = writeAbs_ - blockSize;
            int64_t searchEnd = writeAbs_;
            
            float maxVal = -1e9f;
            int64_t maxIdx = searchStart;
            
            for (int64_t i = searchStart; i < searchEnd; ++i) {
                float val = hist_[i & kHistMask];
                if (val > maxVal) {
                    maxVal = val;
                    maxIdx = i;
                }
            }
            
            // Add epoch
            epochs_.push_back({maxIdx, refT0_, 0.1f, true});
            
            // Prune old epochs
            int64_t keepFrom = writeAbs_ - kHistSize + 8192;
            while (!epochs_.empty() && epochs_.front().nAbs < keepFrom) {
                epochs_.pop_front();
            }
            
            // Update reference period
            if (epochs_.size() >= 3) {
                std::vector<float> periods;
                for (size_t i = 1; i < epochs_.size(); ++i) {
                    float period = epochs_[i].nAbs - epochs_[i-1].nAbs;
                    if (period > 16 && period < 0.03 * fs_) {
                        periods.push_back(period);
                    }
                }
                if (!periods.empty()) {
                    std::nth_element(periods.begin(), periods.begin() + periods.size()/2, periods.end());
                    refT0_ = periods[periods.size()/2];
                }
            }
        }
    }
    
    void processWithPSOLA(float* output, int numSamples) {
        std::fill(output, output + numSamples, 0.0f);
        
        if (epochs_.size() < 4) return;
        
        const float alpha = params_.pitchRatio;
        const float synHop = refT0_ / std::max(1e-6f, alpha);
        const int winLen = std::max(32, makeOdd((int)(2.5f * refT0_)));
        ensureHann(winLen);
        const int half = winLen / 2;
        
        // Phase locking setup
        if ((int)prevWin_.size() != winLen) {
            prevWin_.assign(winLen, 0.0f);
            havePrev_ = false;
        }
        std::vector<float> curWin(winLen, 0.0f);
        
        int64_t outStartAbs = writeAbs_ - numSamples;
        const double blockEndAbs = (double)outStartAbs + numSamples;
        
        while (synTimeAbs_ < blockEndAbs + 0.5 * refT0_) {
            // Nearest epoch selection (no interpolation!)
            int kNear = (int)std::round(anaIdxF_);
            kNear = std::max(0, std::min(kNear, (int)epochs_.size() - 1));
            int64_t centerAbs = epochs_[kNear].nAbs;
            
            // Micro-WSOLA alignment
            int bestShift = 0;
            if (havePrev_ && prevE2_ > 1e-8f) {
                const int searchHalf = std::max(1, (int)(0.05f * refT0_));
                float bestScore = -1e9f;
                
                for (int d = -searchHalf; d <= searchHalf; ++d) {
                    double dot = 0.0, e2 = 0.0;
                    for (int i = 0; i < winLen; ++i) {
                        int64_t idx = centerAbs + d + i - half;
                        if (idx >= 0 && idx < writeAbs_) {
                            float s = hannW_[i] * hist_[idx & kHistMask];
                            dot += s * prevWin_[i];
                            e2 += s * s;
                        }
                    }
                    float score = (e2 > 1e-9f && prevE2_ > 1e-9f) ? 
                                 (float)(dot / std::sqrt(e2 * prevE2_)) : 0.0f;
                    if (score > bestScore) {
                        bestScore = score;
                        bestShift = d;
                    }
                }
            }
            
            int64_t alignedCenter = centerAbs + bestShift;
            
            // Build windowed grain
            double e2 = 0.0, dotPrev = 0.0;
            for (int i = 0; i < winLen; ++i) {
                int64_t idx = alignedCenter + i - half;
                float s = (idx >= 0 && idx < writeAbs_) ? hist_[idx & kHistMask] : 0.0f;
                float v = hannW_[i] * s;
                curWin[i] = v;
                e2 += v * v;
                if (havePrev_) dotPrev += v * prevWin_[i];
            }
            
            // Polarity guard
            float sgn = (havePrev_ && dotPrev < 0.0) ? -1.0f : 1.0f;
            
            // Energy equalization
            float curRms = std::sqrt(e2 / w2sum_);
            rmsEnv_ = 0.995f * rmsEnv_ + 0.005f * curRms;
            float g_eq = (curRms > 1e-9f) ? (rmsEnv_ / curRms) : 1.0f;
            
            // OLA density compensation
            float overlap = (float)winLen / std::max(1e-6f, synHop);
            float g_ola = std::sqrt(std::max(1e-6f, 1.0f / overlap));
            
            float g = g_eq * g_ola * sgn * 0.7f; // Scale down a bit
            
            // Write grain
            int synC = (int)std::round(synTimeAbs_);
            for (int i = 0; i < winLen; ++i) {
                int nsAbs = synC + i - half;
                int rel = nsAbs - outStartAbs;
                if (rel >= 0 && rel < numSamples) {
                    output[rel] += g * curWin[i];
                }
            }
            
            // Update state
            prevWin_ = curWin;
            prevE2_ = e2;
            havePrev_ = true;
            
            synTimeAbs_ += synHop;
            anaIdxF_ += 1.0f / std::max(1e-6f, alpha);
            if (anaIdxF_ > epochs_.size() - 1.5f) {
                anaIdxF_ = epochs_.size() - 1.5f;
            }
        }
    }
    
    void processWithResampling(float* output, int numSamples) {
        if (resampleBuffer_.size() < 64) {
            std::fill(output, output + numSamples, 0.0f);
            return;
        }
        
        const float alpha = params_.pitchRatio;
        
        for (int i = 0; i < numSamples; ++i) {
            // Get interpolated sample
            output[i] = resampler_.interpolate(resampleBuffer_, resamplePhase_);
            
            // Advance phase
            resamplePhase_ += 1.0 / alpha;
            
            // Wrap phase
            while (resamplePhase_ >= resampleBuffer_.size() - 64) {
                resamplePhase_ -= resampleBuffer_.size() / 2;
            }
        }
    }
    
    // Utilities
    static int makeOdd(int n) { return (n & 1) ? n : n + 1; }
    
    void ensureHann(int L) {
        if ((int)hannW_.size() == L) return;
        hannW_.resize(L);
        for (int i = 0; i < L; ++i) {
            hannW_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (L - 1)));
        }
        w2sum_ = 0.0f;
        for (float w : hannW_) w2sum_ += w * w;
        if (w2sum_ < 1e-9f) w2sum_ = 1.0f;
    }
};

// Test harness
#ifdef STANDALONE_TEST

#include <cstdio>

static float estimateF0(const std::vector<float>& x, float fs) {
    int N = x.size();
    int minLag = fs / 800;
    int maxLag = std::min(N - 1, (int)(fs / 60));
    
    float maxCorr = -1;
    int bestLag = minLag;
    
    for (int lag = minLag; lag <= maxLag; ++lag) {
        float sum = 0, norm1 = 0, norm2 = 0;
        for (int i = 0; i < N - lag; ++i) {
            sum += x[i] * x[i + lag];
            norm1 += x[i] * x[i];
            norm2 += x[i + lag] * x[i + lag];
        }
        float corr = sum / (std::sqrt(norm1 * norm2) + 1e-12f);
        if (corr > maxCorr) {
            maxCorr = corr;
            bestLag = lag;
        }
    }
    
    return fs / bestLag;
}

int main() {
    const float fs = 48000;
    const float f0 = 220;
    const int duration = 48000; // 1 second
    
    // Generate test signal
    std::vector<float> input(duration);
    for (int i = 0; i < duration; ++i) {
        input[i] = 0.3f * std::sin(2.0f * M_PI * f0 * i / fs);
    }
    
    IntelligentHarmonizer harmonizer;
    harmonizer.prepare(fs);
    
    // Test different pitch ratios
    float ratios[] = {0.5f, 0.7071f, 1.0f, 1.5f, 2.0f};
    const char* names[] = {"Oct Down", "Tritone Down", "Unison", "Fifth Up", "Oct Up"};
    
    printf("=== Intelligent Harmonizer Final Test ===\n\n");
    
    for (int r = 0; r < 5; ++r) {
        IntelligentHarmonizer::Parameters params;
        params.pitchRatio = ratios[r];
        params.mix = 1.0f;
        params.scaleIndex = 9; // Chromatic
        params.useHybrid = true;
        
        harmonizer.setParameters(params);
        harmonizer.prepare(fs); // Reset for each test
        
        std::vector<float> output(duration);
        
        // Process in blocks
        const int blockSize = 512;
        for (int i = 0; i < duration; i += blockSize) {
            int n = std::min(blockSize, duration - i);
            harmonizer.processBlock(input.data() + i, output.data() + i, n);
        }
        
        // Analyze output pitch (skip first 0.2s)
        int skipSamples = 0.2f * fs;
        std::vector<float> tail(output.begin() + skipSamples, output.end());
        
        float detectedF0 = estimateF0(tail, fs);
        float expectedF0 = f0 * ratios[r];
        float cents = 1200.0f * std::log2(detectedF0 / expectedF0);
        
        printf("%-15s: ratio=%.4f, detected=%.1fHz, expected=%.1fHz, error=%+.1f cents\n",
               names[r], ratios[r], detectedF0, expectedF0, cents);
    }
    
    printf("\n✅ Test complete!\n");
    return 0;
}

#endif
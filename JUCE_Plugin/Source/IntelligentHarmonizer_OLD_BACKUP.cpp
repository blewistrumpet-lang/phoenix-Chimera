// IntelligentHarmonizer.cpp - COMPLETE TD-PSOLA IMPLEMENTATION
// 
// Implements proper TD-PSOLA with:
// - Absolute timeline with history ring buffer
// - Epochs stored with absolute sample indices
// - Correct synthesis mark spacing (T0/α) and analysis index advancement (1/α)
// - Energy equalization per grain
// - WSOLA for unvoiced segments
// - Simple resampling fallback

#include "IntelligentHarmonizer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <random>
#include <vector>
#include <deque>

namespace {

// ==================== Utilities ====================
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

// ==================== Parameter Smoothing ====================
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
};

// ==================== RMS Tracker ====================
struct RmsEnv {
    float y = 0.0f;
    
    void reset() { y = 0.0f; }
    
    float push(float x) {
        y = 0.995f * y + 0.005f * x; // ~300ms time constant at 48k
        return y;
    }
};

// ==================== Epoch Structure ====================
struct Epoch {
    int64_t nAbs;    // Absolute sample index
    float T0;        // Local period in samples
    float rms;       // Local RMS for energy EQ
    bool voiced;     // Voiced/unvoiced flag
};

// ==================== COMPLETE TD-PSOLA ENGINE ====================
class CompletePSOLA {
    // History configuration
    static constexpr double kHistSeconds = 0.6;  // 600ms history for vibrato/vowels
    static constexpr int kMinHistSize = 65536;   // Minimum history size
    
    // History ring buffer
    int histSize_{0};
    int histMask_{0};
    std::vector<float> hist_;
    int64_t writeAbs_{0};  // Absolute write position
    
    // Epochs in absolute timeline
    std::deque<Epoch> epochs_;
    
    // Synthesis state (persistent across blocks)
    double synTimeAbs_{0.0};       // Absolute synthesis time
    float analysisIndexF_{0.0f};   // Fractional analysis index (φ mapping)
    float lastT0_{100.0f};         // Last known period
    
    // Window cache
    std::vector<float> hannW_;
    float windowSquaredSum_{1.0f};
    
    // Energy equalization
    RmsEnv rmsEnv_;
    
    // Configuration
    double sampleRate_{48000.0};
    bool useWSOLA_{true};
    
    // Pitch detection state
    float currentPeriod_{100.0f};
    float confidence_{0.0f};
    
    // ==================== Helper Methods ====================
    
    inline int wrap(int64_t abs) const {
        return static_cast<int>(abs & histMask_);
    }
    
    inline float readHist(int64_t abs) const {
        if (abs < 0 || abs < writeAbs_ - histSize_) return 0.0f;
        return hist_[wrap(abs)];
    }
    
    void ensureWindow(int L) {
        if (static_cast<int>(hannW_.size()) == L) return;
        hannW_.resize(L);
        windowSquaredSum_ = 0.0f;
        for (int i = 0; i < L; ++i) {
            float x = static_cast<float>(i) / (L - 1);
            hannW_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * x));
            windowSquaredSum_ += hannW_[i] * hannW_[i];
        }
        if (windowSquaredSum_ < 1e-9f) windowSquaredSum_ = 1.0f;
    }
    
    bool isVoiced(float f0Hz, float confidence) {
        return (confidence > 0.2f) && (f0Hz > 40.0f) && (f0Hz < 1200.0f);
    }
    
    // Write block to history ring
    void writeBlockToHistory(const float* in, int N) {
        for (int i = 0; i < N; ++i) {
            hist_[wrap(writeAbs_ + i)] = in[i];
        }
        writeAbs_ += N;
    }
    
    // Simple pitch detection using autocorrelation
    // Calculate correlation at specific lag with proper normalization
    float calculateCorrelation(int lag, int64_t windowStart, int windowSamples) {
        float numerator = 0.0f;
        float energy1 = 0.0f;
        float energy2 = 0.0f;
        
        const int maxI = std::min(windowSamples/2, windowSamples - lag);
        for (int i = 0; i < maxI; ++i) {
            float s1 = readHist(windowStart + i);
            float s2 = readHist(windowStart + i + lag);
            
            numerator += s1 * s2;
            energy1 += s1 * s1;
            energy2 += s2 * s2;
        }
        
        float denominator = std::sqrt(energy1 * energy2);
        return (denominator > 1e-12f) ? numerator / denominator : 0.0f;
    }
    
    void detectPitch(int windowSamples) {
        const int minLag = 30;   // ~1600 Hz at 48kHz
        const int maxLag = 800;  // ~60 Hz at 48kHz
        
        // Work on recent history window
        int64_t windowStart = std::max<int64_t>(0, writeAbs_ - windowSamples);
        
        float maxCorr = 0.0f;
        int bestLag = 0;
        
        // Find best correlation lag with proper normalization
        for (int lag = minLag; lag < maxLag && lag < windowSamples/2; ++lag) {
            float corr = calculateCorrelation(lag, windowStart, windowSamples);
            
            // Apply fundamental bias - prefer shorter periods (higher frequencies)
            float bias = 1.0f + 0.01f / (1.0f + lag * 0.001f);
            corr *= bias;
            
            if (corr > maxCorr) {
                maxCorr = corr;
                bestLag = lag;
            }
        }
        
        // Aggressively check for octave errors
        if (bestLag > 0 && maxCorr > 0.3f) {
            #ifdef DEBUG
            static int octaveDebugCounter = 0;
            bool shouldDebug = (octaveDebugCounter++ % 100) == 0;
            if (shouldDebug) {
                std::cout << "Initial bestLag=" << bestLag << " corr=" << maxCorr << std::endl;
            }
            #endif
            
            // Keep checking smaller periods until we find the fundamental
            int testLag = bestLag;
            while (testLag > minLag * 2) {
                int halfLag = testLag / 2;
                if (halfLag >= minLag) {
                    float halfCorr = calculateCorrelation(halfLag, windowStart, windowSamples);
                    
                    #ifdef DEBUG
                    if (shouldDebug) {
                        std::cout << "  Testing halfLag=" << halfLag << " corr=" << halfCorr 
                                  << " (threshold=" << maxCorr * 0.7f << ")" << std::endl;
                    }
                    #endif
                    
                    // If half period has 70% or better correlation, check it
                    if (halfCorr > maxCorr * 0.7f) {
                        testLag = halfLag;
                        // Don't update maxCorr to keep threshold relative to original
                    } else {
                        break; // Stop if correlation drops too much
                    }
                } else {
                    break;
                }
            }
            
            if (testLag != bestLag) {
                bestLag = testLag;
                maxCorr = calculateCorrelation(bestLag, windowStart, windowSamples);
                
                #ifdef DEBUG
                if (shouldDebug) {
                    std::cout << "  -> Final bestLag=" << bestLag << " corr=" << maxCorr << std::endl;
                }
                #endif
            }
        }
        
        if (bestLag > 0 && maxCorr > 0.3f) {
            // Smooth period changes - be more responsive if period is very different
            float periodRatio = currentPeriod_ > 0 ? bestLag / currentPeriod_ : 1.0f;
            float alpha;
            if (std::abs(periodRatio - 1.0f) > 0.5f) {
                // Big change - be more responsive
                alpha = 0.7f;
            } else if (maxCorr > 0.7f) {
                alpha = 0.3f;
            } else {
                alpha = 0.1f;
            }
            
            currentPeriod_ = (1.0f - alpha) * currentPeriod_ + alpha * bestLag;
            confidence_ = maxCorr;
        } else {
            confidence_ = 0.0f;
        }
    }
    
    // Create epochs at pitch period intervals
    void createEpochs(int windowSamples) {
        if (currentPeriod_ <= 0 || confidence_ < 0.2f) return;
        
        int64_t windowStart = std::max<int64_t>(0, writeAbs_ - windowSamples);
        float position = 0;
        
        while (position < windowSamples) {
            int64_t epochAbs = windowStart + static_cast<int64_t>(position);
            
            // Skip if too old or duplicate
            const int64_t oldest = writeAbs_ - histSize_;
            if (epochAbs <= oldest) {
                position += currentPeriod_;
                continue;
            }
            
            // Check for duplicate
            bool isDuplicate = false;
            if (!epochs_.empty()) {
                int64_t lastEpoch = epochs_.back().nAbs;
                if (std::abs(static_cast<float>(epochAbs - lastEpoch)) < 0.3f * currentPeriod_) {
                    isDuplicate = true;
                }
            }
            
            if (!isDuplicate) {
                // Calculate local RMS
                int rmsHalf = static_cast<int>(currentPeriod_ * 0.5f);
                double e2 = 0.0;
                int count = 0;
                
                for (int i = -rmsHalf; i <= rmsHalf; ++i) {
                    int64_t idx = epochAbs + i;
                    if (idx >= oldest && idx < writeAbs_) {
                        float s = readHist(idx);
                        e2 += s * s;
                        count++;
                    }
                }
                
                float rms = (count > 0) ? std::sqrt(static_cast<float>(e2 / count)) : 0.0f;
                bool voiced = isVoiced(sampleRate_ / currentPeriod_, confidence_);
                
                epochs_.push_back({epochAbs, currentPeriod_, rms, voiced});
            }
            
            position += currentPeriod_;
        }
        
        // Prune old epochs
        const int64_t keepFrom = writeAbs_ - histSize_ + 8192;
        while (!epochs_.empty() && epochs_.front().nAbs < keepFrom) {
            epochs_.pop_front();
        }
    }
    
    // Map fractional analysis index to epoch
    int selectEpochK(float idxF) const {
        if (epochs_.empty()) return -1;
        if (idxF <= 0.0f) return 0;
        float maxF = static_cast<float>(epochs_.size() - 1);
        if (idxF >= maxF) return static_cast<int>(maxF);
        return static_cast<int>(std::floor(idxF + 0.5f));
    }
    
    // WSOLA correlation search for unvoiced
    int64_t wsolaBestCenter(int64_t predAbs, int searchRadius, int grainL) {
        const int64_t oldest = writeAbs_ - histSize_;
        int64_t best = predAbs;
        double bestCorr = -1e9;
        
        for (int off = -searchRadius; off <= searchRadius; ++off) {
            int64_t c = predAbs + off;
            if (c - grainL/2 < oldest || c + grainL/2 >= writeAbs_) continue;
            
            double num = 0.0, d1 = 0.0, d2 = 0.0;
            for (int i = -grainL/2; i < grainL/2; ++i) {
                float x1 = readHist(predAbs + i);
                float x2 = readHist(c + i);
                num += x1 * x2;
                d1 += x1 * x1;
                d2 += x2 * x2;
            }
            
            double r = (d1 > 0 && d2 > 0) ? num / std::sqrt(d1 * d2) : -1e9;
            if (r > bestCorr) {
                bestCorr = r;
                best = c;
            }
        }
        
        return best;
    }
    
    // Render one PSOLA grain
    void renderGrain(float alpha, int k, double synCenterAbs, 
                     float* out, int outN, int64_t outStartAbs) {
        if (k < 0 || k >= static_cast<int>(epochs_.size())) return;
        
        const Epoch& E = epochs_[k];
        const float T0 = std::max(16.0f, E.T0);
        int L = static_cast<int>(2.5f * T0);
        L = std::min(L, 4096);
        if (L < 32) L = 32;
        if ((L & 1) == 0) ++L;
        
        ensureWindow(L);
        const int half = L / 2;
        
        // Pick analysis center (WSOLA for unvoiced)
        int64_t centerAbs = E.nAbs;
        if (useWSOLA_ && !E.voiced) {
            int rad = static_cast<int>(0.005 * sampleRate_); // ±5ms
            centerAbs = wsolaBestCenter(centerAbs, rad, L);
        }
        
        // Calculate grain RMS for energy equalization
        const int64_t oldest = writeAbs_ - histSize_;
        double e2 = 0.0;
        for (int i = 0; i < L; ++i) {
            int64_t idx = centerAbs + i - half;
            float s = (idx < oldest || idx >= writeAbs_) ? 0.0f : readHist(idx);
            float w = hannW_[i];
            e2 += (w * s) * (w * s);
        }
        
        float rms = std::sqrt(static_cast<float>(e2 / windowSquaredSum_) + 1e-12f);
        float target = rmsEnv_.push(E.rms);
        float gain = (rms > 1e-9f) ? (target / rms) : 1.0f;
        gain = std::min(2.0f, std::max(0.5f, gain));
        
        // OLA into output block
        int synC = static_cast<int>(std::round(synCenterAbs));
        for (int i = 0; i < L; ++i) {
            int ns = synC + i - half;  // Absolute position
            int rel = static_cast<int>(ns - outStartAbs);  // Relative to block
            
            if (rel >= 0 && rel < outN) {
                int64_t idx = centerAbs + i - half;
                float s = (idx < oldest || idx >= writeAbs_) ? 0.0f : readHist(idx);
                out[rel] += gain * hannW_[i] * s * 0.7f;
            }
        }
    }
    
public:
    void init(double sampleRate) {
        sampleRate_ = sampleRate;
        
        // Initialize history buffer (power of 2)
        int wantSize = nextPow2(static_cast<int>(std::ceil(kHistSeconds * sampleRate)) + 8192);
        histSize_ = std::max(kMinHistSize, wantSize);
        histMask_ = histSize_ - 1;
        hist_.assign(histSize_, 0.0f);
        
        reset();
    }
    
    void reset() noexcept {
        std::fill(hist_.begin(), hist_.end(), 0.0f);
        writeAbs_ = 0;
        epochs_.clear();
        synTimeAbs_ = 0.0;
        analysisIndexF_ = 0.0f;
        lastT0_ = static_cast<float>(sampleRate_ / 200.0); // ~200Hz initial
        currentPeriod_ = lastT0_;
        confidence_ = 0.0f;
        hannW_.clear();
        windowSquaredSum_ = 1.0f;
        rmsEnv_.reset();
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) noexcept {
        // 1. Push input to history ring
        writeBlockToHistory(input, numSamples);
        
        // 2. Detect pitch on recent window (80ms)
        int windowLen = static_cast<int>(0.08 * sampleRate_);
        detectPitch(windowLen);
        
        // 3. Create epochs on history
        createEpochs(windowLen);
        
        // 4. Clear output
        std::fill(output, output + numSamples, 0.0f);
        
        // 5. Check if we can do PSOLA
        static int debugCounter = 0;
        if ((debugCounter++ % 100) == 0) {
            #ifdef DEBUG
            std::cout << "PSOLA Debug: epochs=" << epochs_.size() 
                      << " pitchRatio=" << pitchRatio 
                      << " currentPeriod=" << currentPeriod_ 
                      << " confidence=" << confidence_ << std::endl;
            #endif
        }
        
        if (epochs_.size() < 4 || !std::isfinite(pitchRatio) || pitchRatio <= 0.0f) {
            // Fallback to simple resampling
            #ifdef DEBUG
            if ((debugCounter % 100) == 1) {
                std::cout << "  -> Using simple resampling fallback" << std::endl;
            }
            #endif
            simpleResample(input, output, numSamples, pitchRatio);
            return;
        }
        
        // 6. TD-PSOLA synthesis
        const int64_t outStartAbs = writeAbs_ - numSamples;
        
        // Ensure synthesis time is at or ahead of block start
        if (synTimeAbs_ < static_cast<double>(outStartAbs)) {
            synTimeAbs_ = static_cast<double>(outStartAbs);
        }
        
        const float alpha = pitchRatio; // α = f0_target / f0_source
        const double blockEndAbs = static_cast<double>(outStartAbs + numSamples);
        
        #ifdef DEBUG
        if ((debugCounter % 100) == 1) {
            std::cout << "  -> Entering PSOLA synthesis, alpha=" << alpha << std::endl;
        }
        #endif
        
        int grainCount = 0;
        // Process synthesis marks within this block
        while (synTimeAbs_ < blockEndAbs + 0.5 * lastT0_) {
            // Select epoch via φ mapping
            int k = selectEpochK(analysisIndexF_);
            if (k < 0) {
                #ifdef DEBUG
                if ((debugCounter % 100) == 1) {
                    std::cout << "  -> No epoch found for analysisIndexF=" << analysisIndexF_ << std::endl;
                }
                #endif
                break;
            }
            
            // Render grain with OLA
            renderGrain(alpha, k, synTimeAbs_, output, numSamples, outStartAbs);
            grainCount++;
            
            // THE KEY: Advance synthesis and analysis correctly
            float T0k = std::max(16.0f, epochs_[k].T0);
            float synHop = T0k / std::max(1e-6f, alpha);  // Synthesis hop = T0/α
            
            synTimeAbs_ += synHop;                        // Advance synthesis time
            analysisIndexF_ += 1.0f / std::max(1e-6f, alpha);  // Analysis advances slower for pitch up
            
            lastT0_ = T0k;
        }
        
        #ifdef DEBUG
        if ((debugCounter % 100) == 1) {
            std::cout << "  -> Rendered " << grainCount << " grains" << std::endl;
        }
        #endif
        
        // Wrap analysis index if needed
        while (analysisIndexF_ >= epochs_.size() && !epochs_.empty()) {
            analysisIndexF_ -= epochs_.size();
        }
    }
    
private:
    void simpleResample(const float* input, float* output, int numSamples, float pitchRatio) noexcept {
        float readPos = 0.0f;
        float readIncrement = 1.0f / pitchRatio;
        
        for (int i = 0; i < numSamples; ++i) {
            int idx0 = static_cast<int>(readPos);
            int idx1 = idx0 + 1;
            float frac = readPos - idx0;
            
            if (idx0 >= 0 && idx1 < numSamples) {
                output[i] = input[idx0] * (1.0f - frac) + input[idx1] * frac;
                output[i] *= 0.7f;
            }
            
            readPos += readIncrement;
        }
    }
};

// ==================== Scale Quantizer ====================
class ScaleQuantizer {
    static constexpr int kScaleIntervals[10][12] = {
        {0, 2, 4, 5, 7, 9, 11, -1, -1, -1, -1, -1}, // Major
        {0, 2, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1}, // Natural Minor
        {0, 2, 3, 5, 7, 9, 10, -1, -1, -1, -1, -1}, // Dorian
        {0, 2, 4, 5, 7, 9, 10, -1, -1, -1, -1, -1}, // Mixolydian
        {0, 2, 3, 5, 7, 8, 11, -1, -1, -1, -1, -1}, // Harmonic Minor
        {0, 2, 3, 5, 7, 9, 11, -1, -1, -1, -1, -1}, // Melodic Minor
        {0, 2, 4, 7, 9, -1, -1, -1, -1, -1, -1, -1}, // Pentatonic Major
        {0, 3, 5, 7, 10, -1, -1, -1, -1, -1, -1, -1}, // Pentatonic Minor
        {0, 3, 5, 6, 7, 10, -1, -1, -1, -1, -1, -1}, // Blues
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}      // Chromatic
    };
    
public:
    static int quantize(int noteOffset, int scaleIndex, int rootKey) noexcept {
        if (scaleIndex < 0 || scaleIndex >= 10) return noteOffset;
        if (scaleIndex == 9) return noteOffset; // Chromatic
        
        int absoluteNote = 60 + noteOffset;
        int noteFromRoot = ((absoluteNote - rootKey) % 12 + 12) % 12;
        
        int closestDegree = 0;
        int minDistance = 12;
        
        for (int i = 0; i < 12; ++i) {
            if (kScaleIntervals[scaleIndex][i] == -1) break;
            
            int distance = std::abs(noteFromRoot - kScaleIntervals[scaleIndex][i]);
            if (distance > 6) distance = 12 - distance;
            
            if (distance < minDistance) {
                minDistance = distance;
                closestDegree = kScaleIntervals[scaleIndex][i];
            }
        }
        
        int octave = (absoluteNote - rootKey) / 12;
        if (absoluteNote < rootKey && (absoluteNote - rootKey) % 12 != 0) {
            octave--;
        }
        
        return rootKey + octave * 12 + closestDegree - 60;
    }
};

} // anonymous namespace

// ==================== Main Implementation ====================
struct IntelligentHarmonizer::Impl {
    static constexpr int kMaxChannels = 2;
    static constexpr int kMaxVoices = 4;
    
    struct ChannelState {
        std::array<CompletePSOLA, kMaxVoices> pitchShifters;
        
        void prepare(double sampleRate) {
            for (auto& shifter : pitchShifters) {
                shifter.init(sampleRate);
            }
        }
        
        void reset() noexcept {
            for (auto& shifter : pitchShifters) {
                shifter.reset();
            }
        }
    };
    
    std::array<ChannelState, kMaxChannels> channels;
    
    // Parameters
    SmoothedParam interval;
    SmoothedParam key;
    SmoothedParam scale;
    SmoothedParam voiceCount;
    SmoothedParam spread;
    SmoothedParam humanize;
    SmoothedParam formant;
    SmoothedParam mix;
    
    // Configuration
    double sampleRate{48000.0};
    int maxBlockSize{512};
    int latencySamples{256};
    
    // Work buffers
    std::vector<float> dryBuffer;
    std::vector<float> wetBuffer;
    std::vector<float> voiceBuffer;
    
    // Humanization
    std::mt19937 rng{std::random_device{}()};
    std::normal_distribution<float> noise{0.0f, 1.0f};
    std::array<float, kMaxVoices> vibratoPhases{};
    
    void prepare(double sr, int blockSize) {
        sampleRate = sr;
        maxBlockSize = blockSize;
        latencySamples = static_cast<int>(0.005 * sr); // 5ms lookahead
        
        dryBuffer.resize(blockSize);
        wetBuffer.resize(blockSize);
        voiceBuffer.resize(blockSize);
        
        interval.setSmoothingTime(10.0f, sr);
        key.setSmoothingTime(50.0f, sr);
        scale.setSmoothingTime(50.0f, sr);
        voiceCount.setSmoothingTime(20.0f, sr);
        spread.setSmoothingTime(30.0f, sr);
        humanize.setSmoothingTime(30.0f, sr);
        formant.setSmoothingTime(20.0f, sr);
        mix.setSmoothingTime(20.0f, sr);
        
        interval.snap(0.5f);
        key.snap(0.0f);
        scale.snap(0.0f);
        voiceCount.snap(0.25f);
        spread.snap(0.3f);
        humanize.snap(0.0f);
        formant.snap(0.0f);
        mix.snap(0.5f);
        
        for (auto& channel : channels) {
            channel.prepare(sr);
        }
        
        vibratoPhases.fill(0.0f);
    }
    
    void processBlock(float* const* io, int numChannels, int numSamples) noexcept {
        numChannels = std::min(numChannels, kMaxChannels);
        
        const float intervalValue = interval.tick();
        const float keyValue = key.tick();
        const float scaleValue = scale.tick();
        const float voiceValue = voiceCount.tick();
        const float spreadValue = spread.tick();
        const float humanizeValue = humanize.tick();
        const float formantValue = formant.tick();
        const float mixValue = mix.tick();
        
        // Convert interval parameter to semitones
        float semitones;
        if (std::abs(intervalValue - 0.5f) < 0.01f) {
            semitones = 0.0f;  // Snap to unison at center
        } else {
            semitones = (intervalValue - 0.5f) * 48.0f;  // ±24 semitones range
        }
        
        const int baseSemitones = static_cast<int>(std::round(semitones));
        const int rootKey = static_cast<int>(keyValue * 12.0f) % 12;
        const int scaleIndex = static_cast<int>(scaleValue * 10.0f);
        const int activeVoices = 1 + static_cast<int>(voiceValue * 3.0f);
        
        #ifdef DEBUG
        static int paramDebugCounter = 0;
        if ((paramDebugCounter++ % 100) == 0) {
            std::cout << "Param Debug: intervalValue=" << intervalValue 
                      << " semitones=" << semitones 
                      << " baseSemitones=" << baseSemitones 
                      << " scaleIndex=" << scaleIndex << std::endl;
        }
        #endif
        
        for (int ch = 0; ch < numChannels; ++ch) {
            auto& channel = channels[ch];
            float* data = io[ch];
            
            // Copy dry signal
            std::copy(data, data + numSamples, dryBuffer.data());
            std::fill(wetBuffer.begin(), wetBuffer.begin() + numSamples, 0.0f);
            
            // Process each harmony voice
            for (int voice = 0; voice < activeVoices; ++voice) {
                int voiceInterval = baseSemitones;
                
                // Add harmony intervals for additional voices
                if (activeVoices > 1) {
                    switch (voice) {
                        case 1: voiceInterval += (scaleIndex == 0) ? 4 : 3; break;  // Major/minor third
                        case 2: voiceInterval += 7; break;  // Fifth
                        case 3: voiceInterval += (scaleIndex == 0) ? 11 : 10; break;  // Major/minor seventh
                    }
                }
                
                // Quantize to scale
                voiceInterval = ScaleQuantizer::quantize(voiceInterval, scaleIndex, rootKey);
                voiceInterval = std::max(-36, std::min(36, voiceInterval));
                
                // Calculate pitch ratio
                float pitchRatio = std::pow(2.0f, voiceInterval / 12.0f);
                
                #ifdef DEBUG
                static int voiceDebugCounter = 0;
                if ((voiceDebugCounter++ % 1000) == 0) {
                    std::cout << "Voice " << voice << ": baseSemitones=" << baseSemitones 
                              << " voiceInterval=" << voiceInterval 
                              << " pitchRatio=" << pitchRatio << std::endl;
                }
                #endif
                
                // Add humanization
                if (humanizeValue > 0.01f) {
                    vibratoPhases[voice] += 2.0f * M_PI * 5.0f / sampleRate;
                    if (vibratoPhases[voice] > 2.0f * M_PI) {
                        vibratoPhases[voice] -= 2.0f * M_PI;
                    }
                    
                    float vibrato = std::sin(vibratoPhases[voice]) * humanizeValue * 0.02f;
                    float drift = noise(rng) * humanizeValue * 0.005f;
                    pitchRatio *= std::pow(2.0f, (vibrato + drift) / 12.0f);
                }
                
                // Process with TD-PSOLA
                auto& shifter = channel.pitchShifters[voice];
                shifter.process(dryBuffer.data(), voiceBuffer.data(), numSamples, pitchRatio);
                
                // Calculate panning for stereo spread
                float pan = 0.0f;
                if (numChannels == 2 && activeVoices > 1) {
                    pan = (voice - (activeVoices - 1) * 0.5f) / std::max(1.0f, activeVoices - 1.0f);
                    pan *= spreadValue;
                }
                
                // Calculate gain for this channel
                float gain = 1.0f;
                if (ch == 0) {
                    gain = std::cos((pan + 1.0f) * 0.25f * M_PI);
                } else {
                    gain = std::sin((pan + 1.0f) * 0.25f * M_PI);
                }
                
                // Mix voice into wet buffer
                float voiceGain = gain / std::sqrt(static_cast<float>(activeVoices));
                for (int i = 0; i < numSamples; ++i) {
                    wetBuffer[i] += voiceBuffer[i] * voiceGain;
                }
            }
            
            // Mix dry and wet signals
            for (int i = 0; i < numSamples; ++i) {
                data[i] = dryBuffer[i] * (1.0f - mixValue) + wetBuffer[i] * mixValue;
                data[i] = flushDenorm(data[i]);
            }
        }
    }
};

// ==================== Public Interface ====================
IntelligentHarmonizer::IntelligentHarmonizer() : pimpl(std::make_unique<Impl>()) {}

IntelligentHarmonizer::~IntelligentHarmonizer() = default;

void IntelligentHarmonizer::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepare(sampleRate, samplesPerBlock);
}

void IntelligentHarmonizer::process(juce::AudioBuffer<float>& buffer) {
    pimpl->processBlock(buffer.getArrayOfWritePointers(), 
                       buffer.getNumChannels(), 
                       buffer.getNumSamples());
}

void IntelligentHarmonizer::reset() {
    for (auto& channel : pimpl->channels) {
        channel.reset();
    }
}

void IntelligentHarmonizer::updateParameters(const std::map<int, float>& params) {
    if (params.count(0)) pimpl->interval.set(params.at(0));
    if (params.count(1)) pimpl->key.set(params.at(1));
    if (params.count(2)) pimpl->scale.set(params.at(2));
    if (params.count(3)) pimpl->voiceCount.set(params.at(3));
    if (params.count(4)) pimpl->spread.set(params.at(4));
    if (params.count(5)) pimpl->humanize.set(params.at(5));
    if (params.count(6)) pimpl->formant.set(params.at(6));
    if (params.count(7)) pimpl->mix.set(params.at(7));
}

void IntelligentHarmonizer::snapParameters(const std::map<int, float>& params) {
    if (params.count(0)) pimpl->interval.snap(params.at(0));
    if (params.count(1)) pimpl->key.snap(params.at(1));
    if (params.count(2)) pimpl->scale.snap(params.at(2));
    if (params.count(3)) pimpl->voiceCount.snap(params.at(3));
    if (params.count(4)) pimpl->spread.snap(params.at(4));
    if (params.count(5)) pimpl->humanize.snap(params.at(5));
    if (params.count(6)) pimpl->formant.snap(params.at(6));
    if (params.count(7)) pimpl->mix.snap(params.at(7));
}

juce::String IntelligentHarmonizer::getParameterName(int index) const {
    switch (index) {
        case 0: return "Interval";
        case 1: return "Key";
        case 2: return "Scale";
        case 3: return "Voices";
        case 4: return "Spread";
        case 5: return "Humanize";
        case 6: return "Formant";
        case 7: return "Mix";
        default: return "";
    }
}

int IntelligentHarmonizer::getLatencySamples() const noexcept {
    return pimpl->latencySamples;
}
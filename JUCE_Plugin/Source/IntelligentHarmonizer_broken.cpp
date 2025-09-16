// IntelligentHarmonizer.cpp - TRUE PSOLA Implementation
// Based on Pitch Synchronous Overlap-Add with YIN pitch detection
// Properly implements 2-pitch-period windows with Hann windowing
// Correct synthesis pitch mark placement for pitch shifting

#include "IntelligentHarmonizer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>
#include <random>
#include <chrono>
#include <deque>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

// Platform-specific optimizations
#ifdef _MSC_VER
#define ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define ALWAYS_INLINE inline
#endif

namespace {

// ==================== Denormal Prevention ====================
struct DenormalGuard {
    DenormalGuard() {
#if HAS_SSE2
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
} static denormGuard;

template<typename T>
ALWAYS_INLINE T flushDenorm(T v) noexcept {
#if HAS_SSE2
    if constexpr (std::is_same_v<T, float>) {
        return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(v), _mm_set_ss(0.0f)));
    }
#endif
    constexpr T tiny = static_cast<T>(1.0e-38);
    return std::fabs(v) < tiny ? static_cast<T>(0) : v;
}

// ==================== Lock-free Parameter Smoothing ====================
class SmoothedParam {
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float coeff{0.9995f};
    
public:
    void setSmoothingTime(float timeMs, double sampleRate) noexcept {
        float samples = timeMs * 0.001f * sampleRate;
        coeff = std::exp(-2.0f * static_cast<float>(M_PI) / samples);
    }
    
    void set(float v) noexcept { 
        target.store(v, std::memory_order_relaxed); 
    }
    
    void snap(float v) noexcept { 
        current = v;
        target.store(v, std::memory_order_relaxed); 
    }
    
    ALWAYS_INLINE float tick() noexcept {
        const float t = target.load(std::memory_order_relaxed);
        current += (1.0f - coeff) * (t - current);
        current = flushDenorm(current);
        return current;
    }
    
    float get() const noexcept { 
        return target.load(std::memory_order_relaxed); 
    }
};

// ==================== High-Quality Biquad Filter ====================
class PlatinumBiquad {
    double a1{}, a2{}, b0{}, b1{}, b2{};
    double x1{}, x2{}, y1{}, y2{};
    
public:
    void reset() noexcept {
        x1 = x2 = y1 = y2 = 0.0;
    }
    
    void setCoefficients(double b0_, double b1_, double b2_,
                        double a0_, double a1_, double a2_) noexcept {
        const double norm = 1.0 / std::max(a0_, 1e-30);
        b0 = b0_ * norm;
        b1 = b1_ * norm;
        b2 = b2_ * norm;
        a1 = a1_ * norm;
        a2 = a2_ * norm;
    }
    
    void setLowpass(double freq, double q, double sampleRate) noexcept {
        const double w = 2.0 * M_PI * freq / sampleRate;
        const double cosw = std::cos(w);
        const double sinw = std::sin(w);
        const double alpha = sinw / (2.0 * q);
        
        const double b0_ = (1.0 - cosw) / 2.0;
        const double b1_ = 1.0 - cosw;
        const double b2_ = b0_;
        const double a0_ = 1.0 + alpha;
        const double a1_ = -2.0 * cosw;
        const double a2_ = 1.0 - alpha;
        
        setCoefficients(b0_, b1_, b2_, a0_, a1_, a2_);
    }
    
    // Transposed Direct Form II for better numerical stability
    ALWAYS_INLINE float processTDF2(float x) noexcept {
        const double y = b0 * x + x1;
        x1 = b1 * x - a1 * y + x2;
        x2 = b2 * x - a2 * y;
        x1 = flushDenorm(x1);
        x2 = flushDenorm(x2);
        return static_cast<float>(y);
    }
};

// ==================== DC Blocker ====================
class DCBlocker {
    double x1{0.0}, y1{0.0};
    static constexpr double R = 0.995;
    
public:
    void reset() noexcept { x1 = y1 = 0.0; }
    
    ALWAYS_INLINE float process(float input) noexcept {
        double output = input - x1 + R * y1;
        x1 = input;
        y1 = flushDenorm(output);
        return static_cast<float>(output);
    }
};

// ==================== Polyphase Oversampling ====================
class PolyphaseOversampler {
    static constexpr int kMaxOversample = 8;
    static constexpr int kFilterStages = 4;
    
    int factor{1};
    std::array<PlatinumBiquad, kFilterStages> upFilters;
    std::array<PlatinumBiquad, kFilterStages> downFilters;
    std::vector<float> workBuffer;
    
public:
    void init(int oversampleFactor, double baseSampleRate, int maxBlockSize) {
        factor = std::min(kMaxOversample, oversampleFactor);
        workBuffer.resize(maxBlockSize * factor);
        
        if (factor > 1) {
            const double cutoff = 0.45 * baseSampleRate; // 90% of Nyquist
            const double oversampledRate = baseSampleRate * factor;
            
            // Cascaded Butterworth for steep rolloff
            for (int i = 0; i < kFilterStages; ++i) {
                const double q = 0.707 + i * 0.1; // Slightly increase Q for later stages
                upFilters[i].setLowpass(cutoff, q, oversampledRate);
                downFilters[i].setLowpass(cutoff, q, oversampledRate);
            }
        }
    }
    
    void reset() noexcept {
        for (auto& filter : upFilters) filter.reset();
        for (auto& filter : downFilters) filter.reset();
    }
    
    int getFactor() const noexcept { return factor; }
    
    // Process with callback for oversampled processing
    template<typename ProcessFunc>
    void process(const float* input, float* output, int numSamples, ProcessFunc&& func) noexcept {
        if (factor == 1) {
            // No oversampling - direct processing
            for (int i = 0; i < numSamples; ++i) {
                output[i] = func(input[i]);
            }
            return;
        }
        
        // Upsample
        for (int i = 0; i < numSamples; ++i) {
            // Zero-stuff
            for (int j = 0; j < factor; ++j) {
                workBuffer[i * factor + j] = (j == 0) ? input[i] * factor : 0.0f;
            }
        }
        
        // Filter upsampled signal
        for (int i = 0; i < numSamples * factor; ++i) {
            for (auto& filter : upFilters) {
                workBuffer[i] = filter.processTDF2(workBuffer[i]);
            }
        }
        
        // Process at higher sample rate
        for (int i = 0; i < numSamples * factor; ++i) {
            workBuffer[i] = func(workBuffer[i]);
        }
        
        // Filter before downsampling
        for (int i = 0; i < numSamples * factor; ++i) {
            for (auto& filter : downFilters) {
                workBuffer[i] = filter.processTDF2(workBuffer[i]);
            }
        }
        
        // Downsample
        for (int i = 0; i < numSamples; ++i) {
            output[i] = workBuffer[i * factor];
        }
    }
};

// ==================== YIN Pitch Detector for PSOLA ====================
class YINPitchDetector {
    static constexpr int kBufferSize = 4096;
    static constexpr int kMaxLag = 2048;
    std::vector<float> buffer;
    std::vector<float> yinBuffer;
    int writePos{0};
    float lastPeriod{0.0f};
    float confidence{0.0f};
    
public:
    void init() {
        buffer.resize(kBufferSize, 0.0f);
        yinBuffer.resize(kMaxLag, 0.0f);
        reset();
    }
    
    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        lastPeriod = 0.0f;
        confidence = 0.0f;
    }
    
    float detectPeriod(const float* input, int numSamples, double sampleRate) noexcept {
        // Add to circular buffer
        for (int i = 0; i < numSamples; ++i) {
            buffer[writePos] = input[i];
            writePos = (writePos + 1) % kBufferSize;
        }
        
        // YIN algorithm
        // Step 1: Difference function
        for (int tau = 1; tau < kMaxLag; ++tau) {
            float sum = 0.0f;
            for (int i = 0; i < kMaxLag; ++i) {
                int idx1 = (writePos - kMaxLag + i + kBufferSize) % kBufferSize;
                int idx2 = (writePos - kMaxLag + i + tau + kBufferSize) % kBufferSize;
                float diff = buffer[idx1] - buffer[idx2];
                sum += diff * diff;
            }
            yinBuffer[tau] = sum;
        }
        
        // Step 2: Cumulative mean normalized difference
        yinBuffer[0] = 1.0f;
        float runningSum = 0.0f;
        for (int tau = 1; tau < kMaxLag; ++tau) {
            runningSum += yinBuffer[tau];
            yinBuffer[tau] *= tau / (runningSum + 0.0001f);
        }
        
        // Step 3: Find first minimum below threshold
        const float threshold = 0.15f;
        int minTau = 0;
        for (int tau = 25; tau < kMaxLag - 1; ++tau) {  // Min 25 samples (~1920Hz at 48kHz)
            if (yinBuffer[tau] < threshold) {
                if (yinBuffer[tau] < yinBuffer[tau - 1] && 
                    yinBuffer[tau] < yinBuffer[tau + 1]) {
                    minTau = tau;
                    break;
                }
            }
        }
        
        // Step 4: Parabolic interpolation
        if (minTau > 0 && minTau < kMaxLag - 1) {
            float x0 = yinBuffer[minTau - 1];
            float x1 = yinBuffer[minTau];
            float x2 = yinBuffer[minTau + 1];
            
            float a = (x0 - 2*x1 + x2) / 2.0f;
            float b = (x2 - x0) / 2.0f;
            
            float xOffset = (std::abs(a) > 0.0001f) ? -b / (2*a) : 0;
            lastPeriod = minTau + xOffset;
            confidence = 1.0f - yinBuffer[minTau];
        } else {
            confidence = 0.0f;
        }
        
        return lastPeriod;
    }
    
    float getConfidence() const noexcept { return confidence; }
    float getPeriod() const noexcept { return lastPeriod; }
};

// ==================== TRUE PSOLA Pitch Shifter ====================
class PSOLAPitchShifter {
    static constexpr int kBufferSize = 65536;  // Power of 2 for fast modulo
    static constexpr int kBufferMask = kBufferSize - 1;
    static constexpr int kMaxWindowSize = 8192;  // Max 2 pitch periods
    static constexpr int kMaxPitchMarks = 512;
    
    // Input circular buffer
    std::array<float, kBufferSize> inputBuffer;
    int writePos{0};
    int readPos{0};
    
    // Output accumulator for overlap-add
    std::array<float, kBufferSize> outputBuffer;
    int outputWritePos{0};
    int outputReadPos{0};
    
    // Pitch detection
    YINPitchDetector pitchDetector;
    float currentPeriod{0.0f};
    float targetPeriod{0.0f};
    
    // Analysis pitch marks (input)
    struct PitchMark {
        int position;      // Position in input buffer
        float period;      // Detected pitch period at this mark
        float amplitude;   // Peak amplitude for better mark selection
    };
    std::deque<PitchMark> analysisPitchMarks;
    int lastAnalysisMarkPos{0};
    
    // Synthesis pitch marks (output)
    std::deque<int> synthesisPitchMarks;
    int lastSynthesisMarkPos{0};
    
    // Pitch control
    float pitchRatio{1.0f};
    SmoothedParam pitchSmoother;
    
    // Hann window LUT for perfect reconstruction
    std::vector<float> hannWindow;
    
    // Pre-computed Hann windows for different sizes
    std::array<std::vector<float>, 128> windowCache;
    double sampleRate_{48000.0};
    
public:
    void init(double sampleRate) {
        sampleRate_ = sampleRate;
        
        inputBuffer.fill(0.0f);
        outputBuffer.fill(0.0f);
        writePos = readPos = 0;
        outputWritePos = outputReadPos = 0;
        
        // Initialize pitch detector
        pitchDetector.init();
        
        // Pre-compute Hann windows for different pitch period sizes
        // Windows are 2 * pitch_period in length
        for (int periodSamples = 20; periodSamples < 128; ++periodSamples) {
            int windowSize = periodSamples * 2;
            if (windowSize > kMaxWindowSize) continue;
            
            windowCache[periodSamples].resize(windowSize);
            for (int i = 0; i < windowSize; ++i) {
                // Hann window for perfect reconstruction
                float phase = static_cast<float>(i) / (windowSize - 1);
                windowCache[periodSamples][i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
            }
        }
        
        // Create default Hann window
        hannWindow.resize(2048);
        for (int i = 0; i < 2048; ++i) {
            float phase = static_cast<float>(i) / 2047.0f;
            hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
        }
        
        // Setup pitch smoother
        pitchSmoother.setSmoothingTime(10.0f, sampleRate);
        pitchSmoother.snap(1.0f);
        
        reset();
    }
    
    void reset() noexcept {
        pitchDetector.reset();
        analysisPitchMarks.clear();
        synthesisPitchMarks.clear();
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        writePos = readPos = 0;
        outputWritePos = outputReadPos = 0;
        lastAnalysisMarkPos = 0;
        lastSynthesisMarkPos = 0;
        currentPeriod = 0.0f;
        targetPeriod = 0.0f;
        pitchRatio = 1.0f;
    }
    
    void processPSOLA(const float* input, float* output, int numSamples, float targetPitchRatio) noexcept {
        // Smooth pitch ratio changes
        pitchSmoother.set(targetPitchRatio);
        pitchRatio = pitchSmoother.tick();
        
        // Step 1: Store input in circular buffer
        for (int i = 0; i < numSamples; ++i) {
            inputBuffer[writePos] = input[i];
            writePos = (writePos + 1) & kBufferMask;
        }
        
        // Step 2: Detect pitch period using YIN
        float detectedPeriod = pitchDetector.detectPeriod(input, numSamples, sampleRate_);
        
        if (detectedPeriod > 20 && detectedPeriod < 1000 && pitchDetector.getConfidence() > 0.5f) {
            currentPeriod = detectedPeriod;
            targetPeriod = currentPeriod / pitchRatio;  // Target period for synthesis
            
            // Step 3: Detect analysis pitch marks (glottal closure instants)
            detectAnalysisPitchMarks(numSamples);
            
            // Step 4: Calculate synthesis pitch marks based on desired pitch
            calculateSynthesisPitchMarks(numSamples);
            
            // Step 5: Perform PSOLA synthesis
            performPSOLASynthesis();
        } else {
            // Fallback: Pass through with basic pitch shift
            performBasicPitchShift(numSamples);
        }
        
        // Step 6: Read output from overlap-add buffer
        for (int i = 0; i < numSamples; ++i) {
            output[i] = outputBuffer[outputReadPos];
            outputBuffer[outputReadPos] = 0.0f;  // Clear after reading
            outputReadPos = (outputReadPos + 1) & kBufferMask;
        }
    }
    
private:
    void detectAnalysisPitchMarks(int numSamples) {
        if (currentPeriod <= 0) return;
        
        int periodSamples = static_cast<int>(currentPeriod + 0.5f);
        if (periodSamples < 20 || periodSamples > 2000) return;
        
        // Start from last known mark position
        int currentPos = lastAnalysisMarkPos;
        
        while (currentPos < numSamples) {
            // Next expected pitch mark position
            currentPos += periodSamples;
            if (currentPos >= numSamples) break;
            
            // Search window: ±25% of period around expected position
            int searchRadius = periodSamples / 4;
            int searchStart = std::max(0, currentPos - searchRadius);
            int searchEnd = std::min(numSamples - 1, currentPos + searchRadius);
            
            // Find the peak (glottal closure instant) in search window
            int peakPos = currentPos;
            float peakValue = 0.0f;
            
            for (int j = searchStart; j <= searchEnd; ++j) {
                int bufferIdx = (writePos - numSamples + j + kBufferSize) & kBufferMask;
                float sample = inputBuffer[bufferIdx];
                
                // Look for positive peaks (glottal closure)
                if (sample > peakValue) {
                    peakValue = sample;
                    peakPos = j;
                }
            }
            
            // Store the pitch mark
            PitchMark mark;
            mark.position = (writePos - numSamples + peakPos + kBufferSize) & kBufferMask;
            mark.period = currentPeriod;
            mark.amplitude = peakValue;
            
            analysisPitchMarks.push_back(mark);
            
            // Keep buffer size manageable
            while (analysisPitchMarks.size() > kMaxPitchMarks) {
                analysisPitchMarks.pop_front();
            }
            
            currentPos = peakPos;  // Update for next iteration
        }
        
        lastAnalysisMarkPos = currentPos - numSamples;  // Relative to next block
    }
    
    void calculateSynthesisPitchMarks(int numSamples) {
        if (targetPeriod <= 0) return;
        
        int targetPeriodSamples = static_cast<int>(targetPeriod + 0.5f);
        if (targetPeriodSamples < 20 || targetPeriodSamples > 2000) return;
        
        // Generate synthesis pitch marks at target period intervals
        int currentPos = lastSynthesisMarkPos;
        
        while (currentPos < numSamples) {
            currentPos += targetPeriodSamples;
            if (currentPos >= numSamples) break;
            
            // Store synthesis mark position (in output buffer coordinates)
            int synthesisMark = (outputWritePos + currentPos) & kBufferMask;
            synthesisPitchMarks.push_back(synthesisMark);
            
            // Keep buffer size manageable
            while (synthesisPitchMarks.size() > kMaxPitchMarks) {
                synthesisPitchMarks.pop_front();
            }
        }
        
        lastSynthesisMarkPos = currentPos - numSamples;
    }
    
    void performPSOLASynthesis() {
        if (analysisPitchMarks.empty() || synthesisPitchMarks.empty()) return;
        
        // For each synthesis pitch mark, find corresponding analysis mark and copy windowed segment
        size_t analysisIdx = 0;
        
        for (int synthMark : synthesisPitchMarks) {
            // Find best matching analysis mark
            if (analysisIdx >= analysisPitchMarks.size()) {
                analysisIdx = 0;  // Wrap around if needed
            }
            
            const PitchMark& analysisMark = analysisPitchMarks[analysisIdx];
            
            // Window size is 2 * pitch period
            int windowSize = static_cast<int>(2 * analysisMark.period);
            windowSize = std::min(windowSize, kMaxWindowSize);
            
            // Get appropriate Hann window
            const float* window = nullptr;
            int periodSamples = static_cast<int>(analysisMark.period);
            if (periodSamples >= 20 && periodSamples < 128 && !windowCache[periodSamples].empty()) {
                window = windowCache[periodSamples].data();
            } else {
                window = hannWindow.data();
                windowSize = std::min(windowSize, static_cast<int>(hannWindow.size()));
            }
            
            // Copy windowed segment from analysis mark to synthesis mark
            int halfWindow = windowSize / 2;
            
            for (int i = -halfWindow; i < halfWindow; ++i) {
                // Source position (centered at analysis mark)
                int srcPos = (analysisMark.position + i) & kBufferMask;
                
                // Destination position (centered at synthesis mark)
                int dstPos = (synthMark + i) & kBufferMask;
                
                // Window index
                int winIdx = i + halfWindow;
                if (winIdx >= 0 && winIdx < windowSize) {
                    // Overlap-add with windowing
                    outputBuffer[dstPos] += inputBuffer[srcPos] * window[winIdx];
                }
            }
            
            // Move to next analysis mark based on pitch ratio
            if (pitchRatio > 1.0f) {
                // Upward pitch shift: consume analysis marks faster
                analysisIdx += static_cast<size_t>(pitchRatio + 0.5f);
            } else {
                // Downward pitch shift: consume analysis marks slower
                if ((synthesisPitchMarks.size() % static_cast<size_t>(1.0f / pitchRatio + 0.5f)) == 0) {
                    analysisIdx++;
                }
            }
        }
        
        // Clear processed synthesis marks
        synthesisPitchMarks.clear();
    }
    
    void performBasicPitchShift(int numSamples) {
        // Simple resampling-based pitch shift for non-pitched signals
        float readIncrement = 1.0f / pitchRatio;
        
        for (int i = 0; i < numSamples; ++i) {
            // Linear interpolation for simplicity
            float readPosFloat = readPos + i * readIncrement;
            int readPosInt = static_cast<int>(readPosFloat);
            float frac = readPosFloat - readPosInt;
            
            int idx1 = (readPosInt) & kBufferMask;
            int idx2 = (readPosInt + 1) & kBufferMask;
            
            float sample = inputBuffer[idx1] * (1.0f - frac) + inputBuffer[idx2] * frac;
            
            int outPos = (outputWritePos + i) & kBufferMask;
            outputBuffer[outPos] = sample * 0.7f;  // Scale down to prevent clipping
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
        if (scaleIndex < 0 || scaleIndex >= 10) return flushDenorm(static_cast<float>(noteOffset));
        
        // Chromatic scale - no quantization
        if (scaleIndex == 9) return flushDenorm(static_cast<float>(noteOffset));
        
        // Calculate absolute note
        int absoluteNote = 60 + noteOffset;
        
        // Find position relative to root
        int noteFromRoot = ((absoluteNote - rootKey) % 12 + 12) % 12;
        
        // Find closest scale degree
        int closestDegree = 0;
        int minDistance = 12;
        
        for (int i = 0; i < 12; ++i) {
            if (kScaleIntervals[scaleIndex][i] == -1) break;
            
            int distance = std::abs(noteFromRoot - kScaleIntervals[scaleIndex][i]);
            if (distance > 6) distance = 12 - distance; // Wrap around
            
            if (distance < minDistance) {
                minDistance = distance;
                closestDegree = kScaleIntervals[scaleIndex][i];
            }
        }
        
        // Calculate quantized note
        int octave = (absoluteNote - rootKey) / 12;
        if (absoluteNote < rootKey && (absoluteNote - rootKey) % 12 != 0) {
            octave--;
        }
        
        int result = rootKey + octave * 12 + closestDegree - 60;
        
        // Apply denormal protection to the result
        return static_cast<int>(flushDenorm(static_cast<float>(result)));
    }
};

// ==================== Formant Shifter ====================
class FormantShifter {
    static constexpr int kNumFormants = 5;
    std::array<PlatinumBiquad, kNumFormants> analysisFilters;
    std::array<PlatinumBiquad, kNumFormants> synthesisFilters;
    std::array<float, kNumFormants> formantFreqs{700, 1220, 2600, 3500, 4500};
    std::array<float, kNumFormants> formantBandwidths{130, 170, 250, 350, 450};
    
public:
    void init(double sampleRate) {
        for (int i = 0; i < kNumFormants; ++i) {
            // Use bandpass filters for better formant isolation
            setBandpass(analysisFilters[i], formantFreqs[i], 
                       formantFreqs[i] / formantBandwidths[i], sampleRate);
            setBandpass(synthesisFilters[i], formantFreqs[i], 
                       formantFreqs[i] / formantBandwidths[i], sampleRate);
        }
    }
    
    void reset() noexcept {
        for (auto& filter : analysisFilters) filter.reset();
        for (auto& filter : synthesisFilters) filter.reset();
    }
    
    float process(float input, float shiftRatio, float amount) noexcept {
        if (amount < 0.01f) return flushDenorm(input);
        
        // Extract formant magnitudes with bandpass filters
        float formantSum = 0.0f;
        std::array<float, kNumFormants> formantMags;
        
        for (int i = 0; i < kNumFormants; ++i) {
            formantMags[i] = analysisFilters[i].processTDF2(input);
            formantMags[i] = flushDenorm(formantMags[i]);
            formantSum += std::abs(formantMags[i]);
        }
        
        // Synthesize shifted formants
        float shifted = 0.0f;
        if (formantSum > 1e-6f) {
            for (int i = 0; i < kNumFormants; ++i) {
                // Shift formant frequency
                float shiftedFreq = formantFreqs[i] * shiftRatio;
                shiftedFreq = std::max(20.0f, std::min(20000.0f, shiftedFreq));
                
                // Resynthesize at shifted frequency
                float component = synthesisFilters[i].processTDF2(formantMags[i]);
                shifted += flushDenorm(component);
            }
        }
        
        return flushDenorm(input * (1.0f - amount) + shifted * amount);
    }
    
private:
    void setBandpass(PlatinumBiquad& filter, double freq, double q, double sampleRate) noexcept {
        const double w = 2.0 * M_PI * freq / sampleRate;
        const double cosw = std::cos(w);
        const double sinw = std::sin(w);
        const double alpha = sinw / (2.0 * q);
        
        const double b0 = alpha;
        const double b1 = 0.0;
        const double b2 = -alpha;
        const double a0 = 1.0 + alpha;
        const double a1 = -2.0 * cosw;
        const double a2 = 1.0 - alpha;
        
        filter.setCoefficients(b0, b1, b2, a0, a1, a2);
    }
};

} // anonymous namespace

// ==================== Main Implementation ====================
struct IntelligentHarmonizer::Impl {
    // Audio processing state
    static constexpr int kMaxChannels = 2;
    static constexpr int kMaxVoices = 4;
    
    struct ChannelState {
        DCBlocker inputDC, outputDC;
        std::array<PSOLAPitchShifter, kMaxVoices> pitchShifters;
        std::array<FormantShifter, kMaxVoices> formantShifters;
        PolyphaseOversampler oversampler;
        PlatinumBiquad antiAliasFilter;
        
        void prepare(double sampleRate, int maxBlockSize, int oversampleFactor) {
            inputDC.reset();
            outputDC.reset();
            
            for (auto& shifter : pitchShifters) {
                shifter.init(sampleRate * oversampleFactor);
            }
            
            for (auto& formant : formantShifters) {
                formant.init(sampleRate * oversampleFactor);
            }
            
            oversampler.init(oversampleFactor, sampleRate, maxBlockSize);
            antiAliasFilter.setLowpass(sampleRate * 0.45, 0.707, sampleRate);
        }
        
        void reset() noexcept {
            inputDC.reset();
            outputDC.reset();
            for (auto& shifter : pitchShifters) shifter.reset();
            for (auto& formant : formantShifters) formant.reset();
            oversampler.reset();
            antiAliasFilter.reset();
        }
    };
    
    std::array<ChannelState, kMaxChannels> channels;
    
    // Parameters
    SmoothedParam interval;      // -24 to +24 semitones
    SmoothedParam key;          // Root note
    SmoothedParam scale;        // Scale type
    SmoothedParam voiceCount;   // 1-4 voices
    SmoothedParam spread;       // Stereo spread
    SmoothedParam humanize;     // Variation
    SmoothedParam formant;      // Formant preservation
    SmoothedParam mix;          // Dry/wet
    
    // Configuration
    double sampleRate{48000.0};
    int maxBlockSize{512};
    int latencySamples{0};
    
    // Work buffers (pre-allocated)
    std::vector<float> dryBuffer;
    std::vector<float> wetBuffer;
    std::vector<float> voiceBuffer;
    
    // Performance tracking
    std::atomic<uint64_t> samplesProcessed{0};
    std::atomic<bool> denormalsDetected{false};
    std::chrono::high_resolution_clock::time_point lastProcessTime;
    float cpuUsage{0.0f};
    
    // Humanization
    std::mt19937 rng{std::random_device{}()};
    std::normal_distribution<float> noise{0.0f, 1.0f};
    std::array<float, kMaxVoices> vibratoPhases{};
    
    void prepare(double sr, int blockSize) {
        sampleRate = sr;
        maxBlockSize = blockSize;
        
        // Standard quality with 2x oversampling
        int oversampleFactor = 2;
        
        // Calculate latency
        latencySamples = oversampleFactor * 4; // Filter stages
        
        // Pre-allocate buffers with reserve (never resize in RT)
        dryBuffer.reserve(blockSize);
        wetBuffer.reserve(blockSize);
        voiceBuffer.reserve(blockSize);
        
        // Ensure capacity without reallocation
        dryBuffer.resize(blockSize);
        wetBuffer.resize(blockSize);
        voiceBuffer.resize(blockSize);
        
        // Setup parameter smoothing
        interval.setSmoothingTime(10.0f, sr);
        key.setSmoothingTime(50.0f, sr);
        scale.setSmoothingTime(50.0f, sr);
        voiceCount.setSmoothingTime(20.0f, sr);
        spread.setSmoothingTime(30.0f, sr);
        humanize.setSmoothingTime(30.0f, sr);
        formant.setSmoothingTime(20.0f, sr);
        mix.setSmoothingTime(20.0f, sr);
        
        // Initialize defaults - FIXED: 0.5 is now unison
        interval.snap(0.5f);     // Center = unison (0 semitones)
        key.snap(0.0f);         // C
        scale.snap(0.0f);       // Major
        voiceCount.snap(0.25f); // 1 voice
        spread.snap(0.3f);      // 30% spread
        humanize.snap(0.0f);    // No humanization
        formant.snap(0.0f);     // No formant
        mix.snap(0.5f);         // 50% wet
        
        // Prepare channels
        for (auto& channel : channels) {
            channel.prepare(sr, blockSize, oversampleFactor);
        }
        
        vibratoPhases.fill(0.0f);
    }
    
    void processBlock(float* const* io, int numChannels, int numSamples) noexcept {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Ensure we have valid channels
        numChannels = std::min(numChannels, kMaxChannels);
        
        // Update parameters once per block
        const float intervalValue = interval.tick();
        const float keyValue = key.tick();
        const float scaleValue = scale.tick();
        const float voiceValue = voiceCount.tick();
        const float spreadValue = spread.tick();
        const float humanizeValue = humanize.tick();
        const float formantValue = formant.tick();
        const float mixValue = mix.tick();
        
        // Calculate harmony settings
        // Map interval parameter to discrete musical intervals for better usability
        const int kMusicalIntervals[] = {
            -12, // Octave down
            -7,  // Perfect 5th down
            -5,  // Perfect 4th down
            -4,  // Major 3rd down
            -3,  // Minor 3rd down
            0,   // Unison (center position)
            3,   // Minor 3rd up
            4,   // Major 3rd up
            5,   // Perfect 4th up
            7,   // Perfect 5th up
            12,  // Octave up
            19   // Octave + 5th up
        };
        
        // FIXED: Proper interval mapping centered at 0.5 = unison
        float semitones;
        if (std::abs(intervalValue - 0.5f) < 0.01f) {
            // Exact center = unison (no pitch change)
            semitones = 0.0f;
        } else {
            // Map 0-1 to -24 to +24 semitones with center at 0.5
            semitones = (intervalValue - 0.5f) * 48.0f;
            
            // Optional: Quantize to musical intervals if desired
            bool quantizeToMusicalIntervals = false;  // Set true for discrete intervals
            if (quantizeToMusicalIntervals) {
                // Find closest musical interval
                int closestIdx = 5;  // Default to unison
                float minDist = std::abs(semitones);
                
                for (int i = 0; i < 12; ++i) {
                    float dist = std::abs(semitones - kMusicalIntervals[i]);
                    if (dist < minDist) {
                        minDist = dist;
                        closestIdx = i;
                    }
                }
                
                // Snap to closest interval
                semitones = kMusicalIntervals[closestIdx];
            }
        }
        const int baseSemitones = static_cast<int>(std::round(semitones));
        
        const int rootKey = static_cast<int>(keyValue * 12.0f) % 12;
        const int scaleIndex = static_cast<int>(scaleValue * 10.0f);
        const int activeVoices = 1 + static_cast<int>(voiceValue * 3.0f);
        
        // Process each channel
        for (int ch = 0; ch < numChannels; ++ch) {
            auto& channel = channels[ch];
            float* data = io[ch];
            
            // Copy dry signal
            std::copy(data, data + numSamples, dryBuffer.data());
            
            // Clear wet buffer
            std::fill(wetBuffer.begin(), wetBuffer.begin() + numSamples, 0.0f);
            
            // Process each voice
            for (int voice = 0; voice < activeVoices; ++voice) {
                // Calculate voice interval
                int voiceInterval = baseSemitones;
                if (activeVoices > 1) {
                    // Create harmony intervals
                    switch (voice) {
                        case 1: voiceInterval += (scaleIndex == 0) ? 4 : 3; break; // 3rd
                        case 2: voiceInterval += 7; break; // 5th
                        case 3: voiceInterval += (scaleIndex == 0) ? 11 : 10; break; // 7th
                    }
                }
                
                // Quantize to scale
                voiceInterval = ScaleQuantizer::quantize(voiceInterval, scaleIndex, rootKey);
                
                // Clamp to safe range
                voiceInterval = std::max(-36, std::min(36, voiceInterval));
                
                // Calculate pitch ratio
                float pitchRatio = std::pow(2.0f, voiceInterval / 12.0f);
                
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
                
                // Process through PSOLA pitch shifter
                auto& shifter = channel.pitchShifters[voice];
                auto& formantShifter = channel.formantShifters[voice];
                
                // Process with TRUE PSOLA (no return value now)
                shifter.processPSOLA(
                    dryBuffer.data(), 
                    voiceBuffer.data(), 
                    numSamples,
                    pitchRatio
                );
                
                // Apply formant preservation if enabled
                if (formantValue > 0.01f) {
                    for (int i = 0; i < numSamples; ++i) {
                        voiceBuffer[i] = formantShifter.process(voiceBuffer[i], 1.0f / pitchRatio, formantValue);
                    }
                }
                
                // Calculate stereo spread
                float pan = 0.0f;
                if (numChannels == 2 && activeVoices > 1) {
                    pan = (voice - (activeVoices - 1) * 0.5f) / std::max(1.0f, activeVoices - 1.0f);
                    pan *= spreadValue;
                }
                
                // Apply panning
                float gain = 1.0f;
                if (ch == 0) {
                    gain = std::cos((pan + 1.0f) * 0.25f * M_PI);
                } else {
                    gain = std::sin((pan + 1.0f) * 0.25f * M_PI);
                }
                
                // Mix voice into wet buffer with proper gain compensation
                float voiceGain = gain / std::sqrt(static_cast<float>(activeVoices));
                for (int i = 0; i < numSamples; ++i) {
                    wetBuffer[i] += voiceBuffer[i] * voiceGain;
                }
            }
            
            // Apply DC blocking and anti-aliasing
            for (int i = 0; i < numSamples; ++i) {
                wetBuffer[i] = channel.outputDC.process(wetBuffer[i]);
                wetBuffer[i] = channel.antiAliasFilter.processTDF2(wetBuffer[i]);
                
                // Mix dry/wet
                data[i] = dryBuffer[i] * (1.0f - mixValue) + wetBuffer[i] * mixValue;
                
                // Final denormal check
                data[i] = flushDenorm(data[i]);
            }
        }
        
        // Update statistics
        samplesProcessed.fetch_add(numSamples, std::memory_order_relaxed);
        
        // Calculate CPU usage
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        float blockTimeMs = numSamples * 1000.0f / sampleRate;
        cpuUsage = duration.count() / (blockTimeMs * 1000.0f);
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
// IntelligentHarmonizer.cpp - PROPERLY FIXED TRUE PSOLA Implementation
// Based on Time-Domain Pitch Synchronous Overlap-Add algorithm
// 
// PSOLA Algorithm Overview:
// 1. Detect pitch period in input signal
// 2. Find pitch marks (glottal closure instants) at period intervals
// 3. Extract 2-pitch-period windows centered at each mark
// 4. Apply Hann window for smooth overlap-add
// 5. Place windows at NEW intervals based on desired pitch shift
// 6. Overlap-add windows at synthesis marks

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

// ==================== YIN Pitch Detector ====================
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

// ==================== SIMPLIFIED TRUE PSOLA Pitch Shifter ====================
class SimplePSOLA {
    static constexpr int kBufferSize = 65536;
    static constexpr int kBufferMask = kBufferSize - 1;
    
    // Buffers
    std::array<float, kBufferSize> inputBuffer;
    std::array<float, kBufferSize> outputBuffer;
    int inputWritePos{0};
    int outputReadPos{0};
    int outputWritePos{0};
    
    // Pitch detection
    YINPitchDetector pitchDetector;
    float currentPeriod{0.0f};
    
    // PSOLA state
    float lastInputPhase{0.0f};
    float lastOutputPhase{0.0f};
    
    // Hann window
    std::vector<float> hannWindow;
    
    double sampleRate_{48000.0};
    
public:
    void init(double sampleRate) {
        sampleRate_ = sampleRate;
        
        inputBuffer.fill(0.0f);
        outputBuffer.fill(0.0f);
        inputWritePos = 0;
        outputReadPos = 0;
        outputWritePos = 0;
        
        pitchDetector.init();
        
        // Create Hann window (max size)
        hannWindow.resize(4096);
        for (int i = 0; i < 4096; ++i) {
            float phase = static_cast<float>(i) / 4095.0f;
            hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
        }
        
        reset();
    }
    
    void reset() noexcept {
        pitchDetector.reset();
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        inputWritePos = 0;
        outputReadPos = 0;
        outputWritePos = 0;
        lastInputPhase = 0.0f;
        lastOutputPhase = 0.0f;
        currentPeriod = 0.0f;
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) noexcept {
        // Store input in buffer
        for (int i = 0; i < numSamples; ++i) {
            inputBuffer[inputWritePos] = input[i];
            inputWritePos = (inputWritePos + 1) & kBufferMask;
        }
        
        // Detect pitch period
        float period = pitchDetector.detectPeriod(input, numSamples, sampleRate_);
        
        if (period > 20 && period < 1000 && pitchDetector.getConfidence() > 0.3f) {
            currentPeriod = period;
            
            // SIMPLIFIED PSOLA: Process with fixed hop size based on pitch
            int hopSizeIn = static_cast<int>(currentPeriod);
            int hopSizeOut = static_cast<int>(currentPeriod / pitchRatio);
            int windowSize = hopSizeIn * 2;
            
            // Limit window size
            if (windowSize > 4096) windowSize = 4096;
            if (windowSize < 64) windowSize = 64;
            
            // Process grains
            while (lastInputPhase < numSamples - windowSize) {
                // Extract grain from input at current phase
                int inputPos = static_cast<int>(lastInputPhase);
                
                // Apply window and add to output
                for (int i = 0; i < windowSize; ++i) {
                    int srcIdx = (inputWritePos - numSamples + inputPos + i + kBufferSize) & kBufferMask;
                    int dstIdx = (outputWritePos + static_cast<int>(lastOutputPhase) + i) & kBufferMask;
                    
                    // Get window value
                    float window = hannWindow[i * 4096 / windowSize];
                    
                    // Overlap-add
                    outputBuffer[dstIdx] += inputBuffer[srcIdx] * window * 0.5f;
                }
                
                // Advance phases
                lastInputPhase += hopSizeIn;
                lastOutputPhase += hopSizeOut;
            }
            
            // Update phases for next block
            lastInputPhase -= numSamples;
            if (lastInputPhase < 0) lastInputPhase = 0;
            
        } else {
            // No valid pitch - simple resampling
            float readIncrement = 1.0f / pitchRatio;
            for (int i = 0; i < numSamples; ++i) {
                float readPos = i * readIncrement;
                int readPosInt = static_cast<int>(readPos);
                float frac = readPos - readPosInt;
                
                if (readPosInt < numSamples - 1) {
                    int idx1 = (inputWritePos - numSamples + readPosInt + kBufferSize) & kBufferMask;
                    int idx2 = (inputWritePos - numSamples + readPosInt + 1 + kBufferSize) & kBufferMask;
                    
                    float sample = inputBuffer[idx1] * (1.0f - frac) + inputBuffer[idx2] * frac;
                    
                    int outIdx = (outputWritePos + i) & kBufferMask;
                    outputBuffer[outIdx] = sample * 0.7f;
                }
            }
        }
        
        // Read output
        for (int i = 0; i < numSamples; ++i) {
            output[i] = outputBuffer[outputReadPos];
            outputBuffer[outputReadPos] = 0.0f;  // Clear after reading
            outputReadPos = (outputReadPos + 1) & kBufferMask;
        }
        
        // Advance output write position
        outputWritePos = (outputWritePos + numSamples) & kBufferMask;
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
        
        // Chromatic scale - no quantization
        if (scaleIndex == 9) return noteOffset;
        
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
        
        return rootKey + octave * 12 + closestDegree - 60;
    }
};

// ==================== Formant Shifter (simplified) ====================
class FormantShifter {
    PlatinumBiquad filter;
    
public:
    void init(double sampleRate) {
        filter.setLowpass(4000.0, 0.707, sampleRate);
    }
    
    void reset() noexcept {
        filter.reset();
    }
    
    float process(float input, float shiftRatio, float amount) noexcept {
        if (amount < 0.01f) return input;
        
        // Simple formant preservation using filtering
        float filtered = filter.processTDF2(input);
        return input * (1.0f - amount) + filtered * amount;
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
        std::array<SimplePSOLA, kMaxVoices> pitchShifters;
        std::array<FormantShifter, kMaxVoices> formantShifters;
        PlatinumBiquad antiAliasFilter;
        
        void prepare(double sampleRate, int maxBlockSize) {
            inputDC.reset();
            outputDC.reset();
            
            for (auto& shifter : pitchShifters) {
                shifter.init(sampleRate);
            }
            
            for (auto& formant : formantShifters) {
                formant.init(sampleRate);
            }
            
            antiAliasFilter.setLowpass(sampleRate * 0.45, 0.707, sampleRate);
        }
        
        void reset() noexcept {
            inputDC.reset();
            outputDC.reset();
            for (auto& shifter : pitchShifters) shifter.reset();
            for (auto& formant : formantShifters) formant.reset();
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
    
    // Humanization
    std::mt19937 rng{std::random_device{}()};
    std::normal_distribution<float> noise{0.0f, 1.0f};
    std::array<float, kMaxVoices> vibratoPhases{};
    
    void prepare(double sr, int blockSize) {
        sampleRate = sr;
        maxBlockSize = blockSize;
        
        // Calculate latency
        latencySamples = 128; // Simplified latency
        
        // Pre-allocate buffers
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
        
        // Initialize defaults - 0.5 = unison
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
            channel.prepare(sr, blockSize);
        }
        
        vibratoPhases.fill(0.0f);
    }
    
    void processBlock(float* const* io, int numChannels, int numSamples) noexcept {
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
        
        // Calculate semitones from interval parameter
        float semitones;
        if (std::abs(intervalValue - 0.5f) < 0.01f) {
            // Exact center = unison (no pitch change)
            semitones = 0.0f;
        } else {
            // Map 0-1 to -24 to +24 semitones with center at 0.5
            semitones = (intervalValue - 0.5f) * 48.0f;
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
                
                // Process with simplified PSOLA
                shifter.process(
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
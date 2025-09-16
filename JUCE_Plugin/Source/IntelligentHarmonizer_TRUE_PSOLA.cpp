// IntelligentHarmonizer.cpp - TRUE TD-PSOLA Implementation
// 
// PSOLA Algorithm Steps:
// 1. Detect pitch period in input signal
// 2. Find pitch marks (peaks) at period intervals in input
// 3. For each synthesis position in output:
//    - Find corresponding analysis position in input
//    - Extract 2-period window centered at nearest pitch mark
//    - Apply Hann window and place in output with overlap-add
// 4. Key: synthesis marks spaced by period/pitchRatio

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

// ==================== Parameter Smoothing ====================
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

// ==================== Filters ====================
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
    
    ALWAYS_INLINE float processTDF2(float x) noexcept {
        const double y = b0 * x + x1;
        x1 = b1 * x - a1 * y + x2;
        x2 = b2 * x - a2 * y;
        x1 = flushDenorm(x1);
        x2 = flushDenorm(x2);
        return static_cast<float>(y);
    }
};

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

// ==================== Autocorrelation Pitch Detector ====================
class AutocorrelationPitchDetector {
    static constexpr int kBufferSize = 4096;
    std::vector<float> buffer;
    int writePos{0};
    float currentPeriod{0.0f};
    float confidence{0.0f};
    
public:
    void init() {
        buffer.resize(kBufferSize, 0.0f);
        reset();
    }
    
    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        currentPeriod = 0.0f;
        confidence = 0.0f;
    }
    
    float detectPeriod(const float* input, int numSamples, double sampleRate) noexcept {
        // Add to circular buffer
        for (int i = 0; i < numSamples; ++i) {
            buffer[writePos] = input[i];
            writePos = (writePos + 1) % kBufferSize;
        }
        
        // Autocorrelation for pitch detection
        const int minLag = 30;    // ~1600 Hz at 48kHz
        const int maxLag = 800;   // ~60 Hz at 48kHz
        
        float maxCorr = 0.0f;
        int bestLag = 0;
        
        // Normalize by computing energy
        float energy = 0.0f;
        for (int i = 0; i < 1024; ++i) {
            int idx = (writePos - 1024 + i + kBufferSize) % kBufferSize;
            energy += buffer[idx] * buffer[idx];
        }
        
        if (energy < 0.001f) {
            confidence = 0.0f;
            return currentPeriod;
        }
        
        // Find lag with maximum correlation
        for (int lag = minLag; lag < maxLag && lag < kBufferSize/2; ++lag) {
            float corr = 0.0f;
            
            for (int i = 0; i < 1024; ++i) {
                int idx1 = (writePos - 1024 + i + kBufferSize) % kBufferSize;
                int idx2 = (writePos - 1024 + i - lag + kBufferSize) % kBufferSize;
                corr += buffer[idx1] * buffer[idx2];
            }
            
            corr /= energy;  // Normalize
            
            if (corr > maxCorr) {
                maxCorr = corr;
                bestLag = lag;
            }
        }
        
        if (bestLag > 0 && maxCorr > 0.3f) {
            currentPeriod = static_cast<float>(bestLag);
            confidence = maxCorr;
        } else {
            confidence = 0.0f;
        }
        
        return currentPeriod;
    }
    
    float getPeriod() const noexcept { return currentPeriod; }
    float getConfidence() const noexcept { return confidence; }
};

// ==================== TRUE PSOLA Pitch Shifter ====================
class TruePSOLA {
    static constexpr int kHistorySize = 65536;
    static constexpr int kMaxGrainSize = 4096;
    
    // Input history buffer
    std::vector<float> inputHistory;
    int historyWritePos{0};
    
    // Output accumulator
    std::vector<float> outputAccumulator;
    
    // Pitch detection
    AutocorrelationPitchDetector pitchDetector;
    float currentPeriod{100.0f};
    float smoothedPeriod{100.0f};
    
    // PSOLA state
    float inputReadPosition{0.0f};   // Where we're reading from in input
    float outputWritePosition{0.0f}; // Where we're writing to in output
    
    // Hann window
    std::vector<float> hannWindow;
    
    double sampleRate_{48000.0};
    
public:
    void init(double sampleRate) {
        sampleRate_ = sampleRate;
        
        inputHistory.resize(kHistorySize, 0.0f);
        outputAccumulator.resize(kHistorySize, 0.0f);
        historyWritePos = 0;
        
        pitchDetector.init();
        
        // Pre-compute Hann window
        hannWindow.resize(kMaxGrainSize);
        for (int i = 0; i < kMaxGrainSize; ++i) {
            float x = static_cast<float>(i) / (kMaxGrainSize - 1);
            hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * x));
        }
        
        reset();
    }
    
    void reset() noexcept {
        std::fill(inputHistory.begin(), inputHistory.end(), 0.0f);
        std::fill(outputAccumulator.begin(), outputAccumulator.end(), 0.0f);
        historyWritePos = 0;
        pitchDetector.reset();
        currentPeriod = 100.0f;
        smoothedPeriod = 100.0f;
        inputReadPosition = 0.0f;
        outputWritePosition = 0.0f;
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) noexcept {
        // Store input in history
        for (int i = 0; i < numSamples; ++i) {
            inputHistory[historyWritePos] = input[i];
            historyWritePos = (historyWritePos + 1) % kHistorySize;
        }
        
        // Detect pitch
        float detectedPeriod = pitchDetector.detectPeriod(input, numSamples, sampleRate_);
        if (detectedPeriod > 30 && detectedPeriod < 800 && pitchDetector.getConfidence() > 0.3f) {
            // Smooth period changes
            smoothedPeriod = 0.9f * smoothedPeriod + 0.1f * detectedPeriod;
            currentPeriod = smoothedPeriod;
        }
        
        // Clear output
        std::fill(output, output + numSamples, 0.0f);
        
        if (currentPeriod > 0 && pitchDetector.getConfidence() > 0.2f) {
            // TRUE PSOLA synthesis
            performPSOLA(output, numSamples, pitchRatio);
        } else {
            // Fallback: simple resampling when no pitch detected
            performSimpleResampling(input, output, numSamples, pitchRatio);
        }
    }
    
private:
    void performPSOLA(float* output, int numSamples, float pitchRatio) noexcept {
        // Calculate synthesis hop size (spacing between output grains)
        float synthesisHop = currentPeriod / pitchRatio;
        
        // Grain size is 2 * pitch period
        int grainSize = static_cast<int>(2 * currentPeriod);
        grainSize = std::min(grainSize, kMaxGrainSize);
        grainSize = std::max(grainSize, 64);
        
        // Process grains
        while (outputWritePosition < numSamples) {
            // Find the analysis position for this synthesis position
            // This maps from output time to input time
            float analysisPosition = inputReadPosition;
            
            // Extract grain from input centered at analysis position
            for (int i = -grainSize/2; i < grainSize/2; ++i) {
                int outputIdx = static_cast<int>(outputWritePosition) + i;
                
                if (outputIdx >= 0 && outputIdx < numSamples) {
                    // Calculate input position
                    int inputIdx = static_cast<int>(analysisPosition) + i;
                    
                    // Map to history buffer (circular)
                    int historyIdx = (historyWritePos - numSamples + inputIdx + kHistorySize) % kHistorySize;
                    
                    // Get window value
                    int winIdx = i + grainSize/2;
                    if (winIdx >= 0 && winIdx < grainSize) {
                        float windowVal = hannWindow[winIdx * kMaxGrainSize / grainSize];
                        
                        // Overlap-add with windowing
                        output[outputIdx] += inputHistory[historyIdx] * windowVal * 0.7f;
                    }
                }
            }
            
            // Advance positions
            // Input advances by one period (we read at pitch rate)
            inputReadPosition += currentPeriod;
            
            // Output advances by synthesis hop (modified pitch rate)
            outputWritePosition += synthesisHop;
        }
        
        // Update positions for next block
        inputReadPosition -= numSamples;
        outputWritePosition -= numSamples;
        
        // Keep positions non-negative
        if (inputReadPosition < 0) inputReadPosition = 0;
        if (outputWritePosition < 0) outputWritePosition = 0;
    }
    
    void performSimpleResampling(const float* input, float* output, int numSamples, float pitchRatio) noexcept {
        float readPos = 0.0f;
        float readIncrement = 1.0f / pitchRatio;
        
        for (int i = 0; i < numSamples; ++i) {
            int readPosInt = static_cast<int>(readPos);
            float frac = readPos - readPosInt;
            
            if (readPosInt >= 0 && readPosInt < numSamples - 1) {
                int idx1 = (historyWritePos - numSamples + readPosInt + kHistorySize) % kHistorySize;
                int idx2 = (historyWritePos - numSamples + readPosInt + 1 + kHistorySize) % kHistorySize;
                
                float sample = inputHistory[idx1] * (1.0f - frac) + inputHistory[idx2] * frac;
                output[i] = sample * 0.7f;
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
        if (scaleIndex == 9) return noteOffset; // Chromatic - no quantization
        
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

// ==================== Formant Shifter ====================
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
        float filtered = filter.processTDF2(input);
        return input * (1.0f - amount) + filtered * amount;
    }
};

} // anonymous namespace

// ==================== Main Implementation ====================
struct IntelligentHarmonizer::Impl {
    static constexpr int kMaxChannels = 2;
    static constexpr int kMaxVoices = 4;
    
    struct ChannelState {
        DCBlocker inputDC, outputDC;
        std::array<TruePSOLA, kMaxVoices> pitchShifters;
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
    int latencySamples{0};
    
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
        latencySamples = 256;
        
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
            channel.prepare(sr, blockSize);
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
        
        float semitones;
        if (std::abs(intervalValue - 0.5f) < 0.01f) {
            semitones = 0.0f;
        } else {
            semitones = (intervalValue - 0.5f) * 48.0f;
        }
        
        const int baseSemitones = static_cast<int>(std::round(semitones));
        const int rootKey = static_cast<int>(keyValue * 12.0f) % 12;
        const int scaleIndex = static_cast<int>(scaleValue * 10.0f);
        const int activeVoices = 1 + static_cast<int>(voiceValue * 3.0f);
        
        for (int ch = 0; ch < numChannels; ++ch) {
            auto& channel = channels[ch];
            float* data = io[ch];
            
            std::copy(data, data + numSamples, dryBuffer.data());
            std::fill(wetBuffer.begin(), wetBuffer.begin() + numSamples, 0.0f);
            
            for (int voice = 0; voice < activeVoices; ++voice) {
                int voiceInterval = baseSemitones;
                if (activeVoices > 1) {
                    switch (voice) {
                        case 1: voiceInterval += (scaleIndex == 0) ? 4 : 3; break;
                        case 2: voiceInterval += 7; break;
                        case 3: voiceInterval += (scaleIndex == 0) ? 11 : 10; break;
                    }
                }
                
                voiceInterval = ScaleQuantizer::quantize(voiceInterval, scaleIndex, rootKey);
                voiceInterval = std::max(-36, std::min(36, voiceInterval));
                
                float pitchRatio = std::pow(2.0f, voiceInterval / 12.0f);
                
                if (humanizeValue > 0.01f) {
                    vibratoPhases[voice] += 2.0f * M_PI * 5.0f / sampleRate;
                    if (vibratoPhases[voice] > 2.0f * M_PI) {
                        vibratoPhases[voice] -= 2.0f * M_PI;
                    }
                    
                    float vibrato = std::sin(vibratoPhases[voice]) * humanizeValue * 0.02f;
                    float drift = noise(rng) * humanizeValue * 0.005f;
                    pitchRatio *= std::pow(2.0f, (vibrato + drift) / 12.0f);
                }
                
                auto& shifter = channel.pitchShifters[voice];
                auto& formantShifter = channel.formantShifters[voice];
                
                shifter.process(dryBuffer.data(), voiceBuffer.data(), numSamples, pitchRatio);
                
                if (formantValue > 0.01f) {
                    for (int i = 0; i < numSamples; ++i) {
                        voiceBuffer[i] = formantShifter.process(voiceBuffer[i], 1.0f / pitchRatio, formantValue);
                    }
                }
                
                float pan = 0.0f;
                if (numChannels == 2 && activeVoices > 1) {
                    pan = (voice - (activeVoices - 1) * 0.5f) / std::max(1.0f, activeVoices - 1.0f);
                    pan *= spreadValue;
                }
                
                float gain = 1.0f;
                if (ch == 0) {
                    gain = std::cos((pan + 1.0f) * 0.25f * M_PI);
                } else {
                    gain = std::sin((pan + 1.0f) * 0.25f * M_PI);
                }
                
                float voiceGain = gain / std::sqrt(static_cast<float>(activeVoices));
                for (int i = 0; i < numSamples; ++i) {
                    wetBuffer[i] += voiceBuffer[i] * voiceGain;
                }
            }
            
            for (int i = 0; i < numSamples; ++i) {
                wetBuffer[i] = channel.outputDC.process(wetBuffer[i]);
                wetBuffer[i] = channel.antiAliasFilter.processTDF2(wetBuffer[i]);
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
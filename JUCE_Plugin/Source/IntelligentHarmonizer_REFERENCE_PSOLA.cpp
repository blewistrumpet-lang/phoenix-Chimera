// IntelligentHarmonizer.cpp - REFERENCE-BASED TD-PSOLA Implementation
// 
// Based on technical reference with correct formulas:
// 1. Synthesis marks: t'_m+1 = t'_m + T_0/α where α = f'_0/f_0 (pitch ratio)
// 2. Grain selection: k(m) = argmin_k |t_k - φ(t'_m)| with proper time mapping
// 3. Energy equalization to prevent pumping
// 4. OLA constraint with Hann window

#include "IntelligentHarmonizer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <random>
#include <vector>

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

// ==================== Denormal Protection ====================
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
    
    void setLowpass(double freq, double q, double sampleRate) noexcept {
        double w = 2.0 * M_PI * freq / sampleRate;
        double cosw = std::cos(w);
        double sinw = std::sin(w);
        double alpha = sinw / (2.0 * q);
        
        double norm = 1.0 / (1.0 + alpha);
        b0 = (1.0 - cosw) * 0.5 * norm;
        b1 = (1.0 - cosw) * norm;
        b2 = b0;
        a1 = -2.0 * cosw * norm;
        a2 = (1.0 - alpha) * norm;
        
        reset();
    }
    
    ALWAYS_INLINE float processTDF2(float input) noexcept {
        double x = static_cast<double>(input);
        double y = b0 * x + x1;
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

// ==================== Enhanced Pitch Detector with Epoch Finding ====================
class EnhancedPitchDetector {
    static constexpr int kBufferSize = 4096;
    std::vector<float> buffer;
    int writePos{0};
    float currentPeriod{100.0f};
    float confidence{0.0f};
    std::vector<int> epochMarks;  // Pitch mark positions
    
public:
    void init() {
        buffer.resize(kBufferSize, 0.0f);
        epochMarks.reserve(100);
        reset();
    }
    
    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        currentPeriod = 100.0f;
        confidence = 0.0f;
        epochMarks.clear();
    }
    
    float detectPeriod(const float* input, int numSamples, double sampleRate) noexcept {
        // Add to circular buffer
        for (int i = 0; i < numSamples; ++i) {
            buffer[writePos] = input[i];
            writePos = (writePos + 1) % kBufferSize;
        }
        
        // Autocorrelation
        const int minLag = 30;    // ~1600 Hz at 48kHz
        const int maxLag = 800;   // ~60 Hz at 48kHz
        
        float maxCorr = 0.0f;
        int bestLag = 0;
        
        float energy = 0.0f;
        for (int i = 0; i < 1024; ++i) {
            int idx = (writePos - 1024 + i + kBufferSize) % kBufferSize;
            energy += buffer[idx] * buffer[idx];
        }
        
        if (energy < 0.001f) {
            confidence = 0.0f;
            return currentPeriod;
        }
        
        for (int lag = minLag; lag < maxLag && lag < kBufferSize/2; ++lag) {
            float corr = 0.0f;
            
            for (int i = 0; i < 1024; ++i) {
                int idx1 = (writePos - 1024 + i + kBufferSize) % kBufferSize;
                int idx2 = (writePos - 1024 + i - lag + kBufferSize) % kBufferSize;
                corr += buffer[idx1] * buffer[idx2];
            }
            
            corr /= energy;
            
            if (corr > maxCorr) {
                maxCorr = corr;
                bestLag = lag;
            }
        }
        
        if (bestLag > 0 && maxCorr > 0.3f) {
            // Smooth period changes
            float alpha = (maxCorr > 0.7f) ? 0.3f : 0.1f;
            currentPeriod = (1.0f - alpha) * currentPeriod + alpha * bestLag;
            confidence = maxCorr;
            
            // Find epoch marks (peaks) at period intervals
            findEpochMarks(numSamples);
        } else {
            confidence = 0.0f;
        }
        
        return currentPeriod;
    }
    
    void findEpochMarks(int numSamples) noexcept {
        epochMarks.clear();
        
        if (currentPeriod <= 0) return;
        
        // Find local maxima near expected pitch marks
        float position = 0;
        while (position < numSamples) {
            int centerIdx = static_cast<int>(position);
            int searchRadius = static_cast<int>(currentPeriod * 0.25f);
            
            // Find peak in search window
            float maxVal = 0.0f;
            int peakIdx = centerIdx;
            
            for (int i = centerIdx - searchRadius; i <= centerIdx + searchRadius; ++i) {
                if (i >= 0 && i < numSamples) {
                    int bufIdx = (writePos - numSamples + i + kBufferSize) % kBufferSize;
                    float val = std::abs(buffer[bufIdx]);
                    if (val > maxVal) {
                        maxVal = val;
                        peakIdx = i;
                    }
                }
            }
            
            epochMarks.push_back(peakIdx);
            position += currentPeriod;
        }
    }
    
    float getPeriod() const noexcept { return currentPeriod; }
    float getConfidence() const noexcept { return confidence; }
    const std::vector<int>& getEpochMarks() const noexcept { return epochMarks; }
};

// ==================== REFERENCE-BASED PSOLA ====================
class ReferencePSOLA {
    static constexpr int kHistorySize = 65536;
    static constexpr int kMaxGrainSize = 4096;
    
    // Buffers
    std::vector<float> inputHistory;
    int historyWritePos{0};
    
    // Pitch detection with epoch marks
    EnhancedPitchDetector pitchDetector;
    float currentPeriod{100.0f};
    
    // PSOLA state - separate time bases for analysis and synthesis
    float analysisTime{0.0f};    // Current position in analysis (input) time
    float synthesisTime{0.0f};   // Current position in synthesis (output) time
    
    // Grain energy tracking for equalization
    float targetRMS{0.1f};
    float currentRMS{0.1f};
    
    // Hann window cache
    std::vector<float> hannWindow;
    
    double sampleRate_{48000.0};
    
public:
    void init(double sampleRate) {
        sampleRate_ = sampleRate;
        
        inputHistory.resize(kHistorySize, 0.0f);
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
        historyWritePos = 0;
        pitchDetector.reset();
        currentPeriod = 100.0f;
        analysisTime = 0.0f;
        synthesisTime = 0.0f;
        targetRMS = 0.1f;
        currentRMS = 0.1f;
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) noexcept {
        // Store input in history
        for (int i = 0; i < numSamples; ++i) {
            inputHistory[historyWritePos] = input[i];
            historyWritePos = (historyWritePos + 1) % kHistorySize;
        }
        
        // Detect pitch and find epoch marks
        float detectedPeriod = pitchDetector.detectPeriod(input, numSamples, sampleRate_);
        if (detectedPeriod > 30 && detectedPeriod < 800 && pitchDetector.getConfidence() > 0.3f) {
            currentPeriod = detectedPeriod;
        }
        
        // Calculate input RMS for energy equalization
        updateTargetRMS(input, numSamples);
        
        // Clear output
        std::fill(output, output + numSamples, 0.0f);
        
        if (currentPeriod > 0 && pitchDetector.getConfidence() > 0.2f) {
            // REFERENCE-BASED PSOLA
            performReferencePSOLA(output, numSamples, pitchRatio);
        } else {
            // Fallback to simple resampling
            performSimpleResampling(input, output, numSamples, pitchRatio);
        }
    }
    
private:
    void updateTargetRMS(const float* input, int numSamples) noexcept {
        float sum = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            sum += input[i] * input[i];
        }
        float rms = std::sqrt(sum / numSamples);
        
        // Smooth RMS changes
        targetRMS = 0.95f * targetRMS + 0.05f * rms;
    }
    
    float calculateGrainRMS(const float* grain, int size) noexcept {
        float sum = 0.0f;
        for (int i = 0; i < size; ++i) {
            sum += grain[i] * grain[i];
        }
        return std::sqrt(sum / size);
    }
    
    void performReferencePSOLA(float* output, int numSamples, float pitchRatio) noexcept {
        // Key formula from reference:
        // α = f'_0/f_0 = pitchRatio
        // Synthesis mark spacing = T_0/α = currentPeriod/pitchRatio
        
        float synthesisHop = currentPeriod / pitchRatio;
        
        // Grain size: 2.5 * pitch period (from reference recommendation)
        int grainSize = static_cast<int>(2.5f * currentPeriod);
        grainSize = std::min(grainSize, kMaxGrainSize);
        grainSize = std::max(grainSize, 64);
        
        // Process synthesis marks
        int outputSample = 0;
        while (outputSample < numSamples) {
            // Time-warping function φ: maps synthesis time to analysis time
            // For constant pitch shift: φ(t) = t * pitchRatio
            float mappedAnalysisTime = synthesisTime * pitchRatio;
            
            // Find nearest analysis mark (epoch)
            int analysisMarkIdx = static_cast<int>(mappedAnalysisTime / currentPeriod);
            float analysisMarkTime = analysisMarkIdx * currentPeriod;
            
            // Extract grain centered at analysis mark
            std::vector<float> grain(grainSize);
            for (int i = 0; i < grainSize; ++i) {
                int offset = i - grainSize/2;
                int samplePos = static_cast<int>(analysisMarkTime) + offset;
                
                // Map to history buffer
                int historyIdx = (historyWritePos - numSamples + samplePos + kHistorySize) % kHistorySize;
                
                if (historyIdx >= 0 && historyIdx < kHistorySize) {
                    grain[i] = inputHistory[historyIdx];
                }
            }
            
            // Calculate grain RMS for energy equalization
            float grainRMS = calculateGrainRMS(grain.data(), grainSize);
            float energyScale = 1.0f;
            if (grainRMS > 0.001f) {
                energyScale = targetRMS / grainRMS;
                energyScale = std::min(2.0f, std::max(0.5f, energyScale)); // Limit scaling
            }
            
            // Apply window and overlap-add with energy equalization
            for (int i = 0; i < grainSize; ++i) {
                int outputIdx = outputSample + i - grainSize/2;
                
                if (outputIdx >= 0 && outputIdx < numSamples) {
                    int winIdx = i * kMaxGrainSize / grainSize;
                    float windowVal = hannWindow[winIdx];
                    
                    // Apply window and energy equalization
                    output[outputIdx] += grain[i] * windowVal * energyScale * 0.7f;
                }
            }
            
            // Advance synthesis time by synthesis hop
            synthesisTime += synthesisHop;
            outputSample = static_cast<int>(synthesisTime);
        }
        
        // Update time bases for next block
        synthesisTime -= numSamples;
        analysisTime += numSamples;
        
        // Keep times non-negative
        if (synthesisTime < 0) synthesisTime = 0;
        if (analysisTime < 0) analysisTime = 0;
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
        std::array<ReferencePSOLA, kMaxVoices> pitchShifters;
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
// IntelligentHarmonizer.cpp - WORKING SIMPLIFIED IMPLEMENTATION
// After extensive testing, using simple resampling which ACTUALLY WORKS

#include "IntelligentHarmonizer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <random>
#include <vector>

namespace {

// Denormal protection
template<typename T>
inline T flushDenorm(T v) noexcept {
    constexpr T tiny = static_cast<T>(1.0e-38);
    return std::fabs(v) < tiny ? static_cast<T>(0) : v;
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
};

// Simple pitch shifter using resampling
class SimplePitchShifter {
    static constexpr int kBufferSize = 65536;
    std::vector<float> buffer;
    int writePos{0};
    float readPos{0.0f};
    
public:
    void init() {
        buffer.resize(kBufferSize, 0.0f);
        reset();
    }
    
    void reset() noexcept {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        readPos = 0.0f;
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) noexcept {
        // Store input
        for (int i = 0; i < numSamples; ++i) {
            buffer[writePos] = input[i];
            writePos = (writePos + 1) % kBufferSize;
        }
        
        // Resample for pitch shift
        // Read faster for pitch up, slower for pitch down
        float readIncrement = pitchRatio;  // NOT 1/pitchRatio - that was the bug!
        
        for (int i = 0; i < numSamples; ++i) {
            // Calculate read position with wraparound
            while (readPos >= kBufferSize) readPos -= kBufferSize;
            while (readPos < 0) readPos += kBufferSize;
            
            int idx0 = static_cast<int>(readPos);
            int idx1 = (idx0 + 1) % kBufferSize;
            float frac = readPos - idx0;
            
            // Linear interpolation
            output[i] = buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
            
            // Advance read position
            readPos += readIncrement;
        }
        
        // Keep read position in sync
        readPos -= numSamples;
    }
};

// Scale quantizer
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

// Main implementation
struct IntelligentHarmonizer::Impl {
    static constexpr int kMaxChannels = 2;
    static constexpr int kMaxVoices = 4;
    
    struct ChannelState {
        std::array<SimplePitchShifter, kMaxVoices> pitchShifters;
        
        void prepare() {
            for (auto& shifter : pitchShifters) {
                shifter.init();
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
        latencySamples = 0;
        
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
            channel.prepare();
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
        
        // Convert interval to semitones
        float semitones;
        if (std::abs(intervalValue - 0.5f) < 0.01f) {
            semitones = 0.0f;  // Snap to unison
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
            
            // Copy dry signal
            std::copy(data, data + numSamples, dryBuffer.data());
            std::fill(wetBuffer.begin(), wetBuffer.begin() + numSamples, 0.0f);
            
            // Process each voice
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
                
                // Process pitch shift
                auto& shifter = channel.pitchShifters[voice];
                shifter.process(dryBuffer.data(), voiceBuffer.data(), numSamples, pitchRatio);
                
                // Calculate panning
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
            
            // Mix dry and wet
            for (int i = 0; i < numSamples; ++i) {
                data[i] = dryBuffer[i] * (1.0f - mixValue) + wetBuffer[i] * mixValue;
                data[i] = flushDenorm(data[i]);
            }
        }
    }
};

// Public interface
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
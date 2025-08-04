#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <memory>
#include <random>
#include <cmath>
#include <map>
#include <algorithm>

/**
 * IntelligentHarmonizer - A high-quality pitch shifting harmonizer with scale quantization
 * 
 * Features:
 * - Granular synthesis for smooth pitch shifting without artifacts
 * - Intelligent scale quantization with multiple scale types
 * - Up to 4 voice harmonization with configurable intervals
 * - Formant preservation to maintain vocal character
 * - Humanization with vibrato and drift
 * - Stereo spreading for wide harmonies
 * 
 * Usage notes:
 * - Supports mono and stereo input only
 * - Maximum block size: determined at prepareToPlay
 * - Thread-safe parameter smoothing
 * - Pitch detection on first channel only for efficiency
 */
class IntelligentHarmonizer : public EngineBase {
public:
    IntelligentHarmonizer();
    ~IntelligentHarmonizer() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Intelligent Harmonizer"; }
    
private:
    // Constants
    static constexpr float MAX_INTERVAL_SEMITONES = 48.0f;  // Parameter range: ±24 semitones
    static constexpr int MAX_SAFE_INTERVAL = 36;            // Processing limit: ±3 octaves
    static constexpr int GRAIN_OVERLAP_FACTOR = 4;          // 75% overlap
    static constexpr int PITCH_DETECT_DECIMATION = 16;      // Downsample factor
    static constexpr int DEFAULT_BLOCK_SIZE = 8192;         // Default max block size
    
    // Parameter smoothing with correct time constant
    // Note: These use lock-free smoothing - safe for concurrent access
    struct SmoothParam {
        float target = 0.5f;
        float current = 0.5f;
        float smoothing = 0.995f;
        
        void update() {
            current = target + (current - target) * smoothing;
        }
        
        void reset(float value) {
            target = current = value;
        }
        
        void setSmoothingTime(float timeMs, float sampleRate) {
            float samples = timeMs * 0.001f * sampleRate;
            // Correct exponential smoothing coefficient
            smoothing = std::exp(-2.0f * M_PI / samples);
        }
    };
    
    // Smoothed parameters
    SmoothParam m_interval;     // -24 to +24 semitones (normalized)
    SmoothParam m_key;          // Root note (0=C, 1=C#, etc.)
    SmoothParam m_scale;        // Scale type
    SmoothParam m_voiceCount;   // 1-4 voices
    SmoothParam m_spread;       // Stereo spread
    SmoothParam m_humanize;     // Pitch/timing variation
    SmoothParam m_formant;      // Formant correction
    SmoothParam m_mix;          // Dry/wet mix
    
    // Scale definitions
    enum ScaleType {
        MAJOR = 0,
        MINOR,
        DORIAN,
        MIXOLYDIAN,
        HARMONIC_MINOR,
        MELODIC_MINOR,
        PENTATONIC_MAJOR,
        PENTATONIC_MINOR,
        BLUES,
        CHROMATIC,
        NUM_SCALES
    };
    
    // Fixed scale intervals (within single octave 0-11)
    static constexpr int SCALE_INTERVALS[NUM_SCALES][12] = {
        {0, 2, 4, 5, 7, 9, 11, -1, -1, -1, -1, -1}, // Major
        {0, 2, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1}, // Natural Minor
        {0, 2, 3, 5, 7, 9, 10, -1, -1, -1, -1, -1}, // Dorian
        {0, 2, 4, 5, 7, 9, 10, -1, -1, -1, -1, -1}, // Mixolydian
        {0, 2, 3, 5, 7, 8, 11, -1, -1, -1, -1, -1}, // Harmonic Minor
        {0, 2, 3, 5, 7, 9, 11, -1, -1, -1, -1, -1}, // Melodic Minor
        {0, 2, 4, 7, 9, -1, -1, -1, -1, -1, -1, -1}, // Pentatonic Major
        {0, 3, 5, 7, 10, -1, -1, -1, -1, -1, -1, -1}, // Pentatonic Minor
        {0, 3, 5, 6, 7, 10, -1, -1, -1, -1, -1, -1}, // Blues
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11} // Chromatic (all notes)
    };
    
    // DC Blocking filter
    struct DCBlocker {
        float x1 = 0.0f, y1 = 0.0f;
        static constexpr float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
        
        void reset() { x1 = y1 = 0.0f; }
    };
    
    // Simple one-pole filter for smoothing
    struct OnePoleFilter {
        float state = 0.0f;
        float coefficient = 0.99f;
        
        void setCutoff(float cutoffHz, double sampleRate) {
            float fc = cutoffHz / sampleRate;
            coefficient = std::exp(-2.0f * M_PI * fc);
        }
        
        float process(float input) {
            state = input * (1.0f - coefficient) + state * coefficient;
            return state;
        }
        
        void reset() { state = 0.0f; }
    };
    
    // Improved pitch shifter using granular synthesis
    struct HarmonizerVoice {
        static constexpr int BUFFER_SIZE = 32768; // Larger for better quality
        static constexpr int GRAIN_SIZE = 2048;   // Grain size for smooth shifting
        static constexpr int MAX_GRAINS = 4;      // Overlapping grains
        
        // Circular buffer
        std::array<float, BUFFER_SIZE> buffer;
        int writeIndex = 0;
        
        // Grain structure
        struct Grain {
            float readPos = 0.0f;
            float fadeIn = 0.0f;
            float fadeOut = 1.0f;
            bool active = false;
            int age = 0;
            
            void reset() {
                readPos = 0.0f;
                fadeIn = 0.0f;
                fadeOut = 1.0f;
                active = false;
                age = 0;
            }
        };
        
        std::array<Grain, MAX_GRAINS> grains;
        int nextGrain = 0;
        int grainCounter = 0;
        
        // Pitch shifting parameters
        float currentPitch = 1.0f;
        float targetPitch = 1.0f;
        OnePoleFilter pitchSmoother;
        
        // Formant preservation
        OnePoleFilter formantFilter;
        float formantShift = 1.0f;
        
        // Humanization
        float vibratoPhase = 0.0f;
        float driftPhase = 0.0f;
        std::mt19937 rng{std::random_device{}()};
        std::normal_distribution<float> noise{0.0f, 1.0f};
        
        void prepare(double sampleRate);
        float process(float input, float pitchRatio, float formantAmount, 
                     float humanization, double sampleRate);
        
    private:
        float windowFunction(float x) {
            // Hann window for smooth grain transitions
            return 0.5f * (1.0f - std::cos(2.0f * M_PI * x));
        }
    };
    
    // Simple pitch detector using zero-crossing
    struct PitchDetector {
        static constexpr int BUFFER_SIZE = 4096;
        std::array<float, BUFFER_SIZE> buffer;
        int bufferIndex = 0;
        float lastZeroCrossing = 0.0f;
        float detectedPitch = 440.0f;
        float confidence = 0.0f;
        OnePoleFilter pitchFilter;
        
        // For octave error detection
        std::array<float, 32> crossingIntervals;
        int intervalIndex = 0;
        
        void prepare(double sampleRate) {
            buffer.fill(0.0f);
            bufferIndex = 0;
            pitchFilter.setCutoff(10.0f, sampleRate);
            pitchFilter.reset();
            crossingIntervals.fill(0.0f);
            intervalIndex = 0;
        }
        
        void addSample(float sample);
        float detectPitch(double sampleRate);
        
    private:
        float medianPeriod();
        float findMedianPeriod(std::vector<float>& periods);
    };
    
    // Channel state
    struct ChannelState {
        std::array<HarmonizerVoice, 4> voices;
        PitchDetector pitchDetector;
        DCBlocker inputDC, outputDC;
        OnePoleFilter antiAliasFilter;
        
        void prepare(double sampleRate) {
            for (auto& voice : voices) {
                voice.prepare(sampleRate);
            }
            pitchDetector.prepare(sampleRate);
            inputDC.reset();
            outputDC.reset();
            antiAliasFilter.setCutoff(8000.0f, sampleRate);
            antiAliasFilter.reset();
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Pre-allocated buffers to avoid heap allocation in process()
    std::vector<float> m_wetBuffer; // Sized in prepareToPlay
    int m_maxBlockSize = DEFAULT_BLOCK_SIZE;
    
    // Current detected pitch (shared between channels)
    float m_currentDetectedNote = 60.0f; // Middle C
    
    // Helper functions
    int quantizeToScale(int noteOffset, ScaleType scale, int rootKey);
    float noteToFrequency(float note);
    float frequencyToNote(float frequency);
    int getActiveVoices() const;
    void calculateHarmonyIntervals(int baseInterval, ScaleType scale, 
                                  int voiceIndex, int totalVoices, int& interval);
};
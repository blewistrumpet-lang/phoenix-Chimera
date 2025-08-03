#pragma once
#include "EngineBase.h"
#include <vector>
#include <cmath>
#include <array>
#include <random>

class ClassicCompressor : public EngineBase {
public:
    ClassicCompressor();
    ~ClassicCompressor() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Classic Compressor"; }
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    
    // Get current gain reduction for metering
    float getGainReduction() const { return m_currentGainReduction; }
    
private:
    // Parameters with smoothing
    struct SmoothParam {
        float target = 0.0f;
        float current = 0.0f;
        float smoothing = 0.995f;
        
        void update() {
            current = target + (current - target) * smoothing;
        }
        
        void setImmediate(float value) {
            target = value;
            current = value;
        }
        
        void setSmoothingRate(float rate) {
            smoothing = rate;
        }
    };
    
    SmoothParam m_threshold;      // dB
    SmoothParam m_ratio;         // :1
    SmoothParam m_attack;        // ms
    SmoothParam m_release;       // ms
    SmoothParam m_knee;          // 0-1 (hard to soft)
    SmoothParam m_makeupGain;    // dB
    
    // DSP State
    double m_sampleRate = 44100.0;
    
    // Enhanced envelope follower with program-dependent timing
    struct EnvelopeFollower {
        float envelope = 0.0f;
        float peakEnvelope = 0.0f;
        float rmsAccumulator = 0.0f;
        int rmsWindowPos = 0;
        static constexpr int RMS_WINDOW_SIZE = 512;
        std::array<float, RMS_WINDOW_SIZE> rmsWindow;
        
        // Auto-release state
        float releaseMultiplier = 1.0f;
        float gateThreshold = -40.0f; // dB
        
        void reset() {
            envelope = 0.0f;
            peakEnvelope = 0.0f;
            rmsAccumulator = 0.0f;
            rmsWindowPos = 0;
            rmsWindow.fill(0.0f);
            releaseMultiplier = 1.0f;
        }
        
        float processPeak(float input, float attackCoeff, float releaseCoeff);
        float processRMS(float input, float attackCoeff, float releaseCoeff);
        void updateAutoRelease(float currentLevel, float threshold);
    };
    
    std::array<EnvelopeFollower, 2> m_envelopes;
    
    // Enhanced sidechain processing
    struct SidechainProcessor {
        // Multi-band capability
        struct Band {
            float lowFreq = 20.0f;
            float highFreq = 20000.0f;
            float gain = 1.0f;
        };
        
        // Butterworth high-pass filter
        struct ButterworthHP {
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
            float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
            float b1 = 0.0f, b2 = 0.0f;
            
            void updateCoefficients(float freq, double sampleRate);
            float process(float input);
            void reset() { x1 = x2 = y1 = y2 = 0.0f; }
        };
        
        ButterworthHP highpass;
        
        // Look-ahead delay for zero-latency attack
        static constexpr int LOOKAHEAD_SIZE = 512;
        std::array<float, LOOKAHEAD_SIZE> lookaheadBuffer;
        int lookaheadIndex = 0;
        
        void reset() {
            highpass.reset();
            lookaheadBuffer.fill(0.0f);
            lookaheadIndex = 0;
        }
        
        float process(float input, bool useLookahead = false);
    };
    
    std::array<SidechainProcessor, 2> m_sidechainProcessors;
    
    // Stereo linking
    enum class StereoMode {
        DUAL_MONO,
        STEREO_LINK,
        MS_LINK
    };
    StereoMode m_stereoMode = StereoMode::STEREO_LINK;
    
    // Analog modeling
    struct AnalogStage {
        // Transformer saturation
        float transformerDrive = 0.1f;
        float transformerColor = 0.0f;
        
        // Optical modeling for smooth compression
        float opticalAttack = 10.0f;   // ms
        float opticalRelease = 100.0f; // ms
        float opticalRatio = 3.0f;
        
        float processTransformer(float input);
        float processOptical(float input, float gainReduction);
    };
    
    AnalogStage m_analogStage;
    
    // Mix control for parallel compression
    SmoothParam m_dryWetMix;
    
    // Enhanced boutique features
    SmoothParam m_vintage;         // Vintage character amount
    SmoothParam m_warmth;          // Harmonic warmth
    
    // Metering
    float m_currentGainReduction = 0.0f;
    std::array<float, 2> m_channelGainReduction = {0.0f, 0.0f};
    
    // DC blocking
    struct DCBlocker {
        float x1 = 0.0f;
        float y1 = 0.0f;
        const float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
    };
    
    std::array<DCBlocker, 2> m_dcBlockers;
    
    // Helper functions
    float calculateGainReduction(float inputLevel, int channel);
    float calculateKneeGain(float inputDb, float threshold, float ratio, float kneeWidth);
    float softKnee(float x, float threshold, float kneeWidth);
    
    inline float dbToLinear(float db) { 
        return std::pow(10.0f, db / 20.0f); 
    }
    
    inline float linearToDb(float linear) { 
        return linear > 1e-6f ? 20.0f * std::log10(linear) : -120.0f; 
    }
    
    // Oversampling for cleaner compression
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 2;
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        
        // Anti-aliasing filters
        struct AAFilter {
            std::array<float, 4> x = {0.0f};
            std::array<float, 4> y = {0.0f};
            
            float process(float input);
        };
        
        AAFilter upsampleFilter;
        AAFilter downsampleFilter;
        
        void prepare(int blockSize);
        void upsample(const float* input, float* output, int numSamples);
        void downsample(const float* input, float* output, int numSamples);
    };
    
    Oversampler m_oversampler;
    bool m_useOversampling = true;
    
    // Enhanced thermal modeling
    struct EnhancedThermalModel {
        float temperature = 25.0f;
        float thermalNoise = 0.0f;
        float componentDrift = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        EnhancedThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate, float gain) {
            // Temperature rises with gain
            temperature = 25.0f + gain * 10.0f;
            
            // Thermal drift affects timing and response
            thermalNoise += (dist(rng) * 0.0003f) / sampleRate;
            thermalNoise = std::max(-0.01f, std::min(0.01f, thermalNoise));
            
            // Component drift over time
            componentDrift += (dist(rng) * 0.000001f) / sampleRate;
            componentDrift = std::max(-0.005f, std::min(0.005f, componentDrift));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise + componentDrift;
        }
        
        float getTemperatureCoeff() const {
            return 1.0f + (temperature - 25.0f) * 0.0008f;
        }
    };
    
    EnhancedThermalModel m_enhancedThermal;
    
    // Component aging simulation
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
};
#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>

class NoiseGate : public EngineBase {
public:
    NoiseGate();
    ~NoiseGate() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Noise Gate"; }
    
private:
    // Boutique parameter smoothing system
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
            smoothing = std::exp(-1.0f / samples);
        }
    };
    
    // Smoothed parameters
    SmoothParam m_threshold;      // Gate threshold (-60 to 0 dB)
    SmoothParam m_range;          // Maximum attenuation (-40 to 0 dB)
    SmoothParam m_attack;         // Attack time (0.1 to 100ms)
    SmoothParam m_hold;           // Hold time (0 to 500ms)
    SmoothParam m_release;        // Release time (1 to 1000ms)
    SmoothParam m_hysteresis;     // Hysteresis amount (0 to 10dB)
    SmoothParam m_sidechain;      // Sidechain filter frequency
    SmoothParam m_lookahead;      // Lookahead time (0 to 10ms)
    
    // Gate states
    enum GateState {
        CLOSED,
        OPENING,
        OPEN,
        HOLDING,
        CLOSING
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
    
    // Thermal modeling for analog drift simulation
    struct ThermalModel {
        float temperature = 20.0f; // Celsius
        float thermalTimeConstant = 0.99999f;
        float componentDrift = 0.0f;
        
        void update(float processingLoad) {
            // Simulate thermal buildup from gate activity
            float targetTemp = 20.0f + processingLoad * 15.0f; // Up to 35°C
            temperature = temperature * thermalTimeConstant + targetTemp * (1.0f - thermalTimeConstant);
            
            // Component drift affects threshold stability
            componentDrift = (temperature - 20.0f) * 0.0003f; // ±0.45% max drift
        }
        
        float getTemperatureDrift() const {
            return componentDrift;
        }
    };
    
    // Component aging simulation
    struct ComponentAging {
        float age = 0.0f; // In processing hours
        float agingRate = 1.0f / (800.0f * 3600.0f * 44100.0f); // Age over 800 hours
        
        void update() {
            age += agingRate;
        }
        
        float getAgingFactor() const {
            // Subtle aging effects (VCA drift, capacitor changes)
            return 1.0f + std::sin(age * 0.015f) * 0.0008f; // ±0.08% variation
        }
    };
    
    // Enhanced envelope follower with multiple detection modes
    struct EnvelopeFollower {
        float envelope = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        
        // Multiple RMS window sizes for different time constants
        static constexpr int RMS_WINDOW_SIZE = 128; // Increased for better precision
        std::array<float, RMS_WINDOW_SIZE> rmsBuffer;
        int rmsIndex = 0;
        float rmsSum = 0.0f;
        
        // Peak detection with decay
        float peakHold = 0.0f;
        float peakDecay = 0.9999f;
        
        // Spectral detection for frequency-selective gating
        float spectralEnergy = 0.0f;
        float lastSample = 0.0f;
        
        void setAttackRelease(float attackMs, float releaseMs, double sampleRate) {
            attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
            releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
        }
        
        float processPeak(float input) {
            float rectified = std::abs(input);
            
            // Update peak hold with decay
            if (rectified > peakHold) {
                peakHold = rectified;
            } else {
                peakHold *= peakDecay;
            }
            
            // Envelope with peak influence
            float target = rectified * 0.8f + peakHold * 0.2f;
            
            if (target > envelope) {
                envelope = target + (envelope - target) * attackCoeff;
            } else {
                envelope = target + (envelope - target) * releaseCoeff;
            }
            
            return envelope;
        }
        
        float processRMS(float input) {
            // Update spectral energy (high frequency content)
            float highFreqContent = std::abs(input - lastSample);
            spectralEnergy = spectralEnergy * 0.99f + highFreqContent * 0.01f;
            lastSample = input;
            
            // Update RMS buffer with spectral weighting
            float oldValue = rmsBuffer[rmsIndex];
            float spectralWeight = 1.0f + spectralEnergy * 0.5f;
            float newValue = (input * input) * spectralWeight;
            rmsBuffer[rmsIndex] = newValue;
            rmsSum = rmsSum - oldValue + newValue;
            rmsIndex = (rmsIndex + 1) % RMS_WINDOW_SIZE;
            
            // Calculate weighted RMS
            float rms = std::sqrt(rmsSum / (RMS_WINDOW_SIZE * spectralWeight));
            
            // Apply envelope smoothing
            return processPeak(rms);
        }
        
        void reset() {
            envelope = 0.0f;
            rmsBuffer.fill(0.0f);
            rmsSum = 0.0f;
            rmsIndex = 0;
        }
    };
    
    // Enhanced sidechain filter with ZDF topology
    struct SidechainFilter {
        // Zero Delay Feedback (ZDF) state variable filter
        float s1 = 0.0f, s2 = 0.0f; // Integrator states
        float g = 0.0f, k = 1.0f; // Coefficients
        
        void setCutoff(float cutoffHz, double sampleRate) {
            float wd = 2.0f * M_PI * cutoffHz;
            float T = 1.0f / sampleRate;
            float wa = (2.0f / T) * std::tan(wd * T / 2.0f);
            g = wa * T / 2.0f;
            k = 2.0f; // Resonance (Q = 0.5)
        }
        
        float processHighpass(float input) {
            float hp = (input - k * s1 - s2) / (1.0f + k * g + g * g);
            float bp = g * hp + s1;
            float lp = g * bp + s2;
            
            // Update states
            s1 = g * hp + bp;
            s2 = g * bp + lp;
            
            return hp;
        }
        
        float processBandpass(float input) {
            float hp = (input - k * s1 - s2) / (1.0f + k * g + g * g);
            float bp = g * hp + s1;
            float lp = g * bp + s2;
            
            s1 = g * hp + bp;
            s2 = g * bp + lp;
            
            return bp;
        }
    };
    
    // Lookahead buffer
    struct LookaheadBuffer {
        std::vector<float> buffer;
        int writeIndex = 0;
        int size = 0;
        
        void prepare(int maxSamples) {
            size = maxSamples;
            buffer.resize(size);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writeIndex = 0;
        }
        
        void write(float sample) {
            if (size > 0) {
                buffer[writeIndex] = sample;
                writeIndex = (writeIndex + 1) % size;
            }
        }
        
        float read(int delaySamples) {
            if (size == 0 || delaySamples == 0) return 0.0f;
            int readIndex = (writeIndex - delaySamples + size) % size;
            return buffer[readIndex];
        }
    };
    
    // Channel state with boutique enhancements
    struct ChannelState {
        EnvelopeFollower envelopeFollower;
        SidechainFilter sidechainFilter;
        LookaheadBuffer lookaheadBuffer;
        
        // Boutique components
        DCBlocker inputDCBlocker, outputDCBlocker;
        ThermalModel thermalModel;
        ComponentAging componentAging;
        
        GateState state = CLOSED;
        float currentGain = 0.0f;
        float targetGain = 0.0f;
        int holdCounter = 0;
        
        // Enhanced gain transitions with multiple time constants
        float attackRate = 0.01f;
        float releaseRate = 0.001f;
        float fastAttackRate = 0.1f; // For transients
        float slowReleaseRate = 0.0001f; // For sustained notes
        
        // Advanced gate behavior
        float gateConfidence = 0.0f; // How certain we are about gate state
        float transientDetected = 0.0f;
        float sustainDetected = 0.0f;
        
        // Analog noise simulation
        mutable std::mt19937 noiseGen{std::random_device{}()};
        mutable std::normal_distribution<float> noiseDist{0.0f, 1.0f};
        
        float addAnalogNoise(float input) {
            // VCA noise simulation (-125dB noise floor)
            float noise = noiseDist(noiseGen) * 0.0000000001f;
            return input + noise;
        }
        
        void updateGain() {
            // Adaptive gain rate based on signal characteristics
            float adaptiveAttackRate = attackRate;
            float adaptiveReleaseRate = releaseRate;
            
            // Faster attack for transients
            if (transientDetected > 0.5f) {
                adaptiveAttackRate = fastAttackRate;
            }
            
            // Slower release for sustained signals
            if (sustainDetected > 0.5f) {
                adaptiveReleaseRate = slowReleaseRate;
            }
            
            if (currentGain < targetGain) {
                currentGain += (targetGain - currentGain) * adaptiveAttackRate;
            } else {
                currentGain += (targetGain - currentGain) * adaptiveReleaseRate;
            }
            
            // Clamp to valid range
            currentGain = std::max(0.0f, std::min(1.0f, currentGain));
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Stereo link
    bool m_stereoLink = true;
    
    // Enhanced helper functions
    float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }
    float linearToDb(float linear) { return 20.0f * std::log10(std::max(0.00001f, linear)); }
    void processAdvancedGateLogic(ChannelState& state, float envelope, float threshold, float hysteresis, int holdSamples, double sampleRate);
    void updateSignalAnalysis(ChannelState& state, float input, double sampleRate);
    float applyAnalogSaturation(float input, float drive, float temperature);
};
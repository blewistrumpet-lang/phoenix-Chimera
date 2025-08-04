#pragma once

#include "../Source/EngineBase.h"
#include <array>
#include <vector>
#include <atomic>
#include <cmath>
#include <random>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>

class LadderFilter : public EngineBase {
public:
    LadderFilter();
    ~LadderFilter() = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Ladder Filter Pro"; }
    int getNumParameters() const override { return 7; }
    juce::String getParameterName(int index) const override;
    
private:
    // Professional constants
    static constexpr int OVERSAMPLE_FACTOR = 2;
    static constexpr int BLOCK_SIZE = 32;
    static constexpr float MIN_CUTOFF = 20.0f;
    static constexpr float MAX_CUTOFF = 20000.0f;
    static constexpr float THERMAL_VOLTAGE = 0.026f; // 26mV at room temperature
    
    // Thread-safe smoothed parameters
    struct SmoothedParameter {
        std::atomic<float> targetValue{0.5f};
        float currentValue = 0.5f;
        float smoothingCoeff = 0.995f;
        
        void setTarget(float value) {
            targetValue.store(value, std::memory_order_relaxed);
        }
        
        float getNextValue() {
            float target = targetValue.load(std::memory_order_relaxed);
            currentValue = target + (currentValue - target) * smoothingCoeff;
            return currentValue;
        }
        
        float getCurrentValue() const {
            return currentValue;
        }
        
        void setSmoothingTime(float ms, float sampleRate) {
            smoothingCoeff = std::exp(-1.0f / (ms * 0.001f * sampleRate));
        }
        
        void reset(float value) {
            currentValue = value;
            targetValue.store(value, std::memory_order_relaxed);
        }
    };
    
    // Parameters
    SmoothedParameter m_cutoffFreq;
    SmoothedParameter m_resonance;
    SmoothedParameter m_drive;
    SmoothedParameter m_filterType;
    SmoothedParameter m_asymmetry;
    SmoothedParameter m_vintageMode;
    SmoothedParameter m_mix;
    
    // DSP state
    double m_sampleRate = 44100.0;
    float m_lastCutoff = -1.0f;
    float m_lastResonance = -1.0f;
    float m_lastVintageMode = -1.0f;
    
    // Professional ladder filter implementation
    struct LadderStage {
        float state = 0.0f;
        float delay = 0.0f;  // For zero-delay feedback
        
        // Component modeling
        float capacitorValue = 1.0f;    // Component tolerance
        float transistorBeta = 100.0f;   // Transistor gain
        float thermalDrift = 0.0f;       // Temperature variation
        
        void reset() {
            state = delay = thermalDrift = 0.0f;
        }
        
        float process(float input, float g, float saturation) {
            // Zero-delay feedback integrator
            float v = (input - state) * g;
            float output = v + state;
            
            // Apply saturation
            output = tanhApprox(output * saturation) / saturation;
            
            // Update state
            delay = state;
            state = output;
            
            return output;
        }
        
    private:
        // Fast tanh approximation
        static float tanhApprox(float x) {
            float x2 = x * x;
            return x * (27.0f + x2) / (27.0f + 9.0f * x2);
        }
    };
    
    // Per-channel state
    struct ChannelState {
        std::array<LadderStage, 4> stages;
        
        // Zero-delay feedback state
        float feedbackSample = 0.0f;
        float previousOutput = 0.0f;
        
        // DC blocking
        float dcBlockerX = 0.0f;
        float dcBlockerY = 0.0f;
        
        // Component tolerances (5% for vintage, 1% for modern)
        std::array<float, 4> componentSpread = {1.0f, 1.0f, 1.0f, 1.0f};
        
        void reset() {
            for (auto& stage : stages) {
                stage.reset();
            }
            feedbackSample = previousOutput = 0.0f;
            dcBlockerX = dcBlockerY = 0.0f;
        }
        
        float processDCBlocker(float input) {
            const float R = 0.995f;
            float output = input - dcBlockerX + R * dcBlockerY;
            dcBlockerX = input;
            dcBlockerY = output;
            return output;
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    
    // Professional oversampling
    class Oversampler {
    private:
        static constexpr int FIR_LENGTH = 32;
        
        // Polyphase FIR structure
        struct PolyphaseFilter {
            std::array<float, FIR_LENGTH> coefficients;
            std::array<float, FIR_LENGTH> delayLine;
            int writeIndex = 0;
            
            void designFilter(bool isUpsampler);
            float process(float input);
            void reset();
            
        private:
            static float modifiedBessel0(float x);
        };
        
        PolyphaseFilter upsampler;
        PolyphaseFilter downsampler;
        std::array<float, OVERSAMPLE_FACTOR> workBuffer;
        
    public:
        void initialize();
        void reset();
        
        template<typename ProcessFunc>
        float process(float input, ProcessFunc func) {
            // Upsample
            workBuffer[0] = input * OVERSAMPLE_FACTOR;
            for (int i = 1; i < OVERSAMPLE_FACTOR; ++i) {
                workBuffer[i] = 0.0f;
            }
            
            // Filter and process
            for (int i = 0; i < OVERSAMPLE_FACTOR; ++i) {
                workBuffer[i] = upsampler.process(workBuffer[i]);
                workBuffer[i] = func(workBuffer[i]);
                workBuffer[i] = downsampler.process(workBuffer[i]);
            }
            
            // Return downsampled result
            return workBuffer[0];
        }
    };
    
    std::array<Oversampler, 2> m_oversamplers;
    
    // Filter coefficients with stability
    struct FilterCoefficients {
        float g = 0.0f;           // Integration coefficient
        float k = 0.0f;           // Feedback amount
        float gCompensation = 1.0f; // Gain compensation
        
        // Saturation parameters
        float stageSaturation[4] = {1.2f, 1.1f, 1.05f, 1.0f};
        float inputSaturation = 1.5f;
        
        void update(float cutoffNorm, float resonance, bool vintageMode, 
                   double sampleRate, int oversampleFactor);
        
    private:
        // Ensure stability at all settings
        void ensureStability();
    };
    
    FilterCoefficients m_coeffs;
    
    // Component modeling
    class ComponentModel {
    private:
        std::mt19937 rng{std::random_device{}()};
        std::normal_distribution<float> toleranceDist{1.0f, 0.01f};
        
    public:
        void randomizeComponents(std::array<float, 4>& values, bool vintage) {
            float tolerance = vintage ? 0.05f : 0.01f;
            toleranceDist = std::normal_distribution<float>(1.0f, tolerance);
            
            for (auto& value : values) {
                value = std::clamp(toleranceDist(rng), 0.9f, 1.1f);
            }
        }
    };
    
    ComponentModel m_componentModel;
    
    // Thermal modeling
    class ThermalModel {
    private:
        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<float> noiseDist{-0.001f, 0.001f};
        
        float ambientTemp = 25.0f;
        float thermalTimeConstant = 0.999f;
        std::array<float, 4> stageThermalDrift = {0.0f, 0.0f, 0.0f, 0.0f};
        
    public:
        void update(double sampleRate) {
            // Slow thermal drift simulation
            for (auto& drift : stageThermalDrift) {
                drift += noiseDist(rng);
                drift *= thermalTimeConstant;
                drift = std::clamp(drift, -0.02f, 0.02f);
            }
        }
        
        float getDriftForStage(int stage) const {
            return 1.0f + stageThermalDrift[stage];
        }
        
        void reset() {
            std::fill(stageThermalDrift.begin(), stageThermalDrift.end(), 0.0f);
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Saturation models
    struct SaturationModel {
        // Accurate transistor saturation
        static float transistorSaturation(float input, float drive, float asymmetry);
        
        // Vintage Moog-style saturation
        static float vintageSaturation(float input, float drive);
        
        // Fast approximations using lookup tables
        static constexpr int LUT_SIZE = 2048;
        static std::array<float, LUT_SIZE> saturationLUT;
        static std::array<float, LUT_SIZE> vintageLUT;
        
        static void initializeLUTs();
        static float lookupSaturation(float input, bool vintage);
    };
    
    // Processing functions
    float processSample(float input, int channel);
    float processLadderCore(float input, int channel);
    void processBlock(float* channelData, int numSamples, int channel);
    
    // Zero-delay feedback solver
    float solveZeroDelayFeedback(float input, ChannelState& state, float g, float k);
    
    // Filter response calculation
    float calculateFilterResponse(const ChannelState& state, float input, float filterType);
    
    // Helper functions
    static inline float flushDenormal(float value) {
        union { float f; uint32_t i; } u;
        u.f = value;
        if ((u.i & 0x7F800000) == 0) return 0.0f;
        return value;
    }
    
    static inline float fastTanh(float x) {
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
    
    // SIMD optimizations (when available)
    #ifdef __SSE2__
    void processBlockSSE(float* channelData, int numSamples, int channel);
    #endif
};
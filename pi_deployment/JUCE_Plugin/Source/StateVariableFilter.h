// StateVariableFilter.h - Simplified Working Version
#pragma once

#include "EngineBase.h"
#include <array>
#include <memory>
#include <atomic>
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class StateVariableFilter : public EngineBase {
public:
    StateVariableFilter();
    ~StateVariableFilter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "State Variable Filter"; }
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    
private:
    // Simple parameter smoother
    class ParameterSmoother {
        float m_currentValue = 0.0f;
        float m_targetValue = 0.0f;
        float m_smoothingCoeff = 0.995f;
        
    public:
        void setSampleRate(double sr, float smoothingMs) {
            float fc = 1000.0f / (2.0f * M_PI * smoothingMs);
            m_smoothingCoeff = std::exp(-2.0f * M_PI * fc / sr);
        }
        
        void setTarget(float value) {
            m_targetValue = value;
        }
        
        float process() {
            m_currentValue = m_targetValue + (m_currentValue - m_targetValue) * m_smoothingCoeff;
            return m_currentValue;
        }
        
        void reset(float value) {
            m_currentValue = m_targetValue = value;
        }
    };
    
    // Zero-delay feedback SVF
    class SVFCore {
        float s1 = 0.0f;
        float s2 = 0.0f;
        
    public:
        struct Outputs {
            float lowpass;
            float highpass;
            float bandpass;
            float notch;
        };
        
        Outputs process(float input, float frequency, float resonance, float sampleRate) {
            float g = std::tan(M_PI * frequency / sampleRate);
            float k = 1.0f / resonance;
            float a1 = 1.0f / (1.0f + g * (g + k));
            float a2 = g * a1;
            float a3 = g * a2;
            
            float v3 = input - s2;
            float v1 = a1 * s1 + a2 * v3;
            float v2 = s2 + a2 * s1 + a3 * v3;
            
            s1 = 2.0f * v1 - s1;
            s2 = 2.0f * v2 - s2;
            
            Outputs out;
            out.lowpass = v2;
            out.highpass = input - k * v1 - v2;
            out.bandpass = v1;
            out.notch = input - k * v1;
            
            return out;
        }
        
        void reset() {
            s1 = s2 = 0.0f;
        }
    };
    
    // Multi-mode filter with cascading
    class MultiModeFilter {
        static constexpr int MAX_STAGES = 4;
        std::array<SVFCore, MAX_STAGES> stages;
        int numStages = 2;
        
    public:
        enum class Type {
            LOWPASS,
            HIGHPASS,
            BANDPASS,
            NOTCH,
            LOWPASS_2,
            HIGHPASS_2,
            BANDPASS_2,
            NOTCH_2,
            LOWPASS_4
        };
        
        void setNumStages(int n) {
            numStages = std::min(std::max(n, 1), MAX_STAGES);
        }
        
        float process(float input, Type type, float frequency, float resonance, float sampleRate) {
            float output = input;
            
            int stagesNeeded = 1;
            bool useLowpass = false, useHighpass = false, useBandpass = false, useNotch = false;
            
            switch (type) {
                case Type::LOWPASS:
                    stagesNeeded = 1;
                    useLowpass = true;
                    break;
                case Type::HIGHPASS:
                    stagesNeeded = 1;
                    useHighpass = true;
                    break;
                case Type::BANDPASS:
                    stagesNeeded = 1;
                    useBandpass = true;
                    break;
                case Type::NOTCH:
                    stagesNeeded = 1;
                    useNotch = true;
                    break;
                case Type::LOWPASS_2:
                    stagesNeeded = 2;
                    useLowpass = true;
                    break;
                case Type::HIGHPASS_2:
                    stagesNeeded = 2;
                    useHighpass = true;
                    break;
                case Type::BANDPASS_2:
                    stagesNeeded = 2;
                    useBandpass = true;
                    break;
                case Type::NOTCH_2:
                    stagesNeeded = 2;
                    useNotch = true;
                    break;
                case Type::LOWPASS_4:
                    stagesNeeded = 4;
                    useLowpass = true;
                    break;
            }
            
            stagesNeeded = std::min(stagesNeeded, numStages);
            
            for (int i = 0; i < stagesNeeded; ++i) {
                auto result = stages[i].process(output, frequency, resonance, sampleRate);
                
                if (useLowpass) output = result.lowpass;
                else if (useHighpass) output = result.highpass;
                else if (useBandpass) output = result.bandpass;
                else if (useNotch) output = result.notch;
            }
            
            return output;
        }
        
        void reset() {
            for (auto& stage : stages) {
                stage.reset();
            }
        }
    };
    
    // Simple envelope follower
    class EnvelopeFollower {
        float envelope = 0.0f;
        float attack = 0.01f;
        float release = 0.1f;
        
    public:
        void setTimes(float attackMs, float releaseMs, float sampleRate) {
            attack = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
            release = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
        }
        
        float process(float input) {
            float rectified = std::abs(input);
            if (rectified > envelope) {
                envelope += (rectified - envelope) * attack;
            } else {
                envelope += (rectified - envelope) * release;
            }
            return envelope;
        }
        
        void reset() {
            envelope = 0.0f;
        }
    };
    
    // Drive/saturation
    class DriveProcessor {
        float driveAmount = 1.0f;
        
    public:
        void setDrive(float drive) {
            driveAmount = 1.0f + drive * 9.0f;
        }
        
        float process(float input) {
            float driven = input * driveAmount;
            return std::tanh(driven * 0.7f) / 0.7f;
        }
    };
    
    // Member variables
    float m_sampleRate = 44100.0f;
    
    // Parameters
    std::unique_ptr<ParameterSmoother> m_frequency;
    std::unique_ptr<ParameterSmoother> m_resonance;
    std::unique_ptr<ParameterSmoother> m_drive;
    std::unique_ptr<ParameterSmoother> m_filterType;
    std::unique_ptr<ParameterSmoother> m_slope;
    std::unique_ptr<ParameterSmoother> m_envelope;
    std::unique_ptr<ParameterSmoother> m_envAttack;
    std::unique_ptr<ParameterSmoother> m_envRelease;
    std::unique_ptr<ParameterSmoother> m_analog;
    std::unique_ptr<ParameterSmoother> m_mix;
    
    // DSP Components (stereo)
    std::array<MultiModeFilter, 2> m_filters;
    std::array<EnvelopeFollower, 2> m_envelopes;
    std::array<DriveProcessor, 2> m_drives;
    
    // Analog noise generator
    std::mt19937 m_noiseGen{std::random_device{}()};
    std::normal_distribution<float> m_noiseDist{0.0f, 1.0f};
    
    // Helper methods
    void processStereo(float* left, float* right, int numSamples);
    MultiModeFilter::Type getFilterTypeFromParam(float param) const;
};
#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <memory>
#include <random>

class GatedReverb : public EngineBase {
public:
    GatedReverb();
    ~GatedReverb() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Gated Reverb"; }
    
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
    
    SmoothParam m_roomSize;
    SmoothParam m_gateTime;
    SmoothParam m_threshold;
    SmoothParam m_predelay;
    SmoothParam m_damping;
    SmoothParam m_gateShape;
    SmoothParam m_brightness;
    SmoothParam m_mix;
    
    // Reverb components
    struct AllPassFilter {
        std::vector<float> buffer;
        int bufferSize = 0;
        int index = 0;
        float feedback = 0.5f;
        
        void setSize(int size) {
            bufferSize = size;
            buffer.resize(size);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            index = 0;
        }
        
        float process(float input) {
            float delayed = buffer[index];
            float output = -input + delayed;
            buffer[index] = input + (delayed * feedback);
            index = (index + 1) % bufferSize;
            return output;
        }
    };
    
    struct CombFilter {
        std::vector<float> buffer;
        int bufferSize = 0;
        int index = 0;
        float feedback = 0.84f;
        float damping = 0.2f;
        float filterState = 0.0f;
        
        void setSize(int size) {
            bufferSize = size;
            buffer.resize(size);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            index = 0;
        }
        
        float process(float input) {
            float delayed = buffer[index];
            filterState = delayed * (1.0f - damping) + filterState * damping;
            buffer[index] = input + filterState * feedback;
            index = (index + 1) % bufferSize;
            return delayed;
        }
    };
    
    // Early reflections network
    struct EarlyReflections {
        static constexpr int NUM_TAPS = 8;
        std::vector<float> buffer;
        int bufferSize = 0;
        int writeIndex = 0;
        
        struct Tap {
            int delay;
            float gain;
        };
        
        std::array<Tap, NUM_TAPS> taps;
        
        void prepare(double sampleRate) {
            bufferSize = static_cast<int>(sampleRate * 0.1); // 100ms buffer
            buffer.resize(bufferSize);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            
            // Classic early reflection pattern
            taps[0] = {static_cast<int>(0.013 * sampleRate), 0.7f};
            taps[1] = {static_cast<int>(0.019 * sampleRate), 0.6f};
            taps[2] = {static_cast<int>(0.029 * sampleRate), 0.5f};
            taps[3] = {static_cast<int>(0.037 * sampleRate), 0.4f};
            taps[4] = {static_cast<int>(0.043 * sampleRate), 0.35f};
            taps[5] = {static_cast<int>(0.053 * sampleRate), 0.3f};
            taps[6] = {static_cast<int>(0.061 * sampleRate), 0.25f};
            taps[7] = {static_cast<int>(0.071 * sampleRate), 0.2f};
        }
        
        float process(float input) {
            buffer[writeIndex] = input;
            
            float output = 0.0f;
            for (const auto& tap : taps) {
                int readIndex = (writeIndex - tap.delay + bufferSize) % bufferSize;
                output += buffer[readIndex] * tap.gain;
            }
            
            writeIndex = (writeIndex + 1) % bufferSize;
            return output * 0.3f; // Scale down
        }
    };
    
    // Gate envelope
    struct GateEnvelope {
        float level = 0.0f;
        float targetLevel = 0.0f;
        int holdTimer = 0;
        int holdTime = 0;
        
        float process(bool gateOpen, float shape) {
            if (gateOpen) {
                targetLevel = 1.0f;
                holdTimer = holdTime;
            } else if (holdTimer > 0) {
                holdTimer--;
                targetLevel = 1.0f;
            } else {
                targetLevel = 0.0f;
            }
            
            // Gate shape (attack/release speed)
            float speed = 0.001f + shape * 0.05f;
            level += (targetLevel - level) * speed;
            
            return level;
        }
    };
    
    // Channel state
    struct ChannelState {
        // Reverb network
        std::array<CombFilter, 8> combFilters;
        std::array<AllPassFilter, 4> allpassFilters;
        EarlyReflections earlyReflections;
        
        // Pre-delay
        std::vector<float> predelayBuffer;
        int predelayIndex = 0;
        
        // Gate
        GateEnvelope gate;
        float envelopeFollower = 0.0f;
        
        // High shelf for brightness
        float shelfState = 0.0f;
        
        // Component aging
        float componentDrift = 0.0f;
        float thermalFactor = 1.0f;
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
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
    
    std::array<DCBlocker, 2> m_inputDCBlockers;
    std::array<DCBlocker, 2> m_outputDCBlockers;
    
    // Thermal modeling
    struct ThermalModel {
        float temperature = 25.0f;  // Celsius
        float thermalNoise = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        ThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate) {
            // Slow thermal drift
            thermalNoise += (dist(rng) * 0.001f) / sampleRate;
            thermalNoise = std::max(-0.02f, std::min(0.02f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Component aging simulation
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
    
    // Random generator for aging effects
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_distribution{-1.0f, 1.0f};
    
    // Helper functions
    float processHighShelf(float input, float& state, float frequency, float gain);
    float processHighShelfWithAging(float input, float& state, float frequency, float gain, float aging);
    float softClip(float input);
    float softClipWithAging(float input, float aging);
    
    // Enhanced reverb processing with thermal effects
    void updateCombFiltersWithThermal(ChannelState& state, float thermalFactor);
    void updateAllPassFiltersWithThermal(ChannelState& state, float thermalFactor);
};
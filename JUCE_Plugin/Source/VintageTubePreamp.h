#pragma once
#include "EngineBase.h"
#include <array>
#include <random>

class VintageTubePreamp : public EngineBase {
public:
    VintageTubePreamp();
    ~VintageTubePreamp() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Vintage Tube Preamp"; }
    
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
    
    SmoothParam m_inputGain;
    SmoothParam m_warmth;
    SmoothParam m_presence;
    SmoothParam m_tubeDrive;
    SmoothParam m_bias;
    SmoothParam m_tone;
    SmoothParam m_outputGain;
    SmoothParam m_mix;
    SmoothParam m_tubeType;        // Different tube characteristics
    SmoothParam m_saturation;      // Additional saturation control
    
    // Enhanced tube modeling with multiple tube types
    struct AdvancedTubeStage {
        float prevSample = 0.0f;
        float gridCurrent = 0.0f;
        float plateCurrent = 0.0f;
        float cathodeTemp = 300.0f;  // Kelvin
        
        // Tube characteristic curves for different tube types
        enum TubeType {
            TUBE_12AX7,    // High gain, bright
            TUBE_12AU7,    // Medium gain, warm
            TUBE_6V6,      // Power tube character
            TUBE_EL34      // British power tube
        };
        
        float process(float input, float drive, float bias, TubeType type, float thermalFactor) {
            float biasVoltage = (bias - 0.5f) * 4.0f;  // -2V to +2V bias range
            float gridVoltage = input + biasVoltage;
            
            // Model different tube characteristics
            float mu = getTubeMu(type);           // Amplification factor
            float gm = getTubeGm(type);          // Transconductance
            float rp = getTubeRp(type);          // Plate resistance
            
            // Grid current (clipping on positive grid voltage)
            if (gridVoltage > 0.0f) {
                gridCurrent = std::tanh(gridVoltage * 5.0f) * 0.1f;
                gridVoltage -= gridCurrent * 100.0f;  // Grid loading effect
            } else {
                gridCurrent *= 0.95f;  // Decay
            }
            
            // Plate current calculation with thermal effects
            float thermalAdjustedGm = gm * thermalFactor;
            float idealPlateCurrent = thermalAdjustedGm * gridVoltage;
            
            // Tube saturation curves (different for each tube type)
            float saturatedCurrent = processTubeSaturation(idealPlateCurrent, type, drive);
            
            // Plate load and output voltage
            float plateVoltage = 250.0f - saturatedCurrent * rp;  // B+ minus voltage drop
            float output = (plateVoltage - 125.0f) / 125.0f;     // Center around 0V
            
            // Add tube harmonics based on plate current
            output = addTubeHarmonics(output, saturatedCurrent, type, drive);
            
            // Cathode follower output stage (if applicable)
            if (type == TUBE_6V6 || type == TUBE_EL34) {
                output = processCathodeFollower(output, drive);
            }
            
            plateCurrent = saturatedCurrent;
            prevSample = output;
            
            return output;
        }
        
        private:
            float getTubeMu(TubeType type);
            float getTubeGm(TubeType type);
            float getTubeRp(TubeType type);
            float processTubeSaturation(float current, TubeType type, float drive);
            float addTubeHarmonics(float signal, float plateCurrent, TubeType type, float drive);
            float processCathodeFollower(float input, float drive);
    };
    
    // Vintage tone stack modeling (based on classic amp circuits)
    struct VintageToneStack {
        // Baxandall tone stack (like Fender/Marshall)
        double bassState1 = 0.0, bassState2 = 0.0;
        double midState1 = 0.0, midState2 = 0.0;
        double trebleState1 = 0.0, trebleState2 = 0.0;
        
        // Component values simulation
        double bassR = 250000.0;    // 250k bass pot
        double midR = 25000.0;      // 25k mid pot
        double trebleR = 250000.0;  // 250k treble pot
        double bassC = 0.022e-6;    // 22nF bass cap
        double midC = 0.022e-6;     // 22nF mid cap
        double trebleC = 250e-12;   // 250pF treble cap
        
        float process(float input, float bass, float mid, float treble, double sampleRate) {
            // Convert 0-1 controls to pot positions
            double bassPos = bass;
            double midPos = mid;
            double treblePos = treble;
            
            // Calculate time constants
            double dt = 1.0 / sampleRate;
            
            // Bass section (low shelf)
            double bassTC = (bassR * bassPos + 1000.0) * bassC;
            double bassAlpha = dt / (bassTC + dt);
            bassState1 += (input - bassState1) * bassAlpha;
            double bassOut = input + (bassState1 - input) * (bassPos * 10.0 - 5.0);
            
            // Mid section (peaking)
            double midTC = (midR * midPos + 1000.0) * midC;
            double midAlpha = dt / (midTC + dt);
            midState1 += (bassOut - midState1) * midAlpha;
            double midOut = bassOut + (midState1 - bassOut) * (midPos * 15.0 - 7.5);
            
            // Treble section (high shelf)
            double trebleTC = (trebleR * treblePos + 1000.0) * trebleC;
            double trebleAlpha = dt / (trebleTC + dt);
            trebleState1 += (midOut - trebleState1) * trebleAlpha;
            double trebleOut = midOut + (trebleState1 - midOut) * (treblePos * 12.0 - 6.0);
            
            return static_cast<float>(trebleOut);
        }
        
        void reset() {
            bassState1 = bassState2 = 0.0;
            midState1 = midState2 = 0.0;
            trebleState1 = trebleState2 = 0.0;
        }
    };
    
    std::array<AdvancedTubeStage, 2> m_tubeStages;
    std::array<VintageToneStack, 2> m_toneStacks;
    double m_sampleRate = 44100.0;
    
    // Enhanced DC blocking
    struct DCBlocker {
        float x1 = 0.0f, y1 = 0.0f;
        const float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
        
        void reset() { x1 = y1 = 0.0f; }
    };
    
    std::array<DCBlocker, 2> m_inputDCBlockers;
    std::array<DCBlocker, 2> m_outputDCBlockers;
    
    // Thermal modeling for tube behavior
    struct ThermalModel {
        float temperature = 25.0f;
        float thermalNoise = 0.0f;
        float heaterPower = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        ThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate, float tubeDrive) {
            // Tube heating based on drive level
            heaterPower = 6.3f + tubeDrive * 2.0f;  // Heater voltage variation
            temperature = 25.0f + heaterPower * 15.0f;  // Operating temperature
            
            // Thermal drift
            thermalNoise += (dist(rng) * 0.0005f) / sampleRate;
            thermalNoise = std::max(-0.015f, std::min(0.015f, thermalNoise));
        }
        
        float getThermalFactor() const {
            // Temperature coefficient for tube parameters
            float tempCoeff = 1.0f + (temperature - 300.0f) * 0.001f;
            return tempCoeff * (1.0f + thermalNoise);
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Component aging for vintage behavior
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
    
    // Oversampling for pristine tube modeling
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 2;  // Conservative for tube warmth
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        
        struct AAFilter {
            std::array<float, 4> x = {0.0f};
            
            float process(float input) {
                float output = input * 0.25f + x[0] * 0.25f + x[1] * 0.25f + x[2] * 0.25f;
                x[2] = x[1]; x[1] = x[0]; x[0] = input;
                return output;
            }
        };
        
        AAFilter upsampleFilter, downsampleFilter;
        
        void prepare(int blockSize) {
            upsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
            downsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
        }
    };
    
    Oversampler m_oversampler;
    bool m_useOversampling = true;
};
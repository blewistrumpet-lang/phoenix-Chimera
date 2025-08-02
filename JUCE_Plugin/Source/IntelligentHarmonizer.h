#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <memory>
#include <random>

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
    SmoothParam m_interval; // -24 to +24 semitones (normalized)
    SmoothParam m_key; // Root note (0=C, 1=C#, etc.)
    SmoothParam m_scale; // Scale type
    SmoothParam m_voiceCount; // 1-4 voices
    SmoothParam m_spread; // Stereo spread
    SmoothParam m_humanize; // Pitch/timing variation
    SmoothParam m_formant; // Formant correction
    SmoothParam m_mix;
    
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
    
    // Interval lookup tables (semitones from root)
    static constexpr int SCALE_INTERVALS[NUM_SCALES][12] = {
        {0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19}, // Major
        {0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19}, // Minor
        {0, 2, 3, 5, 7, 9, 10, 12, 14, 15, 17, 19}, // Dorian
        {0, 2, 4, 5, 7, 9, 10, 12, 14, 16, 17, 19}, // Mixolydian
        {0, 2, 3, 5, 7, 8, 11, 12, 14, 15, 17, 19}, // Harmonic Minor
        {0, 2, 3, 5, 7, 9, 11, 12, 14, 15, 17, 19}, // Melodic Minor
        {0, 2, 4, 7, 9, 12, 14, 16, 19, 21, 24, 26}, // Pentatonic Major
        {0, 3, 5, 7, 10, 12, 15, 17, 19, 22, 24, 27}, // Pentatonic Minor
        {0, 3, 5, 6, 7, 10, 12, 15, 17, 18, 19, 22}, // Blues
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11} // Chromatic (no correction)
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
            // Simulate thermal buildup from processing
            float targetTemp = 20.0f + processingLoad * 20.0f; // Up to 40°C
            temperature = temperature * thermalTimeConstant + targetTemp * (1.0f - thermalTimeConstant);
            
            // Component drift based on temperature (subtle pitch drift)
            componentDrift = (temperature - 20.0f) * 0.0001f; // ±0.2% max drift
        }
        
        float getTemperatureDrift() const {
            return componentDrift;
        }
    };
    
    // Component aging simulation
    struct ComponentAging {
        float age = 0.0f; // In processing hours
        float agingRate = 1.0f / (200.0f * 3600.0f * 44100.0f); // Age over 200 hours
        
        void update() {
            age += agingRate;
        }
        
        float getAgingFactor() const {
            // Subtle aging effects (oscillator drift, capacitor changes)
            return 1.0f + std::sin(age * 0.05f) * 0.001f; // ±0.1% variation
        }
    };
    
    // Enhanced oversampling system
    struct Oversampler {
        static constexpr int FACTOR = 4; // 4x oversampling for pitch shifting
        std::array<float, FACTOR * 256> buffer;
        int bufferIndex = 0;
        
        // Lanczos interpolation for superior quality
        struct LanczosInterpolator {
            static constexpr int TAPS = 8;
            
            float interpolate(const std::array<float, BUFFER_SIZE>& buffer, float position) {
                int intPos = static_cast<int>(position);
                float fracPos = position - intPos;
                
                float sum = 0.0f;
                for (int i = -TAPS/2; i < TAPS/2; ++i) {
                    int idx = (intPos + i + BUFFER_SIZE) % BUFFER_SIZE;
                    float x = fracPos - i;
                    
                    // Lanczos kernel
                    float kernel = (x == 0.0f) ? 1.0f : 
                                  (std::sin(M_PI * x) * std::sin(M_PI * x / (TAPS/2))) / 
                                  (M_PI * M_PI * x * x / (TAPS/2));
                    
                    sum += buffer[idx] * kernel;
                }
                return sum;
            }
        };
        
        LanczosInterpolator interpolator;
        
        void prepare(double sampleRate) {
            buffer.fill(0.0f);
            bufferIndex = 0;
        }
    };
    
    // Voice harmonizer with boutique quality
    struct HarmonizerVoice {
        // Enhanced pitch shifter using delay line interpolation
        static constexpr int BUFFER_SIZE = 16384; // Increased for better quality
        std::array<float, BUFFER_SIZE> buffer;
        float writePos = 0.0f;
        float readPos = 0.0f;
        
        // Advanced formant preservation using spectral envelope
        struct FormantPreserver {
            std::array<float, 1024> spectralEnvelope;
            std::array<float, 512> formantBuffer;
            int formantIndex = 0;
            
            void preserveFormants(float input, float pitchRatio) {
                // Simplified formant preservation
                formantBuffer[formantIndex] = input;
                formantIndex = (formantIndex + 1) % 512;
                
                // Extract spectral envelope (simplified)
                if (formantIndex == 0) {
                    for (int i = 0; i < 512; ++i) {
                        spectralEnvelope[i] = std::abs(formantBuffer[i]);
                    }
                }
            }
            
            float applyFormantCorrection(float output, float correction) {
                // Apply inverse pitch shift to maintain formants
                float formantShift = 1.0f + correction * 0.5f;
                return output * formantShift;
            }
        };
        
        FormantPreserver formantPreserver;
        
        // Boutique components
        DCBlocker inputDCBlocker, outputDCBlocker;
        ThermalModel thermalModel;
        ComponentAging componentAging;
        Oversampler oversampler;
        
        // Enhanced smoothing with multiple time constants
        float currentPitch = 0.0f;
        float targetPitch = 0.0f;
        float pitchSmoothing = 0.999f; // Slower for pitch stability
        
        // Advanced humanization
        float pitchLFO = 0.0f;
        float timingOffset = 0.0f;
        float vibratoPhase = 0.0f;
        float chorusPhase = 0.0f;
        
        // Analog noise simulation
        mutable std::mt19937 noiseGen{std::random_device{}()};
        mutable std::normal_distribution<float> noiseDist{0.0f, 1.0f};
        
        void prepare(double sampleRate) {
            buffer.fill(0.0f);
            writePos = 0.0f;
            readPos = BUFFER_SIZE * 0.5f;
            currentPitch = 1.0f;
            targetPitch = 1.0f;
            pitchLFO = 0.0f;
            vibratoPhase = 0.0f;
            chorusPhase = 0.0f;
            
            // Initialize boutique components
            inputDCBlocker.reset();
            outputDCBlocker.reset();
            thermalModel = ThermalModel();
            componentAging = ComponentAging();
            oversampler.prepare(sampleRate);
        }
        
        float addAnalogNoise(float input) {
            // Thermal noise simulation (-140dB noise floor)
            float noise = noiseDist(noiseGen) * 0.0000001f;
            return input + noise;
        }
        
        float process(float input, float pitchRatio, float formantAmount, 
                     float humanization, double sampleRate);
    };
    
    // Pitch detection for intelligent harmonization
    struct PitchDetector {
        static constexpr int WINDOW_SIZE = 2048;
        std::array<float, WINDOW_SIZE> buffer;
        int bufferIndex = 0;
        float detectedPitch = 440.0f;
        float confidence = 0.0f;
        
        void addSample(float sample);
        float detectPitch(double sampleRate);
        
    private:
        float autocorrelation(int lag);
    };
    
    // Channel state with boutique enhancements
    struct ChannelState {
        std::array<HarmonizerVoice, 4> voices;
        PitchDetector pitchDetector;
        
        // Enhanced filtering with ZDF topology
        struct ZDFFilter {
            float s = 0.0f; // State
            float g = 0.0f; // Coefficient
            
            void setCutoff(float cutoffHz, double sampleRate) {
                float wd = 2.0f * M_PI * cutoffHz;
                float T = 1.0f / sampleRate;
                float wa = (2.0f / T) * std::tan(wd * T / 2.0f);
                g = wa * T / 2.0f;
            }
            
            float processLowpass(float input) {
                float v = (input - s) / (1.0f + g);
                float lp = s + g * v;
                s = lp + g * v;
                return lp;
            }
            
            float processHighpass(float input) {
                float v = (input - s) / (1.0f + g);
                float hp = input - s - g * v;
                s += 2.0f * g * v;
                return hp;
            }
        };
        
        ZDFFilter antiAliasingFilter;
        ZDFFilter dcBlockingFilter;
        
        // Boutique components
        DCBlocker inputDCBlocker, outputDCBlocker;
        ThermalModel thermalModel;
        ComponentAging componentAging;
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Helper functions
    int quantizeToScale(int noteOffset, ScaleType scale, int key);
    float noteToFrequency(float note);
    float frequencyToNote(float frequency);
    int getActiveVoices() const;
    float calculateHarmonizedPitch(float inputNote, int voiceIndex, int interval);
};
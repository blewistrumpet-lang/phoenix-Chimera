#pragma once
#include "EngineBase.h"
#include <vector>
#include <memory>
#include <array>
#include <juce_dsp/juce_dsp.h>

class ConvolutionReverb : public EngineBase {
public:
    ConvolutionReverb();
    ~ConvolutionReverb() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Convolution Reverb"; }
    
private:
    // Smoothed parameters following boutique patterns
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
    
    SmoothParam m_mixAmount;
    SmoothParam m_preDelay;
    SmoothParam m_damping;
    SmoothParam m_size;
    SmoothParam m_width;
    SmoothParam m_modulation;
    SmoothParam m_earlyLate;
    SmoothParam m_highCut;
    
    // Enhanced convolution engines with zero-latency mode
    juce::dsp::Convolution m_convolutionEngine;
    juce::dsp::Convolution m_zeroLatencyEngine;
    bool m_useZeroLatency = false;
    
    // Advanced oversampling for pristine quality
    struct Oversampler {
        static constexpr int FACTOR = 4; // 4x oversampling for highest quality
        juce::dsp::Oversampling<float> oversampling{2, FACTOR, 
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true};
            
        void prepare(const juce::dsp::ProcessSpec& spec) {
            oversampling.initProcessing(spec.maximumBlockSize);
        }
        
        juce::dsp::AudioBlock<float> upsample(juce::dsp::AudioBlock<float>& block) {
            return oversampling.processSamplesUp(block);
        }
        
        void downsample(juce::dsp::AudioBlock<float>& block) {
            oversampling.processSamplesDown(block);
        }
        
        void reset() {
            oversampling.reset();
        }
    };
    
    Oversampler m_oversampler;
    
    // Enhanced pre-delay with modulation and filtering
    struct AdvancedPreDelay {
        juce::dsp::DelayLine<float> delayLine{192000}; // Max 4 seconds at 48kHz
        
        // Modulation system
        float modulationPhase = 0.0f;
        float modulationDepth = 0.0f;
        float modulationRate = 0.3f;
        
        // Diffusion allpasses
        struct AllPass {
            std::vector<float> buffer;
            int writePos = 0;
            float coefficient = 0.5f;
            
            void prepare(int size) {
                buffer.resize(size, 0.0f);
                writePos = 0;
            }
            
            float process(float input) {
                float delayed = buffer[writePos];
                float output = -input + delayed;
                buffer[writePos] = input + delayed * coefficient;
                writePos = (writePos + 1) % buffer.size();
                return output;
            }
        };
        
        std::array<AllPass, 4> diffusers;
        
        void prepare(const juce::dsp::ProcessSpec& spec) {
            delayLine.prepare(spec);
            
            // Initialize diffusion allpasses with prime delays
            diffusers[0].prepare(113);
            diffusers[1].prepare(337);
            diffusers[2].prepare(557);
            diffusers[3].prepare(797);
        }
        
        void setDelay(float delayMs, float modulation, double sampleRate) {
            modulationDepth = modulation * 0.001f; // Up to 1ms modulation
            float baseDelay = delayMs * 0.001f * sampleRate;
            
            // Add subtle modulation
            modulationPhase += 2.0f * M_PI * modulationRate / sampleRate;
            if (modulationPhase > 2.0f * M_PI) modulationPhase -= 2.0f * M_PI;
            
            float modAmount = std::sin(modulationPhase) * modulationDepth * sampleRate;
            float finalDelay = baseDelay + modAmount;
            
            delayLine.setDelay(std::max(1.0f, finalDelay));
        }
        
        float process(float input) {
            // Apply diffusion for more natural pre-delay
            float diffused = input;
            for (auto& diffuser : diffusers) {
                diffused = diffuser.process(diffused) * 0.3f + diffused * 0.7f;
            }
            
            return delayLine.popSample(0, diffused);
        }
    };
    
    AdvancedPreDelay m_preDelayProcessor;
    
    // Advanced filtering system
    struct FilterSystem {
        juce::dsp::StateVariableTPTFilter<float> highCutFilter;
        juce::dsp::StateVariableTPTFilter<float> dampingFilter;
        juce::dsp::StateVariableTPTFilter<float> lowShelfFilter;
        juce::dsp::StateVariableTPTFilter<float> highShelfFilter;
        
        void prepare(const juce::dsp::ProcessSpec& spec) {
            highCutFilter.prepare(spec);
            dampingFilter.prepare(spec);
            lowShelfFilter.prepare(spec);
            highShelfFilter.prepare(spec);
            
            highCutFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            dampingFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            lowShelfFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            highShelfFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        }
        
        void updateParameters(float highCut, float damping, double sampleRate) {
            // High cut with resonance control
            float cutoffFreq = 200.0f + highCut * 19800.0f;
            highCutFilter.setCutoffFrequency(cutoffFreq);
            highCutFilter.setResonance(0.5f + damping * 0.3f);
            
            // Damping filter (for late reverb)
            float dampingFreq = 1000.0f + (1.0f - damping) * 9000.0f;
            dampingFilter.setCutoffFrequency(dampingFreq);
            
            // Shelf filters for tonal shaping
            lowShelfFilter.setCutoffFrequency(300.0f);
            highShelfFilter.setCutoffFrequency(8000.0f);
        }
        
        float process(float input, int channel) {
            float filtered = highCutFilter.processSample(channel, input);
            filtered = dampingFilter.processSample(channel, filtered);
            
            // Subtle tonal shaping
            filtered = lowShelfFilter.processSample(channel, filtered) * 0.1f + filtered * 0.9f;
            filtered = highShelfFilter.processSample(channel, filtered) * 0.1f + filtered * 0.9f;
            
            return filtered;
        }
    };
    
    FilterSystem m_filterSystem;
    
    // DC Blocking
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
    
    std::array<DCBlocker, 2> m_dcBlockers;
    
    // Thermal modeling for analog warmth
    struct ThermalModel {
        float temperature = 25.0f; // Room temperature in Celsius
        float thermalNoise = 0.0f;
        float thermalDrift = 0.0f;
        
        void update(double sampleRate) {
            // Slow temperature variations
            static float phase = 0.0f;
            phase += 0.00001f / sampleRate; // Very slow variation
            temperature = 25.0f + std::sin(phase) * 2.0f; // ±2°C variation
            
            // Thermal noise increases with temperature
            float noiseLevel = (temperature - 20.0f) * 0.0001f;
            thermalNoise = ((rand() % 1000) / 1000.0f - 0.5f) * noiseLevel;
            
            // Thermal drift affects parameters
            thermalDrift = (temperature - 25.0f) * 0.002f;
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalDrift;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Component aging simulation
    float m_componentAge = 0.0f; // In hours of operation
    float m_ageNoiseFactor = 0.0f;
    float m_ageFrequencyShift = 0.0f;
    
    void updateComponentAging(double sampleRate) {
        // Age components very slowly (1 hour = 3600 seconds)
        m_componentAge += 1.0f / (sampleRate * 3600.0f);
        
        // After 1000 hours, components show some wear
        float ageYears = m_componentAge / 8760.0f; // Convert to years
        
        // Noise increases with age
        m_ageNoiseFactor = std::min(0.0001f, ageYears * 0.00001f);
        
        // Frequency response shifts slightly with age
        m_ageFrequencyShift = std::min(0.02f, ageYears * 0.005f);
    }
    
    // Advanced IR generation and management
    struct IRGenerator {
        // Multiple IR banks for different room types
        enum class RoomType {
            Chamber, Hall, Cathedral, Plate, Spring, Ambient
        };
        
        static std::vector<float> generateAdvancedIR(double sampleRate, float size, 
                                                   float damping, float earlyLate,
                                                   RoomType roomType);
        
        static void applyAdvancedEarlyReflections(std::vector<float>& ir, float amount, 
                                                float size, double sampleRate);
        
        static void applySpectralDiffusion(std::vector<float>& ir, float amount);
        
        static void applyRealisticDecay(std::vector<float>& ir, float rt60, 
                                      float damping, double sampleRate);
        
        static juce::AudioBuffer<float> createStereoIR(const std::vector<float>& monoIR, 
                                                     float width, double sampleRate);
    };
    
    // IR caching system for performance
    struct IRCache {
        struct CacheEntry {
            std::unique_ptr<juce::AudioBuffer<float>> ir;
            float size, damping, earlyLate, width;
            double sampleRate;
            bool isValid() const { return ir != nullptr; }
        };
        
        static constexpr int CACHE_SIZE = 8;
        std::array<CacheEntry, CACHE_SIZE> cache;
        int nextCacheIndex = 0;
        
        juce::AudioBuffer<float>* findCachedIR(float size, float damping, 
                                             float earlyLate, float width, 
                                             double sampleRate);
        
        void cacheIR(std::unique_ptr<juce::AudioBuffer<float>> ir, 
                    float size, float damping, float earlyLate, 
                    float width, double sampleRate);
    };
    
    IRCache m_irCache;
    
    double m_sampleRate = 44100.0;
    int m_samplesPerBlock = 512;
    bool m_needsIRUpdate = true;
    IRGenerator::RoomType m_currentRoomType = IRGenerator::RoomType::Hall;
    
    // Enhanced processing functions
    void generateEnhancedImpulseResponse();
    void updateIRIfNeeded();
    float processModulation(float input, int channel);
    
    // Analog modeling
    float applyAnalogCharacter(float input, float amount);
    float applyVintageNoise(float input);
};
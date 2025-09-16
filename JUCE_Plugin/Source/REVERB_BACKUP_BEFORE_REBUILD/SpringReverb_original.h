#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>
#include <memory>

class SpringReverb : public EngineBase {
public:
    SpringReverb();
    ~SpringReverb() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Spring Reverb"; }
    
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
    
    SmoothParam m_springCount;    // 1 to 4 springs simulation
    SmoothParam m_tension;        // Spring tension/decay time
    SmoothParam m_damping;        // High frequency damping
    SmoothParam m_preDelay;       // Pre-delay time
    SmoothParam m_modulation;     // Spring wobble amount
    SmoothParam m_drip;           // "Drip" characteristic
    SmoothParam m_tone;           // Tone control
    SmoothParam m_mix;            // Dry/wet mix
    
    // Physical modeling parameters
    static constexpr int MAX_SPRINGS = 4;
    static constexpr int WAVEGUIDE_SIZE = 4096;
    static constexpr int MAX_DELAY_SIZE = 2048;
    
    // Spring characteristics (based on real spring reverb tanks)
    struct SpringCharacteristics {
        float delay;          // Initial delay (ms)
        float decay;          // Decay time multiplier
        float dispersion;     // Frequency dispersion amount
        float modDepth;       // Modulation depth
        float modRate;        // Modulation rate (Hz)
    };
    
    static constexpr SpringCharacteristics SPRING_TYPES[4] = {
        {29.0f, 0.85f, 0.3f, 0.002f, 0.7f},   // Short spring
        {37.0f, 0.90f, 0.4f, 0.003f, 0.5f},   // Medium spring
        {41.0f, 0.93f, 0.5f, 0.004f, 0.3f},   // Long spring
        {43.0f, 0.95f, 0.6f, 0.005f, 0.2f}    // Extra long spring
    };
    
    // Enhanced spring waveguide with physical modeling
    struct SpringWaveguide {
        std::vector<float> delayLine;
        std::vector<float> auxiliaryLine; // For bidirectional propagation
        int writePos = 0;
        int size = 0;
        
        // Advanced all-pass filters for dispersion modeling
        struct AllPass {
            float state = 0.0f;
            float coefficient = 0.7f;
            float gain = 1.0f;
            
            void setCoefficient(float c, float g = 1.0f) {
                coefficient = c;
                gain = g;
            }
            
            float process(float input) {
                float output = -input * gain + state;
                state = input + coefficient * output;
                return output;
            }
            
            void reset() { state = 0.0f; }
        };
        
        std::array<AllPass, 12> dispersionFilters; // More filters for better dispersion
        
        // Multi-stage damping system
        struct DampingSystem {
            float lowpassState = 0.0f;
            float highpassState = 0.0f;
            float bandpassState = 0.0f;
            float dampingCutoff = 0.8f;
            float dampingResonance = 0.3f;
            
            float process(float input, float aging) {
                // Age-dependent frequency response
                float cutoff = dampingCutoff * (1.0f - aging * 0.3f);
                float resonance = dampingResonance * (1.0f + aging * 0.4f);
                
                // Multi-mode filtering for realistic spring damping
                lowpassState += (input - lowpassState) * cutoff;
                highpassState += (input - highpassState) * (1.0f - cutoff * 0.5f);
                float lowpass = lowpassState;
                float highpass = input - highpassState;
                
                // Resonant peak around spring resonance frequency
                bandpassState += (lowpass - bandpassState) * resonance;
                
                return lowpass + highpass * 0.1f + bandpassState * 0.2f;
            }
        };
        
        DampingSystem damping;
        
        // Complex modulation system for spring movement
        struct SpringModulation {
            float wobblePhase = 0.0f;
            float bouncePhase = 0.0f;
            float tensionPhase = 0.0f;
            
            float wobbleRate = 0.5f;      // Primary spring wobble
            float bounceRate = 2.3f;      // Higher frequency bouncing
            float tensionRate = 0.13f;    // Slow tension variations
            
            float wobbleDepth = 0.002f;
            float bounceDepth = 0.0008f;
            float tensionDepth = 0.001f;
            
            // Random spring imperfections
            float randomPhase = 0.0f;
            float randomWalk = 0.0f;
            float randomTarget = 0.0f;
            
            // Thread-safe random generation for spring imperfections
            mutable std::mt19937 springRng{std::random_device{}()};
            mutable std::uniform_int_distribution<int> changeDist{0, 1999};
            mutable std::uniform_real_distribution<float> targetDist{-1.0f, 1.0f};
            
            float process(float amount, double sampleRate) {
                float phaseInc = 2.0f * M_PI / sampleRate;
                
                // Update phases
                wobblePhase += wobbleRate * phaseInc;
                bouncePhase += bounceRate * phaseInc;
                tensionPhase += tensionRate * phaseInc;
                randomPhase += 0.1f * phaseInc;
                
                // Wrap phases
                if (wobblePhase > 2.0f * M_PI) wobblePhase -= 2.0f * M_PI;
                if (bouncePhase > 2.0f * M_PI) bouncePhase -= 2.0f * M_PI;
                if (tensionPhase > 2.0f * M_PI) tensionPhase -= 2.0f * M_PI;
                if (randomPhase > 2.0f * M_PI) randomPhase -= 2.0f * M_PI;
                
                // Random walk for imperfections (thread-safe)
                if (changeDist(springRng) < 5) { // Very occasional changes
                    randomTarget = targetDist(springRng);
                }
                randomWalk += (randomTarget - randomWalk) * 0.0001f;
                
                // Combine modulations
                float wobble = std::sin(wobblePhase) * wobbleDepth;
                float bounce = std::sin(bouncePhase) * bounceDepth;
                float tension = std::sin(tensionPhase) * tensionDepth;
                float random = randomWalk * 0.0003f;
                
                return (wobble + bounce + tension + random) * amount;
            }
        };
        
        SpringModulation modulation;
        
        void prepare(int maxSize) {
            delayLine.resize(maxSize);
            auxiliaryLine.resize(maxSize);
            std::fill(delayLine.begin(), delayLine.end(), 0.0f);
            std::fill(auxiliaryLine.begin(), auxiliaryLine.end(), 0.0f);
            writePos = 0;
            size = maxSize;
            
            // Initialize dispersion filters with frequency-dependent coefficients
            for (size_t i = 0; i < dispersionFilters.size(); ++i) {
                float freq = 0.3f + 0.6f * (i / float(dispersionFilters.size()));
                float coeff = 0.4f + 0.5f * freq;
                float gain = 0.8f + 0.3f * (1.0f - freq);
                dispersionFilters[i].setCoefficient(coeff, gain);
            }
        }
        
        float process(float input, float feedback, float aging, double sampleRate) {
            // Denormal prevention
            input += 1e-10f;
            
            // Advanced modulation processing - use modulation parameter directly
            // Note: modulation amount should be passed from the main process function
            float springModulation = modulation.process(1.0f, sampleRate); // The amount is controlled by wobbleDepth/bounceDepth set externally
            float modulatedDelay = size + springModulation * size * 0.05f; // Much larger modulation for audible effect
            
            // Ensure modulated delay is within bounds (use actual size, not buffer size)
            modulatedDelay = std::clamp(modulatedDelay, 1.0f, static_cast<float>(size - 1));
            
            // Hermite interpolation for high-quality delay reading
            float readPos = writePos - modulatedDelay;
            while (readPos < 0) readPos += size;  // Use actual delay size
            
            int idx0 = static_cast<int>(readPos) % size;
            int idx1 = (idx0 + 1) % size;
            int idx2 = (idx0 + 2) % size;
            int idx3 = (idx0 + 3) % size;
            float frac = readPos - idx0;
            
            // Hermite interpolation coefficients
            float c0 = delayLine[idx1];
            float c1 = 0.5f * (delayLine[idx2] - delayLine[idx0]);
            float c2 = delayLine[idx0] - 2.5f * delayLine[idx1] + 2.0f * delayLine[idx2] - 0.5f * delayLine[idx3];
            float c3 = 0.5f * (delayLine[idx3] - delayLine[idx0]) + 1.5f * (delayLine[idx1] - delayLine[idx2]);
            
            float delayed = ((c3 * frac + c2) * frac + c1) * frac + c0;
            
            // Process through dispersion filters with controlled gain
            for (auto& filter : dispersionFilters) {
                delayed = filter.process(delayed) * 0.98f; // Gentler damping for better tone
            }
            
            // Apply advanced damping system
            delayed = damping.process(delayed, aging);
            
            // Bidirectional wave propagation modeling with stable gains
            float forward = delayed * 0.6f; // Further reduced for stability
            float backward = auxiliaryLine[writePos] * 0.3f; // Much less backward energy
            auxiliaryLine[writePos] = forward * 0.02f; // Minimal reflections
            
            // Controlled feedback with proper gain staging
            float nonlinearFeedback = feedback * 0.75f; // Further reduced for stability
            
            // Soft limiting for spring saturation (unity gain)
            if (std::abs(delayed) > 0.8f) {
                delayed = 0.8f * std::tanh(delayed / 0.8f); // Unity-gain limiter
            }
            
            // Mix input and feedback with conservative gain staging
            float toWrite = input * 0.4f + delayed * nonlinearFeedback + backward * 0.05f;
            
            // Denormal flush
            if (std::abs(toWrite) < 1e-10f) toWrite = 0.0f;
            
            // Soft clipping to prevent distortion
            if (std::abs(toWrite) > 1.0f) {
                toWrite = std::tanh(toWrite * 0.8f) * 1.25f;
            }
            
            delayLine[writePos] = toWrite;
            writePos = (writePos + 1) % size;  // Use actual delay size
            
            return delayed * 0.8f + backward * 0.1f; // Reduced output gain
        }
        
        void setSpringCharacteristics(const SpringCharacteristics& chars, double sampleRate) {
            size = static_cast<int>(chars.delay * sampleRate * 0.001);
            size = std::min(size, static_cast<int>(delayLine.size()));
            
            // Update modulation parameters
            modulation.wobbleDepth = chars.modDepth;
            modulation.wobbleRate = chars.modRate;
            modulation.bounceDepth = chars.modDepth * 0.4f;
            modulation.tensionDepth = chars.modDepth * 0.5f;
            
            // Adjust dispersion based on spring characteristics
            for (size_t i = 0; i < dispersionFilters.size(); ++i) {
                float base = 0.3f + chars.dispersion * 0.5f;
                float coeff = base + 0.3f * (i / float(dispersionFilters.size()));
                float gain = 0.8f + chars.dispersion * 0.3f;
                dispersionFilters[i].setCoefficient(coeff, gain);
            }
        }
        
        void setDamping(float dampingAmount) {
            damping.dampingCutoff = 1.0f - dampingAmount * 0.6f;
            damping.dampingResonance = 0.2f + dampingAmount * 0.4f;
        }
        
        float getLastOutput() const {
            // Return the current delay line output for energy monitoring
            int readIdx = (writePos - 1 + size) % size;
            return delayLine[readIdx];
        }
        
        void reset() {
            std::fill(delayLine.begin(), delayLine.end(), 0.0f);
            std::fill(auxiliaryLine.begin(), auxiliaryLine.end(), 0.0f);
            for (auto& filter : dispersionFilters) {
                filter.reset();
            }
            damping = DampingSystem{};
            modulation = SpringModulation{};
            writePos = 0;
        }
    };
    
    // Drip generator (characteristic spring sound)
    struct DripGenerator {
        std::mt19937 rng{42};
        std::uniform_real_distribution<float> dist{0.0f, 1.0f};
        float threshold = 0.98f;
        float lastSample = 0.0f;
        
        float process(float input, float amount) {
            if (amount < 0.01f) return 0.0f;
            
            // Detect transients
            float diff = std::abs(input - lastSample);
            lastSample = input;
            
            // Generate drip on transients - lower threshold and always some probability
            if (diff > 0.01f && dist(rng) > threshold) {  // Much lower threshold
                float drip = (dist(rng) - 0.5f) * 2.0f;
                drip *= std::exp(-diff * 5.0f); // Slower decay
                return drip * amount;
            }
            
            // Also generate random drips based on amount even without transients
            if (dist(rng) > (1.0f - amount * 0.001f)) {
                float randomDrip = (dist(rng) - 0.5f) * amount * 0.5f;
                return randomDrip;
            }
            
            return 0.0f;
        }
        
        void setThreshold(float amount) {
            threshold = 1.0f - amount * 0.3f; // More drips with higher amount - increased sensitivity
        }
    };
    
    // Pre-delay
    struct PreDelay {
        std::vector<float> buffer;
        int writePos = 0;
        int delayTime = 0;
        
        void prepare(int maxSize) {
            buffer.resize(maxSize);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
        
        float process(float input) {
            buffer[writePos] = input;
            
            int readPos = writePos - delayTime;
            if (readPos < 0) readPos += buffer.size();
            
            float output = buffer[readPos];
            writePos = (writePos + 1) % buffer.size();
            
            return output;
        }
        
        void setDelayTime(float ms, double sampleRate) {
            delayTime = static_cast<int>(ms * sampleRate * 0.001);
            delayTime = std::max(0, std::min(delayTime, static_cast<int>(buffer.size()) - 1));
        }
    };
    
    // Tone control (tilt EQ)
    struct ToneControl {
        float lowState = 0.0f;
        float highState = 0.0f;
        
        float process(float input, float tone) {
            // Enhanced tilt EQ with more dramatic effect
            float lowCutoff = 0.02f + tone * 0.08f;   // Variable low cutoff
            float highCutoff = 0.1f + tone * 0.4f;    // Variable high cutoff
            
            // Extract bands with improved filtering
            lowState += (input - lowState) * lowCutoff;
            float high = input - lowState;
            highState += (high - highState) * highCutoff;
            
            // More dramatic tone shaping
            float low = lowState;
            high = highState;
            
            // Create more extreme tone variations
            if (tone < 0.5f) {
                // Emphasize lows, reduce highs
                float darkening = (0.5f - tone) * 2.0f;
                low = low * (1.0f + darkening * 0.8f);
                high = high * (1.0f - darkening * 0.6f);
            } else {
                // Emphasize highs, reduce lows
                float brightening = (tone - 0.5f) * 2.0f;
                low = low * (1.0f - brightening * 0.6f);
                high = high * (1.0f + brightening * 0.8f);
            }
            
            return low + high;
        }
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
    
    // Oversampling for reduced aliasing
    struct Oversampler {
        static constexpr int FACTOR = 2;
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        
        // Simple 2x oversampling with basic anti-aliasing
        struct AntiAliasFilter {
            float z1 = 0.0f, z2 = 0.0f;
            
            float process(float input) {
                // Simple 2-pole Butterworth at Nyquist/2
                float output = input * 0.067455273 + z1 * 0.134910546 + z2 * 0.067455273 
                             - z1 * -1.142980502 - z2 * 0.412801336;
                z2 = z1;
                z1 = input;
                return output;
            }
            
            void reset() { z1 = z2 = 0.0f; }
        };
        
        AntiAliasFilter upFilter, downFilter;
        
        void prepare(int maxSamples) {
            upsampleBuffer.resize(maxSamples * FACTOR);
            downsampleBuffer.resize(maxSamples * FACTOR);
        }
        
        void upsample(const float* input, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                upsampleBuffer[i * FACTOR] = upFilter.process(input[i] * FACTOR);
                upsampleBuffer[i * FACTOR + 1] = 0.0f; // Zero-stuff
            }
        }
        
        void downsample(float* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                output[i] = downFilter.process(downsampleBuffer[i * FACTOR]) / FACTOR;
            }
        }
    };
    
    // Enhanced channel state with boutique processing
    struct ChannelState {
        std::array<SpringWaveguide, MAX_SPRINGS> springs;
        DripGenerator dripGen;
        PreDelay preDelay;
        ToneControl toneControl;
        DCBlocker dcBlocker;
        Oversampler oversampler;
        
        // Advanced feedback matrix for inter-spring coupling
        std::array<std::array<float, MAX_SPRINGS>, MAX_SPRINGS> feedbackMatrix;
        
        // Additional processing elements
        float springAging = 0.0f; // Simulates spring aging over time
        
        void updateAging(float aging) {
            springAging = aging;
        }
        
        void prepare(double sampleRate) {
            for (auto& spring : springs) {
                spring.prepare(WAVEGUIDE_SIZE);
            }
            
            preDelay.prepare(MAX_DELAY_SIZE);
            oversampler.prepare(1024);
            
            // Initialize advanced feedback matrix with reduced coupling
            for (int i = 0; i < MAX_SPRINGS; ++i) {
                for (int j = 0; j < MAX_SPRINGS; ++j) {
                    if (i == j) {
                        feedbackMatrix[i][j] = 0.65f; // Further reduced self-feedback for stability
                    } else {
                        // Minimal cross-coupling to prevent buildup
                        float proximity = 1.0f / (1.0f + std::abs(i - j));
                        feedbackMatrix[i][j] = 0.005f + 0.01f * proximity; // Much smaller coupling for stability
                    }
                }
            }
        }
        
        void reset() {
            for (auto& spring : springs) {
                spring.reset();
            }
            dcBlocker.reset();
            oversampler.upFilter.reset();
            oversampler.downFilter.reset();
            springAging = 0.0f;
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Input diffusion
    struct InputDiffuser {
        std::array<float, 4> states = {0};
        std::array<float, 4> coeffs = {0.75f, 0.70f, 0.65f, 0.60f};
        
        float process(float input) {
            float output = input;
            for (size_t i = 0; i < states.size(); ++i) {
                float temp = output;
                output = -output + states[i];
                states[i] = temp + coeffs[i] * output;
            }
            return output;
        }
    };
    
    InputDiffuser m_inputDiffuser;
    
    // Thermal modeling for boutique realism
    struct ThermalModel {
        float temperature = 25.0f;  // Celsius
        float thermalNoise = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        ThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate) {
            // Slow thermal drift
            thermalNoise += (dist(rng) * 0.0008f) / sampleRate;
            thermalNoise = std::max(-0.015f, std::min(0.015f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Component aging simulation
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
};
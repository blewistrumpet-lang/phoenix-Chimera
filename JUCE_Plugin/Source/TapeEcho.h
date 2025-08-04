#pragma once

#include "../Source/EngineBase.h"
#include <JuceHeader.h>
#include <vector>
#include <array>
#include <memory>
#include <random>
#include <cmath>
#include <algorithm>

/**
 * TapeEcho - Authentic analog tape delay emulation
 * 
 * Features:
 * - Multiple tape head simulation with authentic wow/flutter
 * - Tape saturation with hysteresis modeling
 * - Age-dependent frequency response
 * - Tape compression and self-oscillation
 * - Multi-tap delay architecture
 * - Realistic tape transport mechanics
 * 
 * Technical notes:
 * - Uses interpolated delay lines for smooth modulation
 * - Models tape head bump and frequency losses
 * - Includes pre-emphasis/de-emphasis filtering
 * - Thread-safe parameter smoothing
 * 
 * @param Time [0,1] - Delay time from 10ms to 2000ms
 * @param Feedback [0,1] - Regeneration amount (>0.75 = self-oscillation)
 * @param Wow/Flutter [0,1] - Tape transport instability
 * @param Saturation [0,1] - Tape compression and harmonic distortion
 * @param Mix [0,1] - Dry/wet balance
 */
class TapeEcho : public EngineBase {
public:
    TapeEcho();
    ~TapeEcho() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Tape Echo"; }
    int getNumParameters() const override { return 5; }
    juce::String getParameterName(int index) const override;
    
private:
    // Constants
    static constexpr float MAX_DELAY_MS = 2000.0f;
    static constexpr float MIN_DELAY_MS = 10.0f;
    static constexpr float TAPE_SPEED_IPS = 7.5f;  // Inches per second
    static constexpr float HEAD_GAP_INCHES = 1.0f;  // Space between heads
    static constexpr int MAX_BLOCK_SIZE = 2048;
    
    // LFO rates (exposed as constants for tweaking)
    static constexpr float WOW_RATE = 0.5f;          // Hz
    static constexpr float FLUTTER_RATE1 = 5.2f;     // Hz
    static constexpr float FLUTTER_RATE2 = 6.7f;     // Hz
    static constexpr float DRIFT_RATE = 0.08f;       // Hz
    static constexpr float SCRAPE_RATE = 47.0f;      // Hz
    static constexpr float MECHANICAL_RES_FREQ = 2.3f; // Hz
    
    // LFO depths (exposed for tweaking)
    static constexpr float WOW_DEPTH = 0.015f;       // ±1.5%
    static constexpr float FLUTTER_DEPTH1 = 0.004f;  // ±0.4%
    static constexpr float FLUTTER_DEPTH2 = 0.003f;  // ±0.3%
    static constexpr float DRIFT_DEPTH = 0.008f;     // ±0.8%
    static constexpr float SCRAPE_DEPTH = 0.0005f;   // ±0.05%
    
    // Smoothed parameters with proper time constants
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
        
        void setSmoothingTime(float timeMs, double sampleRate) {
            float samples = timeMs * 0.001f * static_cast<float>(sampleRate);
            smoothing = std::exp(-2.0f * M_PI / samples);
        }
    };
    
    SmoothParam m_time;
    SmoothParam m_feedback;
    SmoothParam m_wowFlutter;
    SmoothParam m_saturation;
    SmoothParam m_mix;
    
    double m_sampleRate = 44100.0;
    int m_maxBlockSize = MAX_BLOCK_SIZE;
    
    // Filter coefficient update tracking
    float m_lastSaturation = -1.0f;
    
    // Enhanced delay line with fractional delay
    class DelayLine {
    public:
        void prepare(double sampleRate, float maxDelayMs);
        void write(float sample);
        float read(float delaySamples) const;
        float readInterpolated(float delaySamples) const;
        void clear();
        int getMaxDelaySamples() const { return m_size - 4; } // Public accessor
        
    private:
        std::vector<float> m_buffer;
        int m_writePos = 0;
        int m_size = 0;
        
        // Hermite interpolation for better quality
        float hermiteInterpolate(float fractionalDelay, 
                               float y0, float y1, float y2, float y3) const;
    };
    
    // Enhanced tape modulation with multiple components
    class TapeModulation {
    public:
        TapeModulation();
        void reset();
        void prepare(double sampleRate);
        float process(float amount);
        void updateRandomTarget(); // Called once per block
        
    private:
        // Multiple LFO phases
        float m_wowPhase = 0.0f;
        float m_flutterPhase1 = 0.0f;
        float m_flutterPhase2 = 0.0f;
        float m_driftPhase = 0.0f;
        float m_scrapePhase = 0.0f;
        
        // Pre-computed phase increment
        float m_phaseIncrement = 0.0f;
        
        // Random modulation (simplified)
        float m_randomWalk = 0.0f;
        float m_randomTarget = 0.0f;
        uint32_t m_rngState = 1; // Simple LCG state
        
        // Mechanical resonance simulation
        float m_mechanicalResonance = 0.0f;
        float m_resonanceCoeff = 0.0f;
        
        // Fast random number generator
        float fastRandom() {
            m_rngState = m_rngState * 1664525u + 1013904223u;
            return (m_rngState & 0x7FFFFFFF) / float(0x7FFFFFFF) * 2.0f - 1.0f;
        }
    };
    
    // Enhanced tape saturation with proper hysteresis
    class TapeSaturation {
    public:
        TapeSaturation();
        void reset();
        float process(float input, float amount, float bias);
        
    private:
        // Hysteresis model
        float m_prevInput = 0.0f;
        float m_prevOutput = 0.0f;
        float m_hysteresisState = 0.0f;
        float m_magnetization = 0.0f;
        
        // Hysteresis decay factor (exposed for tweaking)
        static constexpr float HYSTERESIS_DECAY = 0.999f;
        
        // Tape characteristics
        static constexpr float COERCIVITY = 0.5f;
        static constexpr float REMANENCE = 0.8f;
        static constexpr float SATURATION_LEVEL = 0.9f;
        
        // Soft clipper with hysteresis
        float softClipWithHysteresis(float input, float drive);
    };
    
    // Multi-stage tape filtering
    class TapeFilter {
    public:
        void prepare(double sampleRate);
        void reset();
        float processRecord(float input);
        float processPlayback(float input, float tapeAge);
        void updateCoefficients(float tapeAge, double sampleRate);
        
    private:
        // State Variable Filters for stability
        struct SVFilter {
            float s1 = 0.0f, s2 = 0.0f;
            float g = 0.0f, k = 1.0f;
            
            void setFrequency(float freq, double sampleRate) {
                g = std::tan(M_PI * freq / sampleRate);
            }
            
            void setResonance(float q) {
                k = 1.0f / q;
            }
            
            float processLowpass(float input) {
                float v = (input - s2 - k * s1) / (1.0f + g * (g + k));
                float lp = v + s1;
                s1 = lp + v;
                s2 = s1 + lp;
                return lp;
            }
            
            float processHighpass(float input) {
                float v = (input - s2 - k * s1) / (1.0f + g * (g + k));
                float hp = input - k * v - s2;
                s1 = v + s1;
                s2 = s1 + v;
                return hp;
            }
            
            float processBandpass(float input) {
                float v = (input - s2 - k * s1) / (1.0f + g * (g + k));
                float bp = k * v;
                s1 = v + s1;
                s2 = s1 + v;
                return bp;
            }
            
            void reset() {
                s1 = s2 = 0.0f;
            }
        };
        
        // Filter stages
        SVFilter m_recordEQ;      // Pre-emphasis
        SVFilter m_headBump;      // Low-mid resonance
        SVFilter m_gapLoss;       // High frequency loss
        SVFilter m_biasFilter;    // Bias frequency trap
        
        // DC blocker
        float m_dcBlockerX = 0.0f, m_dcBlockerY = 0.0f;
        
        // Current filter settings (to avoid recalculation)
        float m_currentTapeAge = -1.0f;
    };
    
    // Tape compression with program-dependent behavior
    class TapeCompressor {
    public:
        void reset();
        float process(float input, float amount);
        
    private:
        float m_envelope = 0.0f;
        float m_attackTime = 0.01f;
        float m_releaseTime = 0.0005f;
        
        // Program-dependent timing
        void updateTimeConstants(float inputLevel);
    };
    
    // Per-channel processing state
    struct ChannelState {
        DelayLine delayLine;
        TapeModulation modulation;
        TapeSaturation saturation;
        TapeFilter filter;
        TapeCompressor compressor;
        
        // Feedback path state
        struct FeedbackState {
            float highpassState = 0.0f;
            float lowpassState = 0.0f;
            float delayedSample = 0.0f;
            
            // Pre-computed filter coefficients
            float hpAlpha = 0.0f;
            float lpAlpha = 0.0f;
            
            void reset() {
                highpassState = lowpassState = delayedSample = 0.0f;
            }
        } feedback;
        
        // Tape characteristics
        float tapeAge = 0.0f;  // 0 = new, 1 = worn
        static constexpr float TAPE_BIAS = 0.05f; // Bias current
        
        void prepare(double sampleRate);
        void reset();
    };
    
    std::array<ChannelState, 2> m_channelStates;
    
    // Processing functions
    float processSample(float input, int channel);
    float processFeedback(float signal, ChannelState& state);
    float applyTapeCharacter(float signal, ChannelState& state);
};
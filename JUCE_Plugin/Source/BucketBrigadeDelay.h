#pragma once

#include "../Source/EngineBase.h"
#include <vector>
#include <array>
#include <memory>

class BucketBrigadeDelay : public EngineBase {
public:
    BucketBrigadeDelay();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Bucket Brigade Delay"; }
    int getNumParameters() const override { return 5; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters with smoothing
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
    
    SmoothParam m_delayTime;
    SmoothParam m_feedback;
    SmoothParam m_mix;
    SmoothParam m_clockNoise;
    SmoothParam m_age;  // BBD aging characteristic
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // Enhanced BBD simulation with multiple delay line modeling
    static constexpr int BBD_STAGES = 1024;
    static constexpr int MAX_DELAY_MS = 600; // Extended range
    
    struct DelayLine {
        std::vector<float> buffer;
        int writePos = 0;
        
        void prepare(int maxSamples) {
            buffer.resize(maxSamples + 1, 0.0f);
            writePos = 0;
        }
        
        void write(float sample) {
            buffer[writePos] = sample;
            writePos = (writePos + 1) % buffer.size();
        }
        
        float readInterpolated(float delaySamples) {
            float readPos = writePos - delaySamples;
            while (readPos < 0) readPos += buffer.size();
            
            int pos1 = static_cast<int>(readPos);
            int pos2 = (pos1 + 1) % buffer.size();
            float frac = readPos - pos1;
            
            return buffer[pos1] * (1.0f - frac) + buffer[pos2] * frac;
        }
        
        void clear() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        }
    };
    
    // BBD Clock and Stage Modeling
    struct BBDStageProcessor {
        // Multiple clocks for realistic BBD behavior
        struct ClockGenerator {
            float phase = 0.0f;
            float nominalRate = 100000.0f; // 100kHz nominal
            float jitter = 0.0f;
            float drift = 0.0f;
            
            // Clock noise components
            float highFreqNoise = 0.0f;
            float lowFreqNoise = 0.0f;
            float noisePhase1 = 0.0f;
            float noisePhase2 = 0.0f;
            
            float generateClock(float amount, double sampleRate);
        };
        
        ClockGenerator clock1, clock2; // Dual-phase clocking
        
        // Stage capacitors (charge storage)
        std::array<float, BBD_STAGES> stageCapacitors;
        std::array<float, BBD_STAGES> chargeLeakage;
        
        // Transfer characteristics
        float transferEfficiency = 0.98f; // Charge transfer efficiency
        float feedthrough = 0.002f;       // Capacitive feedthrough
        
        void reset() {
            stageCapacitors.fill(0.0f);
            chargeLeakage.fill(0.0f);
            clock1 = ClockGenerator{};
            clock2 = ClockGenerator{};
        }
        
        float process(float input, float delayTime, float clockNoise, float aging, double sampleRate);
    };
    
    // Companding system (faithful to MN3xxx series)
    struct CompandingProcessor {
        // VCA states
        float compressorGain = 1.0f;
        float expanderGain = 1.0f;
        
        // Envelope followers
        float compEnvelope = 0.0f;
        float expEnvelope = 0.0f;
        
        // Bias and offset
        float dcBias = 0.0f;
        float offsetDrift = 0.0f;
        
        float processCompress(float input);
        float processExpand(float input, float aging);
        void updateGains(float level);
    };
    
    // Anti-aliasing and reconstruction filters
    struct BBDFilter {
        // Input anti-aliasing (pre-BBD)
        struct ButterworthLP {
            float x1 = 0.0f, x2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
            float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
            float b1 = 0.0f, b2 = 0.0f;
            
            void updateCoefficients(float freq, double sampleRate);
            float process(float input);
            void reset() { x1 = x2 = y1 = y2 = 0.0f; }
        };
        
        ButterworthLP inputFilter;
        ButterworthLP outputFilter;
        
        // Pre-emphasis/de-emphasis
        float preEmphZ1 = 0.0f;
        float deEmphZ1 = 0.0f;
        
        float processInput(float input, float aging, double sampleRate);
        float processOutput(float input, float aging, double sampleRate);
        void reset();
    };
    
    // Per-channel processing state
    struct ChannelState {
        DelayLine delayLine;
        BBDStageProcessor bbdProcessor;
        CompandingProcessor companding;
        BBDFilter filtering;
        
        // Feedback processing
        float feedbackSample = 0.0f;
        float feedbackHighpass = 0.0f;
        float feedbackSaturation = 0.0f;
        
        // DC blocking
        float dcBlockerX = 0.0f;
        float dcBlockerY = 0.0f;
        
        void prepare(double sampleRate) {
            int maxSamples = static_cast<int>(MAX_DELAY_MS * 0.001 * sampleRate) + 1;
            delayLine.prepare(maxSamples);
            bbdProcessor.reset();
            filtering.reset();
        }
        
        void reset() {
            delayLine.clear();
            bbdProcessor.reset();
            filtering.reset();
            feedbackSample = 0.0f;
            feedbackHighpass = 0.0f;
            feedbackSaturation = 0.0f;
            dcBlockerX = dcBlockerY = 0.0f;
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    
    // Analog modeling parameters
    struct AnalogModeling {
        float temperatureDrift = 0.0f;
        float supplyVariation = 0.0f;
        float componentAging = 0.0f;
        
        void update(double sampleRate) {
            // Slow parameter drift
            temperatureDrift += (((rand() % 1000) / 1000.0f - 0.5f) * 0.001f) / sampleRate;
            temperatureDrift = std::max(-0.02f, std::min(0.02f, temperatureDrift));
        }
    };
    
    AnalogModeling m_analogModeling;
    
    // Processing functions
    float processSample(float input, int channel);
    float processFeedback(float sample, int channel);
    float applyDCBlocking(float input, int channel);
    
    // Utility functions
    inline float softClip(float x) {
        return std::tanh(x * 0.7f) / 0.7f;
    }
    
    inline float softLimit(float x, float limit = 0.95f) {
        return x > limit ? limit : (x < -limit ? -limit : x);
    }
};
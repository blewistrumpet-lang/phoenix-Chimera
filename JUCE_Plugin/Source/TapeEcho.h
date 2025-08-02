#pragma once

#include "../Source/EngineBase.h"
#include <vector>
#include <array>
#include <memory>

class TapeEcho : public EngineBase {
public:
    TapeEcho();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Tape Echo"; }
    int getNumParameters() const override { return 5; }
    juce::String getParameterName(int index) const override;
    
private:
    // Smoothed parameters
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
    
    SmoothParam m_time;
    SmoothParam m_feedback;
    SmoothParam m_wowFlutter;
    SmoothParam m_saturation;
    SmoothParam m_mix;
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // Delay line with multiple tap points
    struct DelayLine {
        std::vector<float> buffer;
        int writePos = 0;
        float maxDelayMs = 2000.0f; // 2 seconds max
        
        void prepare(double sampleRate) {
            int size = static_cast<int>(maxDelayMs * 0.001 * sampleRate) + 1;
            buffer.resize(size, 0.0f);
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
    
    // Tape modulation system
    struct TapeModulation {
        // Multiple LFOs for realistic tape wobble
        float wowPhase = 0.0f;
        float flutterPhase1 = 0.0f;
        float flutterPhase2 = 0.0f;
        float driftPhase = 0.0f;
        
        // Rates
        float wowRate = 0.5f;      // 0.5 Hz
        float flutterRate1 = 5.2f;  // 5.2 Hz
        float flutterRate2 = 6.7f;  // 6.7 Hz (slightly detuned)
        float driftRate = 0.08f;    // Very slow drift
        
        // Random walk for organic variation
        float randomWalk = 0.0f;
        float randomTarget = 0.0f;
        float randomSmooth = 0.999f;
        
        float process(float amount, double sampleRate) {
            // Update phases
            float phaseInc = 2.0f * M_PI / sampleRate;
            wowPhase += wowRate * phaseInc;
            flutterPhase1 += flutterRate1 * phaseInc;
            flutterPhase2 += flutterRate2 * phaseInc;
            driftPhase += driftRate * phaseInc;
            
            // Wrap phases
            auto wrapPhase = [](float& phase) {
                while (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
            };
            wrapPhase(wowPhase);
            wrapPhase(flutterPhase1);
            wrapPhase(flutterPhase2);
            wrapPhase(driftPhase);
            
            // Update random walk
            if ((rand() % 1000) < 10) { // 1% chance per sample
                randomTarget = (rand() / float(RAND_MAX)) * 2.0f - 1.0f;
            }
            randomWalk += (randomTarget - randomWalk) * (1.0f - randomSmooth);
            
            // Combine modulations
            float wow = std::sin(wowPhase) * 0.015f; // ±1.5% pitch
            float flutter1 = std::sin(flutterPhase1) * 0.004f; // ±0.4%
            float flutter2 = std::sin(flutterPhase2) * 0.003f; // ±0.3%
            float drift = std::sin(driftPhase) * 0.008f; // ±0.8%
            float random = randomWalk * 0.002f; // ±0.2%
            
            return (wow + flutter1 + flutter2 + drift + random) * amount;
        }
    };
    
    // Tape saturation modeling
    struct TapeSaturation {
        // Hysteresis state
        float prevInput = 0.0f;
        float prevOutput = 0.0f;
        float hysteresisState = 0.0f;
        
        // Bias
        float bias = 0.05f;
        
        float process(float input, float amount) {
            // Input gain staging
            float drive = 1.0f + amount * 4.0f;
            float x = input * drive;
            
            // Simple hysteresis modeling
            float delta = x - prevInput;
            hysteresisState += delta * (1.0f - std::abs(hysteresisState) * 0.5f);
            
            // Asymmetric saturation with bias
            float biased = x + bias * amount;
            
            // Soft saturation curve
            float y;
            if (std::abs(biased) < 0.5f) {
                y = biased;
            } else {
                float sign = biased < 0 ? -1.0f : 1.0f;
                float excess = std::abs(biased) - 0.5f;
                y = sign * (0.5f + std::tanh(excess * 2.0f) * 0.5f);
            }
            
            // Add subtle harmonics
            float harmonics = y * y * y * amount * 0.1f;
            y += harmonics;
            
            // Mix with hysteresis
            y = y * 0.9f + hysteresisState * 0.1f * amount;
            
            // Update states
            prevInput = x;
            prevOutput = y;
            
            return y * (1.0f / drive) * 0.9f; // Compensate for drive
        }
    };
    
    // Filtering
    struct TapeFilter {
        // Multi-stage filtering for authentic tape response
        
        // Pre-emphasis/de-emphasis
        float preEmphZ1 = 0.0f;
        float deEmphZ1 = 0.0f;
        
        // Tape head bump (low-mid resonance)
        float bumpZ1 = 0.0f;
        float bumpZ2 = 0.0f;
        
        // High frequency loss
        float hfLossZ1 = 0.0f;
        
        // Low frequency loss  
        float lfLossZ1 = 0.0f;
        
        float processInput(float input, float age) {
            // Pre-emphasis (boost highs before recording)
            float preEmphFreq = 2000.0f;
            float preEmphAmount = 0.3f;
            // Simplified for now - would implement proper pre-emphasis
            
            return input;
        }
        
        float processPlayback(float input, float age, double sampleRate) {
            // Tape head bump (resonance around 80-120Hz)
            float bumpFreq = 100.0f / sampleRate;
            float bumpQ = 2.0f - age * 1.5f; // Less resonance with age
            float bumpGain = 1.0f + (1.0f - age) * 0.3f;
            
            // Implement simple resonant filter
            float omega = 2.0f * M_PI * bumpFreq;
            float sin_omega = std::sin(omega);
            float cos_omega = std::cos(omega);
            float alpha = sin_omega / (2.0f * bumpQ);
            
            float a0 = 1.0f + alpha;
            float a1 = -2.0f * cos_omega;
            float a2 = 1.0f - alpha;
            float b0 = 1.0f + alpha * bumpGain;
            float b1 = -2.0f * cos_omega;
            float b2 = 1.0f - alpha * bumpGain;
            
            float bumpOut = (b0 * input + b1 * bumpZ1 + b2 * bumpZ2 - a1 * bumpZ1 - a2 * bumpZ2) / a0;
            bumpZ2 = bumpZ1;
            bumpZ1 = input;
            
            // High frequency loss (varies with tape age/speed)
            float hfCutoff = (8000.0f - age * 6000.0f) / sampleRate; // 8kHz down to 2kHz
            float hfAlpha = hfCutoff / (hfCutoff + 1.0f);
            hfLossZ1 += hfAlpha * (bumpOut - hfLossZ1);
            
            // Low frequency loss
            float lfCutoff = (30.0f + age * 50.0f) / sampleRate; // 30Hz up to 80Hz
            float lfAlpha = lfCutoff / (lfCutoff + 1.0f);
            float lfHighpass = hfLossZ1 - lfLossZ1;
            lfLossZ1 += lfAlpha * (hfLossZ1 - lfLossZ1);
            
            return lfHighpass;
        }
        
        void reset() {
            preEmphZ1 = deEmphZ1 = 0.0f;
            bumpZ1 = bumpZ2 = 0.0f;
            hfLossZ1 = lfLossZ1 = 0.0f;
        }
    };
    
    // Tape compression
    struct TapeCompression {
        float envelope = 0.0f;
        
        float process(float input, float amount) {
            // Simple tape compression model
            float attackTime = 0.95f;
            float releaseTime = 0.995f;
            
            float inputLevel = std::abs(input);
            
            if (inputLevel > envelope) {
                envelope += (inputLevel - envelope) * (1.0f - attackTime);
            } else {
                envelope += (inputLevel - envelope) * (1.0f - releaseTime);
            }
            
            // Soft knee compression
            float threshold = 0.5f;
            float ratio = 1.0f + amount * 2.0f; // 1:1 to 3:1
            
            float gain = 1.0f;
            if (envelope > threshold) {
                float excess = envelope - threshold;
                float compressedExcess = excess / ratio;
                gain = (threshold + compressedExcess) / envelope;
            }
            
            return input * gain;
        }
    };
    
    // Per-channel processing state
    struct ChannelState {
        DelayLine delayLine;
        TapeModulation modulation;
        TapeSaturation saturation;
        TapeFilter filter;
        TapeCompression compression;
        
        // Feedback filtering
        float feedbackHighpass = 0.0f;
        float feedbackLowpass = 0.0f;
        
        void prepare(double sampleRate) {
            delayLine.prepare(sampleRate);
            filter.reset();
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    
    // Processing functions
    float processSample(float input, int channel);
};
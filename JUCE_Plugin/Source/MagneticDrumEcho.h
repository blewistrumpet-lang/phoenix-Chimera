#pragma once

#include "../Source/EngineBase.h"
#include <vector>
#include <array>
#include <cmath>

class MagneticDrumEcho : public EngineBase {
public:
    MagneticDrumEcho();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Magnetic Drum Echo"; }
    int getNumParameters() const override { return 6; }
    juce::String getParameterName(int index) const override;
    
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
    };
    
    SmoothParam m_delayTime;      // Delay time (0.0 - 1.0 = 50ms - 800ms)
    SmoothParam m_head2;          // Head 2 level
    SmoothParam m_head3;          // Head 3 level  
    SmoothParam m_feedback;       // Feedback amount
    SmoothParam m_saturation;     // Tube saturation amount
    SmoothParam m_mix;            // Dry/wet mix
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // Multi-head delay system (like Binson Echorec)
    struct MagneticHead {
        static constexpr size_t MAX_BUFFER_SIZE = 88200; // 2 seconds at 44.1kHz
        std::array<float, MAX_BUFFER_SIZE> buffer;
        double writePos = 0.0;
        double readPos = 0.0;
        float headSpacing = 1.0f; // Relative position on drum
        
        // Magnetic tape characteristics
        float magnetization = 0.0f;
        float hysteresis = 0.0f;
        
        void reset() {
            buffer.fill(0.0f);
            writePos = 0.0;
            readPos = MAX_BUFFER_SIZE * 0.5;
            magnetization = 0.0f;
            hysteresis = 0.0f;
        }
        
        float read(double delaySamples);
        void write(float sample);
        float processMagnetic(float input);
    };
    
    // 4-head system (record + 3 playback heads)
    std::array<MagneticHead, 4> m_heads; // [0] = record, [1-3] = playback
    
    // Tube preamp stage
    struct TubeStage {
        float prevSample = 0.0f;
        float dcBlockerX1 = 0.0f;
        float dcBlockerY1 = 0.0f;
        
        // Tube transfer curves
        float processTriode(float input, float drive);
        float processPentode(float input, float drive);
        float dcBlock(float input);
    };
    
    std::array<TubeStage, 2> m_tubeStages;
    
    // Vintage filtering (motor speed variations affect frequency response)
    struct VintageFilter {
        // Variable lowpass (motor speed affects high frequencies)
        float lpX1 = 0.0f, lpX2 = 0.0f;
        float lpY1 = 0.0f, lpY2 = 0.0f;
        
        // Fixed highpass (AC coupling)
        float hpX1 = 0.0f;
        float hpY1 = 0.0f;
        
        // Motor speed wobble
        float wobblePhase = 0.0f;
        float wobbleRate = 0.13f; // Hz
        float wobbleDepth = 0.02f; // 2% speed variation
        
        void updateLowpass(float cutoffHz, double sampleRate);
        float processLowpass(float input);
        float processHighpass(float input, double sampleRate);
        float getSpeedModulation(double sampleRate);
    };
    
    std::array<VintageFilter, 2> m_filters;
    
    // Drum motor simulation
    struct DrumMotor {
        float currentSpeed = 1.0f;
        float targetSpeed = 1.0f;
        float inertia = 0.98f; // Motor inertia
        
        // Mechanical resonances
        float resonancePhase = 0.0f;
        float resonanceFreq = 0.7f; // Hz
        float resonanceAmount = 0.005f;
        
        void update() {
            currentSpeed = currentSpeed * inertia + targetSpeed * (1.0f - inertia);
        }
        
        float getSpeedVariation(double sampleRate);
    };
    
    DrumMotor m_motor;
    
    // Feedback path processing
    struct FeedbackProcessor {
        float prevSample = 0.0f;
        float saturation = 0.0f;
        
        float process(float input, float amount);
    };
    
    std::array<FeedbackProcessor, 2> m_feedbackProcessors;
    
    // Head configuration (spacing on drum in degrees)
    const std::array<float, 4> m_headPositions = {0.0f, 90.0f, 180.0f, 270.0f};
    
    // Helper functions
    float processDrumEcho(float input, int channel);
    void updateHeadDelays();
    float mixHeads(int channel);
};
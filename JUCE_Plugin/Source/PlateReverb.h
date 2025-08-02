#pragma once

#include "../Source/EngineBase.h"
#include <vector>
#include <array>
#include <memory>

class PlateReverb : public EngineBase {
public:
    PlateReverb();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Plate Reverb"; }
    int getNumParameters() const override { return 4; }
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
    
    SmoothParam m_size;      // Room size / decay time
    SmoothParam m_damping;   // High frequency damping
    SmoothParam m_predelay;  // Pre-delay time (0-100ms)
    SmoothParam m_mix;       // Dry/wet mix
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // Enhanced comb filter structure
    struct CombFilter {
        std::vector<float> buffer;
        int writePos = 0;
        float filterState = 0.0f;
        
        // Modulation for more organic sound
        float modPhase = 0.0f;
        float modRate = 0.0f;
        float modDepth = 0.0f;
        
        void init(int delaySamples, float modFreq, float modAmt) {
            buffer.resize(delaySamples);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
            filterState = 0.0f;
            modRate = modFreq;
            modDepth = modAmt;
            modPhase = static_cast<float>(rand()) / RAND_MAX; // Random phase
        }
        
        float process(float input, float feedback, float damping, double sampleRate);
    };
    
    // Enhanced allpass filter structure
    struct AllpassFilter {
        std::vector<float> buffer;
        int writePos = 0;
        
        void init(int delaySamples) {
            buffer.resize(delaySamples);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
        
        float process(float input, float feedback = 0.5f);
    };
    
    // Lattice allpass network for dense late reflections
    struct LatticeAllpass {
        AllpassFilter stage1, stage2;
        
        void init(int delay1, int delay2) {
            stage1.init(delay1);
            stage2.init(delay2);
        }
        
        float process(float input) {
            float mid = stage1.process(input, 0.5f);
            return stage2.process(mid, -0.5f);
        }
    };
    
    // Comb filter bank (12 for richer sound)
    static constexpr int NUM_COMBS = 12;
    std::array<CombFilter, NUM_COMBS> m_combs;
    
    // Allpass filter bank (6 for smoother diffusion)
    static constexpr int NUM_ALLPASS = 6;
    std::array<AllpassFilter, NUM_ALLPASS> m_allpasses;
    
    // Additional lattice network for density
    std::array<LatticeAllpass, 2> m_latticeNetwork;
    
    // Pre-delay with interpolation
    struct PreDelay {
        std::vector<float> buffer;
        float writePos = 0.0f;
        
        void init(int maxSamples) {
            buffer.resize(maxSamples);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0.0f;
        }
        
        float process(float input, float delaySamples);
    };
    
    PreDelay m_predelayLeft, m_predelayRight;
    
    // Input diffusion allpasses
    std::array<AllpassFilter, 4> m_inputDiffusion;
    
    // High-shelf filter for air
    struct HighShelf {
        float x1 = 0.0f, y1 = 0.0f;
        float a0 = 1.0f, a1 = 0.0f, b1 = 0.0f;
        
        void updateCoefficients(float freq, float gain, double sampleRate);
        float process(float input);
    };
    
    std::array<HighShelf, 2> m_highShelves;
    
    // DC blocker
    struct DCBlocker {
        float x1 = 0.0f, y1 = 0.0f;
        const float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
    };
    
    std::array<DCBlocker, 2> m_dcBlockers;
    
    // Plate reverb specific delay times (in samples at 44.1kHz)
    // Based on measurements from real plate reverbs
    const std::array<int, NUM_COMBS> m_combDelayTimes = {
        1116, 1188, 1277, 1356, 1422, 1491,
        1557, 1617, 1687, 1742, 1803, 1867
    };
    
    const std::array<int, NUM_ALLPASS> m_allpassDelayTimes = {
        225, 341, 441, 556, 667, 779
    };
    
    // Modulation rates for each comb filter (Hz)
    const std::array<float, NUM_COMBS> m_combModRates = {
        0.7f, 0.83f, 0.91f, 1.03f, 1.13f, 1.27f,
        1.39f, 1.51f, 1.63f, 1.79f, 1.93f, 2.11f
    };
    
    void initializeDelays();
    float processChannel(float input, int channel);
};
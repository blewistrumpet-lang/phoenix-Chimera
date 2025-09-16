#pragma once
#include "EngineBase.h"
#include "IPitchShiftStrategy.h"  // Use strategy pattern for flexibility
#include <memory>
#include <array>
#include <atomic>
#include <random>
#include <cmath>

/**
 * VOCAL DESTROYER - Focused 3-Mode Pitch/Formant Effect
 * 
 * Three distinct modes, each with 3 purposeful controls:
 * 
 * Mode 1: GENDER BENDER - Vocal character transformation
 * Mode 2: GLITCH MACHINE - Rhythmic stutters and freezes  
 * Mode 3: ALIEN TRANSFORM - Creative sound mangling
 * 
 * Design Philosophy:
 * - Every parameter position sounds musical
 * - Parameters work together, not against each other
 * - Clear purpose for each mode
 * - Streamlined workflow with only 4 active parameters at once
 */
class PitchShifter final : public EngineBase {
public:
    PitchShifter();
    ~PitchShifter() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 4; } // Mode + 3 controls
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Vocal Destroyer"; }
    
    // Parameter display for UI
    juce::String getParameterDisplayString(int index, float value) const;
    
    // Simplified parameter indices - only 4 parameters!
    enum ParamIndex {
        kMode = 0,     // Mode selector (Gender/Glitch/Alien)
        kControl1 = 1, // Changes based on mode
        kControl2 = 2, // Changes based on mode  
        kControl3 = 3  // Changes based on mode (always includes mix)
    };
    
    // Three focused modes
    enum Mode {
        MODE_GENDER = 0,   // Gender Bender mode
        MODE_GLITCH = 1,   // Glitch Machine mode
        MODE_ALIEN = 2     // Alien Transform mode
    };
    
private:
    // Core processing - using strategy pattern for pitch shifting
    std::array<std::unique_ptr<IPitchShiftStrategy>, 2> pitchShifters;  // One per channel
    
    // Current mode
    Mode currentMode = MODE_GENDER;
    
    // Universal processing state
    double sampleRate = 44100.0;
    int currentBlockSize = 512;
    
    // Parameter smoothing with musical curves
    struct SmoothedParam {
        float current = 0.0f;
        float target = 0.0f;
        float smoothing = 0.99f;
        
        void set(float value) { 
            target = value;
            if (std::abs(current - value) > 0.5f) {
                // Jump immediately for large changes (like mode switches)
                current = value;
            }
        }
        
        float tick() { 
            // Check for NaN/inf
            if (!std::isfinite(current)) current = 0.0f;
            if (!std::isfinite(target)) target = 0.0f;
            
            current += (target - current) * (1.0f - smoothing);
            return current;
        }
        
        void setSmoothingSpeed(float speed) {
            smoothing = 1.0f - juce::jlimit(0.0001f, 0.1f, speed);
        }
    };
    
    SmoothedParam modeParam;
    SmoothedParam control1Param;
    SmoothedParam control2Param;
    SmoothedParam control3Param;
    
    // --- MODE 1: GENDER BENDER ---
    struct GenderProcessor {
        float formantShift = 1.0f;
        float ageBlend = 0.5f;
        float intensity = 1.0f;
        
        void process(float& formantRatio, float& pitchRatio, float control1, float control2, float control3);
        float calculateCompensation(float formantRatio);
    };
    GenderProcessor genderProcessor;
    
    // --- MODE 2: GLITCH MACHINE ---
    struct GlitchProcessor {
        static constexpr int MAX_BUFFER_SIZE = 48000; // 1 second at 48kHz
        std::array<std::array<float, MAX_BUFFER_SIZE>, 2> buffers;
        int writePos = 0;
        int sliceSize = 4800; // 100ms default
        int currentSlice = 0;
        bool frozen = false;
        float scatter = 0.0f;
        
        // Crossfade for smooth transitions
        float crossfadePos = 1.0f;
        std::array<float, 512> crossfadeBuffer;
        
        void updateSliceSize(float control1, double sampleRate);
        float process(float input, int channel, float scatter, bool freeze);
        void reset();
    };
    GlitchProcessor glitchProcessor;
    
    // --- MODE 3: ALIEN TRANSFORM ---
    struct AlienProcessor {
        float species = 0.0f; // Morphs through different alien types
        float evolution = 0.0f; // Auto-modulation depth/speed
        float dimension = 0.0f; // Spatial warping/feedback
        
        // Internal modulation
        float lfoPhase = 0.0f;
        float lfoRate = 2.0f;
        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<float> dist{-1.0f, 1.0f};
        
        // Feedback spiral buffer
        std::array<std::array<float, 4800>, 2> spiralBuffers;
        std::array<int, 2> spiralPos{0, 0};
        float spiralFeedback = 0.0f;
        float pitchAccumulation = 1.0f;
        
        void process(float& formantRatio, float& pitchRatio, 
                    float control1, float control2, float control3,
                    double sampleRate);
        float processSpiral(float input, int channel);
    };
    AlienProcessor alienProcessor;
    
    // Transient detection for smarter processing
    struct TransientDetector {
        float envelope = 0.0f;
        float attackTime = 0.001f;
        float releaseTime = 0.1f;
        
        bool detectTransient(float input);
    };
    std::array<TransientDetector, 2> transientDetectors;
    
    // Helper methods
    float semitonesToRatio(float semitones) const {
        return std::pow(2.0f, semitones / 12.0f);
    }
    
    Mode getCurrentMode(float normalized) const {
        int modeInt = static_cast<int>(normalized * 2.99f); // 0, 1, or 2
        return static_cast<Mode>(juce::jlimit(0, 2, modeInt));
    }
    
    // Musical curve shaping
    enum CurveType { LINEAR, EXPONENTIAL, LOGARITHMIC, S_CURVE };
    float applyMusicalCurve(float input, CurveType type) const;
    
    // Get parameter names based on current mode
    juce::String getModeParameterName(int paramIndex) const;
    juce::String getModeParameterDisplay(int paramIndex, float value) const;
};
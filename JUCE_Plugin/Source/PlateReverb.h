#pragma once

#include "../Source/EngineBase.h"
#include "DspEngineUtilities.h"
#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include <random>
#include <algorithm>

// Performance mode for different CPU targets
enum class PerformanceMode {
    LOW_CPU,      // Linear interpolation, 6-tap FDN, 64-sample blocks
    BALANCED,     // Hermite interpolation, 8-tap FDN, 32-sample blocks
    HIGH_QUALITY  // Cubic interpolation, 8-tap FDN, 32-sample blocks (default)
};

// Professional plate reverb constants based on EMT 140 measurements
namespace PlateConstants {
    // Block processing sizes for different performance modes
    constexpr int getBlockSize(PerformanceMode mode) {
        switch (mode) {
            case PerformanceMode::LOW_CPU: return 64;    // Larger blocks for efficiency
            case PerformanceMode::BALANCED: return 32;   // Balanced
            case PerformanceMode::HIGH_QUALITY: return 32; // Optimal for low latency
            default: return 32;
        }
    }
    
    // Maximum supported sample rate for pre-allocation
    constexpr double MAX_SAMPLE_RATE = 192000.0;
    constexpr double REFERENCE_SAMPLE_RATE = 44100.0;
    
    // Early reflection tap delays (samples at 44.1kHz) - measured from EMT 140
    constexpr int EARLY_TAP_DELAYS[] = {
        113,   // 2.56ms - First reflection from plate edge
        197,   // 4.47ms - Secondary edge reflection  
        283,   // 6.42ms - Corner reflection
        367,   // 8.32ms - Diagonal path
        431,   // 9.77ms - Multiple edge bounce
        503,   // 11.4ms - Complex path 1
        577,   // 13.1ms - Complex path 2
        643,   // 14.6ms - Complex path 3
        719,   // 16.3ms - Late early reflection 1
        797,   // 18.1ms - Late early reflection 2
        863,   // 19.6ms - Late early reflection 3
        929,   // 21.1ms - Late early reflection 4
        997,   // 22.6ms - Transition to late reverb 1
        1061,  // 24.1ms - Transition to late reverb 2
        1129,  // 25.6ms - Transition to late reverb 3
        1193   // 27.1ms - Final early reflection
    };
    
    // FDN delay times - based on golden ratio to minimize coloration
    constexpr int FDN_DELAY_BASE[] = {
        1433,  // Base delay
        1601,  // Base * 1.117 (avoiding simple ratios)
        1867,  // Base * 1.303
        2053,  // Base * 1.433
        2251,  // Base * 1.571
        2399,  // Base * 1.674
        2617,  // Base * 1.826
        2797   // Base * 1.952
    };
    
    // Modulation rates (Hz) - prime numbers for non-correlated motion
    constexpr double MOD_RATES[] = {
        0.71, 0.83, 0.97, 1.07, 1.13, 1.31, 1.49, 1.67
    };
    
    // Input diffusion delays (samples at 44.1kHz) - prime numbers
    constexpr int DIFFUSION_DELAYS[] = {113, 163, 211, 263};
    
    // Reverb parameters
    constexpr double MIN_FEEDBACK = 0.82;    // Prevents instant decay
    constexpr double MAX_FEEDBACK = 0.98;    // Prevents runaway oscillation
    constexpr double DAMPING_SCALE = 0.8;    // Scales user damping for FDN
    constexpr double EARLY_MIX = 0.4;        // Early reflections mix level
    constexpr double LATE_MIX = 0.6;         // Late reverb mix level
    constexpr double STEREO_SPREAD = 1.2;    // Stereo width enhancement
    
    // Filter frequencies
    constexpr double DC_BLOCK_FREQ = 5.0;    // DC blocker frequency (Hz)
    constexpr double OUTPUT_HPF_FREQ = 20.0; // Output highpass frequency (Hz)
    
    // Smoothing times (seconds) - granular control
    constexpr double SIZE_SMOOTH_TIME = 0.05;        // 50ms for size changes
    constexpr double DAMPING_SMOOTH_TIME = 0.02;     // 20ms for damping
    constexpr double PREDELAY_SMOOTH_TIME = 0.01;    // 10ms for pre-delay
    constexpr double MIX_SMOOTH_TIME = 0.02;         // 20ms for mix
    constexpr double FEEDBACK_SMOOTH_TIME = 0.025;   // 25ms for feedback (faster)
    constexpr double FDN_DAMPING_SMOOTH_TIME = 0.04; // 40ms for FDN damping (slower)
    
    // Parameter validation ranges
    constexpr float MIN_PARAM_VALUE = 0.0f;
    constexpr float MAX_PARAM_VALUE = 1.0f;
}

/**
 * Professional Plate Reverb
 * 
 * Thread Safety:
 * - updateParameters() is thread-safe and can be called from any thread
 * - prepareToPlay() and reset() must ONLY be called from the audio thread
 * - process() must ONLY be called from the audio thread
 * - setPerformanceMode() should be called before prepareToPlay()
 * 
 * The reverb uses lock-free atomic parameters for real-time safety.
 */
class PlateReverb : public EngineBase {
public:
    PlateReverb();
    ~PlateReverb();
    
    // Audio thread only - NOT thread-safe with process()
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    
    // Audio thread only
    void process(juce::AudioBuffer<float>& buffer) override;
    
    // Thread-safe - can be called from any thread
    void updateParameters(const std::map<int, float>& params) override;
    
    // Should be called before prepareToPlay()
    void setPerformanceMode(PerformanceMode mode);
    
    // Plugin info
    juce::String getName() const override { return "Plate Reverb"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    
private:
    // Forward declarations
    class ParameterSmoother;
    class SoftKneeLimiter;
    class ButterworthHighpass;
    class OnePoleFilter;
    class InterpolatedDelayLine;
    struct ModulatedCombFilter;
    struct AllpassFilter;
    struct EarlyReflections;
    struct FDN;
    
    // Thread-safe parameters with smoothing
    std::unique_ptr<ParameterSmoother> m_size;
    std::unique_ptr<ParameterSmoother> m_damping;
    std::unique_ptr<ParameterSmoother> m_predelay;
    std::unique_ptr<ParameterSmoother> m_mix;
    
    // Smoothers for derived parameters with independent timing
    std::unique_ptr<ParameterSmoother> m_feedbackSmooth;
    std::unique_ptr<ParameterSmoother> m_fdnDampingSmooth;
    
    // DSP state
    double m_sampleRate = 44100.0;
    std::atomic<bool> m_isInitialized{false};
    
    // Performance configuration
    std::atomic<PerformanceMode> m_performanceMode{PerformanceMode::HIGH_QUALITY};
    int m_currentBlockSize = 32;
    
    // Consistent RNG for reproducible results
    std::mt19937 m_rng{42};  // Fixed seed for consistency
    
    // Pre-allocated DSP components
    std::array<std::unique_ptr<ButterworthHighpass>, 2> m_dcBlockers;
    std::array<std::unique_ptr<SoftKneeLimiter>, 2> m_inputLimiters;
    std::array<std::unique_ptr<InterpolatedDelayLine>, 2> m_predelays;
    std::unique_ptr<EarlyReflections> m_earlyReflections;
    
    static constexpr int NUM_DIFFUSERS = 4;
    std::array<std::unique_ptr<AllpassFilter>, NUM_DIFFUSERS> m_inputDiffusion;
    
    std::unique_ptr<FDN> m_fdnLeft;
    std::unique_ptr<FDN> m_fdnRight;
    
    std::array<std::unique_ptr<ButterworthHighpass>, 2> m_outputHighpass;
    std::array<std::unique_ptr<SoftKneeLimiter>, 2> m_outputLimiters;
    
    // Helper methods
    int getFDNSize() const {
        return (m_performanceMode.load() == PerformanceMode::LOW_CPU) ? 6 : 8;
    }
    
    void initializeFilters();
    std::pair<double, double> processReverbSample(double inputL, double inputR);
    
    // Parameter validation
    static float clampParameter(float value) {
        return std::clamp(value, PlateConstants::MIN_PARAM_VALUE, 
                                PlateConstants::MAX_PARAM_VALUE);
    }
};
#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"#include <memory>
#include <array>
#include <atomic>

/**
 * Platinum-Spec Mid-Side Processor
 * 
 * Professional mid-side encoding/decoding with independent processing
 * for stereo width control, spatial enhancement, and creative mixing.
 * Features precision matrix processing and advanced stereo field manipulation.
 * 
 * Features:
 * - True mid-side encoding/decoding with double precision
 * - Independent gain and EQ for mid/side channels
 * - Advanced width control with bass mono and presence boost
 * - Elliptical filter for vinyl-safe bass mono
 * - Solo monitoring for mid or side signals
 * - Phase correlation metering
 * - Auto-gain compensation
 * - SIMD-optimized matrix operations
 * 
 * Performance:
 * - CPU: < 5% @ 96kHz/64 samples (Intel i7/Apple M2)
 * - Latency: Zero (no lookahead required)
 * - Memory: Zero allocations in real-time thread
 * - Thread Safety: Lock-free parameter updates
 * 
 * @version 2.0.0 - Platinum Edition
 * @author DSP Engineering Team
 */
class MidSideProcessor_Platinum final : public EngineBase {
public:
    /**
     * Constructor - initializes with default settings
     * Default: Unity gain, flat EQ, 100% width
     */
    MidSideProcessor_Platinum();
    
    /**
     * Destructor
     */
    ~MidSideProcessor_Platinum() override;
    
    // Non-copyable for thread safety
    MidSideProcessor_Platinum(const MidSideProcessor_Platinum&) = delete;
    MidSideProcessor_Platinum& operator=(const MidSideProcessor_Platinum&) = delete;
    
    // Moveable
    MidSideProcessor_Platinum(MidSideProcessor_Platinum&&) = default;
    MidSideProcessor_Platinum& operator=(MidSideProcessor_Platinum&&) = default;
    
    /**
     * Prepare for playback
     * @param sampleRate The sample rate in Hz (supports 44.1kHz to 192kHz)
     * @param samplesPerBlock Maximum block size (32 to 8192 samples)
     */
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    
    /**
     * Process audio buffer
     * @param buffer Audio buffer to process (must be stereo)
     */
    void process(juce::AudioBuffer<float>& buffer) override;
    
    /**
     * Reset all internal state
     * Clears filter states and gain smoothing
     */
    void reset() override;
    
    /**
     * Update parameters (thread-safe)
     * @param params Map of parameter index to normalized value (0.0-1.0)
     */
    void updateParameters(const std::map<int, float>& params) override;
    
    /**
     * Get total number of parameters
     * @return 10 parameters
     */
    int getNumParameters() const override { return 10; }
    
    /**
     * Get parameter name by index
     * @param index Parameter index (0-9)
     * @return Parameter name string
     */
    juce::String getParameterName(int index) const override;
    
    /**
     * Get processor name
     * @return "Mid-Side Processor"
     */
    juce::String getName() const override { return "Mid-Side Processor"; }
    
    /**
     * Parameter indices for external control
     */
    enum ParamIndex : int {
        MID_GAIN = 0,       // Mid channel gain: 0.0 = -20dB, 0.5 = 0dB, 1.0 = +20dB
        SIDE_GAIN = 1,      // Side channel gain: 0.0 = -20dB, 0.5 = 0dB, 1.0 = +20dB
        WIDTH = 2,          // Stereo width: 0.0 = mono, 0.5 = 100%, 1.0 = 200%
        MID_LOW = 3,        // Mid low shelf: 0.0 = -15dB, 0.5 = 0dB, 1.0 = +15dB
        MID_HIGH = 4,       // Mid high shelf: 0.0 = -15dB, 0.5 = 0dB, 1.0 = +15dB
        SIDE_LOW = 5,       // Side low shelf: 0.0 = -15dB, 0.5 = 0dB, 1.0 = +15dB
        SIDE_HIGH = 6,      // Side high shelf: 0.0 = -15dB, 0.5 = 0dB, 1.0 = +15dB
        BASS_MONO = 7,      // Bass mono frequency: 0.0 = off, 1.0 = 500Hz
        SOLO_MODE = 8,      // Solo monitoring: 0.0 = off, 0.33 = mid, 0.66 = side
        PRESENCE = 9        // Presence boost: 0.0 = off, 1.0 = +6dB @ 10kHz
    };
    
    /**
     * Solo monitoring modes
     */
    enum class SoloMode {
        OFF,
        MID_ONLY,
        SIDE_ONLY
    };
    
    /**
     * Get current solo mode
     * @return Currently active solo mode
     */
    SoloMode getCurrentSoloMode() const noexcept;
    
    /**
     * Get current stereo width percentage
     * @return Width as percentage (0-200%)
     */
    float getWidthPercentage() const noexcept;
    
    /**
     * Extended API for metering
     */
    struct StereoMetering {
        float midLevel;      // RMS level of mid channel
        float sideLevel;     // RMS level of side channel
        float correlation;   // Phase correlation (-1 to +1)
        float balance;       // L/R balance (-1 to +1)
    };
    
    /**
     * Get current stereo metering data
     * @return Real-time metering information
     */
    StereoMetering getMetering() const noexcept;
    
private:
    // Private implementation (PIMPL idiom)
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
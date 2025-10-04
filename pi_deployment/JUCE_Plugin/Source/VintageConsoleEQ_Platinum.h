#pragma once
#include "EngineBase.h"
#include <memory>
#include <array>
#include <atomic>

/**
 * Platinum-Spec Vintage Console EQ
 * 
 * Professional-grade 3-band equalizer with authentic console emulation.
 * Models the sonic characteristics of classic mixing consoles including
 * Neve 1073, API 550, SSL 4000, and Pultec EQP-1A.
 * 
 * Features:
 * - SIMD-optimized biquad filter processing (4x throughput)
 * - 4 classic console emulation modes with authentic saturation
 * - Low shelf: 30-300Hz ±15dB
 * - Mid bell: 200Hz-8kHz ±15dB with proportional Q (0.3-3.0)
 * - High shelf: 3kHz-16kHz ±15dB
 * - Zero-copy coefficient interpolation for smooth parameter changes
 * - Guaranteed denormal-free operation
 * - Thread-safe lock-free parameter updates
 * - DC-coupled signal path with optional blocking
 * 
 * Performance:
 * - CPU: < 10% @ 96kHz/64 samples (Intel i7/Apple M2)
 * - Latency: Zero (no lookahead required)
 * - Memory: Zero allocations in real-time thread
 * - Thread Safety: Lock-free parameter updates
 * 
 * @version 2.0.0 - Platinum Edition
 * @author DSP Engineering Team
 */
class VintageConsoleEQ_Platinum final : public EngineBase {
public:
    /**
     * Constructor - initializes with default settings
     * Default: Flat response, Neve 1073 mode, 30% drive
     */
    VintageConsoleEQ_Platinum();
    
    /**
     * Destructor
     */
    ~VintageConsoleEQ_Platinum() override;
    
    // Non-copyable for thread safety
    VintageConsoleEQ_Platinum(const VintageConsoleEQ_Platinum&) = delete;
    VintageConsoleEQ_Platinum& operator=(const VintageConsoleEQ_Platinum&) = delete;
    
    // Moveable
    VintageConsoleEQ_Platinum(VintageConsoleEQ_Platinum&&) = default;
    VintageConsoleEQ_Platinum& operator=(VintageConsoleEQ_Platinum&&) = default;
    
    /**
     * Prepare for playback
     * @param sampleRate The sample rate in Hz (supports 44.1kHz to 192kHz)
     * @param samplesPerBlock Maximum block size (32 to 8192 samples)
     */
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    
    /**
     * Process audio buffer
     * @param buffer Audio buffer to process (mono or stereo)
     */
    void process(juce::AudioBuffer<float>& buffer) override;
    
    /**
     * Reset all internal state
     * Clears filter states, DC blockers, and saturation history
     */
    void reset() override;
    
    /**
     * Update parameters (thread-safe)
     * @param params Map of parameter index to normalized value (0.0-1.0)
     */
    void updateParameters(const std::map<int, float>& params) override;
    
    /**
     * Get total number of parameters
     * @return 11 parameters
     */
    int getNumParameters() const override { return 11; }
    
    /**
     * Get parameter name by index
     * @param index Parameter index (0-10)
     * @return Parameter name string
     */
    juce::String getParameterName(int index) const override;
    
    /**
     * Get processor name
     * @return "Vintage Console EQ"
     */
    juce::String getName() const override { return "Vintage Console EQ"; }
    
    /**
     * Parameter indices for external control
     */
    enum ParamIndex : int {
        LOW_GAIN = 0,      // Low shelf gain: 0.0 = -15dB, 0.5 = 0dB, 1.0 = +15dB
        LOW_FREQ = 1,      // Low shelf frequency: 0.0 = 30Hz, 1.0 = 300Hz
        MID_GAIN = 2,      // Mid bell gain: 0.0 = -15dB, 0.5 = 0dB, 1.0 = +15dB
        MID_FREQ = 3,      // Mid frequency: 0.0 = 200Hz, 1.0 = 8kHz (logarithmic)
        MID_Q = 4,         // Mid Q width: 0.0 = 0.3 (wide), 1.0 = 3.0 (narrow)
        HIGH_GAIN = 5,     // High shelf gain: 0.0 = -15dB, 0.5 = 0dB, 1.0 = +15dB
        HIGH_FREQ = 6,     // High shelf frequency: 0.0 = 3kHz, 1.0 = 16kHz
        DRIVE = 7,         // Console saturation: 0.0 = clean, 1.0 = maximum color
        CONSOLE_TYPE = 8,  // Console model: 0.0-0.25 = SSL, 0.25-0.5 = API, 
                          //                0.5-0.75 = Neve, 0.75-1.0 = Pultec
        VINTAGE = 9,       // Vintage character: 0.0 = modern, 1.0 = vintage
        MIX = 10          // Dry/wet mix: 0.0 = dry, 1.0 = wet
    };
    
    /**
     * Console emulation types
     */
    enum class ConsoleModel {
        SSL_4000,    // Clean, surgical, minimal coloration
        API_550,     // Punchy, musical, proportional Q
        NEVE_1073,   // Warm, transformer-coupled, musical saturation
        PULTEC       // Smooth, passive curves, gentle saturation
    };
    
    /**
     * Get current console model
     * @return Currently active console emulation type
     */
    ConsoleModel getCurrentConsoleModel() const noexcept;
    
    /**
     * Get current CPU load
     * @return CPU usage percentage (0-100%)
     */
    float getCPULoad() const noexcept;
    
    /**
     * Extended API for analysis
     */
    struct FilterResponse {
        float magnitude;  // Linear magnitude response
        float phase;      // Phase response in radians
    };
    
    /**
     * Calculate frequency response at given frequency
     * @param frequency Test frequency in Hz
     * @return Magnitude and phase response
     */
    FilterResponse getFrequencyResponse(float frequency) const noexcept;
    
private:
    // Private implementation (PIMPL idiom)
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
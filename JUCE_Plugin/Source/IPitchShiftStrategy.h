#pragma once
#include <memory>
#include <string>

/**
 * IPitchShiftStrategy - Abstract interface for pitch shifting algorithms
 * 
 * This interface allows us to swap implementations without changing engine code.
 * Beta release uses SimplePitchShift, future releases can add better algorithms.
 * 
 * Design Principles:
 * - Clean interface for any pitch shifting algorithm
 * - Report latency for future compensation
 * - Self-documenting quality levels
 * - Easy to extend with new implementations
 */
class IPitchShiftStrategy {
public:
    virtual ~IPitchShiftStrategy() = default;
    
    /**
     * Prepare the pitch shifter for processing
     * @param sampleRate The sample rate in Hz
     * @param maxBlockSize Maximum block size that will be processed
     */
    virtual void prepare(double sampleRate, int maxBlockSize) = 0;
    
    /**
     * Reset internal state (clear buffers, etc)
     */
    virtual void reset() = 0;
    
    /**
     * Process audio with pitch shifting
     * @param input Input samples
     * @param output Output samples (can be same as input for in-place)
     * @param numSamples Number of samples to process
     * @param pitchRatio Pitch ratio (1.0 = no change, 2.0 = octave up, 0.5 = octave down)
     */
    virtual void process(const float* input, float* output, int numSamples, float pitchRatio) = 0;
    
    /**
     * Get the latency in samples introduced by this algorithm
     * @return Latency in samples (0 for zero-latency algorithms)
     */
    virtual int getLatencySamples() const = 0;
    
    /**
     * Get the name of this implementation
     * @return Human-readable name like "Simple (Beta)" or "High Quality"
     */
    virtual const char* getName() const = 0;
    
    /**
     * Check if this is a high-quality implementation
     * @return true for production quality, false for beta/simple implementations
     */
    virtual bool isHighQuality() const = 0;
    
    /**
     * Get quality rating (0-100)
     * @return Quality rating where 100 is studio quality, <50 is beta
     */
    virtual int getQualityRating() const = 0;
    
    /**
     * Get CPU usage estimate (0-100)
     * @return Approximate CPU usage where 100 is very heavy
     */
    virtual int getCpuUsage() const = 0;
};

/**
 * Factory for creating pitch shift implementations
 */
class PitchShiftFactory {
public:
    enum class Algorithm {
        Simple,         // Beta quality, zero latency
        Signalsmith,    // High latency, not working yet
        PSOLA,          // Medium quality, low latency (future)
        PhaseVocoder,   // High quality, medium latency (future)
        RubberBand      // Professional quality (future)
    };
    
    /**
     * Create a pitch shifter with the specified algorithm
     * Falls back to Simple if requested algorithm unavailable
     */
    static std::unique_ptr<IPitchShiftStrategy> create(Algorithm algo = Algorithm::Simple);
    
    /**
     * Get the best available algorithm
     * For beta, this returns Simple
     */
    static Algorithm getBestAvailable();
    
    /**
     * Check if an algorithm is available
     */
    static bool isAvailable(Algorithm algo);
};

// Convenience typedef
using PitchShiftPtr = std::unique_ptr<IPitchShiftStrategy>;
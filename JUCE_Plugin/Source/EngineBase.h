#pragma once

#include <JuceHeader.h>
#include <map>

class EngineBase {
public:
    virtual ~EngineBase() = default;
    
    // ========== Existing Core API ==========
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;  // Clear all internal state
    virtual void updateParameters(const std::map<int, float>& params) = 0;
    virtual juce::String getName() const = 0;
    virtual int getNumParameters() const = 0;
    virtual juce::String getParameterName(int index) const = 0;

    // ========== Extended API (with safe defaults) ==========
    
    // Report true latency so hosts can compensate correctly (PDC)
    // Override this for lookahead limiters, FFT/OLA processors, linear-phase filters, etc.
    virtual int getLatencySamples() const noexcept { return 0; }
    
    // DAWs may change block size at runtime; this hint lets engines pre-allocate safely
    // Called before prepareToPlay() and whenever max block size changes
    virtual void setMaxBlockSizeHint(int maxBlockSize) { 
        juce::ignoreUnused(maxBlockSize); 
    }
    
    // Channel/layout awareness (default: handle inside prepareToPlay)
    // Useful for engines that need different processing for mono/stereo/surround
    virtual void setNumChannels(int numIn, int numOut) { 
        juce::ignoreUnused(numIn, numOut); 
    }
    
    // Transport info for tempo-sync'd effects (delays, chorus, tremolo, etc.)
    struct TransportInfo {
        double bpm = 120.0;
        double timeSigNumerator = 4.0;
        double timeSigDenominator = 4.0;
        double ppqPosition = 0.0;        // Quarter note position
        bool isPlaying = false;
        bool isRecording = false;
        bool isLooping = false;
        double loopStartPpq = 0.0;
        double loopEndPpq = 0.0;
    };
    virtual void setTransportInfo(const TransportInfo& t) { 
        juce::ignoreUnused(t); 
    }
    
    // Hard bypass that engines may honor internally for zero-CPU or clickless ramps
    // When bypassed, engines should either pass audio through unchanged or fade to silence
    virtual void setBypassed(bool shouldBypass) { 
        juce::ignoreUnused(shouldBypass); 
    }
    
    // Processing precision hint (for future double-precision support)
    enum class Precision { 
        Single,  // 32-bit float processing
        Double   // 64-bit double processing
    };
    virtual void setProcessingPrecision(Precision p) { 
        juce::ignoreUnused(p); 
    }
    
    // Quality/CPU tradeoff setting
    enum class Quality {
        Draft,      // Lowest CPU, suitable for live/tracking
        Normal,     // Balanced quality/CPU
        High,       // High quality, more CPU
        Ultra       // Maximum quality, highest CPU
    };
    virtual void setQuality(Quality q) {
        juce::ignoreUnused(q);
    }
    
    // Sidechain input support (for compressors, gates, vocoders, etc.)
    virtual void processSidechain(juce::AudioBuffer<float>& mainBuffer, 
                                 const juce::AudioBuffer<float>& sidechainBuffer) {
        juce::ignoreUnused(sidechainBuffer);
        process(mainBuffer);  // Default: ignore sidechain, process normally
    }
    
    // Get current CPU usage estimate (0.0 to 1.0)
    virtual float getCpuUsage() const noexcept { return 0.0f; }
    
    // Check if engine supports a specific feature
    enum class Feature {
        Sidechain,
        TempoSync,
        Oversampling,
        LatencyCompensation,
        Bypass,
        DoublePrecision
    };
    virtual bool supportsFeature(Feature f) const noexcept { 
        juce::ignoreUnused(f);
        return false; 
    }
};
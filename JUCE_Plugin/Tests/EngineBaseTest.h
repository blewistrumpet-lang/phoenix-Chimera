#pragma once

#include "JuceHeaderTest.h"
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
    
    // MIDI input for engines that need it (filters with keytrack, pitch shifters, etc.)
    virtual void processMidi(const juce::MidiBuffer& midiIn) { 
        juce::ignoreUnused(midiIn); 
    }
    
    // Engine state save/restore (for DAW session recall)
    // Return empty MemoryBlock if not implemented
    virtual juce::MemoryBlock getState() const { 
        return juce::MemoryBlock(); 
    }
    virtual void setState(const juce::MemoryBlock& data) { 
        juce::ignoreUnused(data); 
    }
    
    // Performance monitoring (optional - for plugin hosts to track CPU usage)
    virtual double getCpuUsage() const { return 0.0; }
    
    // Quality metrics (optional - for A/B testing, QA validation)
    struct QualityMetrics {
        float thd_percent = 0.0f;         // Total Harmonic Distortion
        float noise_floor_db = -120.0f;   // Noise floor level
        float dynamic_range_db = 120.0f;  // Dynamic range
        float latency_samples = 0.0f;     // Actual processing latency
    };
    virtual QualityMetrics getQualityMetrics() const { 
        return QualityMetrics{}; 
    }
    
    // Engine information for documentation/debugging
    struct EngineInfo {
        juce::String version = "1.0";
        juce::String author = "Chimera Audio";
        juce::String description = "Audio Engine";
        juce::String category = "Effect";
        bool isSynth = false;
        bool acceptsMidi = false;
        bool producesMidi = false;
        int numPrograms = 0;
    };
    virtual EngineInfo getEngineInfo() const {
        EngineInfo info;
        info.description = getName();
        return info;
    }
};
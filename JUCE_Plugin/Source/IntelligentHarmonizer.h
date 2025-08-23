#pragma once
#include "EngineBase.h"
#include <memory>

class IntelligentHarmonizer : public EngineBase {
public:
    IntelligentHarmonizer();
    ~IntelligentHarmonizer() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    void snapParameters(const std::map<int, float>& params);
    
    int getNumParameters() const override { return 15; }  // Using all 15 parameters
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Intelligent Harmonizer"; }
    
    // Get parameter display string for UI
    juce::String getParameterDisplayString(int index, float normalizedValue) const;
    
    // Get total processing latency in samples
    int getLatencySamples() const noexcept override;
    
    // Parameter indices (15 total)
    enum ParamID {
        kVoices = 0,        // Number of voices (1-3)
        kChordType = 1,     // Chord preset selection
        kRootKey = 2,       // Root key (C-B)
        kScale = 3,         // Scale type
        kMasterMix = 4,     // Overall dry/wet
        kVoice1Volume = 5,  // Voice 1 volume
        kVoice1Formant = 6, // Voice 1 formant
        kVoice2Volume = 7,  // Voice 2 volume
        kVoice2Formant = 8, // Voice 2 formant
        kVoice3Volume = 9,  // Voice 3 volume
        kVoice3Formant = 10,// Voice 3 formant
        kQuality = 11,      // Low latency vs high quality
        kHumanize = 12,     // Humanization amount
        kWidth = 13,        // Stereo width
        kTranspose = 14     // Global transpose
    };
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};
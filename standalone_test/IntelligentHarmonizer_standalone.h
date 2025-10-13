#pragma once
#include <memory>
#include <map>

// Forward declaration
namespace juce {
template<typename T> class AudioBuffer;
class String;
}

class IntelligentHarmonizer_Standalone {
public:
    IntelligentHarmonizer_Standalone();
    ~IntelligentHarmonizer_Standalone();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(const float* input, float* output, int numSamples);
    void reset();
    void updateParameters(const std::map<int, float>& params);
    void snapParameters(const std::map<int, float>& params);
    int getLatencySamples() const noexcept;

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

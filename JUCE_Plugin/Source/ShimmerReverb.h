// ShimmerReverb.h - Platinum-spec header (minimal interface)
#pragma once
#include "EngineBase.h"
#include <memory>
#include <array>

// Forward declarations
namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

class ShimmerReverb final : public EngineBase {
public:
    ShimmerReverb();
    ~ShimmerReverb() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Shimmer Reverb"; }
    
    // Parameter IDs
    enum class ParamID : int {
        Size = 0,
        Shimmer = 1,
        Pitch = 2,
        Damping = 3,
        Diffusion = 4,
        Modulation = 5,
        Predelay = 6,
        Width = 7,
        Freeze = 8,
        Mix = 9
    };
    
    // Quality modes
    enum class QualityMode {
        Draft,      // No oversampling, basic algorithms
        Standard,   // 2x oversampling, good algorithms  
        Premium,    // 4x oversampling, best algorithms
        Platinum    // 8x oversampling, all features
    };
    
    void setQualityMode(QualityMode mode);
    void setReverbType(int type); // 0=Hall, 1=Plate, 2=Spring, 3=Shimmer
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
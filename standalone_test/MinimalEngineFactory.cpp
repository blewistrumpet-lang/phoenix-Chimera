#include "MinimalEngineFactory.h"
#include "../JUCE_Plugin/Source/VintageOptoCompressor_Platinum.h"
#include "../JUCE_Plugin/Source/ClassicCompressor.h"
#include "../JUCE_Plugin/Source/TransientShaper_Platinum.h"
#include "../JUCE_Plugin/Source/NoiseGate_Platinum.h"
#include "../JUCE_Plugin/Source/MasteringLimiter_Platinum.h"

// Simple NoneEngine implementation
class SimpleNoneEngine : public EngineBase {
public:
    SimpleNoneEngine() {}
    ~SimpleNoneEngine() override {}

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    void process(juce::AudioBuffer<float>& buffer) override {}
    void reset() override {}
    void updateParameters(const std::map<int, float>& params) override {}
    int getNumParameters() const override { return 0; }
    juce::String getName() const override { return "None Engine"; }
    juce::String getParameterName(int index) const override { return ""; }
};

std::unique_ptr<EngineBase> MinimalEngineFactory::createEngine(int engineID) {
    switch (engineID) {
        case 0:
            return std::make_unique<SimpleNoneEngine>();
        case 1:
            return std::make_unique<VintageOptoCompressor_Platinum>();
        case 2:
            return std::make_unique<ClassicCompressor>();
        case 3:
            return std::make_unique<TransientShaper_Platinum>();
        case 4:
            return std::make_unique<NoiseGate_Platinum>();
        case 5:
            return std::make_unique<MasteringLimiter_Platinum>();
        default:
            return nullptr;
    }
}

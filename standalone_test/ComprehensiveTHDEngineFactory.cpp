#include "ComprehensiveTHDEngineFactory.h"

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
    juce::String getName() const override { return "None"; }
    juce::String getParameterName(int index) const override { return ""; }
};

// Include only the engines we're testing
#include "../JUCE_Plugin/Source/OptoCompressor.h"
#include "../JUCE_Plugin/Source/VCACompressor.h"
#include "../JUCE_Plugin/Source/TransientShaper.h"
#include "../JUCE_Plugin/Source/NoiseGate.h"
#include "../JUCE_Plugin/Source/MasteringLimiter.h"
#include "../JUCE_Plugin/Source/DynamicEQ.h"

#include "../JUCE_Plugin/Source/ParametricEQ.h"
#include "../JUCE_Plugin/Source/VintageConsoleEQ.h"
#include "../JUCE_Plugin/Source/LadderFilter.h"
#include "../JUCE_Plugin/Source/StateVariableFilter.h"
#include "../JUCE_Plugin/Source/FormantFilter.h"
#include "../JUCE_Plugin/Source/EnvelopeFilter.h"
#include "../JUCE_Plugin/Source/CombResonator.h"
#include "../JUCE_Plugin/Source/VocalFormant.h"

#include "../JUCE_Plugin/Source/ResonantChorus.h"
#include "../JUCE_Plugin/Source/AnalogPhaser.h"
#include "../JUCE_Plugin/Source/RingModulator.h"
#include "../JUCE_Plugin/Source/FrequencyShifter.h"
#include "../JUCE_Plugin/Source/HarmonicTremolo.h"
#include "../JUCE_Plugin/Source/ClassicTremolo.h"
#include "../JUCE_Plugin/Source/RotarySpeaker.h"
#include "../JUCE_Plugin/Source/PitchShifter.h"
#include "../JUCE_Plugin/Source/DetuneDoubler.h"

#include "../JUCE_Plugin/Source/TapeEcho.h"
#include "../JUCE_Plugin/Source/DigitalDelay.h"
#include "../JUCE_Plugin/Source/MagneticDrumEcho.h"
#include "../JUCE_Plugin/Source/BucketBrigadeDelay.h"
#include "../JUCE_Plugin/Source/BufferRepeat.h"

#include "../JUCE_Plugin/Source/ShimmerReverb.h"
#include "../JUCE_Plugin/Source/GatedReverb.h"

#include "../JUCE_Plugin/Source/DimensionExpander.h"
#include "../JUCE_Plugin/Source/SpectralFreeze.h"
#include "../JUCE_Plugin/Source/SpectralGate.h"

#include "../JUCE_Plugin/Source/GranularCloud.h"
#include "../JUCE_Plugin/Source/ChaosGenerator.h"
#include "../JUCE_Plugin/Source/FeedbackNetwork.h"

std::unique_ptr<EngineBase> ComprehensiveTHDEngineFactory::createEngine(int engineID) {
    switch (engineID) {
        case 0: return std::make_unique<SimpleNoneEngine>();

        // Dynamics (1-6)
        case 1: return std::make_unique<OptoCompressor>();
        case 2: return std::make_unique<VCACompressor>();
        case 3: return std::make_unique<TransientShaper>();
        case 4: return std::make_unique<NoiseGate>();
        case 5: return std::make_unique<MasteringLimiter>();
        case 6: return std::make_unique<DynamicEQ>();

        // Filters/EQ (7-14)
        case 7: return std::make_unique<ParametricEQ>();
        case 8: return std::make_unique<VintageConsoleEQ>();
        case 9: return std::make_unique<LadderFilter>();
        case 10: return std::make_unique<StateVariableFilter>();
        case 11: return std::make_unique<FormantFilter>();
        case 12: return std::make_unique<EnvelopeFilter>();
        case 13: return std::make_unique<CombResonator>();
        case 14: return std::make_unique<VocalFormant>();

        // Modulation (24-31) - excluding 23
        case 24: return std::make_unique<ResonantChorus>();
        case 25: return std::make_unique<AnalogPhaser>();
        case 26: return std::make_unique<RingModulator>();
        case 27: return std::make_unique<FrequencyShifter>();
        case 28: return std::make_unique<HarmonicTremolo>();
        case 29: return std::make_unique<ClassicTremolo>();
        case 30: return std::make_unique<RotarySpeaker>();
        case 31: return std::make_unique<PitchShifter>();

        // Delays (34-38)
        case 34: return std::make_unique<TapeEcho>();
        case 35: return std::make_unique<DigitalDelay>();
        case 36: return std::make_unique<MagneticDrumEcho>();
        case 37: return std::make_unique<BucketBrigadeDelay>();
        case 38: return std::make_unique<BufferRepeat>();

        // Reverbs (42-43)
        case 42: return std::make_unique<ShimmerReverb>();
        case 43: return std::make_unique<GatedReverb>();

        // Spectral (46-48)
        case 46: return std::make_unique<DimensionExpander>();
        case 47: return std::make_unique<SpectralFreeze>();
        case 48: return std::make_unique<SpectralGate>();

        // Special (50-52)
        case 50: return std::make_unique<GranularCloud>();
        case 51: return std::make_unique<ChaosGenerator>();
        case 52: return std::make_unique<FeedbackNetwork>();

        default: return nullptr;
    }
}

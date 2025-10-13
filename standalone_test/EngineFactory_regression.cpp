#include <memory>
#include <JuceHeader.h>

// Forward declarations
class DSPEngine;

// Engine forward declarations
class StereoChorus;
class ResonantChorus;
class FrequencyShifter;
class HarmonicTremolo;
class PlateReverb;
class ShimmerReverb;
class ConvolutionReverb;
class SpringReverb;
class GatedReverb;
class PhasedVocoder;

std::unique_ptr<DSPEngine> createEngine(int engineID, int sampleRate) {
    switch (engineID) {
        case 23: return std::make_unique<StereoChorus>();
        case 24: return std::make_unique<ResonantChorus>();
        case 27: return std::make_unique<FrequencyShifter>();
        case 28: return std::make_unique<HarmonicTremolo>();
        case 39: return std::make_unique<PlateReverb>();
        case 40: return std::make_unique<ShimmerReverb>();
        case 41: return std::make_unique<ConvolutionReverb>();
        case 42: return std::make_unique<SpringReverb>();
        case 43: return std::make_unique<GatedReverb>();
        case 49: return std::make_unique<PhasedVocoder>();
        default: return nullptr;
    }
}

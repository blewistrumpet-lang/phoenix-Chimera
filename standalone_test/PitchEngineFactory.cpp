#include "PitchEngineFactory.h"
#include "../JUCE_Plugin/Source/PitchShifter.h"
#include "../JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "../JUCE_Plugin/Source/TapeEcho.h"
#include "../JUCE_Plugin/Source/DigitalDelay.h"
#include "../JUCE_Plugin/Source/MagneticDrumEcho.h"
#include "../JUCE_Plugin/Source/BucketBrigadeDelay.h"
#include "../JUCE_Plugin/Source/GranularCloud.h"

std::unique_ptr<EngineBase> EngineFactory::createEngine(int engineID) {
    switch (engineID) {
        case 32:
            return std::make_unique<PitchShifter>();
        case 33:
            return std::make_unique<IntelligentHarmonizer>();
        case 34:
            return std::make_unique<TapeEcho>();
        case 35:
            return std::make_unique<AudioDSP::DigitalDelay>();
        case 36:
            return std::make_unique<MagneticDrumEcho>();
        case 37:
            return std::make_unique<BucketBrigadeDelay>();
        case 38:
            // BufferRepeat - we'll map to PitchShifter for now
            return std::make_unique<PitchShifter>();
        case 49:
            // Pitch Shifter Alt
            return std::make_unique<PitchShifter>();
        case 50:
            return std::make_unique<GranularCloud>();
        default:
            return nullptr;
    }
}

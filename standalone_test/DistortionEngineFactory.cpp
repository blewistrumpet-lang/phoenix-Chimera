#include "DistortionEngineFactory.h"

// Include distortion engines 15-22
#include "../JUCE_Plugin/Source/VintageTubePreamp_Studio.h"    // Engine 15
#include "../JUCE_Plugin/Source/WaveFolder.h"                   // Engine 16
#include "../JUCE_Plugin/Source/HarmonicExciter_Platinum.h"    // Engine 17
#include "../JUCE_Plugin/Source/BitCrusher.h"                   // Engine 18
#include "../JUCE_Plugin/Source/MultibandSaturator.h"           // Engine 19
#include "../JUCE_Plugin/Source/MuffFuzz.h"                     // Engine 20
#include "../JUCE_Plugin/Source/RodentDistortion.h"             // Engine 21
#include "../JUCE_Plugin/Source/KStyleOverdrive.h"              // Engine 22

std::unique_ptr<EngineBase> DistortionEngineFactory::createEngine(int engineID) {
    switch (engineID) {
        case 15: return std::make_unique<VintageTubePreamp_Studio>();
        case 16: return std::make_unique<WaveFolder>();
        case 17: return std::make_unique<HarmonicExciter_Platinum>();
        case 18: return std::make_unique<BitCrusher>();
        case 19: return std::make_unique<MultibandSaturator>();
        case 20: return std::make_unique<MuffFuzz>();
        case 21: return std::make_unique<RodentDistortion>();
        case 22: return std::make_unique<KStyleOverdrive>();
        default: return nullptr;
    }
}

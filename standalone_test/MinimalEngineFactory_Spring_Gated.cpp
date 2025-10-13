// Minimal EngineFactory for SpringReverb and GatedReverb testing
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/SpringReverb.h"
#include "../JUCE_Plugin/Source/GatedReverb.h"
#include <memory>

std::unique_ptr<EngineBase> EngineFactory::createEngine(int engineId) {
    switch (engineId) {
        case 42: return std::make_unique<SpringReverb>();
        case 43: return std::make_unique<GatedReverb>();
        default: return nullptr;
    }
}

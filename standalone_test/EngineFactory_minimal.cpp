// Minimal EngineFactory for testing Engine 32 (DetuneDoubler) only
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/DetuneDoubler.h"
#include <memory>

std::unique_ptr<EngineBase> EngineFactory::createEngine(int engineId) {
    if (engineId == 32) {
        return std::make_unique<AudioDSP::DetuneDoubler>();
    }
    return nullptr;
}

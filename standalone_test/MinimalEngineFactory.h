#pragma once

#include <memory>
#include "../JUCE_Plugin/Source/EngineBase.h"

// Minimal factory for testing engines 0-5 only
class MinimalEngineFactory {
public:
    static std::unique_ptr<EngineBase> createEngine(int engineID);
};

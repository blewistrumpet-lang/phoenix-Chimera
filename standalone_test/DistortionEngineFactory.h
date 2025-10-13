#pragma once

#include "../JUCE_Plugin/Source/EngineBase.h"
#include <memory>

class DistortionEngineFactory {
public:
    static std::unique_ptr<EngineBase> createEngine(int engineID);
};

/**
 * Minimal Engine Factory for Spectral Effects Testing
 */

#pragma once

#include "../pi_deployment/JUCE_Plugin/Source/EngineBase.h"
#include <memory>

class SpectralEngineFactory {
public:
    static std::unique_ptr<EngineBase> createEngine(int engineID);
};

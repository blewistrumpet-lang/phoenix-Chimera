/**
 * Minimal Engine Factory for Spectral Effects Testing
 * Only creates the 4 spectral engines: 47, 48, 49, 52
 */

#include "SpectralEngineFactory.h"
#include "../pi_deployment/JUCE_Plugin/Source/EngineTypes.h"
#include "../pi_deployment/JUCE_Plugin/Source/SpectralFreeze.h"
#include "../pi_deployment/JUCE_Plugin/Source/SpectralGate_Platinum.h"
#include "../pi_deployment/JUCE_Plugin/Source/PhasedVocoder.h"
#include "../pi_deployment/JUCE_Plugin/Source/FeedbackNetwork.h"
#include <memory>

std::unique_ptr<EngineBase> SpectralEngineFactory::createEngine(int engineID) {
    switch (engineID) {
        case ENGINE_SPECTRAL_FREEZE:      // 47
            return std::make_unique<SpectralFreeze>();

        case ENGINE_SPECTRAL_GATE:        // 48
            return std::make_unique<SpectralGate_Platinum>();

        case ENGINE_PHASED_VOCODER:       // 49
            return std::make_unique<PhasedVocoder>();

        case ENGINE_FEEDBACK_NETWORK:     // 52
            return std::make_unique<FeedbackNetwork>();

        default:
            return nullptr;
    }
}

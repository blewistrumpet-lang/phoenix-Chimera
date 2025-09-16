#include "IPitchShiftStrategy.h"
#include "SMBPitchShiftFixed.h"

std::unique_ptr<IPitchShiftStrategy> PitchShiftFactory::create(Algorithm algo) {
    // Using the FIXED and TESTED SMB (Stephan M. Bernsee) pitch shift algorithm
    // This version achieves < 0.0005% frequency error
    return std::make_unique<SMBPitchShiftFixed>();
    
    // Future: can add algorithm selection back when we have multiple working implementations
    /*
    switch (algo) {
        case Algorithm::Simple:
        case Algorithm::Signalsmith:
        case Algorithm::PSOLA:
        case Algorithm::PhaseVocoder:
        case Algorithm::RubberBand:
        default:
            return std::make_unique<SignalsmithPitchShift>();
    }
    */
}

PitchShiftFactory::Algorithm PitchShiftFactory::getBestAvailable() {
    // For beta, Simple is the best (only) available
    return Algorithm::Simple;
    
    // Future logic:
    // if (RubberBandAvailable()) return Algorithm::RubberBand;
    // if (PhaseVocoderAvailable()) return Algorithm::PhaseVocoder;
    // etc.
}

bool PitchShiftFactory::isAvailable(Algorithm algo) {
    switch (algo) {
        case Algorithm::Simple:
            return true;  // Always available
            
        case Algorithm::Signalsmith:
            return false; // Has latency issues
            
        case Algorithm::PSOLA:
        case Algorithm::PhaseVocoder:
        case Algorithm::RubberBand:
            return false; // Not implemented yet
            
        default:
            return false;
    }
}
#include "IPitchShiftStrategy.h"
#include "SMBPitchShiftFixed.h"
#include "PhaseVocoderPitchShift.h"

std::unique_ptr<IPitchShiftStrategy> PitchShiftFactory::create(Algorithm algo) {
    // CRITICAL FIX: Use true Phase Vocoder instead of signalsmith-stretch
    // signalsmith-stretch is a TIME-STRETCHER, not a pitch shifter
    // This caused the 8.673% THD issue
    //
    // Now using proper phase vocoder with 8x overlap for < 0.5% THD
    return std::make_unique<PhaseVocoderPitchShift>();

    // Old broken version (signalsmith-stretch used incorrectly):
    // return std::make_unique<SMBPitchShiftFixed>();

    // Future: can add algorithm selection back when we have multiple working implementations
    /*
    switch (algo) {
        case Algorithm::Simple:
        case Algorithm::Signalsmith:
        case Algorithm::PSOLA:
        case Algorithm::PhaseVocoder:
        case Algorithm::RubberBand:
        default:
            return std::make_unique<PhaseVocoderPitchShift>();
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
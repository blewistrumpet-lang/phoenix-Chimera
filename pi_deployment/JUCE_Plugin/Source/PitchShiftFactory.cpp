#include "IPitchShiftStrategy.h"
#include "SMBPitchShiftFixed.h"
#include <cstring>
#include <cmath>

// Wrapper to adapt SMBPitchShiftFixed to IPitchShiftStrategy interface
class SignalsmithStrategyAdapter : public IPitchShiftStrategy {
private:
    SMBPitchShiftFixed shifter;
    float currentPitchRatio = 1.0f;
    
public:
    void prepare(double sampleRate, int maxBlockSize) override {
        shifter.prepare(sampleRate, maxBlockSize);
    }
    
    void reset() override {
        shifter.reset();
        currentPitchRatio = 1.0f;
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) override {
        if (std::abs(pitchRatio - currentPitchRatio) > 0.001f) {
            currentPitchRatio = pitchRatio;
            // Convert ratio to semitones
            float semitones = 12.0f * std::log2(pitchRatio);
            shifter.setPitchShift(semitones);
        }
        shifter.process(input, output, numSamples);
    }
    
    int getLatencySamples() const override { 
        return shifter.getLatencySamples();
    }
    
    const char* getName() const override { return "Signalsmith"; }
    bool isHighQuality() const override { return true; }
    int getQualityRating() const override { return 85; }
    int getCpuUsage() const override { return 40; }
};

std::unique_ptr<IPitchShiftStrategy> PitchShiftFactory::create(Algorithm algo) {
    // Use Signalsmith for all pitch shifting
    return std::make_unique<SignalsmithStrategyAdapter>();
    
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
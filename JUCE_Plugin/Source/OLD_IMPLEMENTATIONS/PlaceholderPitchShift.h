#pragma once
#include "IPitchShiftStrategy.h"

/**
 * PlaceholderPitchShift - Temporary placeholder for beta release
 * 
 * Simply passes audio through unchanged.
 * Real pitch shifting will be added post-beta with proper library integration.
 * 
 * Affected engines in beta:
 * - PitchShifter (Vocal Destroyer) - Gender effect won't change pitch
 * - DetuneDoubler - No detuning effect
 * - IntelligentHarmonizer - No harmony generation
 * - FrequencyShifter - No frequency shifting
 * - ShimmerReverb - No shimmer effect
 */
class PlaceholderPitchShift : public IPitchShiftStrategy {
public:
    void reset() override {}
    
    void prepare(double sampleRate, int maxBlockSize) override {
        // Nothing to prepare
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) override {
        // For beta: just pass through
        // TODO: Integrate RubberBand or similar library post-beta
        for (int i = 0; i < numSamples; ++i) {
            output[i] = input[i];
        }
    }
    
    // IPitchShiftStrategy interface
    int getLatencySamples() const override { return 0; }
    const char* getName() const override { return "Placeholder (Beta)"; }
    bool isHighQuality() const override { return false; }
    int getQualityRating() const override { return 0; }
    int getCpuUsage() const override { return 0; }
};
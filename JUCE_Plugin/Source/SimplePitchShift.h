#pragma once
#include "IPitchShiftStrategy.h"

class SimplePitchShift : public IPitchShiftStrategy {
public:
    SimplePitchShift() = default;
    ~SimplePitchShift() = default;
    
    void prepare(double sampleRate, int maxBlockSize) override;
    void reset() override;
    void process(const float* input, float* output, int numSamples, float pitchRatio) override;
    int getLatencySamples() const override { return 0; }
    const char* getName() const override { return "Simple (Beta)"; }
    bool isHighQuality() const override { return false; }
    int getQualityRating() const override { return 30; }
    int getCpuUsage() const override { return 5; }
    
private:
    double currentSampleRate = 44100.0;
};
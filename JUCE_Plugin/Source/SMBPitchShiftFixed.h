#pragma once
#include "IPitchShiftStrategy.h"
#include <memory>

// SMB pitch shifter using signalsmith-stretch library
// Uses PIMPL pattern to isolate signalsmith compilation
class SMBPitchShiftFixed : public IPitchShiftStrategy {
private:
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    SMBPitchShiftFixed();
    ~SMBPitchShiftFixed() override;

    void prepare(double sr, int maxBlockSize) override;
    void reset() override;
    void setPitchShift(float semitones);
    void process(float* buffer, int numSamples);
    void process(const float* input, float* output, int numSamples);

    void process(const float* input, float* output, int numSamples, float pitchRatio) override;


    int getLatencySamples() const override;

    const char* getName() const override {
        return "SMB Pitch Shift (Fixed)";
    }

    bool isHighQuality() const override {
        return true;
    }

    int getQualityRating() const override {
        return 80;
    }

    int getCpuUsage() const override {
        return 40;
    }
};
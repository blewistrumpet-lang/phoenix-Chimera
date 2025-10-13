#pragma once
#include <memory>

// SMB pitch shifter using signalsmith-stretch library
// Uses PIMPL pattern to isolate signalsmith compilation
class SMBPitchShiftFixed {
private:
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    SMBPitchShiftFixed();
    ~SMBPitchShiftFixed();

    void prepare(double sr, int maxBlockSize);
    void reset();
    void setPitchShift(float semitones);
    void process(float* buffer, int numSamples);
    void process(const float* input, float* output, int numSamples);
    void process(const float* input, float* output, int numSamples, float pitchRatio);

    int getLatencySamples() const;
};
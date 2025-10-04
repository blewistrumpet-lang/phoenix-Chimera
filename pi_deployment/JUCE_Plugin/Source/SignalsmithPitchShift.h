#pragma once
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>

// Forward declaration to avoid including the problematic header here
namespace signalsmith {
    namespace stretch {
        template<typename Sample> class SignalsmithStretch;
    }
}

class SignalsmithPitchShift {
private:
    // Use pimpl idiom to isolate the signalsmith includes
    class Impl;
    std::unique_ptr<Impl> pimpl;
    
public:
    SignalsmithPitchShift();
    ~SignalsmithPitchShift();
    
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void setPitchShift(float semitones);
    void process(float* buffer, int numSamples);
    void process(const float* input, float* output, int numSamples);
    float getLatencySamples() const;
};
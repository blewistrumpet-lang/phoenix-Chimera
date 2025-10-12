#include "SignalsmithPitchShift.h"
#include "signalsmith-stretch.h"
#include <iostream>

class SignalsmithPitchShift::Impl {
public:
    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    std::vector<float> interleavedInput;
    std::vector<float> interleavedOutput;
    std::vector<float*> inputPointers;
    std::vector<float*> outputPointers;
    float currentPitchRatio = 1.0f;
    double sampleRate = 44100.0;
    int blockSize = 512;
    int channels = 1;  // Mono for simplicity
    
    Impl() {
        // Initialize with mono
        inputPointers.resize(1);
        outputPointers.resize(1);
    }
    
    void prepare(double sr, int maxBlockSize) {
        sampleRate = sr;
        blockSize = maxBlockSize;
        
        // Configure stretcher for mono operation
        stretcher.configure(1, blockSize, sampleRate);
        stretcher.setTransposeFactor(1.0f);
        
        // Allocate buffers
        interleavedInput.resize(maxBlockSize);
        interleavedOutput.resize(maxBlockSize);
    }
    
    void reset() {
        stretcher.reset();
        currentPitchRatio = 1.0f;
        std::fill(interleavedInput.begin(), interleavedInput.end(), 0.0f);
        std::fill(interleavedOutput.begin(), interleavedOutput.end(), 0.0f);
    }
    
    void setPitchShift(float semitones) {
        currentPitchRatio = std::pow(2.0f, semitones / 12.0f);
        stretcher.setTransposeFactor(currentPitchRatio);
    }
    
    void process(const float* input, float* output, int numSamples) {
        if (std::abs(currentPitchRatio - 1.0f) < 0.001f) {
            // No pitch shift - just copy
            if (input != output) {
                std::copy(input, input + numSamples, output);
            }
            return;
        }
        
        // Copy input to buffer
        std::copy(input, input + numSamples, interleavedInput.data());
        
        // Set up pointers for signalsmith (it expects array of channel pointers)
        inputPointers[0] = interleavedInput.data();
        outputPointers[0] = interleavedOutput.data();
        
        // Process with correct channel format
        // Using data() ensures we have proper pointer arrays
        stretcher.process(inputPointers.data(), numSamples, 
                         outputPointers.data(), numSamples);
        
        // Copy output back
        std::copy(interleavedOutput.data(), interleavedOutput.data() + numSamples, output);
    }
    
    float getLatencySamples() const {
        return stretcher.inputLatency() + stretcher.outputLatency();
    }
};

// Public interface implementation
SignalsmithPitchShift::SignalsmithPitchShift() 
    : pimpl(std::make_unique<Impl>()) {
}

SignalsmithPitchShift::~SignalsmithPitchShift() = default;

void SignalsmithPitchShift::prepare(double sampleRate, int maxBlockSize) {
    pimpl->prepare(sampleRate, maxBlockSize);
}

void SignalsmithPitchShift::reset() {
    pimpl->reset();
}

void SignalsmithPitchShift::setPitchShift(float semitones) {
    pimpl->setPitchShift(semitones);
}

void SignalsmithPitchShift::process(float* buffer, int numSamples) {
    pimpl->process(buffer, buffer, numSamples);
}

void SignalsmithPitchShift::process(const float* input, float* output, int numSamples) {
    pimpl->process(input, output, numSamples);
}

float SignalsmithPitchShift::getLatencySamples() const {
    return pimpl->getLatencySamples();
}